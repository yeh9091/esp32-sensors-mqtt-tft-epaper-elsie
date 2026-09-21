int sensorPin = 23;   // 光敏感測器 DO
int led1 = 15;
int led2 = 2;
int led3 = 4;

void setup() {
  pinMode(sensorPin, INPUT);

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);

  Serial.begin(115200);
}

void loop() {
  int sensorValue = digitalRead(sensorPin);

  Serial.print("Sensor = ");
  Serial.println(sensorValue);

  // 天黑
  if (sensorValue == HIGH) {
    digitalWrite(led1, HIGH);
    digitalWrite(led2, HIGH);
    digitalWrite(led3, HIGH);

    Serial.println("Dark -> LED ON");
  }
  // 天亮
  else {
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);
    digitalWrite(led3, LOW);

    Serial.println("Bright -> LED OFF");
  }

  delay(500);
}