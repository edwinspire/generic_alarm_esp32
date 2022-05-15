#include <Arduino.h>
#include <string>
#include <ezOutput.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <SPI.h>
#include <MFRC522.h>
#include "CtrlMaster.cpp"

//#define LED 2

// Wifi section
/*
  Steps:
  1. Connect to the access point "yourAp"
  2. Point your web browser to http://192.168.4.1/H to turn the LED on or http://192.168.4.1/L to turn it off
     OR
     Run raw TCP "GET /H" and "GET /L" on PuTTY terminal with 192.168.4.1 as IP address and 80 as port
     */
// Set these to your desired credentials.
const char *ssid = "ESP32-AP";
const char *password = "1234567890ABCDEF";

WiFiServer server(80);

// RFID section
#define SS_PIN 5   // ESP32 pin GIOP5
#define RST_PIN 27 // ESP32 pin GIOP27

MFRC522 rfid(SS_PIN, RST_PIN);

byte managerKeyUID[4] = {0x7C, 0x2F, 0xD6, 0x21}; // Key
byte secretaryKeyUID[4] = {0x30, 0x01, 0x8B, 0x15};

String KeysUID[2] = {"7C2FD621"};

/**/
const char CONFIG[] PROGMEM = R"(
  Some formatted text here !
  makes life easy for HTML
  servers, for example.
  )";

void setup()
{
  pinMode(LED, OUTPUT);

  Serial.begin(115200);
  system_status.armed_status = SystemArmedStatus::UNDEFINED;

  SPI.begin();     // init SPI bus
  rfid.PCD_Init(); // init MFRC522

  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");

  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");
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
}

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

void loop()
{
  MainLED.loop();
  EZ_OUT_01.loop();
  EZ_OUT_02.loop();

  SystemStatusLoop();
  WifiLoop();

  // RFIDloop();
}