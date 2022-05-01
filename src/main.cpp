#include <Arduino.h>
#include <string>

#define LED 2

const int Zone01 = 34; // Input 34 is zone sensor
const int Zone02 = 35; // Input 35 is zone sensor
const int Zone03 = 36; // Input 36 is zone sensor

int red_light_pin = 4;
int green_light_pin = 5;
int blue_light_pin = 18;

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
    NO_USED,  // The zone will not operate in any way. Zones that are not used should be programmed as Null zones
    DELAY,    // If this zone is violated when the panel is armed it will provide entry delay
    INSTANT,  // If this zone type is violated when the panel is armed it will cause an instant alarm. Typically this zone is used for windows, patio doors or other perimeter type zones
    INTERIOR, // If this type of zone is violated when the panel is armed it will provide entry if a delay type zone was violated first. Otherwise it will cause an instant alarm. Typically this zone is used for interior protection devices, such as motion detectors.
    ARMED     // If this type of zone is violated when the panel is armed it will cause an instant alarm. Typically this zone is used for 24 hour zones.
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
  READY, // The system is disarmed and ready to be armed
  NOT_READY, // The system is disarmed, open zones are not ready to be armed
  ARMED, // The system is armed and ready to go
  ARMED_MEMORY_ALARM, // An alarm occurred while the system was armed.
  ARMED_FORCED, // System armed but with some zone open or in trouble
  UNDEFINED
};

struct SystemStatus{
  SystemArmedStatus armed_status;
  Zone::Status zone_status;
};

SystemStatus system_status = {SystemArmedStatus::UNDEFINED, Zone::Status::UNDEFINED};

Zone::Config zones[2] = {
    {Zone01, "Zone 01", 500, 0, Zone::Status::UNDEFINED, SensorType::NORMALLY_CLOSED, Zone::Definition::INSTANT, true, Zone::Attributes::AUDIBLE},
    {Zone02, "Zone 02", 1500, 0, Zone::Status::UNDEFINED, SensorType::NORMALLY_OPEN, Zone::Definition::NO_USED, false, Zone::Attributes::PULSED}
};

void GetZoneStatus(int numzone);

void GetZoneStatus(int numzone)
{
  if (millis() - zones[numzone].last_time > zones[numzone].interval && zones[numzone].definition != Zone::Definition::NO_USED)
  {
    zones[numzone].last_time = millis();

    int valuez = analogRead(zones[numzone].pin);
    // int valuez = 777;
    Serial.println(valuez);

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

void LedBlink()
{
  static long blink_last_time = 0;
  if (millis() - blink_last_time > 500)
  {
    blink_last_time = millis();
    digitalWrite(LED, !digitalRead(LED));
  }
}

void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
{
  static long RGB_color_last_time = 0;
  if (millis() - RGB_color_last_time > 500)
  {
    RGB_color_last_time = millis();
    analogWrite(red_light_pin, red_light_value);
    analogWrite(green_light_pin, green_light_value);
    analogWrite(blue_light_pin, blue_light_value);
  }
  /*
  analogWrite(red_light_pin, red_light_value);
  analogWrite(green_light_pin, green_light_value);
  analogWrite(blue_light_pin, blue_light_value);
  */
}

void setup()
{
  pinMode(LED, OUTPUT);

  pinMode(red_light_pin, OUTPUT);
  pinMode(green_light_pin, OUTPUT);
  pinMode(blue_light_pin, OUTPUT);

  Serial.begin(115200);
  delay(1000);
  RGB_color(255, 0, 0); // Red
  delay(1000);
  RGB_color(0, 255, 0); // Green
  delay(1000);
  RGB_color(0, 0, 255); // Blue
  delay(1000);
}

void GeneralStatusLed()
{
  system_status.zone_status = Zone::Status::UNDEFINED;
  int nz = sizeof(zones) / sizeof(Zone::Config);

  // Verifica si hay alguna zona con problema
  for (int i = 0; i < nz; i++)
  {
    if (zones[i].status == Zone::Status::TROUBLE)
    {
      system_status.zone_status = Zone::Status::TROUBLE;
      break;
    }
  }

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
        break;
      }
    }
  }

  switch (system_status.zone_status)
  {
  case Zone::Status::ALARM:
    RGB_color(255, 0, 0); // Red
    break;
  case Zone::Status::TROUBLE:
    RGB_color(0, 0, 255); // Blue
    break;
  case Zone::Status::NORMAL:
    RGB_color(0, 255, 0); // Green
    break;
  case Zone::Status::UNDEFINED:
    RGB_color(0, 0, 0); // Blue
    break;
  }
}

void loop()
{
  int nz = sizeof(zones) / sizeof(Zone::Config);

  // Sondea el estado de cada zona
  for (int i = 0; i < nz; i++)
  {
    GetZoneStatus(i);
  }

  LedBlink();

  GeneralStatusLed();
  /*
   RGB_color(255, 0, 0); // Red
    delay(500);
    RGB_color(0, 255, 0); // Green
    delay(500);
    RGB_color(0, 0, 255); // Blue
    delay(500);
    RGB_color(255, 255, 125); // Raspberry
    delay(500);
    RGB_color(0, 255, 255); // Cyan
    delay(500);
    RGB_color(255, 0, 255); // Magenta
    delay(500);
    RGB_color(255, 255, 0); // Yellow
    delay(1500);
    RGB_color(255, 255, 255); // White
    delay(500);
    */

  delay(100);
}