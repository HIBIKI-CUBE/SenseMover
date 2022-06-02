#include <Wire.h> //I2C用通信用ライブラリ先頭文
#include <math.h>

//前後のセンサーのキャリブレーションとテストを行います。
//プログラム開始時にキャリブレーションを行います。
//通常の姿勢で1度ボタンを押します。これで中心のキャリブレーションを行います。
//前進を開始する姿勢で1度ボタンを押します。これで前後のキャリブレーションを行います。
//最後に、最速前進時の姿勢で1度ボタンを押します。これで前後最大のキャリブレーションを行います。
//その後はセンサーのテストができます。ボタンを押している間、認識している方向を音で示します。
//前進は音階が上がり、後退は音階が下がります。中立時は同じ音を繰り返します。

static const uint8_t t = 83;
static const uint8_t buzzer = 12;
static const uint8_t button = 8;
static const uint8_t rightAndLeft = A0;
static const uint8_t backAndForth = A2;
static const uint8_t Left_Motor = 0x61;  //デバイスアドレス（スレーブ）
static const uint8_t Right_Motor = 0x60; //
int centerRL = 511;
int centerBF = 511;
int deltaMaxRL = 150;
int deltaMaxBF = 150;
int deltaMinRL = 50;
int deltaMinBF = 50;

void waitButtonUntil(int v)
{
  while (digitalRead(button) != v)
  {
  }
}

void setup()
{
  Serial.begin(9600);
  Wire.begin();
  Wire.setClock(400000);
  pinMode(button, INPUT);
  pinMode(13, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(7, INPUT);

  waitButtonUntil(HIGH);
  centerBF = analogRead(backAndForth);
  centerRL = analogRead(rightAndLeft) - (abs(analogRead(backAndForth) - 511) * 0.33);
  tone(buzzer, 1047, t);
  delay(t);
  waitButtonUntil(LOW);
  delay(50);

  while (digitalRead(button) == LOW)
  {
    if (analogRead(backAndForth) <= centerBF || 1023 <= analogRead(backAndForth))
    {
      tone(buzzer, 1865, t);
      delay(t);
    }
  }
  deltaMinBF = analogRead(backAndForth) - centerBF;
  deltaMinRL = analogRead(rightAndLeft) - centerRL;
  tone(buzzer, 1319, t);
  delay(t);
  waitButtonUntil(LOW);
  delay(50);

  while (digitalRead(button) == LOW)
  {
    Serial.print(1023);
    Serial.print(", ");
    Serial.print(0);
    Serial.print(", ");
    Serial.print(511);
    Serial.print(", ");
    Serial.print(analogRead(backAndForth));
    Serial.print(", ");
    Serial.println(analogRead(rightAndLeft) - (abs(analogRead(backAndForth) - 511) * 0.33));
    if (analogRead(backAndForth) <= centerBF || 1023 <= analogRead(backAndForth))
    {
      tone(buzzer, 1865, t);
      delay(t);
    }
  }
  deltaMaxRL = analogRead(rightAndLeft) - centerRL;
  deltaMaxBF = analogRead(backAndForth) - centerBF;
  tone(buzzer, 1568, t);
  delay(t);
  waitButtonUntil(LOW);

  while (!(-deltaMinBF < analogRead(backAndForth) - centerBF && analogRead(backAndForth) - centerBF < deltaMinBF && -deltaMinRL < analogRead(rightAndLeft) - centerRL && analogRead(rightAndLeft) - centerRL < deltaMinRL))
  {
  }

  tone(buzzer, 1047, t);
  delay(t);
  tone(buzzer, 1319, t);
  delay(t);
  tone(buzzer, 1568, t);
}

unsigned long startDelay = 0;
bool active = false;
#define rcGain 0.3
float rlRC = 2047;
float bfRC = 2047;

void loop()
{
  uint16_t Vl = 2047;
  uint16_t Vr = 2047;
  int bf = 0;
  int radius = 0;
  int theta = 0;
  int rl = 0;
  bf = analogRead(backAndForth) - centerBF;
  rl = analogRead(rightAndLeft) - (abs(analogRead(backAndForth) - 511) * 0.33) - centerRL;

  radius = sqrt(pow(bf, 2) + pow(rl, 2));
  theta = atan2(rl, bf) * 180.0 / PI;

  // Serial.print(radius);
  // Serial.print(", ");
  Serial.println(theta);
  // rlRC = rcGain * rlRC + (1 - rcGain) * (float)rl;
  // bfRC = rcGain * bfRC + (1 - rcGain) * (float)bf;
  // Serial.print(", ");
  // Serial.print(bfRC);
  // Serial.print(", ");
  // Serial.println(rlRC);

  if (digitalRead(button) == HIGH)
  {
    active = !active;
    if (active)
    {
      digitalWrite(10, HIGH);
      digitalWrite(13, HIGH);
    }
    else
    {
      digitalWrite(10, LOW);
      digitalWrite(13, LOW);
    }
    tone(buzzer, 1047, t);
    delay(t);
    waitButtonUntil(LOW);
  }

  if (active)
  {
    if (digitalRead(7) == HIGH)
    {
      if (millis() - startDelay >= 1000)
      {
        tone(buzzer, 987, 500);
        startDelay = millis();
      }
      Vl = 2600;
      Vr = Vl;
    }
    else
    {
      if (radius >= deltaMinBF)
      {
        Vl = map(radius, deltaMinBF, deltaMaxBF, 2047, 4095);
        Vr = Vl;
        if (abs(theta) <= 150)
        {
          if (theta >= 0)
          { //右折
            Vr -= (Vr - 2047.5) * (((float)map(abs(constrain(theta, -90, 90)), 0, 45, 0, 100)) / 100);
          }
          else
          { //左折
            Vl -= (Vl - 2047.5) * (((float)map(abs(constrain(theta, -90, 90)), 0, 45, 0, 100)) / 100);
          }
        }
        else
        {
          Vl = map(Vl, 2047, 4095, 2047, 410);
          Vr = map(Vr, 2047, 4095, 2047, 410);
          if (millis() - startDelay >= 1000)
          {
            tone(buzzer, 987, 500);
            startDelay = millis();
          }
        }
      }
    }
  }
  else
  {
    Vl = 2047;
    Vr = 2047;

    if (millis() - startDelay >= 250)
    {
      startDelay = millis();
    }
  }

  // if (abs(Vl - 2047) <= 450)
  // {
  //   Vl = 2047;
  // }
  // if (abs(Vr - 2047) <= 450)
  // {
  //   Vr = 2047;
  // }
  Vl = constrain(Vl, 410, 4095);
  Vr = constrain(Vr, 410, 4095);

  Wire.beginTransmission(Left_Motor);
  Wire.write((Vl >> 8) & 0x0F);
  Wire.write(Vl);
  Wire.endTransmission();
  Wire.beginTransmission(Right_Motor);
  Wire.write((Vr >> 8) & 0x0F);
  Wire.write(Vr);
  Wire.endTransmission();
}
