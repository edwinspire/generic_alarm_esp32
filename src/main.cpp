#include <Arduino.h>
#include <string>
#include <ezOutput.h>

#define LED 2

const int alarm_time = 30; // on seconds
// Inputs
const int Zone01 = 34; // Input 34 is zone sensor
const int Zone02 = 35; // Input 35 is zone sensor
const int Zone03 = 36; // Input 36 is zone sensor

// Outputs
int OUT_01 = 18; // As digital
int OUT_02 = 4;  // As digital

ezOutput MainLED(LED);
ezOutput EZ_OUT_01(OUT_01); // Best used to connect a buzzer
ezOutput EZ_OUT_02(OUT_02); // Best used to connect a buzzer

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
    NO_USED,                // The zone will not operate in any way. Zones that are not used should be programmed as Null zones
    DELAY,                  // If this zone is violated when the panel is armed it will provide entry delay
    INSTANT,                // If this zone type is violated when the panel is armed it will cause an instant alarm. Typically this zone is used for windows, patio doors or other perimeter type zones
    INTERIOR,               // If this type of zone is violated when the panel is armed it will provide entry if a delay type zone was violated first. Otherwise it will cause an instant alarm. Typically this zone is used for interior protection devices, such as motion detectors.
    ALWAYS_ARMED,           // If this type of zone is violated when the panel is armed or disarmed it will cause an instant alarm. Typically this zone is used for 24 hour zones.
    KEYSWITCH_ARM,          // When this zone is violated, the system will arm. When this zone is secured, the system will disarm.
    MOMENTARY_KEYSWITCH_ARM // Momentary violation of this zone will alternately arm/disarm the system.
  };

  enum Attributes
  {
    AUDIBLE,
    SILENT,
    PULSED,
    NONE
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
  DISARMED_READY,     // The system is disarmed and ready to be armed
  DISARMED_NOT_READY, // The system is disarmed, open zones are not ready to be armed
  ARMED,              // The system is armed and ready to go
  ARMED_MEMORY_ALARM, // An alarm occurred while the system was armed.
  ARMED_FORCED,       // System armed but with some zone open or in trouble
  UNDEFINED
};

struct SystemStatus
{
  SystemArmedStatus armed_status;
  bool trouble_zone;
  bool alarm_audible;
  bool alarm_silent;
  bool alarm_pulsed;
  bool alarm_zone;
  // Zone::Status zone_status;
  // Zone::Attributes zone_attributes;
};

SystemStatus system_status = {SystemArmedStatus::UNDEFINED, false};

Zone::Config zones[2] = {
    {Zone01, "Zone 01", 500, 0, Zone::Status::UNDEFINED, SensorType::NORMALLY_CLOSED, Zone::Definition::INSTANT, true, Zone::Attributes::AUDIBLE},
    {Zone02, "Zone 02", 1500, 0, Zone::Status::UNDEFINED, SensorType::NORMALLY_OPEN, Zone::Definition::MOMENTARY_KEYSWITCH_ARM, false, Zone::Attributes::NONE}};

// Get status zone
void GetZoneStatus(int numzone)
{
  if (millis() - zones[numzone].last_time > zones[numzone].interval && zones[numzone].definition != Zone::Definition::NO_USED)
  {
    zones[numzone].last_time = millis();

    int valuez = analogRead(zones[numzone].pin);

    if (valuez > 4000 && zones[numzone].sensor_type == SensorType::NORMALLY_CLOSED)
    {
      zones[numzone].status = Zone::Status::NORMAL;
      Serial.print(zones[numzone].zone_label + " NORMAL > " + String(valuez) + "\n\r");
    }
    else if (valuez > 1700 && valuez < 1900 && zones[numzone].sensor_type == SensorType::NORMALLY_OPEN)
    {
      Serial.print(zones[numzone].zone_label + " NORMAL > " + String(valuez) + "\n\r");
      zones[numzone].status = Zone::Status::NORMAL;
    }
    else if (valuez > 4000 && zones[numzone].sensor_type == SensorType::NORMALLY_OPEN)
    {
      Serial.print(zones[numzone].zone_label + " ALARM > " + String(valuez) + "\n\r");
      zones[numzone].status = Zone::Status::ALARM;
    }
    else if (valuez > 1700 && valuez < 1900 && zones[numzone].sensor_type == SensorType::NORMALLY_CLOSED)
    {
      Serial.print(zones[numzone].zone_label + " ALARM > " + String(valuez) + "\n\r");
      zones[numzone].status = Zone::Status::ALARM;
    }
    else
    {
      Serial.print(zones[numzone].zone_label + " TROUBLE > " + String(valuez) + "\n\r");
      zones[numzone].status = Zone::Status::TROUBLE;
    }
  }
}

