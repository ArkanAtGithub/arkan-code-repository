#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <MQUnifiedsensor.h>
#include <ThingSpeak.h>

#define rainPin D6
#define dhtPin D7
#define dhtType DHT11
#define placa "ESP8266"
#define Voltage_Resolution 3.3
#define pin A0
#define type "MQ-135"
#define ADC_Bit_Resolution 10
#define RatioMQ135CleanAir 3.6

unsigned long myChannelNumber = 2499370;
const char * myWriteAPIKey = "Git command sucks";

char ssid[] = "N/A";
char pass[] = "Arkan101";
WiFiClient  client;

Adafruit_BMP280 bmp;
DHT dht(dhtPin, dhtType);
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);

float temp = 25;
int humid = 50;
float baro = 1013.25;
float heat = 25;
float dew = 13.8;
int rssi = -60;
float ppm = 425.38;
float r0 = 0;
int rdsstat = 1;

void setup() {
  WiFi.begin(ssid, pass);
  bmp.begin(0x76);
  dht.begin();
  pinMode(rainPin, INPUT);
  MQ135.setRegressionMethod(1);
  MQ135.setA(110.47); MQ135.setB(-2.862);
  MQ135.setRL(10.2);
  MQ135.init();
  for(int i = 1; i<=10; i ++)
  {
    MQ135.update();
    r0 += MQ135.calibrate(RatioMQ135CleanAir);
  }
  MQ135.setR0(r0/10);
  ThingSpeak.begin(client);
}

void getreading() {
  temp = bmp.readTemperature();
  humid = dht.readHumidity();
  baro = bmp.readPressure() / 100.0;
  heat = dht.computeHeatIndex(temp, humid);
  dew = temp - ((100 - humid)/5);
  rssi = WiFi.RSSI();
  rdsstat = digitalRead(rainPin);
  MQ135.update();
  ppm = MQ135.readSensor() + 425.38;
}

void loop() {
  getreading();
  String status = "";

  ThingSpeak.setField(1, temp);
  ThingSpeak.setField(2, humid);
  ThingSpeak.setField(3, baro);
  ThingSpeak.setField(4, heat);
  ThingSpeak.setField(5, dew);
  ThingSpeak.setField(6, rssi);
  ThingSpeak.setField(7, ppm);
  ThingSpeak.setField(8, rdsstat);

  if(isnan(humid)){
    status = String("DHT11 output NaN");
  }
  else if(isnan(temp) | isnan(baro)){
    status = String("BMP280 output NaN");
  }
  else{status = String("DHT11, BMP280 Operational");}

  ThingSpeak.setStatus(status);

  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  
  delay(15000);
}
