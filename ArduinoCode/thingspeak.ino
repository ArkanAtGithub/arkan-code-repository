#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <ThingSpeak.h>

unsigned long myChannelNumber = 2499370;
const char * myWriteAPIKey = "";

char ssid[] = "N/A";
char pass[] = "Arkan101";
WiFiClient  client;

Adafruit_BMP280 bmp;
DHT dht(D7, DHT11);

void setup() {
  WiFi.begin(ssid, pass);
  bmp.begin(0x76);
  dht.begin();
  ThingSpeak.begin(client);
}

void loop() {
  float temp = bmp.readTemperature();
  int humid = dht.readHumidity();
  float baro = bmp.readPressure() / 100.0;
  float heat = dht.computeHeatIndex(temp, humid);
  float dew = temp - ((100 - humid)/5);
  int rssi = WiFi.RSSI();
  String status = "";

  ThingSpeak.setField(1, temp);
  ThingSpeak.setField(2, humid);
  ThingSpeak.setField(3, baro);
  ThingSpeak.setField(4, heat);
  ThingSpeak.setField(5, dew);
  ThingSpeak.setField(6, rssi);

  if(isnan(humid)){
    status = String("DHT11 output NaN");
  }
  else if(isnan(temp) | isnan(baro)){
    status = String("BMP280 output NaN");
  }
  else{status = String("DHT11, BMP280 Operational");}

  ThingSpeak.setStatus(status);

  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    digitalWrite(LED_BUILTIN, LOW);
  } else {digitalWrite(LED_BUILTIN, HIGH);}
  
  delay(15000);
}

