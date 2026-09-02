#include <Arduino.h>
#include <Wire.h>

const int MPU_ADDR = 0x68;
int16_t accX, accY, accZ;
float pitch, roll;
float baselinePitch, baselineRoll;
const float THRESHOLD_ANGLE = 15.0; // graus de desvio que disparam o alarme
const int BUZZER = 13;

void readAccel() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // início dos registradores de aceleração
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  accX = Wire.read() << 8 | Wire.read();
  accY = Wire.read() << 8 | Wire.read();
  accZ = Wire.read() << 8 | Wire.read();

  pitch = atan2(accY, sqrt((float)accX * accX + (float)accZ * accZ)) * 180.0 / PI;
  roll  = atan2(-accX, accZ) * 180.0 / PI;
}

void setup() {
  Wire.begin();
  Serial.begin(9600);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1
  Wire.write(0);    // tira do modo sleep
  Wire.endTransmission(true);

  pinMode(BUZZER, OUTPUT);
  delay(100);

  readAccel();
  baselinePitch = pitch;
  baselineRoll = roll;
  Serial.println("Baseline calibrado.");
}

void loop() {
  readAccel();

  float deltaPitch = abs(pitch - baselinePitch);
  float deltaRoll  = abs(roll - baselineRoll);

  Serial.print("Pitch: "); Serial.print(pitch);
  Serial.print(" | Roll: "); Serial.println(roll);

  if (deltaPitch > THRESHOLD_ANGLE || deltaRoll > THRESHOLD_ANGLE) {
    digitalWrite(BUZZER, HIGH);
    Serial.println("ALERTA: possível deslizamento detectado!");
  } else {
    digitalWrite(BUZZER, LOW);
  }

  delay(200);
}
