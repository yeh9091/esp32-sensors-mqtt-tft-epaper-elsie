/*
 * Waveshare 2.9-inch e-Paper (B) V4 / Rev2.1
 * ESP32 Dev Module
 *
 * Pin map:
 *   DIN  -> GPIO23
 *   CLK  -> GPIO18
 *   CS   -> GPIO27
 *   DC   -> GPIO26
 *   RST  -> GPIO25
 *   BUSY -> GPIO34
 *
 * Displays a three-color electronic shelf label inspired by the reference
 * photo.  The price is shown in New Taiwan dollars (NTD).
 *
 * The text is rendered with a 90-degree clockwise rotation so it appears
 * correctly on the physical 296 x 128 panel.
 */

#include <Arduino.h>
#include "epd2in9b_V4.h"
#include "moon_rabbit_3color.h"

static constexpr uint16_t BUFFER_SIZE = (EPD_WIDTH / 8) * EPD_HEIGHT;
static uint8_t blackImage[BUFFER_SIZE];
static uint8_t redImage[BUFFER_SIZE];
Epd epd;
constexpr int DHT_PIN = 14;
constexpr int LIGHT_PIN = 33;

// 5x7 font, one byte per row, left-to-right bits are the glyph columns.
const uint8_t FONT_5X7[26][7] = {
  {0b01110,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001}, // A
  {0b11110,0b10001,0b10001,0b11110,0b10001,0b10001,0b11110}, // B
  {0b01110,0b10001,0b10000,0b10000,0b10000,0b10001,0b01110}, // C
  {0b11110,0b10001,0b10001,0b10001,0b10001,0b10001,0b11110}, // D
  {0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b11111}, // E
  {0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b10000}, // F
  {0b01110,0b10001,0b10000,0b10111,0b10001,0b10001,0b01110}, // G
  {0b10001,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001}, // H
  {0b11111,0b00100,0b00100,0b00100,0b00100,0b00100,0b11111}, // I
  {0b00111,0b00010,0b00010,0b00010,0b10010,0b10010,0b01100}, // J
  {0b10001,0b10010,0b10100,0b11000,0b10100,0b10010,0b10001}, // K
  {0b10000,0b10000,0b10000,0b10000,0b10000,0b10000,0b11111}, // L
  {0b10001,0b11011,0b10101,0b10101,0b10001,0b10001,0b10001}, // M
  {0b10001,0b11001,0b10101,0b10011,0b10001,0b10001,0b10001}, // N
  {0b01110,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110}, // O
  {0b11110,0b10001,0b10001,0b11110,0b10000,0b10000,0b10000}, // P
  {0b01110,0b10001,0b10001,0b10001,0b10101,0b10010,0b01101}, // Q
  {0b11110,0b10001,0b10001,0b11110,0b10100,0b10010,0b10001}, // R
  {0b01111,0b10000,0b10000,0b01110,0b00001,0b00001,0b11110}, // S
  {0b11111,0b00100,0b00100,0b00100,0b00100,0b00100,0b00100}, // T
  {0b10001,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110}, // U
  {0b10001,0b10001,0b10001,0b10001,0b10001,0b01010,0b00100}, // V
  {0b10001,0b10001,0b10001,0b10101,0b10101,0b11011,0b10001}, // W
  {0b10001,0b10001,0b01010,0b00100,0b01010,0b10001,0b10001}, // X
  {0b10001,0b10001,0b01010,0b00100,0b00100,0b00100,0b00100}, // Y
  {0b11111,0b00001,0b00010,0b00100,0b01000,0b10000,0b11111}  // Z
};

