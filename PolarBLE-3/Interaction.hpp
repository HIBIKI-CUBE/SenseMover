#ifndef _Interaction_h
#define _Interaction_h

#include <Arduino.h>

static const uint8_t led = 27;
static const uint8_t button = 13;
static const uint8_t buzzerPin = 23;
static const uint8_t buzzerChannel = 0;

void waitButtonUntil(int v)
{
  while (digitalRead(button) != v)
  {
  }
}

void setupInteraction(){
#define BUZZER_FREQ 12000
#define BUZZER_TIMERBIT 8
  ledcSetup(buzzerChannel, BUZZER_FREQ, BUZZER_TIMERBIT);
  ledcAttachPin(buzzerPin, buzzerChannel);

  pinMode(led, OUTPUT);

}

void note(note_t note = NOTE_C, uint8_t octave = 7, uint32_t duration = 83, uint8_t channel = buzzerChannel)
{
  ledcWriteNote(channel, note, octave);
  delay(duration);
  ledcWriteTone(channel, 0);
}

void blink(int t = 20)
{
  digitalWrite(led, LOW);
  delay(t);
  digitalWrite(led, HIGH);
  note(NOTE_G, 6, t);
  digitalWrite(led, LOW);
}

#endif
