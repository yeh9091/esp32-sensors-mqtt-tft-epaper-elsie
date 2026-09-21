void setup() {
  // 程式初始化,只執行一次
Serial.begin(115200);//序列啟動,115200速率,功能為送出除錯訊息
}

void loop() {
  //重複執行,無止無盡
Serial.println("老師");
delay(500);
Serial.println("好像要吃飯了");
delay(500);
}
