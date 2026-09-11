/*
  GeoTracker V1.0
  Full Integration Skeleton

  Confirmed subsystem pin plan:

  TFT SPI:
    DIN/MOSI -> GPIO23
    CLK/SCK  -> GPIO18
    MISO     -> GPIO19 shared with SD
    CS       -> GPIO5
    DC       -> GPIO2
    RST      -> GPIO4
    BL       -> 3.3V

  SD SPI:
    VCC  -> 5V
    GND  -> GND
    SCK  -> GPIO18
    MOSI -> GPIO23
    MISO -> GPIO19
    CS   -> GPIO13

  Main I2C Bus:
    SDA -> GPIO21
    SCL -> GPIO22
    Devices:
      OLED #1 at 0x3C
      BME280 at 0x76 or 0x77
      MPU6050 at 0x68 or 0x69
      BMM150 at 0x13
      Numberpad, address set below

  Second I2C Bus:
    SDA -> GPIO32
    SCL -> GPIO33
    Device:
      OLED #2 at 0x3C

  GPS:
    GPS TX -> ESP32 RX2 GPIO16
    GPS RX -> ESP32 TX2 GPIO17
    Baud   -> 9600
*/

#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_SSD1306.h>

#include <Adafruit_BME280.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "DFRobot_BMM150.h"

#include <TinyGPSPlus.h>

// =====================================================
// Pin Definitions
// =====================================================

// TFT
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4
#define TFT_CLK   18
#define TFT_MOSI  23
#define TFT_MISO  19

// SD
#define SD_CS     13

// I2C
#define MAIN_SDA  21
#define MAIN_SCL  22

#define OLED2_SDA 32
#define OLED2_SCL 33

// OLED
#define OLED_ADDR   0x3C
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_RESET  -1

// GPS
#define GPS_RX 16
#define GPS_TX 17
#define GPS_BAUD 9600

// Numberpad
// Change this if your numberpad scanner showed another address.
#define KEYPAD_ADDR 0x65

// =====================================================
// Display Objects
// =====================================================

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

TwoWire I2C_OLED2 = TwoWire(1);

Adafruit_SSD1306 oledLeft(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 oledRight(OLED_WIDTH, OLED_HEIGHT, &I2C_OLED2, OLED_RESET);

// =====================================================
// Sensor Objects
// =====================================================

Adafruit_BME280 bme;
Adafruit_MPU6050 mpu;

DFRobot_BMM150_I2C bmmAddr1(&Wire, I2C_ADDRESS_1);  // 0x10
DFRobot_BMM150_I2C bmmAddr2(&Wire, I2C_ADDRESS_2);  // 0x11
DFRobot_BMM150_I2C bmmAddr3(&Wire, I2C_ADDRESS_3);  // 0x12
DFRobot_BMM150_I2C bmmAddr4(&Wire, I2C_ADDRESS_4);  // 0x13

DFRobot_BMM150_I2C *bmm = NULL;

// GPS
TinyGPSPlus gps;
HardwareSerial GPSSerial(2);

// =====================================================
// Status Flags
// =====================================================

bool tftOK = false;
bool oled1OK = false;
bool oled2OK = false;

bool bmeOK = false;
bool mpuOK = false;
bool bmmOK = false;
bool sdOK = false;
bool gpsSerialOK = false;
bool keypadOK = false;

uint8_t bmeAddress = 0x00;
uint8_t mpuAddress = 0x00;
uint8_t bmmAddress = 0x00;

// =====================================================
// Sensor Values
// =====================================================

// BME280
float tempC = 0.0;
float humidity = 0.0;
float pressureHpa = 0.0;
float pressureAltM = 0.0;

// MPU6050
float accelX = 0.0;
float accelY = 0.0;
float accelZ = 0.0;
float gyroX = 0.0;
float gyroY = 0.0;
float gyroZ = 0.0;
float pitchDeg = 0.0;
float rollDeg = 0.0;
float accelMagnitude = 0.0;

// BMM150
float magX = 0.0;
float magY = 0.0;
float magZ = 0.0;
float fieldStrength = 0.0;
float headingDeg = 0.0;

// GPS
double gpsLat = 0.0;
double gpsLon = 0.0;
double gpsAltM = 0.0;
double gpsSpeedKmph = 0.0;
double gpsCourseDeg = 0.0;
int gpsSatellites = 0;
double gpsHDOP = 0.0;

bool gpsFix = false;

// Keypad
int lastKeypadRaw = -1;

// SD logging
const char *logFolder = "/GEOTRK";
const char *logFile = "/GEOTRK/LOG.CSV";
unsigned long logRow = 0;

// Timing
unsigned long lastSensorRead = 0;
unsigned long lastOLEDUpdate = 0;
unsigned long lastTFTUpdate = 0;
unsigned long lastSDLog = 0;

int tftScreen = 0;

#define SEA_LEVEL_HPA 1013.25

// =====================================================
// Utility Functions
// =====================================================

float wrap360(float angle) {
  while (angle < 0) angle += 360.0;
  while (angle >= 360.0) angle -= 360.0;
  return angle;
}

String cardinalDirection(float heading) {
  if (heading >= 337.5 || heading < 22.5) return "N";
  if (heading >= 22.5 && heading < 67.5) return "NE";
  if (heading >= 67.5 && heading < 112.5) return "E";
  if (heading >= 112.5 && heading < 157.5) return "SE";
  if (heading >= 157.5 && heading < 202.5) return "S";
  if (heading >= 202.5 && heading < 247.5) return "SW";
  if (heading >= 247.5 && heading < 292.5) return "W";
  return "NW";
}

bool i2cDevicePresent(TwoWire &bus, uint8_t address) {
  bus.beginTransmission(address);
  return bus.endTransmission() == 0;
}

void scanMainI2C() {
  Serial.println();
  Serial.println("Scanning main I2C bus GPIO21/GPIO22...");

  int devices = 0;

  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Main I2C device found at 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
      devices++;
    }
  }

  Serial.print("Main I2C devices found: ");
  Serial.println(devices);
}

