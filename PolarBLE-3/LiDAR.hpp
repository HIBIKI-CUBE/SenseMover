#ifndef _LiDAR_h
#define _LiDAR_h

float angleCorrect(float distance, float angle)
{
  return angle + (distance < 1 ? 0 : (1 / (0.00082 * distance - 0.002) - 8));
}

void setupLiDAR()
{
  Serial2.begin(115200);
  lidarFrontOn();
  lidarSideOn();
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
  int vMax;
  float vMaxRatio;
  safetyStatus status = unknown;
};

const uint8_t tireRadius = 100;
const uint8_t rpmMax = 150;
const uint16_t width = 560;
const uint16_t treadWidth = 408;
const uint16_t wheel2LiDAR = 645;
const uint16_t decelMax = 2730; // mm/s^2

bool measuringLeft = false;
bool measuringRight = false;
bool measuringFront = false;

float leftMin;
float rightMin;

bool cautionLeft;
bool cautionRight;
bool cautionRollR;
bool cautionRollL;

bool lidarFront = true;
bool lidarSide = true;

unsigned long lastLiDAR = 0;

float v2speed(int voltage)
{ // mm/s
  return (voltage - 127.0) / 128 * rpmMax * 2 * PI * tireRadius / 60;
}

int speed2v(float speed)
{ // mm/s
  return (speed / (rpmMax * 2 * PI * tireRadius / 60)) * 128 + 127;
}

struct resultLiDAR result;

resultLiDAR LiDAR(int vLeft, int vRight, int protectionSecs, int cautionDistance, int stopDistance)
{
  float seconds = (millis() - lastLiDAR) / 1000;

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

      measuringLeft = (85 < startAngle && startAngle < 158 || 80 < endAngle && endAngle < 158);
      measuringFront = (158 < startAngle && startAngle < 203 || 158 < endAngle && endAngle < 203);
      measuringRight = (203 < startAngle && startAngle < 275 || 203 < endAngle && endAngle < 275);

      float theta = (v2speed(vLeft) - v2speed(vRight)) / treadWidth * seconds;
      float travel = (v2speed(vLeft) + v2speed(vRight)) / 2 * seconds;
      // 車輪間中心の変位
      float dx = travel * sin(theta);
      float dy = travel * cos(theta);
      // LiDARセンサーの変位
      float dxL = dx + wheel2LiDAR * sin(theta);
      float dyL = -wheel2LiDAR + dy + wheel2LiDAR * cos(theta);

      if (measuringFront)
      {
        result.vMax = 255;
        result.vMaxRatio = 1.0;
      }
      if (measuringLeft)
      {
        cautionLeft = false;
        cautionRollL = false;
        leftMin = 1000;
      }
      if (measuringRight)
      {
        cautionRight = false;
        cautionRollR = false;
        rightMin = 1000;
      }

      for (unsigned int i = 10; i < dataCount * 2 + 10; i += 2)
      {
        float distance = (packet[i + 1] << 8 | packet[i]) / 4;
        float angle = startAngle + (angleDiff / dataCount) * ((i - 10) / 2);
        if (80 <= angle && angle <= 280)
        {
          if (distance > 0)
          {
            float x = distance * cos((angle + 90) * PI / 180.0);
            float y = distance * sin((angle - 90) * PI / 180.0);

            float x2t = x - dxL;
            float y2t = y - dyL;
            float distance2 = sqrt(sq(x2t) + sq(y2t));
            float angle2 = x2t == 0 ? PI / 2 : -(atan(y2t / x2t) + theta);
            angle2 += angle2 < 0 ? PI : 0;

            float x2 = -distance2 * cos(angle2);
            float y2 = distance2 * sin(angle2);

            if (vLeft > 127 && vRight > 127)
            {
              if (measuringFront && -width / 2 <= x2 && x2 < width / 2 && y2 < y)
              {
                result.vMax = abs(result.vMax - 127) < abs(speed2v((y2 - stopDistance) / protectionSecs) - 127) ? result.vMax : speed2v((y2 - stopDistance) / protectionSecs);
              }
              else if (y <= cautionDistance)
              {
                if (measuringLeft)
                {
                  leftMin = min(leftMin, abs(x2));
                  if (leftMin < (width / 2 + stopDistance))
                  {
                    cautionLeft = true;
                    result.status = caution;
                  }
                }

                if (measuringRight)
                {
                  rightMin = min(rightMin, abs(x2));
                  if (rightMin < (width / 2 + stopDistance))
                  {
                    cautionRight = true;
                    result.status = caution;
                  }
                }
              }
            }
            else if (signDifferent(vLeft - 127, vRight - 127) && (y <= stopDistance || y2 <= stopDistance) && abs(x2) < abs(x) && (abs(x) <= (width / 2 + stopDistance * 2) || abs(x2) <= (width / 2 + stopDistance * 2)))
            {
              if (vLeft < vRight && measuringLeft)
              {
                cautionRollL = true;
              }
              else if (measuringRight){
                cautionRollR = true;
              }
              result.status = caution;
            }
            if (result.status == unknown)
            {
              result.status = safe;
            }
          }
        }
        lastLiDAR = millis();
      }
    }
  }
  if (lidarFront)
  {
    result.vMaxRatio = abs(min(result.vMax, 255) - 127) / 127.0;
    if(result.vMaxRatio < 0.1){
      result.status = caution;
    }
    result.vLeft = (result.vLeft - 127) * result.vMaxRatio + 127;
    result.vRight = (result.vRight - 127) * result.vMaxRatio + 127;
  }
  if (lidarSide)
  {
    if (cautionRollL || cautionRollR)
    {
      result.vLeft = 127;
      result.vRight = 127;
    }
    if (cautionLeft && cautionRight)
    {
      if (leftMin < rightMin)
      {
        result.vRight = (result.vLeft - 127) / 2 + 127;
      }
      else
      {
        result.vLeft = (result.vRight - 127) / 2 + 127;
      }
    }
    else
    {
      if (cautionLeft)
      {
        result.vRight = (result.vLeft - 127) / 2 + 127;
      }
      if (cautionRight)
      {
        result.vLeft = (result.vRight - 127) / 2 + 127;
      }
    }
  }
  return result;
}

#endif
