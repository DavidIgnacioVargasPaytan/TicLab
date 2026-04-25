#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <RTClib.h>

// ── Pines ─────────────────────────────────────────────────
#define TRIG_PIN   7
#define ECHO_PIN   6
#define POT_PIN    A0
#define SERVO_PIN  9

// ── Umbrales servo ─────────────────────────────────────────
#define DIST_CERCA 20    // cm -> servo 0°
#define DIST_MEDIA 60    // cm -> servo 90°  |  >60 -> 180°

// ── Objetos ────────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo miServo;
RTC_DS3231 rtc;

// ── Timers no bloqueantes ──────────────────────────────────
unsigned long tSensor = 0;
unsigned long tCSV    = 0;
const long INT_SENSOR = 500;    // leer sensores cada 500 ms
const long INT_CSV    = 2000;   // imprimir CSV cada 2 s

// ── Variables globales ─────────────────────────────────────
float distancia = 0;
int   potRaw    = 0;
int   servoPos  = 90;
float temp      = 0;
float volt      = 0;
bool  headerOK  = false;

// ── Funcion: medir distancia HC-SR04 ──────────────────────
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30 ms
  if (dur == 0) return -1;                   // sin eco
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

  // Pantalla de bienvenida
  lcd.setCursor(0, 0); lcd.print("  IoT Station   ");
  lcd.setCursor(0, 1); lcd.print("  EPIS UNSA     ");
  delay(2000);
}

// ──────────────────────────────────────────────────────────
void loop() {
  int porcentaje = map(potRaw, 0, 1023, 0, 100);
  unsigned long ahora = millis();

  // ── Lectura de sensores cada 500 ms ─────────────────────
  if (ahora - tSensor >= INT_SENSOR) {
    tSensor = ahora;

    distancia = medirDistancia();
    potRaw    = analogRead(POT_PIN);

    // EJ. 1: convertir ADC a temperatura y voltaje
    temp = map(potRaw, 0, 1023, 0, 100);    // 0-100 °C
    volt = potRaw * (5.0 / 1023.0);         // voltaje real

    // Control servo por distancia
    if (distancia > 0 && distancia <= DIST_CERCA) servoPos = 0;
    else if (distancia <= DIST_MEDIA)              servoPos = 90;
    else                                           servoPos = 180;
    miServo.write(servoPos);

    // ── LCD linea 1: distancia ──────────────────────────
    lcd.setCursor(0, 0);
    lcd.print("Dist:");
    if (distancia < 0) {
      lcd.print(" Sin eco    ");
    } else {
      lcd.print(distancia, 1);
      lcd.print(" cm     ");
    }

    // ── LCD linea 2: temperatura simulada (EJ. 1) ───────
    lcd.setCursor(0, 1);
    lcd.print("Temp:");
    lcd.print(temp, 1);
    lcd.print((char)223);   // simbolo °
    lcd.print("C       ");

    // ── Monitor Serial: depuracion ──────────────────────
    Serial.print("Dist:");    Serial.print(distancia, 1);
    Serial.print(" Pot:");    Serial.print(potRaw);
    Serial.print(" Volt:");   Serial.print(volt, 3);
    Serial.print("V Temp:");  Serial.print(temp, 1);
    Serial.print("C Srv:");   Serial.println(servoPos);
    Serial.print("Potenciometro: "); Serial.print(porcentaje); Serial.println("%");
  }

  // ── CSV por Serial cada 2 s (sustituye microSD) ─────────
  if (ahora - tCSV >= INT_CSV) {
    tCSV = ahora;
    DateTime now = rtc.now();

    // Imprimir cabecera una sola vez
    if (!headerOK) {
      Serial.println("Timestamp,Dist_cm,Pot_raw,Volt_V,Temp_C,Servo_deg");
      headerOK = true;
    }

    // Linea de datos
    Serial.print(now.timestamp(DateTime::TIMESTAMP_FULL));
    Serial.print(','); Serial.print(distancia, 1);
    Serial.print(','); Serial.print(potRaw);
    Serial.print(','); Serial.print(volt, 3);
    Serial.print(','); Serial.print(temp, 1);
    Serial.print(','); Serial.println(servoPos);
  }
}
