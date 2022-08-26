
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <math.h>
#include "./Interaction.hpp"
#include <Arduino_JSON.h>
#define SERVICE_UUID "2ba23aa3-f921-451e-a54b-e3093e5e3112"
#define CHARACTERISTIC_UUID "f46a5236-5e85-4933-b171-48b7461722c3"

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
byte bleMode = 0;
int radius = 0;
int theta = 0;
uint16_t bleDistance = 0; // 0 ~ 1000
int bleAngle = 0;
bool active = false;
bool activeToggled = false;
bool calibrated = false;
unsigned long startDelay = 0;
bool backSignRunnning = false;
unsigned long backSignDelay = 0;
unsigned long lastBleCommand = 0;
unsigned long bleReadDelay = 0;
BLECharacteristic *pCharacteristic;
bool calibrating = false;
enum CalibrationPhase
{
  CENTER,
  MIN,
  MAX,
  FINISH
};
CalibrationPhase calibrationPhase = CENTER;

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
  {
    esp_ble_conn_update_params_t conn_params = {0};
    memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
    conn_params.latency = 0;
    conn_params.max_int = 0x14; // max_int = 0x14*1.25ms = 25ms
    conn_params.min_int = 0xc;  // min_int = 0xc*1.25ms = 15ms
    conn_params.timeout = 400;  // timeout = 400*10ms = 4000ms
    // start sent the update connection parameters to the peer device.
    esp_ble_gap_update_conn_params(&conn_params);

    // VULCAN Startup Sound
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

    // Water crown
    // note(NOTE_C, 7, 136);
    // note(NOTE_G, 6, 136);
    // note(NOTE_C, 7, 136);
    // note(NOTE_E, 7, 136);
    // note(NOTE_A, 7, 136 * 2);
    // note(NOTE_G, 7, 136 * 2);
    // note(NOTE_D, 7, 136);
    // note(NOTE_F, 7, 136);
    // note(NOTE_A, 7, 136);
    // note(NOTE_C, 8, 136);
    // note(NOTE_B, 7, 136);
    // note(NOTE_G, 7, 136);
    // note(NOTE_B, 7, 136);
    // note(NOTE_D, 8, 136);
    // for (uint8_t i = 0; i < 20; i++)
    // {
    //   note(NOTE_A, 7, 45);
    //   note(NOTE_Cs, 8, 45);
    // }

    // ゼルダの伝説
    // note(NOTE_G, 7, 136);
    // note(NOTE_Fs, 7, 136);
    // note(NOTE_Eb, 7, 136);
    // note(NOTE_A, 7, 136);
    // note(NOTE_Gs, 7, 136);
    // note(NOTE_E, 7, 136);
    // note(NOTE_Gs, 7, 136);
    // note(NOTE_C, 7, 136 * 5);
  };
  void onDisconnect(BLEServer *pServer)
  {
    note(NOTE_E, 7);
    note(NOTE_D, 7);
    note(NOTE_Bb, 7);
    BLEDevice::startAdvertising();
  }
};

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

void nyancat()
{
  int q = 60 / 144 / 4 * 1000;
  note(NOTE_Eb, 7, q);
  note(NOTE_E, 7, q);
  note(NOTE_Fs, 7, q);
  delay(q);
  note(NOTE_B, 7, q * 2);
  note(NOTE_Eb, 7, q);
  note(NOTE_E, 7, q);
  note(NOTE_Fs, 7, q);
  note(NOTE_B, 7, q);
  note(NOTE_Cs, 8, q);
  note(NOTE_Eb, 8, q);
  note(NOTE_Cs, 8, q);
  note(NOTE_Bb, 7, q);
  note(NOTE_B, 7, q * 2);
  note(NOTE_Fs, 7, q * 2);
  note(NOTE_Eb, 7, q);
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
class BleCallbacks : public BLECharacteristicCallbacks
{
public:
  static const uint8_t bleWaitDuration = 30;

private:
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0)
    {
      String raw = value.c_str();
      JSONVar data = JSON.parse(raw);
      if (data.hasOwnProperty("d"))
      {
        bleDistance = (int)data["d"];
      }
      if (data.hasOwnProperty("a"))
      {
        bleAngle = (int)data["a"];
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
          calibrating = true;
        }
        else
        {
          note(NOTE_Bb, 5);
        }
      }
    }
    lastBleCommand = millis();
  }
};

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

  BLEDevice::init("VULCAN Super wheelchair"); // この名前がスマホなどに表示される
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE_NR);
  pCharacteristic->setCallbacks(new BleCallbacks());
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
        if (!backSignRunnning && millis() - backSignDelay >= 500)
        {
          ledcWriteNote(buzzerChannel, NOTE_E, 5);
          digitalWrite(led, HIGH);
          backSignRunnning = true;
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

  if (backSignRunnning && millis() - backSignDelay >= 500)
  {
    ledcWriteTone(buzzerChannel, 0);
    digitalWrite(led, LOW);
    backSignRunnning = false;
    backSignDelay = millis();
  }
}
