/*
  GeoTracker V1.0
  Final Integration UI v2

  Major UI changes in v2:
  - OLED #1 always shows compass/navigation
  - OLED #2 always shows GPS status with satellite/orbit animation
  - TFT uses a natural Home + side-menu layout
  - Path Tracking is the main mission menu
  - Supports tracking logs, GPS waypoints, manual coordinate targets,
    waypoint-ID target loading, return-to-start, environment, movement,
    logging, and status views

  Confirmed hardware plan:

  TFT SPI:
    DIN/MOSI -> GPIO23
    CLK/SCK  -> GPIO18
    MISO     -> GPIO19 shared with SD
    CS       -> GPIO5
    DC       -> GPIO2
    RST      -> GPIO4
    BL       -> 3.3V

  SD SPI:
    VCC  -> 5V  // confirmed required by your SD module
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
      BMM150 at 0x13 with DFRobot_BMM150
      NULLLAB numberpad at 0x65

  Second I2C Bus:
    SDA -> GPIO32
    SCL -> GPIO33
    Device:
      OLED #2 at 0x3C

  GPS:
    GPS TX -> ESP32 RX2 GPIO16
    GPS RX -> ESP32 TX2 GPIO17
    Baud   -> 9600

  Keypad mapping:
    Normal/menu mode:
      A = next item / next screen
      B = previous item / back
      C = enter / select / start-stop tracking
      D = save current waypoint
      * = jump to Logging
      # = Home menu

    Coordinate input mode:
      0-9 = digits
      *   = decimal point
      #   = negative sign toggle
      A   = confirm field
      B   = cancel/back
      C   = confirm field / save
      D   = delete/backspace
*/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <math.h>

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

#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4
#define TFT_CLK   18
#define TFT_MOSI  23
#define TFT_MISO  19

#define SD_CS     13

#define MAIN_SDA  21
#define MAIN_SCL  22

#define OLED2_SDA 32
#define OLED2_SCL 33

#define OLED_ADDR   0x3C
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_RESET  -1

#define GPS_RX 16
#define GPS_TX 17
#define GPS_BAUD 9600

#define KEYPAD_ADDR       0x65
#define KEYPAD_STATUS_REG 0x08

#define SEA_LEVEL_HPA 1013.25

// =====================================================
// Timing
// =====================================================

#define SENSOR_INTERVAL_MS 1000
#define OLED_INTERVAL_MS   350
#define TFT_INTERVAL_MS    1000 //1000 original
#define LOG_INTERVAL_MS    5000
#define SERIAL_INTERVAL_MS 3000
#define KEYPAD_DEBOUNCE_MS 160

// =====================================================
// Objects
// =====================================================

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

TwoWire I2C_OLED2 = TwoWire(1);
Adafruit_SSD1306 oledCompass(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 oledGPS(OLED_WIDTH, OLED_HEIGHT, &I2C_OLED2, OLED_RESET);

Adafruit_BME280 bme;
Adafruit_MPU6050 mpu;

DFRobot_BMM150_I2C bmmAddr1(&Wire, I2C_ADDRESS_1);  // 0x10
DFRobot_BMM150_I2C bmmAddr2(&Wire, I2C_ADDRESS_2);  // 0x11
DFRobot_BMM150_I2C bmmAddr3(&Wire, I2C_ADDRESS_3);  // 0x12
DFRobot_BMM150_I2C bmmAddr4(&Wire, I2C_ADDRESS_4);  // 0x13
DFRobot_BMM150_I2C *bmm = NULL;

TinyGPSPlus gps;
TinyGPSCustom gsaFixType(gps, "GNGSA", 2);
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
bool gpsFix = false;
double gpsLat = 0.0;
double gpsLon = 0.0;
double gpsAltM = 0.0;
double gpsSpeedKmph = 0.0;
double gpsCourseDeg = 0.0;
int gpsSatellites = 0;
double gpsHDOP = 0.0;
unsigned long gpsCharsProcessed = 0;

// =====================================================
// Keypad State
// =====================================================

const char keyByBit[16] = {
  '1', '4', '7', '*',
  '2', '5', '8', '0',
  '3', '6', '9', '#',
  'A', 'B', 'C', 'D'
};

uint16_t keypadLastState = 0;
unsigned long lastKeyTime = 0;
char lastKeyPressed = '\0';

// =====================================================
// UI State
// =====================================================

enum PageId {
  PAGE_HOME = 0,
  PAGE_PATH_TRACKING,
  PAGE_ENVIRONMENT,
  PAGE_LOGGING,
  PAGE_MOVEMENT,
  PAGE_STATUS,
  PAGE_COUNT
};

const char *mainMenuItems[] = {
  "Path Tracking",
  "Environment",
  "Logging",
  "Movement",
  "Status"
};
const int MAIN_MENU_COUNT = 5;

PageId currentPage = PAGE_HOME;
int homeMenuIndex = 0;

const char *pageTitle(PageId page) {
  switch (page) {
    case PAGE_HOME:          return "HOME";
    case PAGE_PATH_TRACKING: return "PATH TRACKING";
    case PAGE_ENVIRONMENT:   return "ENVIRONMENT";
    case PAGE_LOGGING:       return "LOGGING";
    case PAGE_MOVEMENT:      return "MOVEMENT";
    case PAGE_STATUS:        return "STATUS";
    default:                 return "UNKNOWN";
  }
}

enum PathOption {
  PATH_LIVE_GPS = 0,
  PATH_START_STOP,
  PATH_NAV_COORDS,
  PATH_NAV_WAYPOINT,
  PATH_RETURN_START,
  PATH_OPTION_COUNT
};

const char *pathMenuItems[] = {
  "Live GPS",
  "Start Track",
  "Navigate To",
  "Nav Waypoint",
  "Return Start"
};

int pathMenuIndex = 0;

enum PathView {
  PATH_VIEW_MENU = 0,
  PATH_VIEW_LIVE_GPS,
  PATH_VIEW_TRACKING_CONTROL,
  PATH_VIEW_NAV_ACTIVE,
  PATH_VIEW_RETURN_START,
  PATH_VIEW_INPUT
};

PathView pathView = PATH_VIEW_MENU;

enum InputMode {
  INPUT_NONE = 0,
  INPUT_TARGET_LAT,
  INPUT_TARGET_LON,
  INPUT_MANUAL_WPT_LAT,
  INPUT_MANUAL_WPT_LON,
  INPUT_WAYPOINT_ID
};

InputMode inputMode = INPUT_NONE;
String inputBuffer = "";
String inputError = "";
double pendingLat = 0.0;
double pendingLon = 0.0;

// =====================================================
// Tracking / Navigation State
// =====================================================

bool trackingActive = false;
unsigned long trackingStartMillis = 0;

bool startPointValid = false;
double startLat = 0.0;
double startLon = 0.0;
double startAltM = 0.0;

bool lastTrackPointValid = false;
double lastTrackLat = 0.0;
double lastTrackLon = 0.0;
double lastTrackAltM = 0.0;

float totalDistanceM = 0.0;
float elevationGainM = 0.0;
float elevationLossM = 0.0;

bool targetActive = false;
double targetLat = 0.0;
double targetLon = 0.0;
double targetAltM = 0.0;
String targetName = "NONE";
String targetSource = "NONE";

unsigned long logRow = 0;
unsigned long waypointCount = 0;
unsigned long eventCount = 0;

// SD paths use short names for broad FAT compatibility.
const char *logFolder    = "/GEOTRK";
const char *logFile      = "/GEOTRK/LOG.CSV";
const char *waypointFile = "/GEOTRK/WAYPTS.CSV";
const char *eventFile    = "/GEOTRK/EVENTS.CSV";
const char *statusFile   = "/GEOTRK/STATUS.TXT";

// Status message
String statusMessage = "Booting...";
unsigned long statusMessageUntil = 0;

// Timing state
unsigned long lastSensorRead = 0;
unsigned long lastOLEDUpdate = 0;
unsigned long lastTFTUpdate = 0;
unsigned long lastSDLog = 0;
unsigned long lastSerialPrint = 0;

// =====================================================
// Utility
// =====================================================

float wrap360(float angle) {
  while (angle < 0.0) angle += 360.0;
  while (angle >= 360.0) angle -= 360.0;
  return angle;
}

float signedAngleDiff(float target, float current) {
  float diff = target - current;
  while (diff > 180.0) diff -= 360.0;
  while (diff < -180.0) diff += 360.0;
  return diff;
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

String activeStatusMessage() {
  if (millis() < statusMessageUntil) return statusMessage;
  if (trackingActive) return "Tracking active";
  if (targetActive) return "Navigation target active";
  return "Ready";
}

void setStatusMessage(const String &message, unsigned long durationMs = 3000) {
  statusMessage = message;
  statusMessageUntil = millis() + durationMs;
  Serial.print("[STATUS] ");
  Serial.println(statusMessage);
}

String gpsDateString() {
  if (!gps.date.isValid()) return "NA";

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", gps.date.year(), gps.date.month(), gps.date.day());
  return String(buffer);
}

String gpsTimeString() {
  if (!gps.time.isValid()) return "NA";

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
  return String(buffer);
}

String gpsFixString() {
  if (gsaFixType.isValid()) {
    int fix = atoi(gsaFixType.value());
    if (fix == 1) return "NO";
    if (fix == 2) return "2D";
    if (fix == 3) return "3D";
  }

  if (gps.location.isValid()) return "LOCK";
  return "NO";
}

String durationString(unsigned long durationMs) {
  unsigned long totalSeconds = durationMs / 1000;
  unsigned int hours = totalSeconds / 3600;
  unsigned int minutes = (totalSeconds % 3600) / 60;
  unsigned int seconds = totalSeconds % 60;

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", hours, minutes, seconds);
  return String(buffer);
}

unsigned long trackingDurationMs() {
  if (!trackingActive && trackingStartMillis == 0) return 0;
  if (trackingStartMillis == 0) return 0;
  return millis() - trackingStartMillis;
}

String csvSafe(String text) {
  text.replace(",", ";");
  text.replace("\n", " ");
  text.replace("\r", " ");
  return text;
}

void deselectSPI() {
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
}

bool i2cDevicePresent(TwoWire &bus, uint8_t address) {
  bus.beginTransmission(address);
  return bus.endTransmission() == 0;
}

// =====================================================
// GPS math
// =====================================================

const double EARTH_RADIUS_M = 6371000.0;

double degToRad(double deg) {
  return deg * PI / 180.0;
}

double radToDeg(double rad) {
  return rad * 180.0 / PI;
}

float distanceMeters(double lat1, double lon1, double lat2, double lon2) {
  double phi1 = degToRad(lat1);
  double phi2 = degToRad(lat2);
  double dPhi = degToRad(lat2 - lat1);
  double dLambda = degToRad(lon2 - lon1);

  double a = sin(dPhi / 2.0) * sin(dPhi / 2.0) +
             cos(phi1) * cos(phi2) * sin(dLambda / 2.0) * sin(dLambda / 2.0);
  double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));

  return (float)(EARTH_RADIUS_M * c);
}

