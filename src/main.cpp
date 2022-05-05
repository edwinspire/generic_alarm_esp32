#include <Arduino.h>
#include <string>
#include <ezOutput.h>
#include <WiFi.h>

#define LED 2

// Wifi section
const char *ssid = "edwinspire";
const char *password = "Caracol1980";

WiFiServer server(80);

const uint alarm_time = 39; // on seconds
// Inputs
const int Zone01 = 34; // Input 34 is zone sensor
const int Zone02 = 35; // Input 35 is zone sensor
const int Zone03 = 36; // Input 36 is zone sensor

// Outputs
int OUT_01 = 18; // As digital
int OUT_02 = 4;  // As digital

// Other config
bool allow_forced_arming = false; // Allows you to arm the system with open zones.
float zone_threshold = 25;        // 25%

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
  volatile uint trouble_zone;
  volatile uint alarm_audible;
  volatile uint alarm_silent;
  volatile uint alarm_pulsed;
  volatile uint alarm_zone;
};

SystemStatus system_status = {SystemArmedStatus::UNDEFINED, false};

Zone::Config zones[2] = {
    {Zone01, "Zone 01", 250, 0, Zone::Status::UNDEFINED, SensorType::NORMALLY_OPEN, Zone::Definition::INSTANT, true, Zone::Attributes::AUDIBLE},
    {Zone02, "Zone 02", 500, 0, Zone::Status::UNDEFINED, SensorType::NORMALLY_OPEN, Zone::Definition::ALWAYS_ARMED, false, Zone::Attributes::PULSED}};

// Get status zone
void GetZoneStatus(int numzone)
{
  if (millis() - zones[numzone].last_time > zones[numzone].interval && zones[numzone].definition != Zone::Definition::NO_USED)
  {
    zones[numzone].last_time = millis();
    float center = 4096 / 2;
    float upper_threshold = center + ((zone_threshold / 100) * center);
    float lower_threshold = center - ((zone_threshold / 100) * center);

    // Serial.print("Max " + String(upper_threshold) + " Min " + String(lower_threshold) + "\n\r");

    int valuez = analogRead(zones[numzone].pin);

    if (valuez > upper_threshold && zones[numzone].sensor_type == SensorType::NORMALLY_CLOSED)
    {
      zones[numzone].status = Zone::Status::NORMAL;
      Serial.print(zones[numzone].zone_label + " NORMAL > " + String(valuez) + "\n\r");
    }
    else if (valuez > lower_threshold && valuez < upper_threshold && zones[numzone].sensor_type == SensorType::NORMALLY_OPEN)
    {
      Serial.print(zones[numzone].zone_label + " NORMAL > " + String(valuez) + "\n\r");
      zones[numzone].status = Zone::Status::NORMAL;
    }
    else if (valuez > upper_threshold && zones[numzone].sensor_type == SensorType::NORMALLY_OPEN)
    {
      Serial.print(zones[numzone].zone_label + " ALARM > " + String(valuez) + "\n\r");
      zones[numzone].status = Zone::Status::ALARM;
    }
    else if (valuez > lower_threshold && valuez < upper_threshold && zones[numzone].sensor_type == SensorType::NORMALLY_CLOSED)
    {
      Serial.print(zones[numzone].zone_label + " ALARM > " + String(valuez) + "\n\r");
      zones[numzone].status = Zone::Status::ALARM;
    }
    else if (valuez <= lower_threshold)
    {
      Serial.print(zones[numzone].zone_label + " TROUBLE > " + String(valuez) + "\n\r");
      zones[numzone].status = Zone::Status::TROUBLE;
    }
  }
}

