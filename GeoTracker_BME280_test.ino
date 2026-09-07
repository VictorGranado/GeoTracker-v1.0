/*
  GeoScope v1.0
  BME280 Sensor Test + All Displays

  Purpose:
  - Confirm TFT display still works
  - Confirm both OLED displays still work
  - Test BME280 temperature, humidity, pressure, and pressure altitude

  Main I2C bus:
    SDA -> GPIO21
    SCL -> GPIO22
    Devices:
      OLED #1 at 0x3C
      BME280 at 0x76 or 0x77

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
#include <Adafruit_BME280.h>

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
// BME280 settings
// =====================================================
#define SEA_LEVEL_HPA 1013.25

// =====================================================
// Display objects
// =====================================================
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Main I2C bus uses default Wire
// OLED #1 is on Wire
Adafruit_SSD1306 oledEnv(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// OLED #2 is on second I2C bus
TwoWire I2C_OLED2 = TwoWire(1);
Adafruit_SSD1306 oledStatus(OLED_WIDTH, OLED_HEIGHT, &I2C_OLED2, OLED_RESET);

// =====================================================
// Sensor object
// =====================================================
Adafruit_BME280 bme;

// =====================================================
// Global values
// =====================================================
bool bmeOK = false;
uint8_t bmeAddress = 0x00;

float tempC = 0.0;
float humidity = 0.0;
float pressureHpa = 0.0;
float pressureAltM = 0.0;

unsigned long lastUpdate = 0;

// =====================================================
// I2C scanner for main bus
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
// Start BME280
// =====================================================
void beginBME280() {
  if (bme.begin(0x76, &Wire)) {
    bmeOK = true;
    bmeAddress = 0x76;
  } 
  else if (bme.begin(0x77, &Wire)) {
    bmeOK = true;
    bmeAddress = 0x77;
  } 
  else {
    bmeOK = false;
  }

  if (bmeOK) {
    Serial.print("BME280 initialized at address 0x");
    Serial.println(bmeAddress, HEX);
  } else {
    Serial.println("BME280 not found at 0x76 or 0x77.");
  }
}

// =====================================================
// Read BME280
// =====================================================
void readBME280() {
  if (!bmeOK) return;

  tempC = bme.readTemperature();
  humidity = bme.readHumidity();
  pressureHpa = bme.readPressure() / 100.0;

  pressureAltM = 44330.0 * (1.0 - pow(pressureHpa / SEA_LEVEL_HPA, 0.1903));
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
// TFT BME280 screen
// =====================================================
void drawTFTBME280() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("BME280 TEST");

  if (!bmeOK) {
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(20, 70);
    tft.println("BME280 FAIL");

    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(20, 115);
    tft.println("Check wiring/address:");
    tft.setCursor(20, 130);
    tft.println("SDA=21 SCL=22");
    tft.setCursor(20, 145);
    tft.println("Try address 0x76/0x77");

    return;
  }

  tft.setTextSize(2);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 50);
  tft.print("Temp:");

  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(135, 50);
  tft.print(tempC, 1);
  tft.print(" C");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 85);
  tft.print("Hum:");

  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(135, 85);
  tft.print(humidity, 1);
  tft.print(" %");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 120);
  tft.print("Press:");

  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(135, 120);
  tft.print(pressureHpa, 1);
  tft.print(" hPa");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 155);
  tft.print("P Alt:");

  tft.setTextColor(ILI9341_MAGENTA);
  tft.setCursor(135, 155);
  tft.print(pressureAltM, 1);
  tft.print(" m");

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 222);
  tft.print("BME280 address: 0x");
  tft.print(bmeAddress, HEX);
}

// =====================================================
// OLED #1: Environmental compact screen
// =====================================================
void drawEnvOLED() {
  oledEnv.clearDisplay();
  oledEnv.setTextColor(SSD1306_WHITE);

  oledEnv.setTextSize(1);
  oledEnv.setCursor(0, 0);
  oledEnv.println("GEOSCOPE");
  oledEnv.println("BME280 ENV");
  oledEnv.drawLine(0, 18, 127, 18, SSD1306_WHITE);

  if (!bmeOK) {
    oledEnv.setCursor(0, 28);
    oledEnv.println("BME280 FAIL");
    oledEnv.setCursor(0, 42);
    oledEnv.println("Check I2C");
  } else {
    oledEnv.setCursor(0, 24);
    oledEnv.print("T: ");
    oledEnv.print(tempC, 1);
    oledEnv.println(" C");

    oledEnv.setCursor(0, 36);
    oledEnv.print("H: ");
    oledEnv.print(humidity, 1);
    oledEnv.println(" %");

    oledEnv.setCursor(0, 48);
    oledEnv.print("P: ");
    oledEnv.print(pressureHpa, 1);
    oledEnv.println(" hPa");
  }

  oledEnv.display();
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

  oledStatus.setCursor(0, 26);
  oledStatus.print("BME280: ");
  oledStatus.println(bmeOK ? "OK" : "FAIL");

  oledStatus.setCursor(0, 40);
  oledStatus.print("Addr: ");
  if (bmeOK) {
    oledStatus.print("0x");
    oledStatus.println(bmeAddress, HEX);
  } else {
    oledStatus.println("--");
  }

  oledStatus.setCursor(0, 54);
  oledStatus.println("Test 1/3");

  oledStatus.display();
}

// =====================================================
// Serial debug
// =====================================================
void printBME280Serial() {
  Serial.println("----- BME280 Reading -----");

  if (!bmeOK) {
    Serial.println("BME280 not found.");
    Serial.println();
    return;
  }

  Serial.print("Temperature C: ");
  Serial.println(tempC);

  Serial.print("Humidity %: ");
  Serial.println(humidity);

  Serial.print("Pressure hPa: ");
  Serial.println(pressureHpa);

  Serial.print("Pressure Altitude m: ");
  Serial.println(pressureAltM);

  Serial.println();
}

// =====================================================
// Setup
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("GeoScope BME280-only test starting...");

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
  tft.println("BME280 Test");

  delay(1000);

  // Start OLED #1
  if (!oledEnv.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false)) {
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

  beginBME280();

  readBME280();

  drawEnvOLED();
  drawStatusOLED();
  drawTFTBME280();

  Serial.println("BME280-only test running.");
}

// =====================================================
// Loop
// =====================================================
void loop() {
  if (millis() - lastUpdate >= 1500) {
    lastUpdate = millis();

    readBME280();

    drawEnvOLED();
    drawStatusOLED();
    drawTFTBME280();

    printBME280Serial();
  }
}