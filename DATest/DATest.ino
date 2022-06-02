
uint8_t v = 0;
int d = 10;

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  dacWrite(26, v);
  if (v == 0)
  {
    v = 255;
  }
  else
  {
    v = 0;
  }
  delay(5000);
}