void drawTFTHeader(const char *title) {
  tft.fillRect(0, 0, 320, 30, ILI9341_NAVY);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 7);
  tft.print(title);
}

// =====================================================
// Initialization Functions
// =====================================================

void beginDisplays() {
  Serial.println("Starting displays...");

  Wire.begin(MAIN_SDA, MAIN_SCL);
  Wire.setClock(100000);

  I2C_OLED2.begin(OLED2_SDA, OLED2_SCL, 400000);

  SPI.begin(TFT_CLK, TFT_MISO, TFT_MOSI);

  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  tftOK = true;

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(25, 80);
  tft.println("GeoTracker V1.0");

  tft.setCursor(25, 115);
  tft.println("Booting...");

  oled1OK = oledLeft.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false);
  oled2OK = oledRight.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false);

  Serial.print("OLED #1: ");
  Serial.println(oled1OK ? "OK" : "FAIL");

  Serial.print("OLED #2: ");
  Serial.println(oled2OK ? "OK" : "FAIL");
}

void beginBME280() {
  if (bme.begin(0x76, &Wire)) {
    bmeOK = true;
    bmeAddress = 0x76;
  } else if (bme.begin(0x77, &Wire)) {
    bmeOK = true;
    bmeAddress = 0x77;
  }

  Serial.print("BME280: ");
  Serial.println(bmeOK ? "OK" : "FAIL");
}

void beginMPU6050() {
  if (mpu.begin(0x68, &Wire)) {
    mpuOK = true;
    mpuAddress = 0x68;
  } else if (mpu.begin(0x69, &Wire)) {
    mpuOK = true;
    mpuAddress = 0x69;
  }

  if (mpuOK) {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }

  Serial.print("MPU6050: ");
  Serial.println(mpuOK ? "OK" : "FAIL");
}

bool tryBMM150(DFRobot_BMM150_I2C &sensor, uint8_t address) {
  if (sensor.begin() == 0) {
    bmm = &sensor;
    bmmAddress = address;
    bmmOK = true;
    return true;
  }

  return false;
}

void beginBMM150() {
  bmmOK = false;
  bmm = NULL;

  if (tryBMM150(bmmAddr1, 0x10)) {
  } else if (tryBMM150(bmmAddr2, 0x11)) {
  } else if (tryBMM150(bmmAddr3, 0x12)) {
  } else if (tryBMM150(bmmAddr4, 0x13)) {
  }

  if (bmmOK) {
    bmm->setOperationMode(BMM150_POWERMODE_NORMAL);
    bmm->setPresetMode(BMM150_PRESETMODE_HIGHACCURACY);
    bmm->setRate(BMM150_DATA_RATE_10HZ);
    bmm->setMeasurementXYZ();
  }

  Serial.print("BMM150: ");
  Serial.println(bmmOK ? "OK" : "FAIL");
}

