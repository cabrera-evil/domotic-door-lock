#include <LiquidCrystal.h>
#include <Keypad.h>
#include <Servo.h>

// LCD display initialization (RS, Enable, D4, D5, D6, D7)
LiquidCrystal lcd(12, 11, 14, 15, 16, 17);

// Keypad configuration - 4x4 matrix keypad
const byte ROWS = 4;  // Number of rows in keypad
const byte COLS = 4;  // Number of columns in keypad

// Pin assignments for keypad rows and columns
byte rowPins[ROWS] = {9, 8, 7, 6};    // Row pins connected to Arduino
byte colPins[COLS] = {5, 4, 3, 2};    // Column pins connected to Arduino

// Key layout matrix for 4x4 keypad
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Initialize keypad object
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Servo motor for door lock mechanism
Servo doorServo;

// Pin assignments for various components
const int SERVO_PIN = 10;  // Servo motor control pin
const int PIR_PIN   = A4;  // PIR motion sensor input pin
const int LED_R = A5;      // Red LED pin for RGB status indicator
const int LED_G = 13;      // Green LED pin for RGB status indicator
const int LED_B = 0;       // Blue LED pin for RGB status indicator

// Security PIN configuration
const int PIN_LENGTH = 4;                    // Length of the security PIN
int correctPin[PIN_LENGTH] = {1, 2, 3, 4};  // The correct PIN (changeable)
int enteredPin[PIN_LENGTH];                  // Array to store user input
int pinIndex = 0;                            // Current position in PIN entry

// LED color constants for status indication
const uint8_t LED_OFF   = 0;  // All LEDs off
const uint8_t LED_WHITE = 1;  // White (system armed/idle)
const uint8_t LED_BLUE  = 2;  // Blue (motion detected, waiting for PIN)
const uint8_t LED_GREEN = 3;  // Green (access granted)
const uint8_t LED_RED   = 4;  // Red (access denied)

// System state enumeration
enum SystemState { Idle, Active };
SystemState systemState = Idle;  // Current system state

// Timing variables for system behavior
unsigned long lastKeypressTime = 0;               // Last time a key was pressed
unsigned long firstMotionTime = 0;                // First time motion was detected
const unsigned long KEYPRESS_TIMEOUT = 10000;    // Timeout for PIN entry (10 seconds)
const unsigned long MOTION_TRIGGER_DELAY = 500;  // Delay before activating on motion (500ms)

/**
 * Set RGB LED color by individual component control
 * @param r Red component state (true = on, false = off)
 * @param g Green component state (true = on, false = off)
 * @param b Blue component state (true = on, false = off)
 */
void setRgb(bool r, bool g, bool b) {
  // First turn off all LEDs to prevent color mixing
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);

  // Small delay to ensure clean switching
  delayMicroseconds(100);

  // Set the desired color combination
  digitalWrite(LED_R, r ? HIGH : LOW);
  digitalWrite(LED_G, g ? HIGH : LOW);
  digitalWrite(LED_B, b ? HIGH : LOW);
}

/**
 * Set LED to predefined color based on system status
 * @param mode LED color mode constant (LED_WHITE, LED_BLUE, LED_GREEN, LED_RED, LED_OFF)
 */
void setLed(uint8_t mode) {
  switch (mode) {
    case LED_WHITE: setRgb(true,  true,  true);  break;  // System idle/armed
    case LED_BLUE:  setRgb(false, false, true);  break;  // Motion detected
    case LED_GREEN: setRgb(false, true,  false); break;  // Access granted
    case LED_RED:   setRgb(true,  false, false); break;  // Access denied
    default:        setRgb(false, false, false); break;  // All off
  }
}

/**
 * Display text on specified LCD row, clearing the line first
 * @param row LCD row number (0 or 1 for 16x2 display)
 * @param text Text string to display (up to 16 characters)
 */
void lcdLine(byte row, const char* text) {
  lcd.setCursor(0, row);
  lcd.print("                ");  // Clear the entire row with spaces
  lcd.setCursor(0, row);
  lcd.print(text);                // Display the new text
}

/**
 * Reset PIN entry state and display empty PIN prompt
 */
void resetPinEntry() {
  pinIndex = 0;           // Reset PIN entry position to beginning
  lcdLine(1, "PIN: ");    // Display empty PIN prompt
}

/**
 * Set system to idle state - waiting for motion detection
 * Clears all timing variables and displays armed status
 */
void goIdle() {
  systemState = Idle;              // Set system to idle state
  setLed(LED_WHITE);               // White LED indicates system is armed
  lcdLine(0, "System armed");      // Display armed status
  lcdLine(1, "Waiting motion");    // Display waiting message
  pinIndex = 0;                    // Reset PIN entry
  firstMotionTime = 0;             // Clear motion timing
  lastKeypressTime = 0;            // Clear keypress timing
}

/**
 * Set system to active state - motion detected, ready for PIN entry
 * Starts keypress timeout timer and displays PIN entry prompt
 */
void goActive() {
  systemState = Active;            // Set system to active state
  setLed(LED_BLUE);                // Blue LED indicates motion detected
  lcdLine(0, "Motion detected");   // Display motion detection status
  lcdLine(1, "PIN: ");             // Display PIN entry prompt
  pinIndex = 0;                    // Reset PIN entry position
  lastKeypressTime = millis();     // Start timeout timer
}

/**
 * Check if a keypad character is a digit (0-9)
 * @param key Character to check
 * @return true if key is a digit, false otherwise
 */
bool isDigitKey(char key) {
  return key >= '0' && key <= '9';
}

