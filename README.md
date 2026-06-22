# Automatic Pill Dispenser — Android + Arduino Project

A complete IoT-based Smart Pill Dispenser that alerts patients at scheduled times, detects whether the hand is placed to take medicine, automatically dispenses doses, and marks missed doses.
The system includes:

 Android App (Set dose timings via Bluetooth)
 Arduino Device (Real-time clock + servo + IR sensor + buzzer)
 Bluetooth Communication (HC-05 module)
 LCD Display (Status, alerts, and time)

## Features

 ### Android App

* Connects to HC-05 Bluetooth module
* Lets the user select up to 3 dose times
* Automatically pads empty timings: `"00:00"`
* Sends all doses in one message:
 HH:MM,HH:MM,HH:MM\n
* Syncs phone time (IST) to Arduino RTC
* Minimal, clean UI designed for elderly use

 ### Arduino Device

* Uses DS3231 RTC for accurate timekeeping
* First alert (buzzer + LCD) when dose time matches
* IR sensor detects hand presence
* Second alert if user does not respond
* Marks dose as dispensed or missed
* Servo rotates to drop medicine
* LCD shows live time & status
* Resets daily at midnight
  
 ### Hardware Components

| Component          | Description                 |
| ------------------ | --------------------------- |
| Arduino UNO / Nano | Main microcontroller        |
| HC-05 Module       | Bluetooth communication     |
| DS3231 RTC         | Real-time clock module      |
| SG90 / MG995 Servo | Medicine dispenser          |
| IR Sensor          | Detects hand presence       |
| Buzzer             | Alerts the user             |
| 16×2 LCD + I2C     | Display for time & messages |
| Power supply       | 5V regulated                |

### Android App Tech Stack

* Java (Android Studio)
* Bluetooth Classic (RFCOMM)
* Material UI (XML layout)
* ScrollView-based interface
* TimePicker for selecting doses
  
### Bluetooth Data Format

The app sends exactly three times every time:
HH:MM,HH:MM,HH:MM\n

Examples:
* User selects 1 dose →
  `14:20,00:00,00:00`
* User selects 2 doses →
  `08:30,20:15,00:00`
* User selects 3 doses →
  `09:00,14:00,21:00`

Arduino ignores `"00:00"` (disables slot).

 ### RTC Time Sync Command

Android → Arduino:
RTCTIME HH:MM:SS\n
Automatically sets real-time clock.

### Arduino Libraries Used

Wire.h  
RTClib.h  
LiquidCrystal.h  
Servo.h  
SoftwareSerial.h  

 ### How the System Works

1. User opens app → selects dose timings
2. App connects to HC-05
3. RTC auto-syncs from phone time
4. App sends `"HH:MM,HH:MM,HH:MM"`
5. Arduino:

   * Saves dose times
   * At exact time: activates First Alert
   * Waits 20 seconds
   * No IR detection → Second Alert
   * No response → marks Missed Dose
   * With IR → rotates servo to dispense

### Repository Structure

SmartPillDispenser/
│
├── android_app/
│   ├── app/src/main/java/com/example/timesettingapp/MainActivity.java
│   ├── app/src/main/res/layout/activity_main.xml
│   └── ...
│
├── arduino_code/
│   ├── SmartPillDispenser.ino
│   └── ...
│
├── README.md
└── LICENSE (optional)

 ### How to Run

#### Arduino
1. Install required libraries
2. Upload `.ino` file
3. Connect hardware
4. Power the board

#### Android
1. Open folder in Android Studio
2. Sync Gradle
3. Enable Bluetooth
4. Pair with HC-05 (PIN: `1234` or `0000`)
5. Run app on phone
6. Press Connect → Send Timings

 ### Future Enhancements
* Add dose history log
* Add automatic refill status
* Add mobile notifications
* Add Firebase sync
* Add voice reminder

