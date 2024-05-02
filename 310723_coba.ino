#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "GP2Y1010AU0F.h"
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WiFi.h>

#define DHTPIN 27    // DHT22 data pin
#define DHTTYPE DHT22
#define sensorO2pin 32
#define sensorCOpin 35
#define dustSensorPin 34
const int analogCO2Pin = 33;

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);



// const char* ssid = "Ketua Dari Pulau";
// const char* password = "sahabatkita";
const char* ssid = "Solo";
const char* password = "12345678";
const char* server = "kualitas-udara.my.id";
const int port = 443;
const char* url = "https://kualitas-udara.my.id/api/post/data/sensor";
unsigned long send_time, saved_time, delay_time;
unsigned long timepoint = 0;
const int blueLedPin = 2;

WiFiClientSecure secured_client;

void connectToWiFi() {
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.print("Connected to Wifi");
  Serial.println(ssid);
}


void sendDataHttps(float temperature, float humidity, float dustConcentration, float co2Concentration, float sensorO2_value, float sensorCO_value) {
  StaticJsonDocument<128> doc;
  doc["suhu"] = temperature;
  doc["kelembapan"] = humidity;
  doc["debu"] = dustConcentration;
  doc["co2"] = co2Concentration;
  doc["o2"] = sensorO2_value;
  doc["co"] = sensorCO_value;

  String data;
  serializeJson(doc, data);


  // secured_client.setCACert(rootCACertificate);
  // secured_client.setInsecure();

  // if (!secured_client.connect(server, port)) {
  //   Serial.println("Connection failed");
  //   if (secured_client.connected()) {
  //     Serial.println("Client is connected");
  //   } else {
  //     Serial.println("Client is not connected");
  //   }
  //   return;
  // }
  Serial.println("Connected to server!");
  HTTPClient https;
  https.begin(url);
  https.addHeader("Content-Type", "application/json");
  send_time = millis();
  int httpsCode = https.POST(data);

  if (https.getString().indexOf("Data sensor berhasil disimpan") != -1) {
    saved_time = millis();
    delay_time = saved_time - send_time;
    Serial.print("Delay time: ");
    Serial.print(delay_time);
    Serial.println(" ms");
  }

  String response;
  if (httpsCode == 201) {
    Serial.printf("[HTTPS] POST code: %d\n", httpsCode);
    response = https.getString();
    Serial.println(response);
//    sendDelayTimeHttps(https, delay_time);
  } else if (httpsCode > 0) {
    Serial.printf("[HTTPS] POST code: %d\n", httpsCode);
    response = https.getString();
    Serial.println(response);
  } else {
    Serial.printf("[HTTPS] POST code: %d\n", httpsCode);
  }

  https.end();
}


void setup() {
  Serial.begin(115200);

  pinMode(blueLedPin, OUTPUT);

  // Menghubungkan ke jaringan Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Memeriksa status koneksi internet
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(blueLedPin, HIGH);  // Hidupkan lampu biru
  } else {
    digitalWrite(blueLedPin, LOW);   // Matikan lampu biru
  }
  connectToWiFi();
  delay(1000);
  pinMode(sensorO2pin, INPUT);
  pinMode(sensorCOpin, INPUT);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(3,0);
  lcd.print("Penelitian");
  lcd.setCursor(1,1);
  lcd.print("Kualitas Udara");
  Wire.begin();
  dht.begin();
}

void loop() {
  // Read CO2 sensor data
  int sensorValue = analogRead(analogCO2Pin);
  float voltage = sensorValue * 5.0 / 4095;
  float co2Concentration = map(sensorValue, 0, 4095, 0, 5000); // Assuming 0-5000 ppm range, adjust accordingly

  Serial.print("CO2: ");
  Serial.print(co2Concentration);
  Serial.println("ppm");

  float sensorO2 = analogRead(sensorO2pin);
  float sensorO2_value = sensorO2 * 5.0 / 1024;
  Serial.print("SensorO2 Analog: ");
  Serial.println(sensorO2);
  Serial.print("SensorO2 : ");
  Serial.println(sensorO2_value);

  float sensorCO = analogRead(sensorCOpin);
  int CO = ((sensorCO * 18623) + 222.67);
  float sensorCO_value = sensorCO * 5.0 / 1024;
  Serial.print("SensorCO Analog: ");
  Serial.println(sensorCO);
  Serial.print("SensorCO : ");
  Serial.println(sensorCO_value);

  int rawValue = analogRead(dustSensorPin);
  // Convert the raw analog value to voltage (0 to 3.3V)
  float voltage_dust = (float)rawValue * 5.0 / 1024;
  float dustConcentration = 0.17 * voltage_dust - 0.1;

  Serial.print("Debu: ");
  Serial.print(dustConcentration);
  Serial.println("ug/m3");
  //0-50 Baik //51-150 Sedang //151-350 Tidak Sehat //351-420 Sangat Tidak Sehat //> 420 Berbahaya

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Gagal membaca data dari sensor DHT22!");
    return;
  }

  Serial.print("Suhu: "); 
  Serial.print(temperature);
  Serial.println("*C \t");

  Serial.print("Kelembaban: ");
  Serial.print(humidity);
  Serial.println("%");

  lcd.clear();  
  lcd.print("Suhu: ");
  lcd.print(temperature);
  lcd.write(B11011111);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Kelembaban:");
  lcd.print(humidity);
  lcd.print("%");

  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Debu: ");
  lcd.print(dustConcentration);
  lcd.print(" ug/m3");

  lcd.setCursor(0, 1);
  lcd.print("CO2 : ");
  lcd.print(co2Concentration);
  lcd.print(" ppm");

  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("O2: ");
  lcd.print(sensorO2_value);
  lcd.print(" ppm");

  lcd.setCursor(0, 1);
  lcd.print("CO: ");
  lcd.print(sensorCO_value);
  lcd.print(" ppm");
  sendDataHttps(temperature, humidity,  dustConcentration,  co2Concentration,  sensorO2_value,  sensorCO_value) ;
  delay(1000);
}
