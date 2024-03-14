#include <Arduino.h>
#include <LiquidCrystal_I2c.h>
#include <RTClib.h>
#include <Wire.h>
#include "file_system.h"
#include "wifi_manager.h"
#include "firebase_manager.h"
#include "async_web_server_manager.h"

#define TURBIDITY_SENSOR 34
#define FLOW_SENSOR 21

LiquidCrystal_I2C lcd(0x27, 16, 2);

RTC_DS3231 rtc;

byte statusLed = 13;
byte sensorInterrupt = 0; // 0 = digital pin 2
byte sensorPin = 21;
String getTubidityDate();

float calibrationFactor = 7.9;
volatile byte pulseCount;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

unsigned long oldTime;

void pulseCounter();
String getWaterFlowData();

FileSystem fileSystem;
WifiManager wifi;
FirebaseManager firebase;
AsyncWebServerManager httpServer(80, 1337);

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  fileSystem.begin();

  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH); // We have an active-low LED attached

  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  oldTime = 0;

  wifi.begin(fileSystem);
  httpServer.begin(fileSystem);

  if (wifi.isConnected())
  {
    firebase.begin(fileSystem);

    String token = fileSystem.getConfig("APP_TOKEN");

    if (token.length())
    {
      String tile = "Aquavista";
      String message = token;

      FirebaseJson json;

      json.add("status", "online");
      json.add("device_local_ip", WiFi.localIP().toString().c_str());

      httpServer.sendNotification(tile, message, fileSystem);
      firebase.sendJson("/datawater", json);

      fileSystem.setConfig("APP_TOKEN", "");
    }
  }
}

void loop()
{
  httpServer.loop();

  String token = "sfáodjfa";
  String title = "Aquavista";
  String message = "Hello, world!";

  String flowInfo = getWaterFlowData();
  String turbidity = getTubidityDate();

  // Serial.println(flowInfo);
  // Serial.println("\n");
  Serial.println(turbidity);

  /// getWaterFlowData();
  if (flowInfo != NULL)
  {
    // Crear un objeto FirebaseJson y asignarle el JsonDocument
    FirebaseJson fbJson;
    fbJson.setJsonData(flowInfo);

    // Enviar los datos a Firebase
    firebase.sendJson("/datawater", fbJson);
  }

  delay(1000);
}

String getWaterFlowData()
{
  try
  {
    if ((millis() - oldTime) > 1000) // Only process counters once per second
    {
      detachInterrupt(sensorInterrupt);

      flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;

      oldTime = millis();

      flowMilliLitres = (flowRate / 60) * 1000;

      // Add the millilitres passed in this second to the cumulative total
      totalMilliLitres += flowMilliLitres;

      unsigned int frac;

      // Reset the pulse counter so we can start incrementing again
      pulseCount = 0;

      // build a json object to send to the server
      StaticJsonDocument<200> doc;

      doc["flow_rate"] = flowRate;
      doc["flow_millilitres"] = flowMilliLitres;
      doc["total_millilitres"] = totalMilliLitres;
      doc["total_litres"] = totalMilliLitres / 1000;

      String jsonStr;

      String jsonString = "{\"flow_rate\": " + String(flowRate) + ",";
      jsonString += "\"flow_millilitres\": " + String(flowMilliLitres) + ",";
      jsonString += "\"total_millilitres\": " + String(totalMilliLitres) + ",";
      jsonString += "\"total_litres\": " + String(totalMilliLitres / 1000) + "}";

      serializeJson(doc, jsonStr);
      // Enable the interrupt again now that we've finished sending output
      attachInterrupt(sensorInterrupt, pulseCounter, FALLING);

      return jsonString;
    }

    return "";
  }
  catch (const std::exception &e)
  {
    Serial.println(e.what());
  }
}

String getTubidityDate()
{
  float volt;
  float ntu;

  lcd.begin(16, 2);
  lcd.backlight();

  volt = analogRead(TURBIDITY_SENSOR) * (5.0 / 1023.0);
  ntu = 133.42 * pow(volt, 3) - 255.86 * pow(volt, 2) + 857.39 * volt;

  String turbidity = String(ntu) + " NTU";

  lcd.setCursor(0, 0);

  lcd.print(turbidity);

  delay(5000);

  lcd.clear();

  return turbidity;
}

void pulseCounter()
{
  pulseCount++;
}