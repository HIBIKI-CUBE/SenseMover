#include <deque> //This won't work on regular Arduino boards. Use it with ESP familly.

//---------------------------------------------------//
// ロードセル　シングルポイント（ ビーム型）　ＳＣ１３３　２０ｋＧ [P-12034]
//---------------------------------------------------//
#define OUT_VOL 0.001f //定格出力 [V]
#define LOAD 20000.0f  //定格容量 [g]

#define LF_DAT 21
#define LF_CLK 22
#define LB_DAT 16
#define LB_CLK 17
#define RF_DAT 18
#define RF_CLK 19
#define RB_DAT 2
#define RB_CLK 4

#define CELL_COUNT 4

struct dataSet
{
  float data[CELL_COUNT];
};

byte dataPins[] = {LF_DAT, LB_DAT, RF_DAT, RB_DAT};
byte clockPins[] = {LF_CLK, LB_CLK, RF_CLK, RB_CLK};
byte invertData[] = {true, false, true, false};

std::deque<dataSet> offsetsBuffer;
std::deque<dataSet> weightsBuffer;

float data[CELL_COUNT];
float offsets[CELL_COUNT];
bool calibrated = false;
float offsetBF = 0;
float offsetRL = 0;
float bf = 0;
float rl = 0;

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

  while (every(cycleDone) != true)
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
          if (cell < 24)
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

void setup()
{
  Serial.begin(115200);
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
  delay(1000);
  for (byte i = 0; i < CELL_COUNT; i++)
  {
    char S1[20];
    char s[20];
    sprintf(S1, "%s", dtostrf(-(offsets[i]), 5, 3, s));
    Serial.print(S1);
    if (i < CELL_COUNT - 1)
    {
      Serial.print(",");
    }
    else
    {
      Serial.println("");
    }
  }

  delay(1000);
}

void loop()
{
  struct dataSet weights;
  putWeightsTo(weights.data);
  weightsBuffer.push_back(weights);
  if (weightsBuffer.size() < 10)
    return;

  for (byte cell = 0; cell < CELL_COUNT; cell++)
  {
    float ave = 0;
    byte excludeIndex = 0;
    byte minIndex = 0;
    for (byte i = 0; i < weightsBuffer.size(); i++)
    {
      if(invertData[cell]){
        excludeIndex = weightsBuffer[i].data[cell] < weightsBuffer[excludeIndex].data[cell] ? i : excludeIndex;
      }else{
        excludeIndex = weightsBuffer[i].data[cell] > weightsBuffer[excludeIndex].data[cell] ? i : excludeIndex;
      }
      ave += weightsBuffer[i].data[cell];
    }
    ave -= weightsBuffer[excludeIndex].data[cell];
    ave /= (weightsBuffer.size() - 1);
    data[cell] = (ave - offsets[cell]) * (invertData[cell] ? 1 : -1);
  }
  weightsBuffer.pop_back();

  bf = ((data[0] + data[2]) / 2) - ((data[1] + data[3]) / 2) - offsetBF;
  rl = ((data[2] + data[3]) / 2) - ((data[0] + data[1]) / 2) - offsetRL;
  if (!calibrated)
  {
    offsetBF = bf;
    offsetRL = rl;
    calibrated = true;
  }
  Serial.print(bf);
  Serial.print(",");
  Serial.println(rl);
}
