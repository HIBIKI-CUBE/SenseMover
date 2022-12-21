void setup()
{
  // initialize both serial ports:
  Serial.begin(115200);
  Serial2.begin(921600);
}

bool t = false;
unsigned long count = 0;

void loop()
{
  if (millis() > 1000 && !t)
  {
    const uint8_t start[9] = {0xA5, 0xA5, 0xA5, 0xA5, 0x00, 0x63, 0x00, 0x00, 0x63};
    Serial.write(start, sizeof(start));
    Serial2.write(start, sizeof(start));
    t = true;
  }

  // read from port 1, send to port 0:
  if (Serial2.available())
  {
    // int inByte = Serial2.read();

    uint8_t packet[331] = {};
    Serial2.read(packet, sizeof(packet));
    for (int i = 0; i < sizeof(packet); i++)
    {
      Serial.print(packet[i], HEX);
      Serial.print(", ");
    }
    Serial.println();
    // Serial.print(count++);
    // Serial.print(":");
    // Serial.println(inByte, HEX);
    // if(count == 9 || (count - 9) % 331 == 0){
    //   Serial.println();
    // }
  }

  // read from port 0, send to port 1:
  if (Serial.available())
  {
    //    String line = Serial.readString();
    //    int inByte = line.toInt();
    int inByte = Serial.read();
    Serial.print(inByte, HEX);
    Serial2.print(inByte, HEX);
  }
}
