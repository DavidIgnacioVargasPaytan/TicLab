// ============================================================
// P02 - EJERCICIO 4: Integracion con Dos Potenciometros
//       Control Bidimensional del Servo
// Arduino Mega 2560 | Wokwi | Taller IoT - EPIS UNSA
// ============================================================
// CONEXIONES:
//   HC-SR04    : TRIG=7, ECHO=6, VCC=5V, GND=GND
//   Potenc. 1  : izq=5V, der=GND, cursor=A0 (velocidad LCD)
//   Potenc. 2  : izq=5V, der=GND, cursor=A1 (posicion servo)
//   LCD I2C    : SDA=20, SCL=21, VCC=5V, GND=GND
//   Servo      : señal=9, VCC=5V, GND=GND
//   DS3231     : SDA=20, SCL=21, VCC=3.3V, GND=GND
// ── NUEVO en EJ.4: segundo potenciometro en A1 ─────────────
// ============================================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <RTClib.h>

// ── Pines ─────────────────────────────────────────────────
#define TRIG_PIN   7
#define ECHO_PIN   6
#define POT1_PIN   A0   // Pot 1: controla velocidad LCD
#define POT2_PIN   A1   // Pot 2: controla posicion servo
#define SERVO_PIN  9

// ── Objetos ────────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo miServo;
RTC_DS3231 rtc;

// ── Timers no bloqueantes ──────────────────────────────────
unsigned long tSensor  = 0;
unsigned long tLCD     = 0;   // timer dinamico del LCD
unsigned long tAlt     = 0;   // alternancia linea 2
unsigned long tCSV     = 0;
const long INT_SENSOR  = 200; // leer sensores rapido (200 ms)
const long INT_CSV     = 2000;

// ── Variables globales ─────────────────────────────────────
float distancia    = 0;
int   pot1Raw      = 0;    // A0: velocidad LCD
int   pot2Raw      = 0;    // A1: posicion servo
int   servoPos     = 90;
long  intervaloLCD = 1000; // ms entre actualizaciones LCD
bool  modoVel      = true; // true=mostrar velocidad, false=servo
bool  headerOK     = false;

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

  Serial.println("=== P02 Ejercicio 4 - Dos potenciometros ===");
  Serial.println("Pot1(A0)=velocidad LCD | Pot2(A1)=posicion servo");
}

// ──────────────────────────────────────────────────────────
void loop() {
  unsigned long ahora = millis();

  // ── Lectura de sensores cada 200 ms ─────────────────────
  if (ahora - tSensor >= INT_SENSOR) {
    tSensor = ahora;

    distancia = medirDistancia();

    // EJ. 4: leer ambos potenciometros
    pot1Raw = analogRead(POT1_PIN);
    pot2Raw = analogRead(POT2_PIN);

    // Pot1 -> velocidad de actualizacion del LCD (200-2000 ms)
    intervaloLCD = map(pot1Raw, 0, 1023, 200, 2000);

    // Pot2 -> posicion directa del servo (0-180°)
    servoPos = map(pot2Raw, 0, 1023, 0, 180);
    servoPos = constrain(servoPos, 0, 180); // seguridad
    miServo.write(servoPos);

    // Calcular porcentajes de cada potenciometro
    float pct1 = (pot1Raw / 1023.0) * 100.0;
    float pct2 = (pot2Raw / 1023.0) * 100.0;

    // Monitor Serial con porcentajes — facil de leer para la tabla
    Serial.print("Pot1:");     Serial.print(pot1Raw);
    Serial.print("(");         Serial.print(pct1, 1);
    Serial.print("%) Int:");   Serial.print(intervaloLCD);
    Serial.print("ms | Pot2:"); Serial.print(pot2Raw);
    Serial.print("(");         Serial.print(pct2, 1);
    Serial.print("%) Srv:");   Serial.print(servoPos);
    Serial.print(" Dist:");    Serial.println(distancia, 1);
  }

  // ── Actualizar LCD linea 1 segun intervalo dinamico ─────
  if (ahora - tLCD >= intervaloLCD) {
    tLCD = ahora;

    // Linea 1: siempre muestra distancia
    lcd.setCursor(0, 0);
    lcd.print("Dist:");
    if (distancia < 0) {
      lcd.print(" Sin eco    ");
    } else {
      lcd.print(distancia, 1);
      lcd.print(" cm     ");
    }
  }

  // ── Alternar linea 2 cada 1 segundo ─────────────────────
  if (ahora - tAlt >= 1000) {
    tAlt  = ahora;
    modoVel = !modoVel;

    lcd.setCursor(0, 1);
    if (modoVel) {
      // Mostrar velocidad de actualizacion
      lcd.print("Vel:");
      lcd.print(intervaloLCD);
      lcd.print("ms      ");
    } else {
      // Mostrar posicion del servo
      lcd.print("Srv:");
      lcd.print(servoPos);
      lcd.print((char)223);   // simbolo °
      lcd.print("         ");
    }
  }

  // ── CSV por Serial cada 2 s ─────────────────────────────
  if (ahora - tCSV >= INT_CSV) {
    tCSV = ahora;
    DateTime now = rtc.now();

    if (!headerOK) {
      Serial.println("Timestamp,Dist_cm,Pot1_raw,IntLCD_ms,Pot2_raw,Servo_deg");
      headerOK = true;
    }

    Serial.print(now.timestamp(DateTime::TIMESTAMP_FULL));
    Serial.print(','); Serial.print(distancia, 1);
    Serial.print(','); Serial.print(pot1Raw);
    Serial.print(','); Serial.print(intervaloLCD);
    Serial.print(','); Serial.print(pot2Raw);
    Serial.print(','); Serial.println(servoPos);
  }
}
