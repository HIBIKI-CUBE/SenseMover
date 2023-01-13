#ifndef _LoadCell_h
#define _LoadCell_h

#include <deque> //This won't work on regular Arduino boards. Use it with ESP family.
#include <numeric>
#include "./Interaction.hpp"

//---------------------------------------------------//
// ロードセル　シングルポイント（ ビーム型）　ＳＣ１３３　２０ｋＧ [P-12034]
//---------------------------------------------------//
#define OUT_VOL 0.001f //定格出力 [V]
#define LOAD 20000.0f  //定格容量 [g]

#define LF_DAT 21
#define LF_CLK 22
#define LB_DAT 14
#define LB_CLK 27
#define RF_DAT 18
#define RF_CLK 19
#define RB_DAT 5
#define RB_CLK 4

#define CELL_COUNT 4

struct dataSet
{
  float data[CELL_COUNT];
};

struct CoG
{
  float bf = 0;
  float rl = 0;
  float weight = 0;
};

byte dataPins[] = {LF_DAT, LB_DAT, RF_DAT, RB_DAT};
byte clockPins[] = {LF_CLK, LB_CLK, RF_CLK, RB_CLK};
byte invertData[] = {true, true, false, false};

std::deque<dataSet> offsetsBuffer;
std::deque<dataSet> weightsBuffer;

float offsets[CELL_COUNT];
bool calibrated = false;
int centerRL = 0;
int centerBF = 0;
int deltaMaxRL = 1500;
int deltaMaxBF = 1500;
int deltaMinRL = 120;
int deltaMinBF = 120;
float bf = 0;
float rl = 0;
bool calibrating = false;

bool every(bool boolArray[], bool equals = true)
{
  for (byte i = 0; i < CELL_COUNT; i++)
  {
    if (boolArray[i] != equals)
    {
      return false;
    }
  }
  return true;
}

void putWeightsTo(float *weights)
{
// #define HX711_R1 20000.0f
// #define HX711_R2 8200.0f
// #define HX711_VBG 1.25f
#define HX711_AVDD 4.2987f                  //(HX711_VBG*((HX711_R1+HX711_R2)/HX711_R2))
#define HX711_ADC1bit HX711_AVDD / 16777216 // 16777216=(2^24)
#define HX711_PGA 128
#define HX711_SCALE (OUT_VOL * HX711_AVDD / LOAD * HX711_PGA)

  bool cycleDone[CELL_COUNT];
  for (byte cell = 0; cell < CELL_COUNT; cell++)
  {
    cycleDone[cell] = false;
  }

  while (!every(cycleDone))
  {
    for (byte cell = 0; cell < CELL_COUNT; cell++)
    {
      if (!cycleDone[cell] && digitalRead(dataPins[cell]) == LOW)
      {
        long data = 0;
        delayMicroseconds(1);
        for (byte i = 0; i < 25; i++)
        {
          digitalWrite(clockPins[cell], HIGH);
          delayMicroseconds(1);
          digitalWrite(clockPins[cell], LOW);
          delayMicroseconds(1);
          if (i < 24)
          {
            data = (data << 1) | (digitalRead(dataPins[cell]));
          }
        }
        weights[cell] = (data ^ 0x800000) * HX711_ADC1bit / HX711_SCALE;
        cycleDone[cell] = true;
      }
    }
  }
}

void setupLoadCell()
{
  for (byte i = 0; i < CELL_COUNT; i++)
  {
    pinMode(dataPins[i], INPUT);
    pinMode(clockPins[i], OUTPUT);
  }

  // Reset HX711
  for (byte i = 0; i < CELL_COUNT; i++)
  {
    digitalWrite(clockPins[i], HIGH);
  }
  delayMicroseconds(1);
  for (byte i = 0; i < CELL_COUNT; i++)
  {
    digitalWrite(clockPins[i], LOW);
  }
  // Calibrate
  for (byte i = 0; i < 10; i++)
  {
    putWeightsTo(offsets);
  }

  note(NOTE_C, 7);
  note(NOTE_E, 7);
  note(NOTE_G, 7);
}

