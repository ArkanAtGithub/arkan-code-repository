#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>
#include <ArduinoOTA.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16x2 display

Adafruit_INA219 ina219_batt;
Adafruit_INA219 ina219_out(0x41);

#define INFLUXDB_URL "http://192.168.0.10:8086"
#define INFLUXDB_TOKEN "Baskara putraaaa"
#define INFLUXDB_ORG "ArkanDB"
#define INFLUXDB_BUCKET "UPS"

char ssid[] = "N/A";
char pass[] = "Arkan101";

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

Point sensor("ups_system");

int i = 0; // switch-case
unsigned long previousMillis = 0;  // will store last time
unsigned long previousMeasureMillis = 0;  // store last time on measurement
unsigned long previousInfluxMillis = 0; // last time for writeInfluxdb
unsigned long backlightMillis = 0;  // stores time of last backlight change
bool backlightState = true;  // stores the current state of the backlight (on or off)

const long interval = 3000;  // interval at which to change the info on LCD (milliseconds)
const long measureInterval = 3000; // measuring interval (milliseconds)
const long influxInterval = 5000;  // 5 seconds in milliseconds
const long backlightOffDelay = 10000;  // time after which the backlight turns off (milliseconds)
const long backlightOnDelay = 10000;  // time to keep the backlight on (milliseconds)

#define buttonPin D4  // GPIO pin for the button
bool lastButtonState = LOW;  // previous button state
bool buttonPressed = false;  // flag for button press

float voltageBattery = 0; // VBatt
float voltageOut = 0; // VOut
float currentBattery = 0; // IBatt
float currentOut = 0; // IOut
float powerBattery = 0; // PBatt
float powerOut = 0; // POut

const float R_FIXED = 1000.0; // 1K resistor
const float R_25 = 10000.0;  // Thermistor resistance at 25°C
const float BETA = 3950.0;   // Beta value for thermistor
const float VCC = 3.3;       // Supply voltage
const int ADC_MAX = 1024;     // ADC resolution
float tempC = 0; // Temperature in degree celsius
const float TEMP_THRESHOLD = 50.0; // Temperature threshold in Celsius
bool isOverheated = false; // Flag to track temperature state

float voltageBatteryBuffer[5] = {0};
float voltageOutBuffer[5] = {0};
float currentBatteryBuffer[5] = {0};
float currentOutBuffer[5] = {0};
float powerBatteryBuffer[5] = {0};
float powerOutBuffer[5] = {0};
float batteryTempBuffer[5] = {0};
int bufferIndex = 0;

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);  // set button pin as input with pull-up
  lcd.init();
  lcd.backlight();
  lcd.clear();
  if (!ina219_batt.begin()) {
    lcd.print("ERROR!");
    lcd.setCursor(0, 1);
    lcd.print("NO INA219_batt");
    while (1) { delay(10); }
  }
  ina219_batt.setCalibration_32V_1A();
  if (!ina219_out.begin()) {
    lcd.print("ERROR!");
    lcd.setCursor(0, 1);
    lcd.print("NO INA219_out");
    while (1) { delay(10); }
  }
  ina219_out.setCalibration_32V_1A();
  WiFi.begin(ssid, pass);
  lcd.print("Wait for Wi-Fi");
  lcd.setCursor(0, 1);
  while (WiFi.status() != WL_CONNECTED) {
    lcd.print(".");
    delay(500);
  }
  lcd.clear();
  // Check server connection
  if (client.validateConnection()) {
    lcd.setCursor(0, 0);
    lcd.print("InfluxDB");
    lcd.setCursor(0, 1);
    lcd.print("Connected");
    delay(1000);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("InfluxDB ERROR!");
    lcd.setCursor(0, 1);
    lcd.print(client.getLastErrorMessage());
    delay(1000);
  }
  lcd.clear();
  // ArduinoOTA
  ArduinoOTA.setHostname("UPS-system");
  ArduinoOTA.setPassword((const char *)"Arkan");
  ArduinoOTA.onStart([]() {
    lcd.clear();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("OTA start");
  });
  ArduinoOTA.onEnd([]() {
    lcd.clear();
    lcd.print("OTA end");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    lcd.setCursor(0, 0);
    lcd.printf("Progress: %u%%", (progress / (total / 100)));
  });
  ArduinoOTA.begin();
  lcd.print("ArduinoOTA READY");
  delay(1000);
  lcd.clear();
  lcd.print("IP address: ");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(1000);
  // Lattuce begin
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DIY UPS project");
  delay(1000);
}

