#include <math.h>
#include "./Interaction.hpp"
#include "./Bluetooth.hpp"

static const uint8_t button = 13;
static const uint8_t activeR = 32;
static const uint8_t activeL = 33;
static const uint8_t rightMotor = 25;
static const uint8_t leftMotor = 26;
static const uint8_t rightAndLeft = 34;
static const uint8_t backAndForth = 35;
int adMin = 142;
int adMax = 3150;
int adCenter = (adMax - adMin) / 2;
int daMin = 0;
int daMax = 255;
int daCenter = (daMax - daMin) / 2;
int centerRL = 511;
int centerBF = 511;
int deltaMaxRL = 150;
int deltaMaxBF = 150;
int deltaMinRL = 50;
int deltaMinBF = 50;
int radius = 0;
int theta = 0;
bool calibrated = false;
unsigned long startDelay = 0;
bool backSignRunning = false;
unsigned long backSignDelay = 0;
enum CalibrationPhase
{
  CENTER,
  MIN,
  MAX,
  FINISH
};
CalibrationPhase calibrationPhase = CENTER;

void waitButtonUntil(int v)
{
  while (digitalRead(button) != v)
  {
  }
}

void monitor()
{
  int bf = analogReadMilliVolts(backAndForth) - centerBF;
  int rl = analogReadMilliVolts(rightAndLeft) - centerRL;
  radius = sqrt(pow(bf, 2) + pow(rl, 2));
  theta = atan2(rl, bf) * 180.0 / PI;
}

void calibrate()
{
  switch (calibrationPhase)
  {
  case CENTER:
    centerBF = adCenter;
    centerRL = adCenter;
    if (digitalRead(button) == HIGH)
    {
      centerBF = analogReadMilliVolts(backAndForth);
      centerRL = analogReadMilliVolts(rightAndLeft);
      note(NOTE_C);
      waitButtonUntil(LOW);
      delay(50);
      calibrationPhase = MIN;
    }
    if (analogReadMilliVolts(backAndForth) <= adMin || adMax <= analogReadMilliVolts(backAndForth))
    {
      note(NOTE_Bb);
    }
    break;
  case MIN:
    if (digitalRead(button) == HIGH)
    {
      deltaMinBF = analogReadMilliVolts(backAndForth) - centerBF;
      deltaMinRL = analogReadMilliVolts(rightAndLeft) - centerRL;
      note(NOTE_E);
      waitButtonUntil(LOW);
      delay(50);
      calibrationPhase = MAX;
    }
    if (analogReadMilliVolts(backAndForth) <= centerBF || adMax <= analogReadMilliVolts(backAndForth))
    {
      note(NOTE_Bb);
    }
    break;
  case MAX:
    if (digitalRead(button) == HIGH)
    {
      deltaMaxBF = analogReadMilliVolts(backAndForth) - centerBF;
      deltaMaxRL = analogReadMilliVolts(rightAndLeft) - centerRL;
      note(NOTE_G);
      waitButtonUntil(LOW);
      calibrationPhase = FINISH;
    }
    if (analogReadMilliVolts(backAndForth) <= centerBF || adMax <= analogReadMilliVolts(backAndForth))
    {
      note(NOTE_Bb);
    }
    break;
  case FINISH:
    if (-deltaMinBF < analogReadMilliVolts(backAndForth) - centerBF && analogReadMilliVolts(backAndForth) - centerBF < deltaMinBF && -deltaMinRL < analogReadMilliVolts(rightAndLeft) - centerRL && analogReadMilliVolts(rightAndLeft) - centerRL < deltaMinRL)
    {
      note(NOTE_C, 7);
      note(NOTE_E, 7);
      note(NOTE_G, 7);
      calibrating = false;
      calibrated = true;
      calibrationPhase = CENTER;
    }
    else
    {
      Serial.print(adMax);
      Serial.print(", ");
      Serial.print(adCenter);
      Serial.print(", ");
      Serial.print(adMin);
      Serial.print(", ");
      Serial.print(analogReadMilliVolts(backAndForth));
      Serial.print(", ");
      Serial.println(analogReadMilliVolts(rightAndLeft) - abs((float)analogReadMilliVolts(backAndForth) - 1575) * 0.5);
    }
    break;
  }
}

