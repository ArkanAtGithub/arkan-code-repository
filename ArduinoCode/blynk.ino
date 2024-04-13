#define BLYNK_TEMPLATE_ID           "COME'ON"
#define BLYNK_TEMPLATE_NAME         "WX"
#define BLYNK_AUTH_TOKEN            ":)"
#define BLYNK_FIRMWARE_VERSION      "0.1.2"
#define BLYNK_HEARTBEAT             30

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>

#define DHTPIN D7
#define DHTTYPE DHT11

Adafruit_BMP280 bmp;
DHT dht(DHTPIN, DHTTYPE);

char ssid[] = "Arkan";
char pass[] = "halilintar";

float temp = 0;
float humid = 0;
float baro = 0;
float heat = 0;
int rssi = 0;

BlynkTimer timer;

void myTimerEvent()
{
  temp = dht.readTemperature();
  humid = dht.readHumidity()-5;
  baro = bmp.readPressure()/100;
  heat = dht.computeHeatIndex(temp, humid);
  rssi = WiFi.RSSI();
  Blynk.beginGroup();
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, humid);
  Blynk.virtualWrite(V2, baro);
  Blynk.virtualWrite(V3, heat);
  Blynk.virtualWrite(V4, rssi);
  Blynk.endGroup();
  if (isnan(humid) | isnan(temp)) {
    Blynk.logEvent("dht_err", "DHT11 ERROR");
  } if (isnan(baro)) {
    Blynk.logEvent("bmp_err", "BMP280 ERROR");
  } if (WiFi.status() == WL_CONNECTION_LOST) {
    Blynk.logEvent("wifi_lost", "WiFi connection lost");
  }
}

void setup()
{
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(1000L, myTimerEvent);

  bmp.begin(0x76);
  dht.begin();
}

void loop()
{
  Blynk.run();
  timer.run();
}

