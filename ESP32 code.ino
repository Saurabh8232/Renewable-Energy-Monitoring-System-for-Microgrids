#include <PZEM004Tv30.h>
#include <Wire.h>
#include <DHT.h>
#include <SPI.h>
#include <RTClib.h>
#include <SD.h>
#include <BH1750.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>

#define DHTPIN 4
#define SD_CS 5
#define DHTTYPE DHT11
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
const float BATTERY_CAPACITY_mAh = 2000.0;
const uint32_t SAMPLE_INTERVAL_MS = 1000;
unsigned long lastSampleTime = 0;
float remaining_mAh = BATTERY_CAPACITY_mAh * 0.5;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
PZEM004Tv30 pzem(Serial2, 16, 17);
Adafruit_INA219 solor(0x40);
Adafruit_INA219 battary(0x41);
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

float frequency = 0.0, PowerFector = 0.0;
float voltage = 0.0, current = 0.0;
float power = 0.0, energy = 0.0;
float temperature = 0.0, humidity = 0.0;
float solorVoltage = 0.0, solorCurrent = 0.0;
float solorPower = 0.0, battaryPercentage = 0.0;
float lightIntencity = 0.0, battaryVoltage = 0.0;

String Time;
String Date;
String Ebme = "0", Esolor = "0", Ebattary = "0" EsdCard = "0", rtcE = "0";

template<typename A>
void write(String File_name, A Data) {
  String path = String("/ESP32/") + File_name + ".txt";
  File file = SD.open(path, FILE_WRITE);
  if (file) {
    file.println(String(Data));
    file.close();
    writeE = "0";
  } else {
    writeE = "1";
  }
}

String fetch(String File_name) {
  String path = String("/ESP32/") + File_name + ".txt";
  File file = SD.open(path);
  if (!file) return "";
  String data = file.readStringUntil('\n');
  data.trim();
  file.close();
  return data;
}

void Rtc() {
  if (!rtcAvailable) return;

  DateTime now = rtc.now();
  int hour = now.hour();
  int currentDay = now.day();

  String ampm = (hour >= 12) ? "PM" : "AM";
  if (hour > 12) hour -= 12;
  if (hour == 0) hour = 12;

  Time = String(hour) + ":" + (now.minute() < 10 ? "0" : "") + String(now.minute()) + ":" + (now.second() < 10 ? "0" : "") + String(now.second()) + " " + ampm;
  Date = String(currentDay) + "-" + String(now.month()) + "-" + String(now.year());
}

void DataLogg() {
  String storedDate = fetch("Date");
  if (Date != storedDate) {
    File file = SD.open("Logged_Data.csv", FILE_APPEND);
    file.println();
    file.close();
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

void pzem004t() {
  if (!isnan(pzem.voltage())) voltage = pzem.voltage();
  if (!isnan(pzem.current())) current = pzem.current();
  if (!isnan(pzem.energy())) energy = pzem.energy();
  if (!isnan(pzem.power())) power = pzem.power();
  if (!isnan(pzem.pf())) PowerFector = pzem.pf();
  if (!isnan(pzem.frequency())) frequency = pzem.frequency();
}

void Solor() {
  solorVoltage = solor.getBusVoltage_V();
  solorCurrent = solor.getCurrent_mA();
  solorPower = solor.getPower_mW();
}

void Battary() {
  unsigned long now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    float current_mA = battary.getCurrent_mA();
    battaryVoltage = battary.getBusVoltage_V();

    float dt_s = (now - lastSampleTime) / 1000.0;
    lastSampleTime = now;
    float delta_mAh = current_mA * (dt_s / 3600.0);
    remaining_mAh -= delta_mAh;
    if (remaining_mAh < 0) remaining_mAh = 0;
    if (remaining_mAh > BATTERY_CAPACITY_mAh) remaining_mAh = BATTERY_CAPACITY_mAh;
    battaryPercentage = (remaining_mAh / BATTERY_CAPACITY_mAh) * 100.0;

    static unsigned long lastCorrection = 0;
    if (now - lastCorrection >= 5UL * 60UL * 1000UL || fabs(current_mA) < 20.0) {
      float v_soc = voltageToSoC(battaryVoltage);
      float voltage_est_mAh = (v_soc / 100.0) * BATTERY_CAPACITY_mAh;
      remaining_mAh = remaining_mAh * 0.90 + voltage_est_mAh * 0.10;
      lastCorrection = now;
    }
  }
}

void tempSensor() {
  if (isnan(humidity) || isnan(temperature)) return;
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
}

void lightSensor() {
  lightIntencity = lightMeter.readLightLevel();
  if (lightIntencity < 0) Ebme = "1";
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  dht.begin();

  if (rtc.begin()) {
    rtcAvailable = true;
    rtcE = "0";
  } else {
    rtcE = "1";
  }
  // rtc.adjust(DateTime(2025, 7, 28, 23, 20, 0));  //  Uncomment if needed YYYY/MM/DD  HH/MM/SS

  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Ebme = "1";
  } else {
    Ebme = "0";
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;) }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (!solor.begin()) Esolor = '1';
  else Esolor = '0';
  if (!battary.begin()) Ebattary = '1';
  else Ebattary = '0';

  if (!SD.begin(SD_CS)) EsdCard = "1";
  else EsdCard = "0";
  lastSampleTime = millis();
}

void loop() {}