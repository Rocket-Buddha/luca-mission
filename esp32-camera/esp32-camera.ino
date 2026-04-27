#include <WiFi.h>
#include "esp_camera.h"
#include <WebServer.h>
#include <initializer_list>
#include <stdarg.h>
#include <freertos/semphr.h>

WebServer webServer(80);

bool servicesStarted = false;

constexpr size_t TELEMETRY_MAX_LINES = 48;
String telemetryLines[TELEMETRY_MAX_LINES];
uint32_t telemetryLineIds[TELEMETRY_MAX_LINES];
size_t telemetryNextIndex = 0;
size_t telemetryCount = 0;
uint32_t telemetryNextId = 1;
String serial2TelemetryBuffer;
SemaphoreHandle_t telemetryMutex = nullptr;

String formatTelemetryLine(const String &line) {
  unsigned long seconds = millis() / 1000;
  unsigned long tenths = (millis() % 1000) / 100;
  return "[" + String(seconds) + "." + String(tenths) + "s] " + line;
}

void appendTelemetryLine(const String &rawLine) {
  String line = rawLine;
  line.trim();

  if (!line.length()) {
    return;
  }

  if (telemetryMutex) {
    xSemaphoreTake(telemetryMutex, portMAX_DELAY);
  }

  telemetryLines[telemetryNextIndex] = formatTelemetryLine(line);
  telemetryLineIds[telemetryNextIndex] = telemetryNextId++;
  telemetryNextIndex = (telemetryNextIndex + 1) % TELEMETRY_MAX_LINES;

  if (telemetryCount < TELEMETRY_MAX_LINES) {
    telemetryCount++;
  }

  if (telemetryMutex) {
    xSemaphoreGive(telemetryMutex);
  }
}

void appendTelemetryf(const char *format, ...) {
  char message[192];
  va_list args;
  va_start(args, format);
  vsnprintf(message, sizeof(message), format, args);
  va_end(args);
  appendTelemetryLine(String(message));
}

void consumeSerial2TelemetryByte(uint8_t value) {
  if (value == '\r') {
    return;
  }

  if (value == '\n') {
    appendTelemetryLine(serial2TelemetryBuffer);
    serial2TelemetryBuffer = "";
    return;
  }

  if ((value >= 32 && value <= 126) || value == '\t') {
    if (serial2TelemetryBuffer.length() < 160) {
      serial2TelemetryBuffer += static_cast<char>(value);
    } else {
      appendTelemetryLine(serial2TelemetryBuffer);
      serial2TelemetryBuffer = static_cast<char>(value);
    }
  }
}

void drainSerial2Telemetry() {
  while (Serial2.available() > 0) {
    unsigned char serial2Buff = Serial2.read() & 0xff;
    consumeSerial2TelemetryByte(serial2Buff);
  }
}

#define gpLED 4  // LED GPIO pins
#define RXD2 14  // GPIO pin of RXD2 (Serial2 input)
#define TXD2 13  // GPIO pin of TXD2 (Serial2 output)
void CameraWebServer_init();
bool CameraWebServer_startServices();

