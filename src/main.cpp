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

// Other config
bool allow_forced_arming = false; // Allows you to arm the system with open zones.

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
  DISARMED,           // The system is disarmed and ready to be armed
  ARMED,              // The system is armed and ready to go
  ARMED_MEMORY_ALARM, // An alarm occurred while the system was armed.
  ARMED_FORCED,       // System armed but with some zone open or in trouble
  UNDEFINED
};

struct SystemStatus
{
  SystemArmedStatus armed_status;
  int trouble_zone;
  int alarm_audible;
  int alarm_silent;
  int alarm_pulsed;
  int alarm_zone;
  // Zone::Status zone_status;
  // Zone::Attributes zone_attributes;
};

SystemStatus system_status = {SystemArmedStatus::UNDEFINED, false};

Zone::Config zones[2] = {
    {Zone01, "Zone 01", 500, 0, Zone::Status::UNDEFINED, SensorType::NORMALLY_OPEN, Zone::Definition::INSTANT, true, Zone::Attributes::AUDIBLE},
    {Zone02, "Zone 02", 500, 0, Zone::Status::UNDEFINED, SensorType::NORMALLY_OPEN, Zone::Definition::KEYSWITCH_ARM, false, Zone::Attributes::NONE}};

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
    else if (valuez > 1800 && valuez < 3000 && zones[numzone].sensor_type == SensorType::NORMALLY_OPEN)
    {
      Serial.print(zones[numzone].zone_label + " NORMAL > " + String(valuez) + "\n\r");
      zones[numzone].status = Zone::Status::NORMAL;
    }
    else if (valuez > 4000 && zones[numzone].sensor_type == SensorType::NORMALLY_OPEN)
    {
      // Serial.print(zones[numzone].zone_label + " ALARM > " + String(valuez) + "\n\r");
      zones[numzone].status = Zone::Status::ALARM;
    }
    else if (valuez > 1800 && valuez < 3000 && zones[numzone].sensor_type == SensorType::NORMALLY_CLOSED)
    {
      // Serial.print(zones[numzone].zone_label + " ALARM > " + String(valuez) + "\n\r");
      zones[numzone].status = Zone::Status::ALARM;
    }
    else
    {
      Serial.print(zones[numzone].zone_label + " TROUBLE > " + String(valuez) + "\n\r");
      zones[numzone].status = Zone::Status::TROUBLE;
    }

    switch (zones[numzone].status)
    {
    case Zone::Status::TROUBLE:
      system_status.trouble_zone++;
      Serial.print(" system_status.trouble_zone > " + String(system_status.trouble_zone) + "\n\r");
      break;
    case Zone::Status::ALARM:
      if (zones[numzone].definition != Zone::Definition::KEYSWITCH_ARM && zones[numzone].definition != Zone::Definition::KEYSWITCH_ARM && zones[numzone].definition != Zone::Definition::NO_USED)
      {

        system_status.alarm_zone++;

        if (system_status.armed_status == SystemArmedStatus::ARMED || system_status.armed_status == SystemArmedStatus::ARMED_FORCED || system_status.armed_status == SystemArmedStatus::ARMED_MEMORY_ALARM || zones[numzone].definition == Zone::ALWAYS_ARMED)
        {
          Serial.print(zones[numzone].zone_label + " SISTEMA EN ALARMA \n\r");
          system_status.armed_status = SystemArmedStatus::ARMED_MEMORY_ALARM;
          switch (zones[numzone].attributes)
          {
          case Zone::Attributes::AUDIBLE:
            system_status.alarm_audible++;
            break;
          case Zone::Attributes::PULSED:
            system_status.alarm_pulsed++;
            break;
          case Zone::Attributes::SILENT:
            system_status.alarm_silent++;
            break;
          }
        }
      }
      break;
    default:
      break;
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
  system_status.armed_status = SystemArmedStatus::UNDEFINED;
}

