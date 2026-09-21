#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <U8g2lib.h>

 const char* WIFI_SSID="YOUR_WIFI_SSID"; const char* WIFI_PASSWORD="YOUR_WIFI_PASSWORD";
const char* MQTT_HOST="mqttgo.io"; const int MQTT_PORT=1883;
const char* DATA_TOPIC="elsie/class305/data"; const char* CTRL_TOPIC="elsie/class305/ctrl";
const int TFT_SCK=18,TFT_MOSI=23,TFT_CS=5,TFT_DC=27,TFT_RST=26;
const int W=176,H=220,DHT_PIN=14,LIGHT_PIN=33,OLED_SDA=21,OLED_SCL=22,PAGE_KEY=0;
const int GLED=15,YLED=2,RLED=4, HISTORY_SIZE=60;
const uint16_t BG=0x0841,CARD=0x18E3,CARD2=0x2104,WHITE=0xFFFF,MUTED=0x9CF3;
const uint16_t ORANGE=0xFD20,CYAN=0x07FF,BLUE=0x081F,YELLOW=0xFFE0,GREEN=0x07E0,RED=0xF800;

WiFiClient net; PubSubClient mqtt(net); U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0,U8X8_PIN_NONE);
unsigned long lastSample=0,lastTry=0,lastClock=0; int page=0,currentTemp=0,currentHumidity=0,currentLight=0; int tempHistory[HISTORY_SIZE],humidityHistory[HISTORY_SIZE],lightHistory[HISTORY_SIZE],historyCount=0,historyHead=0; bool lastKey=HIGH;

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
ILI9225_Adafruit tft;