void loop() {
  unsigned long currentMillis = millis();
  bool buttonState = digitalRead(buttonPin);

  // Check for button press to toggle backlight on/off
  if (buttonState == LOW && lastButtonState == HIGH) {
    backlightState = !backlightState;
    if (backlightState) {
      lcd.backlight();
      backlightMillis = currentMillis;
    } else {
      lcd.noBacklight();
    }
    delay(200);
  }

  lastButtonState = buttonState;

  // Make measurement every second
  if (currentMillis - previousMeasureMillis >= measureInterval) {
    previousMeasureMillis = currentMillis;
    measure();
    
    // Check temperature threshold
    if (tempC >= TEMP_THRESHOLD && !isOverheated) {
      isOverheated = true;
      lcd.backlight(); // Force backlight on
      backlightState = true;
      displayTemperatureWarning();
    } else if (tempC < TEMP_THRESHOLD && isOverheated) {
      isOverheated = false;
      backlightMillis = currentMillis; // Reset backlight timer
    }
  }

  // Only proceed with normal display logic if not overheated
  if (!isOverheated) {
    // Normal backlight management
    if (backlightState && currentMillis - backlightMillis >= backlightOffDelay) {
      lcd.noBacklight();
      backlightState = false;
    }

    // Handle InfluxDB updates every 5 seconds
    if (currentMillis - previousInfluxMillis >= influxInterval) {
      previousInfluxMillis = currentMillis;
      writeInfluxdb();
      ArduinoOTA.handle();
    }

    // Change the LCD display every 3 seconds
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      
      switch(i) {
        case 0:
          infoOne();
          break;
        case 1:
          infoTwo();
          break;
        case 2:
          infoThree();
          break;
      }
      i = (i + 1) % 3;
    }
  } else {
    // Update temperature display while in warning state
    if (currentMillis - previousMillis >= 1000) { // Update every second
      previousMillis = currentMillis;
      displayTemperatureWarning();
    }
  }
  
  delay(100);
}

void displayTemperatureWarning() {
  lcd.clear();
  lcd.print("WARNING!");
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(tempC, 1);
  lcd.print(" C");
}

void measure() {
  voltageBattery = ina219_batt.getBusVoltage_V();
  voltageOut = ina219_out.getBusVoltage_V();
  currentBattery = ina219_batt.getCurrent_mA();
  currentOut = ina219_out.getCurrent_mA();
  powerBattery = ina219_batt.getPower_mW();
  powerOut = ina219_out.getPower_mW();

  int adcValue = analogRead(A0); // Read ADC value
  float vOut = (adcValue * VCC) / ADC_MAX; // Convert ADC value to voltage
  float rThermistor = R_FIXED * ((VCC / vOut) - 1); // Calculate thermistor resistance
  tempC = 1.0 / ((1.0 / 298.15) + (1.0 / BETA) * log(rThermistor / R_25)) - 273.15;

  // Store the new measurements in the circular buffers
  voltageBatteryBuffer[bufferIndex] = voltageBattery;
  voltageOutBuffer[bufferIndex] = voltageOut;
  currentBatteryBuffer[bufferIndex] = currentBattery;
  currentOutBuffer[bufferIndex] = currentOut;
  powerBatteryBuffer[bufferIndex] = powerBattery;
  powerOutBuffer[bufferIndex] = powerOut;
  batteryTempBuffer[bufferIndex] = tempC;

  bufferIndex = (bufferIndex + 1) % 5;
}

void writeInfluxdb() {
  float voltageBatteryAvg = 0;
  float voltageOutAvg = 0;
  float currentBatteryAvg = 0;
  float currentOutAvg = 0;
  float powerBatteryAvg = 0;
  float powerOutAvg = 0;
  float batteryTempAvg = 0;

  // Calculate the averages using the circular buffers
  for (int i = 0; i < 5; i++) {
    voltageBatteryAvg += voltageBatteryBuffer[i];
    voltageOutAvg += voltageOutBuffer[i];
    currentBatteryAvg += currentBatteryBuffer[i];
    currentOutAvg += currentOutBuffer[i];
    powerBatteryAvg += powerBatteryBuffer[i];
    powerOutAvg += powerOutBuffer[i];
    batteryTempAvg += batteryTempBuffer[i];
  }

  voltageBatteryAvg /= 5;
  voltageOutAvg /= 5;
  currentBatteryAvg /= 5;
  currentOutAvg /= 5;
  powerBatteryAvg /= 5;
  powerOutAvg /= 5;
  batteryTempAvg /= 5;

  sensor.addField("battery_voltage", voltageBatteryAvg);
  sensor.addField("bus_voltage", voltageOutAvg);
  sensor.addField("battery_current", currentBatteryAvg);
  sensor.addField("bus_current", currentOutAvg);
  sensor.addField("battery_power", powerBatteryAvg);
  sensor.addField("bus_power", powerOutAvg);
  sensor.addField("battery_temperature", batteryTempAvg);

  client.writePoint(sensor);

  sensor.clearFields();
}

void infoOne() {
  lcd.clear();
  lcd.print("Vbat: ");
  lcd.print(voltageBattery);
  lcd.print("V");
  lcd.setCursor(0, 1);
  lcd.print("Ibat: ");
  lcd.print(currentBattery);
  lcd.print("mA");
  lcd.setCursor(15, 1);
  lcd.print("1");
}

void infoTwo() {
  lcd.clear();
  lcd.print("Vout: ");
  lcd.print(voltageOut);
  lcd.print("V");
  lcd.setCursor(0, 1);
  lcd.print("Iout: ");
  lcd.print(currentOut);
  lcd.print("mA");
  lcd.setCursor(15, 1);
  lcd.print("2");
}

void infoThree() {
  lcd.clear();
  lcd.print("Pbat: ");
  lcd.print(powerBattery);
  lcd.print("mW");
  lcd.setCursor(0, 1);
  lcd.print("Pout: ");
  lcd.print(powerOut);
  lcd.print("mW");
  lcd.setCursor(15, 1);
  lcd.print("3");
}
