/*
  GeoScope v1.0
  All Display Mock Test

  Displays:
  - 2.4" SPI TFT display
  - OLED #1: Compass/Tilt display
  - OLED #2: GPS/Satellite display

  TFT wiring:
    VCC -> 3.3V or 5V depending on module
    GND -> GND
    DIN -> GPIO23
    CLK -> GPIO18
    CS  -> GPIO5
    DC  -> GPIO2
    RST -> GPIO4
    BL  -> 3.3V

  OLED #1:
    SDA -> GPIO21
    SCL -> GPIO22
    Address: 0x3C

  OLED #2:
    SDA -> GPIO32
    SCL -> GPIO33
    Address: 0x3C
*/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_SSD1306.h>

// =====================================================
// TFT pins
// =====================================================
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4

#define TFT_CLK   18
#define TFT_DIN   23
#define TFT_MISO  -1   // Your TFT has no MISO pin

// =====================================================
// OLED pins
// =====================================================
#define OLED1_SDA 21
#define OLED1_SCL 22

#define OLED2_SDA 32
#define OLED2_SCL 33

#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_RESET -1

// =====================================================
// Display objects
// =====================================================

// TFT
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Two independent I2C buses
TwoWire I2C_OLED1 = TwoWire(0);
TwoWire I2C_OLED2 = TwoWire(1);

// OLED displays
Adafruit_SSD1306 oledCompass(OLED_WIDTH, OLED_HEIGHT, &I2C_OLED1, OLED_RESET);
Adafruit_SSD1306 oledGPS(OLED_WIDTH, OLED_HEIGHT, &I2C_OLED2, OLED_RESET);

// =====================================================
// Mock data
// =====================================================
float mockLat = 43.826100;
float mockLon = -111.789700;
float mockAlt = 1486.2;
float mockSpeed = 1.2;

float mockHeading = 0.0;
float mockPitch = -5.0;
float mockRoll = 8.0;

int mockSatellites = 8;
float mockHDOP = 1.2;

float mockTemp = 21.4;
float mockHumidity = 38.0;
float mockPressure = 846.3;

unsigned long lastUpdate = 0;
int tftScreen = 0;

// =====================================================
// OLED #1: Compass/Tilt
// =====================================================
void drawCompassOLED() {
  oledCompass.clearDisplay();
  oledCompass.setTextColor(SSD1306_WHITE);

  oledCompass.setTextSize(1);
  oledCompass.setCursor(0, 0);
  oledCompass.println("GEOSCOPE");
  oledCompass.println("COMPASS/TILT");
  oledCompass.drawLine(0, 18, 127, 18, SSD1306_WHITE);

  oledCompass.setCursor(0, 24);
  oledCompass.print("Head: ");
  oledCompass.print(mockHeading, 0);
  oledCompass.println(" deg");

  oledCompass.setCursor(0, 36);
  oledCompass.print("Pitch:");
  oledCompass.print(mockPitch, 1);

  oledCompass.setCursor(0, 48);
  oledCompass.print("Roll: ");
  oledCompass.print(mockRoll, 1);

  // Compass circle
  int cx = 105;
  int cy = 41;
  int r = 16;

  oledCompass.drawCircle(cx, cy, r, SSD1306_WHITE);

  float angle = mockHeading * PI / 180.0;

  int x2 = cx + sin(angle) * 12;
  int y2 = cy - cos(angle) * 12;

  oledCompass.drawLine(cx, cy, x2, y2, SSD1306_WHITE);
  oledCompass.fillCircle(x2, y2, 2, SSD1306_WHITE);

  oledCompass.display();
}

