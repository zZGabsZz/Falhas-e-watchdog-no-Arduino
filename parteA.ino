// Lab 3 — Parte A: versão ingênua (nenhuma verificação de falha)
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int PINO_TRIG = 9;
const int PINO_ECHO = 10;

LiquidCrystal_I2C lcd(0x27, 16, 2);   // endereço 0x27 (alguns módulos usam 0x3F)

void setup() {
  pinMode(PINO_TRIG, OUTPUT);
  pinMode(PINO_ECHO, INPUT);
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
}

void loop() {
  unsigned long inicio = millis();

  // pulso de 10 µs no TRIG dispara uma medição
  digitalWrite(PINO_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PINO_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PINO_TRIG, LOW);

  unsigned long eco = pulseIn(PINO_ECHO, HIGH);   // espera o eco voltar
  unsigned int cm = eco / 58;                      // 58 µs por cm (ida e volta)

  lcd.setCursor(0, 0);
  lcd.print("Dist: ");
  lcd.print(cm);
  lcd.print(" cm    ");

  Serial.print(cm);
  Serial.print(" cm   volta do loop = ");
  Serial.print(millis() - inicio);
  Serial.println(" ms");

  delay(250);
}