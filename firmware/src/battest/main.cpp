// Battery gauge check: does the MAX17048 report 0 V because the reading needs
// time after power-up, or always? Prints raw registers too, in case the driver
// reads fine but the values are genuinely zero.

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MAX1704X.h>

Adafruit_MAX17048 maxlipo;

uint16_t reg16(uint8_t reg) {
  Wire.beginTransmission(0x36);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0xFFFF;
  if (Wire.requestFrom(0x36, (uint8_t)2) != 2) return 0xFFFF;
  uint16_t hi = Wire.read();
  return (hi << 8) | Wire.read();
}

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 5000) delay(10);
  delay(500);

  pinMode(PIN_I2C_POWER, OUTPUT);
  digitalWrite(PIN_I2C_POWER, HIGH);
  delay(10);
  Wire.begin();
  Serial.printf("MAX17048 begin: %s\n", maxlipo.begin() ? "ok" : "NOT FOUND");
  Serial.printf("chip id 0x%x, version 0x%x\n", maxlipo.getChipID(), reg16(0x08));
}

void loop() {
  static int n = 0;
  // VCELL 0x02 (78.125 uV/cell LSB), SOC 0x04, MODE 0x06, CONFIG 0x0C, STATUS 0x1A
  Serial.printf("%2d  %6.3f V  %5.1f %%   VCELL 0x%04x  SOC 0x%04x  CONFIG 0x%04x  STATUS 0x%04x\n",
                n, maxlipo.cellVoltage(), maxlipo.cellPercent(),
                reg16(0x02), reg16(0x04), reg16(0x0C), reg16(0x1A));
  if (n == 10) {
    Serial.println("-- quickStart() --");
    maxlipo.quickStart();
  }
  n++;
  delay(1000);
}
