#include <PZEM004Tv30.h>
#include <Wire.h>
#include <DHT.h>
#include <BH1750.h>
#include <MAX17048.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>

#define DHTPIN 4
#define SD_CS 5
#define DHTTYPE DHT11
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

unsigned long lastErrorSwitch = 0;
int currentErrorIndex = 0;
unsigned long lastSendTime = 0;
unsigned long lastWiFiAttempt = 0;
bool wifiConnecting = false;

String ssid = "";
String password = "";
String serverName = "";
String Longitude = "";
String Latitude = "";
String Token = "";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
PZEM004Tv30 pzem(Serial2, 16, 17);
Adafruit_INA219 solar;
MAX17048 battery;
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

float frequency = 0.0, powerFactor = 0.0;
float voltage = 0.0, current = 0.0;
float power = 0.0, energy = 0.0;
float temperature = 0.0;
float solarVoltage = 0.0, solarCurrent = 0.0;
float solarPower = 0.0, batteryPercentage = 0.0;
float lightIntensity = 0.0, batteryVoltage = 0.0;

// Error flags
bool E_light = false, E_solar = false, E_battery = false, Epem = false, Etem = false, Ewifi = false, Ehttp = false, Esdcard = false;

String fetch(String File_name) {
  String path = String("/ESP32/") + File_name + ".txt";
  File file = SD.open(path);
  if (!file)
    return "";
  String data = file.readStringUntil('\n');
  data.trim();
  file.close();
  return data;
}

void displayErrors() {
  const char *errors[8];
  int count = 0;

  if (E_light)   errors[count++] = "Light";
  if (E_solar)   errors[count++] = "Solar";
  if (E_battery) errors[count++] = "Battery";
  if (Epem)      errors[count++] = "PZEM";
  if (Etem)      errors[count++] = "DHT";
  if (Ewifi)     errors[count++] = "WiFi";
  if (Ehttp)     errors[count++] = "HTTP";
  if (Esdcard)   errors[count++] = "SDcard";

  unsigned long now = millis();

  if (count > 0) {
    if (now - lastErrorSwitch >= 2000) {  
      currentErrorIndex = (currentErrorIndex + 1) % count;
      lastErrorSwitch = now;
    }
    display.clearDisplay();
    display.setCursor(0, 10);
    display.print("Err: ");
    display.print(errors[currentErrorIndex]);
    display.display();
  } else {
    display.clearDisplay();
    display.setCursor(0, 10);
    display.print("Err: None");
    display.display();
  }
}

void serialData() {
  Serial.print("V:"); Serial.print(voltage);
  Serial.print(" I:"); Serial.print(current);
  Serial.print(" P:"); Serial.print(power);
  Serial.print(" E:"); Serial.print(energy);
  Serial.print(" PF:"); Serial.print(powerFactor);
  Serial.print(" F:"); Serial.print(frequency);

  Serial.print(" | SV:"); Serial.print(solarVoltage);
  Serial.print(" SI:"); Serial.print(solarCurrent);
  Serial.print(" SP:"); Serial.print(solarPower);

  Serial.print(" | BV:"); Serial.print(batteryVoltage);
  Serial.print(" SoC%: "); Serial.print(batteryPercentage);

  Serial.print(" | T:"); Serial.print(temperature);
  Serial.print(" | Lux:"); Serial.print(lightIntensity);

  Serial.print(" | Errs -> L:"); Serial.print(E_light);
  Serial.print(" S:"); Serial.print(E_solar);
  Serial.print(" B:"); Serial.print(E_battery);
  Serial.print(" P:"); Serial.print(Epem);
  Serial.print(" D:"); Serial.print(Etem);
  Serial.println();
}

void dataSend() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastSendTime >= 30000) {
    lastSendTime = currentMillis;

    if (WiFi.status() == WL_CONNECTED) {
      Ewifi = false;
      StaticJsonDocument<512> doc;
      doc["BoxTemperature"] = temperature;
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

      String jsonData;
      serializeJson(doc, jsonData);
      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "application/json");
      http.setTimeout(7000);
      int httpResponseCode = http.POST(jsonData);

      if (httpResponseCode > 0) {
        Ehttp = false;
        Serial.print("Response Code: ");
        Serial.println(httpResponseCode);
        String response = http.getString();
        Serial.println("Server Response: " + response);
      } else {
        Ehttp = true;
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      http.end();
    } else {
      Ewifi = true;
    }
  }
}

void readPZEM() {
  bool ok = true;
  float v = pzem.voltage();
  if (!isnan(v)) voltage = v; else ok = false;
  float c = pzem.current();
  if (!isnan(c)) current = c; else ok = false;
  float e = pzem.energy();
  if (!isnan(e)) energy = e; else ok = false;
  float p = pzem.power();
  if (!isnan(p)) power = p; else ok = false;
  float pf = pzem.pf();
  if (!isnan(pf)) powerFactor = pf; else ok = false;
  float f = pzem.frequency();
  if (!isnan(f)) frequency = f; else ok = false;
  Epem = !ok;
}

void readSolar() {
  solarVoltage = solar.getBusVoltage_V();
  solarCurrent = solar.getCurrent_mA();
  solarPower   = solar.getPower_mW();
}

void readBattery() {
  float v = battery.getVoltage();
  float soc = battery.getSOC();
  if (!isnan(v) && !isnan(soc)) {
    batteryVoltage = v;
    batteryPercentage = soc;
    E_battery = false;
  } else {
    E_battery = true;
  }
}

void readDHT() {
  float t = dht.readTemperature();
  if (!isnan(t)) {
    temperature = t;
    Etem = false;
  } else {
    Etem = true;
  }
}

void readLight() {
  float lux = lightMeter.readLightLevel();
  if (!isnan(lux) && lux >= 0) {
    lightIntensity = lux;
    E_light = false;
  } else {
    E_light = true;
  }
}

void connectWiFiNonBlocking() {
  if (WiFi.status() == WL_CONNECTED) {
    Ewifi = false;
    wifiConnecting = false;
    return;
  }

  unsigned long now = millis();
  if (!wifiConnecting) {
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
  Ewifi = (WiFi.status() != WL_CONNECTED);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  dht.begin();

  if (!SD.begin(SD_CS)) {
    Esdcard = true;
    ssid = "OnePlus";
    password = "";
    serverName = "https://renewable-energy-monitoring-system-for.onrender.com/esp32-data";
  } else {
    ssid = fetch("ssid");
    password = fetch("password");
    serverName = fetch("server");
    Longitude = fetch("Longitude");
    Latitude = fetch("Latitude");
    Token = fetch("Token");
  }

  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) E_light = true;

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true) { delay(100); }
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (!solar.begin()) E_solar = true;
  if (!battery.begin()) E_battery = true;
}

void loop() {
  readPZEM();
  readSolar();
  readBattery();
  readDHT();
  readLight();
  connectWiFiNonBlocking();
  dataSend();
  displayErrors();
  serialData();
}
