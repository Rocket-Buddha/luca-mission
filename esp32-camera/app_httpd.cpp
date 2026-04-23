#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include <Arduino.h>
#include <cstdlib>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "img_converters.h"

extern String telemetryLines[48];
extern uint32_t telemetryLineIds[48];
extern size_t telemetryNextIndex;
extern size_t telemetryCount;
extern uint32_t telemetryNextId;
extern SemaphoreHandle_t telemetryMutex;

#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#endif

namespace {

constexpr size_t kTelemetryCapacity = 48;
constexpr const char *kStreamBoundary = "123456789000000000000987654321";
constexpr const char *kStreamContentType = "multipart/x-mixed-replace;boundary=" "123456789000000000000987654321";
constexpr const char *kStreamBoundaryLine = "\r\n--123456789000000000000987654321\r\n";
constexpr const char *kStreamPartHeader = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %lld.%06ld\r\n\r\n";

httpd_handle_t cameraHttpd = nullptr;
httpd_handle_t telemetryHttpd = nullptr;

struct JpgChunkingContext {
    httpd_req_t *req;
    size_t length;
};

struct TelemetryEntry {
    uint32_t id;
    String line;
};

size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len)
{
    JpgChunkingContext *context = static_cast<JpgChunkingContext *>(arg);
    if (index == 0) {
        context->length = 0;
    }

    if (httpd_resp_send_chunk(context->req, static_cast<const char *>(data), len) != ESP_OK) {
        return 0;
    }

    context->length += len;
    return len;
}

uint32_t telemetry_oldest_id()
{
    if (telemetryCount == 0) {
        return telemetryNextId;
    }

    size_t start = (telemetryNextIndex + kTelemetryCapacity - telemetryCount) % kTelemetryCapacity;
    return telemetryLineIds[start];
}

