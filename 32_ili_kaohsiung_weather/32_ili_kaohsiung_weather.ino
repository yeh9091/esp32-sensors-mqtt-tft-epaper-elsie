#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_GFX.h>

// 沿用原專案 Wi-Fi 設定
 const char* WIFI_SSID = "YOUR_WIFI_SSID";
 const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const int TFT_SCK=18, TFT_MOSI=23, TFT_CS=5, TFT_DC=27, TFT_RST=26;
const int W=176, H=220;
unsigned long lastWeatherFetch=0;
const uint16_t NAVY=0x10A2, DARK=0x0841, PANEL=0x18E3, WHITE=0xFFFF;
const uint16_t CYAN=0x07FF, YELLOW=0xFFE0, RED=0xF800, GREEN=0x07E0, GRAY=0xBDF7;

void cmd(uint16_t v){digitalWrite(TFT_DC,LOW);digitalWrite(TFT_CS,LOW);SPI.transfer(v>>8);SPI.transfer(v);digitalWrite(TFT_CS,HIGH);}
void dat(uint16_t v){digitalWrite(TFT_DC,HIGH);digitalWrite(TFT_CS,LOW);SPI.transfer(v>>8);SPI.transfer(v);digitalWrite(TFT_CS,HIGH);}
void reg16(uint16_t r,uint16_t v){cmd(r);dat(v);}
void iliBegin(){
  digitalWrite(TFT_RST,LOW);delay(50);digitalWrite(TFT_RST,HIGH);delay(50);
  reg16(0x28,0x00CE);reg16(0x01,0x011C);reg16(0x03,0x1030);reg16(0x07,0x0017);delay(50);reg16(0x11,0x1000);reg16(0x20,0);reg16(0x21,0);delay(50);
  reg16(0x30,0);reg16(0x31,0x00DB);reg16(0x32,0);reg16(0x33,0);reg16(0x34,0x00DB);reg16(0x35,0);reg16(0x36,0x00AF);reg16(0x37,0);reg16(0x38,0x00DB);reg16(0x39,0);delay(50);
  reg16(0x02,0);reg16(0x61,0x0103);reg16(0xE8,0x1000);reg16(0xB0,0x0812);reg16(0x0B,0);reg16(0x0D,0);reg16(0xB1,0x0404);reg16(0x62,0x0019);delay(50);reg16(0xE8,0x0100);
  reg16(0x50,7);reg16(0x51,0x0708);reg16(0x52,0x0D0A);reg16(0x53,0x0404);reg16(0x54,7);reg16(0x55,0x0706);reg16(0x56,0x0D0A);reg16(0x57,0x0404);reg16(0x58,0);reg16(0x59,0);reg16(0x20,0);reg16(0x21,0);reg16(0x07,0x1017);delay(80);
}
void window(int x0,int y0,int x1,int y1){reg16(0x36,x1);reg16(0x37,x0);reg16(0x38,y1);reg16(0x39,y0);reg16(0x20,x0);reg16(0x21,y0);cmd(0x22);}
void fastFill(int x,int y,int w,int h,uint16_t c){if(w<=0||h<=0)return;window(x,y,x+w-1,y+h-1);digitalWrite(TFT_DC,HIGH);digitalWrite(TFT_CS,LOW);for(long i=0;i<(long)w*h;i++){SPI.transfer(c>>8);SPI.transfer(c);}digitalWrite(TFT_CS,HIGH);}
class ILI9225_Adafruit:public Adafruit_GFX{public:ILI9225_Adafruit():Adafruit_GFX(W,H){}void drawPixel(int16_t x,int16_t y,uint16_t c)override{if(x<0||x>=W||y<0||y>=H)return;x=W-1-x;window(x,y,x,y);dat(c);}};
ILI9225_Adafruit tft;