void setup()
{
  pinMode(LED, OUTPUT);

  Serial.begin(115200);
  system_status.armed_status = SystemArmedStatus::UNDEFINED;

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void ResetSystemStatus()
{
  system_status.alarm_audible = 0;
  system_status.alarm_pulsed = 0;
  system_status.alarm_silent = 0;
  system_status.alarm_zone = 0;
  system_status.trouble_zone = 0;
}

void ProcessZoneStatus(int zones_number)
{

  for (int i = 0; i < zones_number; i++)
  {
    switch (zones[i].status)
    {
    case Zone::Status::TROUBLE:
      system_status.trouble_zone++;
      // Serial.print(String(zones[i].zone_label) +" ProcessZoneStatus > " + String(system_status.trouble_zone) + "\n\r");
      break;
    case Zone::Status::ALARM:
      if (zones[i].definition != Zone::Definition::KEYSWITCH_ARM && zones[i].definition != Zone::Definition::KEYSWITCH_ARM && zones[i].definition != Zone::Definition::NO_USED)
      {

        system_status.alarm_zone++;

        if (system_status.armed_status == SystemArmedStatus::ARMED || system_status.armed_status == SystemArmedStatus::ARMED_FORCED || system_status.armed_status == SystemArmedStatus::ARMED_MEMORY_ALARM || zones[i].definition == Zone::ALWAYS_ARMED)
        {
          Serial.print(zones[i].zone_label + " SISTEMA EN ALARMA \n\r");
          system_status.armed_status = SystemArmedStatus::ARMED_MEMORY_ALARM;
          switch (zones[i].attributes)
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
    }
  }

  // It emits a signal when any zone is in trouble.
  if (system_status.trouble_zone > 0)
  {
    EZ_OUT_01.blink(5000, 2000);
  }
  else
  {
    EZ_OUT_01.low();
  }

  // It emits a signal when any zone is in alarm.
  if (system_status.alarm_zone > 0 && (system_status.armed_status == SystemArmedStatus::ARMED || system_status.armed_status == SystemArmedStatus::ARMED_FORCED || system_status.armed_status == SystemArmedStatus::ARMED_MEMORY_ALARM))
  {
    if (system_status.alarm_audible > 0)
    {
      // EZ_OUT_02.blink(1000, alarm_time * 1000, 5, 10);
      EZ_OUT_02.pulse((alarm_time * 1000) + 1000, 1000);
    }
    else if (system_status.alarm_pulsed > 0)
    {
      float blinkTimes = (alarm_time / 6) * 2;
      Serial.println("blinkTimes " + String(blinkTimes));
      EZ_OUT_02.blink(3000, 3000, 1000, blinkTimes); // alarm_time*1000/5, 5 porque el tiempo de HIGH Y LOW tienen 5 segundos
    }
  }

  // Other signals
  switch (system_status.armed_status)
  {
  case SystemArmedStatus::ARMED:
    MainLED.blink(3000, 2000);
    // Serial.println("SystemArmedStatus::ARMED");
    break;
  case SystemArmedStatus::ARMED_FORCED:
    Serial.println("SystemArmedStatus::ARMED_FORCED");
    MainLED.blink(1500, 2000);
    break;
  case SystemArmedStatus::ARMED_MEMORY_ALARM:
    // Serial.println("SystemArmedStatus::ARMED_MEMORY_ALARM");
    MainLED.blink(1500, 3000);
    break;
  default:
    if (system_status.trouble_zone > 0)
    {
      MainLED.blink(500, 500, 1000, 6);
    }
    else
    {
      //      Serial.println("SystemArmedStatus::OTHER " + String(system_status.armed_status));
      MainLED.blink(3000, 1000, 0, 4);
    }
    break;
  }
}

// Sin probar
void ArmForcedSystemNoAllowed()
{
  Serial.println("Sistema ARMADO FORZADO No permitido");
  EZ_OUT_02.low();
  EZ_OUT_02.blink(500, 2000, 250, 2);
  EZ_OUT_01.blink(500, 2000, 250, 2);
}

// Sin probar
void ArmForcedSystem()
{
  Serial.println("Sistema ARMADO FORZADO");
  EZ_OUT_02.low();
  system_status.armed_status = SystemArmedStatus::ARMED_FORCED;
  EZ_OUT_02.blink(250, 250, 250, 6);
}

// Probado
void ArmSystem()
{

  if (system_status.armed_status != SystemArmedStatus::ARMED)
  {
    Serial.println("Sistema ARMADO");
    EZ_OUT_02.low();
    EZ_OUT_02.blink(250, 250, 250, 4);
    system_status.armed_status = SystemArmedStatus::ARMED;
  }
  else
  {
    Serial.println("Sistema ya se encuentra ARMADO");
  }
}

void DisarmSystem()
{
  if (system_status.armed_status != SystemArmedStatus::DISARMED)
  {
    system_status.armed_status = SystemArmedStatus::DISARMED;
    ResetSystemStatus();
    EZ_OUT_02.low();
    EZ_OUT_02.blink(250, 250, 350, 2);
    Serial.println("Sistema DESARMADO");
  }
  else
  {
    Serial.println("Ya se encuentra DESARMADO");
  }
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
        ArmSystem();
      }
      else if (zones[i].status == Zone::Status::ALARM && system_status.alarm_zone > 0 && allow_forced_arming && (system_status.armed_status == SystemArmedStatus::DISARMED || system_status.armed_status == SystemArmedStatus::UNDEFINED))
      {
        ArmForcedSystem();
      }
      else if (zones[i].status == Zone::Status::ALARM && system_status.alarm_zone > 0 && !allow_forced_arming && (system_status.armed_status == SystemArmedStatus::DISARMED || system_status.armed_status == SystemArmedStatus::UNDEFINED))
      {
        // system_status.armed_status = SystemArmedStatus::ARMED_FORCED;
        ArmForcedSystemNoAllowed();
      }
      else if (zones[i].status == Zone::Status::NORMAL && system_status.armed_status != SystemArmedStatus::DISARMED)
      {
        DisarmSystem();
      }
      break;
    }
  }
  return KeyFound;
}

void CheckStatusSystem()
{
  // Reset status
  ResetSystemStatus();

  int nz = sizeof(zones) / sizeof(Zone::Config);
  int num_zone = 0;

  // Scan all zones to get their status
  for (int i = 0; i < nz; i++)
  {
    GetZoneStatus(i);
  }

  ProcessZoneStatus(nz);

  CheckKeySwitchZone(nz);
}

void WifiLoop()
{
  WiFiClient client = server.available(); // listen for incoming clients

  if (client)
  {                                // if you get a client,
    Serial.println("New Client."); // print a message out the serial port
    String currentLine = "";       // make a String to hold incoming data from the client
    while (client.connected())
    { // loop while the client's connected
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Serial.write(c);        // print it out the serial monitor
        if (c == '\n')
        { // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/H\">here</a> Armar.<br>");
            client.print("Click <a href=\"/L\">here</a> Desarmar.<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          }
          else
          { // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H"))
        {
          // digitalWrite(5, HIGH); // GET /H turns the LED on
          ArmSystem();
        }
        if (currentLine.endsWith("GET /L"))
        {
          // digitalWrite(5, LOW); // GET /L turns the LED off
          DisarmSystem();
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

void loop()
{
  MainLED.loop();
  EZ_OUT_01.loop();
  EZ_OUT_02.loop();

  CheckStatusSystem();
  WifiLoop();
  // delay(1000);
}