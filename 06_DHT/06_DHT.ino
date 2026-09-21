#include <SimpleDHT.h>

// DHT11 接 GPIO19
int pinDHT11 = 19;
SimpleDHT11 dht11(pinDHT11);

// LED 接腳
const int RED_LED    = 4;
const int YELLOW_LED = 2;
const int GREEN_LED  = 15;

void setup() {
  Serial.begin(115200);

  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  Serial.println("蛋糕倉庫環境監測系統啟動");
}

void loop() {

  byte temperature = 0;
  byte humidity = 0;

  int err = SimpleDHTErrSuccess;

  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("DHT11讀取失敗, err=");
    Serial.print(SimpleDHTErrCode(err));
    Serial.print(",");
    Serial.println(SimpleDHTErrDuration(err));

    delay(1500);
    return;
  }

  Serial.println("========================");
  Serial.print("溫度: ");
  Serial.print((int)temperature);
  Serial.print(" °C");

  Serial.print("  濕度: ");
  Serial.print((int)humidity);
  Serial.println(" %");

  // 全部熄滅
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  // 危險條件
  if (temperature >= 29 || humidity >= 70) {

    digitalWrite(RED_LED, HIGH);

    Serial.println("⚠ 危險！");
    Serial.println("可能造成蛋糕變質或發霉");

  }
  // 注意條件
  else if (temperature >= 25 || humidity >= 60) {

    digitalWrite(YELLOW_LED, HIGH);

    Serial.println("△ 注意！");
    Serial.println("環境開始偏離最佳保存條件");

  }
  // 安全條件
  else {

    digitalWrite(GREEN_LED, HIGH);

    Serial.println("✓ 安全");
    Serial.println("環境適合蛋糕保存");
  }

  delay(1500);
}