const char* weatherName(int code){
  if(code==0)return "SUN"; if(code<=3)return "CLOUD"; if(code<=48)return "FOG"; if(code<=67)return "RAIN"; if(code<=77)return "SNOW"; if(code<=82)return "SHOWER"; return "STORM";
}
uint16_t weatherColor(int code){if(code==0)return YELLOW;if(code<=3)return GRAY;if(code<=48)return GREEN;if(code<=82)return CYAN;return RED;}
void connectWiFi(){
  tft.setTextSize(1);tft.setTextColor(WHITE);tft.setCursor(20,100);tft.print("CONNECTING WIFI...");
  WiFi.mode(WIFI_STA);WiFi.begin(WIFI_SSID,WIFI_PASSWORD);Serial.print("WiFi");
  int tries=0;while(WiFi.status()!=WL_CONNECTED&&tries++<40){delay(500);Serial.print('.');}
  Serial.println();if(WiFi.status()==WL_CONNECTED){Serial.print("WiFi IP: ");Serial.println(WiFi.localIP());}
}
void drawHeader(){
  fastFill(0,0,W,H,NAVY);fastFill(0,0,W,27,DARK);tft.setTextColor(WHITE);tft.setTextSize(2);tft.setCursor(26,5);tft.print("KAOHSIUNG");
  tft.setTextSize(1);tft.setTextColor(CYAN);tft.setCursor(55,28);tft.print("7-DAY FORECAST");
}
void showError(const char* msg){fastFill(0,55,W,100,NAVY);tft.setTextColor(RED);tft.setTextSize(1);tft.setCursor(15,80);tft.print("WEATHER ERROR");tft.setTextColor(WHITE);tft.setCursor(15,100);tft.print(msg);}
bool fetchWeather(){
  if(WiFi.status()!=WL_CONNECTED)connectWiFi();
  SPI.beginTransaction(SPISettings(8000000,MSBFIRST,SPI_MODE0));
  fastFill(0,0,W,H,NAVY);tft.setTextColor(WHITE);tft.setTextSize(1);tft.setCursor(22,100);tft.print("FETCHING WEATHER...");
  SPI.endTransaction();
  const char* url="https://api.open-meteo.com/v1/forecast?latitude=22.6273&longitude=120.3014&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max&timezone=Asia%2FTaipei&forecast_days=7";
  WiFiClientSecure client;client.setInsecure();HTTPClient http;Serial.println("Fetching Kaohsiung weather...");
  if(!http.begin(client,url)){showError("HTTP BEGIN FAIL");return false;}int code=http.GET();Serial.printf("HTTP status: %d\n",code);
  if(code!=HTTP_CODE_OK){showError("HTTP GET FAIL");http.end();return false;}
  String payload=http.getString(); Serial.printf("Weather payload: %u bytes\n",payload.length());
  DynamicJsonDocument doc(10000);DeserializationError err=deserializeJson(doc,payload);http.end();
  if(err){Serial.println(err.c_str());showError("JSON PARSE FAIL");return false;}
  JsonArray dates=doc["daily"]["time"].as<JsonArray>();JsonArray codes=doc["daily"]["weather_code"].as<JsonArray>();JsonArray highs=doc["daily"]["temperature_2m_max"].as<JsonArray>();JsonArray lows=doc["daily"]["temperature_2m_min"].as<JsonArray>();JsonArray rain=doc["daily"]["precipitation_probability_max"].as<JsonArray>();
  if(dates.size()<7){showError("DATA INCOMPLETE");return false;}
  drawHeader();
  for(int i=0;i<7;i++){
    int y=43+i*24;uint16_t c=weatherColor(codes[i].as<int>());if(i%2==0)fastFill(6,y-3,164,22,PANEL);fastFill(8,y-3,4,22,c);
    const char* date=dates[i].as<const char*>();char dateText[8];snprintf(dateText,sizeof(dateText),"%c%c/%c%c",date[5],date[6],date[8],date[9]);
    tft.setTextSize(1);tft.setTextColor(WHITE);tft.setCursor(17,y+3);tft.print(dateText);tft.setTextColor(c);tft.setCursor(49,y+3);tft.print(weatherName(codes[i].as<int>()));
    tft.setTextColor(WHITE);tft.setCursor(103,y+3);tft.printf("%d/%dC",(int)highs[i].as<float>(),(int)lows[i].as<float>());tft.setTextColor(YELLOW);tft.setCursor(145,y+3);tft.printf("%d%%",rain[i].as<int>());
  }
  Serial.println("Weather data displayed successfully");return true;
}
void setup(){Serial.begin(115200);Serial.println("BOOT WEATHER TEST");pinMode(TFT_CS,OUTPUT);pinMode(TFT_DC,OUTPUT);pinMode(TFT_RST,OUTPUT);digitalWrite(TFT_CS,HIGH);SPI.begin(TFT_SCK,-1,TFT_MOSI,TFT_CS);SPI.beginTransaction(SPISettings(8000000,MSBFIRST,SPI_MODE0));iliBegin();fastFill(0,0,W,H,NAVY);SPI.endTransaction();tft.setRotation(2);connectWiFi();fetchWeather();lastWeatherFetch=millis();}
void loop(){if(millis()-lastWeatherFetch>=1800000UL){fetchWeather();lastWeatherFetch=millis();}delay(1000);}
