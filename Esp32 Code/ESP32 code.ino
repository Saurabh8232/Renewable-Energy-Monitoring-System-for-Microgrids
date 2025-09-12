#include <PZEM004Tv30.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_MAX1704X.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>
#include <HardwareSerial.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <esp_now.h>

#define SD_CS 5
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

String ssid = "";
String password = "";
String serverName = "";
String Longitude = "";
String Latitude = "";
String Token = "";
String RoomEsp = "";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
HardwareSerial wifiDevice(1);
HardwareSerial pzemSerial(2);
PZEM004Tv30 pzem(pzemSerial, 16, 17);
Adafruit_INA219 solar;
Adafruit_MAX17048 battery;
BH1750 lightMeter;

float frequency = 0.0, powerFactor = 0.0;
float voltage = 0.0, current = 0.0;
float power = 0.0, energy = 0.0;
int inverterLoad = 0;
float solarVoltage = 0.0, solarCurrent = 0.0;
float solarPower = 0.0, batteryPercentage = 0.0;
float lightIntensity = 0.0, batteryVoltage = 0.0;

uint8_t broadcastAddress[] = { 0x3C, 0x8A, 0x1F, 0x0A, 0x05, 0xE0 };

// Error flags
bool E_light = false, E_solar = false, E_battery = false, Epem = false, Ewifi = false, Ehttp = false, Esdcard = false;

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

template<typename A>
void write(String File_name, A Data) {
  String path = String("/ESP32/") + File_name + ".txt";
  File file = SD.open(path, FILE_WRITE);
  if (file) {
    file.println(String(Data));
    file.close();
    Serial.println("kjfdsjldfgjshlgkjfgvjksdhhlgh;dfknnmvkljfgh;kjdhgfhjsdhnfkjhsjkghaljhkjghajkljlkadsj;fklsjvlhfksdlhfasdjkfhdsjaglf");
  } 
}

void onSend(const esp_now_send_info_t* info, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    RoomEsp = "Connected";
  } else RoomEsp = "Disconnected";
}

void sendToRoomEsp() {
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["RoomESP"] = overLoading();

  size_t jsonSize = measureJson(jsonDoc) + 1;
  char jsonBuffer[jsonSize];
  serializeJson(jsonDoc, jsonBuffer, jsonSize);
  esp_now_send(broadcastAddress, (uint8_t*)jsonBuffer, jsonSize);
}

String overLoading() {
  String roomEsp;
  float overload_limit = inverterLoad * 1.00;
  if (power > overload_limit) roomEsp = "false";
  else roomEsp = "true";
  return roomEsp;
}

void displayErrors() {
  String a;

  if (E_light) a += "Light.";
  if (E_solar) a += "Solar.";
  if (E_battery) a += "Battery.";
  if (Epem) a += "PZEM.";
  if (Ewifi) a += "WiFi.";
  if (Ehttp) a += "HTTP.";
  if (Esdcard) a += "SDcard.";


  display.clearDisplay();
  display.setCursor(0, 0);

  display.print("Solar Gen.  : ");
  display.print(random(10, 500));
  display.println(" WH");

  display.print("Inver. Load : ");
  display.print(random(10, 500));
  display.println(" WH");

  display.print("Battery     : ");
  display.print(batteryPercentage, 1);
  display.println(" %");
  display.println("");
  display.print(a);
  display.display();
}

void RANDOMdata() {
  frequency = random(49, 50);
  powerFactor = random(9, 1);
  voltage = random(218, 230);
  current = random(1, 10);
  power = voltage * current;
  energy = random(12, 18);
  inverterLoad = 3000;
  solarVoltage = random(200, 500);
  solarCurrent = random(1, 5);
  solarPower = solarVoltage * solarCurrent;
  batteryPercentage = random(10, 100);
  lightIntensity = random(200, 999);
  batteryVoltage = random(3.6, 4.8);
}

void serialData() {
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

  Serial.print(" | Lux:");
  Serial.print(lightIntensity);

  Serial.print(" | Errs -> L:");
  Serial.print(E_light);
  Serial.print(" S:");
  Serial.print(E_solar);
  Serial.print(" B:");
  Serial.print(E_battery);
  Serial.print(" P:");
  Serial.println(Epem);
  Serial.print("RoomEsp : ");
  Serial.println(RoomEsp);
  Serial.println();
}

void dataSendToEsp8266() {
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
  doc["Server"] = serverName;
  doc["Ssid"] = ssid;
  doc["Password"] = password;
  doc["RoomEsp"] = RoomEsp;

  Serial.println("Sending data to WiFi Device: ");
  serializeJson(doc, wifiDevice);
  wifiDevice.println();
  serializeJson(doc, Serial);
  Serial.println();

  if (wifiDevice.available()) {
    String response = wifiDevice.readStringUntil('\n');
    Serial.print("Received from ESP8266: ");
    Serial.println(response);

    StaticJsonDocument<300> doc1;
    deserializeJson(doc1, response);

    String a = doc1["Ewifi"].as<String>();
    if (a != "false") Ewifi = false;
    else if (a != "true") Ewifi = true;

    String b = doc1["Ehttp"].as<String>();
    if (b != "false") Ehttp = false;
    else if (b != "true") Ehttp = true;
  }
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
  float v = battery.cellVoltage();
  float soc = battery.cellPercent();
  if (!isnan(v) && !isnan(soc)) {
    batteryVoltage = v;
    batteryPercentage = soc;
    E_battery = false;
  } else {
    E_battery = true;
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

void setupESPNow() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(onSend);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  pzemSerial.begin(9600, SERIAL_8N1, 16, 17);
  wifiDevice.begin(115200, SERIAL_8N1, 32, 33);
  setupESPNow();

  if (!SD.begin(SD_CS)) {
    Esdcard = true;
    ssid = "OnePlus";
    password = "";
    serverName = "https://renewable-energy-monitoring-system-for.onrender.com/esp32-data";
    Token = "rozHpF7JqlEw6XnPluAF";
  } else {
    ssid = fetch("ssid");
    password = fetch("password");
    serverName = fetch("server");
    Longitude = fetch("Longitude");
    Latitude = fetch("Latitude");
    Token = fetch("Token");
    inverterLoad = fetch("inverterLoad").toInt();

    write("inverterLoad", 700);
    write("Token", "rozHpF7JqlEw6XnPluAF");
    write("Latitude", 28.30);
    write("Longitude", 79.49);
    write("server", "https://renewable-energy-monitoring-system-for.onrender.com/esp32-data");
    write("password", "");
    write("ssid", "OnePlus");
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
  RANDOMdata();

  // readPZEM();
  // readSolar();
  // readBattery();
  // readLight();
  sendToRoomEsp();
  dataSendToEsp8266();
  displayErrors();
  serialData();
}
