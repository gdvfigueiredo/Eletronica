// ====== INCLUSÃO DE BIBLIOTECAS ======
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>

// ====== OBJETOS ======
Adafruit_MPU6050 mpu;
File dataFile;
LiquidCrystal_I2C lcd(0x27, 20, 4); // Endereço 0x27, display 20x4

// ====== CONFIGURAÇÃO DO CARTÃO SD ======
const int chipSelect = 4; // no Arduino UNO geralmente é 4
const char* nomeArquivo = "DADOS.CSV";
bool sdWorking = false;

// ====== VARIÁVEIS DE TEMPO ======
unsigned long ultimoSalvo = 0;
const unsigned long intervalo = 1000; // gravação a cada 1s

// ====== VARIÁVEIS RPM ======
volatile int rpmCount = 0;
float rpm = 0;
const int pinRPM = 2; // no Arduino UNO usar pino digital 2 (INT0)

// ====== INTERRUPÇÃO DO SENSOR DE RPM ======
void contarRPM() {
  rpmCount++;
}

// ====== SETUP ======
void setup(void) {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("Inicializando sensores...");

  // Inicia MPU6050
  if (!mpu.begin()) {
    Serial.println("Falha ao encontrar o chip MPU6050");
    while (1) delay(10);
  }
  Serial.println("MPU6050 encontrado!");

  // Configuração do MPU
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Inicializa cartão SD
  if (!SD.begin(chipSelect)) {
    Serial.println("Falha ao inicializar o cartão SD!");
    while (1);
  }
  Serial.println("Cartão SD pronto.");

  // Cria arquivo se não existir
  if (!SD.exists(nomeArquivo)) {
    dataFile = SD.open(nomeArquivo, FILE_WRITE);
    if (dataFile) {
      dataFile.println("Tempo(ms);Ax;Ay;Az;Gx;Gy;Gz;RPM");
      dataFile.close();
    }
  }

  // Configura interrupção para RPM
  pinMode(pinRPM, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinRPM), contarRPM, RISING);

  // Inicializa LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema pronto!");
  delay(2000);
  lcd.clear();
}

// ====== LOOP ======
void loop() {
  unsigned long agora = millis();

  // Leitura do sensor MPU6050
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // A cada intervalo, calcula RPM e grava
  if (agora - ultimoSalvo >= intervalo) {

    ultimoSalvo = agora;

    // Calcula RPM (1 pulso por volta)
    rpm = (rpmCount * 60.0 * 1000.0) / intervalo;
    rpmCount = 0; // zera contador

    // Abre arquivo e salva dados
    dataFile = SD.open(nomeArquivo, FILE_WRITE);
    if (dataFile) {
      Serial.println("Escrevendo no arquivo CSV");
      dataFile.print(agora/1000);
      dataFile.print(";");
      dataFile.print(a.acceleration.x);
      dataFile.print(";");
      dataFile.print(a.acceleration.y);
      dataFile.print(";");
      dataFile.print(a.acceleration.z);
      dataFile.print(";");
      dataFile.print(g.gyro.x);
      dataFile.print(";");
      dataFile.print(g.gyro.y);
      dataFile.print(";");
      dataFile.print(g.gyro.z);
      dataFile.print(";");
      dataFile.println(rpm);
      dataFile.close();
      Serial.println("Escrita concluída");
      sdWorking = true;
    } else {
      Serial.println("Erro ao gravar no SD");
      sdWorking = false;
    }

    // Debug serial
    Serial.print("Ax="); Serial.print(a.acceleration.x);
    Serial.print(" Ay="); Serial.print(a.acceleration.y);
    Serial.print(" Az="); Serial.print(a.acceleration.z);
    Serial.print(" | RPM="); Serial.println(rpm);

    // ====== DISPLAY LCD ======
    lcd.clear();

    // Linha 0: RPM
    lcd.setCursor(0, 0);
    lcd.print("RPM: ");
    lcd.print(rpm, 1);

    // Linha 1: Aceleração X
    lcd.setCursor(0, 1);
    lcd.print("Ax: ");
    lcd.print(a.acceleration.x, 1);

    // Linha 2: Aceleração Y
    lcd.setCursor(0, 2);
    lcd.print("Ay: ");
    lcd.print(a.acceleration.y, 1);

    // Linha 3: Aceleração Z
    lcd.setCursor(0, 3);
    lcd.print("Az: ");
    lcd.print(a.acceleration.z, 1);

    if (sdWorking == true){
    lcd.setCursor(12, 3);
    lcd.print(F("SD OK"));
    } else {
    lcd.setCursor(12, 3);
    lcd.print(F("SD ERROR"));
    }
  }
}
