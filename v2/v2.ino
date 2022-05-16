#include <Wire.h> //I2C用通信用ライブラリ先頭文
int xx[20];
int yy[20];
//shift関数（直近20個のデータを配列に代入）
int shiftX(int Vx){
  for(int i = 1; i <20; i++)
    xx[i] = xx[i - 1];
    xx[0]=Vx;
}
int shiftY(int Vy)
{
  for(int j = 1; j < 20; j++)
    yy[j] = yy[j - 1];
    yy[0] =Vy;
}
//ave関数(配列に代入されたものを平均化）
int aveX(){
  int sum = 0;
  for(int i =0; i < 20; i++)
    sum += xx[i];
   return sum / 20;
}
int aveY(){
  int sum = 0;
  for(int j =0; j < 20; j++)
    sum += yy[j];
  return sum / 20;
}
const int stopPin = 8;
uint8_t DEVICE_ADDRESS60 = 0x60;//デバイスアドレス（スレーブ）
uint8_t DEVICE_ADDRESS61 = 0x61;//デバイスアドレス（スレーブ）

//I2Cを行うためにWireライブラリの初期処理をし、
//デジタル入出力の０～６番ピンを出力、８番ピンを入力に設定
void setup(){
  Wire.begin();
  pinMode(stopPin,INPUT);
  for (int i = 0; i <= 6; i++){
    pinMode(i, OUTPUT);
  }
}
void loop()
{
//消灯(０～６番ピンの出力をLOWにしている）
  for (int a = 0; a <= 6; a++){
    digitalWrite(a, LOW);
  }

//１秒長押しで緊急停止、２秒長押しで再始動
  int stopper = digitalRead(stopPin);
  if(stopper == HIGH){
    do{
      uint16_t Vl = 2048;
      uint16_t Vr = 2048;
      Wire.beginTransmission(DEVICE_ADDRESS60);
      Wire.write((Vl>>8)&0x0F);
      Wire.write(Vl);
      Wire.endTransmission();
      Wire.beginTransmission(DEVICE_ADDRESS61);
      Wire.write((Vr>>8)&0x0F);
      Wire.write(Vr);
      Wire.endTransmission();
      delay(2000);//2秒待機
      stopper = digitalRead(stopPin);
    }while(stopper == LOW);
  }
 
//Vx,Vyに出力された電圧を取得する。
  int Vx = analogRead(A0);//Vxの値をアナログピンから読み取る
  int Vy = analogRead(A1);//Vyの値をアナログピンから読み取る
  shiftX(Vx);
  int XX = aveX();
  shiftY(Vy);
  int YY = aveY();

//範囲分け（10bitを3分割、0~1024を0,1,2の3範囲に)
  int s, f;
  s = map(XX, 0, 1024, 0, 3);
  f = map(YY, 0, 1024, 0, 3);

//左折
if((s == 0)&&(f == 2)){
  digitalWrite(5, HIGH); // 点灯
  int Gl = sqrt(pow(443 - XX, 2) + pow(YY - 580, 2)); //体重移動距離Gl
  uint16_t Vl = map(YY, 580, 1023, 2048, 3496);
  uint16_t Vr = map(Gl, 0, 626, 2048, 4095);
  Wire.beginTransmission(DEVICE_ADDRESS60);
  Wire.write((Vl>>8)&0x0F);
  Wire.write(Vl);
  Wire.endTransmission();
  Wire.beginTransmission(DEVICE_ADDRESS61);
  Wire.write((Vr>>8)&0x0F);
  Wire.write(Vr);
  Wire.endTransmission();
  delay(300);
}

//前進
else if((s == 1)&&(f == 2)){
  digitalWrite(2, HIGH); //点灯
  uint16_t Vl = map(YY, 580, 1023, 2048, 4095);
  uint16_t Vr = map(YY, 580, 1023, 2048, 4095);
  Wire.beginTransmission(DEVICE_ADDRESS60);
  Wire.write((Vl>>8)&0x0F);
  Wire.write(Vl);
  Wire.endTransmission();
  Wire.beginTransmission(DEVICE_ADDRESS61);
  Wire.write((Vr>>8)&0x0F);
  Wire.write(Vr);
  Wire.endTransmission();
  delay(300);
}

//右折
else if((s == 2)&&(f == 2)){
  digitalWrite(0, HIGH); //点灯
  int Gr = sqrt(pow(XX - 580, 2) + pow(YY - 580, 2)); //体重移動距離Gr
  uint16_t Vl = map(Gr, 0, 626, 2048, 4095);
  uint16_t Vr = map(YY, 580, 1023, 2048,3496);
  Wire.beginTransmission(DEVICE_ADDRESS60);
  Wire.write((Vl>>8)&0x0F);
  Wire.write(Vl);
  Wire.endTransmission();
  Wire.beginTransmission(DEVICE_ADDRESS61);
  Wire.write((Vr>>8)&0x0F);
  Wire.write(Vr);
  Wire.endTransmission();
  delay(300);
}

//左回転
else if((s == 0)&&(f == 1)){
  digitalWrite(6, HIGH); //点灯
  uint16_t Vl = 416;
  uint16_t Vr = 3679;
  Wire.beginTransmission(DEVICE_ADDRESS60);
  Wire.write((Vl>>8)&0x0F);
  Wire.write(Vl);
  Wire.endTransmission();
  Wire.beginTransmission(DEVICE_ADDRESS61);
  Wire.write((Vr>>8)&0x0F);
  Wire.write(Vr);
  Wire.endTransmission();
  delay(300);
}

//右回転
else if((s == 2)&&(f == 1)){
  digitalWrite(1, HIGH); //点灯
  uint16_t Vl = 3679;
  uint16_t Vr = 416;
  Wire.beginTransmission(DEVICE_ADDRESS60);
  Wire.write((Vl>>8)&0x0F);
  Wire.write(Vl);
  Wire.endTransmission();
  Wire.beginTransmission(DEVICE_ADDRESS61);
  Wire.write((Vr>>8)&0x0F);
  Wire.write(Vr);
  Wire.endTransmission();
  delay(300);
}

//後退
else if((s == 1)&&(f == 0)){
  digitalWrite(4, HIGH); //点灯
  uint16_t Vl = map(YY, 0, 443, 833, 2048);
  uint16_t Vr = map(YY, 0, 443, 833, 2048);
  Wire.beginTransmission(DEVICE_ADDRESS60);
  Wire.write((Vl>>8)&0x0F);
  Wire.write(Vl);
  Wire.endTransmission();
  Wire.beginTransmission(DEVICE_ADDRESS61);
  Wire.write((Vr>>8)&0x0F);
  Wire.write(Vr);
  Wire.endTransmission();
  delay(300);
 }

 //停止
else{
  digitalWrite(3, HIGH); //点灯
  uint16_t Vl = 2048; //入力するVlの値（16bit)
  uint16_t Vr = 2048; //入力するVrの値（16bit)
  Wire.beginTransmission(DEVICE_ADDRESS60);
  Wire.write((Vl>>8)&0x0F);
  Wire.write(Vl);
  Wire.endTransmission();
  Wire.beginTransmission(DEVICE_ADDRESS61);
  Wire.write((Vr>>8)&0x0F);
  Wire.write(Vr);
  Wire.endTransmission();
  delay(1000);
 }
}
