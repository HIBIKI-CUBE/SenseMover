float angleCorrect(float distance, float angle)
{
  return angle + (distance < 1 ? 0 : (1 / (0.00082 * distance - 0.002) - 8));
}

void setupLiDAR()
{
  Serial2.begin(115200);
}

bool signDifferent(int a, int b)
{
  return (a >= 0 && b <= 0) || (a <= 0 && b >= 0);
}

enum safetyStatus
{
  safe,
  caution,
  stop,
  unknown
};

struct resultLiDAR
{
  int vLeft;
  int vRight;
  safetyStatus status = unknown;
};

const uint8_t tireRadius = 100;
const uint8_t rpmMax = 150;
const uint16_t width = 560;
const uint16_t treadWidth = 408;
const uint16_t wheel2LiDAR = 645;

bool cautionL = false;
bool cautionLF = false;
bool cautionF = false;
bool cautionRF = false;
bool cautionR = false;

unsigned long lastLiDAR = 0;

float v2speed(int voltage)
{ // mm/s
  return (voltage - 127.0) / 128 * rpmMax * 2 * PI * tireRadius / 60;
}

int speed2v(float speed)
{ // mm/s
  return (speed / (rpmMax * 2 * PI * tireRadius / 60)) * 128 + 127;
}

resultLiDAR LiDAR(int vLeft, int vRight, int cautionDistance, int stopDistance)
{
  float seconds = (millis() - lastLiDAR) / 1000;
  struct resultLiDAR result;

  result.vLeft = vLeft;
  result.vRight = vRight;
  result.status = unknown;
  if (Serial2.available() && !(abs(vLeft - 127) < 3 && abs(vRight - 127) < 3))
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

      float theta = (v2speed(vLeft) - v2speed(vRight)) / treadWidth * seconds;
      float travel = (v2speed(vLeft) + v2speed(vRight)) / 2 * seconds;
      float dx = travel * sin(theta);
      float dy = travel * cos(theta);
      float dxL = dx + wheel2LiDAR * sin(theta);
      float dyL = -wheel2LiDAR + dy + wheel2LiDAR * cos(theta);
      for (unsigned int i = 10; i < dataCount * 2 + 10; i += 2)
      {
        float distance = (packet[i + 1] << 8 | packet[i]) / 4;
        float angle = startAngle + (angleDiff / dataCount) * ((i - 10) / 2);
        if (80 <= angle && angle <= 280)
        {
          if (distance > 0)
          {
            float x = distance * cos((angle - 90) * PI / 180.0) - dx;
            float y = distance * sin((angle - 90) * PI / 180.0) - dy;

            float x2t = x - dxL;
            float y2t = y - dyL;
            float distance2 = sqrt(sq(x2t) + sq(y2t));
            float angle2 = x2t == 0 ? PI / 2 : -(atan(y2t / x2t) + theta);

            float x2 = distance2 * cos(angle2);
            float y2 = distance2 * sin(angle2);

            float dxP = x2 - x;
            float dyP = y2 - y;

            // float pointRadius = sqrt(sq(pointY) + sq(pointX));
            // float pointAngle = acos((distance * cos((angle - 90) * PI / 180.0)) / pointRadius) / PI * 180;

            if ((-width / 2 - cautionDistance <= x && x <= width / 2 + cautionDistance && y <= cautionDistance))
            {
              if (signDifferent(vLeft - 127, vRight - 127) && (dxP < 0 || dyP < 0))
              {
                result.status = caution;
                result.vLeft = (vLeft - 127) * (distance - stopDistance) / (v2speed(vLeft) * 5) + 127;
                result.vRight = (vRight - 127) * (distance - stopDistance) / (v2speed(vLeft) * 5) + 127;
                Serial.print("1: ");
                Serial.print(vLeft);
                Serial.print(", ");
                Serial.print(vRight);
                Serial.print(" -> ");
                Serial.print(result.vLeft);
                Serial.print(", ");
                Serial.println(result.vRight);
              }
              else if (vLeft > 127 && vRight > 127)
              {
                if (dyP < 0 && (-width / 2 <= x2 && x2 <= width / 2))
                {
                  result.status = caution;
                  result.vLeft = (vLeft - 127) * (distance - stopDistance) / (v2speed(vLeft) * 5) + 127;
                  result.vRight = (vRight - 127) * (distance - stopDistance) / (v2speed(vLeft) * 5) + 127;
                  Serial.print("2: ");
                  Serial.print(vLeft);
                  Serial.print(", ");
                  Serial.print(vRight);
                  Serial.print(" -> ");
                  Serial.print(result.vLeft);
                  Serial.print(", ");
                  Serial.println(result.vRight);
                }
                else if (dxP < 0)
                {
                  result.status = caution;
                  if (vRight - 127 > vLeft - 127)
                  {
                    result.vRight = (vRight - 127) * (distance - stopDistance) / (v2speed(vLeft) * 5) + 127;
                    Serial.print("3: ");
                    Serial.print(vLeft);
                    Serial.print(", ");
                    Serial.print(vRight);
                    Serial.print(" -> ");
                    Serial.print(result.vLeft);
                    Serial.print(", ");
                    Serial.println(result.vRight);
                  }
                  else
                  {
                    result.vLeft = (vLeft - 127) * (distance - stopDistance) / (v2speed(vLeft) * 5) + 127;
                    Serial.print("4: ");
                    Serial.print(vLeft);
                    Serial.print(", ");
                    Serial.print(vRight);
                    Serial.print(" -> ");
                    Serial.print(result.vLeft);
                    Serial.print(", ");
                    Serial.println(result.vRight);
                  }
                }
              }
            }
            if ((-width / 2 - stopDistance <= x && x <= width / 2 + stopDistance && y <= stopDistance) || (result.status == caution && result.vLeft == 127 && result.vRight == 127))
            {
              result.vLeft = 127;
              result.vRight = 127;
              result.status = stop;
            }
            if (result.status == unknown)
            {
              result.status = safe;
            }
          }
          lastLiDAR = millis();
        }
      }
    }
  }
  return result;
}
