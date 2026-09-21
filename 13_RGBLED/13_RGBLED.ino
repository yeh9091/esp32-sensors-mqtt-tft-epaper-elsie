int R=25;
int G=26;
int B=27;
void setup() {
  // put your setup code here, to run once:
pinMode(R,OUTPUT);
pinMode(G,OUTPUT);
pinMode(B,OUTPUT);
}

void loop() {
  // R->G->B->W
//R
analogWrite(R,255);
analogWrite(G,0);
analogWrite(B,0);
delay(1000);
//G
analogWrite(R,0);
analogWrite(G,255);
analogWrite(B,0);
delay(1000);
//B
analogWrite(R,0);
analogWrite(G,0);
analogWrite(B,255);
delay(1000);
//W
analogWrite(R,255);
analogWrite(G,255);
analogWrite(B,255);
delay(1000);
//橘
analogWrite(R,255);
analogWrite(G,187);
analogWrite(B,0);
delay(1000);
//粉紅
analogWrite(R,255);
analogWrite(G,68);
analogWrite(B,170);
delay(1000);
//青
analogWrite(R,0);
analogWrite(G,255);
analogWrite(B,204);
delay(1000);
//Y
analogWrite(R,255);
analogWrite(G,255);
analogWrite(B,0);
delay(1000);
//紫
analogWrite(R,255);
analogWrite(G,0);
analogWrite(B,255);
delay(1000);
}
