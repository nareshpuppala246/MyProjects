#include <Wire.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define BUZZER 5
#define SWITCH 6
#define ONE_WIRE_BUS 7

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
const int MPU6050_addr = 0x68;

int16_t axis_X, axis_Y, axis_Z;
int minVal = 265;
int maxVal = 402;
float x, y, z;
float latt = 0.0, longg = 0.0;

TinyGPSPlus gps;
SoftwareSerial gsmSerial(8, 9); // RX, TX for GSM
SoftwareSerial gpsSerial(10, 11); // RX, TX for GPS (optional)

String numbers[3] = {"9182284604", "9949187701", "8309853481"};
unsigned long lastSMSTime = 0;
int Flag = 1;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  gsmSerial.begin(9600);
  gpsSerial.begin(9600); // If GPS is connected via SoftwareSerial

  // MPU6050 Initialization
  Wire.beginTransmission(MPU6050_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  sensors.begin();

  pinMode(BUZZER, OUTPUT);
  pinMode(SWITCH, INPUT_PULLUP);

  digitalWrite(BUZZER, HIGH);
  delay(1000);
  digitalWrite(BUZZER, LOW);

  // sendEmergencyMessage("Device Started");
}

void loop() {
  // Read MPU6050
  Wire.beginTransmission(MPU6050_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_addr, 14, true);
  axis_X = Wire.read() << 8 | Wire.read();
  axis_Y = Wire.read() << 8 | Wire.read();
  axis_Z = Wire.read() << 8 | Wire.read();

  int xAng = map(axis_X, minVal, maxVal, -90, 90);
  int yAng = map(axis_Y, minVal, maxVal, -90, 90);
  int zAng = map(axis_Z, minVal, maxVal, -90, 90);

  x = RAD_TO_DEG * (atan2(-yAng, -zAng) + PI);
  y = RAD_TO_DEG * (atan2(-xAng, -zAng) + PI);
  z = RAD_TO_DEG * (atan2(-yAng, -xAng) + PI);

  Serial.print("X: "); Serial.println(x);
  Serial.print("Y: "); Serial.println(y);
  Serial.print("Z: "); Serial.println(z);
// delay(1000);
  // // Read Temperature
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  Serial.println(tempC);

  // Read GPS data
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isValid()) {
    latt = gps.location.lat();
    longg = gps.location.lng();
  }
  else{
   latt =  17.307675445789098;
   longg =  78.73660346190773;
  }

  // Fall detection logic
  if ((x < 50) || (y > 0) && (y < 350)) {
    Flag = 0;
  } else {
    Flag = 1;
  }

  // Send temperature SMS every 5 seconds
  if (millis() - lastSMSTime > 160000) {
    lastSMSTime = millis();
    String msg = "Temperature: " + String(tempC) + " C\nLocation: http://maps.google.com/maps?q=";
    msg += String(latt, 6) + "," + String(longg, 6);
    sendEmergencyMessage(msg);
  }
Serial.println(digitalRead(SWITCH));
  // Emergency: switch press or fall
  if ((digitalRead(SWITCH) == LOW) || (Flag == 0)) {
    digitalWrite(BUZZER, HIGH);
    delay(500);
    digitalWrite(BUZZER, LOW);

    String emergencyMsg = "Emergency! Help needed. Location: http://maps.google.com/maps?q=";
    emergencyMsg += String(latt, 6) + "," + String(longg, 6);
    sendEmergencyMessage(emergencyMsg);
    delay(2000); // prevent repeated triggers
  }
}

void sendEmergencyMessage(String message) {
  for (int i = 0; i < 3; i++) {
    SendMessage(numbers[i], message);
  }
}

void SendMessage(String num, String msg) {
  gsmSerial.println("AT+CMGF=1");
  delay(1000);
  gsmSerial.println("AT+CMGS=\"" + num + "\"");
  delay(1000);
  gsmSerial.print(msg);
  gsmSerial.write(26); // Ctrl+Z to send
  delay(3000);
  Serial.println("MESSAGE SENT TO " + num);
}
