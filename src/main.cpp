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
  NORMALLY_CLOSED,
  NOT_USED
};

enum ZoneStatus
{
  TROUBLE,
  NORMAL,
  ALARM,
  UNKNOWN
};

ZoneStatus generalStatus = ZoneStatus::UNKNOWN;

struct Zone
{
  int pin;
  String zone_label;
  long interval;
  long last_time;
  ZoneStatus status;
  SensorType sensor_type;
};

Zone zones[2] = {
    {Zone01, "Zone 01", 500, 0, ZoneStatus::UNKNOWN, SensorType::NORMALLY_CLOSED},
    {Zone02, "Zone 02", 1500, 0, ZoneStatus::UNKNOWN, SensorType::NOT_USED}
    //{Zone03, "Zone 03", 5000, 0, ZoneStatus::UNKNOWN},
};

void GetZoneStatus(int numzone);

void GetZoneStatus(int numzone)
{
  if (millis() - zones[numzone].last_time > zones[numzone].interval && zones[numzone].sensor_type != SensorType::NOT_USED)
  {
    zones[numzone].last_time = millis();

    int valuez = analogRead(zones[numzone].pin);
    // int valuez = 777;
    Serial.println(valuez);

    if (valuez > 4000 && zones[numzone].sensor_type == SensorType::NORMALLY_CLOSED)
    {
      zones[numzone].status = ZoneStatus::NORMAL;
    }
    else if (valuez > 1700 && valuez < 1900 && zones[numzone].sensor_type == SensorType::NORMALLY_OPEN)
    {
      zones[numzone].status = ZoneStatus::NORMAL;
    }
    else if (valuez > 4000 && zones[numzone].sensor_type == SensorType::NORMALLY_OPEN)
    {
      zones[numzone].status = ZoneStatus::ALARM;
    }
    else if (valuez > 1700 && valuez < 1900 && zones[numzone].sensor_type == SensorType::NORMALLY_CLOSED)
    {
      zones[numzone].status = ZoneStatus::ALARM;
    }
    else
    {
      zones[numzone].status = ZoneStatus::TROUBLE;
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
  generalStatus = ZoneStatus::UNKNOWN;
  int nz = sizeof(zones) / sizeof(Zone);

  // Verifica si hay alguna zona con problema
  for (int i = 0; i < nz; i++)
  {
    if (zones[i].status == ZoneStatus::TROUBLE)
    {
      generalStatus = ZoneStatus::TROUBLE;
      break;
    }
  }

  if (generalStatus == ZoneStatus::UNKNOWN)
  {
    // Verifica si hay alguna zona en alarma
    for (int i = 0; i < nz; i++)
    {
      if (zones[i].status == ZoneStatus::ALARM)
      {
        generalStatus = ZoneStatus::ALARM;
        break;
      }
    }
  }

  if (generalStatus == ZoneStatus::UNKNOWN)
  {
    // Verifica si hay alguna zona en alarma
    for (int i = 0; i < nz; i++)
    {
      if (zones[i].status == ZoneStatus::NORMAL)
      {
        generalStatus = ZoneStatus::NORMAL;
        break;
      }
    }
  }

  switch (generalStatus)
  {
  case ZoneStatus::ALARM:
    RGB_color(255, 0, 0); // Red
    break;
  case ZoneStatus::TROUBLE:
    RGB_color(0, 0, 255); // Blue
    break;
  case ZoneStatus::NORMAL:
    RGB_color(0, 255, 0); // Green
    break;
  case ZoneStatus::UNKNOWN:
    RGB_color(0, 0, 0); // Blue
    break;
  }
}

void loop()
{
  int nz = sizeof(zones) / sizeof(Zone);

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