size_t telemetry_copy_since(uint32_t lastSeenId, TelemetryEntry *entries, size_t maxEntries)
{
    size_t copied = 0;

    if (!telemetryMutex) {
        return 0;
    }

    if (xSemaphoreTake(telemetryMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return 0;
    }

    if (telemetryCount > 0) {
        uint32_t oldestId = telemetry_oldest_id();
        if (lastSeenId + 1 < oldestId) {
            lastSeenId = oldestId - 1;
        }

        size_t start = (telemetryNextIndex + kTelemetryCapacity - telemetryCount) % kTelemetryCapacity;
        for (size_t i = 0; i < telemetryCount && copied < maxEntries; ++i) {
            size_t index = (start + i) % kTelemetryCapacity;
            if (telemetryLineIds[index] <= lastSeenId) {
                continue;
            }

            entries[copied].id = telemetryLineIds[index];
            entries[copied].line = telemetryLines[index];
            copied++;
        }
    }

    xSemaphoreGive(telemetryMutex);
    return copied;
}

esp_err_t telemetry_send_line_event(httpd_req_t *req, uint32_t id, const String &line)
{
    String chunk;
    chunk.reserve(line.length() + 32);
    chunk += "id: ";
    chunk += String(id);
    chunk += "\n";
    chunk += "event: line\n";
    chunk += "data: ";
    chunk += line;
    chunk += "\n\n";
    return httpd_resp_send_chunk(req, chunk.c_str(), chunk.length());
}

esp_err_t telemetry_send_ping(httpd_req_t *req)
{
    static const char *pingChunk = "event: ping\ndata: alive\n\n";
    return httpd_resp_send_chunk(req, pingChunk, strlen(pingChunk));
}

uint32_t telemetry_last_event_id(httpd_req_t *req)
{
    size_t headerLength = httpd_req_get_hdr_value_len(req, "Last-Event-ID");
    if (headerLength == 0 || headerLength >= 32) {
        return 0;
    }

    char value[32];
    if (httpd_req_get_hdr_value_str(req, "Last-Event-ID", value, sizeof(value)) != ESP_OK) {
        return 0;
    }

    return static_cast<uint32_t>(strtoul(value, nullptr, 10));
}

esp_err_t telemetry_handler(httpd_req_t *req)
{
    esp_err_t result = httpd_resp_set_type(req, "text/event-stream");
    if (result != ESP_OK) {
        return result;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    httpd_resp_set_hdr(req, "X-Accel-Buffering", "no");

    TelemetryEntry pending[kTelemetryCapacity];
    uint32_t lastSeenId = telemetry_last_event_id(req);
    uint32_t lastPingMs = millis();

    while (true) {
        size_t pendingCount = telemetry_copy_since(lastSeenId, pending, kTelemetryCapacity);
        for (size_t i = 0; i < pendingCount; ++i) {
            result = telemetry_send_line_event(req, pending[i].id, pending[i].line);
            if (result != ESP_OK) {
                return result;
            }

            lastSeenId = pending[i].id;
            lastPingMs = millis();
        }

        if (millis() - lastPingMs >= 5000) {
            result = telemetry_send_ping(req);
            if (result != ESP_OK) {
                return result;
            }
            lastPingMs = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(120));
    }
}

esp_err_t capture_handler(httpd_req_t *req)
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        log_e("Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    esp_err_t result = httpd_resp_set_type(req, "image/jpeg");
    if (result == ESP_OK) {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Cache-Control", "no-store");
        httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");

        char timestamp[32];
        snprintf(
            timestamp,
            sizeof(timestamp),
            "%lld.%06ld",
            static_cast<long long>(fb->timestamp.tv_sec),
            static_cast<long>(fb->timestamp.tv_usec)
        );
        httpd_resp_set_hdr(req, "X-Timestamp", timestamp);
    }

    if (result != ESP_OK) {
        esp_camera_fb_return(fb);
        return result;
    }

    if (fb->format == PIXFORMAT_JPEG) {
        result = httpd_resp_send(req, reinterpret_cast<const char *>(fb->buf), fb->len);
    } else {
        JpgChunkingContext context{req, 0};
        result = frame2jpg_cb(fb, 80, jpg_encode_stream, &context) ? ESP_OK : ESP_FAIL;
        if (result == ESP_OK) {
            result = httpd_resp_send_chunk(req, nullptr, 0);
        } else {
            httpd_resp_send_500(req);
        }
    }

    esp_camera_fb_return(fb);
    return result;
}

esp_err_t stream_handler(httpd_req_t *req)
{
    esp_err_t result = httpd_resp_set_type(req, kStreamContentType);
    if (result != ESP_OK) {
        return result;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            log_e("Camera capture failed");
            return ESP_FAIL;
        }

        uint8_t *jpgBuffer = fb->buf;
        size_t jpgLength = fb->len;
        bool mustFreeJpg = false;

        if (fb->format != PIXFORMAT_JPEG) {
            if (!frame2jpg(fb, 80, &jpgBuffer, &jpgLength)) {
                esp_camera_fb_return(fb);
                log_e("JPEG compression failed");
                return ESP_FAIL;
            }
            mustFreeJpg = true;
        }

        char partHeader[128];
        size_t headerLength = snprintf(
            partHeader,
            sizeof(partHeader),
            kStreamPartHeader,
            static_cast<unsigned>(jpgLength),
            static_cast<long long>(fb->timestamp.tv_sec),
            static_cast<long>(fb->timestamp.tv_usec)
        );

        result = httpd_resp_send_chunk(req, kStreamBoundaryLine, strlen(kStreamBoundaryLine));
        if (result == ESP_OK) {
            result = httpd_resp_send_chunk(req, partHeader, headerLength);
        }
        if (result == ESP_OK) {
            result = httpd_resp_send_chunk(req, reinterpret_cast<const char *>(jpgBuffer), jpgLength);
        }

        if (mustFreeJpg) {
            free(jpgBuffer);
        }
        esp_camera_fb_return(fb);

        if (result != ESP_OK) {
            return result;
        }
    }
}

}  // namespace

void startCameraServer()
{
    httpd_config_t cameraConfig = HTTPD_DEFAULT_CONFIG();
    cameraConfig.server_port = 81;
    cameraConfig.max_uri_handlers = 4;

    httpd_uri_t captureUri = {
        .uri = "/capture",
        .method = HTTP_GET,
        .handler = capture_handler,
        .user_ctx = nullptr
#ifdef CONFIG_HTTPD_WS_SUPPORT
        ,
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr
#endif
    };

    httpd_uri_t streamUri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = nullptr
#ifdef CONFIG_HTTPD_WS_SUPPORT
        ,
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr
#endif
    };

    log_i("Starting camera server on port: '%d'", cameraConfig.server_port);
    if (httpd_start(&cameraHttpd, &cameraConfig) == ESP_OK) {
        httpd_register_uri_handler(cameraHttpd, &captureUri);
        httpd_register_uri_handler(cameraHttpd, &streamUri);
    }

    httpd_config_t telemetryConfig = HTTPD_DEFAULT_CONFIG();
    telemetryConfig.server_port = 82;
    telemetryConfig.ctrl_port = 32769;
    telemetryConfig.max_uri_handlers = 4;

    httpd_uri_t telemetryUri = {
        .uri = "/telemetry",
        .method = HTTP_GET,
        .handler = telemetry_handler,
        .user_ctx = nullptr
#ifdef CONFIG_HTTPD_WS_SUPPORT
        ,
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr
#endif
    };

    log_i("Starting telemetry server on port: '%d'", telemetryConfig.server_port);
    if (httpd_start(&telemetryHttpd, &telemetryConfig) == ESP_OK) {
        httpd_register_uri_handler(telemetryHttpd, &telemetryUri);
    }
}