const uint8_t DIGIT_5X7[10][7] = {
  {0b01110,0b10001,0b10011,0b10101,0b11001,0b10001,0b01110}, // 0
  {0b00100,0b01100,0b00100,0b00100,0b00100,0b00100,0b01110}, // 1
  {0b01110,0b10001,0b00001,0b00010,0b00100,0b01000,0b11111}, // 2
  {0b11110,0b00001,0b00001,0b01110,0b00001,0b00001,0b11110}, // 3
  {0b00010,0b00110,0b01010,0b10010,0b11111,0b00010,0b00010}, // 4
  {0b11111,0b10000,0b10000,0b11110,0b00001,0b00001,0b11110}, // 5
  {0b00110,0b01000,0b10000,0b11110,0b10001,0b10001,0b01110}, // 6
  {0b11111,0b00001,0b00010,0b00100,0b01000,0b01000,0b01000}, // 7
  {0b01110,0b10001,0b10001,0b01110,0b10001,0b10001,0b01110}, // 8
  {0b01110,0b10001,0b10001,0b01111,0b00001,0b00010,0b11100}  // 9
};

const uint8_t SYMBOL_5X7[3][7] = {
  {0b00000,0b00000,0b00000,0b11111,0b00000,0b00000,0b00000}, // -
  {0b00000,0b00100,0b00100,0b00000,0b00100,0b00100,0b00000}, // :
  {0b00000,0b00000,0b00000,0b00000,0b00000,0b00100,0b00100}  // .
};

void setPixelLogical(int x, int y, bool black, bool red) {
  if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT) return;
  const uint16_t index = y * (EPD_WIDTH / 8) + x / 8;
  const uint8_t mask = 0x80 >> (x % 8);
  if (black) blackImage[index] &= ~mask;
  else blackImage[index] |= mask;
  if (red) redImage[index] &= ~mask;
  else redImage[index] |= mask;
}

// The controller buffer is 128 x 296 while the panel is 296 x 128.
// Map a desired physical pixel through a 90-degree clockwise rotation.
void setPixelPhysical(int x, int y, bool black, bool red) {
  setPixelLogical(y, EPD_HEIGHT - 1 - x, black, red);
}

const uint8_t* glyphFor(char c) {
  if (c >= 'A' && c <= 'Z') return FONT_5X7[c - 'A'];
  if (c >= '0' && c <= '9') return DIGIT_5X7[c - '0'];
  if (c == '-') return SYMBOL_5X7[0];
  if (c == ':') return SYMBOL_5X7[1];
  if (c == '.') return SYMBOL_5X7[2];
  return nullptr;
}

void drawText(const char* text, int x, int y, int scale, bool black, bool red) {
  constexpr int glyphWidth = 5;
  constexpr int glyphHeight = 7;
  constexpr int gap = 1;
  for (int letter = 0; text[letter] != '\0'; ++letter) {
    const uint8_t* glyph = glyphFor(text[letter]);
    if (glyph == nullptr) continue;
    const int letterX = x + letter * (glyphWidth + gap) * scale;
    for (int row = 0; row < glyphHeight; ++row) {
      for (int col = 0; col < glyphWidth; ++col) {
        if ((glyph[row] >> (glyphWidth - 1 - col)) & 0x01) {
          for (int dy = 0; dy < scale; ++dy) {
            for (int dx = 0; dx < scale; ++dx) {
              setPixelPhysical(letterX + col * scale + dx, y + row * scale + dy,
                               black, red);
            }
          }
        }
      }
    }
  }
}

void drawCenteredText(const char* text, int y, int scale, bool black, bool red) {
  constexpr int panelWidth = 296;
  constexpr int glyphWidth = 5;
  constexpr int gap = 1;
  const int length = strlen(text);
  const int totalWidth = length * glyphWidth * scale + max(0, length - 1) * gap * scale;
  drawText(text, (panelWidth - totalWidth) / 2, y, scale, black, red);
}

void drawCenteredTextInPanel(const char* text, int panelX, int panelWidth,
                             int y, int scale, bool black, bool red) {
  constexpr int glyphWidth = 5;
  constexpr int gap = 1;
  const int length = strlen(text);
  const int totalWidth = length * glyphWidth * scale + max(0, length - 1) * gap * scale;
  drawText(text, panelX + (panelWidth - totalWidth) / 2, y, scale, black, red);
}

