#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"

// Motor pins
int enableRightMotor = 22;
int rightMotorPin1 = 16;
int rightMotorPin2 = 17;

int enableLeftMotor = 23;
int leftMotorPin1 = 18;
int leftMotorPin2 = 19;

const int MAX_SPEED = 255;
const int PWMFreq = 1000;
const int PWMResolution = 8;
const int rightMotorChannel = 4;
const int leftMotorChannel = 5;

typedef struct struct_message {
  int8_t move;
} struct_message;

void rotateMotor(int rightSpeed, int leftSpeed) {
  digitalWrite(rightMotorPin1, rightSpeed > 0 ? HIGH : LOW);
  digitalWrite(rightMotorPin2, rightSpeed < 0 ? HIGH : LOW);

  digitalWrite(leftMotorPin1, leftSpeed > 0 ? HIGH : LOW);
  digitalWrite(leftMotorPin2, leftSpeed < 0 ? HIGH : LOW);

  ledcWrite(rightMotorChannel, abs(rightSpeed));
  ledcWrite(leftMotorChannel, abs(leftSpeed));
}

void onDataReceive(const uint8_t * mac, const uint8_t *incomingData, int len) {
  struct_message receivedData;
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  Serial.print("Received command: ");
  switch (receivedData.move) {
    case 0:
      Serial.println("STOP");
      rotateMotor(0, 0);
      break;
    case 1:
      Serial.println("FORWARD");
      rotateMotor(MAX_SPEED, MAX_SPEED);
      break;
    case 2:
      Serial.println("LEFT");
      rotateMotor(-MAX_SPEED, MAX_SPEED);
      break;
    case 3:
      Serial.println("RIGHT");
      rotateMotor(MAX_SPEED, -MAX_SPEED);
      break;
    default:
      Serial.println("Unknown");
      rotateMotor(0, 0);
      break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(enableRightMotor, OUTPUT);
  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);

  pinMode(enableLeftMotor, OUTPUT);
  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);

  ledcSetup(rightMotorChannel, PWMFreq, PWMResolution);
  ledcSetup(leftMotorChannel, PWMFreq, PWMResolution);
  ledcAttachPin(enableRightMotor, rightMotorChannel);
  ledcAttachPin(enableLeftMotor, leftMotorChannel);

  rotateMotor(0, 0);  // Stop motors initially

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE); // Same channel as sender

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(onDataReceive);

  Serial.println("ESP-NOW Receiver ready");
}

void loop() {
  // blank
}
