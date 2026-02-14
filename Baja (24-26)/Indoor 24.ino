#include "max6675.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Variáveis RPM e Velocidade
volatile int rpmcount = 0;
volatile int velcount = 0;
float rpm = 0;
float vel = 0;
float d = 0.5334;  // diâmetro da roda em metros

int PinRPM = 25;
int PinVel = 26;

// Termopar MAX6675
int thermoSO = 4;
int thermoCS = 2;
int thermoCLK = 15;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoSO);

// Cronômetro
unsigned long startMillis = 0;
unsigned long lastCalcMillis = 0;
unsigned long lastDisplayMillis = 0;

// SD Card
const int chipSelect = 5;
File dataFile;
char filename[20];

// Funções de interrupção
void IRAM_ATTR rpm_fan() { rpmcount++; }
void IRAM_ATTR vel_fan() { velcount++; }

// Gera nome de arquivo único
void gerarNomeArquivo() {
  for (int i = 1; i < 100; i++) {
    sprintf(filename, "/dados_%02d.csv", i);
    if (!SD.exists(filename)) {
      dataFile = SD.open(filename, FILE_WRITE);
      if (dataFile) {
        dataFile.println("Tempo,Temperatura,RPM,Velocidade");
        dataFile.close();
      }
      break;
    }
  }
}

void setup() {
  Serial.begin(115200);

  // LCD
  lcd.init();
  lcd.backlight();

  // Sensores
  pinMode(PinRPM, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PinRPM), rpm_fan, FALLING);
  pinMode(PinVel, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PinVel), vel_fan, FALLING);

  delay(500);  // Estabilização do MAX6675
  startMillis = millis();

  // SD Card
  lcd.setCursor(0, 0);
  lcd.print("Iniciando SD...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Erro ao iniciar SD");
    lcd.setCursor(0, 1);
    lcd.print("SD NAO INICIADO");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("SD OK, criando arq");
    gerarNomeArquivo();
    delay(1000);
  }

  lcd.clear();
}

void loop() {
  unsigned long currentMillis = millis();

  // Cronômetro
  unsigned long elapsed = currentMillis - startMillis;
  unsigned long seconds = (elapsed / 1000) % 60;
  unsigned long minutes = (elapsed / 60000) % 60;
  unsigned long hours   = (elapsed / 3600000);

  // Atualiza display a cada 250ms
  if (currentMillis - lastDisplayMillis >= 250) {
    lastDisplayMillis = currentMillis;

    lcd.setCursor(0, 0);
    lcd.print("Cronometro: ");
    lcd.print(hours < 10 ? "0" : ""); lcd.print(hours); lcd.print(":");
    lcd.print(minutes < 10 ? "0" : ""); lcd.print(minutes); lcd.print(":");
    lcd.print(seconds < 10 ? "0" : ""); lcd.print(seconds);

    float temperatura = thermocouple.readCelsius();
    if (isnan(temperatura)) temperatura = -1.0;

    lcd.setCursor(0, 1);
    lcd.print("Temperatura: ");
    lcd.print(temperatura, 1);
    lcd.print("C   ");
  }

  // Cálculo RPM, Velocidade e gravação SD a cada 1 segundo
  if (currentMillis - lastCalcMillis >= 1000) {
    lastCalcMillis = currentMillis;

    detachInterrupt(digitalPinToInterrupt(PinRPM));
    detachInterrupt(digitalPinToInterrupt(PinVel));

    rpm = rpmcount * 60.0;
    vel = velcount * 2 * 3.1416 * d * 3.6;

    rpmcount = 0;
    velcount = 0;

    lcd.setCursor(0, 2);
    lcd.print("RPM: ");
    lcd.print(rpm, 1);
    lcd.print("       ");

    lcd.setCursor(0, 3);
    lcd.print("Velocidade: ");
    lcd.print(vel, 1);
    lcd.print("km/h   ");

    // Gravação no SD
    float temperatura = thermocouple.readCelsius();
    if (isnan(temperatura)) temperatura = -1.0;

    dataFile = SD.open(filename, FILE_WRITE);
    if (dataFile) {
      char buffer[64];
      sprintf(buffer, "%02lu:%02lu:%02lu,%.1f,%.1f,%.1f", hours, minutes, seconds, temperatura, rpm, vel);
      dataFile.println(buffer);
      dataFile.close();
    } else {
      Serial.println("Erro ao gravar no SD");
    }

    attachInterrupt(digitalPinToInterrupt(PinRPM), rpm_fan, FALLING);
    attachInterrupt(digitalPinToInterrupt(PinVel), vel_fan, FALLING);
  }
}
