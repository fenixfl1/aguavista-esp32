#include <Arduino.h>
#include "file_system.h"
#include "wifi_manager.h"
#include "firebase_manager.h"
#include "async_web_server_manager.h"

void restarDevice();
JsonVariant readJsonFromArduino();

FileSystem fileSystem;
WifiManager wifi;
FirebaseManager firebase;
AsyncWebServerManager httpServer(80, 1337);

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  fileSystem.begin();

  wifi.begin(fileSystem);
  httpServer.begin(fileSystem);

  if (wifi.isConnected())
  {
    firebase.begin(fileSystem);

    if (fileSystem.getConfig("APP_TOKEN"))
    {
      String tile = "Aquavista";
      String message = fileSystem.getConfig("APP_TOKEN");
      FirebaseJson json;

      json.add("status", "online");
      json.add("device_local_ip", WiFi.localIP().toString());

      firebase.sendNotification(tile, message, json, fileSystem);
      firebase.sendJson("/device_status", json);

      fileSystem.setConfig("APP_TOKEN", "");
    }
  }

  httpServer.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request)
                { restarDevice(); });
}

void loop()
{
  httpServer.loop();
}

void restarDevice()
{
  Serial.println("\nRestarting device...\n");

  fileSystem.defaultConfig();

  ESP.restart();
}

// read data form arfduino trough UART protocol and return a json object
JsonVariant readJsonFromArduino()
{
  String data = "";
  while (Serial.available())
  {
    data += (char)Serial.read();
  }

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, data);

  return doc.as<JsonVariant>();
}