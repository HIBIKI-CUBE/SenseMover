#include <math.h>            //各種算術計算用ライブラリ
#include "./Interaction.hpp" //UIモジュール
#include "./Bluetooth.hpp"   //Bluetooth®通信モジュール
#include "./LoadCell.hpp"    //体重移動モジュール
#include "./LiDAR.hpp"       //衝突保護モジュール

//各種ピン設定
static const uint8_t activeR = 32;
static const uint8_t activeL = 33;
static const uint8_t rightMotor = 25;
static const uint8_t leftMotor = 26;

//各種変数定義
int radius = 0;
int theta = 0;
float vLeft = 127;
float vRight = 127;
static const float accelDefault = 0.0512; // 50 rpm / ms | 256 / 300 * 30 / 1000
float accel = accelDefault;
unsigned long lastTime = 0;
bool backSignRunning = false;
unsigned long backSignDelay = 0;
bool isEmergency = false;
unsigned long lastEmergency = 0;
unsigned long lastUnlock = 0;
int vlTarget = 127;
int vrTarget = 127;
int8_t elapsed = 0;
struct resultLiDAR safety;
struct CoG CoG;

// 起動時セットアップ
void setup()
{
  setupInteraction();
  pinMode(activeR, OUTPUT);
  pinMode(activeL, OUTPUT);

  digitalWrite(activeL, LOW);
  digitalWrite(activeR, LOW);
  dacWrite(leftMotor, 127);
  dacWrite(rightMotor, 127);

  Serial.begin(115200);

  setupLiDAR();
  setupLoadCell();
  BluetoothSetup();
}

// メインループ
void loop()
{
  vlTarget = 127;
  vrTarget = 127;
  switch (bleMode)
  {
  case 0:
    // リモートモードならばBluetoothで取得した値を操縦の値として設定
    radius = bleDistance;
    theta = bleAngle;
    break;
  case 1:
    // ライドモードならば体重移動の位置を取得し操縦の値として設定
    CoG = getCoG();
    radius = CoG.radius;
    theta = CoG.theta;

    // 前回の送信から所定の時間が経過していれば体重移動の位置を送信
    if (millis() - lastBleSend >= 5)
    {
      pCharacteristic->setValue((String(radius, DEC) + "," + String(theta, DEC)).c_str());
      pCharacteristic->notify();
      lastBleSend = millis();
    }
    break;
  }
  if (calibrating)
  {
    // キャリブレーション中であればキャリブレーションを実行
    calibrate();
  }

  // 操作に応じてモーターの回転可能状態を切り替え {
  if (activeToggled)
  {
    if (active)
    {
      note(NOTE_C);
      note(NOTE_G);
    }
    else
    {
      note(NOTE_E);
      note(NOTE_C);
    }
    activeToggled = false;
  }

  if (active)
  {
    digitalWrite(activeR, HIGH);
    digitalWrite(activeL, HIGH);
  }
  else
  {
    digitalWrite(activeR, LOW);
    digitalWrite(activeL, LOW);
    isEmergency = false;
    accel = accelDefault;
  }
  // }

  // 必要な条件が揃っていれば操縦操作を反映
  if (active && ((bleMode == 0 && millis() - lastBleCommand <= BleCallbacks::bleWaitDuration) || (bleMode == 1 && calibrated)))
  {
    // 操縦操作の半径が規定よりも大きいか判定
    if (radius >= (bleMode == 1 ? deltaMinBF : 50))
    {
      // 操縦操作の半径を速度に変換
      vlTarget = map(radius, bleMode == 0 ? 0 : centerBF, bleMode == 0 ? 1000 : deltaMaxBF, 128, 60 <= abs(theta) && abs(theta) <= 120 ? 192 : 255);
      vrTarget = vlTarget;
      if (abs(theta) <= 120)
      {// 前進または旋回
        if (theta >= 0)
        { // 右折または右旋回
          vrTarget -= (vrTarget - 127) * (((float)map((theta >= 60 ? 90 : theta), 0, 60, 0, 100)) / 100);
        }
        else
        { // 左折または左旋回
          vlTarget -= (vlTarget - 127) * (((float)map((theta <= -60 ? 90 : -theta), 0, 60, 0, 100)) / 100);
        }
      }
      else
      { // 後退
        vlTarget = map(vlTarget, 127, 255, 127, 25);
        vrTarget = map(vrTarget, 127, 255, 127, 25);
        if (theta > 0)
        { // 右折
          vrTarget -= (vrTarget - 127) * (((float)map(theta, 120, 180, 100, 0)) / 100);
        }
        else
        { // 左折
          vlTarget -= (vlTarget - 127) * (((float)map(-theta, 120, 180, 100, 0)) / 100);
        }
        if (!backSignRunning && millis() - backSignDelay >= 500)
        { // 後退時の音を再生
          ledcWriteNote(buzzerChannel, NOTE_E, 5);
          backSignRunning = true;
          backSignDelay = millis();
        }
      }

      // 速度モードに応じた制限
      vlTarget = (vlTarget - 127) / (fast ? 1 : 3.0) + 127;
      vrTarget = (vrTarget - 127) / (fast ? 1 : 3.0) + 127;
    }
  }
  else
  { // 条件が揃っていない場合は停止
    vlTarget = 127;
    vrTarget = 127;
  }

  // 衝突保護 {
  safety = LiDAR(vlTarget, vrTarget, 1, 350, 150);
  vlTarget = safety.vLeft;
  vrTarget = safety.vRight;
  if (active && (digitalRead(button) == HIGH || isEmergency || safety.status == caution || safety.status == stop))
  {
    if (safety.status == caution && (lidarFront || lidarSide))
    {
      note(NOTE_Gs);
    }
    if (safety.status == stop)
    {
      note(NOTE_Bb);
    }

    if (digitalRead(button) == HIGH || safety.status == stop)
    {
      isEmergency = true;
      note(NOTE_Bb);
      accel = 0.427;
      vlTarget = 127;
      vrTarget = 127;
    }
  }
  // }

  // 前回ループからの経過時間を取得
  elapsed = millis() - lastTime;
  // 加速度に応じてモーターの速度を設定
  if (elapsed >= 10)
  {
    if (vlTarget > vLeft)
    {
      vLeft += accel * elapsed;
    }
    else if (vlTarget < vLeft)
    {
      vLeft -= accel * elapsed;
    }

    if (vrTarget > vRight)
    {
      vRight += accel * elapsed;
    }
    else if (vrTarget < vRight)
    {
      vRight -= accel * elapsed;
    }

    lastTime = millis();
  }

  // 緊急停止中は確実に制動
  if (isEmergency)
  {
    vLeft = 127;
    vRight = 127;
  }

  // ライドモードで体重が1kgよりも軽くなったら走行を停止
  if (active && bleMode == 1 && CoG.weight <= 1000)
  {
    active = false;
    activeToggled = true;
  }

  vLeft = constrain(vLeft, 25, 255);
  vRight = constrain(vRight, 25, 255);

  // モーターの速度をモーターコントローラーに指示
  dacWrite(leftMotor, vLeft);
  dacWrite(rightMotor, vRight);

  // 後退時の音の制御
  if (backSignRunning && millis() - backSignDelay >= 500)
  {
    ledcWriteTone(buzzerChannel, 0);
    backSignRunning = false;
    backSignDelay = millis();
  }
}
