#include <TinyGPS++.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <Keypad.h>

// Pin Definitions
#define GAS_SENSOR_PIN A0
#define BUZZER_PIN A1
#define RELAY_PIN 11
#define FINGER_TX 10
#define FINGER_RX 9

// GPS & Fingerprint
TinyGPSPlus gps;
SoftwareSerial fingerSerial(FINGER_TX, FINGER_RX);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

// Keypad Setup
const byte ROWS = 4, COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String correctPassword = "0000";
float latt = 0.0, longg = 0.0;

void setup() {
  Serial.begin(9600);
  finger.begin(57600);

  pinMode(GAS_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RELAY_PIN, HIGH);

  if (!finger.verifyPassword()) while (1);

  digitalWrite(BUZZER_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_PIN, LOW);
}

void loop() {
  while (Serial.available()) gps.encode(Serial.read());

  if (gps.location.isValid()) {
    latt = gps.location.lat();
    longg = gps.location.lng();
  } else {
    latt = 0.0;
    longg = 0.0;
  }

  unsigned long startTime = millis();
  bool fingerMatched = false;

  while (millis() - startTime < 5000) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
    delay(150);
    if (checkFingerprint()) {
      startEngine();
      fingerMatched = true;
      break;
    }
  }

  if (!fingerMatched) {
    if (checkPassword()) {
      startEngine();
    } else {
      alert("Wrong password attempt!");
    }
  }

  int gasout = analogRead(GAS_SENSOR_PIN);
  if (gasout > 900) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

bool checkFingerprint() {
  if (finger.getImage() != FINGERPRINT_OK) return false;
  if (finger.image2Tz() != FINGERPRINT_OK) return false;
  if (finger.fingerFastSearch() == FINGERPRINT_OK) return true;

  alert("Unauthorized fingerprint!");
  return false;
}

bool checkPassword() {
  String input = "";
  while (input.length() < 4) {
    int gasout = analogRead(GAS_SENSOR_PIN);
    if (gasout > 700) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(500);
      digitalWrite(BUZZER_PIN, LOW);
    }

    char key = keypad.getKey();
    if (key) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      input += key;
    }
  }

  if (input == correctPassword) return true;

  alert("Wrong password attempt!");
  return false;
}

void startEngine() {
  digitalWrite(RELAY_PIN, LOW);
  delay(5000);
  digitalWrite(RELAY_PIN, HIGH);
}

void alert(String message) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(3000);
  digitalWrite(BUZZER_PIN, LOW);

  String msg = message + " C\nLocation: http://maps.google.com/maps?q=" + String(latt, 6) + "," + String(longg, 6);
  sendSMS("ALERT: " + msg);
}

void sendSMS(String text) {
  Serial.println("AT+CMGF=1");
  delay(1000);
  Serial.println("AT+CMGS=\"+91XXXXXXXXXX\"");  // Replace with actual number
  delay(1000);
  Serial.print(text);
  delay(500);
  Serial.write(26);
  delay(3000);
}
