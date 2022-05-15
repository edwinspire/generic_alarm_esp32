#include <Arduino.h>
#include <string>
#include <ezOutput.h>
#include <WiFi.h>
//#include <WiFiClient.h>
//#include <WiFiAP.h>
//#include <SPI.h>
//#include <MFRC522.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "CtrlMaster.cpp"

// Set Wifi AP - these to your desired credentials.
const char *ssid = "edwinspire";
const char *password = "mypassword";

// WiFiServer server(80);

// Create AsyncWebServer object on port 80
AsyncWebServer HttpServer(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
  html {
    font-family: Arial, Helvetica, sans-serif;
    text-align: center;
  }
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2{
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
    overflow: hidden;
    background-color: #143642;
  }
  body {
    margin: 0;
  }
  .content {
    padding: 30px;
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }
  .button {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #0f8b8d;
    border: none;
    border-radius: 5px;
    -webkit-touch-callout: none;
    -webkit-user-select: none;
    -khtml-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;
    user-select: none;
    -webkit-tap-highlight-color: rgba(0,0,0,0);
   }
   /*.button:hover {background-color: #0f8b8d}*/
   .button:active {
     background-color: #0f8b8d;
     box-shadow: 2 2px #CDCDCD;
     transform: translateY(2px);
   }
   .state {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
   }
  </style>
<title>ESP Web Server</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <h1>ESP WebSocket Server</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>Output - GPIO 2</h2>
      <p class="state">state: <span id="state">%STATE%</span></p>
      <p><button id="button" class="button">Toggle</button></p>
    </div>
  </div>
<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    var state;
    if (event.data == "1"){
      state = "ON";
    }
    else{
      state = "OFF";
    }
    document.getElementById('state').innerHTML = state;
  }
  function onLoad(event) {
    initWebSocket();
    initButton();
  }
  function initButton() {
    document.getElementById('button').addEventListener('click', toggle);
  }
  function toggle(){
    websocket.send('toggle');
  }
</script>
</body>
</html>
)rawliteral";

void notifyClients()
{
  ws.textAll(String(system_status.armed_status));
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

      notifyClients();
    }
  }
}

// RFID section
//#define SS_PIN 5   // ESP32 pin GIOP5
//#define RST_PIN 27 // ESP32 pin GIOP27

// Create an instance of the MFRC522 class.
// MFRC522 rfid(SS_PIN, RST_PIN);
// byte managerKeyUID[4] = {0x7C, 0x2F, 0xD6, 0x21}; // Key
// byte secretaryKeyUID[4] = {0x30, 0x01, 0x8B, 0x15};

// String KeysUID[2] = {"7C2FD621"};

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
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

void WifiSetupAP()
{
  /*
    WiFiAccessPoint.ino creates a WiFi access point and provides a web server on it.

    Steps:
    1. Connect to the access point "yourAp"
    2. Point your web browser to http://192.168.4.1/H to turn the LED on or http://192.168.4.1/L to turn it off
       OR
       Run raw TCP "GET /H" and "GET /L" on PuTTY terminal with 192.168.4.1 as IP address and 80 as port
       */

  // You can remove the password parameter if you want the AP to be open.
  /*
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();
  */
}

void setup()
{
  pinMode(LED, OUTPUT);

  Serial.begin(115200);
  system_status.armed_status = SystemArmedStatus::UNDEFINED;

  // SPI.begin();     // init SPI bus
  // rfid.PCD_Init(); // init MFRC522

  //  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");

  // Serial.println("Configuring access point...");
  //  WifiSetupAP();
  WifiWebSocketServerSetup();
  // Serial.println("Server started");
}

void WifiLoop()
{
  /*
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
            client.print("Click <a href=\"/STATUS\">here</a> Status.<br>");

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

        if (currentLine.endsWith("GET /STATUS"))
        {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:application/json");
          client.println();

          // the content of the HTTP response follows the header:
          client.print("{\"status\":" + String(system_status.armed_status) + "}");

          // The HTTP response ends with another blank line:
          client.println();
          // break out of the while loop:
          break;
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
  */
}

/*
void RFIDloop()
{

  if (rfid.PICC_IsNewCardPresent())
  { // new tag is available
    if (rfid.PICC_ReadCardSerial())
    { // NUID has been readed
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      Serial.print("RFID/NFC Tag Type: ");
      Serial.println(rfid.PICC_GetTypeName(piccType));

      // print UID in Serial Monitor in the hex format
      Serial.print("UID:");

      // TODO: Esta sección debe ser revisada ya que no todas las tarjetas tienen 4 bytes de UID, pueden tener más
      String tag_id = String(rfid.uid.uidByte[0], HEX) + String(rfid.uid.uidByte[1], HEX) + String(rfid.uid.uidByte[2], HEX) + String(rfid.uid.uidByte[3], HEX);
      tag_id.toUpperCase();
      Serial.println(tag_id);

      int nz = sizeof(KeysUID) / sizeof(KeysUID);

      // Scan all zones to get their status
      for (int i = 0; i < nz; i++)
      {

        if (tag_id == KeysUID[i])
        {
          Serial.println((tag_id + F(" Key found.")));
          DisarmSystem();
          break;
        }
        else
        {
          Serial.println((tag_id + F(" Key NOT found.")));
        }
      }

      rfid.PICC_HaltA();      // halt PICC
      rfid.PCD_StopCrypto1(); // stop encryption on PCD
    }
  }
}
*/

void loop()
{
  MainLED.loop();
  EZ_OUT_01.loop();
  EZ_OUT_02.loop();

  SystemStatusLoop();
  WifiLoop();

  // RFIDloop();
}