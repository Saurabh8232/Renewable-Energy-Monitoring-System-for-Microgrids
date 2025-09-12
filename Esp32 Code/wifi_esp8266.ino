#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

unsigned long lastSendTime = 0;
unsigned long lastWiFiAttempt = 0;
bool wifiConnecting = false;
WiFiClient client;

String ssid = "";
String password = "";
String serverName = "";
String Longitude = "";
String Latitude = "";
String Token = "";
String RoomEsp = "";

float solarVoltage = 0.0, solarCurrent = 0.0;
float solarPower = 0.0, batteryPercentage = 0.0;
float lightIntensity = 0.0, batteryVoltage = 0.0;
float frequency = 0.0, powerFactor = 0.0;
float voltage = 0.0, current = 0.0;
float power = 0.0, energy = 0.0;
int inverterLoad = 0;

String Ewifi = "false", Ehttp = "false";

void TxRxEsp32() {
  if (Serial.available()) {
    String jsonData = Serial.readStringUntil('\n');

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonData);

    if (!error) {
      doc["InverterLoad"] = inverterLoad;
      doc["Frequency"] = frequency;
      doc["PowerFactor"] = powerFactor;
      doc["Voltage"] = voltage;
      doc["Current"] = current;
      doc["Power"] = power;
      doc["Energy"] = energy;
      doc["solarVoltage"] = solarVoltage;
      doc["solarCurrent"] = solarCurrent;
      doc["solarPower"] = solarPower;
      doc["batteryPercentage"] = batteryPercentage;
      doc["batteryVoltage"] = batteryVoltage;
      doc["lightIntensity"] = lightIntensity;
      doc["latitude"] = Latitude;
      doc["longitude"] = Longitude;
      doc["THINGSBOARD_TOKEN"] = Token;
      doc["Server"] = serverName;
      doc["Ssid"] = ssid;
      doc["Password"] = password;
      doc["RoomEsp"] = RoomEsp;
    }
  }

  StaticJsonDocument<216> doc1;
  doc1["Ewifi"] = Ewifi;
  doc1["Ehttp"] = Ehttp;

  serializeJson(doc1, Serial);
  Serial.print('\n');
}

void connectWiFiNonBlocking() {
  if (WiFi.status() == WL_CONNECTED) {
    Ewifi = "false";
    wifiConnecting = false;
    return;
  }

  unsigned long now = millis();
  if (!wifiConnecting) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    wifiConnecting = true;
    lastWiFiAttempt = now;
  }

  if (now - lastWiFiAttempt >= 5000) {
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.disconnect();
      WiFi.begin(ssid.c_str(), password.c_str());
      lastWiFiAttempt = now;
    }
  }
  if (WiFi.status() != WL_CONNECTED) Ewifi = "true";
}

void httpPost() {

  unsigned long currentMillis = millis();

  if (currentMillis - lastSendTime >= 30000) {
    lastSendTime = currentMillis;

    if (WiFi.status() == WL_CONNECTED) {
      Ewifi = "false";
      StaticJsonDocument<512> doc;

      doc["InverterLoad"] = inverterLoad;
      doc["Frequency"] = frequency;
      doc["PowerFactor"] = powerFactor;
      doc["Voltage"] = voltage;
      doc["Current"] = current;
      doc["Power"] = power;
      doc["Energy"] = energy;
      doc["solarVoltage"] = solarVoltage;
      doc["solarCurrent"] = solarCurrent;
      doc["solarPower"] = solarPower;
      doc["batteryPercentage"] = batteryPercentage;
      doc["batteryVoltage"] = batteryVoltage;
      doc["lightIntensity"] = lightIntensity;
      doc["latitude"] = Latitude;
      doc["longitude"] = Longitude;
      doc["THINGSBOARD_TOKEN"] = Token;
      doc["deviceIP"] = WiFi.localIP().toString();
      doc["RoomEsp"] = RoomEsp;

      String jsonData;
      serializeJson(doc, jsonData);
      HTTPClient http;
      http.begin(client, serverName);
      http.addHeader("Content-Type", "application/json");
      http.setTimeout(7000);
      int postCode = http.POST(jsonData);
      Ehttp = (postCode > 0 || postCode == 200) ? "false" : "true";
      http.end();
    } else Ewifi = "true";
  }
}

void setup() {
  Serial.begin(115200);
}

void loop() {
  TxRxEsp32();
  connectWiFiNonBlocking();
  httpPost();
}
