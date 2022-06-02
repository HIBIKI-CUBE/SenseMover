

//
//  server.ino
//
//  Created by 101010.fun on 2021/07/14.
//

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Wire.h>
#include <Arduino_JSON.h>

// こちらのジェネレータでUUIDを生成してください
// https://www.uuidgenerator.net/

#define SERVICE_UUID "2ba23aa3-f921-451e-a54b-e3093e5e3112"
#define CHARACTERISTIC_UUID "f46a5236-5e85-4933-b171-48b7461722c3"

static const uint8_t Left_Motor = 0x61;  //デバイスアドレス（スレーブ）
static const uint8_t Right_Motor = 0x60; //
unsigned long startDelay = 0;
bool active = true;
class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    if (millis() - startDelay > 50)
    {
      std::string value = pCharacteristic->getValue();
      Serial.println(value.c_str());
      if (value.length() > 0)
      {
        String raw = value.c_str();
        JSONVar data = JSON.parse(raw);
        uint16_t num = 2090;
        uint16_t Vl = num;
        uint16_t Vr = num;
        if (data, hasOwnProperty("distance") && data.hasOwnProperty("angle"))
        {
          num = data["distance"]
        }
        uint16_t num = data.toInt() + 50;
        // Serial.println(num);
        uint16_t Vl = num;
        uint16_t Vr = num;
        // if (ledState == "1")
        // {
        //   Vl = 2500;
        //   Vr = 2500;
        // }
        // else
        // {
        // }

      Vl = constrain(Vl, 410, 4095);
      Vr = constrain(Vr, 410, 4095);

        Wire.beginTransmission(Left_Motor);
        Wire.write((Vl >> 8) & 0x0F);
        Wire.write(Vl);
        Wire.endTransmission();
        Wire.beginTransmission(Right_Motor);
        Wire.write((Vr >> 8) & 0x0F);
        Wire.write(Vr);
        Wire.endTransmission();

        startDelay = millis();
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);
  Wire.beginTransmission(Left_Motor);
  Wire.write((2047 >> 8) & 0x0F);
  Wire.write(2047);
  Wire.endTransmission();
  Wire.beginTransmission(Right_Motor);
  Wire.write((2047 >> 8) & 0x0F);
  Wire.write(2047);
  Wire.endTransmission();

  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(27, INPUT);

  BLEDevice::init("VULCAN Super wheelchair"); // この名前がスマホなどに表示される
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE); // キャラクタリスティックの作成　→　「僕はこんなデータをやり取りするよできるよ」的な宣言

  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // iPhone接続の問題に役立つ
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop()
{
  if (digitalRead(27) == HIGH)
  {
    active = false;
  }
  else
  {
    active = true;
  }

  if (active)
  {
    digitalWrite(13, HIGH);
    digitalWrite(14, HIGH);
  }
  else
  {
    digitalWrite(13, LOW);
    digitalWrite(14, LOW);
  }
}
