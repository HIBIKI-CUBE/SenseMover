#ifndef _Bluetooth_h
#define _Bluetooth_h

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "./Interaction.hpp"
#include <Arduino_JSON.h>

#define SERVICE_UUID "2ba23aa3-f921-451e-a54b-e3093e5e3112"
#define CHARACTERISTIC_UUID "f46a5236-5e85-4933-b171-48b7461722c3"

byte bleMode = 0;
uint16_t bleDistance = 0; // 0 ~ 1000
int bleAngle = 0;
unsigned long bleReadDelay = 0;
BLECharacteristic *pCharacteristic;
bool active = false;
bool activeToggled = false;
unsigned long lastBleCommand = 0;
bool calibrating = false;

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
  };
  void onDisconnect(BLEServer *pServer)
  {
    note(NOTE_E, 7);
    note(NOTE_D, 7);
    note(NOTE_Bb, 7);
    BLEDevice::startAdvertising();
  }
};

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

void BluetoothSetup()
{
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
}

#endif
