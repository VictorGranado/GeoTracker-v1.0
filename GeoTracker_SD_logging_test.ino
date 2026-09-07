/*
  GeoScope v1.0
  Advanced MicroSD Serial Test

  This test:
  - Initializes the SD card
  - Creates /GEOSCOPE folder
  - Writes a human-readable report
  - Writes a CSV field log
  - Writes a CSV waypoint file
  - Writes a speed test file
  - Reads files back through Serial Monitor
  - Lets you remove the SD card and inspect the files on your computer

  SD wiring:
    VCC  -> 5V
    GND  -> GND
    SCK  -> GPIO18
    MISO -> GPIO19
    MOSI -> GPIO23
    CS   -> GPIO13
*/

#include <SPI.h>
#include <SD.h>

// =====================================================
// SD pins
// =====================================================

#define SD_SCK   18
#define SD_MISO  19
#define SD_MOSI  23
#define SD_CS    13

// =====================================================
// File paths
// =====================================================

const char *folderPath   = "/GEOSCOPE";
const char *reportPath   = "/GEOSCOPE/REPORT.TXT";
const char *fieldLogPath = "/GEOSCOPE/FIELDLOG.CSV";
const char *waypointPath = "/GEOSCOPE/WAYPTS.CSV";
const char *speedPath    = "/GEOSCOPE/SPEED.TXT";

// =====================================================
// Test settings
// =====================================================

const int FIELD_LOG_ROWS = 30;
const int SPEED_LINES = 300;

bool sdOK = false;

// Mock GeoScope data
float lat = 43.826100;
float lon = -111.789700;
float altM = 1486.2;

float tempC = 21.5;
float humidity = 38.0;
float pressureHpa = 846.5;

float pitchDeg = -2.4;
float rollDeg = 98.0;
float headingDeg = 296.5;

int satellites = 8;

// =====================================================
// Helpers
// =====================================================

void printDivider() {
  Serial.println();
  Serial.println("--------------------------------------------------");
}

void printCardType() {
  uint8_t cardType = SD.cardType();

  Serial.print("Card type: ");

  if (cardType == CARD_NONE) {
    Serial.println("NONE");
  } else if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
}

void printCardInfo() {
  printDivider();
  Serial.println("SD CARD INFO");

  printCardType();

  uint64_t cardSizeMB = SD.cardSize() / (1024 * 1024);

  Serial.print("Card size: ");
  Serial.print(cardSizeMB);
  Serial.println(" MB");

  #if defined(ESP32)
    uint64_t totalMB = SD.totalBytes() / (1024 * 1024);
    uint64_t usedMB  = SD.usedBytes() / (1024 * 1024);

    Serial.print("Filesystem total: ");
    Serial.print(totalMB);
    Serial.println(" MB");

    Serial.print("Filesystem used: ");
    Serial.print(usedMB);
    Serial.println(" MB");
  #endif
}

void removeFileIfExists(const char *path) {
  if (SD.exists(path)) {
    Serial.print("Removing old file: ");
    Serial.println(path);
    SD.remove(path);
  }
}

void createGeoScopeFolder() {
  if (!SD.exists(folderPath)) {
    Serial.println("Creating /GEOSCOPE folder...");
    if (SD.mkdir(folderPath)) {
      Serial.println("Folder created.");
    } else {
      Serial.println("Failed to create folder.");
    }
  } else {
    Serial.println("/GEOSCOPE folder already exists.");
  }
}

void printFileSize(const char *path) {
  File file = SD.open(path);

  if (!file) {
    Serial.print("Could not open file for size check: ");
    Serial.println(path);
    return;
  }

  Serial.print(path);
  Serial.print(" size: ");
  Serial.print(file.size());
  Serial.println(" bytes");

  file.close();
}

