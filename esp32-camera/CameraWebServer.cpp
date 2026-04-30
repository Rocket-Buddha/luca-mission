#include "esp_system.h"
#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>

const char *kStaSsid = "Personal-F0C";
const char *kStaPassword = "00433457754";
const char *kRoverId = "uca-rover";

constexpr uint16_t kDiscoveryPort = 4210;
constexpr uint16_t kDiscoverySourcePort = 4211;
constexpr uint16_t kControlPort = 80;
constexpr uint16_t kStreamPort = 81;
constexpr uint16_t kTelemetryPort = 82;
constexpr unsigned long kConnectAttemptTimeoutMs = 15000;
constexpr unsigned long kRetryDelayMs = 1500;
constexpr uint8_t kAnnouncementBurstCount = 3;
constexpr unsigned long kAnnouncementBurstDelayMs = 120;

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void startCameraServer();
static bool cameraReady = false;

void appendTelemetryLine(const String &rawLine);
void appendTelemetryf(const char *format, ...);

static IPAddress broadcastAddressFor(const IPAddress &ip, const IPAddress &mask)
{
  return IPAddress(
    static_cast<uint8_t>(ip[0] | static_cast<uint8_t>(~mask[0])),
    static_cast<uint8_t>(ip[1] | static_cast<uint8_t>(~mask[1])),
    static_cast<uint8_t>(ip[2] | static_cast<uint8_t>(~mask[2])),
    static_cast<uint8_t>(ip[3] | static_cast<uint8_t>(~mask[3]))
  );
}

static void logConnectedStationSummary()
{
  String ip = WiFi.localIP().toString();
  String mask = WiFi.subnetMask().toString();
  String gateway = WiFi.gatewayIP().toString();
  String broadcast = broadcastAddressFor(WiFi.localIP(), WiFi.subnetMask()).toString();

  Serial.print("\r\n");
  Serial.printf("[WIFI] connected to %s\r\n", kStaSsid);
  Serial.printf("[WIFI] hostname: %s\r\n", kRoverId);
  Serial.printf("[WIFI] ip: %s\r\n", ip.c_str());
  Serial.printf("[WIFI] mask: %s\r\n", mask.c_str());
  Serial.printf("[WIFI] gateway: %s\r\n", gateway.c_str());
  Serial.printf("[WIFI] broadcast: %s\r\n", broadcast.c_str());
  Serial.print("STA Ready! control=http://");
  Serial.print(ip);
  Serial.print("/control stream=http://");
  Serial.print(ip);
  Serial.print(":81/stream telemetry=http://");
  Serial.print(ip);
  Serial.println(":82/telemetry");

  appendTelemetryf("[WIFI] connected ssid=%s ip=%s", kStaSsid, ip.c_str());
  appendTelemetryLine("[WIFI] STA ready");
}

static void connectToStation()
{
  WiFi.persistent(false);
  WiFi.useStaticBuffers(true);
  WiFi.mode(WIFI_OFF);
  delay(250);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(kRoverId);

  unsigned long attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    attempt++;
    Serial.printf("\r\n[WIFI] connecting to %s attempt=%lu\r\n", kStaSsid, attempt);
    appendTelemetryf("[WIFI] connect attempt %lu to %s", attempt, kStaSsid);

    WiFi.disconnect();
    delay(100);
    WiFi.begin(kStaSsid, kStaPassword);

    unsigned long startedAt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startedAt < kConnectAttemptTimeoutMs) {
      delay(250);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      break;
    }

    Serial.print("\r\n");
    Serial.printf("[WIFI] retrying after timeout, status=%d\r\n", static_cast<int>(WiFi.status()));
    appendTelemetryf("[WIFI] retrying, status=%d", static_cast<int>(WiFi.status()));
    delay(kRetryDelayMs);
  }

  logConnectedStationSummary();
}

static bool initCameraHardware()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // WROOM boards without PSRAM need a much smaller framebuffer to leave room for Wi-Fi.
      config.frame_size = FRAMESIZE_QVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
      config.jpeg_quality = 15;
    }
  } else {
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\r\n", err);
    return false;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (psramFound()) {
    s->set_framesize(s, FRAMESIZE_SVGA);
  } else {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }
  s->set_vflip(s, 1);
  s->set_hmirror(s, 0);
  cameraReady = true;
  Serial.printf(
    "Camera init ok, psram=%s, frame=%s\r\n",
    psramFound() ? "yes" : "no",
    psramFound() ? "SVGA" : "QVGA"
  );
  return true;
}

void CameraWebServer_init()
{
  connectToStation();
}

void CameraWebServer_announceDiscovery()
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[DISCOVERY] skipped, WiFi not connected");
    appendTelemetryLine("[DISCOVERY] skipped, no WiFi");
    return;
  }

  WiFiUDP udp;
  if (!udp.begin(kDiscoverySourcePort)) {
    Serial.printf("[DISCOVERY] failed to open UDP source port %u\r\n", kDiscoverySourcePort);
    appendTelemetryLine("[DISCOVERY] UDP open failed");
    return;
  }

  String ip = WiFi.localIP().toString();
  IPAddress subnetBroadcast = broadcastAddressFor(WiFi.localIP(), WiFi.subnetMask());
  const IPAddress globalBroadcast(255, 255, 255, 255);

  char payload[192];
  snprintf(
    payload,
    sizeof(payload),
    "{\"id\":\"%s\",\"ip\":\"%s\",\"ports\":{\"http\":%u,\"stream\":%u,\"telemetry\":%u}}",
    kRoverId,
    ip.c_str(),
    kControlPort,
    kStreamPort,
    kTelemetryPort
  );

  for (uint8_t burst = 0; burst < kAnnouncementBurstCount; burst++) {
    udp.beginPacket(subnetBroadcast, kDiscoveryPort);
    udp.print(payload);
    udp.endPacket();

    udp.beginPacket(globalBroadcast, kDiscoveryPort);
    udp.print(payload);
    udp.endPacket();

    delay(kAnnouncementBurstDelayMs);
  }

  udp.stop();

  Serial.printf(
    "[DISCOVERY] announced rover ip=%s via %s:%u\r\n",
    ip.c_str(),
    subnetBroadcast.toString().c_str(),
    kDiscoveryPort
  );
  appendTelemetryf("[DISCOVERY] announced ip=%s port=%u", ip.c_str(), kDiscoveryPort);
}

bool CameraWebServer_startServices()
{
  if (!cameraReady && !initCameraHardware()) {
    return false;
  }
  startCameraServer();
  return true;
}
