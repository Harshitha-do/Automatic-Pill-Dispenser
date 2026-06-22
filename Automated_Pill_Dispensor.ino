#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// --------------------------------------
// Hardware objects
// --------------------------------------
RTC_DS3231 rtc;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
Servo servo;
SoftwareSerial BT(9, 10);   // RX, TX (HC-05 TX -> 9, RX -> 10 via divider)

// Pins
const int SERVO_PIN  = 6;
const int BUZZER_PIN = 8;
const int IR_PIN     = 7;

// --------------------------------------
// Dose slot structure
// --------------------------------------
struct TimeSlot {
  int hour;
  int minute;
  bool enabled;
  bool firstAlertDone;
  bool secondAlertDone;
  bool dispensed;
  bool missed;
  unsigned long firstAlertMillis;
  unsigned long secondAlertMillis;
};

// 3 doses: morning/afternoon/night
TimeSlot slots[3] = {
  {0,0,false,false,false,false,false,0,0},
  {0,0,false,false,false,false,false,0,0},
  {0,0,false,false,false,false,false,0,0}
};

String btBuffer = "";
int currentAngle = 0;
unsigned long waitDuration = 20000;  // 20 seconds

// --------------------------------------
// Setup
// --------------------------------------
void setup() {
  Serial.begin(9600);
  BT.begin(9600);

  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("RTC NOT FOUND!");
    while (1);
  }

  // Use this only once to set RTC manually, then comment it
  // rtc.adjust(DateTime(2025, 2, 17, 14, 50, 0));

  servo.attach(SERVO_PIN);
  servo.write(0);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(IR_PIN, INPUT);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Smart Pill Box");
  delay(2000);
  lcd.clear();

  Serial.println("System Started");
  Serial.println("Waiting for Bluetooth dose times...");
  BT.println("CONNECTED");
  BT.println("Send: HH:MM,HH:MM,HH:MM");
}

// --------------------------------------
// Main loop
// --------------------------------------
void loop() {
  DateTime now = rtc.now();

  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  printTime(now);

  // check each dose slot
  for (int i = 0; i < 3; i++) {
    handleSlot(slots[i], now, i);
  }

  readBluetooth();

  delay(150);
}

// --------------------------------------
// Bluetooth reading (your format: "HH:MM,HH:MM,HH:MM")
// --------------------------------------
void readBluetooth() {
  while (BT.available()) {
    char c = BT.read();
    Serial.print(c);  // debug: see raw chars

    if (c == '\n' || c == '\r') {
      if (btBuffer.length() > 0) {
        btBuffer.trim();
        Serial.print("\nReceived line: ");
        Serial.println(btBuffer);
        parseDoseTimes(btBuffer);
        btBuffer = "";
      }
    } else {
      btBuffer += c;
    }
  }
}

// Parse: "14:30,18:00,22:15"
void parseDoseTimes(String data) {
  int slotIndex = 0;
  int start = 0;

  while (slotIndex < 3 && start < data.length()) {
    int commaPos = data.indexOf(',', start);
    String token;
    if (commaPos == -1) {
      token = data.substring(start);
    } else {
      token = data.substring(start, commaPos);
    }
    token.trim();

    if (token.length() >= 4) {       // at least "H:MM" or "HH:MM"
      int h = token.substring(0, 2).toInt();
      int m = token.substring(3, 5).toInt();

      slots[slotIndex].hour   = h;
      slots[slotIndex].minute = m;
      slots[slotIndex].enabled         = true;
      slots[slotIndex].firstAlertDone  = false;
      slots[slotIndex].secondAlertDone = false;
      slots[slotIndex].dispensed       = false;
      slots[slotIndex].missed          = false;

      Serial.print("Slot ");
      Serial.print(slotIndex + 1);
      Serial.print(" set to ");
      Serial.print(h);
      Serial.print(":");
      Serial.println(m);
    }

    slotIndex++;

    if (commaPos == -1) break;
    start = commaPos + 1;
  }

  Serial.println("All doses updated.");
}

// --------------------------------------
// Slot logic (first + second alert)
// --------------------------------------
void handleSlot(TimeSlot &slot, DateTime now, int type) {
  if (!slot.enabled) return;

  // First alert at matching HH:MM (ignore seconds)
  if (!slot.firstAlertDone &&
      now.hour() == slot.hour &&
      now.minute() == slot.minute) {

    Serial.print("FIRST ALERT for slot ");
    Serial.println(type + 1);

    lcd.clear();
    if (type == 0) lcd.print("Morning Dose");
    if (type == 1) lcd.print("Afternoon Dose");
    if (type == 2) lcd.print("Night Dose");

    alertBuzzer(5);

    lcd.setCursor(0, 1);
    lcd.print("Place hand...");

    slot.firstAlertDone = true;
    slot.firstAlertMillis = millis();
  }

  // Wait after first alert
  if (slot.firstAlertDone && !slot.secondAlertDone && !slot.dispensed) {
    if (digitalRead(IR_PIN) == LOW) {
      Serial.println("Hand detected after 1st alert");
      dispense(slot);
      return;
    }

    if (millis() - slot.firstAlertMillis >= waitDuration) {
      Serial.print("SECOND ALERT for slot ");
      Serial.println(type + 1);

      lcd.clear();
      lcd.print("Second Alert");
      alertBuzzer(5);
      lcd.setCursor(0, 1);
      lcd.print("Place hand...");
      slot.secondAlertDone = true;
      slot.secondAlertMillis = millis();
    }
  }

  // Wait after second alert
  if (slot.secondAlertDone && !slot.dispensed && !slot.missed) {
    if (digitalRead(IR_PIN) == LOW) {
      Serial.println("Hand detected after 2nd alert");
      dispense(slot);
      return;
    }

    if (millis() - slot.secondAlertMillis >= waitDuration) {
      Serial.print("MISSED DOSE slot ");
      Serial.println(type + 1);
      lcd.clear();
      lcd.print("MISSED DOSE");
      slot.missed = true;
      delay(2000);
      lcd.clear();
    }
  }
}

// --------------------------------------
// Dispense medicine via servo
// --------------------------------------
void dispense(TimeSlot &slot) {
  currentAngle += 30;
  if (currentAngle > 180) currentAngle = 180;
  servo.write(currentAngle);

  successTone();

  lcd.clear();
  lcd.print("Medicine Given");
  delay(2000);
  lcd.clear();

  slot.dispensed = true;
}

// --------------------------------------
// Time display on LCD
// --------------------------------------
void printTime(DateTime t) {
  lcd.setCursor(6, 0);
  lcd.print("        ");      // clear old
  lcd.setCursor(6, 0);
  lcd.print(format2(t.hour()));
  lcd.print(":");
  lcd.print(format2(t.minute()));
  lcd.print(":");
  lcd.print(format2(t.second()));
}

String format2(int n) {
  if (n < 10) return "0" + String(n);
  return String(n);
}

// --------------------------------------
// Buzzer functions
// --------------------------------------
void alertBuzzer(int n) {
  for (int i = 0; i < n; i++) {
    tone(BUZZER_PIN, 1000);
    delay(300);
    noTone(BUZZER_PIN);
    delay(150);
  }
}

void successTone() {
  tone(BUZZER_PIN, 1500);
  delay(300);
  noTone(BUZZER_PIN);
}