/*
  GeoScope v1.0
  Updated BMM150 Magnetometer Test using DFRobot_BMM150 Library

  Main I2C bus:
    SDA -> GPIO21
    SCL -> GPIO22
    Devices:
      OLED #1 at 0x3C
      BMM150 at 0x13 in your case

  Second I2C bus:
    SDA -> GPIO32
    SCL -> GPIO33
    Device:
      OLED #2 at 0x3C

  TFT SPI:
    DIN -> GPIO23
    CLK -> GPIO18
    CS  -> GPIO5
    DC  -> GPIO2
    RST -> GPIO4
    BL  -> 3.3V
*/

#include <Wire.h>
#include <SPI.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_SSD1306.h>

#include "DFRobot_BMM150.h"

// =====================================================
// TFT pins
// =====================================================

#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4

#define TFT_CLK   18
#define TFT_DIN   23
#define TFT_MISO  -1

// =====================================================
// I2C pins
// =====================================================

#define MAIN_SDA 21
#define MAIN_SCL 22

#define OLED2_SDA 32
#define OLED2_SCL 33

// =====================================================
// OLED settings
// =====================================================

#define OLED_ADDR   0x3C
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_RESET  -1

// =====================================================
// Display objects
// =====================================================

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

Adafruit_SSD1306 oledCompass(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

TwoWire I2C_OLED2 = TwoWire(1);
Adafruit_SSD1306 oledStatus(OLED_WIDTH, OLED_HEIGHT, &I2C_OLED2, OLED_RESET);

// =====================================================
// DFRobot BMM150 objects
// =====================================================

DFRobot_BMM150_I2C bmmAddr1(&Wire, I2C_ADDRESS_1);  // 0x10
DFRobot_BMM150_I2C bmmAddr2(&Wire, I2C_ADDRESS_2);  // 0x11
DFRobot_BMM150_I2C bmmAddr3(&Wire, I2C_ADDRESS_3);  // 0x12
DFRobot_BMM150_I2C bmmAddr4(&Wire, I2C_ADDRESS_4);  // 0x13

DFRobot_BMM150_I2C *bmm = NULL;

// =====================================================
// Global values
// =====================================================

bool bmmOK = false;
uint8_t bmmAddress = 0x00;

float magX = 0.0;
float magY = 0.0;
float magZ = 0.0;

float headingDeg = 0.0;
float calculatedHeadingDeg = 0.0;
float dfrobotCompassDeg = 0.0;
float fieldStrength = 0.0;

unsigned long lastUpdate = 0;
int tftScreen = 0;

// =====================================================
// I2C scanner
// =====================================================

void scanMainI2C() {
  Serial.println("Scanning main I2C bus GPIO21/GPIO22...");

  int devices = 0;

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      devices++;
    }
  }

  Serial.print("Total main I2C devices found: ");
  Serial.println(devices);
  Serial.println();
}

// =====================================================
// BMM150 initialization
// =====================================================

bool tryBMM150(DFRobot_BMM150_I2C &sensor, uint8_t address) {
  Serial.print("Trying BMM150 at 0x");
  if (address < 16) Serial.print("0");
  Serial.println(address, HEX);

  // DFRobot begin() returns 0 on success
  if (sensor.begin() == 0) {
    bmm = &sensor;
    bmmAddress = address;
    bmmOK = true;

    Serial.print("BMM150 initialized at 0x");
    if (bmmAddress < 16) Serial.print("0");
    Serial.println(bmmAddress, HEX);

    return true;
  }

  return false;
}

void beginBMM150() {
  bmmOK = false;
  bmm = NULL;

  if (tryBMM150(bmmAddr1, 0x10)) {
    // Found at 0x10
  } else if (tryBMM150(bmmAddr2, 0x11)) {
    // Found at 0x11
  } else if (tryBMM150(bmmAddr3, 0x12)) {
    // Found at 0x12
  } else if (tryBMM150(bmmAddr4, 0x13)) {
    // Found at 0x13
  }

  if (!bmmOK) {
    Serial.println("BMM150 not found with DFRobot library.");
    return;
  }

  bmm->setOperationMode(BMM150_POWERMODE_NORMAL);
  bmm->setPresetMode(BMM150_PRESETMODE_HIGHACCURACY);
  bmm->setRate(BMM150_DATA_RATE_10HZ);
  bmm->setMeasurementXYZ();

  Serial.println("BMM150 configured:");
  Serial.println("Mode: NORMAL");
  Serial.println("Preset: HIGH ACCURACY");
  Serial.println("Rate: 10Hz");
  Serial.println("XYZ measurement enabled");
}