void beginGPS() {
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
  gpsSerialOK = true;

  Serial.println("GPS serial started.");
}

void beginSDCard() {
  Serial.println("Starting SD card...");

  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  if (!SD.begin(SD_CS, SPI, 1000000)) {
    sdOK = false;
    Serial.println("SD: FAIL");
    return;
  }

  if (SD.cardType() == CARD_NONE) {
    sdOK = false;
    Serial.println("SD: NO CARD");
    return;
  }

  sdOK = true;
  Serial.println("SD: OK");

  if (!SD.exists(logFolder)) {
    SD.mkdir(logFolder);
  }

  if (!SD.exists(logFile)) {
    File file = SD.open(logFile, FILE_WRITE);

    if (file) {
      file.println("row,millis,gps_fix,lat,lon,gps_alt_m,gps_speed_kmph,gps_course_deg,gps_sats,gps_hdop,temp_c,humidity_percent,pressure_hpa,pressure_alt_m,pitch_deg,roll_deg,heading_deg,field_strength_ut,keypad_raw");
      file.close();
      Serial.println("Created /GEOTRK/LOG.CSV");
    }
  }
}

void beginKeypad() {
  keypadOK = i2cDevicePresent(Wire, KEYPAD_ADDR);

  Serial.print("Numberpad at 0x");
  Serial.print(KEYPAD_ADDR, HEX);
  Serial.print(": ");
  Serial.println(keypadOK ? "OK" : "NOT FOUND");
}

// =====================================================
// Sensor Reading Functions
// =====================================================

void readBME280() {
  if (!bmeOK) return;

  tempC = bme.readTemperature();
  humidity = bme.readHumidity();
  pressureHpa = bme.readPressure() / 100.0;
  pressureAltM = 44330.0 * (1.0 - pow(pressureHpa / SEA_LEVEL_HPA, 0.1903));
}

void readMPU6050() {
  if (!mpuOK) return;

  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;

  mpu.getEvent(&accel, &gyro, &temp);

  accelX = accel.acceleration.x;
  accelY = accel.acceleration.y;
  accelZ = accel.acceleration.z;

  gyroX = gyro.gyro.x * 180.0 / PI;
  gyroY = gyro.gyro.y * 180.0 / PI;
  gyroZ = gyro.gyro.z * 180.0 / PI;

  accelMagnitude = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);

  pitchDeg = atan2(-accelX, sqrt(accelY * accelY + accelZ * accelZ)) * 180.0 / PI;
  rollDeg  = atan2(accelY, accelZ) * 180.0 / PI;
}

void readBMM150() {
  if (!bmmOK || bmm == NULL) return;

  sBmm150MagData_t magData = bmm->getGeomagneticData();

  magX = magData.x;
  magY = magData.y;
  magZ = magData.z;

  fieldStrength = sqrt((magX * magX) + (magY * magY) + (magZ * magZ));

  // Use DFRobot compass value as GeoTracker heading.
  headingDeg = wrap360(bmm->getCompassDegree());
}

void readGPS() {
  while (GPSSerial.available()) {
    gps.encode(GPSSerial.read());
  }

  gpsFix = gps.location.isValid();

  if (gps.location.isValid()) {
    gpsLat = gps.location.lat();
    gpsLon = gps.location.lng();
  }

  if (gps.altitude.isValid()) {
    gpsAltM = gps.altitude.meters();
  }

  if (gps.speed.isValid()) {
    gpsSpeedKmph = gps.speed.kmph();
  }

  if (gps.course.isValid()) {
    gpsCourseDeg = gps.course.deg();
  }

  if (gps.satellites.isValid()) {
    gpsSatellites = gps.satellites.value();
  }

  if (gps.hdop.isValid()) {
    gpsHDOP = gps.hdop.hdop();
  }
}

void readKeypadRaw() {
  if (!keypadOK) {
    lastKeypadRaw = -1;
    return;
  }

  Wire.requestFrom(KEYPAD_ADDR, 1);

  if (Wire.available()) {
    lastKeypadRaw = Wire.read();
  }
}

