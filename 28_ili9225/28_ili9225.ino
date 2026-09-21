#include <SPI.h>

// ILI9225 SPI 接線（ESP32 VSPI）
const int TFT_SCK  = 18;
const int TFT_MOSI = 23;
const int TFT_CS   = 5;
const int TFT_DC   = 27;  // RS / A0
const int TFT_RST  = 26;

const int TFT_WIDTH  = 176;
const int TFT_HEIGHT = 220;

const uint16_t BLACK = 0x0000;
const uint16_t WHITE = 0xFFFF;
const uint16_t BLUE  = 0x001F;

void writeCommand(uint16_t command) {
  digitalWrite(TFT_DC, LOW);
  digitalWrite(TFT_CS, LOW);
  SPI.transfer(command >> 8);
  SPI.transfer(command & 0xFF);
  digitalWrite(TFT_CS, HIGH);
}

void writeData(uint16_t data) {
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_CS, LOW);
  SPI.transfer(data >> 8);
  SPI.transfer(data & 0xFF);
  digitalWrite(TFT_CS, HIGH);
}

void writeRegister(uint16_t reg, uint16_t value) {
  writeCommand(reg);
  writeData(value);
}

void ili9225Init() {
  digitalWrite(TFT_RST, LOW);
  delay(20);
  digitalWrite(TFT_RST, HIGH);
  delay(50);

  // ILI9225 常用初始化時序（176 x 220）
  writeRegister(0x0028, 0x00CE);
  writeRegister(0x0001, 0x011C);
  writeRegister(0x0003, 0x1030); // 此模組的正常連續像素寫入方向
  writeRegister(0x0007, 0x0017);
  delay(50);
  writeRegister(0x0011, 0x1000);
  writeRegister(0x0020, 0x0000);
  writeRegister(0x0021, 0x0000);
  delay(50);

  writeRegister(0x0030, 0x0000);
  writeRegister(0x0031, 0x00DB);
  writeRegister(0x0032, 0x0000);
  writeRegister(0x0033, 0x0000);
  writeRegister(0x0034, 0x00DB);
  writeRegister(0x0035, 0x0000);
  writeRegister(0x0036, 0x00AF);
  writeRegister(0x0037, 0x0000);
  writeRegister(0x0038, 0x00DB);
  writeRegister(0x0039, 0x0000);
  delay(50);
  writeRegister(0x0002, 0x0000);
  writeRegister(0x0061, 0x0103);
  writeRegister(0x00E8, 0x1000);
  writeRegister(0x00B0, 0x0812);
  writeRegister(0x000B, 0x0000);
  writeRegister(0x000D, 0x0000);
  writeRegister(0x00B1, 0x0404);
  writeRegister(0x0062, 0x0019);
  delay(50);
  writeRegister(0x00E8, 0x0100);

  writeRegister(0x0050, 0x0007);
  writeRegister(0x0051, 0x0708);
  writeRegister(0x0052, 0x0D0A);
  writeRegister(0x0053, 0x0404);
  writeRegister(0x0054, 0x0007);
  writeRegister(0x0055, 0x0706);
  writeRegister(0x0056, 0x0D0A);
  writeRegister(0x0057, 0x0404);
  writeRegister(0x0058, 0x0000);
  writeRegister(0x0059, 0x0000);
  writeRegister(0x0020, 0x0000);
  writeRegister(0x0021, 0x0000);
  writeRegister(0x0007, 0x1017); // 開啟顯示
  delay(80);
}

void setAddressWindow(int x0, int y0, int x1, int y1) {
  writeRegister(0x0036, x1);
  writeRegister(0x0037, x0);
  writeRegister(0x0038, y1);
  writeRegister(0x0039, y0);
  writeRegister(0x0020, x0);
  writeRegister(0x0021, y0);
  writeCommand(0x0022);
}

void fillScreen(uint16_t color) {
  setAddressWindow(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_CS, LOW);
  for (int i = 0; i < TFT_WIDTH * TFT_HEIGHT; i++) {
    SPI.transfer(color >> 8);
    SPI.transfer(color & 0xFF);
  }
  digitalWrite(TFT_CS, HIGH);
}

void drawPixel(int x, int y, uint16_t color) {
  if (x < 0 || x >= TFT_WIDTH || y < 0 || y >= TFT_HEIGHT) return;
  // 每個像素都重新指定視窗，避免不同 ILI9225 模組的自動遞增方向差異。
  writeRegister(0x0036, x);
  writeRegister(0x0037, x);
  writeRegister(0x0038, y);
  writeRegister(0x0039, y);
  writeRegister(0x0020, x);
  writeRegister(0x0021, y);
  writeCommand(0x0022);
  writeData(color);
}

// 5 x 7 字型，只繪製測試用的 Hello
const uint8_t helloBitmap[5][7] = {
  {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // H
  {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, // E
  {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, // L
  {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, // L
  {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}  // O
};

void drawHello(int x, int y, uint16_t color, int scale) {
  // 逐像素指定絕對座標，確保 Hello 不受 GRAM 自動遞增方向影響。
  // 直接將每個像素的座標旋轉 180 度；只改變迴圈順序並不會改變畫面。
  const int textHeight = 7 * scale;
  const int cellWidth = 6 * scale; // 5 個字形像素 + 1 個等寬間隔
  for (int letter = 0; letter < 5; letter++) {
    for (int row = 0; row < 7; row++) {
      for (int col = 0; col < 5; col++) {
        bool on = (helloBitmap[letter][row] >> (4 - col)) & 1;
        for (int sy = 0; sy < scale; sy++) {
          for (int sx = 0; sx < scale; sx++) {
            int localX = letter * cellWidth + col * scale + sx;
            int localY = row * scale + sy;
            drawPixel(x + localX,
                      y + textHeight - 1 - localY,
                      on ? color : BLUE);
          }
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS); // 沒有 SDO，所以 MISO 使用 -1
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  ili9225Init();
  fillScreen(BLUE);
  drawHello(28, 100, WHITE, 4);
  SPI.endTransaction();

  Serial.println("ILI9225 test: Hello");
}

void loop() {
  delay(1000);
}
