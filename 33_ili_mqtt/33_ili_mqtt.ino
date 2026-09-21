#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <U8g2lib.h>

 const char* WIFI_SSID="YOUR_WIFI_SSID";
 const char* WIFI_PASSWORD="YOUR_WIFI_PASSWORD";
const char* MQTT_HOST="mqttgo.io";
const int MQTT_PORT=1883;
const char* MQTT_TOPIC="elsie/class305/data";

const int TFT_SCK=18,TFT_MOSI=23,TFT_CS=5,TFT_DC=27,TFT_RST=26;
const int W=176,H=220,DHT_PIN=14,LIGHT_PIN=33,OLED_SDA=21,OLED_SCL=22;
const uint16_t BG=0x0841,CARD=0x18E3,CARD2=0x2104,WHITE=0xFFFF,MUTED=0x9CF3;
const uint16_t ORANGE=0xFD20,CYAN=0x07FF,BLUE=0x081F,YELLOW=0xFFE0,GREEN=0x07E0,RED=0xF800;

WiFiClient netClient; PubSubClient mqtt(netClient); unsigned long lastPublish=0,lastMqttTry=0;

void cmd(uint16_t v){digitalWrite(TFT_DC,LOW);digitalWrite(TFT_CS,LOW);SPI.transfer(v>>8);SPI.transfer(v);digitalWrite(TFT_CS,HIGH);}
void dat(uint16_t v){digitalWrite(TFT_DC,HIGH);digitalWrite(TFT_CS,LOW);SPI.transfer(v>>8);SPI.transfer(v);digitalWrite(TFT_CS,HIGH);}
void reg(uint16_t r,uint16_t v){cmd(r);dat(v);}
void iliBegin(){
  digitalWrite(TFT_RST,LOW);delay(50);digitalWrite(TFT_RST,HIGH);delay(50);reg(0x28,0x00CE);reg(0x01,0x011C);reg(0x03,0x1030);reg(0x07,0x0017);delay(50);reg(0x11,0x1000);reg(0x20,0);reg(0x21,0);delay(50);
  reg(0x30,0);reg(0x31,0x00DB);reg(0x32,0);reg(0x33,0);reg(0x34,0x00DB);reg(0x35,0);reg(0x36,0x00AF);reg(0x37,0);reg(0x38,0x00DB);reg(0x39,0);delay(50);
  reg(0x02,0);reg(0x61,0x0103);reg(0xE8,0x1000);reg(0xB0,0x0812);reg(0x0B,0);reg(0x0D,0);reg(0xB1,0x0404);reg(0x62,0x0019);delay(50);reg(0xE8,0x0100);
  reg(0x50,7);reg(0x51,0x0708);reg(0x52,0x0D0A);reg(0x53,0x0404);reg(0x54,7);reg(0x55,0x0706);reg(0x56,0x0D0A);reg(0x57,0x0404);reg(0x58,0);reg(0x59,0);reg(0x20,0);reg(0x21,0);reg(0x07,0x1017);delay(80);
}
void window(int x0,int y0,int x1,int y1){reg(0x36,x1);reg(0x37,x0);reg(0x38,y1);reg(0x39,y0);reg(0x20,x0);reg(0x21,y0);cmd(0x22);}
void fastFill(int x,int y,int w,int h,uint16_t c){if(w<=0||h<=0)return;window(x,y,x+w-1,y+h-1);digitalWrite(TFT_DC,HIGH);digitalWrite(TFT_CS,LOW);for(long i=0;i<(long)w*h;i++){SPI.transfer(c>>8);SPI.transfer(c);}digitalWrite(TFT_CS,HIGH);}
class ILI9225_Adafruit:public Adafruit_GFX{public:ILI9225_Adafruit():Adafruit_GFX(W,H){}void drawPixel(int16_t x,int16_t y,uint16_t c)override{if(x<0||x>=W||y<0||y>=H)return;x=W-1-x;window(x,y,x,y);dat(c);}};
ILI9225_Adafruit tft; U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0,U8X8_PIN_NONE);

