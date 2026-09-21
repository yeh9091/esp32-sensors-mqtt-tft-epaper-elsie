//DHT宣告
#include <SimpleDHT.h>
int pinDHT11 = 19;
SimpleDHT11 dht11(pinDHT11);

//OLED宣告
#include "Wire.h"
#include "U8g2lib.h"  //OLED 螢幕解析度為128*64
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

//Buzzer宣告
#include <ESP32Servo.h>

int Buzzer = 17;
int R=25;
int G=26;
int B=27;

void setup() {
// put your setup code here, to run once:
Serial.begin(115200);
u8g2.begin();                                //初始化
u8g2.enableUTF8Print();                      //啟用 UTF8字集
u8g2.setFont(u8g2_font_unifont_t_chinese1);  //設定使用中文字形
u8g2.setFontPosTop();//座標從上開始
pinMode(R,OUTPUT);
pinMode(G,OUTPUT);
pinMode(B,OUTPUT);
pinMode(Buzzer, OUTPUT);
}

void loop() {
// put your main code here, to run repeatedly:
//讀取溫溼度
Serial.println("=================================");
Serial.println("Sample DHT11...");  
//byte=>0~255範圍的整數 ,int=>20億左右, long 天文
byte temperature = 0;
byte humidity = 0;
//如果讀取錯誤,就印出錯誤訊息並返回return,返回開始的地方
int err = SimpleDHTErrSuccess;
if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
{
Serial.print("Read DHT11 failed, err="); Serial.print(SimpleDHTErrCode(err));
Serial.print(","); Serial.println(SimpleDHTErrDuration(err));
delay(1000);
return;
}
  
Serial.println("Sample OK: " + (String)temperature +" *C, " +(String)humidity +" H"); 
if(humidity<=60){analogWrite(R,0);analogWrite(G,255);analogWrite(B,0);
  tone(Buzzer, 262);
  delay(500);
  tone(Buzzer, 294);
  delay(500);
  tone(Buzzer, 262);
  delay(500);
  noTone(Buzzer);
  delay(1000);}//G
if(humidity>=61 and humidity<=70){analogWrite(R,255);analogWrite(G,255);analogWrite(B,0);
  tone(Buzzer, 294);
  delay(500);
  tone(Buzzer, 330);
  delay(500);
  tone(Buzzer, 294);
  delay(500);
  noTone(Buzzer);
  delay(1000);}//Y
if(humidity>=71 and humidity<=80){analogWrite(R,255);analogWrite(G,187);analogWrite(B,0);
  tone(Buzzer, 330);
  delay(500);
  tone(Buzzer, 349);
  delay(500);
  tone(Buzzer, 330);
  delay(500);
  noTone(Buzzer);
  delay(1000);}//橘
if(humidity>=81 and humidity<=90){analogWrite(R,0);analogWrite(G,255);analogWrite(B,204);
  tone(Buzzer, 349);
  delay(500);
  tone(Buzzer, 392);
  delay(500);
  tone(Buzzer, 349);
  delay(500);
  noTone(Buzzer);
  delay(1000);}//青
if(humidity>=91 and humidity<=100){analogWrite(R,255);analogWrite(G,0);analogWrite(B,0);
  tone(Buzzer, 392);
  delay(500);
  tone(Buzzer, 262);
  delay(500);
  tone(Buzzer, 392);
  delay(500);
  noTone(Buzzer);
  delay(1000);}//R
u8g2.clearBuffer();//顯示前清除螢幕
u8g2.setCursor(0, 5);//移動游標
u8g2.print("勞動部高屏澎分屬");//寫入文字
u8g2.setCursor(0, 25);//移動游標
u8g2.print("溫度："+(String)temperature+" C");//寫入文字
u8g2.setCursor(0, 45);//移動游標
u8g2.print("濕度：" + (String)humidity + " %");//寫入文字 
u8g2.sendBuffer();//送到螢幕顯示
delay(1000);
}