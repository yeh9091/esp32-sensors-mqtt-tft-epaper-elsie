#include <ESP32Servo.h>
int Buzzer = 17;
void setup() {
  // put your setup code here, to run once:
  pinMode(Buzzer, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  tone(Buzzer, 262);
  delay(1000);
  tone(Buzzer, 294);
  delay(1000);
  tone(Buzzer, 330);
  delay(1000);
  noTone(Buzzer);
  delay(5000);
}
