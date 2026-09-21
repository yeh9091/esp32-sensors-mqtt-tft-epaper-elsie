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
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "epd2in9b_V4.h"
#include "moon_rabbit_3color.h"

static constexpr uint16_t BUFFER_SIZE = (EPD_WIDTH / 8) * EPD_HEIGHT;
static uint8_t blackImage[BUFFER_SIZE];
static uint8_t redImage[BUFFER_SIZE];
Epd epd;
constexpr int DHT_PIN = 14;
constexpr int LIGHT_PIN = 33;
constexpr int LED_G_PIN = 15;
constexpr int LED_Y_PIN = 2;
constexpr int LED_R_PIN = 4;

 const char* WIFI_SSID = "YOUR_WIFI_SSID";
 const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* MQTT_HOST = "mqttgo.io";
constexpr uint16_t MQTT_PORT = 1883;
const char* MQTT_DATA_TOPIC = "elsie/class305/data";
const char* MQTT_CTRL_TOPIC = "elsie/class305/ctrl";

WiFiClient mqttNetworkClient;
PubSubClient mqttClient(mqttNetworkClient);
String mqttClientId;
unsigned long lastMqttAttempt = 0;
int currentTemperature = 0;
int currentHumidity = 0;
int currentLight = 0;
bool currentDhtValid = false;
bool currentLightValid = false;

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