void previewTextFile(const char *path, int maxLines) {
  File file = SD.open(path);

  if (!file) {
    Serial.print("Could not open file for preview: ");
    Serial.println(path);
    return;
  }

  printDivider();
  Serial.print("PREVIEW: ");
  Serial.println(path);

  int lineCount = 0;

  while (file.available() && lineCount < maxLines) {
    String line = file.readStringUntil('\n');
    Serial.println(line);
    lineCount++;
  }

  if (file.available()) {
    Serial.println("... preview stopped ...");
  }

  file.close();
}

int countLines(const char *path) {
  File file = SD.open(path);

  if (!file) {
    return -1;
  }

  int lines = 0;

  while (file.available()) {
    char c = file.read();

    if (c == '\n') {
      lines++;
    }
  }

  file.close();
  return lines;
}

// =====================================================
// Report file
// =====================================================

void writeReportFile() {
  removeFileIfExists(reportPath);

  File file = SD.open(reportPath, FILE_WRITE);

  if (!file) {
    Serial.println("Failed to create REPORT.TXT");
    return;
  }

  file.println("GeoScope v1.0 - SD Card Validation Report");
  file.println("=========================================");
  file.println();
  file.println("Status: SD card write/read test completed.");
  file.println("Module power: 5V");
  file.println();
  file.println("SPI wiring:");
  file.println("SCK  -> GPIO18");
  file.println("MISO -> GPIO19");
  file.println("MOSI -> GPIO23");
  file.println("CS   -> GPIO13");
  file.println();
  file.println("Files created:");
  file.println("/GEOSCOPE/REPORT.TXT");
  file.println("/GEOSCOPE/FIELDLOG.CSV");
  file.println("/GEOSCOPE/WAYPTS.CSV");
  file.println("/GEOSCOPE/SPEED.TXT");
  file.println();
  file.print("Millis at report creation: ");
  file.println(millis());
  file.println();
  file.println("If you can read this file on your computer,");
  file.println("GeoScope SD storage is working correctly.");

  file.close();

  Serial.println("REPORT.TXT written.");
}

// =====================================================
// Field log CSV
// =====================================================

void updateMockSensorData(int i) {
  lat += 0.000010;
  lon += 0.000015;
  altM += 0.2;

  tempC += 0.05;
  humidity += 0.12;
  pressureHpa += 0.03;

  pitchDeg += 0.2;
  rollDeg -= 0.15;

  headingDeg += 3.5;
  if (headingDeg >= 360.0) headingDeg -= 360.0;

  satellites = 8 + (i % 5);
}

void writeFieldLogCSV() {
  removeFileIfExists(fieldLogPath);

  File file = SD.open(fieldLogPath, FILE_WRITE);

  if (!file) {
    Serial.println("Failed to create FIELDLOG.CSV");
    return;
  }

  file.println("row,millis,lat,lon,alt_m,temp_c,humidity_percent,pressure_hpa,pitch_deg,roll_deg,heading_deg,satellites");

  for (int i = 0; i < FIELD_LOG_ROWS; i++) {
    updateMockSensorData(i);

    file.print(i);
    file.print(",");
    file.print(millis());
    file.print(",");
    file.print(lat, 6);
    file.print(",");
    file.print(lon, 6);
    file.print(",");
    file.print(altM, 1);
    file.print(",");
    file.print(tempC, 2);
    file.print(",");
    file.print(humidity, 2);
    file.print(",");
    file.print(pressureHpa, 2);
    file.print(",");
    file.print(pitchDeg, 2);
    file.print(",");
    file.print(rollDeg, 2);
    file.print(",");
    file.print(headingDeg, 2);
    file.print(",");
    file.println(satellites);

    delay(20);
  }

  file.close();

  Serial.println("FIELDLOG.CSV written.");
}

// =====================================================
// Waypoint CSV
// =====================================================

void writeWaypointCSV() {
  removeFileIfExists(waypointPath);

  File file = SD.open(waypointPath, FILE_WRITE);

  if (!file) {
    Serial.println("Failed to create WAYPTS.CSV");
    return;
  }

  file.println("id,name,lat,lon,alt_m,note");

  file.println("1,START,43.826100,-111.789700,1486.2,Mock starting point");
  file.println("2,WPT_001,43.826450,-111.789120,1487.4,Environmental sample point");
  file.println("3,WPT_002,43.826880,-111.788600,1489.1,Compass check point");
  file.println("4,RETURN,43.826100,-111.789700,1486.2,Return-to-start reference");

  file.close();

  Serial.println("WAYPTS.CSV written.");
}

