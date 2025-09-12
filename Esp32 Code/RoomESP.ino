#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <Wire.h>

String room = "true";
StaticJsonDocument<256> doc;

void onDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  DeserializationError error = deserializeJson(doc, data, len);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    return;
  }
  if (doc.containsKey("RoomESP")) {
    String a = doc["RoomESP"].as<String>();
    if (a == "true") digitalWrite(15, 1);
    else if (a == "false") digitalWrite(15, 0);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(15, OUTPUT);
  digitalWrite(15, 1);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    ;
    return;
  }
  esp_now_register_recv_cb(onDataRecv);
}

void loop(){}
