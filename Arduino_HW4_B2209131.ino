#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <DHT.h>

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= Mode Switch =================
// DIP switch: S0=D12, S1=D13 (INPUT_PULLUP)
const int MODE_S0 = 12;
const int MODE_S1 = 13;

// ================= Mode1: Servo + Joystick =================
Servo servo1, servo2;
const int SERVO1_PIN = 10;
const int SERVO2_PIN = 11;

const int JOY_H_PIN = A1; // X
const int JOY_V_PIN = A2; // Y
const int JOY_PB_PIN = A0; // optional

// ================= Mode2: Line Sensors (Digital DO) =================
const int IR_L = 2;
const int IR_C = 3;
const int IR_R = 4;

const bool BLACK_IS_LOW = true;

// ================= Mode3: Ultrasonic + DHT =================
const int TRIG_PIN = 7;
const int ECHO_PIN = 6;

#define DHTPIN 8
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);

// ================= Utilities =================
int clampInt(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

int readMode() {
  // INPUT_PULLUP：撥到 ON 接地 -> LOW
  int s0 = (digitalRead(MODE_S0) == LOW) ? 1 : 0;
  int s1 = (digitalRead(MODE_S1) == LOW) ? 1 : 0;
  int code = (s1 << 1) | s0;

  if (code == 0) return 1; // 00
  if (code == 1) return 2; // 01
  return 3;                // 10/11
}

int toAngle(int analogValue) {
  int ang = (int)map(analogValue, 0, 1023, 0, 180);
  return clampInt(ang, 0, 180);
}

bool onBlack(int pin) {
  int v = digitalRead(pin);
  if (BLACK_IS_LOW) return (v == LOW);
  else return (v == HIGH);
}

const char* getPosition(bool L, bool C, bool R) {
  // 依照三顆循跡判斷位置
  if (!L && C && !R) return "Normal";
  if (!L && C && R)  return "Slightly Right";
  if (!L && !C && R) return "Right";
  if (L && C && !R)  return "Slightly Left";
  if (L && !C && !R) return "Left";
  return "Abnormal";
}

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (duration == 0) return -1;

  return (duration * 0.0343f) / 2.0f;
}

// ================= Display Helpers =================
void lcdPrintCleanLine(int col, int row, const String &text) {
  lcd.setCursor(col, row);
  lcd.print(text);
  // 清掉殘字
  int len = text.length();
  for (int i = len; i < 16 - col; i++) lcd.print(" ");
}

// ================= Mode1 =================
void runMode1() {
  int hVal = analogRead(JOY_H_PIN);
  int vVal = analogRead(JOY_V_PIN);

  int xAng = toAngle(hVal);
  int yAng = toAngle(vVal);

  servo1.write(xAng);
  servo2.write(yAng);

  lcdPrintCleanLine(0, 0, "Mode1");

  lcd.setCursor(0, 1);
  lcd.print("X:");
  if (xAng < 100) lcd.print(' ');
  if (xAng < 10)  lcd.print(' ');
  lcd.print(xAng);

  lcd.print("  Y:");
  if (yAng < 100) lcd.print(' ');
  if (yAng < 10)  lcd.print(' ');
  lcd.print(yAng);
  lcd.print("   ");
}

// ================= Mode2 =================
void runMode2() {
  bool L = onBlack(IR_L);
  bool C = onBlack(IR_C);
  bool R = onBlack(IR_R);

  const char* pos = getPosition(L, C, R);

  lcdPrintCleanLine(0, 0, "Mode2");

  // 第二行顯示 Position
  String show = pos;

  if (show == "Slightly Right") show = "S-Right";
  else if (show == "Slightly Left") show = "S-Left";
  else if (show == "Abnormal") show = "Abnormal";

  lcd.setCursor(0, 1);
  lcd.print("Position:");
  lcd.setCursor(9, 1);
  lcd.print("       ");
  lcd.setCursor(9, 1);
  lcd.print(show);
}

// ================= Mode3 =================
void runMode3() {
  float dist = readDistanceCm();
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // Line1: Distance
  lcd.setCursor(0, 0);
  lcd.print("Distance:");
  if (dist < 0) {
    lcd.print("--");
  } else {
    int d = (int)(dist + 0.5f);
    lcd.print(d);
  }
  lcd.print("cm   "); // 清殘字

  // Line2: Temp + Humid
  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  if (isnan(t)) lcd.print("--");
  else lcd.print((int)(t + 0.5f));

  lcd.print(" Hum:");
  if (isnan(h)) lcd.print("--");
  else lcd.print((int)(h + 0.5f));
  lcd.print(" ");
}

void setup() {
  // Mode switch
  pinMode(MODE_S0, INPUT_PULLUP);
  pinMode(MODE_S1, INPUT_PULLUP);

  // Mode2 sensors
  pinMode(IR_L, INPUT);
  pinMode(IR_C, INPUT);
  pinMode(IR_R, INPUT);

  // Ultrasonic
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Servo
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);

  // DHT
  dht.begin();

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcdPrintCleanLine(0, 0, "System Booting...");
  delay(800);
  lcd.clear();
}

void loop() {
  int mode = readMode();

  if (mode == 1) runMode1();
  else if (mode == 2) runMode2();
  else runMode3();

  delay(150);
}











