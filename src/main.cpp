#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <time.h>
#include "file_system.h"
#include "wifi_manager.h"
#include "firebase_manager.h"
#include "async_web_server_manager.h"

#define NTP_SERVER "do.pool.ntp.org" // NTP server used to get the time from the internet

#define TURBIDITY_SENSOR 34
#define FLOW_SENSOR 21
#define CALIBRATION_FACTOR 7.9

volatile byte pulseCount;

// VARIABLE TO store the las time the notification was sent
time_t lastNotificationTime = 0;

FileSystem fileSystem;
WifiManager wifi;
FirebaseManager firebase;
AsyncWebServerManager httpServer(80, 1337);

void IRAM_ATTR pulseCounter()
{
  pulseCount++; // Incrementa el contador de pulsos cuando se recibe una interrupción
}

String getWaterFlowData()
{
  pulseCount = 0; // Reinicia el contador de pulsos

  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR), pulseCounter, FALLING); // Habilita la interrupción en el pin 21

  delay(1000); // Espera 1 segundo

  detachInterrupt(digitalPinToInterrupt(FLOW_SENSOR)); // Deshabilita la interrupción en el pin 21

  float flowRate = (pulseCount / CALIBRATION_FACTOR) * 60; // Calcula el flujo en litros por minuto

  // store the last flow rate and compare it with the new one to avoid sending the same value
  static float lastFlowRate = 0;
  if (flowRate == lastFlowRate)
  {
    return "";
  }

  lastFlowRate = flowRate;

  return String(flowRate);
}

float getTurbidityData()
{
  int sensorValue = analogRead(TURBIDITY_SENSOR);
  int digistal = digitalRead(TURBIDITY_SENSOR);

  float voltage = sensorValue * (5.0 / 1023.0);
  float turbidity = 133.33 * voltage - 33.33;

  Serial.print("\nSensor value: ");
  Serial.println(sensorValue);
  Serial.print("Voltage: ");
  Serial.println(voltage);
  Serial.print("Turbidity: ");
  Serial.println(turbidity);
  // convert to NTU
  turbidity = 400.0 * pow(abs(turbidity), -1.087);

  // store the last turbidity and compare it with the new one to avoid sending the same value
  // static float lastTurbidity = 0;

  // if (turbidity == lastTurbidity)
  // {
  //   return "";
  // }

  // lastTurbidity = turbidity;

  return turbidity;
}

// FUNCTION that get the tiempo from the NTP server and return a string with the time
String getFormattedTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return "";
  }
  char timeStringBuff[50]; // 50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(timeStringBuff);
}

// FUNCTION that send a notification to the app every x minutes (defned if fileSystem.getConfit(NOTIFICATION_INTERVAL) to keep the user updated with the water quality and flow rate
void notificationTask()
{
  try
  {
    time_t now = time(nullptr);
    int interval = atoi(fileSystem.getConfig("NOTIFICATION_INTERVAL", "5"));

    if (now - lastNotificationTime > interval * 60)
    {
      String flow_rate = getWaterFlowData();
      float turbidity_value = getTurbidityData();

      Serial.println(turbidity_value);

      String turbidity = String(turbidity_value);

      if (wifi.isConnected() && getFormattedTime().length() > 0)
      {
        httpServer.sendNotification("Aquavista", "La calidad del agua ha cambiado", fileSystem);
      }

      lastNotificationTime = now;
    }
  }
  catch (const std::exception &e)
  {
    Serial.println(e.what());
  }
}

void setup()
{
  try
  {
    Serial.begin(115200);
    fileSystem.begin();

    configTime(0, 0, NTP_SERVER);

    wifi.begin(fileSystem);

    httpServer.begin(fileSystem);

    if (wifi.isConnected())
    {
      firebase.begin(fileSystem);

      String token = fileSystem.getConfig("APP_TOKEN");

      Serial.println("Token: " + token);

      if (token.length())
      {

        String tile = "Aquavista";
        String message = "Su código de confirmación es: " + token;

        FirebaseJson json;

        json.add("status", "online");
        json.add("device_local_ip", WiFi.localIP().toString().c_str());

        httpServer.sendNotification(tile, message, fileSystem);
        firebase.sendJson("/device_status", json);

        fileSystem.setConfig("APP_TOKEN", "");
      }
    }
  }
  catch (const std::exception &e)
  {
    Serial.println(e.what());
  }
}

void loop()
{
  try
  {
    httpServer.loop();

    String flow_rate = getWaterFlowData();
    float turbidity_ = getTurbidityData();

    Serial.println(turbidity_);

    String turbidity = String(turbidity_);

    if (wifi.isConnected() && getFormattedTime().length() > 0)
    {
      FirebaseJson fbJson;

      fbJson.set("flow", flow_rate);
      fbJson.set("turbidity", turbidity);
      fbJson.set("fecha", getFormattedTime());
      fbJson.set("id", fileSystem.getConfig("FIREBASE_REGISTRATION_IDS"));

      firebase.sendJson("/datawater", fbJson);
    }

    Serial.println("Flow rate: " + flow_rate + " L/min");
    Serial.println("Turbidity: " + turbidity + " NTU");

    notificationTask();

    delay(5000);
  }
  catch (const std::exception &e)
  {
    Serial.println(e.what());
    Serial.println("Error en el loop principal");
  }
}
