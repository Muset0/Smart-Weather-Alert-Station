
#include <Adafruit_LiquidCrystal.h>

// Pins
const uint8_t PIN_TMP    = A0;
const uint8_t PIN_LIGHT  = A1;
const uint8_t PIN_BUZZER = 8;
const uint8_t PIN_LED    = 9;
const uint8_t PIN_BTN    = 2;

// Thresholds
const float TEMP_HIGH  = 30.0;
const float TEMP_LOW   = 10.0;
const int   LIGHT_DARK = 150;

// Timing
const uint16_t READ_INTERVAL  = 2000;
const uint16_t BLINK_INTERVAL = 500;
const uint16_t BEEP_DURATION  = 150;

// LCD
Adafruit_LiquidCrystal lcd(0);

// Globals
float gTemp = 0.0;
int   gLight = 0;
bool  gAlert = false;
uint8_t gScreen = 0;
volatile bool gBtnFlag = false;

uint32_t tLastRead = 0;
uint32_t tLastBlink = 0;
uint32_t tBeepEnd = 0;
bool ledState = false;

// Degree symbol (NOT const)
byte DEGREE_CHAR[8] = {
  0b00110,
  0b01001,
  0b01001,
  0b00110,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

// Button ISR
void onButton() {
  gBtnFlag = true;
}

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BTN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_BTN), onButton, FALLING);

  lcd.begin(16, 2);
  lcd.setBacklight(1);
  lcd.createChar(0, DEGREE_CHAR);

  lcd.setCursor(3, 0);
  lcd.print("WeatherBox");
  lcd.setCursor(2, 1);
  lcd.print("Starting...");
  delay(1500);
  lcd.clear();
}

void loop() {
  uint32_t now = millis();

  if (gBtnFlag) {
    gBtnFlag = false;
    gScreen = (gScreen + 1) % 3;
    renderScreen();
  }

  if (now - tLastRead >= READ_INTERVAL) {
    tLastRead = now;
    readSensors();
    checkAlerts(now);
    renderScreen();
  }

  if (gAlert) {
    if (now - tLastBlink >= BLINK_INTERVAL) {
      tLastBlink = now;
      ledState = !ledState;
      digitalWrite(PIN_LED, ledState);
    }
  } else if (ledState) {
    digitalWrite(PIN_LED, LOW);
    ledState = false;
  }

  if (tBeepEnd && now >= tBeepEnd) {
    noTone(PIN_BUZZER);
    tBeepEnd = 0;
  }
}

// Sensors 
void readSensors() {
  int raw = analogRead(PIN_TMP);
  gTemp = (raw * (500.0 / 1023.0)) - 50.0;
  gLight = analogRead(PIN_LIGHT);
}

// Alerts
void checkAlerts(uint32_t now) {
  bool prev = gAlert;
  gAlert = (gTemp >= TEMP_HIGH || gTemp <= TEMP_LOW || gLight <= LIGHT_DARK);

  if (gAlert && !prev) {
    tone(PIN_BUZZER, 1200);
    tBeepEnd = now + BEEP_DURATION;
  }
}

// LCD Screens
void renderScreen() {
  lcd.clear();
  if (gScreen == 0) screenTemp();
  else if (gScreen == 1) screenLight();
  else screenStatus();
}

void screenTemp() {
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(gTemp, 1);
  lcd.write(byte(0));
  lcd.print("C");

  lcd.setCursor(0, 1);
  if (gTemp >= TEMP_HIGH) lcd.print("!! TOO HOT !!");
  else if (gTemp <= TEMP_LOW) lcd.print("!! TOO COLD !!");
  else lcd.print("Status: Normal");
}

void screenLight() {
  lcd.setCursor(0, 0);
  lcd.print("Light ADC: ");
  lcd.print(gLight);

  lcd.setCursor(0, 1);
  if (gLight > 700) lcd.print("Level: Bright");
  else if (gLight > 500) lcd.print("Level: Normal");
  else if (gLight > 350) lcd.print("Level: Dim");
  else lcd.print("Level: DARK");
}

void screenStatus() {
  lcd.setCursor(0, 0);
  lcd.print("Overall Status");
  lcd.setCursor(0, 1);

  if (!gAlert) lcd.print("All Good :)");
  else if (gTemp >= TEMP_HIGH) lcd.print("HIGH TEMP");
  else if (gTemp <= TEMP_LOW) lcd.print("LOW TEMP");
  else lcd.print("DARK ENV");
}