void readAllSubsystems() {
  readGPS();
  readBME280();
  readMPU6050();
  readBMM150();
  readKeypadRaw();
}

// =====================================================
// SD Logging
// =====================================================

void logToSD() {
  if (!sdOK) return;

  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  File file = SD.open(logFile, FILE_APPEND);

  if (!file) {
    Serial.println("Failed to open LOG.CSV for append.");
    return;
  }

  file.print(logRow);
  file.print(",");
  file.print(millis());
  file.print(",");
  file.print(gpsFix ? 1 : 0);
  file.print(",");

  if (gpsFix) {
    file.print(gpsLat, 6);
    file.print(",");
    file.print(gpsLon, 6);
  } else {
    file.print("NA,NA");
  }

  file.print(",");
  file.print(gpsAltM, 1);
  file.print(",");
  file.print(gpsSpeedKmph, 2);
  file.print(",");
  file.print(gpsCourseDeg, 2);
  file.print(",");
  file.print(gpsSatellites);
  file.print(",");
  file.print(gpsHDOP, 2);
  file.print(",");

  file.print(tempC, 2);
  file.print(",");
  file.print(humidity, 2);
  file.print(",");
  file.print(pressureHpa, 2);
  file.print(",");
  file.print(pressureAltM, 1);
  file.print(",");

  file.print(pitchDeg, 2);
  file.print(",");
  file.print(rollDeg, 2);
  file.print(",");
  file.print(headingDeg, 2);
  file.print(",");
  file.print(fieldStrength, 2);
  file.print(",");
  file.println(lastKeypadRaw);

  file.close();

  Serial.print("Logged row ");
  Serial.println(logRow);

  logRow++;
}

// =====================================================
// OLED Screens
// =====================================================

void drawLeftOLED() {
  if (!oled1OK) return;

  oledLeft.clearDisplay();
  oledLeft.setTextColor(SSD1306_WHITE);
  oledLeft.setTextSize(1);

  oledLeft.setCursor(0, 0);
  oledLeft.println("GEOTRACKER");
  oledLeft.println("NAV / COMPASS");
  oledLeft.drawLine(0, 18, 127, 18, SSD1306_WHITE);

  oledLeft.setCursor(0, 24);
  oledLeft.print("Head:");
  oledLeft.print(headingDeg, 0);
  oledLeft.print(" ");
  oledLeft.println(cardinalDirection(headingDeg));

  oledLeft.setCursor(0, 36);
  oledLeft.print("Sat:");
  oledLeft.print(gpsSatellites);
  oledLeft.print(" Fix:");
  oledLeft.println(gpsFix ? "Y" : "N");

  oledLeft.setCursor(0, 48);
  oledLeft.print("Spd:");
  oledLeft.print(gpsSpeedKmph, 1);
  oledLeft.println("kmh");

  oledLeft.display();
}

void drawRightOLED() {
  if (!oled2OK) return;

  oledRight.clearDisplay();
  oledRight.setTextColor(SSD1306_WHITE);
  oledRight.setTextSize(1);

  oledRight.setCursor(0, 0);
  oledRight.println("GEOTRACKER");
  oledRight.println("SYSTEM");
  oledRight.drawLine(0, 18, 127, 18, SSD1306_WHITE);

  oledRight.setCursor(0, 22);
  oledRight.print("BME:");
  oledRight.print(bmeOK ? "OK " : "NO ");
  oledRight.print("MPU:");
  oledRight.println(mpuOK ? "OK" : "NO");

  oledRight.setCursor(0, 34);
  oledRight.print("BMM:");
  oledRight.print(bmmOK ? "OK " : "NO ");
  oledRight.print("SD:");
  oledRight.println(sdOK ? "OK" : "NO");

  oledRight.setCursor(0, 46);
  oledRight.print("KEY:");
  oledRight.print(keypadOK ? "OK " : "NO ");
  oledRight.print("Raw:");
  oledRight.println(lastKeypadRaw);

  oledRight.display();
}

// =====================================================
// TFT Screens
// =====================================================

