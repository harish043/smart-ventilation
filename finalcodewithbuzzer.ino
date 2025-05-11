#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
unsigned long lastLCDUpdate = 0;
int lcdScreenIndex = 0;

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// DHT22 Setup
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// HC-SR04 Setup
#define TRIG_PIN 9
#define ECHO_PIN 10

// MQ-5 Gas Sensor
#define MQ5_PIN A0

// Motor (Fan) Setup
#define MOTOR_PIN 3  // PWM pin

//buzzer setup
#define BUZZER_PIN 8  // Connect buzzer + to pin 8

void setup() {
  Serial.begin(9600);

  dht.begin();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MOTOR_PIN, OUTPUT);

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);  // Make sure it's off at start

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
}

void loop() {
  // Read Distance from HC-SR04
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2;

  // Read Gas Level from MQ-5
  int gasValue = analogRead(MQ5_PIN);
  // int gasValue = 101;

  // Read Temperature and Humidity from DHT22
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Fan control logic
  int motorSpeed = 0;
  String fanLevel = "OFF";

  if (distance < 25) {
    if (!isnan(temp)) {
      motorSpeed = map((int)temp, 29, 35, 125, 255);
      motorSpeed = constrain(motorSpeed, 0, 255);

      // Boost fan speed for high humidity
      if (!isnan(humidity) && humidity > 72) {
        motorSpeed = constrain(motorSpeed + 30, 0, 255);
        fanLevel = "HUMID+";
      }

      // Set fan level
      if (fanLevel != "HUMID+") {
        if (motorSpeed > 210) fanLevel = "HIGH";
        else if (motorSpeed > 160) fanLevel = "MEDIUM";
        else if (motorSpeed >= 80) fanLevel = "LOW";
        else fanLevel = "OFF";
      }
    } else {
      motorSpeed = 0;
      fanLevel = "TEMP ERR";
    }
  } else {
    motorSpeed = 0;
    fanLevel = "OFF";
  }

  if (gasValue > 500) {
    motorSpeed = 255;
    fanLevel = "GAS HIGH";
    tone(BUZZER_PIN, 3000);  // High-pitch sound when gas is high
  } else {
    noTone(BUZZER_PIN);      // Turn off buzzer if gas is normal
  }



  analogWrite(MOTOR_PIN, motorSpeed);

  // Debug output
  Serial.print("Distance: ");
  if (distance > 25) Serial.print("NA");
  else Serial.print(distance);
  Serial.print(" cm, Gas: ");
  Serial.print(gasValue);
  Serial.print(", Temp: ");
  Serial.print(temp);
  Serial.print(" C, Hum: ");
  Serial.print(humidity);
  Serial.print(" %, Fan: ");
  Serial.print(motorSpeed);
  Serial.print(" (");
  Serial.print(fanLevel);
  Serial.println(")");

  // Abbreviated fan level
  String fanAbbrev = "";
  if (fanLevel == "HIGH") fanAbbrev = "HI";
  else if (fanLevel == "MEDIUM") fanAbbrev = "MD";
  else if (fanLevel == "LOW") fanAbbrev = "LO";
  else if (fanLevel == "OFF") fanAbbrev = "OF";
  else if (fanLevel == "GAS HIGH") fanAbbrev = "GH";
  else if (fanLevel == "HUMID+") fanAbbrev = "HU";
  else fanAbbrev = "??";

  // LCD Display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temp, 1);
  lcd.print((char)223);
  lcd.print("C G:");
  lcd.print(gasValue);

  lcd.setCursor(0, 1);
  lcd.print("D:");
  if (distance > 25) lcd.print("NA");
  else lcd.print((int)distance);
  // lcd.print((int)distance);
  lcd.print("cm F:");
  lcd.print(fanAbbrev);
  delay(2000);
}