void drawThermometer(int x,int y,uint16_t c){tft.drawRoundRect(x-5,y-15,10,27,5,c);tft.fillCircle(x,y+12,8,c);tft.fillRoundRect(x-2,y-9,4,22,2,c);tft.fillCircle(x,y+12,4,CARD);tft.fillRoundRect(x-1,y-7,2,17,1,c);}
void drawDrop(int x,int y,uint16_t c){tft.fillTriangle(x,y-16,x-11,y+2,x+11,y+2,c);tft.fillCircle(x,y+3,11,c);tft.fillCircle(x-4,y-1,2,WHITE);}
void drawSun(int x,int y,uint16_t c){tft.fillCircle(x,y,9,c);for(int i=0;i<8;i++){float a=i*0.785398f;tft.drawLine(x+cos(a)*13,y+sin(a)*13,x+cos(a)*19,y+sin(a)*19,c);}}
void drawStatus(){fastFill(0,32,W,13,BG);tft.setTextSize(1);tft.setCursor(19,34);tft.setTextColor(WiFi.status()==WL_CONNECTED?GREEN:RED);tft.print("WIFI: ");tft.print(WiFi.status()==WL_CONNECTED?"O":"X");tft.setTextColor(mqtt.connected()?GREEN:RED);tft.setCursor(104,34);tft.print("MQTT: ");tft.print(mqtt.connected()?"O":"X");}
void drawClock(){struct tm ti;char s[20];if(getLocalTime(&ti,50))strftime(s,sizeof(s),"TW %H:%M:%S",&ti);else snprintf(s,sizeof(s),"TIME --:--:--");fastFill(0,207,W,13,BG);tft.setTextSize(1);tft.setTextColor(MUTED);tft.setCursor(50,210);tft.print(s);}
void drawCard(int y,const char* label,int value,const char* unit,uint16_t c,int type,bool valid){const int x=7,w=162,h=51;fastFill(x,y,w,h,CARD);tft.drawRoundRect(x,y,w,h,7,c);if(type==0)drawThermometer(26,y+23,c);if(type==1)drawDrop(26,y+23,c);if(type==2)drawSun(26,y+23,c);tft.setTextSize(1);tft.setTextColor(MUTED);tft.setCursor(54,y+7);tft.print(label);tft.setTextSize(3);tft.setTextColor(valid?WHITE:MUTED);tft.setCursor(53,y+19);tft.print(valid?value:0);tft.setTextSize(2);tft.setTextColor(c);tft.setCursor(122,y+25);tft.print(unit);int bar=constrain(value,0,100);fastFill(54,y+43,100,3,CARD2);fastFill(54,y+43,bar,3,type==0?ORANGE:c);}
void drawPage1(int t,int h,int l,bool valid){fastFill(0,0,W,H,BG);fastFill(0,0,W,31,BLUE);tft.setTextSize(2);tft.setTextColor(WHITE);tft.setCursor(12,7);tft.print("ROOM MONITOR");tft.fillCircle(158,15,6,GREEN);tft.drawCircle(158,15,8,WHITE);drawStatus();drawCard(47,"TEMPERATURE",t,"C",ORANGE,0,valid);drawCard(101,"HUMIDITY",h,"%",CYAN,1,valid);drawCard(155,"BRIGHTNESS",l,"%",YELLOW,2,true);drawClock();}
void drawGauge(int value,int minVal,int maxVal,int zone1,int zone2,const char* title,const char* unit){
  const int cx=88,cy=101,r=43;int v=constrain(value,minVal,maxVal);tft.setTextSize(1);tft.setTextColor(MUTED);tft.setCursor(48,37);tft.print(title);
  for(int deg=180;deg<=360;deg++){float ratio=(deg-180)/180.0f;int scaled=minVal+ratio*(maxVal-minVal);uint16_t color=(scaled<zone1)?GREEN:((scaled<zone2)?YELLOW:RED);float a=deg*0.0174533f;for(int thick=-4;thick<=4;thick++){int rr=r+thick;int x1=cx+cos(a)*rr;int y1=cy+sin(a)*rr;int x2=cx+cos(a+0.025f)*rr;int y2=cy+sin(a+0.025f)*rr;tft.drawLine(x1,y1,x2,y2,color);}}
  float needle=(180.0f+(v-minVal)*180.0f/(maxVal-minVal))*0.0174533f;int nx=cx+cos(needle)*(r-9),ny=cy+sin(needle)*(r-9);tft.drawLine(cx,cy,nx,ny,WHITE);tft.drawLine(cx+1,cy,nx+1,ny,WHITE);tft.fillCircle(cx,cy,4,WHITE);
  tft.setTextSize(2);tft.setTextColor(WHITE);tft.setCursor(71,78);tft.print(v);tft.setTextSize(1);tft.setCursor(103,86);tft.print(unit);tft.setTextColor(MUTED);tft.setCursor(36,114);tft.print(minVal);tft.setCursor(80,114);tft.print(zone1);tft.setCursor(119,114);tft.print(zone2);tft.setCursor(151,114);tft.print(maxVal);
}
void drawMetricGraph(const int* data,int minVal,int maxVal,uint16_t color){const int x0=25,x1=169,y0=122,y1=185;tft.drawRect(x0,y0,x1-x0+1,y1-y0+1,WHITE);int mid=(minVal+maxVal)/2;for(int v=minVal;v<=maxVal;v+=(maxVal-minVal)/2){int y=y1-(v-minVal)*(y1-y0)/(maxVal-minVal);tft.drawFastHLine(x0+1,y,x1-x0-1,0x4208);tft.setCursor(2,y-3);tft.setTextColor(MUTED);tft.setTextSize(1);tft.printf("%d",v);}tft.setTextColor(MUTED);tft.setCursor(28,198);tft.print("-10m");tft.setCursor(145,198);tft.print("NOW");int n=historyCount;if(n<2)return;int start=(historyCount<HISTORY_SIZE)?0:historyHead;for(int i=1;i<n;i++){int a=constrain(data[(start+i-1)%HISTORY_SIZE],minVal,maxVal),b=constrain(data[(start+i)%HISTORY_SIZE],minVal,maxVal);int xa=x0+1+(i-1)*(x1-x0-2)/max(1,HISTORY_SIZE-1),xb=x0+1+i*(x1-x0-2)/max(1,HISTORY_SIZE-1);int ya=y1-(a-minVal)*(y1-y0-2)/(maxVal-minVal),yb=y1-(b-minVal)*(y1-y0-2)/(maxVal-minVal);tft.drawLine(xa,ya,xb,yb,color);}}
void drawMetricPage(int metric){fastFill(0,0,W,H,BG);tft.setTextSize(2);tft.setTextColor(WHITE);tft.setCursor(12,7);if(metric==0){tft.print("TEMPERATURE");drawGauge(currentTemp,10,40,20,30,"TEMP GAUGE","C");drawMetricGraph(tempHistory,10,40,ORANGE);}else if(metric==1){tft.print("HUMIDITY");drawGauge(currentHumidity,0,100,60,80,"HUMI GAUGE","%");drawMetricGraph(humidityHistory,0,100,CYAN);}else{tft.print("BRIGHTNESS");drawGauge(currentLight,0,100,30,70,"LIGHT GAUGE","%");drawMetricGraph(lightHistory,0,100,YELLOW);}}
void drawOledRow(int y,const char* label,int value,const char* unit,bool valid){char s[8];snprintf(s,sizeof(s),"%d",valid?value:0);oled.setFont(u8g2_font_6x10_tf);oled.drawStr(2,y+10,label);oled.setFont(u8g2_font_logisoso18_tn);oled.drawStr(58,y+20,s);oled.setFont(u8g2_font_6x10_tf);oled.drawStr(101,y+16,unit);}
void drawOled(int t,int h,int l,bool valid){oled.clearBuffer();drawOledRow(0,"TEMP",t,"C",valid);drawOledRow(21,"HUMI",h,"%",valid);drawOledRow(42,"LIGHT",l,"%",true);oled.sendBuffer();}
bool readDHT(int& t,int& h){uint8_t d[5]={0};pinMode(DHT_PIN,OUTPUT);digitalWrite(DHT_PIN,LOW);delay(20);digitalWrite(DHT_PIN,HIGH);delayMicroseconds(30);pinMode(DHT_PIN,INPUT_PULLUP);auto waitLevel=[](uint8_t level,unsigned long timeout){unsigned long s=micros();while(digitalRead(DHT_PIN)!=level)if(micros()-s>timeout)return false;return true;};if(!waitLevel(LOW,100)||!waitLevel(HIGH,100)||!waitLevel(LOW,100))return false;for(int i=0;i<40;i++){if(!waitLevel(HIGH,100))return false;unsigned long s=micros();if(!waitLevel(LOW,100))return false;d[i/8]<<=1;if(micros()-s>40)d[i/8]|=1;}if((uint8_t)(d[0]+d[1]+d[2]+d[3])!=d[4])return false;h=d[0];t=d[2];return true;}