// =====================================================
// OLED #2: GPS/Satellite Status
// =====================================================
void drawGPSOLED() {
  oledGPS.clearDisplay();
  oledGPS.setTextColor(SSD1306_WHITE);

  oledGPS.setTextSize(1);
  oledGPS.setCursor(0, 0);
  oledGPS.println("GEOSCOPE");
  oledGPS.println("GPS STATUS");
  oledGPS.drawLine(0, 18, 127, 18, SSD1306_WHITE);

  oledGPS.setCursor(0, 24);
  oledGPS.print("Fix: ");
  oledGPS.println("3D MOCK");

  oledGPS.setCursor(0, 36);
  oledGPS.print("Sat: ");
  oledGPS.println(mockSatellites);

  oledGPS.setCursor(0, 48);
  oledGPS.print("HDOP:");
  oledGPS.print(mockHDOP, 1);

  // Mock satellite bars
  for (int i = 0; i < 5; i++) {
    int h = 5 + i * 4;
    oledGPS.fillRect(88 + i * 7, 58 - h, 5, h, SSD1306_WHITE);
  }

  oledGPS.display();
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
// TFT Screen 1: GPS Dashboard
// =====================================================
void drawGPSDashboardTFT() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("GEOSCOPE GPS");

  tft.setTextSize(2);

  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(10, 45);
  tft.print("Fix: 3D MOCK");

  tft.setCursor(210, 45);
  tft.print("Sat:");
  tft.print(mockSatellites);

  tft.setTextColor(ILI9341_WHITE);

  tft.setCursor(10, 80);
  tft.print("Lat:");
  tft.setCursor(80, 80);
  tft.print(mockLat, 6);

  tft.setCursor(10, 110);
  tft.print("Lon:");
  tft.setCursor(80, 110);
  tft.print(mockLon, 6);

  tft.setCursor(10, 145);
  tft.print("Alt:");
  tft.setCursor(80, 145);
  tft.print(mockAlt, 1);
  tft.print(" m");

  tft.setCursor(10, 175);
  tft.print("Spd:");
  tft.setCursor(80, 175);
  tft.print(mockSpeed, 1);
  tft.print(" m/s");

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 222);
  tft.print("TFT + dual OLED mock display test");
}

// =====================================================
// TFT Screen 2: Live Track Map Mock
// =====================================================
void drawMapTFT() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("LIVE TRACK MAP");

  // Map area
  tft.drawRect(10, 40, 300, 170, ILI9341_WHITE);

  // Grid
  for (int x = 40; x < 310; x += 30) {
    tft.drawLine(x, 40, x, 210, ILI9341_DARKGREY);
  }

  for (int y = 70; y < 210; y += 30) {
    tft.drawLine(10, y, 310, y, ILI9341_DARKGREY);
  }

  // Mock trail points
  int p[][2] = {
    {55, 175},
    {80, 160},
    {110, 150},
    {140, 135},
    {170, 120},
    {205, 105},
    {240, 85}
  };

  for (int i = 0; i < 6; i++) {
    tft.drawLine(p[i][0], p[i][1], p[i + 1][0], p[i + 1][1], ILI9341_GREEN);
  }

  // Start marker
  tft.fillCircle(55, 175, 5, ILI9341_BLUE);
  tft.setTextColor(ILI9341_BLUE);
  tft.setTextSize(1);
  tft.setCursor(38, 185);
  tft.print("START");

  // Waypoint marker
  tft.fillCircle(170, 120, 5, ILI9341_YELLOW);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(178, 112);
  tft.print("WPT1");

  // Current position
  tft.fillCircle(240, 85, 6, ILI9341_RED);
  tft.setTextColor(ILI9341_RED);
  tft.setCursor(250, 79);
  tft.print("YOU");

  // North arrow
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(282, 50);
  tft.print("N");
  tft.drawLine(286, 70, 286, 56, ILI9341_WHITE);
  tft.fillTriangle(286, 50, 282, 58, 290, 58, ILI9341_WHITE);

  // Footer
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 222);
  tft.print("Dist: 0.84 km   Scale: mock");
}

// =====================================================
// TFT Screen 3: Waypoint Navigation Mock
// =====================================================
void drawNavigationTFT() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("WAYPOINT NAV");

  tft.setTextSize(2);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 48);
  tft.print("Target: WPT_001");

  tft.setCursor(10, 82);
  tft.print("Dist:");
  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(120, 82);
  tft.print("248 m");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 116);
  tft.print("Bearing:");
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(120, 116);
  tft.print("126 deg");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 150);
  tft.print("Heading:");
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(120, 150);
  tft.print(mockHeading, 0);
  tft.print(" deg");

  tft.setTextColor(ILI9341_ORANGE);
  tft.setCursor(10, 195);
  tft.print("Turn: RIGHT 25");

  // Direction arrow
  tft.fillTriangle(265, 80, 235, 140, 295, 140, ILI9341_RED);
  tft.fillRect(255, 140, 20, 55, ILI9341_RED);
}

