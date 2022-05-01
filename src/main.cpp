#include <Arduino.h>
#include <string>
#include <ezOutput.h>

#define LED 2

const int siren_time = 10; // on seconds
const int Zone01 = 34;     // Input 34 is zone sensor
const int Zone02 = 35;     // Input 35 is zone sensor
const int Zone03 = 36;     // Input 36 is zone sensor

int red_light_pin = 4;
int green_light_pin = 5;
int BuzzerPin = 18;
// bool SystemArmed = false;

ezOutput MainLED(LED);
ezOutput BuzzerOUT(BuzzerPin);

enum SensorType
{
  NORMALLY_OPEN,
  NORMALLY_CLOSED
};

namespace Zone
{

  enum Status
  {
    TROUBLE,
    NORMAL,
    ALARM,
    UNDEFINED
  };

  enum Definition
  {
    NO_USED,     // The zone will not operate in any way. Zones that are not used should be programmed as Null zones
    DELAY,       // If this zone is violated when the panel is armed it will provide entry delay
    INSTANT,     // If this zone type is violated when the panel is armed it will cause an instant alarm. Typically this zone is used for windows, patio doors or other perimeter type zones
    INTERIOR,    // If this type of zone is violated when the panel is armed it will provide entry if a delay type zone was violated first. Otherwise it will cause an instant alarm. Typically this zone is used for interior protection devices, such as motion detectors.
    ALWAYS_ARMED // If this type of zone is violated when the panel is armed or disarmed it will cause an instant alarm. Typically this zone is used for 24 hour zones.
  };

  enum Attributes
  {
    AUDIBLE,
    SILENT,
    PULSED
  };

  struct Config
  {
    int pin;
    String zone_label;
    long interval;
    long last_time;
    Status status;
    SensorType sensor_type;
    Definition definition;
    bool chime;
    Attributes attributes;
  };

}

enum SystemArmedStatus
{
  READY,              // The system is disarmed and ready to be armed
  NOT_READY,          // The system is disarmed, open zones are not ready to be armed
  ARMED,              // The system is armed and ready to go
  ARMED_MEMORY_ALARM, // An alarm occurred while the system was armed.
  ARMED_FORCED,       // System armed but with some zone open or in trouble
  UNDEFINED
};

struct SystemStatus
{
  SystemArmedStatus armed_status;
  Zone::Status zone_status;
  Zone::Attributes zone_attributes;
};

SystemStatus system_status = {SystemArmedStatus::UNDEFINED, Zone::Status::UNDEFINED};

Zone::Config zones[2] = {
    {Zone01, "Zone 01", 500, 0, Zone::Status::UNDEFINED, SensorType::NORMALLY_CLOSED, Zone::Definition::INSTANT, true, Zone::Attributes::AUDIBLE},
    {Zone02, "Zone 02", 1500, 0, Zone::Status::UNDEFINED, SensorType::NORMALLY_OPEN, Zone::Definition::NO_USED, false, Zone::Attributes::PULSED}};

void GetZoneStatus(int numzone)
{
  if (millis() - zones[numzone].last_time > zones[numzone].interval && zones[numzone].definition != Zone::Definition::NO_USED)
  {
    zones[numzone].last_time = millis();

    int valuez = analogRead(zones[numzone].pin);
    // int valuez = 777;
    Serial.print(zones[numzone].zone_label + " > " + String(valuez) + "\n\r");

    if (valuez > 4000 && zones[numzone].sensor_type == SensorType::NORMALLY_CLOSED)
    {
      zones[numzone].status = Zone::Status::NORMAL;
    }
    else if (valuez > 1700 && valuez < 1900 && zones[numzone].sensor_type == SensorType::NORMALLY_OPEN)
    {
      zones[numzone].status = Zone::Status::NORMAL;
    }
    else if (valuez > 4000 && zones[numzone].sensor_type == SensorType::NORMALLY_OPEN)
    {
      zones[numzone].status = Zone::Status::ALARM;
    }
    else if (valuez > 1700 && valuez < 1900 && zones[numzone].sensor_type == SensorType::NORMALLY_CLOSED)
    {
      zones[numzone].status = Zone::Status::ALARM;
    }
    else
    {
      zones[numzone].status = Zone::Status::TROUBLE;
    }
  }
}

void setup()
{
  pinMode(LED, OUTPUT);

  pinMode(red_light_pin, OUTPUT);
  pinMode(green_light_pin, OUTPUT);
  //  pinMode(blue_light_pin, OUTPUT);

  Serial.begin(115200);
  system_status.armed_status = SystemArmedStatus::ARMED;
}

void TriggerSiren(Zone::Attributes za)
{
  static long siren_last_time = 0;
  if (millis() - siren_last_time > siren_time && za == Zone::Attributes::AUDIBLE)
  {
    siren_last_time = millis();
    switch (za)
    {
    case Zone::Attributes::AUDIBLE:
      /* code */
      break;
    case Zone::Attributes::PULSED:
      /* code */
      break;
    default:
      Serial.println("Trigger Siren undefined");
      break;
    }
  }
}

void CheckStatusSystem()
{
  system_status.zone_status = Zone::Status::UNDEFINED;
  int nz = sizeof(zones) / sizeof(Zone::Config);
  int num_zone = 0;

  // Sondea el estado de cada zona
  for (int i = 0; i < nz; i++)
  {
    GetZoneStatus(i);
  }

  // Verifica si hay alguna zona con problema
  for (int i = 0; i < nz; i++)
  {
    if (zones[i].status == Zone::Status::TROUBLE)
    {
      system_status.zone_status = Zone::Status::TROUBLE;
      break;
    }
  }

  // Emite señal cuando hay zona con problema
  if (system_status.zone_status == Zone::Status::TROUBLE)
  {
    BuzzerOUT.blink(15000, 2000);
  }
  else
  {
    BuzzerOUT.low();
  }

  // Verifica si hay alguna zona en alarma
  if (system_status.zone_status == Zone::Status::UNDEFINED)
  {
    // Verifica si hay alguna zona en alarma
    for (int i = 0; i < nz; i++)
    {
      if (zones[i].status == Zone::Status::ALARM)
      {
        system_status.zone_status = Zone::Status::ALARM;
        break;
      }
    }
  }

  if (system_status.zone_status == Zone::Status::UNDEFINED)
  {
    // Verifica si hay alguna zona en alarma
    for (int i = 0; i < nz; i++)
    {
      if (zones[i].status == Zone::Status::NORMAL)
      {
        system_status.zone_status = Zone::Status::NORMAL;
        num_zone = i;
        break;
      }
    }
  }
}

void loop()
{

  //  LedBlink();
  MainLED.loop();
  BuzzerOUT.loop();

  switch (system_status.armed_status)
  {
  case SystemArmedStatus::ARMED:
    MainLED.blink(2000, 2000);
    break;
  case SystemArmedStatus::ARMED_FORCED:
    MainLED.blink(1500, 2000);
    break;
  case SystemArmedStatus::ARMED_MEMORY_ALARM:
    MainLED.blink(800, 800);
    break;
  default:
    MainLED.blink(2500, 500);
    break;
  }

  CheckStatusSystem();
}