void setup()
{
  setupInteraction();
  pinMode(activeR, OUTPUT);
  pinMode(activeL, OUTPUT);

  analogSetAttenuation(ADC_11db);
  pinMode(rightAndLeft, ANALOG);
  pinMode(backAndForth, ANALOG);

  digitalWrite(activeL, LOW);
  digitalWrite(activeR, LOW);
  dacWrite(leftMotor, 127);
  dacWrite(rightMotor, 127);

  Serial.begin(115200);

  BluetoothSetup();

  if (bleMode == 1)
  {
    calibrating = true;
  }

  note(NOTE_C, 7);
  note(NOTE_E, 7);
  note(NOTE_G, 7);
}

void loop()
{
  uint16_t Vl = 128;
  uint16_t Vr = 128;
  int bf = 0;
  int rl = 0;

  switch (bleMode)
  {
  case 0:
    radius = bleDistance;
    theta = bleAngle;
    break;
  case 1:
    bf = analogReadMilliVolts(backAndForth) - centerBF;
    rl = analogReadMilliVolts(rightAndLeft) - centerRL;
    // rl = analogReadMilliVolts(rightAndLeft) - (abs(analogRead(backAndForth) - 511) * 0.33) - centerRL;
    radius = sqrt(pow(bf, 2) + pow(rl, 2));
    theta = atan2(rl, bf) * 180.0 / PI;
    break;
  }
  if (bleMode == 1 && millis() - bleReadDelay >= 5)
  {
    pCharacteristic->setValue((String(radius, DEC) + "," + String(theta, DEC)).c_str());
    pCharacteristic->notify();
    bleReadDelay = millis();
  }
  if (calibrating)
  {
    calibrate();
  }

  if (activeToggled)
  {
    if (active)
    {
      digitalWrite(activeR, HIGH);
      digitalWrite(activeL, HIGH);
      note(NOTE_C);
      note(NOTE_G);
    }
    else
    {
      digitalWrite(activeR, LOW);
      digitalWrite(activeL, LOW);
      note(NOTE_E);
      note(NOTE_C);
    }
    activeToggled = false;
  }

  if (active && (((bleMode == 0 || bleMode == 2) && millis() - lastBleCommand <= BleCallbacks::bleWaitDuration * 2) || (bleMode == 1 && calibrated)))
  {
    if (radius >= (bleMode == 1 ? deltaMinBF : 50))
    {
      switch (bleMode)
      {
      case 0:
        Vl = map(radius, 0, 1000, 128, 60 <= abs(theta) && abs(theta) <= 120 ? 192 : 255);
        Vr = Vl;
        break;
      case 1:
        Vl = map(radius, centerBF, deltaMaxBF, 128, 60 <= abs(theta) && abs(theta) <= 120 ? 192 : 255);
        Vr = Vl;
        break;
      }
      if (abs(theta) <= 120)
      {
        if (theta >= 0)
        { //右折
          Vr -= (Vr - 127.5) * (((float)map((theta >= 60 ? 90 : theta), 0, 60, 0, 100)) / 100);
        }
        else
        { //左折
          Vl -= (Vl - 127.5) * (((float)map((theta <= -60 ? 90 : -theta), 0, 60, 0, 100)) / 100);
        }
      }
      else
      {
        Vl = map(Vl, 127, 255, 127, 25);
        Vr = map(Vr, 127, 255, 127, 25);
        if (theta > 0)
        {
          Vr -= (Vr - 127) * (((float)map(theta, 120, 180, 100, 0)) / 100);
        }
        else
        {
          Vl -= (Vl - 127) * (((float)map(-theta, 120, 180, 100, 0)) / 100);
        }
        if (!backSignRunning && millis() - backSignDelay >= 500)
        {
          ledcWriteNote(buzzerChannel, NOTE_E, 5);
          digitalWrite(led, HIGH);
          backSignRunning = true;
          backSignDelay = millis();
        }
      }
    }
  }
  else
  {
    Vl = 127;
    Vr = 127;

    if (millis() - startDelay >= 250)
    {
      startDelay = millis();
    }
  }
  Vl = constrain(Vl, 25, 255);
  Vr = constrain(Vr, 25, 255);

  dacWrite(leftMotor, Vl);
  dacWrite(rightMotor, Vr);

  if (backSignRunning && millis() - backSignDelay >= 500)
  {
    ledcWriteTone(buzzerChannel, 0);
    digitalWrite(led, LOW);
    backSignRunning = false;
    backSignDelay = millis();
  }
}
