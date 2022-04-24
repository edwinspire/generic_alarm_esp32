#include <Arduino.h>

#define LED 2

enum ZoneStatus
{
  TROUBLE,
  NORMAL,
  ALARM
};
const int potPin = 34; // Input 34 is zone sensor
int z1Value = 0;
int z1Status = 0;

void setup()
{
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  delay(1000);
}

ZoneStatus GetZoneStatus(int value)
{

  if (value > 4000)
  {
    return ZoneStatus::ALARM;
  }
  else if (value < 1800)
  {
    return ZoneStatus::TROUBLE;
  }
  else
  {
    return ZoneStatus::NORMAL;
  }
}

void loop()
{
  // digitalWrite(LED, HIGH);
  // delay(1000);
  // digitalWrite(LED, LOW);
  // delay(500);
  z1Value = analogRead(potPin);
  Serial.println(z1Value);

  switch (GetZoneStatus(z1Value))
  {
  case ZoneStatus::ALARM:
    Serial.println("ALARM");
    digitalWrite(LED, HIGH);
    delay(1000);
    break;
  case ZoneStatus::TROUBLE:
    Serial.println("TROUBLE");
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
    delay(500);
    break;
  case ZoneStatus::NORMAL:
    Serial.println("NORMAL");
    digitalWrite(LED, HIGH);
    delay(1000);
    digitalWrite(LED, LOW);
    delay(1000);
    break;
  }
}