float bearingToDeg(double lat1, double lon1, double lat2, double lon2) {
  double phi1 = degToRad(lat1);
  double phi2 = degToRad(lat2);
  double lambda1 = degToRad(lon1);
  double lambda2 = degToRad(lon2);

  double y = sin(lambda2 - lambda1) * cos(phi2);
  double x = cos(phi1) * sin(phi2) - sin(phi1) * cos(phi2) * cos(lambda2 - lambda1);

  return wrap360((float)radToDeg(atan2(y, x)));
}

float targetDistanceM() {
  if (!targetActive || !gpsFix) return -1.0;
  return distanceMeters(gpsLat, gpsLon, targetLat, targetLon);
}

float targetBearingDeg() {
  if (!targetActive || !gpsFix) return -1.0;
  return bearingToDeg(gpsLat, gpsLon, targetLat, targetLon);
}

String turnInstruction() {
  if (!targetActive || !gpsFix) return "NO TARGET";

  float bearing = targetBearingDeg();
  float diff = signedAngleDiff(bearing, headingDeg);

  if (abs(diff) < 10.0) return "AHEAD";
  if (diff > 0) return "RIGHT";
  return "LEFT";
}

// =====================================================
// I2C scan
// =====================================================

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
  Serial.println();
}

// =====================================================
// Initialization
// =====================================================

void beginDisplays() {
  Serial.println("Starting displays...");

  Wire.begin(MAIN_SDA, MAIN_SCL);
  Wire.setClock(100000);

  I2C_OLED2.begin(OLED2_SDA, OLED2_SCL, 400000);

  SPI.begin(TFT_CLK, TFT_MISO, TFT_MOSI);

  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  deselectSPI();

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  tftOK = true;

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(22, 78);
  tft.println("GeoTracker V1.0");
  tft.setCursor(22, 112);
  tft.println("Final UI v2");
  tft.setTextSize(1);
  tft.setCursor(22, 150);
  tft.println("Booting peripherals...");

  oled1OK = oledCompass.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false);
  oled2OK = oledGPS.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false);

  Serial.print("TFT: ");
  Serial.println(tftOK ? "OK" : "FAIL");
  Serial.print("OLED #1 compass: ");
  Serial.println(oled1OK ? "OK" : "FAIL");
  Serial.print("OLED #2 GPS: ");
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
  Serial.print(bmmOK ? "OK" : "FAIL");
  if (bmmOK) {
    Serial.print(" at 0x");
    Serial.print(bmmAddress, HEX);
  }
  Serial.println();
}

void beginGPS() {
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
  gpsSerialOK = true;
  Serial.println("GPS serial started.");
}

void beginKeypad() {
  keypadOK = i2cDevicePresent(Wire, KEYPAD_ADDR);

  Serial.print("Numberpad at 0x");
  Serial.print(KEYPAD_ADDR, HEX);
  Serial.print(": ");
  Serial.println(keypadOK ? "OK" : "NOT FOUND");
}

void createCSVFilesIfNeeded() {
  if (!sdOK) return;

  if (!SD.exists(logFolder)) SD.mkdir(logFolder);

  if (!SD.exists(logFile)) {
    File file = SD.open(logFile, FILE_WRITE);
    if (file) {
      file.println("row,millis,date_utc,time_utc,tracking,duration_s,total_distance_m,elevation_gain_m,elevation_loss_m,nav_active,target_name,target_source,target_distance_m,target_bearing_deg,turn,gps_fix,lat,lon,gps_alt_m,gps_speed_kmph,gps_course_deg,gps_sats,gps_hdop,temp_c,humidity_percent,pressure_hpa,pressure_alt_m,pitch_deg,roll_deg,accel_mag,gyro_x_dps,gyro_y_dps,gyro_z_dps,heading_deg,field_strength_ut,mag_x_ut,mag_y_ut,mag_z_ut,last_key,current_page");
      file.close();
      Serial.println("Created /GEOTRK/LOG.CSV");
    }
  }

  if (!SD.exists(waypointFile)) {
    File file = SD.open(waypointFile, FILE_WRITE);
    if (file) {
      file.println("id,millis,date_utc,time_utc,name,source,lat,lon,alt_m,heading_deg,temp_c,humidity_percent,pressure_hpa,note");
      file.close();
      Serial.println("Created /GEOTRK/WAYPTS.CSV");
    }
  }

  if (!SD.exists(eventFile)) {
    File file = SD.open(eventFile, FILE_WRITE);
    if (file) {
      file.println("id,millis,date_utc,time_utc,event,lat,lon,gps_fix,note");
      file.close();
      Serial.println("Created /GEOTRK/EVENTS.CSV");
    }
  }
}

void createStatusFile() {
  if (!sdOK) return;

  File file = SD.open(statusFile, FILE_WRITE);
  if (!file) return;

  file.println("GeoTracker V1.0 - Final Integration UI v2");
  file.println("Subsystem status at boot:");
  file.print("TFT: "); file.println(tftOK ? "OK" : "FAIL");
  file.print("OLED1 Compass: "); file.println(oled1OK ? "OK" : "FAIL");
  file.print("OLED2 GPS: "); file.println(oled2OK ? "OK" : "FAIL");
  file.print("BME280: "); file.println(bmeOK ? "OK" : "FAIL");
  file.print("MPU6050: "); file.println(mpuOK ? "OK" : "FAIL");
  file.print("BMM150: "); file.println(bmmOK ? "OK" : "FAIL");
  file.print("GPS Serial: "); file.println(gpsSerialOK ? "OK" : "FAIL");
  file.print("Keypad: "); file.println(keypadOK ? "OK" : "FAIL");
  file.print("SD: "); file.println(sdOK ? "OK" : "FAIL");
  file.println();
  file.println("SPI: TFT CS=GPIO5, SD CS=GPIO13, SCK=18, MISO=19, MOSI=23");
  file.println("I2C0: SDA=21, SCL=22");
  file.println("I2C1: SDA=32, SCL=33");
  file.println("GPS UART2: RX=16, TX=17, baud=9600");
  file.println();
  file.println("UI v2: OLED1 compass/navigation, OLED2 GPS status, TFT Home + side menu.");
  file.close();
}

