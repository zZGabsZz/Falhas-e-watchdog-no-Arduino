// Lab 3 — Parte B: detecção de falhas no sensor e no LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int PINO_TRIG = 9;
const int PINO_ECHO = 10;
const uint8_t ENDERECO_LCD = 0x27;

const unsigned long TIMEOUT_ECO = 30000;   // µs: espera máxima pelo eco
const unsigned int DIST_MIN = 2;           // cm: faixa útil do HC-SR04
const unsigned int DIST_MAX = 400;
const int SUSPEITAS_PARA_FALHA = 3;        // leituras ruins seguidas = falha
const unsigned long INTERVALO_SENSOR = 250;   // ms
const unsigned long INTERVALO_LCD = 1000;     // ms

LiquidCrystal_I2C lcd(ENDERECO_LCD, 16, 2);

bool sensorOk = true;
int suspeitas = 0;
unsigned int ultimaBoa = 0;     // último valor válido, em cm
bool lcdOk = false;
unsigned long antesSensor = 0;
unsigned long antesLcd = 0;

// Registro: [instante] nível ORIGEM: mensagem   (I info · W aviso · E erro)
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

// ---------- Sensor ----------
unsigned int medirCm() {                    // devolve 0 se o eco não voltar
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
    suspeitas++;                            // 1 leitura ruim = suspeita
    registrar('W', "SENSOR", "leitura suspeita");
    if (suspeitas >= SUSPEITAS_PARA_FALHA) {
      sensorOk = false;                     // 3 seguidas = falha confirmada
      registrar('E', "SENSOR", "falha confirmada");
    }
  }
}

// ---------- LCD ----------
void iniciarLcd() {
  lcd.init();
  lcd.backlight();
  Wire.setWireTimeout(25000, true);         // I2C nunca espera mais que 25 ms
}

bool lcdPresente() {
  Wire.beginTransmission(ENDERECO_LCD);
  return Wire.endTransmission() == 0;       // 0 = o módulo respondeu (ACK)
}

void escreverLinha(uint8_t linha, const char* texto) {
  char buf[17];                             // 16 colunas + '\0'
  snprintf(buf, sizeof(buf), "%-16s", texto);   // completa com espaços
  lcd.setCursor(0, linha);
  lcd.print(buf);
}

void atualizarLcd() {
  if (!lcdPresente()) {
    if (lcdOk) registrar('E', "LCD", "sem resposta no I2C");
    lcdOk = false;
    return;                                 // o resto do sistema continua
  }
  if (!lcdOk) {                             // acabou de voltar: reinicializa
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
  pinMode(PINO_TRIG, OUTPUT);
  pinMode(PINO_ECHO, INPUT);
  Serial.begin(9600);
  Wire.begin();
  Wire.setWireTimeout(25000, true);
  registrar('I', "SISTEMA", "iniciado");
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
}