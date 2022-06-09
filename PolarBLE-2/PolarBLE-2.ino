
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <math.h>
#include <Arduino_JSON.h>
#define SERVICE_UUID "2ba23aa3-f921-451e-a54b-e3093e5e3112"
#define CHARACTERISTIC_UUID "f46a5236-5e85-4933-b171-48b7461722c3"

static const uint8_t t = 83;
static const uint8_t led = 27;
static const uint8_t button = 13;
static const uint8_t buzzerPin = 23;
static const uint8_t buzzerChannel = 0;
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
byte bleMode = 0;
uint16_t bleDistance = 0; // 0 ~ 1000
int bleAngle = 0;
bool active = false;
bool activeToggled = false;
bool calibrated = false;
unsigned long startDelay = 0;

void note(note_t note = NOTE_C, uint8_t octave = 7, uint32_t duration = t, uint8_t channel = buzzerChannel)
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
class MyServerCallbacks : public BLEServerCallbacks
{
  void onDisconnect(BLEServer *pServer)
  {
    BLEDevice::startAdvertising();
  }
};

void waitButtonUntil(int v)
{
  while (digitalRead(button) != v)
  {
  }
}

void calibrate()
{
  waitButtonUntil(HIGH);
  centerBF = analogReadMilliVolts(backAndForth);
  centerRL = analogReadMilliVolts(rightAndLeft);
  // centerRL = analogRead(rightAndLeft) - (abs(analogRead(backAndForth) - 511) * 0.33);
  note(NOTE_C);
  waitButtonUntil(LOW);
  delay(50);

  while (digitalRead(button) == LOW)
  {
    if (analogReadMilliVolts(backAndForth) <= centerBF || adMax <= analogRead(backAndForth))
    {
      note(NOTE_Bb);
    }
  }
  deltaMinBF = analogReadMilliVolts(backAndForth) - centerBF;
  deltaMinRL = analogReadMilliVolts(rightAndLeft) - centerRL;
  note(NOTE_E);
  waitButtonUntil(LOW);
  delay(50);

  while (digitalRead(button) == LOW)
  {
    // Serial.print(1023);
    // Serial.print(", ");
    // Serial.print(0);
    // Serial.print(", ");
    // Serial.print(511);
    // Serial.print(", ");
    // Serial.print(analogRead(backAndForth));
    // Serial.print(", ");
    // Serial.println(analogRead(rightAndLeft) - (abs(analogRead(backAndForth) - 511) * 0.33));
    if (analogReadMilliVolts(backAndForth) <= centerBF || adMax <= analogReadMilliVolts(backAndForth))
    {
      note(NOTE_Bb);
    }
  }
  deltaMaxRL = analogReadMilliVolts(rightAndLeft) - centerRL;
  deltaMaxBF = analogReadMilliVolts(backAndForth) - centerBF;
  note(NOTE_G);
  waitButtonUntil(LOW);

  while (!(-deltaMinBF < analogReadMilliVolts(backAndForth) - centerBF && analogReadMilliVolts(backAndForth) - centerBF < deltaMinBF && -deltaMinRL < analogReadMilliVolts(rightAndLeft) - centerRL && analogReadMilliVolts(rightAndLeft) - centerRL < deltaMinRL))
  {
  }
  calibrated = true;
}
class MyCallbacks : public BLECharacteristicCallbacks
{
  uint8_t bleWaitDuration = 50;
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    if (millis() - startDelay > bleWaitDuration)
    {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0)
      {
        String raw = value.c_str();
        JSONVar data = JSON.parse(raw);
        if (data.hasOwnProperty("distance"))
        {
          bleDistance = (int)data["distance"];
        }
        if (data.hasOwnProperty("angle"))
        {
          bleAngle = (int)data["angle"];
        }
        if (data.hasOwnProperty("active"))
        {
          if (active != (bool)data["active"])
          {
            activeToggled = true;
          }
          active = (bool)data["active"];
        }
        if (data.hasOwnProperty("mode"))
        {
          blink();
          bleMode = (int)data["mode"];
        }
        if (data.hasOwnProperty("action"))
        {
          blink();
          if (JSON.stringify((const char *)data["action"]) == "\"calibrate\"")
          {
            calibrate();
            note(NOTE_C, 7);
            note(NOTE_E, 7);
            note(NOTE_G, 7);
          }
          else
          {
            note(NOTE_Bb, 5);
          }
        }

        startDelay = millis();
      }
    }
  }
};

void setup()
{
  pinMode(led, OUTPUT);
  pinMode(activeR, OUTPUT);
  pinMode(activeL, OUTPUT);

  analogSetAttenuation(ADC_11db);
  pinMode(rightAndLeft, ANALOG);
  pinMode(backAndForth, ANALOG);

#define BUZZER_FREQ 24000
#define BUZZER_TIMERBIT 11
  ledcSetup(buzzerChannel, BUZZER_FREQ, BUZZER_TIMERBIT);
  ledcAttachPin(buzzerPin, buzzerChannel);

  digitalWrite(activeL, LOW);
  digitalWrite(activeR, LOW);
  dacWrite(leftMotor, 127);
  dacWrite(rightMotor, 127);

  Serial.begin(115200);

  BLEDevice::init("VULCAN Super wheelchair"); // この名前がスマホなどに表示される
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // iPhone接続の問題に役立つ
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  blink();
  Serial.println("Characteristic defined! Now you can read it in your phone!");

  if (bleMode == 1)
  {
    calibrate();
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
  int radius = 0;
  int theta = 0;
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
    if (!calibrated)
    {
      radius = 0;
      theta = 0;
    }
    break;
  }

  if (activeToggled)
  {
    note(NOTE_C);
    activeToggled = false;
  }

  if (active)
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
        if (millis() - startDelay >= 1000)
        {
          // ledcWriteNote(buzzerChannel, NOTE_B, 5);
          blink();
          startDelay = millis();
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

  if (active)
  {
    digitalWrite(activeR, HIGH);
    digitalWrite(activeL, HIGH);
    blink();
  }
  else
  {
    digitalWrite(activeR, LOW);
    digitalWrite(activeL, LOW);
  }
}
