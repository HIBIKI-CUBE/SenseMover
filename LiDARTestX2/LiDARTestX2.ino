void setup()
{
  // initialize both serial ports:
  Serial.begin(115200);
  Serial2.begin(115200);
}

bool t = false;
bool a5 = false;
unsigned long count = 0;
int distances[360] = {};

float angleCorrect(float distance, float angle)
{
  return angle + (distance < 1 ? 0 : (1 / (0.00082 * distance - 0.002) - 8));
}

void loop()
{
  // if (millis() > 1000 && !t) {
  //   const uint8_t start[9] = { 0xA5, 0xA5, 0xA5, 0xA5, 0x00, 0x63, 0x00, 0x00, 0x63 };
  //   Serial.write(start, sizeof(start));
  //   Serial2.write(start, sizeof(start));
  //   t = true;
  // }

  // read from port 1, send to port 0:
  if (Serial2.available())
  {
    // int inByte = Serial2.read();

    uint8_t packet[250] = {};
    Serial2.read(packet, sizeof(packet));
    if (packet[0] == 0xAA && packet[1] == 0x55)
    {
      int dataCount = packet[3];
      float a0 = ((packet[5] << 8 | packet[4]) >> 1) / 64.0;
      float al = ((packet[7] << 8 | packet[6]) >> 1) / 64.0;
      float d0 = (packet[11] << 8 | packet[10]) / 4;
      float startAngle = angleCorrect(d0, a0);
      float dl = (packet[9 + dataCount * 2] << 8 | packet[8 + dataCount * 2]) / 4;
      float endAngle = angleCorrect(dl, al);
      float angleDiff = startAngle < endAngle ? endAngle - startAngle : 360 - startAngle + endAngle;
      for (unsigned int i = 10; i < dataCount * 2 + 10; i += 2)
      {
        float d = (packet[i + 1] << 8 | packet[i]) / 4;
        float a = startAngle + (angleDiff / dataCount) * ((i - 10) / 2);
        if (178 <= a && a <= 182)
        {
          Serial.println(d);
          break;
        }
      }
      // Serial.print(packet[2] & 1 == 1 ? "Start: " : "conti: ");
      // Serial.print(dataCount, DEC);
      // Serial.print("data, ");

      // Serial.print(startAngle);
      // Serial.print(", ");
      // Serial.print(endAngle);
      // Serial.print(", ");
      // Serial.print(angleDiff);
      // Serial.print(", ");
      // Serial.print(ac0);
      // Serial.print(", ");
      // Serial.print(d0);
      // Serial.print(", ");

      // for (int i = 0; i < sizeof(packet); i++) {
      //   Serial.print(packet[i], HEX);
      //   Serial.print(", ");
      // }
      // Serial.println("");
    }
    // Serial.print(count++);
    // Serial.print(":");
    // if(inByte == 0x55 && a5){
    //   Serial.println();
    // }else{
    //   a5 = false;
    // }
    // if(inByte == 0xAA){
    //   a5 = true;
    // }
    // Serial.print(inByte, HEX);
    // Serial.print(" ");
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
