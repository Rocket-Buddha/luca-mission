#include <WiFi.h>
#include <WebServer.h>

const char *ssid = "ESP32-Car-DHCP";

WebServer webServer(80);
int lastStationCount = -1;
unsigned long lastHeartbeatMs = 0;

void logMac(const uint8_t *mac) {
  Serial.printf(
    "%02X:%02X:%02X:%02X:%02X:%02X",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
  );
}

void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_AP_START:
      Serial.println("[AP] started");
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      Serial.println("[AP] stopped");
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      Serial.print("[AP] station connected mac=");
      logMac(info.wifi_ap_staconnected.mac);
      Serial.printf(" aid=%d\n", info.wifi_ap_staconnected.aid);
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      Serial.print("[AP] station disconnected mac=");
      logMac(info.wifi_ap_stadisconnected.mac);
      Serial.printf(" aid=%d\n", info.wifi_ap_stadisconnected.aid);
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      Serial.printf(
        "[AP] station got ip=%s\n",
        IPAddress(info.wifi_ap_staipassigned.ip.addr).toString().c_str()
      );
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("[BOOT] DHCP probe for ESP32-WROOM-32E");

  WiFi.persistent(false);
  WiFi.useStaticBuffers(true);
  WiFi.onEvent(onWiFiEvent);

  WiFi.mode(WIFI_OFF);
  delay(250);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  bool ap_ok = WiFi.softAP(ssid);
  Serial.printf("[AP] softAP status=%s\n", ap_ok ? "ok" : "error");
  Serial.printf("[AP] ssid=%s\n", ssid);
  Serial.printf("[AP] ip=%s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("[AP] mask=%s\n", WiFi.softAPSubnetMask().toString().c_str());

  webServer.on("/", []() {
    Serial.println("[HTTP] GET /");
    webServer.send(200, "text/plain", "dhcp probe ok");
  });

  webServer.on("/health", []() {
    Serial.println("[HTTP] GET /health");
    String body = "ssid=" + String(ssid);
    body += "\nip=" + WiFi.softAPIP().toString();
    body += "\nstations=" + String(WiFi.softAPgetStationNum());
    body += "\nheap=" + String(ESP.getFreeHeap());
    webServer.send(200, "text/plain", body);
  });

  webServer.onNotFound([]() {
    Serial.printf("[HTTP] 404 %s\n", webServer.uri().c_str());
    webServer.send(404, "text/plain", "not found");
  });

  webServer.begin();
  Serial.println("[HTTP] listening on :80");
}

void loop() {
  webServer.handleClient();

  int stationCount = WiFi.softAPgetStationNum();
  if (stationCount != lastStationCount) {
    Serial.printf("[AP] stations=%d\n", stationCount);
    lastStationCount = stationCount;
  }

  if (millis() - lastHeartbeatMs >= 2000) {
    lastHeartbeatMs = millis();
    Serial.printf(
      "[HEARTBEAT] stations=%d heap=%u ip=%s\n",
      stationCount,
      static_cast<unsigned>(ESP.getFreeHeap()),
      WiFi.softAPIP().toString().c_str()
    );
  }
}