bool readDHT11(int& temperature, int& humidity) {
  uint8_t data[5] = {0};

  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
  delay(20);
  digitalWrite(DHT_PIN, HIGH);
  delayMicroseconds(30);
  pinMode(DHT_PIN, INPUT_PULLUP);

  auto waitLevel = [](uint8_t level, unsigned long timeoutUs) {
    const unsigned long start = micros();
    while (digitalRead(DHT_PIN) != level) {
      if (micros() - start > timeoutUs) return false;
    }
    return true;
  };

  if (!waitLevel(LOW, 100) || !waitLevel(HIGH, 100) || !waitLevel(LOW, 100)) {
    return false;
  }
  for (int i = 0; i < 40; ++i) {
    if (!waitLevel(HIGH, 100)) return false;
    const unsigned long start = micros();
    if (!waitLevel(LOW, 100)) return false;
    data[i / 8] <<= 1;
    if (micros() - start > 40) data[i / 8] |= 1;
  }
  if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4]) return false;
  humidity = data[0];
  temperature = data[2];
  return true;
}

void drawThermometerIcon(int cx, int cy, bool black, bool red) {
  for (int y = -18; y <= 8; ++y) {
    for (int x = -4; x <= 4; ++x) {
      const bool tube = (abs(x) <= 2 && y <= 5);
      const bool bulb = (x * x + (y - 9) * (y - 9) <= 36);
      const bool outline = (abs(x) == 4 && y <= 5) || (x * x + (y - 9) * (y - 9) >= 25);
      if (tube || bulb || outline) setPixelPhysical(cx + x, cy + y, black, red);
    }
  }
}

void drawDropIcon(int cx, int cy, bool black, bool red) {
  for (int y = -18; y <= 10; ++y) {
    const int width = (y < 0) ? max(1, 2 + (y + 18) / 2) : 10;
    for (int x = -width; x <= width; ++x) {
      if (y < 0 || x * x + (y - 2) * (y - 2) <= 100) {
        setPixelPhysical(cx + x, cy + y, black, red);
      }
    }
  }
}

void drawSunIcon(int cx, int cy, bool black, bool red) {
  for (int y = -8; y <= 8; ++y) {
    for (int x = -8; x <= 8; ++x) {
      if (x * x + y * y <= 64) setPixelPhysical(cx + x, cy + y, black, red);
    }
  }
  for (int i = 0; i < 8; ++i) {
    const float a = i * 0.785398f;
    const int x0 = cx + (int)(cos(a) * 13);
    const int y0 = cy + (int)(sin(a) * 13);
    const int x1 = cx + (int)(cos(a) * 20);
    const int y1 = cy + (int)(sin(a) * 20);
    for (int j = 0; j <= 20; ++j) {
      const int x = x0 + (x1 - x0) * j / 20;
      const int y = y0 + (y1 - y0) * j / 20;
      setPixelPhysical(x, y, black, red);
    }
  }
}

void drawDegreeSymbol(int cx, int cy, bool black, bool red) {
  setPixelPhysical(cx, cy - 1, black, red);
  setPixelPhysical(cx - 1, cy, black, red);
  setPixelPhysical(cx + 1, cy, black, red);
  setPixelPhysical(cx, cy + 1, black, red);
}

void fillPhysicalRect(int x, int y, int width, int height, bool black, bool red) {
  for (int py = y; py < y + height; ++py) {
    for (int px = x; px < x + width; ++px) {
      setPixelPhysical(px, py, black, red);
    }
  }
}

void drawBarcode(int x, int y, int width, int height) {
  // A decorative shelf-label barcode; it is intentionally not a scannable SKU.
  const uint8_t pattern[] = {2,1,1,3,1,2,2,1,3,1,1,2,1,3,2,1,1,2,3,1,2,1,1,3};
  int cursor = x;
  for (uint8_t i = 0; i < sizeof(pattern); ++i) {
    const int barWidth = pattern[i];
    fillPhysicalRect(cursor, y, barWidth, height, true, false);
    cursor += barWidth + 1;
    if (cursor >= x + width) break;
  }
}

