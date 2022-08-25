#define pin_dout 16
#define pin_slk 17

void setup()
{
  Serial.begin(9600);
  pinMode(pin_slk, OUTPUT);
  pinMode(pin_dout, INPUT);
}

int mics = 1;
int d = 1;

void loop()
{
  Serial.println(mics);
  for (int i = 0; i < 500000 / mics; i++)
  {
    digitalWrite(pin_slk, 1);
    delayMicroseconds(mics);
    digitalWrite(pin_slk, 0);
    delayMicroseconds(mics);
  }
  mics += d;
  if(mics >= 10){
    if(d == 1){
      d = -1;
    }
  }
  if(mics <= 1){
    if(d == -1){
      d = 1;
    }
  }
}
