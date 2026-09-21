//OLED宣告
#include "Wire.h"
#include "U8g2lib.h"  //OLED 螢幕解析度為128*64
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
#include <ESP32Servo.h>
int Trig = 12; // 發出聲波
int Echo = 14; // 接收聲波
int buzzer = 17; // 蜂鳴器
void setup() {
 Serial.begin(115200);
 pinMode(Trig, OUTPUT);
 pinMode(Echo, INPUT);
 pinMode(buzzer, OUTPUT);

u8g2.begin();                                //初始化
u8g2.enableUTF8Print();                      //啟用 UTF8字集
u8g2.setFont(u8g2_font_unifont_t_chinese1);  //設定使用中文字形
u8g2.setFontPosTop();//座標從上開始
}
void loop() {
 // 步驟 1：使用超音波測量距離
 digitalWrite(Trig, LOW); // 關閉
 delayMicroseconds(5);
 digitalWrite(Trig, HIGH); // 啟動
 delayMicroseconds(10);
 digitalWrite(Trig, LOW); // 關閉
 float EchoTime = pulseIn(Echo, HIGH); // 傳回時間
 float CMValue = EchoTime / 29.4 / 2; // 轉換成距離
 Serial.println(CMValue);
 // 步驟 2：使用蜂鳴器發出指定聲音
// if (CMValue < 100 && CMValue >= 50) {
// tone(buzzer, 262, 100); // 注意：發出低音 Do
// }
// if (CMValue < 50 && CMValue >= 10) {
// tone(buzzer, 523, 100); // 小心：發出中音 Do
// }
 if (CMValue < 10) {
 tone(buzzer, 2093, 100); // 危險：發出高音 Do
 }
 delay(50);

u8g2.clearBuffer();//顯示前清除螢幕
u8g2.setCursor(0, 45);//移動游標
u8g2.print("危險" + (String)CMValue + "公分");//寫入文字 
u8g2.sendBuffer();//送到螢幕顯示
delay(1000);
}