CoG getCoG()
{
  struct CoG result;
  float cellRawData[CELL_COUNT];
  do
  {
    struct dataSet weights;
    putWeightsTo(weights.data);
    weightsBuffer.push_back(weights);
  } while (weightsBuffer.size() < 10);

  for (byte cell = 0; cell < CELL_COUNT; cell++)
  {
    float ave = 0;
    byte excludeIndex = 0;
    for (byte i = 0; i < weightsBuffer.size(); i++)
    {
      if (invertData[cell])
      {
        excludeIndex = weightsBuffer[i].data[cell] < weightsBuffer[excludeIndex].data[cell] ? i : excludeIndex;
      }
      else
      {
        excludeIndex = weightsBuffer[i].data[cell] > weightsBuffer[excludeIndex].data[cell] ? i : excludeIndex;
      }
      ave += weightsBuffer[i].data[cell];
    }
    ave -= weightsBuffer[excludeIndex].data[cell];
    ave /= (weightsBuffer.size() - 1);
    cellRawData[cell] = (ave - offsets[cell]) * (invertData[cell] ? 1 : -1);
  }
  weightsBuffer.pop_back();

  result.bf = ((cellRawData[0] + cellRawData[2]) / 2) - ((cellRawData[1] + cellRawData[3]) / 2) - centerBF;
  result.rl = ((cellRawData[2] + cellRawData[3]) / 2) - ((cellRawData[0] + cellRawData[1]) / 2) - centerRL;
  result.weight = std::accumulate(cellRawData, cellRawData + CELL_COUNT, 0);
  return result;
}

enum CalibrationPhase
{
  CENTER,
  MIN,
  MAX,
  FINISH
};
CalibrationPhase calibrationPhase = CENTER;

void calibrate()
{
  switch (calibrationPhase)
  {
  case CENTER:
    if (digitalRead(button) == HIGH)
    {
      centerBF = getCoG().bf;
      centerRL = getCoG().rl;
      note(NOTE_C);
      waitButtonUntil(LOW);
      delay(50);
      calibrationPhase = FINISH;
    }
    break;
  case MIN:
    if (digitalRead(button) == HIGH)
    {
      deltaMinBF = getCoG().bf;
      deltaMinRL = getCoG().rl;
      note(NOTE_E);
      waitButtonUntil(LOW);
      delay(50);
      calibrationPhase = MAX;
    }
    if (getCoG().bf <= 0 || getCoG().rl <= 0)
    {
      note(NOTE_Bb);
    }
    break;
  case MAX:
    if (digitalRead(button) == HIGH)
    {
      deltaMaxBF = getCoG().bf;
      deltaMaxRL = getCoG().rl;
      note(NOTE_G);
      waitButtonUntil(LOW);
      calibrationPhase = FINISH;
    }
    if (getCoG().bf <= deltaMinBF || getCoG().rl <= deltaMinRL)
    {
      note(NOTE_Bb);
    }
    break;
  case FINISH:
    if (-deltaMinBF <= getCoG().bf && getCoG().bf <= deltaMinBF && -deltaMinRL <= getCoG().rl && getCoG().rl <= deltaMinRL)
    {
      note(NOTE_C, 7);
      note(NOTE_E, 7);
      note(NOTE_G, 7);
      Serial.print("Weight: ");
      Serial.println(getCoG().weight);
      Serial.print(", centerBF: ");
      Serial.println(centerBF);
      Serial.print(", centerRL: ");
      Serial.print(centerRL);
      Serial.print(", deltaMinBF: ");
      Serial.print(deltaMinBF);
      Serial.print(", deltaMinRL: ");
      Serial.print(deltaMinRL);
      Serial.print(", deltaMaxBF: ");
      Serial.print(deltaMaxBF);
      Serial.print(", deltaMaxRL: ");
      Serial.println(deltaMaxRL);
      calibrating = false;
      calibrated = true;
      calibrationPhase = CENTER;
    }
    break;
  }
}

#endif
