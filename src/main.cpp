#include <Arduino.h>

#define LED 2

enum ZoneStatus
{
  TROUBLE,
  NORMAL,
  ALARM,
  UNKNOWN
};
const int Zone01 = 34; // Input 34 is zone sensor

void setup()
{
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  delay(1000);
}

void GetZoneStatus(int zone)
{
  ZoneStatus zs = ZoneStatus::UNKNOWN;
  int valuez = analogRead(zone);
  Serial.println(valuez);

  if (valuez > 4000)
  {
    zs = ZoneStatus::ALARM;
  }
  else if (valuez < 1800)
  {
    zs = ZoneStatus::TROUBLE;
  }
  else
  {
    zs = ZoneStatus::NORMAL;
  }

  switch (zs)
  {
  case ZoneStatus::ALARM:
    Serial.println("ALARM!");
    digitalWrite(LED, HIGH);
    delay(1000);
    break;
  case ZoneStatus::TROUBLE:
    Serial.println("TROUBLE!!");
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

void loop()
{
  // digitalWrite(LED, HIGH);
  // delay(1000);
  // digitalWrite(LED, LOW);
  // delay(500);
  GetZoneStatus(Zone01);
}