// =====================================================
// Speed/write stress test
// =====================================================

void writeSpeedTestFile() {
  removeFileIfExists(speedPath);

  Serial.println("Writing SPEED.TXT stress test...");

  unsigned long startTime = millis();

  File file = SD.open(speedPath, FILE_WRITE);

  if (!file) {
    Serial.println("Failed to create SPEED.TXT");
    return;
  }

  for (int i = 0; i < SPEED_LINES; i++) {
    file.print("GeoScope SD speed line ");
    file.print(i);
    file.print(" | millis=");
    file.print(millis());
    file.print(" | sample=");
    file.print(i * 3.14159, 4);
    file.println(" | storage validation data");
  }

  file.close();

  unsigned long elapsed = millis() - startTime;

  Serial.print("SPEED.TXT written in ");
  Serial.print(elapsed);
  Serial.println(" ms.");
}

// =====================================================
// Directory listing
// =====================================================

void listDirectory(const char *dirname) {
  printDivider();

  Serial.print("LISTING DIRECTORY: ");
  Serial.println(dirname);

  File root = SD.open(dirname);

  if (!root) {
    Serial.println("Failed to open directory.");
    return;
  }

  if (!root.isDirectory()) {
    Serial.println("Not a directory.");
    root.close();
    return;
  }

  File file = root.openNextFile();

  while (file) {
    Serial.print("File: ");
    Serial.print(file.name());

    if (!file.isDirectory()) {
      Serial.print(" | Size: ");
      Serial.print(file.size());
      Serial.print(" bytes");
    }

    Serial.println();

    file = root.openNextFile();
  }

  root.close();
}

// =====================================================
// Validation
// =====================================================

void validateFiles() {
  printDivider();
  Serial.println("VALIDATION RESULTS");

  printFileSize(reportPath);
  printFileSize(fieldLogPath);
  printFileSize(waypointPath);
  printFileSize(speedPath);

  int fieldLines = countLines(fieldLogPath);
  int waypointLines = countLines(waypointPath);

  Serial.println();

  Serial.print("FIELDLOG.CSV line count: ");
  Serial.println(fieldLines);

  Serial.print("Expected FIELDLOG.CSV lines: ");
  Serial.println(FIELD_LOG_ROWS + 1);

  Serial.print("WAYPTS.CSV line count: ");
  Serial.println(waypointLines);

  Serial.print("Expected WAYPTS.CSV lines: ");
  Serial.println(5);

  Serial.println();

  if (fieldLines == FIELD_LOG_ROWS + 1 && waypointLines == 5) {
    Serial.println("Validation status: PASS");
  } else {
    Serial.println("Validation status: CHECK FILES");
  }
}

// =====================================================
// Setup
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("GeoScope advanced SD serial test starting...");

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  Serial.println("Initializing SD card...");

  if (!SD.begin(SD_CS, SPI, 1000000)) {
    Serial.println("SD.begin FAILED.");
    Serial.println("Check wiring, power, CS, card, and module.");
    return;
  }

  sdOK = true;

  Serial.println("SD.begin SUCCESS.");

  printCardInfo();

  createGeoScopeFolder();

  writeReportFile();
  writeFieldLogCSV();
  writeWaypointCSV();
  writeSpeedTestFile();

  listDirectory(folderPath);

  validateFiles();

  previewTextFile(reportPath, 20);
  previewTextFile(fieldLogPath, 10);
  previewTextFile(waypointPath, 10);
  previewTextFile(speedPath, 8);

  printDivider();
  Serial.println("Advanced SD test complete.");
  Serial.println("Remove the SD card and check the /GEOSCOPE folder on your computer.");
}

void loop() {
  // Nothing here.
}