void beginSDCard() {
  Serial.println("Starting SD card...");

  deselectSPI();

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

  createCSVFilesIfNeeded();
  createStatusFile();
}

// =====================================================
// Sensor Reading
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

  // Raw orientation; final case mounting should use offsets/calibration.
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

  // Use DFRobot's built-in compass heading as GeoTracker heading.
  headingDeg = wrap360(bmm->getCompassDegree());
}

void readGPS() {
  while (GPSSerial.available()) {
    gps.encode(GPSSerial.read());
  }

  gpsCharsProcessed = gps.charsProcessed();
  gpsFix = gps.location.isValid();

  if (gps.location.isValid()) {
    gpsLat = gps.location.lat();
    gpsLon = gps.location.lng();
  }

  if (gps.altitude.isValid()) gpsAltM = gps.altitude.meters();
  if (gps.speed.isValid()) gpsSpeedKmph = gps.speed.kmph();
  if (gps.course.isValid()) gpsCourseDeg = gps.course.deg();
  if (gps.satellites.isValid()) gpsSatellites = gps.satellites.value();
  if (gps.hdop.isValid()) gpsHDOP = gps.hdop.hdop();
}

void readAllSensors() {
  readGPS();
  readBME280();
  readMPU6050();
  readBMM150();
}

// =====================================================
// Keypad Reading
// =====================================================

bool readKeypadState(uint16_t &state) {
  if (!keypadOK) return false;

  Wire.beginTransmission(KEYPAD_ADDR);
  Wire.write(KEYPAD_STATUS_REG);

  if (Wire.endTransmission() != 0) return false;
  if (Wire.requestFrom(KEYPAD_ADDR, (uint8_t)2) != 2) return false;

  uint8_t lowByte = Wire.read();
  uint8_t highByte = Wire.read();

  state = ((uint16_t)highByte << 8) | lowByte;
  return true;
}

char getPressedKeyEvent() {
  uint16_t state = 0;

  if (!readKeypadState(state)) return '\0';

  char key = '\0';
  uint16_t newPresses = state & (~keypadLastState);

  if (newPresses != 0 && millis() - lastKeyTime > KEYPAD_DEBOUNCE_MS) {
    for (int bit = 0; bit < 16; bit++) {
      if (newPresses & (1U << bit)) {
        key = keyByBit[bit];
        lastKeyTime = millis();
        lastKeyPressed = key;
        break;
      }
    }
  }

  keypadLastState = state;
  return key;
}

// =====================================================
// SD Save Helpers
// =====================================================

void saveEvent(const char *eventName, const String &note) {
  if (!sdOK) return;

  deselectSPI();

  File file = SD.open(eventFile, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open EVENTS.CSV");
    return;
  }

  file.print(eventCount);
  file.print(",");
  file.print(millis());
  file.print(",");
  file.print(gpsDateString());
  file.print(",");
  file.print(gpsTimeString());
  file.print(",");
  file.print(eventName);
  file.print(",");

  if (gpsFix) {
    file.print(gpsLat, 6);
    file.print(",");
    file.print(gpsLon, 6);
  } else {
    file.print("NA,NA");
  }

  file.print(",");
  file.print(gpsFixString());
  file.print(",");
  file.println(csvSafe(note));

  file.close();
  eventCount++;
}

void saveWaypoint(const String &name, const String &source, double wptLat, double wptLon, double wptAlt, const String &note) {
  if (!sdOK) {
    setStatusMessage("No SD for waypoint", 3000);
    return;
  }

  deselectSPI();

  File file = SD.open(waypointFile, FILE_APPEND);
  if (!file) {
    setStatusMessage("Waypoint file fail", 3000);
    Serial.println("Failed to open WAYPTS.CSV");
    return;
  }

  file.print(waypointCount);
  file.print(",");
  file.print(millis());
  file.print(",");
  file.print(gpsDateString());
  file.print(",");
  file.print(gpsTimeString());
  file.print(",");
  file.print(csvSafe(name));
  file.print(",");
  file.print(csvSafe(source));
  file.print(",");
  file.print(wptLat, 6);
  file.print(",");
  file.print(wptLon, 6);
  file.print(",");
  file.print(wptAlt, 1);
  file.print(",");
  file.print(headingDeg, 2);
  file.print(",");
  file.print(tempC, 2);
  file.print(",");
  file.print(humidity, 2);
  file.print(",");
  file.print(pressureHpa, 2);
  file.print(",");
  file.println(csvSafe(note));

  file.close();

  waypointCount++;
  setStatusMessage("Waypoint saved", 3000);
  saveEvent("WAYPOINT", String("Saved waypoint ") + name);
}

String getCsvField(String line, int index) {
  int currentIndex = 0;
  int start = 0;

  for (int i = 0; i <= line.length(); i++) {
    if (i == line.length() || line.charAt(i) == ',') {
      if (currentIndex == index) return line.substring(start, i);
      currentIndex++;
      start = i + 1;
    }
  }

  return "";
}

bool loadWaypointById(long wantedId, double &outLat, double &outLon, double &outAlt, String &outName) {
  if (!sdOK) return false;

  deselectSPI();

  File file = SD.open(waypointFile);
  if (!file) return false;

  bool headerSkipped = false;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();

    if (line.length() == 0) continue;

    if (!headerSkipped) {
      headerSkipped = true;
      continue;
    }

    long id = getCsvField(line, 0).toInt();

    if (id == wantedId) {
      outName = getCsvField(line, 4);
      outLat = getCsvField(line, 6).toDouble();
      outLon = getCsvField(line, 7).toDouble();
      outAlt = getCsvField(line, 8).toDouble();
      file.close();
      return true;
    }
  }

  file.close();
  return false;
}

String fileSizeString(const char *path) {
  if (!sdOK) return "--";

  deselectSPI();

  File file = SD.open(path);
  if (!file) return "missing";

  uint32_t sizeBytes = file.size();
  file.close();

  if (sizeBytes >= 1024) {
    return String(sizeBytes / 1024.0, 1) + "KB";
  }

  return String(sizeBytes) + "B";
}

// =====================================================
// Tracking / Navigation Actions
// =====================================================

void resetTrackStats() {
  totalDistanceM = 0.0;
  elevationGainM = 0.0;
  elevationLossM = 0.0;
  startPointValid = false;
  lastTrackPointValid = false;
}

void captureStartPointIfPossible() {
  if (!gpsFix) return;

  startLat = gpsLat;
  startLon = gpsLon;
  startAltM = gpsAltM;
  startPointValid = true;

  lastTrackLat = gpsLat;
  lastTrackLon = gpsLon;
  lastTrackAltM = gpsAltM;
  lastTrackPointValid = true;

  saveWaypoint("START", "AUTO", startLat, startLon, startAltM, "Tracking start point");
}

void startTracking() {
  if (trackingActive) return;

  trackingActive = true;
  trackingStartMillis = millis();
  resetTrackStats();

  captureStartPointIfPossible();

  lastSDLog = 0;
  setStatusMessage("Tracking started", 3000);
  saveEvent("TRACK_START", "Tracking started from keypad");
}

void stopTracking() {
  if (!trackingActive) return;

  trackingActive = false;
  setStatusMessage("Tracking stopped", 3000);
  saveEvent("TRACK_STOP", "Tracking stopped from keypad");
}

void toggleTracking() {
  if (trackingActive) stopTracking();
  else startTracking();
}

void updateTrackStats() {
  if (!trackingActive || !gpsFix) return;

  if (!startPointValid) {
    captureStartPointIfPossible();
    return;
  }

  if (!lastTrackPointValid) {
    lastTrackLat = gpsLat;
    lastTrackLon = gpsLon;
    lastTrackAltM = gpsAltM;
    lastTrackPointValid = true;
    return;
  }

  float d = distanceMeters(lastTrackLat, lastTrackLon, gpsLat, gpsLon);

  // Ignore tiny jitter and impossible jumps.
  if (d >= 0.75 && d < 100.0) {
    totalDistanceM += d;

    float altDiff = gpsAltM - lastTrackAltM;

    if (altDiff > 0.5) elevationGainM += altDiff;
    if (altDiff < -0.5) elevationLossM += abs(altDiff);

    lastTrackLat = gpsLat;
    lastTrackLon = gpsLon;
    lastTrackAltM = gpsAltM;
  }
}

