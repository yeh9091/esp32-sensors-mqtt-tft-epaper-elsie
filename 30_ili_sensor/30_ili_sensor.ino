#include <SPI.h>

// ILI9225 SPI（與 29_ili_color 相同）
const int TFT_SCK = 18, TFT_MOSI = 23, TFT_CS = 5, TFT_DC = 27, TFT_RST = 26;
const int TFT_WIDTH = 176, TFT_HEIGHT = 220;
const int DHT_PIN = 14, LIGHT_PIN = 33;

const uint16_t NAVY = 0x10A2, PANEL = 0x18E3, WHITE = 0xFFFF;
const uint16_t RED = 0xF800, CYAN = 0x07FF, YELLOW = 0xFFE0;
const uint16_t GREEN = 0x07E0, GRAY = 0xBDF7, DARK = 0x0841;

void command(uint16_t v) {
  digitalWrite(TFT_DC, LOW); digitalWrite(TFT_CS, LOW);
  SPI.transfer(v >> 8); SPI.transfer(v); digitalWrite(TFT_CS, HIGH);
}
void data16(uint16_t v) {
  digitalWrite(TFT_DC, HIGH); digitalWrite(TFT_CS, LOW);
  SPI.transfer(v >> 8); SPI.transfer(v); digitalWrite(TFT_CS, HIGH);
}
void reg16(uint16_t r, uint16_t v) { command(r); data16(v); }

void iliInit() {
  digitalWrite(TFT_RST, LOW); delay(50); digitalWrite(TFT_RST, HIGH); delay(50);
  reg16(0x28, 0x00CE); reg16(0x01, 0x011C); reg16(0x03, 0x1030);
  reg16(0x07, 0x0017); delay(50); reg16(0x11, 0x1000);
  reg16(0x20, 0); reg16(0x21, 0); delay(50);
  reg16(0x30, 0); reg16(0x31, 0x00DB); reg16(0x32, 0);
  reg16(0x33, 0); reg16(0x34, 0x00DB); reg16(0x35, 0);
  reg16(0x36, 0x00AF); reg16(0x37, 0); reg16(0x38, 0x00DB); reg16(0x39, 0);
  delay(50); reg16(0x02, 0); reg16(0x61, 0x0103); reg16(0xE8, 0x1000);
  reg16(0xB0, 0x0812); reg16(0x0B, 0); reg16(0x0D, 0);
  reg16(0xB1, 0x0404); reg16(0x62, 0x0019); delay(50); reg16(0xE8, 0x0100);
  reg16(0x50, 7); reg16(0x51, 0x0708); reg16(0x52, 0x0D0A);
  reg16(0x53, 0x0404); reg16(0x54, 7); reg16(0x55, 0x0706);
  reg16(0x56, 0x0D0A); reg16(0x57, 0x0404); reg16(0x58, 0); reg16(0x59, 0);
  reg16(0x20, 0); reg16(0x21, 0); reg16(0x07, 0x1017); delay(80);
}

void window(int x0, int y0, int x1, int y1) {
  reg16(0x36, x1); reg16(0x37, x0); reg16(0x38, y1); reg16(0x39, y0);
  reg16(0x20, x0); reg16(0x21, y0); command(0x22);
}
void fillRect(int x, int y, int w, int h, uint16_t color) {
  if (w <= 0 || h <= 0) return;
  window(x, y, x + w - 1, y + h - 1);
  digitalWrite(TFT_DC, HIGH); digitalWrite(TFT_CS, LOW);
  for (long i = 0; i < (long)w * h; i++) { SPI.transfer(color >> 8); SPI.transfer(color); }
  digitalWrite(TFT_CS, HIGH);
}
void pixel(int x, int y, uint16_t color) {
  if (x < 0 || x >= TFT_WIDTH || y < 0 || y >= TFT_HEIGHT) return;
  window(x, y, x, y); data16(color);
}
void hLine(int x, int y, int w, uint16_t c) { fillRect(x, y, w, 1, c); }
void vLine(int x, int y, int h, uint16_t c) { fillRect(x, y, 1, h, c); }
void circle(int cx, int cy, int r, uint16_t c) {
  for (int y = -r; y <= r; y++) for (int x = -r; x <= r; x++)
    if (x * x + y * y <= r * r) pixel(cx + x, cy + y, c);
}

