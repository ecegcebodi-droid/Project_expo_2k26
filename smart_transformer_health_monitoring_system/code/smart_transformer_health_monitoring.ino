/************************************************************
   SMART TRANSFORMER HEALTH MONITORING
   ESP32 WROOM - PROTOTYPE VERSION

   STATUS:
   NORMAL  -> GREEN ON, BUZZER OFF
   WARNING -> YELLOW BLINK, BUZZER OFF
   DANGER  -> RED ON, BUZZER ON

   SENSORS:
   DS18B20  -> Temperature
   DHT11    -> Humidity
   Smoke    -> Analog
   Water    -> Analog
   Sound    -> Analog

   LCD:
   16x2 I2C -> 0x27

   BLYNK:
   V0 Temperature
   V1 Humidity
   V2 Smoke %
   V3 Water %
   V4 Sound %
   V5 Approx Voltage
   V6 Approx Current
   V7 Approx Power
   V8 Status
************************************************************/

// ==========================================================
// BLYNK
// ==========================================================

#define BLYNK_TEMPLATE_ID   "TMPL3yBvNlENZ"
#define BLYNK_TEMPLATE_NAME "Smart Transformer Monitoring"
#define BLYNK_AUTH_TOKEN    "TyiLCm9zGehlDzkmcAB15rkdZhZp19D9"

#define BLYNK_PRINT Serial

// ==========================================================
// LIBRARIES
// ==========================================================

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <DHT.h>

// ==========================================================
// WIFI
// ==========================================================

char ssid[] = "GOJO";
char pass[] = "@12345678";

// ==========================================================
// PIN DEFINITIONS
// ==========================================================

// DS18B20
#define DS18B20_PIN 4

// DHT11
#define DHT_PIN 5
#define DHT_TYPE DHT11

// Smoke sensor
#define SMOKE_AO 34
#define SMOKE_DO 27

// Water level sensor
#define WATER_AO 32

// Sound sensor
#define SOUND_AO 33
#define SOUND_DO 26

// Buzzer
#define BUZZER_PIN 25

// LEDs
#define GREEN_LED 12
#define YELLOW_LED 13
#define RED_LED 14

// ==========================================================
// LCD
// ==========================================================

#define LCD_ADDR 0x27

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// ==========================================================
// SENSOR OBJECTS
// ==========================================================

OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

DHT dht(DHT_PIN, DHT_TYPE);

BlynkTimer timer;

// ==========================================================
// SYSTEM STATUS
// ==========================================================

enum SystemStatus
{
  NORMAL,
  WARNING,
  DANGER
};

SystemStatus systemStatus = NORMAL;

// ==========================================================
// SENSOR VARIABLES
// ==========================================================

float temperatureC = 30.0;
float humidity = 50.0;

int smokeValue = 0;
int waterRaw = 0;
int soundValue = 0;

float waterPercent = 0.0;

// Blynk display percentages
float smokePercent = 0.0;
float soundPercent = 0.0;

// ==========================================================
// ELECTRICAL VALUES
// ==========================================================

float voltageApprox = 230.0;
float currentApprox = 5.0;
float powerApprox = 1150.0;

// ==========================================================
// SENSOR HEALTH
// ==========================================================

bool tempSensorOK = true;
bool humiditySensorOK = true;

// ==========================================================
// LCD
// ==========================================================

int lcdPage = 0;

bool yellowState = false;

// ==========================================================
// ANALOG AVERAGE
// ==========================================================

int readAnalogAverage(int pin)
{
  long total = 0;

  for (int i = 0; i < 20; i++)
  {
    total += analogRead(pin);
    delayMicroseconds(500);
  }

  return total / 20;
}

// ==========================================================
// CONVERT ADC TO PERCENTAGE
// ==========================================================

float adcToPercent(int value)
{
  float percent = (value / 4095.0) * 100.0;

  if (percent < 0)
    percent = 0;

  if (percent > 100)
    percent = 100;

  return percent;
}

// ==========================================================
// READ SENSORS
// ==========================================================