void drawTFTStatus() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("SYSTEM STATUS");

  tft.setTextSize(2);

  tft.setTextColor(oled1OK ? ILI9341_GREEN : ILI9341_RED);
  tft.setCursor(10, 45);
  tft.print("OLED1: ");
  tft.print(oled1OK ? "OK" : "FAIL");

  tft.setTextColor(oled2OK ? ILI9341_GREEN : ILI9341_RED);
  tft.setCursor(170, 45);
  tft.print("OLED2: ");
  tft.print(oled2OK ? "OK" : "FAIL");

  tft.setTextColor(bmeOK ? ILI9341_GREEN : ILI9341_RED);
  tft.setCursor(10, 80);
  tft.print("BME280: ");
  tft.print(bmeOK ? "OK" : "FAIL");

  tft.setTextColor(mpuOK ? ILI9341_GREEN : ILI9341_RED);
  tft.setCursor(10, 115);
  tft.print("MPU6050:");
  tft.print(mpuOK ? "OK" : "FAIL");

  tft.setTextColor(bmmOK ? ILI9341_GREEN : ILI9341_RED);
  tft.setCursor(10, 150);
  tft.print("BMM150: ");
  tft.print(bmmOK ? "OK" : "FAIL");

  tft.setTextColor(sdOK ? ILI9341_GREEN : ILI9341_RED);
  tft.setCursor(10, 185);
  tft.print("SD: ");
  tft.print(sdOK ? "OK" : "FAIL");

  tft.setTextColor(keypadOK ? ILI9341_GREEN : ILI9341_YELLOW);
  tft.setCursor(170, 185);
  tft.print("KEY:");
  tft.print(keypadOK ? "OK" : "NO");

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 222);
  tft.print("GeoTracker V1.0 integration skeleton");
}

void drawTFTGPS() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("GPS TRACKER");

  tft.setTextSize(2);

  tft.setTextColor(gpsFix ? ILI9341_GREEN : ILI9341_RED);
  tft.setCursor(10, 45);
  tft.print("Fix: ");
  tft.print(gpsFix ? "YES" : "NO");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(180, 45);
  tft.print("Sat:");
  tft.print(gpsSatellites);

  tft.setCursor(10, 80);
  tft.print("Lat:");
  tft.setCursor(80, 80);
  if (gpsFix) tft.print(gpsLat, 6);
  else tft.print("--");

  tft.setCursor(10, 115);
  tft.print("Lon:");
  tft.setCursor(80, 115);
  if (gpsFix) tft.print(gpsLon, 6);
  else tft.print("--");

  tft.setCursor(10, 150);
  tft.print("Alt:");
  tft.setCursor(80, 150);
  tft.print(gpsAltM, 1);
  tft.print("m");

  tft.setCursor(10, 185);
  tft.print("Spd:");
  tft.setCursor(80, 185);
  tft.print(gpsSpeedKmph, 1);
  tft.print("kmh");

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 222);
  tft.print("GPS RX=16 TX=17 | HDOP=");
  tft.print(gpsHDOP, 1);
}

void drawTFTEnvironment() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("ENVIRONMENT");

  tft.setTextSize(2);

  if (!bmeOK) {
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(20, 90);
    tft.print("BME280 FAIL");
    return;
  }

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 55);
  tft.print("Temp:");
  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(130, 55);
  tft.print(tempC, 1);
  tft.print(" C");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 95);
  tft.print("Hum:");
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(130, 95);
  tft.print(humidity, 1);
  tft.print(" %");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 135);
  tft.print("Press:");
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(130, 135);
  tft.print(pressureHpa, 1);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 175);
  tft.print("P Alt:");
  tft.setTextColor(ILI9341_MAGENTA);
  tft.setCursor(130, 175);
  tft.print(pressureAltM, 0);
  tft.print("m");

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 222);
  tft.print("BME280 address: 0x");
  tft.print(bmeAddress, HEX);
}

void drawTFTMotionCompass() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("MOTION / COMPASS");

  tft.setTextSize(2);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 45);
  tft.print("Pitch:");
  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(145, 45);
  tft.print(pitchDeg, 1);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 80);
  tft.print("Roll:");
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(145, 80);
  tft.print(rollDeg, 1);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 115);
  tft.print("Head:");
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(145, 115);
  tft.print(headingDeg, 1);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 150);
  tft.print("Dir:");
  tft.setTextColor(ILI9341_MAGENTA);
  tft.setCursor(145, 150);
  tft.print(cardinalDirection(headingDeg));

  int cx = 260;
  int cy = 130;
  int r = 45;

  tft.drawCircle(cx, cy, r, ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(cx - 3, cy - r - 12);
  tft.print("N");

  float angle = headingDeg * PI / 180.0;
  int x2 = cx + sin(angle) * 35;
  int y2 = cy - cos(angle) * 35;

  tft.drawLine(cx, cy, x2, y2, ILI9341_RED);
  tft.fillCircle(x2, y2, 4, ILI9341_RED);

  tft.setCursor(10, 222);
  tft.setTextColor(ILI9341_CYAN);
  tft.print("MPU + BMM150 active");
}

