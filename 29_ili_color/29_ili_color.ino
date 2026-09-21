#include <SPI.h>

const int TFT_SCK  = 18;
const int TFT_MOSI = 23;
const int TFT_CS   = 5;
const int TFT_DC   = 27;  // RS / A0
const int TFT_RST  = 26;

const int TFT_WIDTH = 176;
const int TFT_HEIGHT = 220;

const uint16_t RED   = 0xF800;
const uint16_t GREEN = 0x07E0;
const uint16_t BLUE  = 0x001F;
const uint16_t WHITE = 0xFFFF;
const uint16_t BLACK = 0x0000;

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
  delay(50);
  digitalWrite(TFT_RST, HIGH);
  delay(50);

  writeRegister(0x0028, 0x00CE);
  writeRegister(0x0001, 0x011C);
  writeRegister(0x0003, 0x1030);
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
  writeRegister(0x0007, 0x1017);
  delay(80);
}

void fillScreen(uint16_t color) {
  writeRegister(0x0036, TFT_WIDTH - 1);
  writeRegister(0x0037, 0);
  writeRegister(0x0038, TFT_HEIGHT - 1);
  writeRegister(0x0039, 0);
  writeRegister(0x0020, 0);
  writeRegister(0x0021, 0);
  writeCommand(0x0022);

  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_CS, LOW);
  for (int i = 0; i < TFT_WIDTH * TFT_HEIGHT; i++) {
    SPI.transfer(color >> 8);
    SPI.transfer(color & 0xFF);
  }
  digitalWrite(TFT_CS, HIGH);
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS); // 沒有 SDO，不使用 MISO
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  ili9225Init();
  SPI.endTransaction();
}

void loop() {
  const uint16_t colors[] = {RED, GREEN, BLUE, WHITE, BLACK};
  const char* names[] = {"RED", "GREEN", "BLUE", "WHITE", "BLACK"};

  for (int i = 0; i < 5; i++) {
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    fillScreen(colors[i]);
    SPI.endTransaction();
    Serial.println(names[i]);
    delay(3000);
  }
}
