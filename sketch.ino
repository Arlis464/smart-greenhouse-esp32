#include <WiFi.h>
#include <ESP32Servo.h>
#include "DHT.h"
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>
#include <ArduinoJson.h>


#define SERVO_PIN 26 // ESP32 pin GPIO26 connected to servo motor(PWM)
#define DHTPIN 4
#define DHTTYPE DHT22 // Changer a DHT11 (wokwi n'a que DHT22)
DHT dht(DHTPIN, DHTTYPE);
Servo servoMotor;

const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define TB_SERVER "eu.thingsboard.cloud"
#define TOKEN "REPLACE_WITH_THINGSBOARD_TOKEN" //Il faut changer le token a partir du device de ThingsBoard

constexpr uint16_t MAX_MESSAGE_SIZE = 256U;

WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

void connectToThingsBoard() {
  if (!tb.connected()) {
    Serial.println("Connecting to ThingsBoard server");

    if (!tb.connect(TB_SERVER, TOKEN)) {
      Serial.println("Failed to connect to ThingsBoard");
    } else {
      Serial.println("Connected to ThingsBoard");
    }
  }
}

void sendDataToThingsBoard(float temp, float hum, int servoPos) {
  bool door;
  if (servoPos == 180) {
    door = true;
  } else {
    door = false;
  }
  tb.sendTelemetryData("temperature", temp);
  tb.sendTelemetryData("humidity", hum);
  tb.sendTelemetryData("servoPosition", door);
}


void connectToWifi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // Connect to Wi-Fi
  connectToWifi();
  Serial.println("\nWi-Fi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  connectToThingsBoard();
  servoMotor.attach(SERVO_PIN);  // attaches the servo on ESP32 pin
  dht.begin();
}

void loop() {
  static unsigned long previousMillis = 0;
  const long interval = 2000; // 2 seconds
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float hum = dht.readHumidity();
    float temp = dht.readTemperature();

    if (isnan(hum) || isnan(temp)) {
      Serial.println(F("Failed to read from DHT sensor!"));
    }

    float hic = dht.computeHeatIndex(temp, hum, false);

    Serial.print(F("Humidity: "));
    Serial.print(hum);
    Serial.print(F("%  Temperature: "));
    Serial.print(temp);
    Serial.print(F("°C Heat index: "));
    Serial.println(hic);

    static int lastServoPosition = 0; // Stocke la dernière position du servo

    if (temp > 25 && lastServoPosition != 180) {
      servoMotor.write(180);
      lastServoPosition = 180;
      Serial.println("Servo position: 180°");
    } else if (temp > 22 && temp <= 25 && lastServoPosition != 90) {
      servoMotor.write(90);
      lastServoPosition = 90;
      Serial.println("Servo position: 90°");
    } else if (temp <= 22 && lastServoPosition != 0) {
      servoMotor.write(0);
      lastServoPosition = 0;
      Serial.println("Servo position: 0°");
    }
    sendDataToThingsBoard(temp, hum, lastServoPosition);
  }


  delay(10); // this speeds up the simulation

}
