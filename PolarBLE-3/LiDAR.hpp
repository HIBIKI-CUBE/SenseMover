float angleCorrect(float distance, float angle)
{
  return angle + (distance < 1 ? 0 : (1 / (0.00082 * distance - 0.002) - 8));
}

void setupLiDAR()
{
  Serial2.begin(115200);
}

enum safetyStatus
{
  safe,
  caution,
  stop,
  unknown
};

const uint8_t tireRadius = 100;
const uint8_t maxRPM = 150;
const uint16_t width = 560;
const uint16_t treadWidth = 408;
const uint16_t wheel2LiDAR = 645;

unsigned long lastLiDAR = 0;

float v2speed(int voltage)
{
  return (voltage - 127.0) / 128 * maxRPM * (2 * PI / 60) * tireRadius;
}

safetyStatus LiDAR(int vLeft, int vRight, int cautionDistance, int stopDistance)
{
  float seconds = (millis() - lastLiDAR) / 1000;
  if (Serial2.available() && !(vLeft == 127 && vRight == 127))
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

      float dTheta = (v2speed(vRight) - v2speed(vLeft)) / treadWidth * seconds / 2;
      float travel = (v2speed(vRight) + v2speed(vLeft)) / 2 * seconds;
      float dx = travel * cos(dTheta);
      float dy = travel * sin(dTheta);
      for (unsigned int i = 10; i < dataCount * 2 + 10; i += 2)
      {
        float distance = (packet[i + 1] << 8 | packet[i]) / 4;
        float angle = startAngle + (angleDiff / dataCount) * ((i - 10) / 2);
        if (distance > 0 && 80 <= angle && angle <= 280)
        {
          float x = distance * cos((angle - 90) * PI / 180.0) - dx;
          float y = distance * sin((angle - 90) * PI / 180.0) - dy;

          // float pointRadius = sqrt(sq(pointY) + sq(pointX));
          // float pointAngle = acos((distance * cos((angle - 90) * PI / 180.0)) / pointRadius) / PI * 180;
          if (-width / 2 - stopDistance <= x && x <= width / 2 + stopDistance && y < stopDistance)
          {
            return stop;
          }
          if (-width / 2 - cautionDistance <= x && x <= width / 2 + cautionDistance && y < cautionDistance)
          {
            return caution;
          }
        }
      }
    }
    lastLiDAR = millis();
  }
  else
  {
    return unknown;
  }
  return safe;
}
