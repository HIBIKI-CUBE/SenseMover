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
int centerLR = 500;
int centerBF = 500;
int deltaMaxLR = 150;
int deltaMaxBF = 150;
int deltaMinLR = 50;
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
  pinMode(rightAndLeft, INPUT);
  waitButtonUntil(HIGH);
  centerBF = analogRead(backAndForth);
  Serial.print("centerBF: ");
  Serial.println(centerBF);
  tone(buzzer, 1047, t);
  delay(t);
  waitButtonUntil(LOW);
  delay(50);
  waitButtonUntil(HIGH);
  deltaMinBF = analogRead(backAndForth) - centerBF;
  Serial.print("deltaMinBF: ");
  Serial.println(deltaMinBF);
  tone(buzzer, 1319, t);
  delay(t);
  waitButtonUntil(LOW);
  delay(50);
  waitButtonUntil(HIGH);
  deltaMaxBF = analogRead(backAndForth) - centerBF;
  Serial.print("deltaMaxBF: ");
  Serial.println(deltaMaxBF);
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
  while (digitalRead(button) == HIGH)
  {
    digitalWrite(10, HIGH);
    digitalWrite(13, HIGH);

    int rl = analogRead(backAndForth) - centerBF;

    uint16_t Vl = map(rl, -deltaMaxBF, deltaMaxBF, 833, 4095);
    uint16_t Vr = map(rl, -deltaMaxBF, deltaMaxBF, 833, 4095);
    Wire.beginTransmission(DEVICE_ADDRESS60);
    Wire.write((Vl>>8)&0x0F);
    Wire.write(Vl);
    Wire.endTransmission();
    Wire.beginTransmission(DEVICE_ADDRESS61);
    Wire.write((Vr>>8)&0x0F);
    Wire.write(Vr);
    Wire.endTransmission();
    // if (millis() - startDelay >= 100)
    // {
    //   Serial.println(s);
    //   uint16_t Vr = s;
    //   uint16_t Vl = 4095 - s;
    //   Wire.beginTransmission(i2cFirst);
    //   Wire.write((Vr>>8)&0x0F);
    //   Wire.write(Vr);
    //   Wire.endTransmission();
    //   Wire.beginTransmission(i2cLater);
    //   Wire.write((Vl>>8)&0x0F);
    //   Wire.write(Vl);
    //   Wire.endTransmission();
    //   // i2cFirst ^= i2cLater;
    //   // i2cLater ^= i2cFirst;
    //   // i2cFirst ^= i2cLater;
    //   s += d;
    //   if(s <= 0 || 4095 <= s){
    //     d = -d;
    //   }
    //   if(s >= 4095){
    //     s = 4095;
    //   }
    //   if(s <= 0){
    //     s = 0;
    //   }
    //   startDelay = millis();
    // }
    // if (-deltaMinBF < rl && rl <= deltaMinBF){
    //   uint16_t Vl = 2048;
    //   uint16_t Vr = 2048;
    //   Wire.beginTransmission(DEVICE_ADDRESS60);
    //   Wire.write((Vl>>8)&0x0F);
    //   Wire.write(Vl);
    //   Wire.endTransmission();
    //   Wire.beginTransmission(DEVICE_ADDRESS61);
    //   Wire.write((Vr>>8)&0x0F);
    //   Wire.write(Vr);
    //   Wire.endTransmission();
    //   tone(buzzer, 523, t);
    //   delay(t);
    //   tone(buzzer,523,t) ;
    //   delay(t);
    //   delay(1000);
    // }
    // else if (deltaMinBF <= rl)
    // {
    //   uint16_t Vl = map(rl - deltaMinBF, 0, deltaMaxBF - deltaMinBF, 2048, 4095);
    //   uint16_t Vr = map(rl - deltaMinBF, 0, deltaMaxBF - deltaMinBF, 2048, 4095);
    //   Wire.beginTransmission(DEVICE_ADDRESS60);
    //   Wire.write((Vl>>8)&0x0F);
    //   Wire.write(Vl);
    //   Wire.endTransmission();
    //   Wire.beginTransmission(DEVICE_ADDRESS61);
    //   Wire.write((Vr>>8)&0x0F);
    //   Wire.write(Vr);
    //   Wire.endTransmission();

    //   tone(buzzer, 523, t);
    //   delay(t);
    //   tone(buzzer,784,t) ;
    //   delay(t);
    // }
    // else if (rl < -deltaMinBF)
    // {
    //   // uint16_t Vl = map(rl - deltaMinBF, -deltaMaxBF, deltaMaxBF - deltaMinBF, 833, 2048);
    //   // uint16_t Vr = map(rl - deltaMinBF, -deltaMaxBF, deltaMaxBF - deltaMinBF, 833, 2048);
    //   // Wire.beginTransmission(DEVICE_ADDRESS60);
    //   // Wire.write((Vl>>8)&0x0F);
    //   // Wire.write(Vl);
    //   // Wire.endTransmission();
    //   // Wire.beginTransmission(DEVICE_ADDRESS61);
    //   // Wire.write((Vr>>8)&0x0F);
    //   // Wire.write(Vr);
    //   // Wire.endTransmission();
    //   tone(buzzer,784,t) ;
    //   delay(t);
    //   tone(buzzer, 523, t);
    //   delay(t);
    // }
    // // Serial.println(analogRead(rightAndLeft));
  }
  digitalWrite(10, LOW);
  digitalWrite(13, LOW);
  uint16_t Vl = 2047;
  uint16_t Vr = 2047;
  Wire.beginTransmission(DEVICE_ADDRESS60);
  Wire.write((Vl>>8)&0x0F);
  Wire.write(Vl);
  Wire.endTransmission();
  Wire.beginTransmission(DEVICE_ADDRESS61);
  Wire.write((Vr>>8)&0x0F);
  Wire.write(Vr);
  Wire.endTransmission();
  // d = 64;
  // s = 2048;
}