void setNavigationTarget(const String &name, const String &source, double lat, double lon, double alt) {
  targetActive = true;
  targetName = name;
  targetSource = source;
  targetLat = lat;
  targetLon = lon;
  targetAltM = alt;

  currentPage = PAGE_PATH_TRACKING;
  pathView = PATH_VIEW_NAV_ACTIVE;

  setStatusMessage(String("Target set: ") + name, 3000);
  saveEvent("NAV_TARGET", String("Target set: ") + name);
}

void clearNavigationTarget() {
  targetActive = false;
  targetName = "NONE";
  targetSource = "NONE";
  setStatusMessage("Target cleared", 2000);
  saveEvent("NAV_CLEAR", "Navigation target cleared");
}

void returnToStart() {
  if (!startPointValid) {
    setStatusMessage("No start point", 3000);
    saveEvent("RETURN_FAIL", "No start point available");
    return;
  }

  setNavigationTarget("START", "RETURN", startLat, startLon, startAltM);
  pathView = PATH_VIEW_RETURN_START;
}

void saveCurrentGPSWaypoint() {
  if (!gpsFix) {
    setStatusMessage("No GPS fix. Manual WPT.", 3000);
    inputMode = INPUT_MANUAL_WPT_LAT;
    inputBuffer = "";
    inputError = "";
    currentPage = PAGE_PATH_TRACKING;
    pathView = PATH_VIEW_INPUT;
    return;
  }

  String name = "WPT_";
  name += waypointCount;
  saveWaypoint(name, "GPS", gpsLat, gpsLon, gpsAltM, "Saved from current GPS fix");
}

// =====================================================
// SD Logging
// =====================================================

void logToSD() {
  if (!sdOK) return;

  updateTrackStats();

  deselectSPI();

  File file = SD.open(logFile, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open LOG.CSV for append.");
    setStatusMessage("LOG.CSV append fail", 3000);
    return;
  }

  float tDist = targetDistanceM();
  float tBearing = targetBearingDeg();

  file.print(logRow); file.print(",");
  file.print(millis()); file.print(",");
  file.print(gpsDateString()); file.print(",");
  file.print(gpsTimeString()); file.print(",");
  file.print(trackingActive ? 1 : 0); file.print(",");
  file.print(trackingDurationMs() / 1000); file.print(",");
  file.print(totalDistanceM, 2); file.print(",");
  file.print(elevationGainM, 2); file.print(",");
  file.print(elevationLossM, 2); file.print(",");
  file.print(targetActive ? 1 : 0); file.print(",");
  file.print(csvSafe(targetName)); file.print(",");
  file.print(csvSafe(targetSource)); file.print(",");
  if (tDist >= 0) file.print(tDist, 2); else file.print("NA");
  file.print(",");
  if (tBearing >= 0) file.print(tBearing, 2); else file.print("NA");
  file.print(",");
  file.print(turnInstruction()); file.print(",");
  file.print(gpsFixString()); file.print(",");

  if (gpsFix) {
    file.print(gpsLat, 6); file.print(",");
    file.print(gpsLon, 6);
  } else {
    file.print("NA,NA");
  }

  file.print(",");
  file.print(gpsAltM, 1); file.print(",");
  file.print(gpsSpeedKmph, 2); file.print(",");
  file.print(gpsCourseDeg, 2); file.print(",");
  file.print(gpsSatellites); file.print(",");
  file.print(gpsHDOP, 2); file.print(",");
  file.print(tempC, 2); file.print(",");
  file.print(humidity, 2); file.print(",");
  file.print(pressureHpa, 2); file.print(",");
  file.print(pressureAltM, 1); file.print(",");
  file.print(pitchDeg, 2); file.print(",");
  file.print(rollDeg, 2); file.print(",");
  file.print(accelMagnitude, 2); file.print(",");
  file.print(gyroX, 2); file.print(",");
  file.print(gyroY, 2); file.print(",");
  file.print(gyroZ, 2); file.print(",");
  file.print(headingDeg, 2); file.print(",");
  file.print(fieldStrength, 2); file.print(",");
  file.print(magX, 2); file.print(",");
  file.print(magY, 2); file.print(",");
  file.print(magZ, 2); file.print(",");
  if (lastKeyPressed == '\0') file.print("NONE");
  else file.print(lastKeyPressed);
  file.print(",");
  file.println(pageTitle(currentPage));

  file.close();

  Serial.print("Logged row ");
  Serial.println(logRow);
  logRow++;
}

// =====================================================
// Input Handling
// =====================================================

bool validLat(double value) { return value >= -90.0 && value <= 90.0; }
bool validLon(double value) { return value >= -180.0 && value <= 180.0; }

void startCoordinateTargetInput() {
  inputMode = INPUT_TARGET_LAT;
  inputBuffer = "";
  inputError = "";
  currentPage = PAGE_PATH_TRACKING;
  pathView = PATH_VIEW_INPUT;
  setStatusMessage("Enter target lat", 2500);
}

void startWaypointIdInput() {
  inputMode = INPUT_WAYPOINT_ID;
  inputBuffer = "";
  inputError = "";
  currentPage = PAGE_PATH_TRACKING;
  pathView = PATH_VIEW_INPUT;
  setStatusMessage("Enter waypoint ID", 2500);
}

void cancelInput() {
  inputMode = INPUT_NONE;
  inputBuffer = "";
  inputError = "";
  pathView = PATH_VIEW_MENU;
  setStatusMessage("Input cancelled", 2000);
}

void appendInputChar(char c) {
  if (inputBuffer.length() >= 14) {
    inputError = "Input too long";
    setStatusMessage(inputError, 2000);
    return;
  }

  if (c == '.') {
    if (inputMode == INPUT_WAYPOINT_ID) return;

    if (inputBuffer.indexOf('.') >= 0) {
      inputError = "Decimal used";
      setStatusMessage(inputError, 2000);
      return;
    }

    if (inputBuffer.length() == 0 || inputBuffer == "-") inputBuffer += "0";
  }

  inputBuffer += c;
  inputError = "";
}

void toggleNegativeSign() {
  if (inputMode == INPUT_WAYPOINT_ID) return;

  if (inputBuffer.startsWith("-")) inputBuffer.remove(0, 1);
  else inputBuffer = "-" + inputBuffer;
}

void deleteInputChar() {
  if (inputBuffer.length() > 0) inputBuffer.remove(inputBuffer.length() - 1);
}

void finishLatField() {
  if (inputBuffer.length() == 0) {
    inputError = "Latitude empty";
    setStatusMessage(inputError, 2000);
    return;
  }

  double value = inputBuffer.toDouble();
  if (!validLat(value)) {
    inputError = "Latitude range";
    setStatusMessage(inputError, 2000);
    return;
  }

  pendingLat = value;
  inputBuffer = "";
  inputError = "";

  if (inputMode == INPUT_TARGET_LAT) {
    inputMode = INPUT_TARGET_LON;
    setStatusMessage("Enter target lon", 2500);
  } else if (inputMode == INPUT_MANUAL_WPT_LAT) {
    inputMode = INPUT_MANUAL_WPT_LON;
    setStatusMessage("Enter waypoint lon", 2500);
  }
}

void finishLonField() {
  if (inputBuffer.length() == 0) {
    inputError = "Longitude empty";
    setStatusMessage(inputError, 2000);
    return;
  }

  double value = inputBuffer.toDouble();
  if (!validLon(value)) {
    inputError = "Longitude range";
    setStatusMessage(inputError, 2000);
    return;
  }

  pendingLon = value;
  inputBuffer = "";
  inputError = "";

  if (inputMode == INPUT_TARGET_LON) {
    inputMode = INPUT_NONE;
    setNavigationTarget("COORD", "MANUAL", pendingLat, pendingLon, 0.0);
  } else if (inputMode == INPUT_MANUAL_WPT_LON) {
    inputMode = INPUT_NONE;
    String name = "MANUAL_";
    name += waypointCount;
    saveWaypoint(name, "MANUAL", pendingLat, pendingLon, 0.0, "Manual coordinate waypoint");
    pathView = PATH_VIEW_MENU;
  }
}

