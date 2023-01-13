#ifndef _Interaction_h
#define _Interaction_h

#include <Arduino.h>

static const uint8_t button = 2;
static const uint8_t buzzerPin = 23;
static const uint8_t buzzerChannel = 0;

void waitButtonUntil(int v)
{
  while (digitalRead(button) != v)
  {
  }
}

void setupInteraction()
{
#define BUZZER_FREQ 12000
#define BUZZER_TIMERBIT 8
  ledcSetup(buzzerChannel, BUZZER_FREQ, BUZZER_TIMERBIT);
  ledcAttachPin(buzzerPin, buzzerChannel);
}

void note(note_t note = NOTE_C, uint8_t octave = 7, uint32_t duration = 83, uint8_t channel = buzzerChannel)
{
  ledcWriteNote(channel, note, octave);
  delay(duration);
  ledcWriteTone(channel, 0);
}

void blink(int t = 20)
{
  // digitalWrite(led, LOW);
  delay(t);
  // digitalWrite(led, HIGH);
  note(NOTE_G, 6, t);
  // digitalWrite(led, LOW);
}

void bluetoothConnected()
{
  note(NOTE_D, 6, 27);
  note(NOTE_A, 6, 27);
  note(NOTE_D, 7, 27);
  note(NOTE_A, 7, 27);
  note(NOTE_D, 8, 27);
  note(NOTE_Eb, 6, 27);
  note(NOTE_Bb, 6, 27);
  note(NOTE_Eb, 7, 27);
  note(NOTE_Bb, 7, 27);
  note(NOTE_Eb, 8, 27);
  note(NOTE_E, 6, 27);
  note(NOTE_B, 6, 27);
  note(NOTE_E, 7, 27);
  note(NOTE_B, 7, 27);
  note(NOTE_E, 8, 27);
  note(NOTE_F, 6, 27);
  note(NOTE_C, 6, 27);
  note(NOTE_F, 7, 27);
  note(NOTE_C, 7, 27);
  note(NOTE_F, 8, 27);
}

void lidarFrontOn()
{
  note(NOTE_B, 5, 27);
  note(NOTE_B, 6, 27);
  note(NOTE_Eb, 6, 27);
  note(NOTE_Eb, 7, 27);
  note(NOTE_E, 6, 27);
  note(NOTE_E, 7, 27);
  note(NOTE_Fs, 6, 27);
  note(NOTE_Fs, 7, 27);
  note(NOTE_B, 6, 27);
  note(NOTE_B, 7, 27);
}

void lidarFrontOff()
{
  note(NOTE_B, 6, 27);
  note(NOTE_B, 7, 27);
  note(NOTE_A, 6, 27);
  note(NOTE_A, 7, 27);
  note(NOTE_Fs, 6, 27);
  note(NOTE_Fs, 7, 27);
  note(NOTE_E, 6, 27);
  note(NOTE_E, 7, 27);
  note(NOTE_Eb, 6, 27);
  note(NOTE_Eb, 7, 27);
}

void lidarSideOn()
{
  note(NOTE_Gs, 6, 27);
  note(NOTE_Gs, 7, 27);
  note(NOTE_A, 6, 27);
  note(NOTE_A, 7, 27);
  note(NOTE_B, 6, 27);
  note(NOTE_B, 7, 27);
  note(NOTE_E, 7, 27);
  note(NOTE_E, 8, 27);
}

void lidarSideOff()
{
  note(NOTE_E, 7, 27);
  note(NOTE_E, 8, 27);
  note(NOTE_D, 7, 27);
  note(NOTE_D, 8, 27);
  note(NOTE_Cs, 7, 27);
  note(NOTE_Cs, 8, 27);
  note(NOTE_Bb, 6, 27);
  note(NOTE_Bb, 7, 27);
}

#endif
