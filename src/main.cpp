#include <Arduino.h>
#include <WiFiClient.h>
#include "wifi_manager.h"
#include "firebase_manager.h"
#include "async_web_server_manager.h"

FileManager fileManager;
WifiManager wifiManager;
FirebaseManager firebaseManager;
AsyncWebServerManager httpServer(80, 1337);

WiFiClient client(80);

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  fileManager.begin();

  wifiManager.begin(fileManager);
  httpServer.begin(fileManager);

  if (wifiManager.isConnected())
  {
    firebaseManager.begin(fileManager);
  }
}

void loop()
{
  httpServer.loop();
}