void drawMooncakeIcon(int cx, int cy) {
  // Mooncake illustration based on the supplied reference:
  // pale cake body, scalloped flower top, red embossed lines and a
  // traditional square centre stamp.  It is deliberately drawn with
  // the three colours available on the e-paper instead of using a
  // photographic bitmap that would lose detail after dithering.

  // Lower cake body with a gently scalloped bottom edge.
  for (int y = -8; y <= 22; ++y) {
    const int halfWidth = 31 - max(0, y - 12) / 3;
    for (int x = -halfWidth; x <= halfWidth; ++x) {
      setPixelPhysical(cx + x, cy + y, false, false);
    }
  }
  for (int scallop = -4; scallop <= 4; ++scallop) {
    const int sx = cx + scallop * 14;
    for (int y = 13; y <= 24; ++y) {
      for (int x = -8; x <= 8; ++x) {
        if (x * x + (y - 13) * (y - 13) <= 100) {
          setPixelPhysical(sx + x, cy + y, false, false);
        }
      }
    }
  }

  // Vertical mould grooves on the cake side.
  for (int groove = -3; groove <= 3; ++groove) {
    const int gx = cx + groove * 12;
    for (int y = 3; y <= 20; ++y) {
      setPixelPhysical(gx, cy + y, true, false);
      if (y > 8) setPixelPhysical(gx + (groove & 1), cy + y, false, true);
    }
  }

  // Flower-shaped top: eight broad petals with a red embossed outline.
  for (int petal = 0; petal < 8; ++petal) {
    const float a = petal * 0.785398f;
    const int px = cx + (int)(cos(a) * 23);
    const int py = cy - 8 + (int)(sin(a) * 13);
    for (int dy = -7; dy <= 7; ++dy) {
      for (int dx = -11; dx <= 11; ++dx) {
        const float e = (dx * dx) / 121.0f + (dy * dy) / 49.0f;
        if (e <= 1.0f) setPixelPhysical(px + dx, py + dy, false, false);
        if (e > 0.70f && e < 1.0f) setPixelPhysical(px + dx, py + dy, false, true);
      }
    }
  }
  for (int y = -23; y <= 8; ++y) {
    for (int x = -34; x <= 34; ++x) {
      const float outer = (x * x) / 1156.0f + ((y + 8) * (y + 8)) / 529.0f;
      const float inner = (x * x) / 1024.0f + ((y + 8) * (y + 8)) / 441.0f;
      if (outer <= 1.0f && outer >= 0.84f && inner < 1.05f) {
        setPixelPhysical(cx + x, cy + y, false, true);
      }
    }
  }

  // Inner raised rim.
  for (int y = -18; y <= 4; ++y) {
    for (int x = -28; x <= 28; ++x) {
      const float e = (x * x) / 784.0f + ((y + 7) * (y + 7)) / 324.0f;
      if (e > 0.80f && e < 1.03f) setPixelPhysical(cx + x, cy + y, false, true);
    }
  }

  // Square centre stamp, echoing the 中秋 mark in the reference.
  for (int i = -12; i <= 12; ++i) {
    setPixelPhysical(cx + i, cy - 12, false, true);
    setPixelPhysical(cx + i, cy + 12, false, true);
    setPixelPhysical(cx - 12, cy + i, false, true);
    setPixelPhysical(cx + 12, cy + i, false, true);
  }
  for (int i = -8; i <= 8; ++i) {
    setPixelPhysical(cx + i, cy - 4, false, true);
    setPixelPhysical(cx + i, cy + 4, false, true);
    setPixelPhysical(cx - 8, cy - 4 + i, false, true);
    setPixelPhysical(cx + 8, cy - 4 + i, false, true);
  }
  for (int i = -6; i <= 6; ++i) {
    setPixelPhysical(cx - 2, cy - 1 + i, false, true);
    setPixelPhysical(cx + 2, cy - 1 + i, false, true);
  }
  for (int i = -5; i <= 5; ++i) setPixelPhysical(cx + i, cy + 2, false, true);
}

