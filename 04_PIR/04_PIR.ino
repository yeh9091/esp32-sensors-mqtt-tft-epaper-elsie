int pirPin = 18;
int redLED = 4;
int greenLED = 15;

unsigned long lastMotionTime = 0;
unsigned long lastReadTime = 0;

const unsigned long detectInterval = 500;  // 0.5秒
const unsigned long redOnTime = 5000;      // 5秒

void setup() {
  pinMode(pirPin, INPUT);

  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);

  Serial.begin(115200);

  digitalWrite(greenLED, HIGH); // 初始綠燈亮
  digitalWrite(redLED, LOW);
}

void loop() {

  unsigned long currentTime = millis();

  // 每0.5秒讀取一次PIR
  if (currentTime - lastReadTime >= detectInterval) {

    lastReadTime = currentTime;

    int pirValue = digitalRead(pirPin);

    Serial.print("PIR = ");
    Serial.println(pirValue);

    if (pirValue == HIGH) {

      Serial.println("偵測到人體");

      lastMotionTime = currentTime;

      digitalWrite(redLED, HIGH);
      digitalWrite(greenLED, LOW);
    }
  }

  // 超過5秒未偵測到人體
  if (digitalRead(redLED) == HIGH &&
      currentTime - lastMotionTime >= redOnTime) {

    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);

    Serial.println("5秒內無人體切換綠燈");
  }
}