void drawThermometer(int x,int y,uint16_t c){tft.drawRoundRect(x-5,y-15,10,27,5,c);tft.fillCircle(x,y+12,8,c);tft.fillRoundRect(x-2,y-9,4,22,2,c);tft.fillCircle(x,y+12,4,CARD);tft.fillRoundRect(x-1,y-7,2,17,1,c);}
void drawDrop(int x,int y,uint16_t c){tft.fillTriangle(x,y-16,x-11,y+2,x+11,y+2,c);tft.fillCircle(x,y+3,11,c);tft.fillCircle(x-4,y-1,2,WHITE);}
void drawSun(int x,int y,uint16_t c){tft.fillCircle(x,y,9,c);for(int i=0;i<8;i++){float a=i*0.785398f;tft.drawLine(x+cos(a)*13,y+sin(a)*13,x+cos(a)*19,y+sin(a)*19,c);}}
void drawStatus(){
  fastFill(0,32,W,13,BG);tft.setTextSize(1);tft.setCursor(19,34);tft.setTextColor(WiFi.status()==WL_CONNECTED?GREEN:RED);tft.print("WIFI: ");tft.print(WiFi.status()==WL_CONNECTED?"O":"X");
  tft.setTextColor(mqtt.connected()?GREEN:RED);tft.setCursor(104,34);tft.print("MQTT: ");tft.print(mqtt.connected()?"O":"X");
}
void drawCard(int y,const char* label,int value,const char* unit,uint16_t c,int type,bool valid){
  const int x=7,w=162,h=51;fastFill(x,y,w,h,CARD);tft.drawRoundRect(x,y,w,h,7,c);if(type==0)drawThermometer(26,y+23,c);if(type==1)drawDrop(26,y+23,c);if(type==2)drawSun(26,y+23,c);
  tft.setTextSize(1);tft.setTextColor(MUTED);tft.setCursor(54,y+7);tft.print(label);tft.setTextSize(3);tft.setTextColor(valid?WHITE:MUTED);tft.setCursor(53,y+19);tft.print(valid?value:0);tft.setTextSize(2);tft.setTextColor(c);tft.setCursor(122,y+25);tft.print(unit);int bar=constrain(value,0,100);fastFill(54,y+43,100,3,CARD2);fastFill(54,y+43,bar,3,type==0?ORANGE:c);
}
void drawStaticUI(){fastFill(0,0,W,H,BG);fastFill(0,0,W,31,BLUE);tft.setTextSize(2);tft.setTextColor(WHITE);tft.setCursor(12,7);tft.print("ROOM MONITOR");tft.fillCircle(158,15,6,GREEN);tft.drawCircle(158,15,8,WHITE);drawStatus();drawCard(47,"TEMPERATURE",0,"C",ORANGE,0,false);drawCard(101,"HUMIDITY",0,"%",CYAN,1,false);drawCard(155,"BRIGHTNESS",0,"%",YELLOW,2,true);tft.setTextSize(1);tft.setTextColor(MUTED);tft.setCursor(37,211);tft.print("MQTT JSON / 10 SEC");}
void drawValues(int t,int h,int l,bool valid){drawCard(47,"TEMPERATURE",t,"C",ORANGE,0,valid);drawCard(101,"HUMIDITY",h,"%",CYAN,1,valid);drawCard(155,"BRIGHTNESS",l,"%",YELLOW,2,true);drawStatus();}
void drawOledRow(int y,const char* label,int value,const char* unit,bool valid){char s[8];snprintf(s,sizeof(s),"%d",valid?value:0);oled.setFont(u8g2_font_6x10_tf);oled.drawStr(2,y+10,label);oled.setFont(u8g2_font_logisoso18_tn);oled.drawStr(58,y+20,s);oled.setFont(u8g2_font_6x10_tf);oled.drawStr(101,y+16,unit);}
void drawOled(int t,int h,int l,bool valid){oled.clearBuffer();drawOledRow(0,"TEMP",t,"C",valid);drawOledRow(21,"HUMI",h,"%",valid);drawOledRow(42,"LIGHT",l,"%",true);oled.sendBuffer();}
bool readDHT(int& t,int& h){uint8_t d[5]={0};pinMode(DHT_PIN,OUTPUT);digitalWrite(DHT_PIN,LOW);delay(20);digitalWrite(DHT_PIN,HIGH);delayMicroseconds(30);pinMode(DHT_PIN,INPUT_PULLUP);auto waitLevel=[](uint8_t level,unsigned long timeout){unsigned long s=micros();while(digitalRead(DHT_PIN)!=level)if(micros()-s>timeout)return false;return true;};if(!waitLevel(LOW,100)||!waitLevel(HIGH,100)||!waitLevel(LOW,100))return false;for(int i=0;i<40;i++){if(!waitLevel(HIGH,100))return false;unsigned long s=micros();if(!waitLevel(LOW,100))return false;d[i/8]<<=1;if(micros()-s>40)d[i/8]|=1;}if((uint8_t)(d[0]+d[1]+d[2]+d[3])!=d[4])return false;h=d[0];t=d[2];return true;}

