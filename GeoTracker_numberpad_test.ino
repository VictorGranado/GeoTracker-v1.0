#include <Wire.h>

#define SDA_PIN      21
#define SCL_PIN      22
#define KEYPAD_ADDR  0x65
#define STATUS_REG   0x08

// Confirmed NULLLAB keypad mapping by active bit
const char keyByBit[16] = {
  '1', '4', '7', '*',
  '2', '5', '8', '0',
  '3', '6', '9', '#',
  'A', 'B', 'C', 'D'
};

uint16_t lastState = 0;

bool readKeypad(uint16_t &state) {
  Wire.beginTransmission(KEYPAD_ADDR);
  Wire.write(STATUS_REG);

  if (Wire.endTransmission() != 0) {
    return false;
  }

  if (Wire.requestFrom(KEYPAD_ADDR, (uint8_t)2) != 2) {
    return false;
  }

  uint8_t lowByte  = Wire.read();
  uint8_t highByte = Wire.read();

  state = ((uint16_t)highByte << 8) | lowByte;

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  Serial.println();
  Serial.println("==============================");
  Serial.println(" GeoScope NULLLAB Keypad Test");
  Serial.println("==============================");
  Serial.println();
  Serial.println("Press a key...");
}

void loop() {
  uint16_t state;

  if (!readKeypad(state)) {
    Serial.println("I2C read failed");
    delay(500);
    return;
  }

  // Only react when the state changes
  if (state != lastState) {

    // Key press
    if (state != 0) {

      for (int bit = 0; bit < 16; bit++) {
        if (state & (1U << bit)) {

          Serial.print("Key pressed: ");
          Serial.println(keyByBit[bit]);
        }
      }

    // All keys released
    } else if (lastState != 0) {

      Serial.println("Key released");
    }

    lastState = state;
  }

  delay(20);
}