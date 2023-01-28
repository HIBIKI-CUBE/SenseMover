#ifndef _LoadCell_h
#define _LoadCell_h

#include <deque> //This won't work on regular Arduino boards. Use it with ESP family.
#include <numeric>
#include "./Interaction.hpp"

#define OUT_VOL 0.001f // 定格出力 [V]
#define LOAD 20000.0f  // 定格容量 [g]

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
uint8_t calibrationTimer = 3;
unsigned long lastCalibrationFrame = 0;
int centerRL = 0;
int centerBF = 0;
int deltaMaxRL = 1400;
int deltaMaxBF = 1400;
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

  loadCellSetupSound();
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

void calibrate()
{
  if (calibrationTimer == 3 || millis() - lastCalibrationFrame >= 950)
  {
    if (calibrationTimer > 0)
    {
      note(NOTE_G, 6);
      lastCalibrationFrame = millis();
      --calibrationTimer;
    }
    else
    {
      centerBF = getCoG().bf;
      centerRL = getCoG().rl;
      calibratedSound();
      calibrated = true;
      calibrating = false;
      calibrationTimer = 3;
    }
  }
}

#endif
