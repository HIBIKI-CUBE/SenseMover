#ifndef _SuperSonic_h
#define _SuperSonic_h

#define PIN_TRIG 5
#define PIN_ECHO 35

#include <deque> //This won't work on regular Arduino boards. Use with ESP family.
#include "./Interaction.hpp"

namespace
{
  static const uint8_t centerSonicTrig = PIN_TRIG;
  static const uint8_t centerSonicEcho = PIN_ECHO;
  float speed = 0;
  float lastDistance = 400; // cm
  unsigned long lastSonic = 0;
  float distance = 400;
  int stopSignals = 0;
  int warnSignals = 0;
  // float speeds[10];
  // int index = 0;

  std::deque<float> speeds;
}

void setupSuperSonic()
{
  pinMode(centerSonicTrig, OUTPUT);
  pinMode(centerSonicEcho, INPUT);
}

int sonic()
{
  int result = 0;
  //0: normal
  //1: Warn
  //2: Stop
  do
  {
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);
    lastDistance = distance;
    float elapsed = millis() - lastSonic;
    elapsed = elapsed == 0 ? 1 : elapsed;
    distance = pulseIn(PIN_ECHO, HIGH) / 58.0;
    Serial.println(distance);
    speeds.push_back((lastDistance - distance) / elapsed);
  } while (speeds.size() < 10);
  for (int i = 0; i < 10; i++)
  {
    speed = speed + speeds[i];
  }
  speed /= 10;
  speeds.pop_back();
  lastSonic = millis();

  if (distance - speed * 1600 <= 15)
  {
    if (warnSignals > 10)
    {
      result = 1;
      note(NOTE_Bb);
    }
    else
    {
      warnSignals++;
    }
  }
  else
  {
    warnSignals = 0;
  }

  if (distance - speed * 600 <= 15)
  {
    if (stopSignals > 10)
    {
      result = 2;
      note(NOTE_Bb);
    }
    else
    {
      stopSignals++;
    }
  }
  else
  {
    stopSignals = 0;
  }

  return result;
}

#endif