/**
 * Verify if the entered PIN matches the correct PIN
 * @return true if PIN is correct and complete, false otherwise
 */
bool isPinCorrect() {
  // PIN must be completely entered
  if (pinIndex != PIN_LENGTH) return false;
  
  // Compare each digit of entered PIN with correct PIN
  for (int i = 0; i < PIN_LENGTH; i++) {
    if (enteredPin[i] != correctPin[i]) return false;
  }
  return true;
}

/**
 * Handle successful PIN entry - grant access and operate door lock
 * Changes LED to green, displays success message, operates servo, then returns to idle
 */
void handlePinSuccess() {
  setLed(LED_GREEN);               // Green LED indicates access granted
  lcdLine(0, "Access granted");    // Display success message
  lcdLine(1, "Opening door...");   // Display door operation status
  doorServo.write(90);             // Rotate servo to unlock position
  delay(5000);                     // Keep door unlocked for 5 seconds
  doorServo.write(0);              // Return servo to locked position
  goIdle();                        // Return system to idle state
}

/**
 * Handle incorrect PIN entry - deny access and allow retry
 * Changes LED to red, displays denial message, then resets for new attempt
 */
void handlePinFailure() {
  setLed(LED_RED);                 // Red LED indicates access denied
  lcdLine(0, "Access denied");     // Display denial message
  lcdLine(1, "Try again");         // Prompt for retry
  delay(2000);                     // Display denial message for 2 seconds
  setLed(LED_BLUE);                // Return to blue (motion detected) state
  lcdLine(0, "Motion detected");   // Restore motion detected message
  resetPinEntry();                 // Clear PIN entry for new attempt
  lastKeypressTime = millis();     // Reset timeout timer
}

/**
 * Verify entered PIN and handle the result (success or failure)
 * Called when user presses '#' to submit PIN
 */
void handlePinVerification() {
  if (isPinCorrect()) {
    handlePinSuccess();  // Correct PIN - grant access
  } else {
    handlePinFailure();  // Incorrect PIN - deny access
  }
}

/**
 * Handle keypad input when system is in active state
 * Processes digit entry, PIN submission (#), and PIN reset (*)
 */
void handleKeypad() {
  char key = keypad.getKey();
  if (!key) return;  // No key pressed, exit function

  lastKeypressTime = millis();  // Update last keypress time for timeout

  if (key == '#') {
    // Submit PIN for verification
    handlePinVerification();
  } else if (key == '*') {
    // Reset/clear current PIN entry
    resetPinEntry();
  } else if (isDigitKey(key) && pinIndex < PIN_LENGTH) {
    // Add digit to PIN entry if there's space
    enteredPin[pinIndex] = key - '0';  // Convert char to integer
    pinIndex++;

    // Update LCD display with asterisks for security
    lcd.setCursor(0, 1);
    lcd.print("PIN: ");
    for (int i = 0; i < pinIndex; i++) lcd.print('*');        // Show asterisks for entered digits
    for (int i = pinIndex; i < PIN_LENGTH; i++) lcd.print(' '); // Clear remaining positions
  }
}

/**
 * Monitor PIR sensor and handle motion detection when system is idle
 * Implements motion trigger delay to prevent false positives
 */
void updatePIRState() {
  if (systemState != Idle) return;  // Only check motion when system is idle

  int pirState = digitalRead(PIR_PIN);  // Read current PIR sensor state

  if (pirState == HIGH) {
    // Motion detected
    if (firstMotionTime == 0) {
      // First motion detection - start timing
      firstMotionTime = millis();
    }

    // Check if motion has been sustained for the required delay
    if (millis() - firstMotionTime >= MOTION_TRIGGER_DELAY) {
      goActive();  // Activate system for PIN entry
    }
  } else {
    // No motion detected - reset timing
    firstMotionTime = 0;
  }
}

/**
 * Check for keypress timeout when system is active
 * Returns system to idle if no keys pressed within timeout period
 */
void checkKeypressTimeout() {
  if (systemState != Active) return;  // Only check timeout when system is active

  // Check if timeout period has elapsed since last keypress
  if (millis() - lastKeypressTime > KEYPRESS_TIMEOUT) {
    lcdLine(0, "Timeout");        // Display timeout message
    lcdLine(1, "Returning...");   // Display return message
    delay(1000);                  // Brief delay to show message
    goIdle();                     // Return to idle state
  }
}

/**
 * Arduino setup function - initialize all components and set initial state
 * Called once when the Arduino starts up
 */
void setup() {
  // Initialize LCD display (16 columns, 2 rows)
  lcd.begin(16, 2);

  // Initialize servo motor and set to locked position
  doorServo.attach(SERVO_PIN);
  doorServo.write(0);  // Start in locked position (0 degrees)

  // Configure pin modes for sensors and LEDs
  pinMode(PIR_PIN, INPUT);   // PIR motion sensor as input
  pinMode(LED_R, OUTPUT);    // Red LED as output
  pinMode(LED_G, OUTPUT);    // Green LED as output
  pinMode(LED_B, OUTPUT);    // Blue LED as output

  // Set system to initial idle state
  goIdle();
}

/**
 * Arduino main loop function - continuously monitor system state
 * Called repeatedly while the Arduino is running
 */
void loop() {
  updatePIRState();        // Monitor motion sensor for activation
  checkKeypressTimeout();  // Check for PIN entry timeout

  if (systemState == Active) {
    // System is active - process keypad input for PIN entry
    handleKeypad();
  } else {
    // System is idle - clear any pending keypad input
    keypad.getKey();
  }
}
