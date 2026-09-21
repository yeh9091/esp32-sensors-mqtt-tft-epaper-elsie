#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <U8g2lib.h>

// ILI9225 2.2 inch 176x220 + ESP32 sensors
const int TFT_SCK=18, TFT_MOSI=23, TFT_CS=5, TFT_DC=27, TFT_RST=26;
const int W=176, H=220, DHT_PIN=14, LIGHT_PIN=33;
const int OLED_SDA=21, OLED_SCL=22;
const uint16_t BG=0x0841, CARD=0x18E3, CARD2=0x2104, WHITE=0xFFFF;
const uint16_t MUTED=0x9CF3, RED=0xF800, ORANGE=0xFD20, CYAN=0x07FF;
// 深藍標題列提升白色標題的對比度，避免拍照或強光下文字變淡
const uint16_t BLUE=0x081F, YELLOW=0xFFE0, GREEN=0x07E0;

void cmd(uint16_t v) { digitalWrite(TFT_DC,LOW); digitalWrite(TFT_CS,LOW); SPI.transfer(v>>8); SPI.transfer(v); digitalWrite(TFT_CS,HIGH); }
void dat(uint16_t v) { digitalWrite(TFT_DC,HIGH); digitalWrite(TFT_CS,LOW); SPI.transfer(v>>8); SPI.transfer(v); digitalWrite(TFT_CS,HIGH); }
void reg(uint16_t r,uint16_t v) { cmd(r); dat(v); }
void ili9225Begin() {
  digitalWrite(TFT_RST,LOW); delay(50); digitalWrite(TFT_RST,HIGH); delay(50);
  reg(0x28,0x00CE); reg(0x01,0x011C); reg(0x03,0x1030); reg(0x07,0x0017); delay(50);
  reg(0x11,0x1000); reg(0x20,0); reg(0x21,0); delay(50);
  reg(0x30,0); reg(0x31,0x00DB); reg(0x32,0); reg(0x33,0); reg(0x34,0x00DB); reg(0x35,0);
  reg(0x36,0x00AF); reg(0x37,0); reg(0x38,0x00DB); reg(0x39,0); delay(50);
  reg(0x02,0); reg(0x61,0x0103); reg(0xE8,0x1000); reg(0xB0,0x0812); reg(0x0B,0); reg(0x0D,0);
  reg(0xB1,0x0404); reg(0x62,0x0019); delay(50); reg(0xE8,0x0100);
  reg(0x50,7); reg(0x51,0x0708); reg(0x52,0x0D0A); reg(0x53,0x0404); reg(0x54,7);
  reg(0x55,0x0706); reg(0x56,0x0D0A); reg(0x57,0x0404); reg(0x58,0); reg(0x59,0);
  reg(0x20,0); reg(0x21,0); reg(0x07,0x1017); delay(80);
}
void window(int x0,int y0,int x1,int y1) { reg(0x36,x1); reg(0x37,x0); reg(0x38,y1); reg(0x39,y0); reg(0x20,x0); reg(0x21,y0); cmd(0x22); }
void fastFill(int x,int y,int w,int h,uint16_t c) {
  if(w<=0||h<=0)return; window(x,y,x+w-1,y+h-1); digitalWrite(TFT_DC,HIGH); digitalWrite(TFT_CS,LOW);
  for(long i=0;i<(long)w*h;i++){ SPI.transfer(c>>8); SPI.transfer(c); } digitalWrite(TFT_CS,HIGH);
}
class ILI9225_Adafruit : public Adafruit_GFX {
public:
  ILI9225_Adafruit() : Adafruit_GFX(W,H) {}
  void drawPixel(int16_t x,int16_t y,uint16_t color) override {
    if(x<0||x>=W||y<0||y>=H)return; x=W-1-x; window(x,y,x,y); dat(color);
  }
};
ILI9225_Adafruit tft;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