void readSensors()
{
  // --------------------------------------------------------
  // DS18B20
  // --------------------------------------------------------

  ds18b20.requestTemperatures();

  float newTemp = ds18b20.getTempCByIndex(0);

  if (newTemp == DEVICE_DISCONNECTED_C ||
      newTemp < -55 ||
      newTemp > 125)
  {
    tempSensorOK = false;

    Serial.println("DS18B20 WARNING");

    // Keep previous temperature
  }
  else
  {
    tempSensorOK = true;
    temperatureC = newTemp;
  }

  // --------------------------------------------------------
  // DHT11
  // --------------------------------------------------------

  float newHumidity = dht.readHumidity();

  if (isnan(newHumidity))
  {
    humiditySensorOK = false;

    Serial.println("DHT11 WARNING");

    // Keep previous humidity
  }
  else
  {
    humiditySensorOK = true;
    humidity = newHumidity;
  }

  // --------------------------------------------------------
  // SMOKE
  // --------------------------------------------------------

  smokeValue = readAnalogAverage(SMOKE_AO);

  smokePercent = adcToPercent(smokeValue);

  // --------------------------------------------------------
  // WATER
  // --------------------------------------------------------

  waterRaw = readAnalogAverage(WATER_AO);

  waterPercent = adcToPercent(waterRaw);

  // --------------------------------------------------------
  // SOUND
  // --------------------------------------------------------

  soundValue = readAnalogAverage(SOUND_AO);

  soundPercent = adcToPercent(soundValue);

  // --------------------------------------------------------
  // SERIAL MONITOR
  // --------------------------------------------------------

  Serial.println("--------------------------------");

  Serial.print("Temperature : ");
  Serial.print(temperatureC);
  Serial.println(" C");

  Serial.print("Humidity    : ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Smoke ADC   : ");
  Serial.println(smokeValue);

  Serial.print("Smoke Level : ");
  Serial.print(smokePercent, 1);
  Serial.println(" %");

  Serial.print("Water ADC   : ");
  Serial.println(waterRaw);

  Serial.print("Water Level : ");
  Serial.print(waterPercent, 1);
  Serial.println(" %");

  Serial.print("Sound ADC   : ");
  Serial.println(soundValue);

  Serial.print("Sound Level : ");
  Serial.print(soundPercent, 1);
  Serial.println(" %");

  Serial.println("--------------------------------");
}

// ==========================================================
// APPROXIMATE ELECTRICAL VALUES
// ==========================================================

void calculateElectricalValues()
{
  /*
     PROJECT-LEVEL APPROXIMATION ONLY.

     These values are estimated from temperature.
     They are NOT actual voltage/current measurements.
  */

  voltageApprox =
    230.0 + ((temperatureC - 30.0) * 0.50);

  currentApprox =
    5.0 + ((temperatureC - 30.0) * 0.15);

  // Voltage limits
  if (voltageApprox < 180)
    voltageApprox = 180;

  if (voltageApprox > 280)
    voltageApprox = 280;

  // Current limits
  if (currentApprox < 0)
    currentApprox = 0;

  if (currentApprox > 20)
    currentApprox = 20;

  // Power
  powerApprox = voltageApprox * currentApprox;

  Serial.print("Approx Voltage : ");
  Serial.print(voltageApprox, 2);
  Serial.println(" V");

  Serial.print("Approx Current : ");
  Serial.print(currentApprox, 2);
  Serial.println(" A");

  Serial.print("Approx Power   : ");
  Serial.print(powerApprox, 2);
  Serial.println(" W");
}

// ==========================================================
// SYSTEM STATUS
// ==========================================================