// =====================================================
// TFT Screen 4: Environmental Survey Mock
// =====================================================
void drawEnvironmentTFT() {
  tft.fillScreen(ILI9341_BLACK);
  drawTFTHeader("ENV SURVEY");

  tft.setTextSize(2);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 50);
  tft.print("Temp:");
  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(120, 50);
  tft.print(mockTemp, 1);
  tft.print(" C");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 85);
  tft.print("Hum:");
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(120, 85);
  tft.print(mockHumidity, 1);
  tft.print(" %");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 120);
  tft.print("Press:");
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(120, 120);
  tft.print(mockPressure, 1);
  tft.print(" hPa");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 155);
  tft.print("Lat:");
  tft.setCursor(80, 155);
  tft.print(mockLat, 5);

  tft.setCursor(10, 185);
  tft.print("Lon:");
  tft.setCursor(80, 185);
  tft.print(mockLon, 5);

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 222);
  tft.print("Mock environmental survey screen");
}

// =====================================================
// TFT screen cycling
// =====================================================
void updateTFTScreen() {
  if (tftScreen == 0) {
    drawGPSDashboardTFT();
  } else if (tftScreen == 1) {
    drawMapTFT();
  } else if (tftScreen == 2) {
    drawNavigationTFT();
  } else {
    drawEnvironmentTFT();
  }

  tftScreen++;

  if (tftScreen > 3) {
    tftScreen = 0;
  }
}

// =====================================================
// Mock data update
// =====================================================
void updateMockData() {
  mockHeading += 17.0;
  if (mockHeading >= 360.0) mockHeading -= 360.0;

  mockPitch += 1.3;
  if (mockPitch > 12.0) mockPitch = -12.0;

  mockRoll -= 1.1;
  if (mockRoll < -15.0) mockRoll = 15.0;

  mockSatellites++;
  if (mockSatellites > 14) mockSatellites = 6;

  mockHDOP += 0.1;
  if (mockHDOP > 2.2) mockHDOP = 0.8;

  mockLat += 0.000010;
  mockLon += 0.000015;
  mockAlt += 0.2;

  mockSpeed += 0.1;
  if (mockSpeed > 2.4) mockSpeed = 0.4;

  mockTemp += 0.1;
  if (mockTemp > 24.0) mockTemp = 21.0;

  mockHumidity += 0.3;
  if (mockHumidity > 45.0) mockHumidity = 35.0;

  mockPressure += 0.2;
  if (mockPressure > 849.0) mockPressure = 846.0;
}

// =====================================================
// Setup
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("GeoScope all-display mock test starting...");

  // Start OLED I2C buses
  I2C_OLED1.begin(OLED1_SDA, OLED1_SCL, 400000);
  I2C_OLED2.begin(OLED2_SDA, OLED2_SCL, 400000);

  // Start SPI for TFT
  SPI.begin(TFT_CLK, TFT_MISO, TFT_DIN, TFT_CS);

  // Start TFT
  tft.begin();
  tft.setRotation(1);  // Landscape mode
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(25, 80);
  tft.println("GeoScope v1.0");

  tft.setCursor(25, 110);
  tft.println("Display Test");

  tft.setTextSize(1);
  tft.setCursor(25, 145);
  tft.println("Starting TFT + two OLEDs...");
  delay(1000);

  // Start OLED #1
  if (!oledCompass.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false)) {
    Serial.println("OLED #1 failed.");
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(25, 165);
    tft.println("OLED #1 FAIL");
  } else {
    Serial.println("OLED #1 initialized.");
  }

  // Start OLED #2
  if (!oledGPS.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false)) {
    Serial.println("OLED #2 failed.");
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(25, 180);
    tft.println("OLED #2 FAIL");
  } else {
    Serial.println("OLED #2 initialized.");
  }

  delay(1000);

  drawCompassOLED();
  drawGPSOLED();
  drawGPSDashboardTFT();

  Serial.println("All 3 displays initialized.");
}

// =====================================================
// Loop
// =====================================================
void loop() {
  if (millis() - lastUpdate >= 2000) {
    lastUpdate = millis();

    updateMockData();

    drawCompassOLED();
    drawGPSOLED();
    updateTFTScreen();

    Serial.println("All displays updated.");
  }
}