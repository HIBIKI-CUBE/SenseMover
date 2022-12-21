float angleCorrect(float distance, float angle)
{
  return angle + (distance < 1 ? 0 : (1 / (0.00082 * distance - 0.002) - 8));
}

void setupLiDAR(){
  Serial2.begin(115200);
}

float getDistance(){
  if (Serial2.available())
  {
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
        float distance = (packet[i + 1] << 8 | packet[i]) / 4;
        float angle = startAngle + (angleDiff / dataCount) * ((i - 10) / 2);
        if (178 <= angle && angle <= 182)
        {
          return distance;
        }
      }
    }
  }
  return 0;
}