// =====================================================
// BMM150 reading
// =====================================================

void readBMM150() {
  if (!bmmOK || bmm == NULL) return;

  sBmm150MagData_t magData = bmm->getGeomagneticData();

  magX = magData.x;
  magY = magData.y;
  magZ = magData.z;

  fieldStrength = sqrt((magX * magX) + (magY * magY) + (magZ * magZ));

  // Raw calculated heading from X/Y.
  // Kept only for debugging.
  calculatedHeadingDeg = atan2(magY, magX) * 180.0 / PI;

  if (calculatedHeadingDeg < 0) calculatedHeadingDeg += 360.0;
  if (calculatedHeadingDeg >= 360.0) calculatedHeadingDeg -= 360.0;

  // DFRobot's built-in compass heading.
  dfrobotCompassDeg = bmm->getCompassDegree();

  // Main GeoScope heading now uses DFRobot heading.
  headingDeg = dfrobotCompassDeg;

  if (headingDeg < 0) headingDeg += 360.0;
  if (headingDeg >= 360.0) headingDeg -= 360.0;
}

// =====================================================
// Reading validation
// =====================================================

bool readingLooksValid() {
  if (!bmmOK) return false;

  if (abs(magX) > 3000) return false;
  if (abs(magY) > 3000) return false;
  if (abs(magZ) > 4000) return false;

  return true;
}

// =====================================================
// Cardinal direction
// =====================================================

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

// =====================================================
// TFT helper
// =====================================================

void drawTFTHeader(const char* title) {
  tft.fillRect(0, 0, 320, 30, ILI9341_NAVY);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 7);
  tft.print(title);
}

// =====================================================
// TFT Screen 1: Compass
// =====================================================

void drawTFTCompass() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("BMM150 COMPASS");

  if (!bmmOK) {
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(20, 80);
    tft.println("BMM150 FAIL");

    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(20, 120);
    tft.println("Check scanner output.");
    tft.setCursor(20, 135);
    tft.println("Expected: 0x10-0x13");
    return;
  }

  tft.setTextSize(2);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 45);
  tft.print("Head:");

  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(130, 45);
  tft.print(headingDeg, 1);
  tft.print(" deg");

  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(10, 80);
  tft.print("Dir:");

  tft.setCursor(130, 80);
  tft.print(cardinalDirection(headingDeg));

  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 115);
  tft.print("|B|:");

  tft.setCursor(130, 115);
  tft.print(fieldStrength, 1);

  // Compass circle
  int cx = 245;
  int cy = 145;
  int r = 50;

  tft.drawCircle(cx, cy, r, ILI9341_WHITE);
  tft.drawCircle(cx, cy, 3, ILI9341_WHITE);

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);

  tft.setCursor(cx - 3, cy - r - 12);
  tft.print("N");

  tft.setCursor(cx - 3, cy + r + 4);
  tft.print("S");

  tft.setCursor(cx + r + 5, cy - 3);
  tft.print("E");

  tft.setCursor(cx - r - 12, cy - 3);
  tft.print("W");

  float angle = headingDeg * PI / 180.0;

  int x2 = cx + sin(angle) * 38;
  int y2 = cy - cos(angle) * 38;

  tft.drawLine(cx, cy, x2, y2, ILI9341_RED);
  tft.fillCircle(x2, y2, 4, ILI9341_RED);

  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 222);
  tft.print("BMM150 DFRobot addr: 0x");
  tft.print(bmmAddress, HEX);
}

// =====================================================
// TFT Screen 2: Raw Magnetic Field
// =====================================================

