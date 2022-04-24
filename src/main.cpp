#include <Arduino.h>
// Include CoopTask since we want to manage multiple tasks.
#include <CoopTask.h>
#include <CoopSemaphore.h>

#if defined(ARDUINO_attiny)
#define LED_BUILTIN 1
#endif

#if defined(ARDUINO_AVR_MICRO)
#define STACKSIZE_8BIT 92
#else
#define STACKSIZE_8BIT 128
#endif

#define LED 2

void GetZoneStatus(int zone);

CoopSemaphore taskSema(1, 1);
int taskToken = 1;

enum ZoneStatus
{
  TROUBLE,
  NORMAL,
  ALARM,
  UNKNOWN
};

const int Zone01 = 34; // Input 34 is zone sensor
const int Zone02 = 35; // Input 34 is zone sensor
const int Zone03 = 36; // Input 34 is zone sensor

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
    Serial.println("ALARM! " + String(zone));
    //   digitalWrite(LED, HIGH);
    // delay(1000);
    break;
  case ZoneStatus::TROUBLE:
    Serial.println("TROUBLE!! " + String(zone));
    //    digitalWrite(LED, HIGH);
    // delay(500);
    // digitalWrite(LED, LOW);
    // delay(500);
    break;
  case ZoneStatus::NORMAL:
    Serial.println("NORMAL " + String(zone));
    // digitalWrite(LED, HIGH);
    // delay(1000);
    // digitalWrite(LED, LOW);
    // delay(1000);
    break;
  }
}

// Task no.1: blink LED with 1 second delay.
void z01()
{
  for (;;) // explicitly run forever without returning
  {
    taskSema.wait();
    if (1 != taskToken)
    {
      taskSema.post();
      yield();
      continue;
    }
    delay(500);
    GetZoneStatus(Zone01);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);

    taskToken = 2;
    taskSema.post();
  }
}

void z02()
{
  for (;;) // explicitly run forever without returning
  {
    // Serial.println("Z02 " + String(taskToken));

    taskSema.wait();
    if (2 != taskToken)
    {
      taskSema.post();
      yield();
      continue;
    }
    delay(500);
    GetZoneStatus(Zone02);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);

    taskToken = 3;
    taskSema.post();
  }
}

void z03()
{
  for (;;) // explicitly run forever without returning
  {
    taskSema.wait();
    if (3 != taskToken)
    {
      taskSema.post();
      yield();
      continue;
    }

    delay(500);
    GetZoneStatus(Zone03);
    digitalWrite(LED_BUILTIN, LOW);
    delay(5000);

    taskToken = 1;
    taskSema.post();
  }
}

BasicCoopTask<CoopTaskStackAllocatorAsMember<sizeof(unsigned) >= 4 ? 2048 : STACKSIZE_8BIT>> task1("Z01", z01);
BasicCoopTask<CoopTaskStackAllocatorAsMember<sizeof(unsigned) >= 4 ? 2048 : STACKSIZE_8BIT>> task2("Z02", z02);
BasicCoopTask<CoopTaskStackAllocatorFromLoop<sizeof(unsigned) >= 4 ? 2048 : STACKSIZE_8BIT>> task3("Z03", z03);

void setup()
{
  pinMode(LED, OUTPUT);
  Serial.begin(115200);

  // Add "loop1", "loop2" and "loop3" to CoopTask scheduling.
  // "loop" is always started by default, and is not under the control of CoopTask.
  task1.scheduleTask();
  task2.scheduleTask();
  task3.scheduleTask();

  delay(1000);
}
void loop()
{
  // digitalWrite(LED, HIGH);
  // delay(1000);
  // digitalWrite(LED, LOW);
  // delay(500);
  // GetZoneStatus(Zone01);
  // loops forever by default
  runCoopTasks();
}