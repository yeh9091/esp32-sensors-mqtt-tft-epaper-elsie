const int LDR_PIN = 36;

const int LED1 = 15;
const int LED2 = 2;
const int LED3 = 4;

void setup() {
  Serial.begin(115200);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  analogReadResolution(12);
}

void loop() {

  int adcValue = analogRead(LDR_PIN);

  // 轉換成 0~100
  int lightLevel = map(adcValue, 0, 4095, 100, 0);

  Serial.print("ADC=");
  Serial.print(adcValue);
  Serial.print("  Level=");
  Serial.println(lightLevel);

  if (lightLevel > 60) {
    // 明亮
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
  }
  else if (lightLevel > 40) {
    // 微暗
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
  }
  else if (lightLevel > 20) {
    // 更暗
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, LOW);
  }
  else {
    // 很暗
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
  }

  delay(100);  // 每秒10次
}