void drawTFTRawMag() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("MAG FIELD RAW");

  if (!bmmOK) {
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(20, 80);
    tft.println("BMM150 FAIL");
    return;
  }

  tft.setTextSize(2);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 45);
  tft.print("Mag X:");
  tft.setCursor(140, 45);
  tft.print(magX, 1);

  tft.setCursor(10, 80);
  tft.print("Mag Y:");
  tft.setCursor(140, 80);
  tft.print(magY, 1);

  tft.setCursor(10, 115);
  tft.print("Mag Z:");
  tft.setCursor(140, 115);
  tft.print(magZ, 1);

  tft.setCursor(10, 155);
  tft.print("|B|:");

  tft.setTextColor(readingLooksValid() ? ILI9341_GREEN : ILI9341_RED);
  tft.setCursor(140, 155);
  tft.print(fieldStrength, 1);

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 222);
  tft.print("Magnetic field values in uT");
}

// =====================================================
// TFT Screen 3: Heading Compare
// =====================================================

void drawTFTCompare() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("HEADING COMPARE");

  if (!bmmOK) {
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(20, 80);
    tft.println("BMM150 FAIL");
    return;
  }

  tft.setTextSize(2);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 50);
  tft.print("DFRobot:");

  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(160, 50);
  tft.print(dfrobotCompassDeg, 1);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 90);
  tft.print("Calc XY:");

  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(160, 90);
  tft.print(calculatedHeadingDeg, 1);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 135);
  tft.print("Using:");

  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(160, 135);
  tft.print("DFRobot");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 175);
  tft.print("Dir:");

  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(160, 175);
  tft.print(cardinalDirection(headingDeg));

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 222);
  tft.print("GeoScope heading = DFRobot compass degree");
}

// =====================================================
// TFT Screen 4: Status
// =====================================================

void drawTFTStatus() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("BMM150 STATUS");

  tft.setTextSize(2);

  tft.setTextColor(bmmOK ? ILI9341_GREEN : ILI9341_RED);
  tft.setCursor(10, 45);
  tft.print("BMM150: ");
  tft.print(bmmOK ? "OK" : "FAIL");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 85);
  tft.print("Addr:");

  if (bmmOK) {
    tft.setCursor(130, 85);
    tft.print("0x");
    tft.print(bmmAddress, HEX);
  } else {
    tft.setCursor(130, 85);
    tft.print("--");
  }

  tft.setCursor(10, 125);
  tft.print("Data:");

  tft.setTextColor(readingLooksValid() ? ILI9341_GREEN : ILI9341_RED);
  tft.setCursor(130, 125);
  tft.print(readingLooksValid() ? "VALID" : "BAD");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 170);
  tft.print("Dir:");

  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(130, 170);
  tft.print(cardinalDirection(headingDeg));

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 222);
  tft.print("DFRobot_BMM150 updated compass test");
}

// =====================================================
// TFT screen cycling
// =====================================================

void updateTFTScreen() {
  if (tftScreen == 0) {
    drawTFTCompass();
  } else if (tftScreen == 1) {
    drawTFTRawMag();
  } else if (tftScreen == 2) {
    drawTFTCompare();
  } else {
    drawTFTStatus();
  }

  tftScreen++;

  if (tftScreen > 3) {
    tftScreen = 0;
  }
}

// =====================================================
// OLED #1: Compass compact screen
// =====================================================

void drawCompassOLED() {
  oledCompass.clearDisplay();
  oledCompass.setTextColor(SSD1306_WHITE);

  oledCompass.setTextSize(1);
  oledCompass.setCursor(0, 0);
  oledCompass.println("GEOSCOPE");
  oledCompass.println("BMM150 DFR");

  oledCompass.drawLine(0, 18, 127, 18, SSD1306_WHITE);

  if (!bmmOK) {
    oledCompass.setCursor(0, 30);
    oledCompass.println("BMM150 FAIL");
    oledCompass.setCursor(0, 44);
    oledCompass.println("Check I2C");
  } else {
    oledCompass.setCursor(0, 24);
    oledCompass.print("Head:");
    oledCompass.print(headingDeg, 0);
    oledCompass.println(" deg");

    oledCompass.setCursor(0, 36);
    oledCompass.print("Dir: ");
    oledCompass.println(cardinalDirection(headingDeg));

    oledCompass.setCursor(0, 48);
    oledCompass.print("|B|: ");
    oledCompass.print(fieldStrength, 0);

    int cx = 105;
    int cy = 41;
    int r = 16;

    oledCompass.drawCircle(cx, cy, r, SSD1306_WHITE);

    float angle = headingDeg * PI / 180.0;

    int x2 = cx + sin(angle) * 12;
    int y2 = cy - cos(angle) * 12;

    oledCompass.drawLine(cx, cy, x2, y2, SSD1306_WHITE);
    oledCompass.fillCircle(x2, y2, 2, SSD1306_WHITE);
  }

  oledCompass.display();
}