void calculateSystemStatus()
{
  bool danger = false;
  bool warning = false;

  // --------------------------------------------------------
  // SENSOR COMMUNICATION
  // --------------------------------------------------------

  if (!tempSensorOK)
    warning = true;

  if (!humiditySensorOK)
    warning = true;

  // --------------------------------------------------------
  // TEMPERATURE
  // --------------------------------------------------------

  if (temperatureC >= 60.0)
  {
    danger = true;
  }
  else if (temperatureC >= 40.0)
  {
    warning = true;
  }

  // --------------------------------------------------------
  // HUMIDITY
  // --------------------------------------------------------

  if (humidity > 85.0)
  {
    danger = true;
  }
  else if (humidity < 30.0 || humidity > 70.0)
  {
    warning = true;
  }

  // --------------------------------------------------------
  // SMOKE
  // IMPORTANT:
  // RAW ADC VALUE USED FOR THRESHOLD
  // --------------------------------------------------------

  if (smokeValue >= 1800)
  {
    danger = true;
  }
  else if (smokeValue >= 800)
  {
    warning = true;
  }

  // --------------------------------------------------------
  // WATER
  // --------------------------------------------------------

  if (waterPercent < 10.0)
  {
    danger = true;
  }
  else if (waterPercent < 30.0)
  {
    warning = true;
  }

  // --------------------------------------------------------
  // SOUND
  // IMPORTANT:
  // RAW ADC VALUE USED FOR THRESHOLD
  // --------------------------------------------------------

  if (soundValue >= 2500)
  {
    danger = true;
  }
  else if (soundValue >= 1000)
  {
    warning = true;
  }

  // --------------------------------------------------------
  // PRIORITY
  // DANGER > WARNING > NORMAL
  // --------------------------------------------------------

  if (danger)
  {
    systemStatus = DANGER;
  }
  else if (warning)
  {
    systemStatus = WARNING;
  }
  else
  {
    systemStatus = NORMAL;
  }
}

// ==========================================================
// STATUS TEXT
// ==========================================================

String getStatusText()
{
  if (systemStatus == DANGER)
    return "DANGER";

  if (systemStatus == WARNING)
    return "WARNING";

  return "NORMAL";
}

// ==========================================================
// LED + BUZZER
// ==========================================================

void updateOutputs()
{
  // ========================================================
  // NORMAL
  // ========================================================

  if (systemStatus == NORMAL)
  {
    digitalWrite(GREEN_LED, HIGH);

    digitalWrite(YELLOW_LED, LOW);

    digitalWrite(RED_LED, LOW);

    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
  }

  // ========================================================
  // WARNING
  // ========================================================

  else if (systemStatus == WARNING)
  {
    digitalWrite(GREEN_LED, LOW);

    digitalWrite(RED_LED, LOW);

    // Yellow blinking
    yellowState = !yellowState;

    digitalWrite(YELLOW_LED, yellowState);

    // WARNING = BUZZER OFF
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
  }

  // ========================================================
  // DANGER
  // ========================================================

  else if (systemStatus == DANGER)
  {
    digitalWrite(GREEN_LED, LOW);

    digitalWrite(YELLOW_LED, LOW);

    digitalWrite(RED_LED, HIGH);

    // DANGER = BUZZER ON
    tone(BUZZER_PIN, 2000);
  }
}

// ==========================================================
// LCD
// ==========================================================

void updateLCD()
{
  lcd.clear();

  // ========================================================
  // PAGE 0
  // ========================================================

  if (lcdPage == 0)
  {
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperatureC, 1);
    lcd.print("C");

    lcd.setCursor(9, 0);
    lcd.print("H:");
    lcd.print(humidity, 0);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("Transformer");

    lcd.setCursor(12, 1);
    lcd.print("MON");
  }

  // ========================================================
  // PAGE 1
  // ========================================================

  else if (lcdPage == 1)
  {
    lcd.setCursor(0, 0);
    lcd.print("Smoke:");
    lcd.print(smokePercent, 0);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("Water:");
    lcd.print(waterPercent, 0);
    lcd.print("%");
  }

  // ========================================================
  // PAGE 2
  // ========================================================

  else if (lcdPage == 2)
  {
    lcd.setCursor(0, 0);
    lcd.print("Sound:");
    lcd.print(soundPercent, 0);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("Volt:");
    lcd.print(voltageApprox, 1);
    lcd.print("V");
  }

  // ========================================================
  // PAGE 3
  // ========================================================

  else if (lcdPage == 3)
  {
    lcd.setCursor(0, 0);
    lcd.print("Current:");
    lcd.print(currentApprox, 1);
    lcd.print("A");

    lcd.setCursor(0, 1);
    lcd.print("Power:");
    lcd.print(powerApprox, 0);
    lcd.print("W");
  }

  // ========================================================
  // PAGE 4
  // ========================================================

  else if (lcdPage == 4)
  {
    lcd.setCursor(0, 0);
    lcd.print("SYSTEM STATUS");

    lcd.setCursor(0, 1);
    lcd.print(getStatusText());
  }

  // Next page
  lcdPage++;

  if (lcdPage > 4)
    lcdPage = 0;
}

