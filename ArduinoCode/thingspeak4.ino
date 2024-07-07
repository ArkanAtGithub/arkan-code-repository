#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <ThingSpeak.h>

#define rainPin A0
#define dhtPin D7
#define dhtType DHT11

unsigned long myChannelNumber = 2499370;
const char * myWriteAPIKey = "I am so dumb";

char ssid[] = "N/A";
char pass[] = "Arkan101";
WiFiClient  client;

Adafruit_BMP280 bmp;
DHT dht(dhtPin, dhtType);

float temp = 25;
int humid = 50;
float baro = 1013.25;
float heat = 25;
float dew = 13.8;
int rssi = -60;
int rainraw = 0;
float rain = 0;
int count = 0;
float last = 0;
float lastlast = 0;
bool bmpfail = false;
String status = "";

void setup() {
  WiFi.begin(ssid, pass);
  bmp.begin(0x76);
  dht.begin();
  pinMode(rainPin, INPUT);
  ThingSpeak.begin(client);
}

// get sensor reading
void getreading() {
  temp = bmp.readTemperature();
  temp = constrain(temp, 0, 40);
  humid = dht.readHumidity();
  humid = constrain(humid, 0, 101);
  baro = bmp.readPressure() / 100.0;
  baro = constrain(baro, 990, 1010);
  heat = dht.computeHeatIndex(temp, humid);
  dew = temp - ((100 - humid)/5);
  rssi = WiFi.RSSI();
  rainraw = analogRead(rainPin);
  rain = map(rainraw, 1023, 0, 0, 100);
}

// push data to thingspeak and check for error
void thingspeakpush() {
  // data to send
  ThingSpeak.setField(1, temp);
  ThingSpeak.setField(2, humid);
  ThingSpeak.setField(3, baro);
  ThingSpeak.setField(4, heat);
  ThingSpeak.setField(5, dew);
  ThingSpeak.setField(6, rssi);
  ThingSpeak.setField(7, rain);

  // send the data
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
}

void errorhandling() {
  // check for duplicate value
  count++;
    
  if (count == 1) {
       last = baro;
  } else if (count == 2) {
      lastlast = baro;
  } else if (count >= 3) {
      bmpfail = (last == lastlast);
      count = 0;
  }

  // error handling
  if (isnan(humid) || humid > 100) { // if DHT11 failed
      status = "DHT11 Failed, restarting";
      ThingSpeak.setStatus(status);
      ESP.restart();
  } else if (isnan(temp) || isnan(baro) || bmpfail) { // if BMP280 failed
      status = "BMP280 Failed, restarting";
      ThingSpeak.setStatus(status);
      ESP.restart();
  }
}

void loop() {
  getreading();
  thingspeakpush();
  errorhandling();
  delay(15000);
}
