#include <Arduino.h>
#include <string>

#define LED 2

const int Zone01 = 34; // Input 34 is zone sensor
const int Zone02 = 35; // Input 35 is zone sensor
const int Zone03 = 36; // Input 36 is zone sensor

int red_light_pin = 4;
int green_light_pin = 5;
int blue_light_pin = 18;

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

Zone zones[1] = {
    {Zone01, "Zone 01", 500, 0, ZoneStatus::UNKNOWN}
    //{Zone02, "Zone 02", 1000, 0, ZoneStatus::UNKNOWN},
    //{Zone03, "Zone 03", 5000, 0, ZoneStatus::UNKNOWN},
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

void loop()
{
  int nz = sizeof(zones) / sizeof(Zone);

  // Sondea el estado de cada zona
  for (int i = 0; i < nz; i++)
  {
    GetZoneStatus(i);
  }

  // Muestra en LED el estado de cada zona
  for (int i = 0; i < nz; i++)
  {
    if (zones[i].status == ZoneStatus::TROUBLE)
    {
      RGB_color(0, 0, 255); // Blue
      break;
    }

    else if (zones[i].status == ZoneStatus::ALARM)
    {
      RGB_color(255, 0, 0); // Red
      break;
    }
    else if (zones[i].status == ZoneStatus::NORMAL)
    {
      RGB_color(0, 255, 0); // Green
      break;
    }
    else
    {
      RGB_color(0, 0, 0);
    }
  }

  LedBlink();
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

  delay(500);
}