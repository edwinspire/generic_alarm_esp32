#include <Arduino.h>
#include <string>
#include <ezOutput.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "CtrlMaster.cpp"
#include "SPIFFS.h"
/*
  SD card test for esp32

  This example shows how use the utility libraries

  The circuit:
    SD card attached to SPI bus as follows:
        SS    = 5;
        MOSI  = 23;
        MISO  = 19;
        SCK   = 18;


   by Mischianti Renzo <https://www.mischianti.org>

   https://www.mischianti.org
*/
// include the SD library:
#include "FS.h"
#include "SD.h"
#include "SPI.h"
const int chipSelect = SS;

// Set Wifi AP - these to your desired credentials.
const char *ssid = "edwinspire";
const char *password = "Caracol1980";

// WiFiServer server(80);

// Create AsyncWebServer object on port 80
AsyncWebServer HttpServer(80);
AsyncWebSocket ws("/ws");

void notifyClients()
{
  DynamicJsonDocument doc(1024);
  doc["armed_status"] = system_status.armed_status;
  doc["trouble_zone"] = system_status.trouble_zone;
  doc["alarm_audible"] = system_status.alarm_audible;
  doc["alarm_pulsed"] = system_status.alarm_pulsed;
  doc["alarm_silent"] = system_status.alarm_silent;
  doc["alarm_zone"] = system_status.alarm_zone;
  doc["alarm_memory"] = system_status.alarm_memory;
  doc["output_02"] = system_status.output_02;

  int nz = sizeof(zones) / sizeof(Zone::Config);
  int num_zone = 0;
  Serial.println("Algo ha cambiado");
  // Scan all zones to get their status
  for (int i = 0; i < nz; i++)
  {
    doc["zones"][i]["memory_alarm"] = zones[i].alarm_memory;
    doc["zones"][i]["zone_label"] = zones[i].zone_label;
    doc["zones"][i]["gpio"] = zones[i].gpio;
    doc["zones"][i]["status"] = zones[i].status;
    doc["zones"][i]["sensor_type"] = zones[i].sensor_type;
    doc["zones"][i]["definition"] = zones[i].definition;
    doc["zones"][i]["attributes"] = zones[i].attributes;
    doc["zones"][i]["chime"] = zones[i].chime;
  }

  String returnJson;
  serializeJson(doc, returnJson);

  ws.textAll(returnJson);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;

    Serial.println((char *)data);

    DynamicJsonDocument ws_request(1024);
    DeserializationError error = deserializeJson(ws_request, (char *)data);

    Serial.println(error.c_str());

    if (ws_request["armed_status"] >= 0)
    {
      if (ws_request["armed_status"] == 0)
      {
        DisarmSystem();
      }
      else
      {
        ArmSystem();
      }
      notifyClients();
    }
    else if (ws_request["action"] == "softbutton" && ws_request["zone"]["gpio"] >= 1000)
    {
      int nz = sizeof(zones) / sizeof(Zone::Config);
      Serial.println("Softbutton Press");
      // Scan all zones to get their status
      for (int i = 0; i < nz; i++)
      {
        if (zones[i].gpio == ws_request["zone"]["gpio"])
        {
          if (ws_request["zone"]["status"] == 1)
          {
            zones[i].status = Zone::Status::NORMAL;
          }
          else if (ws_request["zone"]["status"] == 2)
          {
            zones[i].status = Zone::Status::ALARM;
          }
          break;
        }
      }
    }

    /*
        if (strcmp((char *)data, "toggle") == 0)
        {
          // ledState = !ledState;
          Serial.print("Cambiar estado");


                if (system_status.armed_status == SystemArmedStatus::ARMED)
                {
                  DisarmSystem();
                }
                else
                {
                  ArmSystem();
                }


          // Se mantiene esta notificación porque en el loop proncipal no se logra detectar el cambio de estado
          notifyClients();
        }
        */
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    notifyClients();
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  HttpServer.addHandler(&ws);
}

String processor(const String &var)
{
  Serial.println(var);
  if (var == "STATE")
  {
    if (system_status.armed_status == SystemArmedStatus::ARMED)
    {
      return "ON";
    }
    else
    {
      return "OFF";
    }
  }
  return String();
}

void printDirectory(File dir, int numTabs)
{
  while (true)
  {

    File entry = dir.openNextFile();
    if (!entry)
    {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++)
    {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory())
    {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    }
    else
    {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.print(entry.size(), DEC);
      time_t lw = entry.getLastWrite();
      struct tm *tmstruct = localtime(&lw);
      Serial.printf("\tLAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
    entry.close();
  }
}

void initSDCard()
{
  //  if (!SD.begin(SS))
  if (!SD.begin(SS))
  {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC)
  {
    Serial.println("MMC");
  }
  else if (cardType == CARD_SD)
  {
    Serial.println("SDSC");
  }
  else if (cardType == CARD_SDHC)
  {
    Serial.println("SDHC");
  }
  else
  {
    Serial.println("UNKNOWN");
  }

  Serial.print("Card size:  ");
  Serial.println((float)SD.cardSize() / 1000);

  Serial.print("Total bytes: ");
  Serial.println(SD.totalBytes());

  Serial.print("Used bytes: ");
  Serial.println(SD.usedBytes());
  // File dir = SD.open("/");
  // printDirectory(dir, 0);
}

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void WifiWebSocketServerSetup()
{
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  initWebSocket();

  HttpServer.serveStatic("/", SD, "/webserver/public/").setDefaultFile("index.html");

  HttpServer.onNotFound([](AsyncWebServerRequest *request)
                        { request->send(404, "text/plain", F("The content you are looking for was not found. Check SDCard")); });
  // Start server
  HttpServer.begin();
}

void setup()
{
  pinMode(LED, OUTPUT);

  Serial.begin(115200);
  system_status.armed_status = SystemArmedStatus::UNDEFINED;

  initSDCard();
  WifiWebSocketServerSetup();
}

void loop()
{
  MainLED.loop();
  EZ_OUT_01.loop();
  EZ_OUT_02.loop();

  // Serial.println("Status " + String(system_status.armed_status));
  // Serial.println("Status Last " + String(system_status_last.armed_status));
  if (SystemStatusLoop() || system_status.alarm_zone != system_status_last.alarm_zone || system_status.armed_status != system_status_last.armed_status || system_status.trouble_zone != system_status_last.trouble_zone)
  {
    notifyClients();
  }
}