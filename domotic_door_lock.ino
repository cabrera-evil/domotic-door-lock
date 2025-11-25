#include <LiquidCrystal.h>
#include <Keypad.h>
#include <Servo.h>

LiquidCrystal lcd(12, 11, 14, 15, 16, 17);

const byte ROWS = 4;
const byte COLS = 4;

byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

Servo doorServo;

const int SERVO_PIN = 10;
const int PIR_PIN   = A4;

// Pines corregidos según tu prueba
const int LED_R = A1;  // A1 = Rojo
const int LED_B = A2;  // A2 = Azul
const int LED_G = A3;  // A3 = Verde

const int PIN_LENGTH = 4;
int correctPin[PIN_LENGTH] = {1, 2, 3, 4};
int enteredPin[PIN_LENGTH];
int pinIndex = 0;

const uint8_t LED_OFF    = 0;
const uint8_t LED_BLUE   = 1;
const uint8_t LED_YELLOW = 2;
const uint8_t LED_GREEN  = 3;
const uint8_t LED_RED    = 4;

enum SystemState { Idle, Active };
SystemState systemState = Idle;

unsigned long lastMotionTime = 0;
const unsigned long MOTION_TIMEOUT = 5000;

// Common cathode - HIGH enciende, LOW apaga
void setRgb(bool r, bool g, bool b) {
  digitalWrite(LED_R, r ? HIGH : LOW);
  digitalWrite(LED_G, g ? HIGH : LOW);
  digitalWrite(LED_B, b ? HIGH : LOW);
}

void setLed(uint8_t mode) {
  switch (mode) {
    case LED_BLUE:   setRgb(false, false, true);  break; // Azul: sistema armado
    case LED_YELLOW: setRgb(true, true, false);   break; // Amarillo: movimiento detectado
    case LED_GREEN:  setRgb(false, true, false);  break; // Verde: acceso concedido
    case LED_RED:    setRgb(true, false, false);  break; // Rojo: acceso denegado
    default:         setRgb(false, false, false); break; // Apagado
  }
}

void lcdLine(byte row, const char* text) {
  lcd.setCursor(0, row);
  lcd.print("                ");
  lcd.setCursor(0, row);
  lcd.print(text);
}

void resetPinEntry() {
  pinIndex = 0;
  lcdLine(1, "PIN: ");
}

void goIdle() {
  systemState = Idle;
  setLed(LED_BLUE);
  lcdLine(0, "System armed");
  lcdLine(1, "Waiting motion");
  pinIndex = 0;
}

void goActive() {
  systemState = Active;
  setLed(LED_YELLOW);
  lcdLine(0, "Motion detected");
  lcdLine(1, "PIN: ");
  pinIndex = 0;
  lastMotionTime = millis();
}

bool isDigitKey(char key) {
  return key >= '0' && key <= '9';
}

bool isPinCorrect() {
  if (pinIndex != PIN_LENGTH) return false;
  for (int i = 0; i < PIN_LENGTH; i++) {
    if (enteredPin[i] != correctPin[i]) return false;
  }
  return true;
}

void handlePinSuccess() {
  setLed(LED_GREEN);
  lcdLine(0, "Access granted");
  lcdLine(1, "Opening door...");
  doorServo.write(90);
  delay(5000);
  doorServo.write(0);
  goIdle();
}

void handlePinFailure() {
  setLed(LED_RED);
  lcdLine(0, "Access denied");
  lcdLine(1, "Try again");
  delay(2000);
  
  setLed(LED_YELLOW);
  lcdLine(0, "Motion detected");
  resetPinEntry();
}

void handlePinVerification() {
  if (isPinCorrect()) {
    handlePinSuccess();
  } else {
    handlePinFailure();
  }
}

void handleKeypad() {
  char key = keypad.getKey();
  if (!key) return;

  if (key == '#') {
    handlePinVerification();
  } else if (key == '*') {
    resetPinEntry();
  } else if (isDigitKey(key) && pinIndex < PIN_LENGTH) {
    enteredPin[pinIndex] = key - '0';
    pinIndex++;

    lcd.setCursor(0, 1);
    lcd.print("PIN: ");
    for (int i = 0; i < pinIndex; i++) lcd.print('*');
    for (int i = pinIndex; i < PIN_LENGTH; i++) lcd.print(' ');
  }
}

void updatePIRState() {
  int pirState = digitalRead(PIR_PIN);
  
  if (pirState == HIGH) {
    lastMotionTime = millis();
    if (systemState == Idle) {
      goActive();
    }
  }
  
  if (systemState == Active && 
      (millis() - lastMotionTime > MOTION_TIMEOUT) && 
      pinIndex == 0) {
    goIdle();
  }
}

void setup() {
  lcd.begin(16, 2);
  
  doorServo.attach(SERVO_PIN);
  doorServo.write(0);

  pinMode(PIR_PIN, INPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  
  goIdle();
}

void loop() {
  updatePIRState();
  if (systemState == Active) {
    handleKeypad();
  } else {
    keypad.getKey();
  }
}