// =====================================================
// OLED #2: Status screen
// =====================================================

void drawStatusOLED() {
  oledStatus.clearDisplay();
  oledStatus.setTextColor(SSD1306_WHITE);

  oledStatus.setTextSize(1);
  oledStatus.setCursor(0, 0);
  oledStatus.println("GEOSCOPE");
  oledStatus.println("SENSOR STATUS");

  oledStatus.drawLine(0, 18, 127, 18, SSD1306_WHITE);

  oledStatus.setCursor(0, 24);
  oledStatus.print("BMM150: ");
  oledStatus.println(bmmOK ? "OK" : "FAIL");

  oledStatus.setCursor(0, 36);
  oledStatus.print("Addr: ");
  if (bmmOK) {
    oledStatus.print("0x");
    oledStatus.println(bmmAddress, HEX);
  } else {
    oledStatus.println("--");
  }

  oledStatus.setCursor(0, 48);
  oledStatus.print("Data: ");
  oledStatus.println(readingLooksValid() ? "OK" : "BAD");

  oledStatus.display();
}

// =====================================================
// Serial debug
// =====================================================

void printBMMSerial() {
  Serial.println("----- DFRobot BMM150 Reading -----");

  if (!bmmOK) {
    Serial.println("BMM150 not found.");
    Serial.println();
    return;
  }

  Serial.print("Address: 0x");
  if (bmmAddress < 16) Serial.print("0");
  Serial.println(bmmAddress, HEX);

  Serial.print("Mag X/Y/Z uT: ");
  Serial.print(magX);
  Serial.print(", ");
  Serial.print(magY);
  Serial.print(", ");
  Serial.println(magZ);

  Serial.print("Field strength uT: ");
  Serial.println(fieldStrength);

  Serial.print("Calculated XY heading deg: ");
  Serial.println(calculatedHeadingDeg);

  Serial.print("DFRobot compass deg: ");
  Serial.println(dfrobotCompassDeg);

  Serial.print("GeoScope heading deg: ");
  Serial.println(headingDeg);

  Serial.print("Direction: ");
  Serial.println(cardinalDirection(headingDeg));

  Serial.print("Reading valid: ");
  Serial.println(readingLooksValid() ? "YES" : "NO");

  Serial.println();
}

// =====================================================
// Setup
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("GeoScope updated DFRobot BMM150 test starting...");

  // Main I2C bus for OLED #1 and BMM150
  Wire.begin(MAIN_SDA, MAIN_SCL);
  Wire.setClock(100000);

  // Second I2C bus for OLED #2
  I2C_OLED2.begin(OLED2_SDA, OLED2_SCL, 400000);

  delay(200);

  scanMainI2C();

  // Start SPI for TFT
  SPI.begin(TFT_CLK, TFT_MISO, TFT_DIN, TFT_CS);

  // Start TFT
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(25, 80);
  tft.println("GeoScope v1.0");

  tft.setCursor(25, 110);
  tft.println("BMM150 Updated");

  tft.setTextSize(1);
  tft.setCursor(25, 145);
  tft.println("Using DFRobot compass heading");

  delay(1000);

  // Start OLED #1
  if (!oledCompass.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false)) {
    Serial.println("OLED #1 failed.");
  } else {
    Serial.println("OLED #1 initialized.");
  }

  // Start OLED #2
  if (!oledStatus.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false)) {
    Serial.println("OLED #2 failed.");
  } else {
    Serial.println("OLED #2 initialized.");
  }

  beginBMM150();

  delay(500);

  readBMM150();

  drawCompassOLED();
  drawStatusOLED();
  drawTFTCompass();

  Serial.println("Updated DFRobot BMM150 test running.");
}

// =====================================================
// Loop
// =====================================================

void loop() {
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();

    readBMM150();

    drawCompassOLED();
    drawStatusOLED();
    updateTFTScreen();

    printBMMSerial();
  }
}