#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define INFLUXDB_URL "http://192.168.0.6:8086"
#define INFLUXDB_TOKEN "git push -f origin 256814b42f57bb71c0841b290e76721dedaceec5:main"
#define INFLUXDB_ORG "ArkanDB"
#define INFLUXDB_BUCKET "WX-1"

#define rainPin A0
#define dhtPin D7
#define dhtType DHT11

char ssid[] = "N/A";
char pass[] = "Arkan101";

Adafruit_BMP280 bmp;
DHT dht(dhtPin, dhtType);
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

Point sensor("weather_station");

float temp = 25.0;
int humid = 50;
float baro = 1013.25;
float heat = 25.0;
float dew = 13.8;
int rssi = -60;
int rainraw = 0;
float rain = 0.0;
int count = 0;
float last = 0.0;
float lastlast = 0.0;
bool bmpfail = false;
bool restart = false;
String status = "";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  bmp.begin(0x76);
  dht.begin();
  pinMode(rainPin, INPUT);

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void getReading() {
  temp = bmp.readTemperature();
  temp = constrain(temp, 0, 40);
  humid = dht.readHumidity();
  humid = constrain(humid, 0, 101);
  baro = bmp.readPressure() / 100.0;
  baro = constrain(baro, 990, 1020);
  heat = dht.computeHeatIndex(temp, humid);
  heat = constrain(heat, temp, 100);
  dew = temp - ((100 - humid)/5);
  rssi = WiFi.RSSI();
  rainraw = analogRead(rainPin);
  rain = map(rainraw, 1023, 0, 0, 100);
}

void influxdbPush() {
  // Add data fields
  sensor.addField("temperature", temp);
  sensor.addField("humidity", humid);
  sensor.addField("pressure", baro);
  sensor.addField("heat_index", heat);
  sensor.addField("dew_point", dew);
  sensor.addField("rssi", rssi);
  sensor.addField("rain", rain);

  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  if (restart == true) {
    ESP.restart();
  }

  sensor.clearFields();
}

void errorHandling() {
  count++;
  if (count == 1) {
    last = baro;
  } else if (count == 2) {
    lastlast = baro;
  } else if (count >= 3) {
    bmpfail = (last == lastlast);
    count = 0;
  }

  if (isnan(humid) || humid > 100) {
    status = "DHT11 Failed, restarting";
    sensor.addField("status", status);
    restart = true;
  } else if (isnan(temp) || isnan(baro) || bmpfail) {
    status = "BMP280 Failed, restarting";
    sensor.addField("status", status);
    restart = true;
  }
}

void loop() {
  getReading();
  influxdbPush();
  errorHandling();
  delay(5000);
}