// 5x7 點陣字型；所有字元固定同一字格大小
const uint8_t font[][7] = {
  {0x1F,0x11,0x11,0x1F,0x11,0x11,0x1F}, // A
  {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, // B
  {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // C
  {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}, // D
  {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // E
  {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // F
  {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}, // G
  {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // H
  {0x1F,0x04,0x04,0x04,0x04,0x04,0x1F}, // I
  {0x01,0x01,0x01,0x01,0x11,0x11,0x0E}, // J
  {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // K
  {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // L
  {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // M
  {0x11,0x19,0x15,0x13,0x11,0x11,0x11}, // N
  {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // O
  {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // P
  {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // Q
  {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // R
  {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, // S
  {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // T
  {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // U
  {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}, // V
  {0x11,0x11,0x11,0x15,0x15,0x1B,0x11}, // W
  {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // X
  {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}, // Y
  {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}, // Z
  {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
  {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
  {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, // 2
  {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}, // 3
  {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
  {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}, // 5
  {0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E}, // 6
  {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
  {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
  {0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}  // 9
};
int glyphIndex(char ch) { if (ch >= 'A' && ch <= 'Z') return ch - 'A'; return 26 + ch - '0'; }
void text(int x, int y, const char* s, int scale, uint16_t color) {
  while (*s) {
    char ch = *s++;
    if (ch == ' ') { x += 6 * scale; continue; }
    if (ch == '%') {
      circle(x + 1 * scale, y + 2 * scale, scale, color);
      circle(x + 4 * scale, y + 5 * scale, scale, color);
      for (int i = 0; i < 6 * scale; i++) pixel(x + 5 * scale - i, y + i, color);
      x += 6 * scale; continue;
    }
    int gi = glyphIndex(ch);
    for (int row = 0; row < 7; row++) for (int col = 0; col < 5; col++)
      if ((font[gi][row] >> (4 - col)) & 1)
        for (int sy = 0; sy < scale; sy++) for (int sx = 0; sx < scale; sx++)
          // 面板字形上下方向校正
          pixel(x + col * scale + sx, y + (7 * scale - 1 - row * scale - sy), color);
    x += 6 * scale;
  }
}

void thermometer(int cx, int cy) {
  circle(cx, cy + 13, 8, RED); fillRect(cx - 4, cy - 12, 8, 27, RED);
  fillRect(cx - 2, cy - 9, 4, 20, WHITE); circle(cx, cy + 13, 4, RED);
}
void droplet(int cx, int cy) {
  for (int y = -14; y <= 10; y++) for (int x = -10; x <= 10; x++) {
    bool inside = (y < 0) ? (x * x + (y + 4) * (y + 4) <= 12 * 12 && y <= 3)
                          : (x * x + (y - 3) * (y - 3) <= 9 * 9);
    if (inside) pixel(cx + x, cy + y, CYAN);
  }
}
void sun(int cx, int cy) {
  circle(cx, cy, 9, YELLOW);
  for (int i = 0; i < 8; i++) { float a = i * 0.7854; int dx = (int)(cos(a) * 15), dy = (int)(sin(a) * 15); hLine(cx + dx - 2, cy + dy, 5, YELLOW); }
}

bool readDHT(int& temperature, int& humidity) {
  uint8_t d[5] = {0}; pinMode(DHT_PIN, OUTPUT); digitalWrite(DHT_PIN, LOW); delay(20);
  digitalWrite(DHT_PIN, HIGH); delayMicroseconds(30); pinMode(DHT_PIN, INPUT_PULLUP);
  auto waitLevel = [](uint8_t level, unsigned long timeout) { unsigned long t = micros(); while (digitalRead(DHT_PIN) != level) if (micros() - t > timeout) return false; return true; };
  if (!waitLevel(LOW, 100) || !waitLevel(HIGH, 100) || !waitLevel(LOW, 100)) return false;
  for (int i = 0; i < 40; i++) { if (!waitLevel(HIGH, 100)) return false; unsigned long t = micros(); if (!waitLevel(LOW, 100)) return false; d[i / 8] <<= 1; if (micros() - t > 40) d[i / 8] |= 1; }
  if ((uint8_t)(d[0] + d[1] + d[2] + d[3]) != d[4]) return false;
  humidity = d[0]; temperature = d[2]; return true;
}

void drawStaticUI() {
  fillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, NAVY);
  fillRect(0, 0, TFT_WIDTH, 24, DARK); text(35, 5, "ENV MONITOR", 1, WHITE);

  // 三個等高資訊區：每區 64 像素
  fillRect(8, 30, 160, 60, PANEL); fillRect(8, 30, 6, 60, RED); thermometer(27, 58);
  fillRect(8, 94, 160, 60, PANEL); fillRect(8, 94, 6, 60, CYAN); droplet(27, 122);
  fillRect(8, 158, 160, 60, PANEL); fillRect(8, 158, 6, 60, YELLOW); sun(27, 187);

  text(48, 35, "TEMP", 1, GRAY);
  text(48, 99, "HUMID", 1, GRAY);
  text(48, 163, "LIGHT", 1, GRAY);
}

void drawValues(int tempValue, int humiValue, int lightValue, bool valid) {
  // 只清除數值區，固定背景與圖示不重畫，可大幅降低閃爍。
  fillRect(48, 51, 112, 35, PANEL);
  fillRect(48, 115, 112, 35, PANEL);
  fillRect(48, 179, 112, 35, PANEL);
  char t[4], h[4], l[4];
  snprintf(t, sizeof(t), "%02d", valid ? tempValue : 0);
  snprintf(h, sizeof(h), "%02d", valid ? humiValue : 0);
  snprintf(l, sizeof(l), "%03d", lightValue);
  text(48, 51, t, 4, WHITE); text(124, 61, "C", 2, RED);
  text(48, 115, h, 4, WHITE); text(124, 125, "%", 2, CYAN);
  text(48, 179, l, 4, WHITE); text(124, 189, "%", 2, YELLOW);
}

void setup() {
  Serial.begin(115200); pinMode(TFT_CS, OUTPUT); pinMode(TFT_DC, OUTPUT); pinMode(TFT_RST, OUTPUT); digitalWrite(TFT_CS, HIGH);
  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS); SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0)); iliInit(); SPI.endTransaction();
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0)); drawStaticUI(); SPI.endTransaction();
}
void loop() {
  int temperature = 0, humidity = 0; int raw = analogRead(LIGHT_PIN);
  int light = constrain(map(raw, 0, 4095, 0, 100), 0, 100); bool valid = readDHT(temperature, humidity);
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0)); drawValues(temperature, humidity, light, valid); SPI.endTransaction();
  Serial.printf("T:%dC H:%d%% L:%d%% %s\n", temperature, humidity, light, valid ? "OK" : "DHT FAIL"); delay(30000);
}