const uint8_t SYMBOL_5X7[4][7] = {
  {0b00000,0b00000,0b00000,0b11111,0b00000,0b00000,0b00000}, // -
  {0b00000,0b00100,0b00100,0b00000,0b00100,0b00100,0b00000}, // :
  {0b00000,0b00000,0b00000,0b00000,0b00000,0b00100,0b00100}, // .
  {0b11000,0b11001,0b00010,0b00100,0b01000,0b10011,0b00011}  // %
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
  if (c == '%') return SYMBOL_5X7[3];
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

void drawBoldCenteredTextInPanel(const char* text, int panelX, int panelWidth,
                                 int y, int scale, bool black, bool red) {
  constexpr int glyphWidth = 5;
  constexpr int gap = 1;
  const int length = strlen(text);
  const int totalWidth = length * glyphWidth * scale + max(0, length - 1) * gap * scale;
  const int x = panelX + (panelWidth - totalWidth) / 2;
  drawText(text, x, y, scale, black, red);
  drawText(text, x + 1, y, scale, black, red);
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
  // Treat an all-zero response as an invalid/no-sensor reading.  This
  // prevents the display from showing 0C and 0% when the DHT11 did not
  // actually provide a valid measurement.
  if (temperature == 0 && humidity == 0) return false;
  return true;
}

void drawThermometerIcon(int cx, int cy, bool black, bool red) {
  // Solid red thermometer, closer to the supplied reference photo:
  // a filled bulb and stem with a narrow white tube/highlight.
  for (int y = -22; y <= 8; ++y) {
    for (int x = -6; x <= 6; ++x) {
      setPixelPhysical(cx + x, cy + y, false, red);
    }
  }
  for (int y = 5; y <= 19; ++y) {
    for (int x = -10; x <= 10; ++x) {
      if (x * x + (y - 12) * (y - 12) <= 100) {
        setPixelPhysical(cx + x, cy + y, false, red);
      }
    }
  }

  // White vertical tube makes the red silhouette read as a thermometer.
  for (int y = -18; y <= 8; ++y) {
    for (int x = -2; x <= 2; ++x) {
      setPixelPhysical(cx + x, cy + y, false, false);
    }
  }
  // Red liquid remains visible in the lower part of the white tube.
  for (int y = 0; y <= 8; ++y) {
    for (int x = -2; x <= 2; ++x) {
      setPixelPhysical(cx + x, cy + y, false, red);
    }
  }

  // Red measurement marks at the left, with alternating lengths.
  for (int mark = -18, index = 0; mark <= 4; mark += 6, ++index) {
    const int length = (index % 2 == 0) ? 8 : 5;
    for (int x = -14; x <= -14 + length; ++x) {
      setPixelPhysical(cx + x, cy + mark, false, red);
    }
  }
}

void drawDropIcon(int cx, int cy, bool black, bool red) {
  // Solid red teardrop, matching the supplied reference:
  // pointed top, rounded lower body, and a white curved highlight.
  for (int y = -25; y <= 24; ++y) {
    int width;
    if (y < -3) {
      width = max(1, (y + 25) * 17 / 22);
    } else {
      const int dy = y - 8;
      width = (int)sqrt(max(0, 18 * 18 - dy * dy));
    }
    for (int x = -width; x <= width; ++x) {
      setPixelPhysical(cx + x, cy + y, false, red);
    }
  }

  // White highlight on the right side of the drop.
  for (int y = -6; y <= 13; ++y) {
    const int offset = (y + 6) / 2;
    for (int x = 10 + offset / 2; x <= 13 + offset / 2; ++x) {
      setPixelPhysical(cx + x, cy + y, false, false);
    }
  }
  for (int y = 14; y <= 17; ++y) {
    for (int x = 8; x <= 11; ++x) {
      setPixelPhysical(cx + x, cy + y, false, false);
    }
  }
}

void drawSunIcon(int cx, int cy, bool black, bool red) {
  // Red sun with a solid circular center and many short dotted rays,
  // matching the supplied reference icon.
  for (int y = -10; y <= 10; ++y) {
    for (int x = -10; x <= 10; ++x) {
      if (x * x + y * y <= 100) setPixelPhysical(cx + x, cy + y, false, true);
    }
  }
  for (int i = 0; i < 24; ++i) {
    const float a = i * (2.0f * PI / 24.0f);
    const int x0 = cx + (int)round(cos(a) * 14.0f);
    const int y0 = cy + (int)round(sin(a) * 14.0f);
    const int x1 = cx + (int)round(cos(a) * 20.0f);
    const int y1 = cy + (int)round(sin(a) * 20.0f);
    for (int j = 0; j <= 6; ++j) {
      const int x = x0 + (x1 - x0) * j / 6;
      const int y = y0 + (y1 - y0) * j / 6;
      setPixelPhysical(x, y, false, true);
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

void setLedState(const char* key, const char* state, int pin) {
  if (state == nullptr) return;
  if (strcasecmp(state, "on") == 0) {
    digitalWrite(pin, HIGH);
  } else if (strcasecmp(state, "off") == 0) {
    digitalWrite(pin, LOW);
  } else {
    return;
  }
  Serial.printf("MQTT LED %s=%s\n", key, state);
}

void setLedFromCommand(const char* key, JsonVariant value, int pin) {
  if (!value.is<const char*>()) return;
  setLedState(key, value.as<const char*>(), pin);
}

bool copyMqttState(byte* payload, unsigned int length, const char* jsonKey,
                   char* state, size_t stateSize) {
  if (length == 0 || stateSize < 2) return false;

  StaticJsonDocument<128> document;
  if (deserializeJson(document, payload, length) == DeserializationError::Ok) {
    JsonVariant value = document[jsonKey];
    if (!value.is<const char*>()) value = document["state"];
    if (value.is<const char*>()) {
      strlcpy(state, value.as<const char*>(), stateSize);
      return true;
    }
  }

  size_t copied = min((size_t)length, stateSize - 1);
  memcpy(state, payload, copied);
  state[copied] = '\0';
  while (copied > 0 && isspace((unsigned char)state[copied - 1])) {
    state[--copied] = '\0';
  }
  char* first = state;
  while (*first != '\0' && isspace((unsigned char)*first)) ++first;
  if (first != state) memmove(state, first, strlen(first) + 1);
  return state[0] != '\0';
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  const char* ledTopicPrefix = "elsie/class305/ctrl/led/";
  if (strncmp(topic, ledTopicPrefix, strlen(ledTopicPrefix)) == 0) {
    const char* suffix = topic + strlen(ledTopicPrefix);
    char state[12] = {};
    const char* jsonKey = nullptr;
    const char* ledKey = nullptr;
    int pin = -1;
    if (strcasecmp(suffix, "g") == 0 || strcasecmp(suffix, "green") == 0) {
      jsonKey = "gled"; ledKey = "gled"; pin = LED_G_PIN;
    } else if (strcasecmp(suffix, "y") == 0 || strcasecmp(suffix, "yellow") == 0) {
      jsonKey = "yled"; ledKey = "yled"; pin = LED_Y_PIN;
    } else if (strcasecmp(suffix, "r") == 0 || strcasecmp(suffix, "red") == 0) {
      jsonKey = "rled"; ledKey = "rled"; pin = LED_R_PIN;
    }
    if (pin >= 0 && copyMqttState(payload, length, jsonKey, state, sizeof(state))) {
      setLedState(ledKey, state, pin);
    }
    return;
  }

  if (strcmp(topic, MQTT_CTRL_TOPIC) != 0) return;
  StaticJsonDocument<192> document;
  DeserializationError error = deserializeJson(document, payload, length);
  if (error != DeserializationError::Ok) {
    Serial.print("MQTT control JSON error: ");
    Serial.println(error.c_str());
    return;
  }
  setLedFromCommand("gled", document["gled"], LED_G_PIN);
  setLedFromCommand("yled", document["yled"], LED_Y_PIN);
  setLedFromCommand("rled", document["rled"], LED_R_PIN);
  setLedFromCommand("green", document["green"], LED_G_PIN);
  setLedFromCommand("yellow", document["yellow"], LED_Y_PIN);
  setLedFromCommand("red", document["red"], LED_R_PIN);
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000UL) {
    Serial.print('.');
    delay(500);
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection timeout");
  }
}

void connectMqttIfNeeded() {
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) return;
  if (millis() - lastMqttAttempt < 5000UL) return;
  lastMqttAttempt = millis();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  // The e-paper refresh blocks for about 12-15 seconds.  Use a longer
  // keep-alive so MQTT does not disconnect during a screen refresh.
  mqttClient.setKeepAlive(60);
  mqttClient.setSocketTimeout(10);
  Serial.print("Connecting to MQTT...");
  if (mqttClient.connect(mqttClientId.c_str())) {
    const bool baseSubscribed = mqttClient.subscribe(MQTT_CTRL_TOPIC);
    const bool ledSubscribed = mqttClient.subscribe("elsie/class305/ctrl/led/+");
    Serial.println("connected");
    Serial.printf("MQTT subscriptions: base=%s led=%s\n",
                  baseSubscribed ? "OK" : "FAIL",
                  ledSubscribed ? "OK" : "FAIL");
  } else {
    Serial.print("failed, state=");
    Serial.println(mqttClient.state());
  }
}

void publishSensorData() {
  if (!mqttClient.connected()) return;
  StaticJsonDocument<160> document;
  document["temp"] = currentDhtValid ? currentTemperature : 0;
  document["humi"] = currentDhtValid ? currentHumidity : 0;
  document["light"] = currentLightValid ? currentLight : 0;
  document["temp_valid"] = currentDhtValid;
  document["light_valid"] = currentLightValid;
  char payload[192];
  serializeJson(document, payload, sizeof(payload));
  mqttClient.publish(MQTT_DATA_TOPIC, payload);
  Serial.print("MQTT publish: ");
  Serial.println(payload);
}

void drawSensorPage(int temperature, int humidity, int light,
                    bool temperatureHumidityValid, bool lightValid) {
  memset(blackImage, 0xFF, sizeof(blackImage));
  memset(redImage, 0xFF, sizeof(redImage));

  // One page with three side-by-side metrics.
  const int columnWidth = 98;
  const int centerX[3] = {49, 148, 247};
  for (int x : {98, 197}) {
    for (int y = 8; y < 120; ++y) setPixelPhysical(x, y, true, false);
  }

  drawCenteredTextInPanel("TEMP", 0, columnWidth, 10, 2, false, true);
  drawCenteredTextInPanel("HUMI", 99, columnWidth, 10, 2, false, true);
  drawCenteredTextInPanel("LIGHT", 198, columnWidth, 10, 2, false, true);
  drawThermometerIcon(centerX[0], 53, false, true);
  drawDropIcon(centerX[1], 53, false, true);
  drawSunIcon(centerX[2], 53, false, true);

  char value[12];
  if (temperatureHumidityValid) {
    snprintf(value, sizeof(value), "%dC", temperature);
    drawBoldCenteredTextInPanel(value, 0, columnWidth, 86, 3, true, false);
    drawDegreeSymbol(59, 82, true, false);
    snprintf(value, sizeof(value), "%d%%", humidity);
    drawBoldCenteredTextInPanel(value, 99, columnWidth, 86, 3, true, false);
  } else {
    drawBoldCenteredTextInPanel("...", 0, columnWidth, 86, 4, true, false);
    drawBoldCenteredTextInPanel("...", 99, columnWidth, 86, 4, true, false);
  }

  if (lightValid) {
    snprintf(value, sizeof(value), "%d%%", light);
    drawBoldCenteredTextInPanel(value, 198, columnWidth, 86, 3, true, false);
  } else {
    drawBoldCenteredTextInPanel("...", 198, columnWidth, 86, 4, true, false);
  }
}

static constexpr unsigned long SENSOR_INTERVAL_MS = 60000UL;
unsigned long lastSensorUpdate = 0;

void updateSensorDisplay() {
  int temperature = 0;
  int humidity = 0;
  const bool dhtValid = readDHT11(temperature, humidity);
  const int rawLight = analogRead(LIGHT_PIN);
  const bool lightValid = rawLight > 2;
  const int lightPercent = constrain(map(rawLight, 0, 4095, 0, 100), 0, 100);

  currentTemperature = temperature;
  currentHumidity = humidity;
  currentLight = lightPercent;
  currentDhtValid = dhtValid;
  currentLightValid = lightValid;

  Serial.printf("TEMP=%s HUMI=%s LIGHT=%s\n",
                dhtValid ? String(temperature).c_str() : "...",
                dhtValid ? String(humidity).c_str() : "...",
                lightValid ? String(lightPercent).c_str() : "...");
  // Publish before the blocking e-paper refresh.
  publishSensorData();
  drawSensorPage(temperature, humidity, lightPercent, dhtValid, lightValid);
  Serial.println("Refreshing sensor page; this can take about 12-15 seconds...");
  epd.Display(blackImage, redImage);
  epd.Sleep();
  lastSensorUpdate = millis();
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("38_mqtt_ctrl_page_epaper: e-paper + MQTT + OLED + LED");

  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_Y_PIN, OUTPUT);
  pinMode(LED_R_PIN, OUTPUT);
  digitalWrite(LED_G_PIN, LOW);
  digitalWrite(LED_Y_PIN, LOW);
  digitalWrite(LED_R_PIN, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_PIN, ADC_11db);

  mqttClientId = "ESP32-EPAPER-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  connectWiFi();
  connectMqttIfNeeded();

  if (epd.Init() != 0) {
    Serial.println("e-Paper init failed");
    return;
  }

  pinMode(LIGHT_PIN, INPUT);
  updateSensorDisplay();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  connectMqttIfNeeded();
  mqttClient.loop();

  if (millis() - lastSensorUpdate >= SENSOR_INTERVAL_MS) {
    updateSensorDisplay();
  }
  delay(20);
}
