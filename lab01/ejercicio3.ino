// ============================================================
// P02 - EJERCICIO 3: Data Logger con Timestamp
//       (Ejercicio 1 + 2 integrados, guardado cada 2 s)
// Arduino Mega 2560 | Wokwi | Taller IoT - EPIS UNSA
// ============================================================
// NOTA WOKWI: La microSD no genera archivo real en simulacion.
// El CSV se imprime por Serial Monitor. Para obtener el archivo:
//   1. Corre la simulacion 3+ minutos
//   2. Copia todo el Serial Monitor (Ctrl+A, Ctrl+C)
//   3. Pega en Notepad y guarda como DATOS.CSV
//   4. Abre en Google Sheets para analisis y grafico
// ============================================================
// CONEXIONES:
//   HC-SR04  : TRIG=7, ECHO=6, VCC=5V, GND=GND
//   Potenc.  : izq=5V, der=GND, cursor=A0
//   LCD I2C  : SDA=20, SCL=21, VCC=5V, GND=GND
//   Servo    : señal=9, VCC=5V, GND=GND
//   DS3231   : SDA=20, SCL=21, VCC=3.3V, GND=GND
// ============================================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <RTClib.h>

// ── Pines ─────────────────────────────────────────────────
#define TRIG_PIN   7
#define ECHO_PIN   6
#define POT_PIN    A0
#define SERVO_PIN  9

// ── Zonas (EJ. 2) ──────────────────────────────────────────
#define ZONA_ROJA     15
#define ZONA_AMARILLA 40

// ── Objetos ────────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo miServo;
RTC_DS3231 rtc;

// ── Timers no bloqueantes ──────────────────────────────────
unsigned long tSensor = 0;
unsigned long tCSV    = 0;
const long INT_SENSOR = 500;
const long INT_CSV    = 2000;   // EJ. 3: guardar cada 2 s

// ── Variables globales ─────────────────────────────────────
float distancia = 0;
int   potRaw    = 0;
int   servoPos  = 90;
float temp      = 0;
float volt      = 0;
char  zona      = 'V';
bool  headerOK  = false;

// ── Estadisticas en tiempo real (para Tabla 3) ─────────────
float distMin =  9999;
float distMax = -9999;
float distSum =  0;
float tempMin =  9999;
float tempMax = -9999;
float tempSum =  0;
int   nRegistros = 0;

// ── Funcion: medir distancia HC-SR04 ──────────────────────
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH, 30000);
  if (dur == 0) return -1;
  return (dur * 0.0343) / 2.0;
}

// ──────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  lcd.init();
  lcd.backlight();
  miServo.attach(SERVO_PIN);

  rtc.begin();
  if (rtc.lostPower())
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  lcd.setCursor(0, 0); lcd.print("  IoT Station   ");
  lcd.setCursor(0, 1); lcd.print("  EPIS UNSA     ");
  delay(2000);

  Serial.println("=== P02 Data Logger iniciado ===");
}

// ──────────────────────────────────────────────────────────
void loop() {
  unsigned long ahora = millis();

  // ── Lectura de sensores cada 500 ms ─────────────────────
  if (ahora - tSensor >= INT_SENSOR) {
    tSensor = ahora;

    distancia = medirDistancia();
    potRaw    = analogRead(POT_PIN);
    temp      = map(potRaw, 0, 1023, 0, 100);  // EJ. 1
    volt      = potRaw * (5.0 / 1023.0);

    // Zona y servo (EJ. 2)
    if (distancia > 0 && distancia <= ZONA_ROJA) {
      zona = 'R'; servoPos = 0;
    } else if (distancia > 0 && distancia <= ZONA_AMARILLA) {
      zona = 'A'; servoPos = 90;
    } else {
      zona = 'V'; servoPos = 180;
    }
    miServo.write(servoPos);

    // LCD linea 1: zona
    lcd.setCursor(0, 0);
    if      (zona == 'R') lcd.print("ZONA: ROJA      ");
    else if (zona == 'A') lcd.print("ZONA: AMARILLA  ");
    else                  lcd.print("ZONA: VERDE     ");

    // LCD linea 2: temperatura
    lcd.setCursor(0, 1);
    lcd.print("Temp:");
    lcd.print(temp, 1);
    lcd.print((char)223);
    lcd.print("C       ");

    // Serial depuracion
    Serial.print(">> Dist:"); Serial.print(distancia, 1);
    Serial.print(" Temp:");   Serial.print(temp, 1);
    Serial.print("C Zona:");  Serial.print(zona);
    Serial.print(" Srv:");    Serial.println(servoPos);
  }

  // ── CSV + estadisticas cada 2 s ─────────────────────────
  if (ahora - tCSV >= INT_CSV) {
    tCSV = ahora;
    DateTime now = rtc.now();

    // Cabecera CSV (una vez)
    if (!headerOK) {
      Serial.println("Timestamp,Dist_cm,Pot_raw,Volt_V,Temp_C,Servo_deg,Zona");
      headerOK = true;
    }

    // Linea CSV
    Serial.print(now.timestamp(DateTime::TIMESTAMP_FULL));
    Serial.print(','); Serial.print(distancia, 1);
    Serial.print(','); Serial.print(potRaw);
    Serial.print(','); Serial.print(volt, 3);
    Serial.print(','); Serial.print(temp, 1);
    Serial.print(','); Serial.print(servoPos);
    Serial.print(','); Serial.println(zona);

    // Actualizar estadisticas (solo lecturas validas)
    if (distancia > 0) {
      nRegistros++;
      distSum += distancia;
      if (distancia < distMin) distMin = distancia;
      if (distancia > distMax) distMax = distancia;
      tempSum += temp;
      if (temp < tempMin) tempMin = temp;
      if (temp > tempMax) tempMax = temp;

      // Imprimir estadisticas cada 10 registros
      if (nRegistros % 10 == 0) {
        Serial.println("--- ESTADISTICAS PARCIALES ---");
        Serial.print("N registros: "); Serial.println(nRegistros);
        Serial.print("Dist  min/max/prom: ");
        Serial.print(distMin,1); Serial.print(" / ");
        Serial.print(distMax,1); Serial.print(" / ");
        Serial.println(distSum / nRegistros, 1);
        Serial.print("Temp  min/max/prom: ");
        Serial.print(tempMin,1); Serial.print(" / ");
        Serial.print(tempMax,1); Serial.print(" / ");
        Serial.println(tempSum / nRegistros, 1);
        Serial.println("------------------------------");
      }
    }
  }
}
