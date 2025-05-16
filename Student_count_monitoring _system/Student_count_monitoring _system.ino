#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>

#define IR_IN 5
#define IR_OUT 4
#define BUZZER 6
#define SD_CS 10

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C LCD address

int count = 0;
bool irInState = false;
bool irOutState = false;
File logFile;

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 1000; // 1 second
bool showingEvent = false;
unsigned long eventStartTime = 0;

const int EEPROM_ADDR = 0; // EEPROM address to store count

void setup() {
  Serial.begin(9600);
  pinMode(IR_IN, INPUT);
  pinMode(IR_OUT, INPUT);
  pinMode(BUZZER, OUTPUT);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Welcome to ");
  lcd.setCursor(0, 1);
  lcd.print("Stu Count System");
  delay(3000);
  lcd.clear();

  // Init RTC
  if (!rtc.begin()) {
    lcd.print("RTC not found!");
    while (1);
  }
  // rtc.adjust(DateTime(2025, 5, 11, 10, 57, 0)); // Optional: Set time once

  // Init SD card
  if (!SD.begin(SD_CS)) {
    lcd.print("SD init failed!");
    while (1);
  }

  // Load stored count from EEPROM
  EEPROM.get(EEPROM_ADDR, count);
  // Safety check to avoid corrupted values
  if (count < 0 || count > 1000) count = 0;

  lcd.print("System Ready");
  delay(2000);
  lcd.clear();
}

void loop() {
  unsigned long currentMillis = millis();

  bool irInDetected = digitalRead(IR_IN) == LOW;
  bool irOutDetected = digitalRead(IR_OUT) == LOW;

  // Entry detected
  if (irInDetected && !irInState) {
    irInState = true;
    count++;
    EEPROM.put(EEPROM_ADDR, count); // Store new count
    buzz();
    showMessage("   Welcome");
    delay(1000);
    logData("IN");
  }
  if (!irInDetected) irInState = false;

  // Exit detected
  if (irOutDetected && !irOutState) {
    irOutState = true;
    if (count > 0) count--;
    EEPROM.put(EEPROM_ADDR, count); // Store new count
    buzz();
    showMessage("    Goodbye");
    delay(1000);
    logData("OUT");
  }
  if (!irOutDetected) irOutState = false;

  // Show normal time & count if not in event message
  if (!showingEvent && currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;
    showTime();
  }

  // Return from welcome/goodbye after 2 seconds
  if (showingEvent && (currentMillis - eventStartTime >= 2000)) {
    showingEvent = false;
    lcd.clear();
    showTime();
  }
}

void buzz() {
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);
}

void showMessage(String msg) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msg);
  lcd.setCursor(3, 1);
  DateTime now = rtc.now();
  lcd.print(now.timestamp(DateTime::TIMESTAMP_TIME));
  showingEvent = true;
  eventStartTime = millis();
}

void showTime() {
  DateTime now = rtc.now();
  lcd.setCursor(4, 0);
  lcd.print(now.timestamp(DateTime::TIMESTAMP_TIME));
  lcd.setCursor(0, 1);
  lcd.print("Stdnts Count: ");
  lcd.print(count);
  lcd.print("   "); // Clear leftover digits
}

void logData(String action) {
  DateTime now = rtc.now();
  String dataString = String(now.timestamp(DateTime::TIMESTAMP_FULL)) +
                      ", Action: " + action +
                      ", Count: " + String(count);

  logFile = SD.open("log.txt", FILE_WRITE);
  if (logFile) {
    logFile.println(dataString);
    logFile.close();
    Serial.println("Logged: " + dataString);
  } else {
    Serial.println("Log failed!");
  }
}