void finishWaypointIdInput() {
  if (inputBuffer.length() == 0) {
    inputError = "ID empty";
    setStatusMessage(inputError, 2000);
    return;
  }

  long id = inputBuffer.toInt();
  double wLat = 0.0;
  double wLon = 0.0;
  double wAlt = 0.0;
  String wName = "";

  if (loadWaypointById(id, wLat, wLon, wAlt, wName)) {
    inputMode = INPUT_NONE;
    inputBuffer = "";
    setNavigationTarget(String("WPT_") + id + " " + wName, "WAYPOINT", wLat, wLon, wAlt);
  } else {
    inputError = "ID not found";
    setStatusMessage(inputError, 2500);
  }
}

void confirmInput() {
  if (inputMode == INPUT_TARGET_LAT || inputMode == INPUT_MANUAL_WPT_LAT) finishLatField();
  else if (inputMode == INPUT_TARGET_LON || inputMode == INPUT_MANUAL_WPT_LON) finishLonField();
  else if (inputMode == INPUT_WAYPOINT_ID) finishWaypointIdInput();
}

void handleInputKey(char key) {
  if (key >= '0' && key <= '9') appendInputChar(key);
  else if (key == '*') appendInputChar('.');
  else if (key == '#') toggleNegativeSign();
  else if (key == 'D') deleteInputChar();
  else if (key == 'B') cancelInput();
  else if (key == 'A' || key == 'C') confirmInput();
}

// =====================================================
// Menu Actions
// =====================================================

void enterHomeMenuSelection() {
  switch (homeMenuIndex) {
    case 0:
      currentPage = PAGE_PATH_TRACKING;
      pathView = PATH_VIEW_MENU;
      break;
    case 1:
      currentPage = PAGE_ENVIRONMENT;
      break;
    case 2:
      currentPage = PAGE_LOGGING;
      break;
    case 3:
      currentPage = PAGE_MOVEMENT;
      break;
    case 4:
      currentPage = PAGE_STATUS;
      break;
  }
  setStatusMessage(String("Open: ") + mainMenuItems[homeMenuIndex], 1500);
}

void selectPathOption() {
  switch (pathMenuIndex) {
    case PATH_LIVE_GPS:
      pathView = PATH_VIEW_LIVE_GPS;
      setStatusMessage("Live GPS", 1500);
      break;
    case PATH_START_STOP:
      toggleTracking();
      pathView = PATH_VIEW_TRACKING_CONTROL;
      break;
    case PATH_NAV_COORDS:
      startCoordinateTargetInput();
      break;
    case PATH_NAV_WAYPOINT:
      startWaypointIdInput();
      break;
    case PATH_RETURN_START:
      returnToStart();
      break;
  }
}

void nextPathView() {
  if (pathView == PATH_VIEW_MENU) {
    pathMenuIndex++;
    if (pathMenuIndex >= PATH_OPTION_COUNT) pathMenuIndex = 0;
  } else if (pathView == PATH_VIEW_LIVE_GPS) {
    pathView = PATH_VIEW_TRACKING_CONTROL;
  } else if (pathView == PATH_VIEW_TRACKING_CONTROL) {
    pathView = PATH_VIEW_NAV_ACTIVE;
  } else if (pathView == PATH_VIEW_NAV_ACTIVE) {
    pathView = PATH_VIEW_RETURN_START;
  } else {
    pathView = PATH_VIEW_MENU;
  }
}

void previousOrBackPath() {
  if (pathView == PATH_VIEW_MENU) {
    currentPage = PAGE_HOME;
    setStatusMessage("Back home", 1500);
  } else {
    pathView = PATH_VIEW_MENU;
    setStatusMessage("Path menu", 1500);
  }
}

void nextPageQuick() {
  int p = (int)currentPage + 1;
  if (p >= PAGE_COUNT) p = 0;
  currentPage = (PageId)p;
  setStatusMessage(String("Page: ") + pageTitle(currentPage), 1200);
}

void handleNormalKey(char key) {
  if (key == 'D') {
    saveCurrentGPSWaypoint();
    return;
  }

  if (key == '*') {
    currentPage = PAGE_LOGGING;
    inputMode = INPUT_NONE;
    setStatusMessage("Logging", 1500);
    return;
  }

  if (key == '#') {
    currentPage = PAGE_HOME;
    pathView = PATH_VIEW_MENU;
    inputMode = INPUT_NONE;
    setStatusMessage("Home", 1500);
    return;
  }

  if (currentPage == PAGE_HOME) {
    if (key == 'A') {
      homeMenuIndex++;
      if (homeMenuIndex >= MAIN_MENU_COUNT) homeMenuIndex = 0;
    } else if (key == 'B') {
      homeMenuIndex--;
      if (homeMenuIndex < 0) homeMenuIndex = MAIN_MENU_COUNT - 1;
    } else if (key == 'C') {
      enterHomeMenuSelection();
    }
    return;
  }

  if (currentPage == PAGE_PATH_TRACKING) {
    if (key == 'A') nextPathView();
    else if (key == 'B') previousOrBackPath();
    else if (key == 'C') {
      if (pathView == PATH_VIEW_MENU) selectPathOption();
      else if (pathView == PATH_VIEW_LIVE_GPS || pathView == PATH_VIEW_TRACKING_CONTROL) toggleTracking();
      else if (pathView == PATH_VIEW_NAV_ACTIVE || pathView == PATH_VIEW_RETURN_START) clearNavigationTarget();
    }
    return;
  }

  // Other top-level pages.
  if (key == 'A') {
    nextPageQuick();
  } else if (key == 'B') {
    currentPage = PAGE_HOME;
    setStatusMessage("Back home", 1500);
  } else if (key == 'C') {
    if (currentPage == PAGE_LOGGING) toggleTracking();
    else saveEvent("SELECT", String("Selected page ") + pageTitle(currentPage));
  }
}

void handleKeypad() {
  char key = getPressedKeyEvent();
  if (key == '\0') return;

  Serial.print("Key pressed: ");
  Serial.println(key);

  if (inputMode != INPUT_NONE) handleInputKey(key);
  else handleNormalKey(key);

  lastTFTUpdate = 0;
  lastOLEDUpdate = 0;
}

// =====================================================
// OLED #1: Always Compass / Navigation
// =====================================================

void drawOLEDCompass() {
  if (!oled1OK) return;

  oledCompass.clearDisplay();
  oledCompass.setTextColor(SSD1306_WHITE);
  oledCompass.setTextSize(1);

  oledCompass.setCursor(0, 0);
  oledCompass.print("NAV");

  if (targetActive) {
    oledCompass.setCursor(70, 0);
    oledCompass.print("TARGET");
  } else {
    oledCompass.setCursor(78, 0);
    oledCompass.print("COMPASS");
  }

  int cx = 64;
  int cy = 31;
  int r = 22;

  oledCompass.drawCircle(cx, cy, r, SSD1306_WHITE);
  oledCompass.drawCircle(cx, cy, 2, SSD1306_WHITE);

  oledCompass.setCursor(cx - 3, cy - r - 8);
  oledCompass.print("N");
  oledCompass.setCursor(cx - 3, cy + r + 1);
  oledCompass.print("S");
  oledCompass.setCursor(cx + r + 3, cy - 3);
  oledCompass.print("E");
  oledCompass.setCursor(cx - r - 8, cy - 3);
  oledCompass.print("W");

  float angle = headingDeg * PI / 180.0;
  int x2 = cx + sin(angle) * 17;
  int y2 = cy - cos(angle) * 17;
  oledCompass.drawLine(cx, cy, x2, y2, SSD1306_WHITE);
  oledCompass.fillCircle(x2, y2, 2, SSD1306_WHITE);

  // When navigation target is active, draw a shorter target bearing pointer.
  if (targetActive && gpsFix) {
    float b = targetBearingDeg();
    float ba = b * PI / 180.0;
    int tx = cx + sin(ba) * 11;
    int ty = cy - cos(ba) * 11;
    oledCompass.drawCircle(tx, ty, 3, SSD1306_WHITE);
  }

  oledCompass.drawLine(0, 54, 127, 54, SSD1306_WHITE);
  oledCompass.setCursor(0, 56);

  if (targetActive && gpsFix) {
    float d = targetDistanceM();
    float b = targetBearingDeg();
    float turn = signedAngleDiff(b, headingDeg);

    if (d < 1000.0) {
      oledCompass.print((int)d);
      oledCompass.print("m ");
    } else {
      oledCompass.print(d / 1000.0, 1);
      oledCompass.print("km ");
    }

    if (turn > 10) oledCompass.print("R");
    else if (turn < -10) oledCompass.print("L");
    else oledCompass.print("A");

    oledCompass.print(" ");
    oledCompass.print(abs(turn), 0);
    oledCompass.print("deg");
  } else {
    oledCompass.print(headingDeg, 0);
    oledCompass.print("deg ");
    oledCompass.print(cardinalDirection(headingDeg));
  }

  oledCompass.display();
}