void startControlServices() {
  if (servicesStarted) {
    return;
  }
  if (!CameraWebServer_startServices()) {
    Serial.println("[BOOT] camera/control services failed to start");
    appendTelemetryLine("[CAM] camera/control services failed to start");
    return;
  }
  webServer.begin();
  servicesStarted = true;
  Serial.println("[BOOT] camera/control services up");
  appendTelemetryLine("[CAM] camera/control services up");
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  telemetryMutex = xSemaphoreCreateMutex();
  CameraWebServer_init();  // Initialize camera web server
  Serial.println("[BOOT] WiFi AP up");
  appendTelemetryLine("[BOOT] WiFi AP up");
  delay(100);

  pinMode(gpLED, OUTPUT);  // Set gpLED pin as output
  for (int i = 0; i < 5; i++) {
    digitalWrite(gpLED, HIGH);  // Turn on LED
    delay(20);
    digitalWrite(gpLED, LOW);  // Turn off LED
    delay(50);
  }

  webServer.on("/control", []() {
    String cmd = webServer.arg("cmd");
    Serial.printf("[HTTP] /control cmd=%s\n", cmd.c_str());
    appendTelemetryf("[HTTP] /control cmd=%s", cmd.c_str());
    uint8_t data[13];
    size_t dataLen = 0;
    auto setPacket = [&](std::initializer_list<uint8_t> bytes) {
      dataLen = bytes.size();
      size_t index = 0;
      for (uint8_t value : bytes) {
        data[index++] = value;
      }
    };

    if (cmd == "car") {
      String direction = webServer.arg("direction");
      appendTelemetryf("[CAM] drive direction=%s", direction.c_str());
      int directionCode = -1;
      if (direction == "Forward") {
        directionCode = 0x01;
      }
      if (direction == "Backward") {
        directionCode = 0x02;
      }
      if (direction == "Left") {
        directionCode = 0x03;
      }
      if (direction == "Right") {
        directionCode = 0x04;
      }
      if (direction == "Anticlockwise") {
        directionCode = 0x09;
      }
      if (direction == "Clockwise") {
        directionCode = 0x0A;
      }
      if (direction == "stop") {
        directionCode = 0x00;
      }
      if (direction == "LeftUp") {
        directionCode = 0x05;
      }
      if (direction == "RightUp") {
        directionCode = 0x07;
      }
      if (direction == "LeftDown") {
        directionCode = 0x06;
      }
      if (direction == "RightDown") {
        directionCode = 0x08;
      }
      if (directionCode >= 0) {
        setPacket({ 0xFF, 0x55, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0C, 0x00, static_cast<uint8_t>(directionCode) });
      }
    }

    if (cmd == "speed") {
      String value = webServer.arg("value");
      appendTelemetryf("[CAM] speed=%s", value.c_str());
      int speedCode = -1;
      if (value == "1") { speedCode = 0x82; }
      if (value == "2") { speedCode = 0xA0; }
      if (value == "3") { speedCode = 0xBE; }
      if (value == "4") { speedCode = 0xDC; }
      if (value == "5") { speedCode = 0xFF; }
      if (speedCode >= 0) {
        setPacket({ 0xFF, 0x55, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0D, 0x00, static_cast<uint8_t>(speedCode) });
      }
    }

    if (cmd == "servo") {
      int angle = constrain(webServer.arg("angle").toInt(), 0, 180);
      appendTelemetryf("[CAM] servo=%d", angle);
      setPacket({ 0xFF, 0x55, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00, static_cast<uint8_t>(angle) });
    }

    if (cmd == "CAM_LED") {
      String value = webServer.arg("value");
      appendTelemetryf("[CAM] front light=%s", value == "1" ? "on" : "off");
      if (value == "1"){  // Turn on LED
        digitalWrite(gpLED, HIGH);  // Turn on LED
      } else if (value == "0"){  // Turn off LED
        digitalWrite(gpLED, LOW);  // Turn off LED
      }
    }

    if (cmd == "Buzzer") {
      String value = webServer.arg("value");
      appendTelemetryf("[CAM] buzzer=%s", value.c_str());
      int buzzerCode = -1;
      if (value == "horn_on") {
        buzzerCode = 0x05;
      }
      if (value == "melody_on") {
        buzzerCode = 0x06;
      }
      if (value == "melody_off") {
        buzzerCode = 0x07;
      }
      if (value == "horn_off") {
        buzzerCode = 0x00;
      }
      if (buzzerCode >= 0) {
        setPacket({ 0xFF, 0x55, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x00, static_cast<uint8_t>(buzzerCode) });
      }
    }

    if (cmd == "Track") {
      String value = webServer.arg("value");
      if (value.length() > 0) {
        appendTelemetryf("[CAM] track mode=3-sensor request=%s", value.c_str());
      } else {
        appendTelemetryLine("[CAM] track mode=3-sensor");
      }
      setPacket({ 0xFF, 0x55, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05 });
    }

    if (cmd == "Avoidance") {
      appendTelemetryLine("[CAM] avoidance mode");
      setPacket({ 0xFF, 0x55, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06 });
    }

    if (cmd == "Follow") {
      appendTelemetryLine("[CAM] follow mode");
      setPacket({ 0xFF, 0x55, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07 });
    }

    if (cmd == "stopA") {
      appendTelemetryLine("[CAM] standby mode");
      setPacket({ 0xFF, 0x55, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03 });
    }


    if (dataLen > 0) {
      Serial2.write(data, dataLen);
    }
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.sendHeader("Cache-Control", "no-store");
    webServer.send(200, "text/plain", "ok");
  });
  webServer.onNotFound([]() {
    Serial.printf("[HTTP] 404 %s\n", webServer.uri().c_str());
    appendTelemetryf("[HTTP] 404 %s", webServer.uri().c_str());
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(404, "text/plain", "not found");
  });

  startControlServices();
}

void loop() {
  drainSerial2Telemetry();
  if (servicesStarted) {
    webServer.handleClient();
  }
}
