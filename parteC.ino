// Lab 3 — Parte C: Parte B + watchdog
// Só as diferenças em relação à Parte B estão comentadas com  // WDT
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/wdt.h>        // WDT
#include <EEPROM.h>         // WDT: contador de boots que sobrevive ao reset

const int PINO_TRIG = 9;
const int PINO_ECHO = 10;
const uint8_t ENDERECO_LCD = 0x27;

const unsigned long TIMEOUT_ECO = 30000;
const unsigned int DIST_MIN = 2;
const unsigned int DIST_MAX = 400;
const int SUSPEITAS_PARA_FALHA = 3;
const unsigned long INTERVALO_SENSOR = 250;
const unsigned long INTERVALO_LCD = 1000;

LiquidCrystal_I2C lcd(ENDERECO_LCD, 16, 2);

bool sensorOk = true;
int suspeitas = 0;
unsigned int ultimaBoa = 0;
bool lcdOk = false;
unsigned long antesSensor = 0;
unsigned long antesLcd = 0;
uint16_t boots = 0;         // WDT

void registrar(char nivel, const char* origem, const char* msg) {
  Serial.print('[');
  Serial.print(millis());
  Serial.print("] ");
  Serial.print(nivel);
  Serial.print(' ');
  Serial.print(origem);
  Serial.print(": ");
  Serial.println(msg);
}

unsigned int medirCm() {
  digitalWrite(PINO_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PINO_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PINO_TRIG, LOW);
  unsigned long eco = pulseIn(PINO_ECHO, HIGH, TIMEOUT_ECO);
  return eco / 58;
}

void lerSensor() {
  unsigned int cm = medirCm();
  bool valida = cm >= DIST_MIN && cm <= DIST_MAX;
  if (valida) {
    suspeitas = 0;
    ultimaBoa = cm;
    if (!sensorOk) {
      sensorOk = true;
      registrar('I', "SENSOR", "voltou a responder");
    }
  } else if (sensorOk) {
    suspeitas++;
    registrar('W', "SENSOR", "leitura suspeita");
    if (suspeitas >= SUSPEITAS_PARA_FALHA) {
      sensorOk = false;
      registrar('E', "SENSOR", "falha confirmada");
    }
  }
}

void iniciarLcd() {
  lcd.init();
  lcd.backlight();
  Wire.setWireTimeout(25000, true);
}

bool lcdPresente() {
  Wire.beginTransmission(ENDERECO_LCD);
  return Wire.endTransmission() == 0;
}

void escreverLinha(uint8_t linha, const char* texto) {
  char buf[17];
  snprintf(buf, sizeof(buf), "%-16s", texto);
  lcd.setCursor(0, linha);
  lcd.print(buf);
}

void atualizarLcd() {
  if (!lcdPresente()) {
    if (lcdOk) registrar('E', "LCD", "sem resposta no I2C");
    lcdOk = false;
    return;
  }
  if (!lcdOk) {
    iniciarLcd();
    lcdOk = true;
    registrar('I', "LCD", "reconectado e reinicializado");
  }
  char texto[17];
  if (sensorOk) snprintf(texto, sizeof(texto), "Dist: %u cm", ultimaBoa);
  else          snprintf(texto, sizeof(texto), "Dist: --- cm");
  escreverLinha(0, texto);
  escreverLinha(1, sensorOk ? "Sensor: OK" : "Sensor: FALHA");
}

void setup() {
  MCUSR = 0;                // WDT: limpa o motivo do último reset
  wdt_disable();            // WDT: desligado durante a inicialização

  pinMode(PINO_TRIG, OUTPUT);
  pinMode(PINO_ECHO, INPUT);
  Serial.begin(9600);
  Wire.begin();
  Wire.setWireTimeout(25000, true);

  EEPROM.get(0, boots);     // WDT: quantas vezes a placa já ligou
  if (boots == 0xFFFF) boots = 0;           // EEPROM nova vem com 0xFFFF
  boots++;
  EEPROM.put(0, boots);

  char msg[20];
  snprintf(msg, sizeof(msg), "boot #%u", boots);
  registrar('I', "SISTEMA", msg);

  wdt_enable(WDTO_2S);      // WDT: sem wdt_reset() por 2 s, o chip reinicia
}

void loop() {
  unsigned long agora = millis();

  if (agora - antesSensor >= INTERVALO_SENSOR) {
    antesSensor = agora;
    lerSensor();
  }
  if (agora - antesLcd >= INTERVALO_LCD) {
    antesLcd = agora;
    atualizarLcd();
  }

  // WDT: teste de travamento. Digite t no Monitor Serial e tecle Enter.
  if (Serial.available() && Serial.read() == 't') {
    registrar('W', "TESTE", "travando o loop de proposito");
    while (true) { }        // laço infinito: só o watchdog tira daqui
  }

  wdt_reset();              // WDT: "estou vivo", um único ponto, no fim da volta
}