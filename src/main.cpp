#include <Arduino.h>
#include <string>

#define LED 2

const int Zone01 = 34; // Input 34 is zone sensor
const int Zone02 = 35; // Input 35 is zone sensor
const int Zone03 = 36; // Input 36 is zone sensor

enum ZoneStatus
{
  TROUBLE,
  NORMAL,
  ALARM,
  UNKNOWN
};

struct Zone
{
  int pin;
  String zone_label;
  long interval;
  long last_time;
  ZoneStatus status;
};

Zone zones[3] = {
    {Zone01, "Zone 01", 500, 0, ZoneStatus::UNKNOWN},
    {Zone02, "Zone 02", 1000, 0, ZoneStatus::UNKNOWN},
    {Zone03, "Zone 03", 5000, 0, ZoneStatus::UNKNOWN},
};

void GetZoneStatus(int numzone);

void GetZoneStatus(int numzone)
{
  if (millis() - zones[numzone].last_time > zones[numzone].interval)
  {
    zones[numzone].last_time = millis();

    int valuez = analogRead(zones[numzone].pin);
    // int valuez = 777;
    Serial.println(valuez);

    if (valuez > 4000)
    {
      zones[numzone].status = ZoneStatus::ALARM;
    }
    else if (valuez < 1800)
    {
      zones[numzone].status = ZoneStatus::TROUBLE;
    }
    else
    {
      zones[numzone].status = ZoneStatus::NORMAL;
    }

    switch (zones[numzone].status)
    {
    case ZoneStatus::ALARM:
      Serial.println("ALARM! " + zones[numzone].zone_label);
      //   digitalWrite(LED, HIGH);
      // delay(1000);
      break;
    case ZoneStatus::TROUBLE:
      Serial.println("TROUBLE!! " + zones[numzone].zone_label);
      //    digitalWrite(LED, HIGH);
      // delay(500);
      // digitalWrite(LED, LOW);
      // delay(500);
      break;
    case ZoneStatus::NORMAL:
      Serial.println("NORMAL " + zones[numzone].zone_label);
      // digitalWrite(LED, HIGH);
      // delay(1000);
      // digitalWrite(LED, LOW);
      // delay(1000);
      break;
    }
  }
}



void setup()
{
  pinMode(LED, OUTPUT);
  Serial.begin(115200);

  delay(1000);
}

void loop()
{
  int nz = sizeof(zones) / sizeof(Zone);

  for (int i = 0; i < nz; i++)
  {
    GetZoneStatus(i);
    delay(100);
  }
  delay(500);
}