// =====================================================
// OLED #2: Always GPS Status / Satellite Animation
// =====================================================

void drawOLEDGPS() {
  if (!oled2OK) return;

  oledGPS.clearDisplay();
  oledGPS.setTextColor(SSD1306_WHITE);
  oledGPS.setTextSize(1);

  oledGPS.setCursor(0, 0);
  oledGPS.print("GPS ");
  oledGPS.print(gpsFix ? "LOCK" : "SEARCH");

  oledGPS.setCursor(0, 12);
  oledGPS.print("SAT:");
  oledGPS.print(gpsSatellites);
  oledGPS.print(" HDOP:");
  if (gpsHDOP > 0.0) oledGPS.print(gpsHDOP, 1);
  else oledGPS.print("--");

  oledGPS.setCursor(0, 24);
  oledGPS.print("FIX:");
  oledGPS.print(gpsFixString());

  oledGPS.setCursor(0, 36);
  oledGPS.print("UTC:");
  oledGPS.print(gpsTimeString());

  // Earth + orbit animation.
  int ex = 99;
  int ey = 42;
  int er = 10;
  int orbitR = 18;

  oledGPS.drawCircle(ex, ey, er, SSD1306_WHITE);
  oledGPS.drawLine(ex - 6, ey, ex + 6, ey, SSD1306_WHITE);
  oledGPS.drawLine(ex, ey - 6, ex, ey + 6, SSD1306_WHITE);
  oledGPS.drawCircle(ex, ey, orbitR, SSD1306_WHITE);

  float orbitAngle = (millis() % 2400) * 2.0 * PI / 2400.0;
  int sx = ex + cos(orbitAngle) * orbitR;
  int sy = ey + sin(orbitAngle) * orbitR;

  if (gpsFix || ((millis() / 300) % 2 == 0)) {
    oledGPS.fillCircle(sx, sy, 2, SSD1306_WHITE);
    oledGPS.drawLine(sx - 3, sy, sx + 3, sy, SSD1306_WHITE);
  }

  oledGPS.display();
}

// =====================================================
// TFT Drawing Helpers
// =====================================================

void drawHeader(const char *title) {
  deselectSPI();
  tft.fillRect(0, 0, 320, 28, ILI9341_NAVY);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(7, 6);
  tft.print(title);

  tft.setTextSize(1);
  tft.setCursor(236, 6);
  tft.print(gpsTimeString());
  tft.setCursor(236, 17);
  tft.print(gpsDateString());
}

void drawFooter(const char *controls) {
  tft.fillRect(0, 218, 320, 22, ILI9341_BLACK);
  tft.drawLine(0, 217, 319, 217, ILI9341_DARKGREY);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(6, 222);
  tft.print(activeStatusMessage());

  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(168, 222);
  tft.print(controls);
}

void drawMenuPanel(const char **items, int count, int selected, int x, int y, int w, int h) {
  tft.drawRect(x, y, w, h, ILI9341_DARKGREY);

  int itemH = h / count;

  for (int i = 0; i < count; i++) {
    int iy = y + i * itemH;

    if (i == selected) {
      tft.fillRect(x + 2, iy + 2, w - 4, itemH - 4, ILI9341_DARKCYAN);
      tft.setTextColor(ILI9341_WHITE);
      tft.setCursor(x + 6, iy + 7);
      tft.print(">");
      tft.setCursor(x + 17, iy + 7);
      tft.print(items[i]);
    } else {
      tft.setTextColor(ILI9341_LIGHTGREY);
      tft.setCursor(x + 17, iy + 7);
      tft.print(items[i]);
    }
  }
}

void drawSmallStatusStrip(int y) {
  tft.setTextSize(1);
  tft.setTextColor(gpsFix ? ILI9341_GREEN : ILI9341_RED);
  tft.setCursor(124, y);
  tft.print("GPS:");
  tft.print(gpsFixString());

  tft.setTextColor(sdOK ? ILI9341_GREEN : ILI9341_RED);
  tft.setCursor(184, y);
  tft.print("SD:");
  tft.print(sdOK ? "OK" : "NO");

  tft.setTextColor(trackingActive ? ILI9341_GREEN : ILI9341_YELLOW);
  tft.setCursor(232, y);
  tft.print("TRK:");
  tft.print(trackingActive ? "ON" : "OFF");
}

// =====================================================
// TFT Pages
// =====================================================

void drawHomeScreen() {
  tft.fillScreen(ILI9341_BLACK);
  drawHeader("GEOTRACKER V1.0");

  drawMenuPanel(mainMenuItems, MAIN_MENU_COUNT, homeMenuIndex, 4, 36, 112, 174);

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(126, 42);
  tft.print("Home");

  drawSmallStatusStrip(66);

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(126, 88);
  tft.print("Fix: ");
  tft.setTextColor(gpsFix ? ILI9341_GREEN : ILI9341_RED);
  tft.print(gpsFixString());
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.print("  Sat: ");
  tft.print(gpsSatellites);

  tft.setCursor(126, 105);
  tft.print("Lat: ");
  if (gpsFix) tft.print(gpsLat, 6);
  else tft.print("--");

  tft.setCursor(126, 122);
  tft.print("Lon: ");
  if (gpsFix) tft.print(gpsLon, 6);
  else tft.print("--");

  tft.setCursor(126, 143);
  tft.print("Heading: ");
  tft.setTextColor(ILI9341_YELLOW);
  tft.print(headingDeg, 0);
  tft.print("deg ");
  tft.print(cardinalDirection(headingDeg));

  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(126, 160);
  tft.print("Temp: ");
  if (bmeOK) {
    tft.print(tempC, 1);
    tft.print(" C");
  } else {
    tft.print("--");
  }

  tft.setCursor(126, 177);
  tft.print("Track duration: ");
  tft.print(durationString(trackingDurationMs()));

  tft.setCursor(126, 194);
  tft.print("Dist: ");
  tft.print(totalDistanceM, 1);
  tft.print(" m");

  drawFooter("A/B menu C enter");
}

void drawPathInputScreen() {
  drawHeader("PATH INPUT");

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 44);

  if (inputMode == INPUT_TARGET_LAT) tft.print("Target Latitude");
  else if (inputMode == INPUT_TARGET_LON) tft.print("Target Longitude");
  else if (inputMode == INPUT_MANUAL_WPT_LAT) tft.print("Waypoint Latitude");
  else if (inputMode == INPUT_MANUAL_WPT_LON) tft.print("Waypoint Longitude");
  else if (inputMode == INPUT_WAYPOINT_ID) tft.print("Waypoint ID");

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(10, 76);

  if (inputMode == INPUT_TARGET_LAT || inputMode == INPUT_TARGET_LON) {
    tft.print("Manual navigation target coordinate.");
  } else if (inputMode == INPUT_MANUAL_WPT_LAT || inputMode == INPUT_MANUAL_WPT_LON) {
    tft.print("Manual waypoint save because GPS fix is unavailable.");
  } else {
    tft.print("Load saved waypoint from WAYPTS.CSV by ID.");
  }

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 112);
  tft.print("Value:");

  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(10, 145);
  if (inputBuffer.length() == 0) tft.print("_");
  else tft.print(inputBuffer);

  if (inputMode == INPUT_TARGET_LON || inputMode == INPUT_MANUAL_WPT_LON) {
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_CYAN);
    tft.setCursor(10, 176);
    tft.print("Latitude locked: ");
    tft.print(pendingLat, 6);
  }

  if (inputError.length() > 0) {
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(10, 194);
    tft.print(inputError);
  }

  drawFooter("0-9 *=. #=- D del");
}

