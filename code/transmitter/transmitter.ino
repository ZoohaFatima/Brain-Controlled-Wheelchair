#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <DabbleESP32.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// ========== Motor Pins ==========
int enableRightMotor = 22;
int rightMotorPin1 = 16;
int rightMotorPin2 = 17;

int enableLeftMotor = 23;
int leftMotorPin1 = 18;
int leftMotorPin2 = 19;

#define MAX_MOTOR_SPEED 255

const int PWMFreq = 1000; // 1 KHz
const int PWMResolution = 8;
const int rightMotorPWMSpeedChannel = 4;
const int leftMotorPWMSpeedChannel = 5;

// ========== EEG & Blink ==========
#define EEG_PIN 34
bool isForward = false;
bool isStop = false;
bool isLeft = false;
bool isRight = false;

unsigned long lastBlinkTime = 0;
int blinkCount = 0;

// ========== Gyroscope ==========
Adafruit_MPU6050 mpu;
sensors_event_t a, g, temp;

void setUpPinModes() {
  pinMode(enableRightMotor, OUTPUT);
  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);

  pinMode(enableLeftMotor, OUTPUT);
  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);

  // Set up PWM for speed
  ledcSetup(rightMotorPWMSpeedChannel, PWMFreq, PWMResolution);
  ledcSetup(leftMotorPWMSpeedChannel, PWMFreq, PWMResolution);
  ledcAttachPin(enableRightMotor, rightMotorPWMSpeedChannel);
  ledcAttachPin(enableLeftMotor, leftMotorPWMSpeedChannel);

  rotateMotor(0, 0);
}

void rotateMotor(int rightMotorSpeed, int leftMotorSpeed) {
  // Right motor direction
  if (rightMotorSpeed < 0) {
    digitalWrite(rightMotorPin1, LOW);
    digitalWrite(rightMotorPin2, HIGH);
  } else if (rightMotorSpeed > 0) {
    digitalWrite(rightMotorPin1, HIGH);
    digitalWrite(rightMotorPin2, LOW);
  } else {
    digitalWrite(rightMotorPin1, LOW);
    digitalWrite(rightMotorPin2, LOW);
  }

  // Left motor direction
  if (leftMotorSpeed < 0) {
    digitalWrite(leftMotorPin1, LOW);
    digitalWrite(leftMotorPin2, HIGH);
  } else if (leftMotorSpeed > 0) {
    digitalWrite(leftMotorPin1, HIGH);
    digitalWrite(leftMotorPin2, LOW);
  } else {
    digitalWrite(leftMotorPin1, LOW);
    digitalWrite(leftMotorPin2, LOW);
  }

  ledcWrite(rightMotorPWMSpeedChannel, abs(rightMotorSpeed));
  ledcWrite(leftMotorPWMSpeedChannel, abs(leftMotorSpeed));
}

void updateEEGInputs() {
  int eegValue = analogRead(EEG_PIN);
  unsigned long currentTime = millis();

  // Blink detection (spike)
  if (eegValue > 3000) {
    if (currentTime - lastBlinkTime > 200) {
      blinkCount++;
      lastBlinkTime = currentTime;
    }
  }

  if (blinkCount >= 3 && (currentTime - lastBlinkTime) < 1500) {
    isStop = true;
    isForward = false;
    isLeft = false;
    isRight = false;
    blinkCount = 0;
  }

  if (currentTime - lastBlinkTime > 1500) {
    blinkCount = 0;
  }

  // Beta wave detection (mid EEG range)
  if (eegValue > 1600 && eegValue < 2200) {
    isForward = true;
    isStop = false;
  } else if (!isStop) {
    isForward = false;
  }
}

void updateGyroscope() {
  mpu.getEvent(&a, &g, &temp);

  float yRotation = g.gyro.y; // head tilt left/right

  // Tune these thresholds as per real movement
  if (yRotation > 0.8) {
    isLeft = true;
    isRight = false;
  } else if (yRotation < -0.8) {
    isRight = true;
    isLeft = false;
  } else {
    isLeft = false;
    isRight = false;
  }
}

void setup() {
  Serial.begin(115200);
  setUpPinModes();
  pinMode(EEG_PIN, INPUT);
  Dabble.begin("MyBluetoothCar");

  Wire.begin();
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void loop() {
  Dabble.processInput();         // Emergency manual control
  updateEEGInputs();             // Brain input
  updateGyroscope();            // Head movement input

  int rightMotorSpeed = 0;
  int leftMotorSpeed = 0;

  if (!isStop) {
    if (isForward) {
      rightMotorSpeed = MAX_MOTOR_SPEED;
      leftMotorSpeed = MAX_MOTOR_SPEED;
    }
    else if (isLeft) {
      rightMotorSpeed = MAX_MOTOR_SPEED;
      leftMotorSpeed = -MAX_MOTOR_SPEED;
    }
    else if (isRight) {
      rightMotorSpeed = -MAX_MOTOR_SPEED;
      leftMotorSpeed = MAX_MOTOR_SPEED;
    }
  }

  // Emergency override
  if (GamePad.isUpPressed()) {
    rightMotorSpeed = MAX_MOTOR_SPEED;
    leftMotorSpeed = MAX_MOTOR_SPEED;
  }
  if (GamePad.isDownPressed()) {
    rightMotorSpeed = -MAX_MOTOR_SPEED;
    leftMotorSpeed = -MAX_MOTOR_SPEED;
  }
  if (GamePad.isLeftPressed()) {
    rightMotorSpeed = MAX_MOTOR_SPEED;
    leftMotorSpeed = -MAX_MOTOR_SPEED;
  }
  if (GamePad.isRightPressed()) {
    rightMotorSpeed = -MAX_MOTOR_SPEED;
    leftMotorSpeed = MAX_MOTOR_SPEED;
  }

  rotateMotor(rightMotorSpeed, leftMotorSpeed);
}
