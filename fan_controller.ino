#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// settings
#define dht_pin 7
#define dht_type DHT11
#define fan_pin 8

// LED pins
#define led_green 2
#define led_yellow 3
#define led_red 4

// input pins
#define touch_pin 6
#define reverse_button 10

DHT dht(dht_pin, dht_type);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// modes:
// 1 = sensor
// 2 = mock
// 3 = AI (sensor)
// 4 = AI (mock)
int mode = 1;
bool lastTouch = LOW;
bool lastReverse = LOW;

// mock data
float mockTemps[] = {70, 72, 75, 78, 81, 84, 87, 90, 87, 84, 78};
int mockIndex = 0;
int mockCount = 11;
float lastTempF = 75.0;

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Temp System Init");
  delay(2000);
  lcd.clear();

  pinMode(fan_pin, OUTPUT);
  pinMode(led_green, OUTPUT);
  pinMode(led_yellow, OUTPUT);
  pinMode(led_red, OUTPUT);
  pinMode(touch_pin, INPUT);
  pinMode(reverse_button, INPUT_PULLUP);
}

void loop() {
  // python commands
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "REQ_MODE") {
      Serial.print("MODE:");
      Serial.println(mode);
      return;
    }
    if (cmd == "REQ_TEMP") {
      Serial.println(lastTempF);
      return;
    }
    if (mode == 3 || mode == 4) {
      if (cmd == "OFF")  analogWrite(fan_pin, 0);
      if (cmd == "LOW")  analogWrite(fan_pin, 80);
      if (cmd == "HIGH") analogWrite(fan_pin, 255);
    }
  }

  // forward cycle
  bool t = digitalRead(touch_pin);
  if (t == HIGH && lastTouch == LOW) {
    mode++;
    if (mode > 4) mode = 1;
    delay(150);
  }
  lastTouch = t;

  // backward cycle
  bool rev = (digitalRead(reverse_button) == LOW);
  if (rev && lastReverse == LOW) {
    mode--;
    if (mode < 1) mode = 4;
    delay(150);
  }
  lastReverse = rev;

  float tempF = lastTempF;

  // get temperature
  if (mode == 1 || mode == 3) {
    float tempC = dht.readTemperature();
    if (!isnan(tempC)) {
      tempF = tempC * 9.0 / 5.0 + 32.0;
    }
  } else {
    tempF = mockTemps[mockIndex];
    mockIndex = (mockIndex + 1) % mockCount;
  }
  lastTempF = tempF;

  // LCD output
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(tempF, 1);
  lcd.print("F   ");
  lcd.setCursor(0, 1);
  switch (mode) {
    case 1: lcd.print("Mode: SENSOR    "); break;
    case 2: lcd.print("Mode: MOCK      "); break;
    case 3: lcd.print("AI (SENSOR)    ");  break;
    case 4: lcd.print("AI (MOCK)      ");  break;
  }

  // fan + LEDs (normal modes)
  if (mode == 1 || mode == 2) {
    if (tempF < 75.0) {
      analogWrite(fan_pin, 0);
      digitalWrite(led_red, HIGH);
      digitalWrite(led_green, LOW);
      digitalWrite(led_yellow, LOW);
    } else if (tempF <= 85.0) {
      analogWrite(fan_pin, 105);
      digitalWrite(led_red, LOW);
      digitalWrite(led_green, HIGH);
      digitalWrite(led_yellow, LOW);
    } else {
      analogWrite(fan_pin, 255);
      digitalWrite(led_red, LOW);
      digitalWrite(led_green, LOW);
      digitalWrite(led_yellow, HIGH);
    }
  }

  // LEDs (AI modes)
  if (mode == 3 || mode == 4) {
    if (tempF < 75.0) {
      digitalWrite(led_red, HIGH);
      digitalWrite(led_green, LOW);
      digitalWrite(led_yellow, LOW);
    } else if (tempF <= 85.0) {
      digitalWrite(led_red, LOW);
      digitalWrite(led_green, HIGH);
      digitalWrite(led_yellow, LOW);
    } else {
      digitalWrite(led_red, LOW);
      digitalWrite(led_green, LOW);
      digitalWrite(led_yellow, HIGH);
    }
  }

  delay(2500);
}
