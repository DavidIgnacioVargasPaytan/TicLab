// ============================================================
// P02 - EJERCICIO 2 CORREGIDO: Sistema de Alerta por Zonas
// Arduino Mega 2560 | Wokwi | Taller IoT - EPIS UNSA
// ============================================================
// COMO USAR EN WOKWI:
//   - Haz clic en el HC-SR04 durante la simulacion
//   - Aparece un slider: arrastralo para cambiar la distancia
//   - 0-15 cm  → ZONA ROJA   → servo 0°
//   - 16-40 cm → ZONA AMARILLA → servo 90°
//   - >40 cm   → ZONA VERDE  → servo 180°
//   - El potenciometro NO cambia la zona (es dato extra)
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

// ── Zonas de seguridad ─────────────────────────────────────
#define ZONA_ROJA     15    // 0 – 15 cm
#define ZONA_AMARILLA 40    // 16 – 40 cm
                            // > 40 cm = VERDE

// ── Objetos ────────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo miServo;
RTC_DS3231 rtc;

// ── Timers no bloqueantes ──────────────────────────────────
unsigned long tSensor = 0;
unsigned long tCSV    = 0;
const long INT_SENSOR = 500;
const long INT_CSV    = 2000;

// ── Variables globales ─────────────────────────────────────
float distancia = 0;
int   potRaw    = 0;
int   servoPos  = 90;
char  zona      = 'V';
bool  headerOK  = false;

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

  Serial.println("=== Ejercicio 2: Zonas de Proximidad ===");
  Serial.println("Mueve el slider del HC-SR04 para cambiar la zona");
}

// ──────────────────────────────────────────────────────────
void loop() {
  unsigned long ahora = millis();

  // ── Lectura de sensores cada 500 ms ─────────────────────
  if (ahora - tSensor >= INT_SENSOR) {
    tSensor = ahora;

    distancia = medirDistancia();
    potRaw    = analogRead(POT_PIN);

    // ── CORRECCION: detectar zona solo si hay eco valido ──
    if (distancia < 0) {
      // Sin eco: mantener ultima zona, no cambiar servo
      // Solo actualizar display
    } else if (distancia <= ZONA_ROJA) {
      zona     = 'R';
      servoPos = 0;
    } else if (distancia <= ZONA_AMARILLA) {
      zona     = 'A';
      servoPos = 90;
    } else {
      zona     = 'V';
      servoPos = 180;
    }

    // Mover servo solo si hay lectura valida
    if (distancia >= 0) {
      miServo.write(servoPos);
    }

    // ── LCD linea 1: zona o sin eco ─────────────────────
    lcd.setCursor(0, 0);
    if (distancia < 0) {
      lcd.print("  Sin objeto    ");
    } else if (zona == 'R') {
      lcd.print("ZONA: ROJA      ");
    } else if (zona == 'A') {
      lcd.print("ZONA: AMARILLA  ");
    } else {
      lcd.print("ZONA: VERDE     ");
    }

    // ── LCD linea 2: distancia actual ───────────────────
    lcd.setCursor(0, 1);
    lcd.print("Dist:");
    if (distancia < 0) {
      lcd.print(" ---        ");
    } else {
      lcd.print(distancia, 1);
      lcd.print(" cm     ");
    }

    // ── Serial: depuracion ──────────────────────────────
    Serial.print("Dist:");
    if (distancia < 0) Serial.print("SinEco");
    else Serial.print(distancia, 1);
    Serial.print(" Zona:"); Serial.print(zona);
    Serial.print(" Pot:");  Serial.print(potRaw);
    Serial.print(" Srv:");  Serial.println(servoPos);
  }

  // ── CSV por Serial cada 2 s ─────────────────────────────
  if (ahora - tCSV >= INT_CSV) {
    tCSV = ahora;
    DateTime now = rtc.now();

    if (!headerOK) {
      Serial.println("Timestamp,Dist_cm,Pot_raw,Servo_deg,Zona");
      headerOK = true;
    }

    // Solo guardar registro si hay lectura valida
    if (distancia >= 0) {
      Serial.print(now.timestamp(DateTime::TIMESTAMP_FULL));
      Serial.print(','); Serial.print(distancia, 1);
      Serial.print(','); Serial.print(potRaw);
      Serial.print(','); Serial.print(servoPos);
      Serial.print(','); Serial.println(zona);
    }
  }
}