void connectWiFi(){WiFi.mode(WIFI_STA);WiFi.begin(WIFI_SSID,WIFI_PASSWORD);for(int i=0;i<30&&WiFi.status()!=WL_CONNECTED;i++){drawStatus();delay(500);}Serial.println(WiFi.status()==WL_CONNECTED?"WiFi connected":"WiFi failed");drawStatus();}
void connectMQTT(){if(WiFi.status()!=WL_CONNECTED)return;String id="ESP32-"+String((uint32_t)(ESP.getEfuseMac()&0xFFFFFFFF),HEX);mqtt.setServer(MQTT_HOST,MQTT_PORT);if(mqtt.connect(id.c_str()))Serial.println("MQTT connected");else Serial.printf("MQTT failed, state=%d\n",mqtt.state());drawStatus();}
void publishData(int t,int h,int l,bool valid){if(!mqtt.connected())return;char json[80];snprintf(json,sizeof(json),"{\"temp\":%d,\"humi\":%d,\"light\":%d}",valid?t:0,valid?h:0,l);if(mqtt.publish(MQTT_TOPIC,json)){Serial.print("MQTT: ");Serial.println(json);}else Serial.println("MQTT publish failed");}

void setup(){Serial.begin(115200);analogReadResolution(12);analogSetPinAttenuation(LIGHT_PIN,ADC_11db);pinMode(TFT_CS,OUTPUT);pinMode(TFT_DC,OUTPUT);pinMode(TFT_RST,OUTPUT);digitalWrite(TFT_CS,HIGH);SPI.begin(TFT_SCK,-1,TFT_MOSI,TFT_CS);SPI.beginTransaction(SPISettings(8000000,MSBFIRST,SPI_MODE0));iliBegin();tft.setRotation(2);drawStaticUI();SPI.endTransaction();Wire.begin(OLED_SDA,OLED_SCL);oled.begin();drawOled(0,0,0,false);connectWiFi();connectMQTT();}
void loop(){if(WiFi.status()!=WL_CONNECTED){connectWiFi();}if(!mqtt.connected()&&millis()-lastMqttTry>5000){lastMqttTry=millis();connectMQTT();}mqtt.loop();int t=0,h=0,light=constrain(map(analogRead(LIGHT_PIN),0,4095,0,100),0,100);bool valid=readDHT(t,h);SPI.beginTransaction(SPISettings(8000000,MSBFIRST,SPI_MODE0));drawValues(t,h,light,valid);SPI.endTransaction();drawOled(t,h,light,valid);if(millis()-lastPublish>=10000){lastPublish=millis();publishData(t,h,light,valid);}delay(1000);}
