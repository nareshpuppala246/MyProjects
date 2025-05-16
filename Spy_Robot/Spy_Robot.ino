#include <WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

// WiFi credentials
#define WIFI_SSID "*****"
#define WIFI_PASS "******"

// Adafruit IO
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME "***********"
#define AIO_KEY "*************************"

// Robot pins
#define IN1 14
#define IN2 27
#define IN3 26
#define IN4 25

// Sensors and buzzer
#define FLAME_PIN 35
#define GAS_PIN 32
#define METAL_PIN 33
#define BUZZER_PIN 18

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Movement buttons (feeds)
Adafruit_MQTT_Subscribe forwardBtn  = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/forward");
Adafruit_MQTT_Subscribe backwardBtn = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/backward");
Adafruit_MQTT_Subscribe leftBtn     = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/left");
Adafruit_MQTT_Subscribe rightBtn    = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/right");

// Sensor feeds
Adafruit_MQTT_Publish gasFeed   = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/gas");
Adafruit_MQTT_Publish flameFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/flame");

void setup() {
  Serial.begin(9600);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(FLAME_PIN, INPUT);
  pinMode(GAS_PIN, INPUT);
  pinMode(METAL_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

      // digit

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  digitalWrite(BUZZER_PIN, HIGH);
delay(1000);
  digitalWrite(BUZZER_PIN, LOW);

  mqtt.subscribe(&forwardBtn);
  mqtt.subscribe(&backwardBtn);
  mqtt.subscribe(&leftBtn);
  mqtt.subscribe(&rightBtn);
}

void loop() {
  MQTT_connect();

  bool moveF = false, moveB = false, moveL = false, moveR = false;

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(10))) {
  String val;

  if (subscription == &forwardBtn) {
    val = String((char*)forwardBtn.lastread);
    if (val == "ON") {
      moveForward();
    } else {
      stopRobot();
    }
  }

  if (subscription == &backwardBtn) {
    val = String((char*)backwardBtn.lastread);
    if (val == "ON") {
      moveBackward();
    } else {
      stopRobot();
    }
  }

  if (subscription == &leftBtn) {
    val = String((char*)leftBtn.lastread);
    if (val == "ON") {
      turnLeft();
    } else {
      stopRobot();
    }
  }

  if (subscription == &rightBtn) {
    val = String((char*)rightBtn.lastread);
    if (val == "ON") {
     turnRight() ;
    } else {
      moveR = false;
      stopRobot();
    }
  }
}

  // Read sensors
  int gasValue = analogRead(GAS_PIN);
  int flameVal = digitalRead(FLAME_PIN);
  int metalVal = digitalRead(METAL_PIN);
Serial.print("Gas_Value: ");Serial.println(gasValue);
Serial.print("flame_Value: ");Serial.println(flameVal);

Serial.print("metal_Value: ");Serial.println(metalVal);

  // Send data
  gasFeed.publish(gasValue);
  delay(2500);
  flameFeed.publish(flameVal == LOW ? 0 : 1);
  delay(2500);

  // Trigger buzzer
  if (flameVal == LOW || metalVal == HIGH) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // delay(2000); // Publish every 2 seconds
}

// Robot movement
void moveForward()  { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }
void moveBackward() { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
void turnLeft()     { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }
void turnRight()    { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
void stopRobot()    { digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);  digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW); }

void MQTT_connect() {
  int8_t ret;
  if (mqtt.connected()) return;

  while ((ret = mqtt.connect()) != 0) {
    Serial.println("MQTT connect failed, retrying...");
    delay(2000);
  }
  Serial.println("MQTT connected");
}