void drawThermometer(int cx,int cy,uint16_t color) {
  tft.drawRoundRect(cx-5,cy-15,10,27,5,color); tft.fillCircle(cx,cy+12,8,color);
  tft.fillRoundRect(cx-2,cy-9,4,22,2,color); tft.fillCircle(cx,cy+12,4,CARD);
  tft.fillRoundRect(cx-1,cy-7,2,17,1,color);
}
void drawDrop(int cx,int cy,uint16_t color) {
  tft.fillTriangle(cx,cy-16,cx-11,cy+2,cx+11,cy+2,color); tft.fillCircle(cx,cy+3,11,color); tft.fillCircle(cx-4,cy-1,2,WHITE);
}
void drawSun(int cx,int cy,uint16_t color) {
  tft.fillCircle(cx,cy,9,color);
  for(int i=0;i<8;i++){ float a=i*0.785398f; int x1=cx+(int)(cos(a)*13),y1=cy+(int)(sin(a)*13); int x2=cx+(int)(cos(a)*19),y2=cy+(int)(sin(a)*19); tft.drawLine(x1,y1,x2,y2,color); }
}
void drawCard(int y,const char* label,int value,const char* unit,uint16_t color,int type,bool valid) {
  const int x=7,w=162,h=51; fastFill(x,y,w,h,CARD); tft.drawRoundRect(x,y,w,h,7,color);
  if(type==0) drawThermometer(26,y+23,color); if(type==1) drawDrop(26,y+23,color); if(type==2) drawSun(26,y+23,color);
  tft.setTextSize(1); tft.setTextColor(MUTED); tft.setCursor(54,y+7); tft.print(label);
  tft.setTextSize(3); tft.setTextColor(valid?WHITE:MUTED); tft.setCursor(53,y+19); tft.print(valid?value:0);
  // 單位放大為 2 倍字體，但仍小於數值的 3 倍字體，並與數值保持間距
  tft.setTextSize(2); tft.setTextColor(color); tft.setCursor(122,y+25); tft.print(unit);
  int bar=constrain(value,0,100); fastFill(54,y+43,100,3,CARD2); fastFill(54,y+43,bar,3,(type==0)?ORANGE:color);
}
void drawStaticUI() {
  fastFill(0,0,W,H,BG); fastFill(0,0,W,31,BLUE);
  tft.setTextSize(2); tft.setTextColor(WHITE); tft.setCursor(12,7); tft.print("ROOM MONITOR");
  tft.fillCircle(158,15,6,GREEN); tft.drawCircle(158,15,8,WHITE);
  tft.setTextSize(1); tft.setTextColor(MUTED); tft.setCursor(11,34); tft.print("LIVE ENVIRONMENT STATUS");
  drawCard(47,"TEMPERATURE",0,"C",ORANGE,0,false); drawCard(101,"HUMIDITY",0,"%",CYAN,1,false); drawCard(155,"BRIGHTNESS",0,"%",YELLOW,2,true);
  tft.setTextSize(1); tft.setTextColor(MUTED); tft.setCursor(37,211); tft.print("UPDATES EVERY 30 SECONDS");
}
void drawValues(int temperature,int humidity,int light,bool valid) {
  drawCard(47,"TEMPERATURE",temperature,"C",ORANGE,0,valid); drawCard(101,"HUMIDITY",humidity,"%",CYAN,1,valid); drawCard(155,"BRIGHTNESS",light,"%",YELLOW,2,true);
}

void drawOledRow(int y,const char* label,int value,const char* unit,bool valid) {
  char valueText[8];
  snprintf(valueText,sizeof(valueText),"%d",valid?value:0);
  oled.setFont(u8g2_font_6x10_tf); oled.drawStr(2,y+10,label);
  oled.setFont(u8g2_font_logisoso18_tn); oled.drawStr(58,y+20,valueText);
  oled.setFont(u8g2_font_6x10_tf); oled.drawStr(101,y+16,unit);
}
void drawOledScreen(int temperature,int humidity,int light,bool valid) {
  oled.clearBuffer();
  drawOledRow(0,"TEMP",temperature,"C",valid);
  drawOledRow(21,"HUMI",humidity,"%",valid);
  drawOledRow(42,"LIGHT",light,"%",true);
  oled.sendBuffer();
}
bool readDHT(int& temperature,int& humidity) {
  uint8_t d[5]={0}; pinMode(DHT_PIN,OUTPUT); digitalWrite(DHT_PIN,LOW); delay(20); digitalWrite(DHT_PIN,HIGH); delayMicroseconds(30); pinMode(DHT_PIN,INPUT_PULLUP);
  auto waitLevel=[](uint8_t level,unsigned long timeout){ unsigned long t=micros(); while(digitalRead(DHT_PIN)!=level) if(micros()-t>timeout)return false; return true; };
  if(!waitLevel(LOW,100)||!waitLevel(HIGH,100)||!waitLevel(LOW,100))return false;
  for(int i=0;i<40;i++){ if(!waitLevel(HIGH,100))return false; unsigned long t=micros(); if(!waitLevel(LOW,100))return false; d[i/8]<<=1; if(micros()-t>40)d[i/8]|=1; }
  if((uint8_t)(d[0]+d[1]+d[2]+d[3])!=d[4])return false; humidity=d[0]; temperature=d[2]; return true;
}
void setup() {
  Serial.begin(115200); pinMode(TFT_CS,OUTPUT); pinMode(TFT_DC,OUTPUT); pinMode(TFT_RST,OUTPUT); digitalWrite(TFT_CS,HIGH);
  SPI.begin(TFT_SCK,-1,TFT_MOSI,TFT_CS); SPI.beginTransaction(SPISettings(8000000,MSBFIRST,SPI_MODE0)); ili9225Begin(); SPI.endTransaction(); tft.setRotation(2);
  SPI.beginTransaction(SPISettings(8000000,MSBFIRST,SPI_MODE0)); drawStaticUI(); SPI.endTransaction();
  Wire.begin(OLED_SDA,OLED_SCL); oled.begin(); drawOledScreen(0,0,0,false);
}
void loop() {
  int temperature=0,humidity=0,raw=analogRead(LIGHT_PIN); int light=constrain(map(raw,0,4095,0,100),0,100); bool valid=readDHT(temperature,humidity);
  SPI.beginTransaction(SPISettings(8000000,MSBFIRST,SPI_MODE0)); drawValues(temperature,humidity,light,valid); SPI.endTransaction();
  drawOledScreen(temperature,humidity,light,valid);
  Serial.printf("T:%dC H:%d%% L:%d%% %s\n",temperature,humidity,light,valid?"OK":"DHT FAIL"); delay(30000);
}
