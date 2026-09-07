/*
  GeoScope v1.0
  MPU-6050 Only Test + All Displays

  Main I2C bus:
    SDA -> GPIO21
    SCL -> GPIO22
    Devices:
      OLED #1 at 0x3C
      MPU-6050 at 0x68 or 0x69

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

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

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

Adafruit_SSD1306 oledMotion(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

TwoWire I2C_OLED2 = TwoWire(1);
Adafruit_SSD1306 oledStatus(OLED_WIDTH, OLED_HEIGHT, &I2C_OLED2, OLED_RESET);

// =====================================================
// MPU object
// =====================================================

Adafruit_MPU6050 mpu;

// =====================================================
// Global values
// =====================================================

bool mpuOK = false;
uint8_t mpuAddress = 0x00;

float accelX = 0.0;
float accelY = 0.0;
float accelZ = 0.0;

float gyroX = 0.0;
float gyroY = 0.0;
float gyroZ = 0.0;

float pitchDeg = 0.0;
float rollDeg = 0.0;

float accelMagnitude = 0.0;

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
// Start MPU-6050
// =====================================================

void beginMPU6050() {
  if (mpu.begin(0x68, &Wire)) {
    mpuOK = true;
    mpuAddress = 0x68;
  } 
  else if (mpu.begin(0x69, &Wire)) {
    mpuOK = true;
    mpuAddress = 0x69;
  } 
  else {
    mpuOK = false;
  }

  if (mpuOK) {
    Serial.print("MPU-6050 initialized at address 0x");
    Serial.println(mpuAddress, HEX);

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  } else {
    Serial.println("MPU-6050 not found at 0x68 or 0x69.");
  }
}

// =====================================================
// Read MPU-6050
// =====================================================

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
// TFT Screen 1: Pitch/Roll
// =====================================================

void drawTFTPitchRoll() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("MPU6050 TILT");

  if (!mpuOK) {
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(20, 80);
    tft.println("MPU6050 FAIL");

    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(20, 120);
    tft.println("Check SDA=21, SCL=22");
    tft.setCursor(20, 135);
    tft.println("Expected addr: 0x68/0x69");
    return;
  }

  tft.setTextSize(2);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 55);
  tft.print("Pitch:");

  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(140, 55);
  tft.print(pitchDeg, 1);
  tft.print(" deg");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 95);
  tft.print("Roll:");

  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(140, 95);
  tft.print(rollDeg, 1);
  tft.print(" deg");

  // Simple tilt crosshair
  int cx = 240;
  int cy = 135;
  int boxSize = 70;

  tft.drawRect(cx - boxSize / 2, cy - boxSize / 2, boxSize, boxSize, ILI9341_WHITE);
  tft.drawLine(cx - 35, cy, cx + 35, cy, ILI9341_DARKGREY);
  tft.drawLine(cx, cy - 35, cx, cy + 35, ILI9341_DARKGREY);

  int dotX = cx + constrain((int)rollDeg, -30, 30);
  int dotY = cy + constrain((int)pitchDeg, -30, 30);

  tft.fillCircle(dotX, dotY, 5, ILI9341_RED);

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 222);
  tft.print("MPU address: 0x");
  tft.print(mpuAddress, HEX);
}

// =====================================================
// TFT Screen 2: Acceleration
// =====================================================

void drawTFTAcceleration() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("MPU6050 ACCEL");

  if (!mpuOK) {
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(20, 80);
    tft.println("MPU6050 FAIL");
    return;
  }

  tft.setTextSize(2);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 50);
  tft.print("Acc X:");
  tft.setCursor(140, 50);
  tft.print(accelX, 2);

  tft.setCursor(10, 85);
  tft.print("Acc Y:");
  tft.setCursor(140, 85);
  tft.print(accelY, 2);

  tft.setCursor(10, 120);
  tft.print("Acc Z:");
  tft.setCursor(140, 120);
  tft.print(accelZ, 2);

  tft.setCursor(10, 165);
  tft.print("|A|:");
  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(140, 165);
  tft.print(accelMagnitude, 2);

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 222);
  tft.print("Acceleration in m/s^2");
}

// =====================================================
// TFT Screen 3: Gyroscope
// =====================================================

void drawTFTGyro() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("MPU6050 GYRO");

  if (!mpuOK) {
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(20, 80);
    tft.println("MPU6050 FAIL");
    return;
  }

  tft.setTextSize(2);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 50);
  tft.print("Gyro X:");
  tft.setCursor(150, 50);
  tft.print(gyroX, 1);

  tft.setCursor(10, 85);
  tft.print("Gyro Y:");
  tft.setCursor(150, 85);
  tft.print(gyroY, 1);

  tft.setCursor(10, 120);
  tft.print("Gyro Z:");
  tft.setCursor(150, 120);
  tft.print(gyroZ, 1);

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 222);
  tft.print("Gyroscope values in deg/s");
}

// =====================================================
// TFT screen cycling
// =====================================================

void updateTFTScreen() {
  if (tftScreen == 0) {
    drawTFTPitchRoll();
  } else if (tftScreen == 1) {
    drawTFTAcceleration();
  } else {
    drawTFTGyro();
  }

  tftScreen++;

  if (tftScreen > 2) {
    tftScreen = 0;
  }
}

// =====================================================
// OLED #1: Motion compact display
// =====================================================

void drawMotionOLED() {
  oledMotion.clearDisplay();
  oledMotion.setTextColor(SSD1306_WHITE);

  oledMotion.setTextSize(1);
  oledMotion.setCursor(0, 0);
  oledMotion.println("GEOSCOPE");
  oledMotion.println("MPU6050");

  oledMotion.drawLine(0, 18, 127, 18, SSD1306_WHITE);

  if (!mpuOK) {
    oledMotion.setCursor(0, 30);
    oledMotion.println("MPU FAIL");
    oledMotion.setCursor(0, 44);
    oledMotion.println("Check I2C");
  } else {
    oledMotion.setCursor(0, 24);
    oledMotion.print("Pitch:");
    oledMotion.print(pitchDeg, 1);

    oledMotion.setCursor(0, 36);
    oledMotion.print("Roll: ");
    oledMotion.print(rollDeg, 1);

    oledMotion.setCursor(0, 48);
    oledMotion.print("|A|:  ");
    oledMotion.print(accelMagnitude, 1);
  }

  oledMotion.display();
}

// =====================================================
// OLED #2: Status display
// =====================================================

void drawStatusOLED() {
  oledStatus.clearDisplay();
  oledStatus.setTextColor(SSD1306_WHITE);

  oledStatus.setTextSize(1);
  oledStatus.setCursor(0, 0);
  oledStatus.println("GEOSCOPE");
  oledStatus.println("SENSOR STATUS");

  oledStatus.drawLine(0, 18, 127, 18, SSD1306_WHITE);

  oledStatus.setCursor(0, 26);
  oledStatus.print("MPU6050: ");
  oledStatus.println(mpuOK ? "OK" : "FAIL");

  oledStatus.setCursor(0, 40);
  oledStatus.print("Addr: ");
  if (mpuOK) {
    oledStatus.print("0x");
    oledStatus.println(mpuAddress, HEX);
  } else {
    oledStatus.println("--");
  }

  oledStatus.setCursor(0, 54);
  oledStatus.println("Test 2/3");

  oledStatus.display();
}

// =====================================================
// Serial debug
// =====================================================

void printMPUSerial() {
  Serial.println("----- MPU-6050 Reading -----");

  if (!mpuOK) {
    Serial.println("MPU-6050 not found.");
    Serial.println();
    return;
  }

  Serial.print("Pitch deg: ");
  Serial.println(pitchDeg);

  Serial.print("Roll deg: ");
  Serial.println(rollDeg);

  Serial.print("Accel X/Y/Z m/s^2: ");
  Serial.print(accelX);
  Serial.print(", ");
  Serial.print(accelY);
  Serial.print(", ");
  Serial.println(accelZ);

  Serial.print("Gyro X/Y/Z deg/s: ");
  Serial.print(gyroX);
  Serial.print(", ");
  Serial.print(gyroY);
  Serial.print(", ");
  Serial.println(gyroZ);

  Serial.print("Accel magnitude: ");
  Serial.println(accelMagnitude);

  Serial.println();
}

// =====================================================
// Setup
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("GeoScope MPU-6050-only test starting...");

  // Start main I2C bus
  Wire.begin(MAIN_SDA, MAIN_SCL, 400000);

  // Start second I2C bus for OLED #2
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
  tft.println("MPU6050 Test");

  delay(1000);

  // Start OLED #1
  if (!oledMotion.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false)) {
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

  beginMPU6050();

  readMPU6050();

  drawMotionOLED();
  drawStatusOLED();
  drawTFTPitchRoll();

  Serial.println("MPU-6050-only test running.");
}

// =====================================================
// Loop
// =====================================================

void loop() {
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();

    readMPU6050();

    drawMotionOLED();
    drawStatusOLED();
    updateTFTScreen();

    printMPUSerial();
  }
}