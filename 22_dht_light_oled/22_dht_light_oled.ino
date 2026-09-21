#include <Wire.h>
#include <U8g2lib.h>

const int DHT_PIN = 14;
const int LIGHT_PIN = 33;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

bool waitForLevel(uint8_t level, unsigned long timeoutMicros) {
  unsigned long start = micros();
  while (digitalRead(DHT_PIN) != level) {
    if (micros() - start > timeoutMicros) return false;
  }
  return true;
}

bool readDHT11(int& temperature, int& humidity) {
  uint8_t data[5] = {0, 0, 0, 0, 0};
  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
  delay(20);
  digitalWrite(DHT_PIN, HIGH);
  delayMicroseconds(30);
  pinMode(DHT_PIN, INPUT_PULLUP);

  if (!waitForLevel(LOW, 100)) return false;
  if (!waitForLevel(HIGH, 100)) return false;
  if (!waitForLevel(LOW, 100)) return false;

  for (uint8_t bitIndex = 0; bitIndex < 40; bitIndex++) {
    if (!waitForLevel(HIGH, 100)) return false;
    unsigned long pulseStart = micros();
    if (!waitForLevel(LOW, 100)) return false;
    data[bitIndex / 8] <<= 1;
    if (micros() - pulseStart > 40) data[bitIndex / 8] |= 1;
  }

  if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4]) return false;
  humidity = data[0];
  temperature = data[2];
  return true;
}

void drawThermometer(int x, int y) {
  oled.drawFrame(x + 5, y, 6, 15);
  oled.drawDisc(x + 8, y + 18, 5);
  oled.drawLine(x + 8, y + 5, x + 8, y + 18);
  oled.drawLine(x + 12, y + 4, x + 15, y + 4);
  oled.drawLine(x + 12, y + 9, x + 14, y + 9);
  oled.drawLine(x + 12, y + 14, x + 15, y + 14);
}

void drawDroplet(int x, int y) {
  oled.drawLine(x + 8, y, x + 1, y + 11);
  oled.drawLine(x + 8, y, x + 15, y + 11);
  oled.drawCircle(x + 8, y + 11, 7);
  oled.drawLine(x + 1, y + 11, x + 2, y + 15);
  oled.drawLine(x + 15, y + 11, x + 14, y + 15);
  oled.drawLine(x + 2, y + 15, x + 14, y + 15);
}

void drawSun(int x, int y) {
  oled.drawCircle(x + 8, y + 8, 5);
  oled.drawLine(x + 8, y, x + 8, y + 3);
  oled.drawLine(x + 8, y + 13, x + 8, y + 16);
  oled.drawLine(x, y + 8, x + 3, y + 8);
  oled.drawLine(x + 13, y + 8, x + 16, y + 8);
  oled.drawLine(x + 2, y + 2, x + 4, y + 4);
  oled.drawLine(x + 12, y + 12, x + 14, y + 14);
  oled.drawLine(x + 12, y + 4, x + 14, y + 2);
  oled.drawLine(x + 2, y + 14, x + 4, y + 12);
}

void drawRow(int y, const char* label, int value, const char* unit, void (*icon)(int, int)) {
  icon(3, y + 1);
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(25, y + 9, label);

  char valueText[8];
  snprintf(valueText, sizeof(valueText), "%d", value);
  oled.setFont(u8g2_font_logisoso18_tn);
  int valueWidth = oled.getStrWidth(valueText);
  oled.drawStr(75 - valueWidth, y + 21, valueText);

  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(80, y + 17, unit);
}

void drawScreen(int temperature, int humidity, int lightPercent, bool dhtValid) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(25, 8, "ESP32 SENSOR");
  oled.drawHLine(25, 10, 100);

  drawRow(11, "TEMP", dhtValid ? temperature : 0, "C", drawThermometer);
  drawRow(32, "HUMI", dhtValid ? humidity : 0, "%", drawDroplet);
  drawRow(53, "LIGHT", lightPercent, "%", drawSun);

  oled.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_PIN, ADC_11db);
  Wire.begin(21, 22);
  oled.begin();
  drawScreen(0, 0, 0, false);
}

void loop() {
  int temperature = 0;
  int humidity = 0;
  int lightRaw = analogRead(LIGHT_PIN);
  int lightPercent = map(lightRaw, 0, 4095, 0, 100);
  lightPercent = constrain(lightPercent, 0, 100);
  bool dhtValid = readDHT11(temperature, humidity);

  if (dhtValid) {
    Serial.printf("Temperature: %d C, Humidity: %d %%, Light: %d %% (ADC %d)\n", temperature, humidity, lightPercent, lightRaw);
  } else {
    Serial.printf("DHT11 read failed, Light: %d %% (ADC %d)\n", lightPercent, lightRaw);
  }

  drawScreen(temperature, humidity, lightPercent, dhtValid);
  delay(1000);
}
