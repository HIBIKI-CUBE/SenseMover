
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID "2ba23aa3-f921-451e-a54b-e3093e5e3112"
#define CHARACTERISTIC_UUID "f46a5236-5e85-4933-b171-48b7461722c3"

void check()
{
  digitalWrite(27, LOW);
  delay(10);
  digitalWrite(27, HIGH);
  delay(10);
  digitalWrite(27, LOW);
}

class MyServerCallbacks : public BLEServerCallbacks
{
  void onDisconnect(BLEServer *pServer)
  {
    check();
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    check();
  }
};

void setup(){
  pinMode(27, OUTPUT);
  check();
  BLEDevice::init("VULCAN Super wheelchair"); // この名前がスマホなどに表示される
  check();
  BLEServer *pServer = BLEDevice::createServer();
  check();
  pServer->setCallbacks(new MyServerCallbacks());
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
}

void loop() {
  digitalWrite(27, HIGH);
}