void drawTFTSDKeypad() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("LOGGING / INPUT");

  tft.setTextSize(2);

  tft.setTextColor(sdOK ? ILI9341_GREEN : ILI9341_RED);
  tft.setCursor(10, 50);
  tft.print("SD: ");
  tft.print(sdOK ? "OK" : "FAIL");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 90);
  tft.print("Rows:");
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(140, 90);
  tft.print(logRow);

  tft.setTextColor(keypadOK ? ILI9341_GREEN : ILI9341_YELLOW);
  tft.setCursor(10, 130);
  tft.print("Keypad:");
  tft.setCursor(140, 130);
  tft.print(keypadOK ? "OK" : "NO");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 170);
  tft.print("Raw:");
  tft.setCursor(140, 170);
  tft.print(lastKeypadRaw);

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 222);
  tft.print("Log file: /GEOTRK/LOG.CSV");
}

void updateTFTScreen() {
  if (tftScreen == 0) drawTFTStatus();
  else if (tftScreen == 1) drawTFTGPS();
  else if (tftScreen == 2) drawTFTEnvironment();
  else if (tftScreen == 3) drawTFTMotionCompass();
  else drawTFTSDKeypad();

  tftScreen++;
  if (tftScreen > 4) tftScreen = 0;
}

// =====================================================
// Serial Debug
// =====================================================

void printSerialSummary() {
  Serial.println();
  Serial.println("----- GeoTracker V1.0 Summary -----");

  Serial.print("GPS Fix: ");
  Serial.print(gpsFix ? "YES" : "NO");
  Serial.print(" | Sat: ");
  Serial.print(gpsSatellites);
  Serial.print(" | Lat/Lon: ");
  if (gpsFix) {
    Serial.print(gpsLat, 6);
    Serial.print(", ");
    Serial.print(gpsLon, 6);
  } else {
    Serial.print("NO FIX");
  }
  Serial.println();

  Serial.print("BME280: ");
  Serial.print(tempC, 1);
  Serial.print(" C, ");
  Serial.print(humidity, 1);
  Serial.print(" %, ");
  Serial.print(pressureHpa, 1);
  Serial.println(" hPa");

  Serial.print("MPU6050 Pitch/Roll: ");
  Serial.print(pitchDeg, 1);
  Serial.print(", ");
  Serial.println(rollDeg, 1);

  Serial.print("BMM150 Heading: ");
  Serial.print(headingDeg, 1);
  Serial.print(" deg ");
  Serial.println(cardinalDirection(headingDeg));

  Serial.print("SD Rows: ");
  Serial.print(logRow);
  Serial.print(" | Keypad raw: ");
  Serial.println(lastKeypadRaw);
}

// =====================================================
// Setup
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("GeoTracker V1.0 full integration skeleton starting...");

  beginDisplays();

  delay(500);

  scanMainI2C();

  beginBME280();
  beginMPU6050();
  beginBMM150();
  beginGPS();
  beginKeypad();
  beginSDCard();

  readAllSubsystems();

  drawLeftOLED();
  drawRightOLED();
  drawTFTStatus();

  Serial.println("GeoTracker V1.0 integration skeleton running.");
}

// =====================================================
// Loop
// =====================================================

void loop() {
  // GPS needs to be read constantly.
  readGPS();

  if (millis() - lastSensorRead >= 1000) {
    lastSensorRead = millis();

    readBME280();
    readMPU6050();
    readBMM150();
    readKeypadRaw();

    printSerialSummary();
  }

  if (millis() - lastOLEDUpdate >= 1000) {
    lastOLEDUpdate = millis();

    drawLeftOLED();
    drawRightOLED();
  }

  if (millis() - lastTFTUpdate >= 3000) {
    lastTFTUpdate = millis();

    updateTFTScreen();
  }

  if (millis() - lastSDLog >= 5000) {
    lastSDLog = millis();

    logToSD();
  }
}