void drawMoonRabbitImage(int x, int y) {
  // Two-bit packed black/white/red image converted from the supplied
  // rabbit-and-mooncake illustration.  The image is intentionally
  // reduced to the colours supported by the Waveshare 2.9-inch panel.
  for (uint16_t py = 0; py < MOON_RABBIT_H; ++py) {
    for (uint16_t px = 0; px < MOON_RABBIT_W; ++px) {
      const uint32_t pixelIndex = (uint32_t)py * MOON_RABBIT_W + px;
      const uint8_t packed = pgm_read_byte(&MOON_RABBIT_3COLOR[pixelIndex / 4]);
      const uint8_t color = (packed >> ((3 - (pixelIndex % 4)) * 2)) & 0x03;
      if (color == 1) {
        setPixelPhysical(x + px, y + py, true, false);
      } else if (color == 2) {
        setPixelPhysical(x + px, y + py, false, true);
      } else {
        setPixelPhysical(x + px, y + py, false, false);
      }
    }
  }
}

void drawShelfLabel() {
  memset(blackImage, 0xFF, sizeof(blackImage));
  memset(redImage, 0xFF, sizeof(redImage));

  constexpr int redPanelWidth = 188;
  constexpr int blackPanelX = 188;
  constexpr int blackPanelWidth = 108;

  // Red product panel and black price panel, matching the reference layout.
  fillPhysicalRect(0, 0, redPanelWidth, 128, false, true);
  fillPhysicalRect(blackPanelX, 0, blackPanelWidth, 128, true, false);

  // The supplied rabbit-and-mooncake illustration, converted to
  // black/white/red for the three-colour e-paper display.
  drawMoonRabbitImage(4, 2);
  drawBarcode(11, 111, 164, 13);

  // Price information on the black panel, in New Taiwan dollars.
  drawCenteredTextInPanel("NOW", blackPanelX, blackPanelWidth, 8, 2, false, false);
  drawCenteredTextInPanel("NTD 399", blackPanelX, blackPanelWidth, 30, 2, false, false);
  drawCenteredTextInPanel("SAVE", blackPanelX, blackPanelWidth, 62, 2, false, false);
  drawCenteredTextInPanel("NTD 50", blackPanelX, blackPanelWidth, 84, 2, false, false);
}

void drawMetricPage(int page, int temperature, int humidity, int light, bool dhtValid) {
  memset(blackImage, 0xFF, sizeof(blackImage));
  memset(redImage, 0xFF, sizeof(redImage));

  // One metric per page: icon, red label, black value.
  const int iconX = 47;
  if (page == 0) {
    drawThermometerIcon(iconX, 64, false, true);
    drawCenteredText("TEMP", 28, 3, false, true);
    char value[8];
    snprintf(value, sizeof(value), dhtValid ? "%dC" : "--C", temperature);
    drawCenteredText(value, 75, 4, true, false);
    if (dhtValid) {
      const int valueWidth = strlen(value) * 6 * 4 - 4;
      drawDegreeSymbol((296 - valueWidth) / 2 + valueWidth - 12, 73, true, false);
    }
  } else if (page == 1) {
    drawDropIcon(iconX, 64, false, true);
    drawCenteredText("HUMI", 28, 3, false, true);
    char value[8];
    snprintf(value, sizeof(value), dhtValid ? "%d%%" : "--%%", humidity);
    drawCenteredText(value, 75, 4, true, false);
  } else {
    drawSunIcon(iconX, 64, false, true);
    drawCenteredText("LIGHT", 28, 3, false, true);
    char value[8];
    snprintf(value, sizeof(value), "%d%%", light);
    drawCenteredText(value, 75, 4, true, false);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("36_epaper: electronic shelf label / NTD price");

  if (epd.Init() != 0) {
    Serial.println("e-Paper init failed");
    return;
  }

  drawShelfLabel();

  Serial.println("Refreshing e-paper; this can take about 12-15 seconds...");
  epd.Display(blackImage, redImage);
  epd.Sleep();
  Serial.println("Display complete");
}

void loop() {
  // The shelf label is static, so leave the panel asleep after the refresh.
  delay(1000);
}
