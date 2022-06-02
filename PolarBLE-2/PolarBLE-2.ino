
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <math.h>
#include <Arduino_JSON.h>

#define SERVICE_UUID "2ba23aa3-f921-451e-a54b-e3093e5e3112"
#define CHARACTERISTIC_UUID "f46a5236-5e85-4933-b171-48b7461722c3"
// #define MODE_UUID "71b436b4-a1f3-4a20-93a2-602bba4f9ffc"
// #define POLAR_DISTANCE_UUID "bcc791a5-21c5-4f44-87d8-cb85ee3eae9c"
// #define POLAR_ANGLE_UUID "849e3ba1-cac5-4dea-a7df-a638ccbea76c"
// #define DIRECT_R_UUID "896132b3-ba54-4310-8796-9dc1c5fdb8aa"
// #define DIRECT_L_UUID "fa8ccef2-3b11-4b88-be5e-b31673fe35e1"

static const uint8_t t = 83;
static const uint8_t button = 27;
static const uint8_t activeR = 13;
static const uint8_t activeL = 14;
static const uint8_t dacLOW10 = 6;
static const uint8_t dacHIGH10 = 33;
static const uint8_t rightMotor = 25;
static const uint8_t leftMotor = 26;
static const uint8_t rightAndLeft = 0;
static const uint8_t backAndForth = 0;
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
unsigned long startDelay = 0;

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    if (millis() - startDelay > 50)
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
          active = (bool)data["active"];
          activeToggled = true;
        }

        startDelay = millis();
      }
    }
  }
};

void waitButtonUntil(int v)
{
  while (digitalRead(button) != v)
  {
  }
}

void check()
{
  digitalWrite(27, LOW);
  delay(50);
  digitalWrite(27, HIGH);
  delay(50);
  digitalWrite(27, LOW);
}

void setup()
{
  pinMode(27, OUTPUT);
  // pinMode(button, INPUT);
  pinMode(activeR, OUTPUT);
  pinMode(activeL, OUTPUT);

  digitalWrite(activeL, LOW);
  digitalWrite(activeR, LOW);
  dacWrite(leftMotor, 127);
  dacWrite(rightMotor, 127);
  digitalWrite(27, LOW);
  delay(50);
  digitalWrite(27, HIGH);
  // Serial.begin(115200);
  check();
  BLEDevice::init("VULCAN Super wheelchair"); // この名前がスマホなどに表示される
  check();
  BLEServer *pServer = BLEDevice::createServer();
  check();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  check();

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  check();
  pCharacteristic->setCallbacks(new MyCallbacks());
  check();
  pService->start();
  check();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  check();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  check();
  pAdvertising->setScanResponse(true);
  check();
  pAdvertising->setMinPreferred(0x06); // iPhone接続の問題に役立つ
  check();
  pAdvertising->setMinPreferred(0x12);
  check();
  BLEDevice::startAdvertising();
  // Serial.println("Characteristic defined! Now you can read it in your phone!");

  if (bleMode == 1)
  {
    waitButtonUntil(HIGH);
    centerBF = analogRead(backAndForth);
    centerRL = analogRead(rightAndLeft) - (abs(analogRead(backAndForth) - 511) * 0.33);
    // tone(buzzer, 1047, t);
    delay(t);
    waitButtonUntil(LOW);
    delay(50);

    while (digitalRead(button) == LOW)
    {
      if (analogRead(backAndForth) <= centerBF || 1023 <= analogRead(backAndForth))
      {
        // tone(buzzer, 1865, t);
        delay(t);
      }
    }
    deltaMinBF = analogRead(backAndForth) - centerBF;
    deltaMinRL = analogRead(rightAndLeft) - centerRL;
    // tone(buzzer, 1319, t);
    delay(t);
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
      if (analogRead(backAndForth) <= centerBF || 1023 <= analogRead(backAndForth))
      {
        // tone(buzzer, 1865, t);
        delay(t);
      }
    }
    deltaMaxRL = analogRead(rightAndLeft) - centerRL;
    deltaMaxBF = analogRead(backAndForth) - centerBF;
    // tone(buzzer, 1568, t);
    delay(t);
    waitButtonUntil(LOW);

    while (!(-deltaMinBF < analogRead(backAndForth) - centerBF && analogRead(backAndForth) - centerBF < deltaMinBF && -deltaMinRL < analogRead(rightAndLeft) - centerRL && analogRead(rightAndLeft) - centerRL < deltaMinRL))
    {
    }
  }

  // tone(buzzer, 1047, t);
  delay(t);
  // tone(buzzer, 1319, t);
  delay(t);
  // tone(buzzer, 1568, t);
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
    bf = analogRead(backAndForth) - centerBF;
    rl = analogRead(rightAndLeft) - (abs(analogRead(backAndForth) - 511) * 0.33) - centerRL;
    radius = sqrt(pow(bf, 2) + pow(rl, 2));
    theta = atan2(rl, bf) * 180.0 / PI;
    break;
  }

  // if (digitalRead(button) == HIGH)
  // {
  //   active = !active;
  //   activeToggled = true;
  //   waitButtonUntil(LOW);
  // }

  if (activeToggled)
  {
    // tone(buzzer, 1047, t);
    delay(t);
    activeToggled = false;
  }

  if (active)
  {
    if (radius >= deltaMinBF)
    {
      switch (bleMode)
      {
      case 0:
        Vl = map(bleDistance, 0, 1000, 128, theta >= 60 ? 192 : 255);
        Vr = Vl;
        break;
      case 1:
        Vl = map(radius, deltaMinBF, deltaMaxBF, 2047, 4095);
        Vr = Vl;
        break;
      }
      if (abs(theta) <= 150)
      {
        if (theta >= 0)
        { //右折
          Vr -= (Vr - 127.5) * (((float)map(abs(constrain(theta, -90, 90)), 0, 60, 0, 100)) / 100);
        }
        else
        { //左折
          Vl -= (Vl - 127.5) * (((float)map(abs(constrain(theta, -90, 90)), 0, 60, 0, 100)) / 100);
        }
      }
      else
      {
        Vl = map(Vl, 127, 255, 127, 25);
        Vr = map(Vr, 127, 255, 127, 25);
        if (millis() - startDelay >= 1000)
        {
          // tone(buzzer, 987, 500);
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

  // Serial.println(Vl);

  dacWrite(leftMotor, Vl);
  dacWrite(rightMotor, Vr);

  if (active)
  {
    digitalWrite(activeR, HIGH);
    digitalWrite(activeL, HIGH);
  }
  else
  {
    digitalWrite(activeR, LOW);
    digitalWrite(activeL, LOW);
  }
}