bool CheckKeySwitchZone(int zones_number)
{
  bool KeyFound = false;
  // Check by zone KEYSWITCH_ARM status, only read first found
  for (int i = 0; i < zones_number; i++)
  {

    if (zones[i].definition == Zone::KEYSWITCH_ARM)
    {
      KeyFound = true;
      if (zones[i].status == Zone::Status::ALARM && system_status.alarm_zone == 0 && (system_status.armed_status == SystemArmedStatus::DISARMED || system_status.armed_status == SystemArmedStatus::UNDEFINED))
      {
        system_status.armed_status = SystemArmedStatus::ARMED;
        EZ_OUT_02.blink(500, 500, 100, 2);
      }
      else if (zones[i].status == Zone::Status::ALARM && system_status.alarm_zone > 0 && allow_forced_arming && (system_status.armed_status == SystemArmedStatus::DISARMED || system_status.armed_status == SystemArmedStatus::UNDEFINED))
      {
        system_status.armed_status = SystemArmedStatus::ARMED_FORCED;
        EZ_OUT_02.blink(500, 500, 100, 3);
      }
      else if (zones[i].status == Zone::Status::ALARM && system_status.alarm_zone > 0 && !allow_forced_arming && (system_status.armed_status == SystemArmedStatus::DISARMED || system_status.armed_status == SystemArmedStatus::UNDEFINED))
      {
        // system_status.armed_status = SystemArmedStatus::ARMED_FORCED;
        EZ_OUT_02.blink(500, 500, 100, 3);
        EZ_OUT_01.blink(500, 500, 100, 3);
      }
      else if (zones[i].status == Zone::Status::NORMAL)
      {
        system_status.armed_status = SystemArmedStatus::DISARMED;
        system_status.alarm_audible = 0;
        system_status.alarm_pulsed = 0;
        system_status.alarm_silent = 0;
        system_status.alarm_zone = 0;
        EZ_OUT_02.blink(500, 500, 100, 1);
      }
      break;
    }
  }
  return KeyFound;
}

void CheckStatusSystem()
{
  // Reset status
  // system_status.armed_status = SystemArmedStatus::UNDEFINED; // Debe mantener el último estado
  system_status.alarm_audible = 0;
  system_status.alarm_pulsed = 0;
  system_status.alarm_silent = 0;
  system_status.alarm_zone = 0;
  system_status.trouble_zone = 0;

  int nz = sizeof(zones) / sizeof(Zone::Config);
  int num_zone = 0;

  // Scan all zones to get their status
  for (int i = 0; i < nz; i++)
  {
    GetZoneStatus(i);
  }

  // It emits a signal when any zone is in trouble.

  //Serial.println("system_status.trouble_zone " + String(system_status.trouble_zone));
  if (system_status.trouble_zone > 0)
  {
    Serial.println("system_status.trouble_zone BLINK ");
    EZ_OUT_01.blink(3000, 2000);
  }
  else
  {
    EZ_OUT_01.low();
  }

  CheckKeySwitchZone(nz);

  // It emits a signal when any zone is in alarm.
  if (system_status.alarm_zone > 0 && (system_status.armed_status == SystemArmedStatus::ARMED || system_status.armed_status == SystemArmedStatus::ARMED_FORCED || system_status.armed_status == SystemArmedStatus::ARMED_MEMORY_ALARM))
  {
    if (system_status.alarm_audible > 0)
    {
      EZ_OUT_02.pulse(alarm_time * 1000, 1000);
    }
    else if (system_status.alarm_pulsed > 0)
    {
      EZ_OUT_02.blink(3000, 2000, 500, (alarm_time * 1000 * 5)); // alarm_time*1000/5, 5 porque el tiempo de HIGH Y LOW tienen 5 segundos
    }
  }
  else
  {
    EZ_OUT_02.low();
  }
}

void loop()
{
  MainLED.loop();
  EZ_OUT_01.loop();
  EZ_OUT_02.loop();

  CheckStatusSystem();

  switch (system_status.armed_status)
  {
  case SystemArmedStatus::ARMED:
    MainLED.blink(2000, 2000);
    // Serial.println("SystemArmedStatus::ARMED");
    break;
  case SystemArmedStatus::ARMED_FORCED:
    Serial.println("SystemArmedStatus::ARMED_FORCED");
    MainLED.blink(1500, 2000);
    break;
  case SystemArmedStatus::ARMED_MEMORY_ALARM:
    // Serial.println("SystemArmedStatus::ARMED_MEMORY_ALARM");
    MainLED.blink(500, 500, 500, 3);
    break;
  default:
    if (system_status.trouble_zone > 0)
    {
      MainLED.blink(500, 500, 2, 3);
    }
    else
    {
      //      Serial.println("SystemArmedStatus::OTHER " + String(system_status.armed_status));
      MainLED.blink(2500, 500);
    }
    break;
  }
   delay(1000);
}