// ==========================================================
// SEND DATA TO BLYNK
// ==========================================================

void sendToBlynk()
{
  if (Blynk.connected())
  {
    // ------------------------------------------------------
    // Temperature
    // ------------------------------------------------------

    Blynk.virtualWrite(V0, temperatureC);

    // ------------------------------------------------------
    // Humidity
    // ------------------------------------------------------

    Blynk.virtualWrite(V1, humidity);

    // ------------------------------------------------------
    // Smoke %
    // ------------------------------------------------------

    Blynk.virtualWrite(V2, smokePercent);

    // ------------------------------------------------------
    // Water %
    // ------------------------------------------------------

    Blynk.virtualWrite(V3, waterPercent);

    // ------------------------------------------------------
    // Sound %
    // ------------------------------------------------------

    Blynk.virtualWrite(V4, soundPercent);

    // ------------------------------------------------------
    // Approx Voltage
    // ------------------------------------------------------

    Blynk.virtualWrite(V5, voltageApprox);

    // ------------------------------------------------------
    // Approx Current
    // ------------------------------------------------------

    Blynk.virtualWrite(V6, currentApprox);

    // ------------------------------------------------------
    // Approx Power
    // ------------------------------------------------------

    Blynk.virtualWrite(V7, powerApprox);

    // ------------------------------------------------------
    // Status
    // ------------------------------------------------------

    Blynk.virtualWrite(V8, getStatusText());
  }
}

// ==========================================================
// SETUP
// ==========================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("====================================");
  Serial.println(" SMART TRANSFORMER HEALTH MONITOR ");
  Serial.println(" ESP32 PROTOTYPE");
  Serial.println("====================================");

  // ========================================================
  // PIN MODES
  // ========================================================

  pinMode(SMOKE_AO, INPUT);
  pinMode(SMOKE_DO, INPUT);

  pinMode(WATER_AO, INPUT);

  pinMode(SOUND_AO, INPUT);
  pinMode(SOUND_DO, INPUT);

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  // ========================================================
  // INITIAL OUTPUT
  // ========================================================

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);

  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);

  // ========================================================
  // ADC
  // ========================================================

  analogReadResolution(12);

  // ========================================================
  // I2C
  // ========================================================

  Wire.begin(21, 22);

  // ========================================================
  // LCD
  // ========================================================

  lcd.init();
  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Transformer");

  lcd.setCursor(0, 1);
  lcd.print("Monitoring...");

  delay(2000);

  // ========================================================
  // SENSORS
  // ========================================================

  ds18b20.begin();

  dht.begin();

  // ========================================================
  // BLYNK + WIFI
  // ========================================================

  Serial.println("Connecting WiFi...");

  Blynk.begin(
    BLYNK_AUTH_TOKEN,
    ssid,
    pass
  );

  Serial.println("Blynk connected!");

  // ========================================================
  // TIMERS
  // ========================================================

  timer.setInterval(1000L, readSensors);

  timer.setInterval(1100L, calculateElectricalValues);

  timer.setInterval(1200L, calculateSystemStatus);

  timer.setInterval(500L, updateOutputs);

  timer.setInterval(1500L, updateLCD);

  timer.setInterval(2000L, sendToBlynk);

  // ========================================================
  // INITIAL
  // ========================================================

  calculateSystemStatus();

  updateOutputs();

  Serial.println("SYSTEM READY!");
}

// ==========================================================
// LOOP
// ==========================================================

void loop()
{
  Blynk.run();

  timer.run();
}