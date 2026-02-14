//Código para o Nacional 2025
/*Este código faz a leitura de 3 sensores:
 * -RPM do motor;
 * -Velocidade do carro;
 * -Temperatura da CVT.
 * 
 * Esses dados são exibidos no display e transmitidos por LoRa 
 */

// INCLUSÃO DE BIBLIOTECAS
#include "max6675.h"
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <LiquidCrystal_I2C.h>
#include "LoRa_E220.h"

//--------------------------------------------------------------------------------------------------------------

//LoRa
#define FREQUENCY_915
LoRa_E220 e220ttl(&Serial2); //  RX AUX M0 M1; // WeMos RX --> e220 TX - WeMos TX --> e220 RX


// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
// set the LCD number of columns and rows
int lcdColumns = 20;
int lcdRows = 4;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

//Variáveis para RPM
byte t; //Variável auxiliar de teste.
volatile int rpmcount = 0; //Variável de contagem de pulsos.
float rpm = 0; //Estado inicial do sensor.
float d=0.5334; //Fatores de conversão para o uso do efeito Hall para a medição de velocidade, 'd' é o diâmetro da roda e 'vel' o estado inicial.
unsigned long lastmillis = 0; //Variável de tempo, armazena o tempo entre as interrupções do sistema
byte v;
//Laço de configuração, será executado apenas uma vez.
int PinRPM=35; 

//Variáveis para velocímetro
volatile int velcount = 0; //Variável de contagem de pulsos.
float Vel0 = 0; //Estado inicial do sensor.
float vel=0; //Fatores de conversão para o uso do efeito Hall para a medição de velocidade, 'd' é o diâmetro da roda e 'vel' o estado inicial.
//unsigned long lastmillis = 0; //Variável de tempo, armazena o tempo entre as interrupções do sistema
int PinVel=34;

//Inicialização para o sensor termopar
int thermoSO = 4;
int thermoCS = 2;
int thermoCLK = 15;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoSO);

// Variáveis para cronômetro
unsigned long startMillis = 0;
unsigned long elapsedMillis = 0;
bool running = false;

void setup(){
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  
  // initialize LCD
  lcd.init();
  // turn on LCD backlight                     
  lcd.backlight();

 
  //Incialização do módulo LoRa
  e220ttl.begin();

  //Inicialização RPM
  pinMode(PinRPM,INPUT_PULLUP); //Pino de entrada do sinal do sensor, o pino 3 que ele se refere é o pino 3 digital do arduino, ou pino 1 do ATMEGA328P
  attachInterrupt(PinRPM, rpm_fan, FALLING);
  /* Os pinos de interrupção do arduino são os pinos digitais 2 e 3 (no ATMEGA328P, são os pinos 32 e 1, respectivamente).
   * Essa função tem a seguinte estrutura = attachInterrupt(pino da interrupção, variável de armazenamento, tipo de leitura);
   * Pino da interrupção: pode ser zero (0) ou um (1), sendo zero utilizando o pino 2 do arduino, enquanto o um utiliza o pino 3 do arduino. 
   * Caso não utilize o Arduino Uno, consulte o datasheet para mais informações sobre os pinos de interrupção.
   * Função de interrupção: Quando é ativa a interrupção, será executada essa função.
   * Tipo de leitura: pode ser FALLING ou RISING, a única diferença entre eles é o ponto no qual o programa será interrompido, se será quando o sinal está caindo (FALLING) ou subindo (RISING).
   * Para essa aplicação, utiliza-se FALLING pois o sensor, quando não acionado, está em nível alto, e o microcontrolador será mais sensível com a mudança de estado para nivel baixo do que o contrário. 
   */

  //Incialização velocímetro
  pinMode(PinVel,INPUT_PULLUP); 
  attachInterrupt(PinVel, vel_fan, FALLING);

  //MAX6675 - termoparz
  Serial.println("MAX6675 test");
  // wait for MAX chip to stabilize
  delay(500);

  startMillis = millis();
}

//Laço de repetição, será executado enquanto ligado.
void loop(){
  //Cronometro 
  unsigned long currentTime = millis();
  unsigned long seconds = ((currentTime - startMillis) / 1000) % 60;
  unsigned long minutes = ((currentTime - startMillis) / 60000) % 60;
  unsigned long hours = (currentTime - startMillis) / 3600000;
  
  lcd.setCursor(0, 0);
  lcd.print("Cronometro: ");
  if (hours < 10 ){
    lcd.print("0");
  }
  lcd.print(hours);
  lcd.print(":");
  if (minutes < 10 ){
    lcd.print("0");
  }
  lcd.print(minutes);
  lcd.print(":");
  if (seconds < 10 ){
    lcd.print("0");
  }
  lcd.print(seconds);

  // basic readout test, just print the current temp
  float temperatura = thermocouple.readCelsius();
  Serial.print("C = ");
  Serial.println(thermocouple.readCelsius());
  lcd.setCursor(0, 1);
  lcd.print("Temperatura: ");
  lcd.print(temperatura);
  lcd.print("C");

  //Parte do sensor RPM
  if (millis() - lastmillis >= 1000){ //Atualiza a cada um segundo, essa leitura será equivalente a ler a frequência em Hz
    detachInterrupt(PinRPM); 
    detachInterrupt(PinVel);//Desabilita a interrupção quando está calculando.
    rpm = (float)rpmcount * 60; //Convterte a frequência para RPM. Para um ímã: 60, para dois ímãs: 30, para três ímãs: 20.
    vel=(float)velcount*2*3.14*d*3.6; //Conversão de frequência para velocidade a partir da equação V = f*2*pi*d*3.6
    rpmcount = 0; // Reinicia o contador
    velcount=0;
    lastmillis = millis(); // Atualiza o tempo.
    //A seguir o processo de envio via Serial, ao utilizar CAN essas linhas são substituidas pelo envio CAN.
    Serial.print("Rpm ");
    Serial.println(rpm);
    lcd.setCursor(0, 2);
    lcd.print("RPM: ");
    lcd.print(rpm, 2);
    Serial.print("Velocidade ");
    Serial.println(vel);
    lcd.setCursor(0, 3);
    lcd.print("Velocidade: ");
    lcd.print(vel, 2);
    attachInterrupt(PinRPM, rpm_fan, FALLING);
    attachInterrupt(PinVel, vel_fan, FALLING);//Liga novamente a interrupção. 
  }

  ResponseStatus rs  = e220ttl.sendMessage("pass,temp " + String(temperatura, 2) + ", rpm " + String(rpm, 2) + ", vel " + String(vel, 2));
  Serial.println("sending: pass,temp ," + String(temperatura, 2) + ", rpm " + String(rpm, 2) + ", vel " + String(vel, 2));
  Serial.println(rs.getResponseDescription());

 delay(250);
}

void rpm_fan(){ //Esta função é responsável pela contagem de interrupções  com rpmcount++, 
  rpmcount++;
}

void vel_fan(){ //Esta função é responsável pela contagem de interrupções  com vel++ 
  velcount++;
}