void drawPathMenu() {
  tft.fillScreen(ILI9341_BLACK);
  drawHeader("PATH TRACKING");

  drawMenuPanel(pathMenuItems, PATH_OPTION_COUNT, pathMenuIndex, 4, 36, 112, 174);

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(126, 42);
  tft.print(pathMenuItems[pathMenuIndex]);

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(126, 72);

  if (pathMenuIndex == PATH_LIVE_GPS) {
    tft.println("View live coordinates, speed,");
    tft.setCursor(126, 86); tft.println("altitude, course, and GPS quality.");
  } else if (pathMenuIndex == PATH_START_STOP) {
    tft.println("Start or stop full CSV tracking.");
    tft.setCursor(126, 86); tft.println("Logs all relevant sensor values.");
  } else if (pathMenuIndex == PATH_NAV_COORDS) {
    tft.println("Input latitude and longitude");
    tft.setCursor(126, 86); tft.println("for direct navigation.");
  } else if (pathMenuIndex == PATH_NAV_WAYPOINT) {
    tft.println("Input a saved waypoint ID");
    tft.setCursor(126, 86); tft.println("from WAYPTS.CSV.");
  } else if (pathMenuIndex == PATH_RETURN_START) {
    tft.println("Navigate back to the GPS point");
    tft.setCursor(126, 86); tft.println("where tracking started.");
  }

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(126, 122);
  tft.print("Tracking: ");
  tft.setTextColor(trackingActive ? ILI9341_GREEN : ILI9341_YELLOW);
  tft.print(trackingActive ? "ON" : "OFF");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(126, 140);
  tft.print("Target: ");
  tft.setTextColor(targetActive ? ILI9341_GREEN : ILI9341_LIGHTGREY);
  tft.print(targetActive ? targetName : "NONE");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(126, 158);
  tft.print("Distance: ");
  tft.print(totalDistanceM, 1);
  tft.print(" m");

  tft.setCursor(126, 176);
  tft.print("Duration: ");
  tft.print(durationString(trackingDurationMs()));

  drawFooter("A/B item C select");
}

void drawPathLiveGPS() {
  tft.fillScreen(ILI9341_BLACK);
  drawHeader("LIVE GPS");

  tft.setTextSize(2);
  tft.setTextColor(gpsFix ? ILI9341_GREEN : ILI9341_RED);
  tft.setCursor(10, 42);
  tft.print("Fix: ");
  tft.print(gpsFixString());

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(190, 42);
  tft.print("Sat:");
  tft.print(gpsSatellites);

  tft.setCursor(10, 78);
  tft.print("Lat:");
  tft.setCursor(80, 78);
  if (gpsFix) tft.print(gpsLat, 6); else tft.print("--");

  tft.setCursor(10, 112);
  tft.print("Lon:");
  tft.setCursor(80, 112);
  if (gpsFix) tft.print(gpsLon, 6); else tft.print("--");

  tft.setCursor(10, 146);
  tft.print("Alt:");
  tft.setCursor(80, 146);
  tft.print(gpsAltM, 1);
  tft.print(" m");

  tft.setCursor(10, 180);
  tft.print("Spd:");
  tft.setCursor(80, 180);
  tft.print(gpsSpeedKmph, 1);
  tft.print(" kmh");

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(210, 180);
  tft.print("HDOP ");
  if (gpsHDOP > 0.0) tft.print(gpsHDOP, 1); else tft.print("--");

  drawFooter("B back C track");
}

void drawPathTrackingControl() {
  tft.fillScreen(ILI9341_BLACK);
  drawHeader("TRACKING");

  tft.setTextSize(2);
  tft.setTextColor(trackingActive ? ILI9341_GREEN : ILI9341_YELLOW);
  tft.setCursor(10, 45);
  tft.print(trackingActive ? "TRACKING ON" : "TRACKING OFF");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 85);
  tft.print("Rows:");
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(135, 85);
  tft.print(logRow);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 120);
  tft.print("Time:");
  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(135, 120);
  tft.print(durationString(trackingDurationMs()));

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 155);
  tft.print("Dist:");
  tft.setTextColor(ILI9341_MAGENTA);
  tft.setCursor(135, 155);
  tft.print(totalDistanceM, 1);
  tft.print("m");

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(10, 195);
  tft.print("Logs GPS, BME280, MPU6050, BMM150, keypad, UI state.");

  drawFooter("C start/stop B back");
}

void drawPathNavActive() {
  tft.fillScreen(ILI9341_BLACK);
  drawHeader("NAVIGATION");

  if (!targetActive) {
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(10, 65);
    tft.print("No target set");
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(10, 105);
    tft.print("Use Navigate To or Nav Waypoint.");
    drawFooter("B back C clear");
    return;
  }

  float d = targetDistanceM();
  float b = targetBearingDeg();
  float turn = (b >= 0) ? signedAngleDiff(b, headingDeg) : 0.0;

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 42);
  tft.print("Target:");
  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(120, 42);
  tft.print(targetName.substring(0, 14));

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 78);
  tft.print("Dist:");
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(120, 78);
  if (d >= 0) {
    if (d < 1000) {
      tft.print(d, 1); tft.print("m");
    } else {
      tft.print(d / 1000.0, 2); tft.print("km");
    }
  } else {
    tft.print("NO GPS");
  }

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 114);
  tft.print("Bearing:");
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(120, 114);
  if (b >= 0) tft.print(b, 1); else tft.print("--");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 150);
  tft.print("Turn:");
  tft.setTextColor(abs(turn) < 10 ? ILI9341_GREEN : ILI9341_MAGENTA);
  tft.setCursor(120, 150);
  tft.print(turnInstruction());
  tft.print(" ");
  tft.print(abs(turn), 0);

  int cx = 265;
  int cy = 125;
  int r = 48;
  tft.drawCircle(cx, cy, r, ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(cx - 3, cy - r - 11); tft.print("N");

  float ha = headingDeg * PI / 180.0;
  int hx = cx + sin(ha) * 38;
  int hy = cy - cos(ha) * 38;
  tft.drawLine(cx, cy, hx, hy, ILI9341_RED);
  tft.fillCircle(hx, hy, 3, ILI9341_RED);

  if (b >= 0) {
    float ba = b * PI / 180.0;
    int bx = cx + sin(ba) * 28;
    int by = cy - cos(ba) * 28;
    tft.drawCircle(bx, by, 5, ILI9341_GREEN);
  }

  drawFooter("B back C clear");
}

void drawPathReturnStart() {
  // Same navigation display, but with clearer context.
  drawPathNavActive();
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_ORANGE);
  tft.setCursor(10, 195);
  tft.print("Return-to-start mode");
}

void drawPathTrackingScreen() {
  if (inputMode != INPUT_NONE) {
    tft.fillScreen(ILI9341_BLACK);
    drawPathInputScreen();
    return;
  }

  if (pathView == PATH_VIEW_MENU) drawPathMenu();
  else if (pathView == PATH_VIEW_LIVE_GPS) drawPathLiveGPS();
  else if (pathView == PATH_VIEW_TRACKING_CONTROL) drawPathTrackingControl();
  else if (pathView == PATH_VIEW_NAV_ACTIVE) drawPathNavActive();
  else if (pathView == PATH_VIEW_RETURN_START) drawPathReturnStart();
  else drawPathMenu();
}

void drawEnvironmentScreen() {
  tft.fillScreen(ILI9341_BLACK);
  drawHeader("ENVIRONMENT");

  if (!bmeOK) {
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(20, 90);
    tft.print("BME280 FAIL");
    drawFooter("B back # home");
    return;
  }

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);

  tft.setCursor(10, 48);
  tft.print("Temp:");
  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(135, 48);
  tft.print(tempC, 1);
  tft.print(" C");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 86);
  tft.print("Humidity:");
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(135, 86);
  tft.print(humidity, 1);
  tft.print(" %");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 124);
  tft.print("Pressure:");
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(135, 124);
  tft.print(pressureHpa, 1);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 162);
  tft.print("P Alt:");
  tft.setTextColor(ILI9341_MAGENTA);
  tft.setCursor(135, 162);
  tft.print(pressureAltM, 0);
  tft.print(" m");

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(10, 198);
  tft.print("BME280 address: 0x");
  tft.print(bmeAddress, HEX);

  drawFooter("A next B back");
}

