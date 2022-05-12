//左右のセンサーのキャリブレーションとテストを行います。
//プログラム開始時にキャリブレーションを行います。
//通常の姿勢で1度ボタンを押します。これで中心のキャリブレーションを行います。
//右旋回開始を開始する姿勢で1度ボタンを押します。これで左右のキャリブレーションを行います。
//最後に、最大右旋回時の姿勢で1度ボタンを押します。これで左右最大のキャリブレーションを行います。
//その後はセンサーのテストができます。ボタンを押している間、認識している方向を音で示します。
//右方向は音階が上がり、左方向は音階が下がります。中立時は同じ音を繰り返します。

const int t = 83;
const int buzzer = 12;
const int button = 8;
const int rightAndLeft = A0;
const int backAndForth = A1;
int centerLR = 500;
int deltaMaxLR = 150;
int deltaMinLR = 50;

void waitButtonUntil(int v){
  while(digitalRead(button) != v){}
}

void setup() {
  Serial.begin(9600);
  pinMode(button, INPUT);
  pinMode(rightAndLeft, INPUT);
  waitButtonUntil(HIGH);
  centerLR = analogRead(rightAndLeft);
  Serial.print("centerLR: ");
  Serial.println(centerLR);
  tone(buzzer, 1047, t);
  delay(t);
  waitButtonUntil(LOW);
  delay(50);
  waitButtonUntil(HIGH);
  deltaMinLR = analogRead(rightAndLeft) - centerLR;
  Serial.print("deltaMinLR: ");
  Serial.println(deltaMinLR);
  tone(buzzer, 1319, t);
  delay(t);
  waitButtonUntil(LOW);
  delay(50);
  waitButtonUntil(HIGH);
  deltaMaxLR = analogRead(rightAndLeft) - centerLR;
  Serial.print("deltaMaxLR: ");
  Serial.println(deltaMaxLR);
  tone(buzzer, 1568, t);
  delay(t);
  waitButtonUntil(LOW);
}

void loop() {
  waitButtonUntil(HIGH); 
  int rl = analogRead(rightAndLeft) - centerLR;
  if (-deltaMinLR < rl && rl <= deltaMinLR){
    tone(buzzer, 523, t);
    delay(t);
    tone(buzzer,523,t) ;
    delay(t);
  }else if (deltaMinLR <= rl){
    tone(buzzer, 523, t);
    delay(t);
    tone(buzzer,784,t) ;
    delay(t);
  }else if (rl < -deltaMinLR){
    tone(buzzer,784,t) ;
    delay(t);
    tone(buzzer, 523, t);
    delay(t);
  }
  delay(1000);
  // Serial.println(analogRead(rightAndLeft));
}
