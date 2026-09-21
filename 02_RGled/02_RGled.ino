int redLED = 0;
int yellowLED = 2;
int greenLED = 15;

void setup() {
  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
}

void loop() {

  // 綠燈亮3秒
  digitalWrite(greenLED, HIGH);
  digitalWrite(yellowLED, LOW);
  digitalWrite(redLED, LOW);
  delay(3000);

  // 黃燈閃爍3次
  digitalWrite(greenLED, LOW);

  for (int i = 0; i < 3; i++) {
    digitalWrite(yellowLED, HIGH);
    delay(500);

    digitalWrite(yellowLED, LOW);
    delay(500);
  }

  // 紅燈亮5秒
  digitalWrite(redLED, HIGH);
  delay(5000);

  // 紅燈熄滅，準備進入下一循環
  digitalWrite(redLED, LOW);
}