void applyLed(const char* key,JsonVariant v,int pin){if(!v.is<const char*>())return;const char* s=v.as<const char*>();if(strcmp(s,"on")==0)digitalWrite(pin,HIGH);else if(strcmp(s,"off")==0)digitalWrite(pin,LOW);Serial.printf("%s=%s\n",key,s);}
void mqttCallback(char* topic,byte* payload,unsigned int length){if(strcmp(topic,CTRL_TOPIC)!=0)return;StaticJsonDocument<192> doc;if(deserializeJson(doc,payload,length)){Serial.println("JSON error");return;}applyLed("gled",doc["gled"],GLED);applyLed("yled",doc["yled"],YLED);applyLed("rled",doc["rled"],RLED);}
void connectWiFi(){WiFi.mode(WIFI_STA);WiFi.begin(WIFI_SSID,WIFI_PASSWORD);for(int i=0;i<30&&WiFi.status()!=WL_CONNECTED;i++){drawStatus();delay(500);}drawStatus();}
void connectMQTT(){if(WiFi.status()!=WL_CONNECTED)return;String id="ESP32-PAGE-"+String((uint32_t)(ESP.getEfuseMac()&0xFFFFFFFF),HEX);mqtt.setServer(MQTT_HOST,MQTT_PORT);mqtt.setCallback(mqttCallback);if(mqtt.connect(id.c_str()))mqtt.subscribe(CTRL_TOPIC);drawStatus();}
void publishData(int t,int h,int l,bool valid){if(!mqtt.connected())return;char json[80];snprintf(json,sizeof(json),"{\"temp\":%d,\"humi\":%d,\"light\":%d}",valid?t:0,valid?h:0,l);mqtt.publish(DATA_TOPIC,json);}
void addSamples(int t,int h,int l){tempHistory[historyHead]=t;humidityHistory[historyHead]=h;lightHistory[historyHead]=l;historyHead=(historyHead+1)%HISTORY_SIZE;if(historyCount<HISTORY_SIZE)historyCount++;}
void checkPageKey(){bool key=digitalRead(PAGE_KEY);if(lastKey==HIGH&&key==LOW){delay(30);if(digitalRead(PAGE_KEY)==LOW){page=(page+1)%4;}}lastKey=key;}

void setup(){Serial.begin(115200);analogReadResolution(12);analogSetPinAttenuation(LIGHT_PIN,ADC_11db);pinMode(PAGE_KEY,INPUT_PULLUP);pinMode(GLED,OUTPUT);pinMode(YLED,OUTPUT);pinMode(RLED,OUTPUT);digitalWrite(GLED,LOW);digitalWrite(YLED,LOW);digitalWrite(RLED,LOW);pinMode(TFT_CS,OUTPUT);pinMode(TFT_DC,OUTPUT);pinMode(TFT_RST,OUTPUT);digitalWrite(TFT_CS,HIGH);SPI.begin(TFT_SCK,-1,TFT_MOSI,TFT_CS);SPI.beginTransaction(SPISettings(8000000,MSBFIRST,SPI_MODE0));iliBegin();tft.setRotation(2);drawPage1(0,0,0,false);SPI.endTransaction();Wire.begin(OLED_SDA,OLED_SCL);oled.begin();drawOled(0,0,0,false);connectWiFi();configTime(8*3600,0,"pool.ntp.org","time.nist.gov","time.google.com");connectMQTT();}
void loop(){checkPageKey();if(WiFi.status()!=WL_CONNECTED)connectWiFi();if(!mqtt.connected()&&millis()-lastTry>5000){lastTry=millis();connectMQTT();}mqtt.loop();if(millis()-lastSample>=10000){lastSample=millis();int t=0,h=0,l=constrain(map(analogRead(LIGHT_PIN),0,4095,0,100),0,100);bool valid=readDHT(t,h);if(valid){currentTemp=t;currentHumidity=h;}currentLight=l;addSamples(currentTemp,currentHumidity,l);publishData(t,h,l,valid);drawOled(t,h,l,valid);SPI.beginTransaction(SPISettings(8000000,MSBFIRST,SPI_MODE0));if(page==0)drawPage1(t,h,l,valid);else drawMetricPage(page-1);SPI.endTransaction();}if(millis()-lastClock>=10000){lastClock=millis();if(page==0){SPI.beginTransaction(SPISettings(8000000,MSBFIRST,SPI_MODE0));drawClock();SPI.endTransaction();}}delay(50);}
