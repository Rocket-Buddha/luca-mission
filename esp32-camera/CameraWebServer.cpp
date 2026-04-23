#include "esp_system.h"
#include "esp_camera.h"
#include <WiFi.h>

const char *ssid = "ESP32-Car";

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
  WiFi.persistent(false);
  WiFi.useStaticBuffers(true);
  WiFi.mode(WIFI_OFF);
  delay(250);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  bool ap_ok = WiFi.softAP(ssid);
  Serial.print("\r\n");
  Serial.printf("softAP status: %s\r\n", ap_ok ? "ok" : "error");
  Serial.printf("softAP SSID: %s\r\n", ssid);
  Serial.printf("softAP IP: %s\r\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("softAP mask: %s\r\n", WiFi.softAPSubnetMask().toString().c_str());
  Serial.printf("softAP security: open\r\n");
  Serial.print("\r\n");
  Serial.print("AP Ready! control=http://");
  Serial.print(WiFi.softAPIP());
  Serial.print("/control stream=http://");
  Serial.print(WiFi.softAPIP());
  Serial.print(":81/stream telemetry=http://");
  Serial.print(WiFi.softAPIP());
  Serial.println(":82/telemetry");
}

bool CameraWebServer_startServices()
{
  if (!cameraReady && !initCameraHardware()) {
    return false;
  }
  startCameraServer();
  return true;
}
