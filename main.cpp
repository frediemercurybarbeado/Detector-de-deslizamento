#include <Wire.h>

// =========================
// PINOS E CONFIGURAÇÕES
// =========================

const int MPU_ADDR = 0x68;
const int BUZZER = 13;
const int BUTTON = 2;

const float THRESHOLD_ANGLE = 3.5;

// Quantidade de leituras usadas na média
const int NUM_LEITURAS = 30;

// Filtro exponencial
// Quanto menor, mais estável.
// Quanto maior, mais rápido responde.
const float ALPHA = 0.08;


// =========================
// VARIÁVEIS
// =========================

int16_t accX, accY, accZ;

float pitch = 0;
float roll = 0;

float filteredPitch = 0;
float filteredRoll = 0;

float baselinePitch = 0;
float baselineRoll = 0;

unsigned long inicioAlerta = 0;
bool contandoAlerta = false;

// =========================
// LEITURA DO MPU6050
// =========================

void readAccel() {

  float somaPitch = 0;
  float somaRoll = 0;

  for (int i = 0; i < NUM_LEITURAS; i++) {

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);

    if (Wire.endTransmission(false) != 0) {
      continue;
    }

    Wire.requestFrom(MPU_ADDR, 6, true);

    if (Wire.available() < 6) {
      continue;
    }

    accX = (Wire.read() << 8) | Wire.read();
    accY = (Wire.read() << 8) | Wire.read();
    accZ = (Wire.read() << 8) | Wire.read();


    // =========================
    // CÁLCULO DOS ÂNGULOS
    // =========================

    float p = atan2(
      accY,
      sqrt((float)accX * accX +
           (float)accZ * accZ)
    ) * 180.0 / PI;

    float r = atan2(
      -accX,
      accZ
    ) * 180.0 / PI;


    somaPitch += p;
    somaRoll += r;

    delay(2);
  }


  // Média
  float novoPitch = somaPitch / NUM_LEITURAS;
  float novoRoll = somaRoll / NUM_LEITURAS;


  // =========================
  // FILTRO EXPONENCIAL
  // =========================

  static bool primeiraLeitura = true;

  if (primeiraLeitura) {

    filteredPitch = novoPitch;
    filteredRoll = novoRoll;

    primeiraLeitura = false;

  } else {

    filteredPitch =
      ALPHA * novoPitch +
      (1.0 - ALPHA) * filteredPitch;

    filteredRoll =
      ALPHA * novoRoll +
      (1.0 - ALPHA) * filteredRoll;
  }


  pitch = filteredPitch;
  roll = filteredRoll;
}


// =========================
// CALIBRAR BASELINE
// =========================

void calibrarBaseline() {

  Serial.println();
  Serial.println("Calibrando...");
  Serial.println("NAO MOVA O SENSOR!");

  delay(1000);

  float somaPitch = 0;
  float somaRoll = 0;

  const int CALIBRACAO = 15;

  for (int i = 0; i < CALIBRACAO; i++) {

    readAccel();

    somaPitch += pitch;
    somaRoll += roll;

    delay(20);
  }

  baselinePitch = somaPitch / CALIBRACAO;
  baselineRoll = somaRoll / CALIBRACAO;

  Serial.println("Baseline calibrado!");

  Serial.print("Baseline Pitch: ");
  Serial.println(baselinePitch);

  Serial.print("Baseline Roll: ");
  Serial.println(baselineRoll);

  Serial.println();
}


// =========================
// SETUP
// =========================

void setup() {
  Wire.begin();

  Serial.begin(9600);


  // =========================
  // ACORDA O MPU6050
  // =========================

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);


  // =========================
  // FILTRO INTERNO DO MPU6050
  // =========================

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1A);

  // DLPF ~5 Hz
  Wire.write(0x06);

  Wire.endTransmission(true);


  // =========================
  // PINOS
  // =========================

  pinMode(BUZZER, OUTPUT);

  pinMode(BUTTON, INPUT_PULLUP);

  digitalWrite(BUZZER, LOW);


  delay(300);


  // =========================
  // CALIBRAÇÃO INICIAL
  // =========================
//uma linha
  calibrarBaseline();
}


// =========================
// LOOP
// =========================

void loop() {
  // =========================
  // BOTÃO DE RECALIBRAÇÃO
  // =========================

  if (digitalRead(BUTTON) == LOW) {

    digitalWrite(BUZZER, LOW);

    calibrarBaseline();

    // Espera o botão ser solto
    while (digitalRead(BUTTON) == LOW) {
      delay(10);
    }

    delay(100);
  }


  // =========================
  // LEITURA
  // =========================

  readAccel();


  // =========================
  // DIFERENÇA DO BASELINE
  // =========================

  float deltaPitch =
    fabs(pitch - baselinePitch);

  float deltaRoll =
    fabs(roll - baselineRoll);


  // =========================
  // MONITOR SERIAL
  // =========================

  Serial.print("Pitch: ");
  Serial.print(pitch, 2);

  Serial.print(" | Roll: ");
  Serial.print(roll, 2);

  Serial.print(" | dPitch: ");
  Serial.print(deltaPitch, 2);

  Serial.print(" | dRoll: ");
  Serial.println(deltaRoll, 2);


  // =========================
  // DETECÇÃO
  // =========================

  bool inclinacaoPerigosa =
      (deltaPitch > THRESHOLD_ANGLE ||
       deltaRoll > THRESHOLD_ANGLE);

  if (inclinacaoPerigosa) {

    // Começou a inclinação
    if (!contandoAlerta) {
      inicioAlerta = millis();
      contandoAlerta = true;

      Serial.println("Threshold ultrapassado. Contando 5 segundos...");
    }

    // Já passaram 5 segundos?
    if (millis() - inicioAlerta >= 5000) {

      digitalWrite(BUZZER, HIGH);

      Serial.println("!!! ALERTA: POSSIVEL DESLIZAMENTO !!!");
    }

  } else {

    // Voltou para uma posição segura
    contandoAlerta = false;
    digitalWrite(BUZZER, LOW);
  }

}
