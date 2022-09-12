#include <math.h>
#include "./Interaction.hpp"
#include "./Bluetooth.hpp"
#include "./LoadCell.hpp"

static const uint8_t activeR = 32;
static const uint8_t activeL = 33;
static const uint8_t rightMotor = 25;
static const uint8_t leftMotor = 26;
static const uint8_t rightAndLeft = 34;
static const uint8_t backAndForth = 35;
int radius = 0;
int theta = 0;
unsigned long startDelay = 0;
bool backSignRunning = false;
unsigned long backSignDelay = 0;

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

  BluetoothSetup();
  setupLoadCell();

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
  switch (bleMode)
  {
  case 0:
    radius = bleDistance;
    theta = bleAngle;
    break;
  case 1:
    struct CoG CoG = getCoG();
    // rl = analogReadMilliVolts(rightAndLeft) - (abs(analogRead(backAndForth) - 511) * 0.33) - centerRL;
    radius = sqrt(pow(CoG.bf, 2) + pow(CoG.rl, 2));
    theta = atan2(CoG.rl, CoG.bf) * 180.0 / PI;
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
  // note(NOTE_E, 4);

  if (backSignRunning && millis() - backSignDelay >= 500)
  {
    ledcWriteTone(buzzerChannel, 0);
    digitalWrite(led, LOW);
    backSignRunning = false;
    backSignDelay = millis();
  }
}
