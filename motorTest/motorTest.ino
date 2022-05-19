#include <Wire.h> //I2C用通信用ライブラリ先頭文

//前後のセンサーのキャリブレーションとテストを行います。
//プログラム開始時にキャリブレーションを行います。
//通常の姿勢で1度ボタンを押します。これで中心のキャリブレーションを行います。
//前進を開始する姿勢で1度ボタンを押します。これで前後のキャリブレーションを行います。
//最後に、最速前進時の姿勢で1度ボタンを押します。これで前後最大のキャリブレーションを行います。
//その後はセンサーのテストができます。ボタンを押している間、認識している方向を音で示します。
//前進は音階が上がり、後退は音階が下がります。中立時は同じ音を繰り返します。

const int t = 83;
const int buzzer = 12;
const int button = 8;
const int rightAndLeft = A0;
const int backAndForth = A1;
const uint8_t DEVICE_ADDRESS60 = 0x60;//デバイスアドレス（スレーブ）
const uint8_t DEVICE_ADDRESS61 = 0x61;//
int centerRL = 511;
int centerBF = 511;
int deltaMaxRL = 150;
int deltaMaxBF = 150;
int deltaMinRL = 50;
int deltaMinBF = 50;
uint8_t i2cFirst = DEVICE_ADDRESS60;
uint8_t i2cLater = DEVICE_ADDRESS61;

void waitButtonUntil(int v){
  while(digitalRead(button) != v){}
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  Wire.setClock(400000);
  pinMode(button, INPUT);
  pinMode(13, OUTPUT);
  pinMode(10, OUTPUT);
  waitButtonUntil(HIGH);
  centerBF = analogRead(backAndForth);
  // Serial.print("centerBF: ");
  // Serial.println(centerBF);
  tone(buzzer, 1047, t);
  delay(t);
  waitButtonUntil(LOW);
  delay(50);
  waitButtonUntil(HIGH);
  deltaMinBF = analogRead(backAndForth) - centerBF;
  // Serial.print("deltaMinBF: ");
  // Serial.println(deltaMinBF);
  tone(buzzer, 1319, t);
  delay(t);
  waitButtonUntil(LOW);
  delay(50);
  waitButtonUntil(HIGH);
  deltaMaxBF = analogRead(backAndForth) - centerBF;
  // Serial.print("deltaMaxBF: ");
  // Serial.println(deltaMaxBF);
  tone(buzzer, 1568, t);
  delay(t);
  // waitButtonUntil(LOW);
  // delay(50);
  // waitButtonUntil(HIGH);
  // centerRL = analogRead(rightAndLeft);
  // // Serial.print("centerRL: ");
  // // Serial.println(centerRL);
  // tone(buzzer, 1047, t);
  // delay(t);
  waitButtonUntil(LOW);
  delay(50);
  waitButtonUntil(HIGH);
  deltaMinRL = analogRead(rightAndLeft) - centerRL;
  // Serial.print("deltaMinRL: ");
  // Serial.println(deltaMinRL);
  tone(buzzer, 1319, t);
  delay(t);
  waitButtonUntil(LOW);
  delay(50);
  waitButtonUntil(HIGH);
  deltaMaxRL = analogRead(rightAndLeft) - centerRL;
  // Serial.print("deltaMaxRL: ");
  // Serial.println(deltaMaxRL);
  tone(buzzer, 1568, t);
  delay(t);
  waitButtonUntil(LOW);
  delay(1000);
  tone(buzzer, 1047, t);
  delay(t);
  tone(buzzer, 1319, t);
  delay(t);
  tone(buzzer, 1568, t);
}

void loop() {
  unsigned long startDelay = 0;
  uint16_t Vl = 2047;
  uint16_t Vr = 2047;
  int bf = 0;
  int bf1 = 0;
  int rl = 0;
  if (digitalRead(button) == HIGH)
  {
    digitalWrite(10, HIGH);
    digitalWrite(13, HIGH);

    bf1 = analogRead(backAndForth) - centerBF;
    bf = sqrt(pow(analogRead(backAndForth) - centerBF, 2) + pow(analogRead(rightAndLeft) - centerRL, 2));
    rl = analogRead(rightAndLeft) - centerRL;


    if (bf1 <= -deltaMinBF || deltaMinBF <= bf1) {
      Vl = map(bf1, -deltaMaxBF, deltaMaxBF, 410, 4095);
      Vr = map(bf1, -deltaMaxBF, deltaMaxBF, 410, 4095);
      if (abs(rl) >= deltaMinRL && bf1 > 0){
        if(rl > 0){
          Vl -= (Vl - 2047) * (((float) map(abs(rl), deltaMinRL, deltaMaxRL, 10, 50)) / 100);
        }else{
          Vr -= (Vr - 2047) * (((float) map(abs(rl), deltaMinRL, deltaMaxRL, 10, 50)) / 100);
        }
      }
      Vl = constrain(Vl, 410, 4095);
      Vr = constrain(Vr, 410, 4095);
    }
    // Serial.print("d: ");
    Serial.println((int)Vr - (int)Vl);
  }
  else
  {
    digitalWrite(10, LOW);
    digitalWrite(13, LOW);
  }
  // Serial.print("bf: ");
  // Serial.print(bf);
  // Serial.print(", rl: ");
  // Serial.println(rl);

  Wire.beginTransmission(DEVICE_ADDRESS60);
  Wire.write((Vl>>8)&0x0F);
  Wire.write(Vl);
  Wire.endTransmission();
  Wire.beginTransmission(DEVICE_ADDRESS61);
  Wire.write((Vr>>8)&0x0F);
  Wire.write(Vr);
  Wire.endTransmission();
}
