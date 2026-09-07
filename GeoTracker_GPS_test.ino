#include <Arduino.h>
#include <TinyGPSPlus.h>

// ======================================================
// GeoScope v1.0
// NEO-M8N GPS - Serial Diagnostic Test
//
// GPS UART:
//   GPS TX -> ESP32 GPIO16 (RX2)
//   GPS RX -> ESP32 GPIO17 (TX2)
//
// GPS baud:     9600
// Serial baud: 115200
// ======================================================

// -----------------------------
// Pin Mapping
// -----------------------------
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17

// -----------------------------
// GPS UART
// -----------------------------
HardwareSerial GPSserial(2);

// -----------------------------
// TinyGPS++
// -----------------------------
TinyGPSPlus gps;

// GSA fix type:
// 1 = No Fix
// 2 = 2D Fix
// 3 = 3D Fix
//
// Your M8N is outputting $GNGSA sentences.
TinyGPSCustom gsaFixType(gps, "GNGSA", 2);

// -----------------------------
// Timing
// -----------------------------
unsigned long lastPrint = 0;
const unsigned long PRINT_INTERVAL = 1000;

// ======================================================
// FIX STATUS
// ======================================================

const char* getFixStatus()
{
  if (!gsaFixType.isValid()) {
    if (gps.location.isValid())
      return "VALID";
    else
      return "NO FIX";
  }

  int fix = atoi(gsaFixType.value());

  switch (fix) {
    case 1:
      return "NO FIX";

    case 2:
      return "2D";

    case 3:
      return "3D";

    default:
      return "UNKNOWN";
  }
}

// ======================================================
// DATE
// ======================================================

void printDate()
{
  if (!gps.date.isValid()) {
    Serial.print("N/A");
    return;
  }

  if (gps.date.month() < 10) Serial.print('0');
  Serial.print(gps.date.month());
  Serial.print('/');

  if (gps.date.day() < 10) Serial.print('0');
  Serial.print(gps.date.day());
  Serial.print('/');

  Serial.print(gps.date.year());
}

// ======================================================
// UTC TIME
// ======================================================

void printTime()
{
  if (!gps.time.isValid()) {
    Serial.print("N/A");
    return;
  }

  if (gps.time.hour() < 10) Serial.print('0');
  Serial.print(gps.time.hour());
  Serial.print(':');

  if (gps.time.minute() < 10) Serial.print('0');
  Serial.print(gps.time.minute());
  Serial.print(':');

  if (gps.time.second() < 10) Serial.print('0');
  Serial.print(gps.time.second());

  Serial.print(" UTC");
}

// ======================================================
// GPS REPORT
// ======================================================

void printGPSReport()
{
  Serial.println();
  Serial.println("========================================");
  Serial.println("        GeoScope GPS Reading");
  Serial.println("========================================");

  // -------------------------
  // Fix
  // -------------------------
  Serial.print("Fix:           ");
  Serial.println(getFixStatus());

  // -------------------------
  // Satellites
  // -------------------------
  Serial.print("Satellites:    ");

  if (gps.satellites.isValid()) {
    Serial.println(gps.satellites.value());
  }
  else {
    Serial.println("N/A");
  }

  // -------------------------
  // Location
  // -------------------------
  Serial.print("Latitude:      ");

  if (gps.location.isValid()) {
    Serial.println(gps.location.lat(), 6);
  }
  else {
    Serial.println("N/A");
  }

  Serial.print("Longitude:     ");

  if (gps.location.isValid()) {
    Serial.println(gps.location.lng(), 6);
  }
  else {
    Serial.println("N/A");
  }

  // -------------------------
  // GPS Altitude
  // -------------------------
  Serial.print("GPS Altitude:  ");

  if (gps.altitude.isValid()) {
    Serial.print(gps.altitude.meters(), 1);
    Serial.println(" m");
  }
  else {
    Serial.println("N/A");
  }

  // -------------------------
  // Speed
  // -------------------------
  Serial.print("Speed:         ");

  if (gps.speed.isValid()) {
    Serial.print(gps.speed.mps(), 2);
    Serial.println(" m/s");
  }
  else {
    Serial.println("N/A");
  }

  // -------------------------
  // Course Over Ground
  // -------------------------
  Serial.print("Course:        ");

  if (gps.course.isValid()) {
    Serial.print(gps.course.deg(), 1);
    Serial.println(" deg");
  }
  else {
    Serial.println("N/A");
  }

  // -------------------------
  // Date
  // -------------------------
  Serial.print("GPS Date:      ");
  printDate();
  Serial.println();

  // -------------------------
  // Time
  // -------------------------
  Serial.print("GPS Time:      ");
  printTime();
  Serial.println();

  // -------------------------
  // HDOP
  // Diagnostic value
  // -------------------------
  Serial.print("HDOP:          ");

  if (gps.hdop.isValid()) {
    Serial.println(gps.hdop.hdop(), 2);
  }
  else {
    Serial.println("N/A");
  }

  // -------------------------
  // Data Age
  // -------------------------
  Serial.print("Position Age:  ");

  if (gps.location.isValid()) {
    Serial.print(gps.location.age());
    Serial.println(" ms");
  }
  else {
    Serial.println("N/A");
  }

  // -------------------------
  // UART Diagnostics
  // -------------------------
  Serial.print("Chars RX:      ");
  Serial.println(gps.charsProcessed());

  Serial.print("Checksum OK:   ");
  Serial.println(gps.passedChecksum());

  Serial.print("Checksum Fail: ");
  Serial.println(gps.failedChecksum());

  Serial.println("========================================");
}

// ======================================================
// SETUP
// ======================================================

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" GeoScope v1.0 - NEO-M8N GPS Test");
  Serial.println("========================================");
  Serial.println();

  Serial.println("Initializing GPS...");
  Serial.println("GPS UART: 9600 baud");
  Serial.println("GPS RX:   GPIO16");
  Serial.println("GPS TX:   GPIO17");

  GPSserial.begin(
    9600,
    SERIAL_8N1,
    GPS_RX_PIN,
    GPS_TX_PIN
  );

  delay(500);

  Serial.println();
  Serial.println("GPS UART initialized.");
  Serial.println("Waiting for GNSS data...");
}

// ======================================================
// LOOP
// ======================================================

void loop()
{
  // Always continuously feed incoming GPS characters
  // into TinyGPS++.
  while (GPSserial.available()) {
    gps.encode(GPSserial.read());
  }

  // Print report once per second
  if (millis() - lastPrint >= PRINT_INTERVAL) {

    lastPrint = millis();

    // Detect a completely dead UART connection
    if (millis() > 5000 && gps.charsProcessed() < 10) {

      Serial.println();
      Serial.println("WARNING: No GPS data received.");
      Serial.println("Check GPS power, TX/RX wiring, and baud rate.");

      return;
    }

    printGPSReport();
  }
}