void setup()
{
  pinMode(LED, OUTPUT);

  //  pinMode(red_light_pin, OUTPUT);
  //  pinMode(green_light_pin, OUTPUT);
  //  pinMode(blue_light_pin, OUTPUT);

  Serial.begin(115200);
  system_status.armed_status = SystemArmedStatus::DISARMED_READY;
}

void CheckStatusSystem()
{
  // Reset status
  // system_status.armed_status = SystemArmedStatus::UNDEFINED; // Debe mantener el último estado
  system_status.alarm_audible = false;
  system_status.alarm_pulsed = false;
  system_status.alarm_silent = false;
  system_status.alarm_zone = false;
  system_status.trouble_zone = false;

  int nz = sizeof(zones) / sizeof(Zone::Config);
  int num_zone = 0;

  // Scan all zones to get their status
  for (int i = 0; i < nz; i++)
  {
    GetZoneStatus(i);
  }

  // Check if any area has a problem.
  for (int i = 0; i < nz; i++)
  {
    if (zones[i].status == Zone::Status::TROUBLE)
    {
      system_status.trouble_zone = true;
      break;
    }
  }

  // It emits a signal when any zone is in trouble.
  if (system_status.trouble_zone)
  {
    EZ_OUT_01.blink(15000, 2000);
  }
  else
  {
    EZ_OUT_01.low();
  }

  // Check if any zone is alarm
  for (int i = 0; i < nz; i++)
  {

    if (zones[i].definition == Zone::KEYSWITCH_ARM)
    {

      if (zones[i].status == Zone::Status::ALARM)
      {
        system_status.armed_status = SystemArmedStatus::ARMED;
      }
      else if (zones[i].status == Zone::Status::NORMAL)
      {
        system_status.armed_status = SystemArmedStatus::DISARMED_READY;
      }
    }
    else if ((zones[i].status == Zone::Status::ALARM && (system_status.armed_status == SystemArmedStatus::ARMED || system_status.armed_status == SystemArmedStatus::ARMED_FORCED || system_status.armed_status == SystemArmedStatus::ARMED_MEMORY_ALARM)) || zones[i].definition == Zone::ALWAYS_ARMED)
    {
      Serial.print(zones[i].zone_label + " SYSTEMA EN ALARMA \n\r");
      switch (zones[i].attributes)
      {
      case Zone::Attributes::AUDIBLE:
        system_status.alarm_audible = true;
        break;
      case Zone::Attributes::PULSED:
        system_status.alarm_pulsed = true;
        break;
      case Zone::Attributes::SILENT:
        system_status.alarm_silent = true;
        break;
      }
    }
  }

  // It emits a signal when any zone is in alarm.

  if (system_status.alarm_audible)
  {
    EZ_OUT_02.pulse(alarm_time * 1000);
  }
  else if (system_status.alarm_pulsed)
  {
    EZ_OUT_02.blink(3000, 2000, 0, (alarm_time*1000/5)); // alarm_time*1000/5, 5 porque el tiempo de HIGH Y LOW tienen 5 segundos
  }
  else
  {
    EZ_OUT_02.low();
  }

  // Verifica si hay alguna zona en alarma
  for (int i = 0; i < nz; i++)
  {
    if (zones[i].status == Zone::Status::NORMAL)
    {
      //      system_status.zone_status = Zone::Status::NORMAL;
      //    num_zone = i;
      break;
    }
  }
}

void loop()
{
  MainLED.loop();
  EZ_OUT_01.loop();
  EZ_OUT_02.loop();

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