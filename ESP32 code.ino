#include <PZEM004Tv30.h>
#include <Wire.h>
#include <DHT.h>
#include <BH1750.h>
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

const uint32_t SAMPLE_INTERVAL_MS = 1000;
unsigned long lastErrorSwitch = 0;
int currentErrorIndex = 0;
unsigned long lastSendTime = 0;

float BATTERY_CAPACITY_mAh;
unsigned long lastSampleTime = 0;
float remaining_mAh;

String ssid = "";
String password = "";
String serverName = "";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
PZEM004Tv30 pzem(Serial2, 16, 17);
Adafruit_INA219 solar(0x40);
Adafruit_INA219 battery(0x41);
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

float frequency = 0.0, powerFactor = 0.0;
float voltage = 0.0, current = 0.0;
float power = 0.0, energy = 0.0;
float temperature = 0.0;
float solarVoltage = 0.0, solarCurrent = 0.0;
float solarPower = 0.0, batteryPercentage = 0.0;
float lightIntensity = 0.0, batteryVoltage = 0.0;

// Error flags (bool instead of String)
bool E_light = false, E_solar = false, E_battery = false, Epem = false, Etem = false, Ewifi = false, Ehttp = false, Esdcard = false;

String fetch(String File_name) {
  String path = String("/ESP32/") + File_name + ".txt";
  File file = SD.open(path);
  if (!file) return "";
  String data = file.readStringUntil('\n');
  data.trim();
  file.close();
  return data;
}

void displayErrors() {
  // Collect error labels
  const char *errors[8];
  int count = 0;

  if (E_light) errors[count++] = "Light";
  if (E_solar) errors[count++] = "Solar";
  if (E_battery) errors[count++] = "Battery";
  if (Epem) errors[count++] = "PZEM";
  if (Etem) errors[count++] = "DHT";
  if (Ewifi) errors[count++] = "WiFi";
  if (Ehttp) errors[count++] = "HTTP";
  if (Esdcard) errors[count++] = "SDcard";

  unsigned long now = millis();

  if (count > 0) {
    if (now - lastErrorSwitch >= 2000) {  // rotate slower (2s)
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
  // ---- Serial Debug Output ----
  Serial.print("V:");
  Serial.print(voltage);
  Serial.print(" I:");
  Serial.print(current);
  Serial.print(" P:");
  Serial.print(power);
  Serial.print(" E:");
  Serial.print(energy);
  Serial.print(" PF:");
  Serial.print(powerFactor);
  Serial.print(" F:");
  Serial.print(frequency);

  Serial.print(" | SV:");
  Serial.print(solarVoltage);
  Serial.print(" SI:");
  Serial.print(solarCurrent);
  Serial.print(" SP:");
  Serial.print(solarPower);

  Serial.print(" | BV:");
  Serial.print(batteryVoltage);
  Serial.print(" SoC%: ");
  Serial.print(batteryPercentage);

  Serial.print(" | T:");
  Serial.print(temperature);

  Serial.print(" | Lux:");
  Serial.print(lightIntensity);

  Serial.print(" | Errs -> L:");
  Serial.print(E_light);
  Serial.print(" S:");
  Serial.print(E_solar);
  Serial.print(" B:");
  Serial.print(E_battery);
  Serial.print(" P:");
  Serial.print(Epem);
  Serial.print(" D:");
  Serial.print(Etem);
  Serial.println();
  // ------------------------------
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
      doc["SolarVoltage"] = solarVoltage;
      doc["solarCurrent"] = solarCurrent;
      doc["solarPower"] = solarPower;
      doc["batteryPercentage"] = batteryPercentage;
      doc["lightIntensity"] = lightIntensity;
      doc["batteryVoltage"] = batteryVoltage;

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
    } else Ewifi = true;
  }
}

float voltageToSoC(float v) {
  if (v >= 4.20) return 100.0;
  if (v >= 4.10) return 95.0;
  if (v >= 4.00) return 90.0;
  if (v >= 3.90) return 80.0;
  if (v >= 3.80) return 70.0;
  if (v >= 3.70) return 60.0;
  if (v >= 3.60) return 45.0;
  if (v >= 3.50) return 30.0;
  if (v >= 3.40) return 15.0;
  if (v >= 3.30) return 5.0;
  return 0.0;
}

void readPZEM() {
  bool ok = true;
  float v = pzem.voltage();
  if (!isnan(v)) voltage = v;
  else ok = false;
  float c = pzem.current();
  if (!isnan(c)) current = c;
  else ok = false;
  float e = pzem.energy();
  if (!isnan(e)) energy = e;
  else ok = false;
  float p = pzem.power();
  if (!isnan(p)) power = p;
  else ok = false;
  float pf = pzem.pf();
  if (!isnan(pf)) powerFactor = pf;
  else ok = false;
  float f = pzem.frequency();
  if (!isnan(f)) frequency = f;
  else ok = false;
  Epem = !ok;
}

void readSolar() {
  solarVoltage = solar.getBusVoltage_V();
  solarCurrent = solar.getCurrent_mA();
  solarPower = solar.getPower_mW();
}

void readBattery() {
  unsigned long now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    float current_mA = battery.getCurrent_mA();
    batteryVoltage = battery.getBusVoltage_V();

    float dt_s = (now - lastSampleTime) / 1000.0f;
    lastSampleTime = now;

    float delta_mAh = current_mA * (dt_s / 3600.0f);
    remaining_mAh -= delta_mAh;

    if (remaining_mAh < 0) remaining_mAh = 0;
    if (remaining_mAh > BATTERY_CAPACITY_mAh) remaining_mAh = BATTERY_CAPACITY_mAh;

    batteryPercentage = (remaining_mAh / BATTERY_CAPACITY_mAh) * 100.0f;

    static unsigned long lastCorrection = 0;
    if (now - lastCorrection >= 5UL * 60UL * 1000UL || fabs(current_mA) < 20.0f) {
      float v_soc = voltageToSoC(batteryVoltage);
      float voltage_est_mAh = (v_soc / 100.0f) * BATTERY_CAPACITY_mAh;
      remaining_mAh = remaining_mAh * 0.90f + voltage_est_mAh * 0.10f;
      lastCorrection = now;
    }
  }
}

void readDHT() {
  bool ok = true;
  float t = dht.readTemperature();
  if (!isnan(t)) temperature = t;
  else ok = false;
  Etem = !ok;
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

void checkWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Ewifi = true;
    WiFi.begin(ssid.c_str(), password.c_str());
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 5) {
      delay(500);
      retry++;
    }
    Ewifi = (WiFi.status() != WL_CONNECTED);
  } else {
    Ewifi = false;
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  dht.begin();
  if (!SD.begin(SD_CS)) Esdcard = true;
  else {
    ssid = fetch("ssid");
    password = fetch("password");
    serverName = fetch("server");
    BATTERY_CAPACITY_mAh = fetch("battery_capacity").toFloat();
    remaining_mAh = BATTERY_CAPACITY_mAh * 0.5;

    WiFi.begin(ssid.c_str(), password.c_str());
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 10) {
      delay(500);
      retry++;
    }
  }

  Ewifi = (WiFi.status() == WL_CONNECTED) ? false : true;

  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    E_light = true;
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true) { delay(100); }
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (!solar.begin()) E_solar = true;
  else E_solar = false;
  if (!battery.begin()) E_battery = true;
  else E_battery = false;

  lastSampleTime = millis();
}

void loop() {
  readPZEM();
  readSolar();
  readBattery();
  readDHT();
  readLight();
  checkWiFi();
  dataSend();
  displayErrors();
  serialData();
}
