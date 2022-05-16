#include <Arduino.h>
#include <string>
#include <ezOutput.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "CtrlMaster.cpp"

// Set Wifi AP - these to your desired credentials.
const char *ssid = "edwinspire";
const char *password = "1234567";

// WiFiServer server(80);

// Create AsyncWebServer object on port 80
AsyncWebServer HttpServer(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><title>ESP32 Generic Alarm</title><meta name="viewport" content="width=device-width,initial-scale=1"><style>html{font-family:Arial,Helvetica,sans-serif;text-align:center}h1{font-size:1.8rem;color:#fff}h2{font-size:1.5rem;font-weight:700;color:#143642}.topnav{overflow:hidden;background-color:#143642}body{margin:0}.card{background-color:#f8f7f9;box-shadow:2px 2px 12px 1px rgba(140,140,140,.5);padding-top:10px;padding-bottom:20px}.switch{position:relative;display:inline-block;width:60px;height:34px}.switch input{opacity:0;width:0;height:0}.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#ccc;-webkit-transition:.4s;transition:.4s}.slider:before{position:absolute;content:"";height:26px;width:26px;left:4px;bottom:4px;background-color:#fff;-webkit-transition:.4s;transition:.4s}input:checked+.slider{background-color:#2196f3}input:checked+.slider:before{-webkit-transform:translateX(26px);-ms-transform:translateX(26px);transform:translateX(26px)}.slider.round{border-radius:34px}.slider.round:before{border-radius:50%}table{border-collapse:collapse;width:100%;width:-webkit-fill-available}td,th{padding:8px;text-align:left;border-bottom:1px solid #ddd}tr:hover{background-color:#d6eeee}.on_memory_alarm{background-color:#ff4500}.on_alarm{background-color:#db1919}.content{padding:30px;max-width:75%;margin:0 auto}</style><title>ESP Web Server</title><meta name="viewport" content="width=device-width,initial-scale=1"><link rel="icon" href="data:,"></head><body><div class="topnav"><h1>ESP32 WebSocket Server</h1></div><div class="content"><div class="card" id="CardStatus"><h2>Arm Status</h2><div><label class="switch"><input type="checkbox" checked id="ArmStatus"> <span class="slider"></span></label></div></div><p></p><div class="card"><h2>Zone Status</h2><p>Muestra el estado general de las zonas</p><table><tr><th>GPIO</th><th>Label</th><th>Status</th><th>Sensor Type</th><th>Definition</th><th>Chime</th><th>Attributes</th><th>Memory</th></tr><tr><td id="c_1_1">-</td><td id="c_1_2">-</td><td id="c_1_3">-</td><td id="c_1_4">-</td><td id="c_1_5">-</td><td id="c_1_6">-</td><td id="c_1_7">-</td><td id="c_1_8">-</td></tr><tr><td id="c_2_1">-</td><td id="c_2_2">-</td><td id="c_2_3">-</td><td id="c_2_4">-</td><td id="c_2_5">-</td><td id="c_2_6">-</td><td id="c_2_7">-</td><td id="c_2_8">-</td></tr><tr><td id="c_3_1">-</td><td id="c_3_2">-</td><td id="c_3_3">-</td><td id="c_3_4">-</td><td id="c_3_5">-</td><td id="c_3_6">-</td><td id="c_3_7">-</td><td id="c_3_8">-</td></tr></table></div></div><script>var gateway = `ws://${window.location.hostname}/ws`;
        var websocket;
        window.addEventListener("load", onLoad);
        function initWebSocket() {
            console.log("Trying to open a WebSocket connection...");
            websocket = new WebSocket(gateway);
            websocket.onopen = onOpen;
            websocket.onclose = onClose;
            websocket.onmessage = onMessage; // <-- add this line
        }
        function onOpen(event) {
            console.log("Connection opened");
        }
        function onClose(event) {
            console.log("Connection closed");
            setTimeout(initWebSocket, 2000);
        }
        function onMessage(event) {
            var state;
            //console.log('Message received: ' + event.data);
            try {
                let data = JSON.parse(event.data);
                console.log(data);
                let inputArmStatus = document.getElementById("ArmStatus");
                let CardStatus = document.getElementById("CardStatus");


                if (data.armed_status == 0) {
                    inputArmStatus.checked = false;
                } else {
                    inputArmStatus.checked = true;
                }

                if (data.alarm_audible > 0 || data.alarm_pulsed > 0 || data.alarm_silent > 0 || data.alarm_zone > 0) {
                    CardStatus.style.backgroundColor = "red";
                } else if (data.alarm_memory > 0) {
                    CardStatus.style.backgroundColor = "orange";
                } else {
                    CardStatus.style.backgroundColor = "white";
                }


                if (data.zones && Array.isArray(data.zones)) {
                    data.zones.forEach((item, i) => {

                        document.getElementById(`c_${i + 1}_1`).innerHTML = item.gpio;
                        document.getElementById(`c_${i + 1}_2`).innerHTML = item.zone_label;
                        let status = "?";

                        switch (item.status) {
                            case 0:
                                status = "TROUBLE";
                                break;
                            case 1:
                                status = "NORMAL";
                                break;
                            case 2:
                                status = "ALARM";
                                break;
                            default:
                                status = "UNDEFINED";
                                break;
                        }
                        document.getElementById(`c_${i + 1}_3`).innerHTML = status;

                        let sensorType = "?";

                        switch (item.sensor_type) {
                            case 0:
                                sensorType = "NORMALLY OPEN";
                                break;
                            case 1:
                                sensorType = "NORMALLY CLOSED";
                                break;
                            default:
                                sensorType = "UNDEFINED";
                                break;
                        }
                        document.getElementById(`c_${i + 1}_4`).innerHTML = sensorType;

                        let definition = "?";

                        switch (item.definition) {
                            case 0:
                                definition = "NO USED";
                                break;
                            case 1:
                                definition = "DELAY";
                                break;
                            case 2:
                                definition = "INSTANT";
                                break;
                            case 3:
                                definition = "INTERIOR";
                                break;
                            case 4:
                                definition = "ALWAYS ARMED";
                                break;
                            case 5:
                                definition = "KEYSWITCH ARM";
                                break;
                            case 6:
                                definition = "MOMENTARY KEYSWITCH ARM";
                                break;
                            default:
                                definition = "UNDEFINED";
                                break;
                        }
                        document.getElementById(`c_${i + 1}_5`).innerHTML = definition;

                        let chime = false;

                        if (item.chime) {
                            document.getElementById(`c_${i + 1}_6`).innerHTML = "TRUE";
                        } else {
                            document.getElementById(`c_${i + 1}_6`).innerHTML = "FALSE";
                        }

                        let attr = "?";
                        switch (item.attributes) {
                            case 0:
                                attr = "AUDIBLE";
                                break;
                            case 1:
                                attr = "SILENT";
                                break;
                            case 2:
                                attr = "PULSED";
                                break;
                            case 3:
                                attr = "NONE";
                                break;
                            default:
                                attr = "UNDEFINED";
                                break;
                        }
                        document.getElementById(`c_${i + 1}_7`).innerHTML = attr;

                        if (item.memory_alarm) {
                            document.getElementById(`c_${i + 1}_8`).innerHTML = "TRUE";
                        } else {
                            document.getElementById(`c_${i + 1}_8`).innerHTML = "FALSE";
                        }

                    });
                }

            } catch (error) {
                console.error(error, event.data);
            }
        }
        function onLoad(event) {
            initWebSocket();
            initButton();
        }
        function initButton() {
            document.getElementById("ArmStatus").addEventListener("change", toggle);
        }
        function toggle() {
            websocket.send("toggle");
        }</script></body></html>
)rawliteral";

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
  // Route for root / web page
  HttpServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                { request->send_P(200, "text/html", index_html, processor); });

  // Start server
  HttpServer.begin();
}

void setup()
{
  pinMode(LED, OUTPUT);

  Serial.begin(115200);
  system_status.armed_status = SystemArmedStatus::UNDEFINED;
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
  // RFIDloop();
}