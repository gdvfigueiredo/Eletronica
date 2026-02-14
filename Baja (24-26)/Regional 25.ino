#include "max6675.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Variáveis RPM
volatile int rpmcount = 0;
float rpm = 0;
unsigned long lastmillis = 0;
int PinRPM = 25;

// Variáveis velocidade
volatile int velcount = 0;
float vel = 0;
float d = 0.5334;
int PinVel = 26;

// Termopar MAX6675
int thermoSO = 4;
int thermoCS = 2;
int thermoCLK = 15;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoSO);

// Cronômetro
unsigned long startMillis = 0;

// SD Card
const int chipSelect = 5;
File dataFile;

// Funções de interrupção
void IRAM_ATTR rpm_fan() {
  rpmcount++;
}

void IRAM_ATTR vel_fan() {
  velcount++;
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

  delay(500);  // Estabilização MAX6675
  startMillis = millis();

  // Inicializa SD
  Serial.println("Iniciando módulo SD...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Erro ao iniciar o cartão SD");
    lcd.setCursor(0, 0);
    lcd.print("SD NAO INICIADO");
  } else {
    Serial.println("Cartão SD pronto!");
    lcd.setCursor(0, 0);
    lcd.print("SD INICIADO OK");
    delay(1000);
    lcd.clear();

    // Cria arquivo e cabeçalho
    dataFile = SD.open("/dados.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.println("Tempo,Temperatura,RPM,Velocidade");
      dataFile.close();
    }
  }
}

void loop() {
  // Cronômetro
  unsigned long currentTime = millis();
  unsigned long seconds = ((currentTime - startMillis) / 1000) % 60;
  unsigned long minutes = ((currentTime - startMillis) / 60000) % 60;
  unsigned long hours   = ((currentTime - startMillis) / 3600000);

  lcd.setCursor(0, 0);
  lcd.printf("Cronometro: %02lu:%02lu:%02lu", hours, minutes, seconds);

  // Temperatura
  float temperatura = thermocouple.readCelsius();
  lcd.setCursor(0, 1);
  lcd.print("Temperatura: ");
  lcd.print(temperatura, 1);
  lcd.print("C   ");

  // RPM e velocidade a cada segundo
  if (millis() - lastmillis >= 1000) {
    detachInterrupt(digitalPinToInterrupt(PinRPM));
    detachInterrupt(digitalPinToInterrupt(PinVel));

    rpm = rpmcount * 60.0;
    vel = velcount * 2 * 3.1416 * d * 3.6;

    rpmcount = 0;
    velcount = 0;
    lastmillis = millis();

    lcd.setCursor(0, 2);
    lcd.print("RPM: ");
    lcd.print(rpm, 1);
    lcd.print("     ");

    lcd.setCursor(0, 3);
    lcd.print("Velocidade: ");
    lcd.print(vel, 1);
    lcd.print("km/h   ");

    // Grava dados no SD
    dataFile = SD.open("/dados.csv", FILE_WRITE);
    if (dataFile) {
      char buffer[64];
      sprintf(buffer, "%02lu:%02lu:%02lu,%.1f,%.1f,%.1f", hours, minutes, seconds, temperatura, rpm, vel);
      dataFile.println(buffer);
      dataFile.close();
    } else {
      Serial.println("Erro ao escrever no SD");
    }

    attachInterrupt(digitalPinToInterrupt(PinRPM), rpm_fan, FALLING);
    attachInterrupt(digitalPinToInterrupt(PinVel), vel_fan, FALLING);
  }

  delay(250);
}