void drawLoggingScreen() {
  String logSize = fileSizeString(logFile);
  String wptSize = fileSizeString(waypointFile);
  String evtSize = fileSizeString(eventFile);

  tft.fillScreen(ILI9341_BLACK);
  drawHeader("LOGGING");

  tft.setTextSize(1);
  tft.setTextColor(sdOK ? ILI9341_GREEN : ILI9341_RED);
  tft.setCursor(10, 38);
  tft.print("SD: ");
  tft.print(sdOK ? "OK" : "FAIL");

  tft.setTextColor(trackingActive ? ILI9341_GREEN : ILI9341_YELLOW);
  tft.setCursor(75, 38);
  tft.print("Tracking: ");
  tft.print(trackingActive ? "ON" : "OFF");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 58); tft.print("LOG.CSV     "); tft.print(logSize);
  tft.setCursor(10, 74); tft.print("WAYPTS.CSV  "); tft.print(wptSize);
  tft.setCursor(10, 90); tft.print("EVENTS.CSV  "); tft.print(evtSize);

  tft.drawLine(10, 112, 310, 112, ILI9341_DARKGREY);

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 122);
  tft.print("Rows:");
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(140, 122);
  tft.print(logRow);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 152);
  tft.print("Dist:");
  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(140, 152);
  tft.print(totalDistanceM, 1);
  tft.print("m");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 182);
  tft.print("Elev:");
  tft.setTextColor(ILI9341_MAGENTA);
  tft.setCursor(140, 182);
  tft.print("+");
  tft.print(elevationGainM, 0);
  tft.print("/-");
  tft.print(elevationLossM, 0);

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(230, 182);
  tft.print(durationString(trackingDurationMs()));

  drawFooter("C track B back");
}

void drawMovementScreen() {
  tft.fillScreen(ILI9341_BLACK);
  drawHeader("MOVEMENT");

  tft.setTextSize(2);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 42);
  tft.print("Pitch:");
  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(135, 42);
  tft.print(pitchDeg, 1);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 76);
  tft.print("Roll:");
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(135, 76);
  tft.print(rollDeg, 1);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 110);
  tft.print("Accel:");
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(135, 110);
  tft.print(accelMagnitude, 2);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 144);
  tft.print("Head:");
  tft.setTextColor(ILI9341_MAGENTA);
  tft.setCursor(135, 144);
  tft.print(headingDeg, 0);
  tft.print(" ");
  tft.print(cardinalDirection(headingDeg));

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(10, 180);
  tft.print("Field: ");
  tft.print(fieldStrength, 1);
  tft.print(" uT   GyroZ: ");
  tft.print(gyroZ, 1);
  tft.print(" deg/s");

  int cx = 260;
  int cy = 122;
  int r = 45;
  tft.drawCircle(cx, cy, r, ILI9341_WHITE);
  tft.setCursor(cx - 3, cy - r - 12); tft.print("N");
  float a = headingDeg * PI / 180.0;
  int x2 = cx + sin(a) * 35;
  int y2 = cy - cos(a) * 35;
  tft.drawLine(cx, cy, x2, y2, ILI9341_RED);
  tft.fillCircle(x2, y2, 4, ILI9341_RED);

  drawFooter("A next B back");
}

void drawStatusScreen() {
  tft.fillScreen(ILI9341_BLACK);
  drawHeader("DEVICE STATUS");

  tft.setTextSize(1);

  int x1 = 10;
  int x2 = 165;
  int y = 42;

  auto drawStatusItem = [&](int x, int yPos, const char *label, bool ok) {
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(x, yPos);
    tft.print(label);
    tft.setTextColor(ok ? ILI9341_GREEN : ILI9341_RED);
    tft.setCursor(x + 82, yPos);
    tft.print(ok ? "OK" : "FAIL");
  };

  drawStatusItem(x1, y,      "TFT", tftOK);
  drawStatusItem(x2, y,      "OLED1", oled1OK);
  drawStatusItem(x1, y + 18, "OLED2", oled2OK);
  drawStatusItem(x2, y + 18, "BME280", bmeOK);
  drawStatusItem(x1, y + 36, "MPU6050", mpuOK);
  drawStatusItem(x2, y + 36, "BMM150", bmmOK);
  drawStatusItem(x1, y + 54, "SD", sdOK);
  drawStatusItem(x2, y + 54, "GPS UART", gpsSerialOK);
  drawStatusItem(x1, y + 72, "KEYPAD", keypadOK);
  drawStatusItem(x2, y + 72, "GPS LOCK", gpsFix);

  tft.drawLine(10, 132, 310, 132, ILI9341_DARKGREY);

  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(10, 146);
  tft.print("I2C0: 21/22  OLED1 + sensors + keypad");
  tft.setCursor(10, 162);
  tft.print("I2C1: 32/33  OLED2 GPS display");
  tft.setCursor(10, 178);
  tft.print("SPI: TFT CS=5, SD CS=13");
  tft.setCursor(10, 194);
  tft.print("GPS chars: ");
  tft.print(gpsCharsProcessed);

  drawFooter("A next B back");
}

void drawCurrentTFTScreen() {
  if (!tftOK) return;

  switch (currentPage) {
    case PAGE_HOME:
      drawHomeScreen();
      break;
    case PAGE_PATH_TRACKING:
      drawPathTrackingScreen();
      break;
    case PAGE_ENVIRONMENT:
      drawEnvironmentScreen();
      break;
    case PAGE_LOGGING:
      drawLoggingScreen();
      break;
    case PAGE_MOVEMENT:
      drawMovementScreen();
      break;
    case PAGE_STATUS:
      drawStatusScreen();
      break;
    default:
      drawHomeScreen();
      break;
  }
}

// =====================================================
// Serial Debug
// =====================================================

void printSerialSummary() {
  Serial.println();
  Serial.println("----- GeoTracker V1.0 UI v2 Summary -----");

  Serial.print("Page: ");
  Serial.print(pageTitle(currentPage));
  Serial.print(" | Tracking: ");
  Serial.print(trackingActive ? "ON" : "OFF");
  Serial.print(" | Target: ");
  Serial.println(targetActive ? targetName : "NONE");

  Serial.print("GPS Fix: ");
  Serial.print(gpsFixString());
  Serial.print(" | Sat: ");
  Serial.print(gpsSatellites);
  Serial.print(" | Chars: ");
  Serial.print(gpsCharsProcessed);
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

  Serial.print("MPU6050 Pitch/Roll/AccelMag: ");
  Serial.print(pitchDeg, 1);
  Serial.print(", ");
  Serial.print(rollDeg, 1);
  Serial.print(", ");
  Serial.println(accelMagnitude, 2);

  Serial.print("BMM150 Heading: ");
  Serial.print(headingDeg, 1);
  Serial.print(" deg ");
  Serial.print(cardinalDirection(headingDeg));
  Serial.print(" | Field: ");
  Serial.print(fieldStrength, 1);
  Serial.println(" uT");

  Serial.print("Track distance: ");
  Serial.print(totalDistanceM, 1);
  Serial.print(" m | Duration: ");
  Serial.print(durationString(trackingDurationMs()));
  Serial.print(" | Rows: ");
  Serial.println(logRow);
}

// =====================================================
// Setup / Loop
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" GeoTracker V1.0 Final Integration UI v2");
  Serial.println("========================================");

  beginDisplays();

  delay(300);
  scanMainI2C();

  beginBME280();
  beginMPU6050();
  beginBMM150();
  beginGPS();
  beginKeypad();
  beginSDCard();

  readAllSensors();

  setStatusMessage("Boot complete", 3000);

  drawOLEDCompass();
  drawOLEDGPS();
  drawCurrentTFTScreen();

  saveEvent("BOOT", "GeoTracker UI v2 started");

  Serial.println("GeoTracker V1.0 Final UI v2 running.");
}

void loop() {
  // GPS must be serviced constantly.
  readGPS();

  handleKeypad();

  if (millis() - lastSensorRead >= SENSOR_INTERVAL_MS) {
    lastSensorRead = millis();
    readBME280();
    readMPU6050();
    readBMM150();

    if (trackingActive) {
      updateTrackStats();
    }
  }

  if (millis() - lastOLEDUpdate >= OLED_INTERVAL_MS) {
    lastOLEDUpdate = millis();
    drawOLEDCompass();
    drawOLEDGPS();
  }

  if (millis() - lastTFTUpdate >= TFT_INTERVAL_MS) {
    lastTFTUpdate = millis();
    drawCurrentTFTScreen();
  }

  if (trackingActive && millis() - lastSDLog >= LOG_INTERVAL_MS) {
    lastSDLog = millis();
    logToSD();
  }

  if (millis() - lastSerialPrint >= SERIAL_INTERVAL_MS) {
    lastSerialPrint = millis();
    printSerialSummary();
  }
}