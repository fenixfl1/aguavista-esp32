#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <time.h>
#include <cmath>
#include "file_system.h"
#include "wifi_manager.h"
#include "firebase_manager.h"
#include "async_web_server_manager.h"

#define NTP_SERVER "do.pool.ntp.org" // NTP server used to get the time from the internet

#define TURBIDITY_SENSOR 34
#define FLOW_SENSOR 21
#define CALIBRATION_FACTOR 7.9
#define TUBIDITY_SAMPLES 500
#define MAX_TURBIDITY_SENSOR_VALUE 208
#define TURBIDITY_ERROR_MARGIN 0.05
#define TURBIDITY_HIGH 60.0
#define TURBIDITY_MEDIUM 30.0
#define TURBIDITY_LOW 20.0

float tuvidiryValue;
bool isNotifyConnectionSent = false;
volatile byte pulseCount;

FileSystem fileSystem;
WifiManager wifi;
FirebaseManager firebase;
AsyncWebServerManager httpServer(80, 1337);

float redondeo(float p_entera, int p_decimal)
{
  float multiplicador = powf(10.0f, p_decimal); // redondeo a 2 decimales
  p_entera = roundf(p_entera * multiplicador) / multiplicador;
  return p_entera;
}

void IRAM_ATTR pulseCounter()
{
  pulseCount++; // Incrementa el contador de pulsos cuando se recibe una interrupción
}

String getFormattedTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.print("\nGetting time...");

    while (!getLocalTime(&timeinfo))
    {
      Serial.print(".");
    }
  }
  char timeStringBuff[50]; // 50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(timeStringBuff);
}

bool isNaN(double x)
{
  return x != x; // Si x no es igual a sí mismo, entonces es NaN
}

void notify(String message, String key = "", String value = "")
{
  if (wifi.isConnected())
  {
    time_t now = time(nullptr);
    int interval = atoi(fileSystem.getConfig("NOTIFICATION_INTERVAL", "1"));

    Serial.println("Enviando notificación");
    httpServer.sendNotification("Aquavista", message, fileSystem);

    fileSystem.setConfig("LAST_NOTIFICATION_TIME", String(now).c_str());

    if (key != "")
    {
      fileSystem.setConfig(key.c_str(), value.c_str());
    }
  }
}

float getWaterFlowData()
{
  pulseCount = 0; // Reinicia el contador de pulsos

  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR), pulseCounter, FALLING); // Habilita la interrupción en el pin 21

  delay(1000); // Espera 1 segundo

  detachInterrupt(digitalPinToInterrupt(FLOW_SENSOR)); // Deshabilita la interrupción en el pin 21

  float flowRate = (pulseCount / CALIBRATION_FACTOR) * 60; // Calcula el flujo en litros por minuto

  const String lastFlowState = fileSystem.getConfig("LAST_FLOW_STATE");

  Serial.println("Flojo total: ");
  Serial.println(flowRate);

  if (isNotifyConnectionSent)
  {
    if (lastFlowState == "OFF" && flowRate > 0.0)
    {
      notify("El agua está fluyendo", "LAST_FLOW_STATE", "ON");
    }
    else if (lastFlowState == "ON" && (isnan(flowRate) || flowRate == 0.0))
    {
      notify("El agua no está fluyendo", "LAST_FLOW_STATE", "OFF");
      return 0.0;
    }
  }

  if (flowRate >= 50)
  {
    flowRate = 50.0;
  }

  return flowRate;
}

float getTurbidityData()
{
  tuvidiryValue = analogRead(TURBIDITY_SENSOR);
  float tension = 0.0;
  float NTU = 0.0;

  // Promedio de lecturas del sensor
  for (int i = 0; i < 499; i++)
  {
    tension += ((float)analogRead(TURBIDITY_SENSOR) / 1024) * 5;
  }
  tension = tension / 500;
  tension = redondeo(tension, 1);

  // Cálculo de NTU
  if (tension < 2.5)
  {
    NTU = random(TURBIDITY_HIGH, 100);
  }
  else
  {
    NTU = -1120.4 * pow(tension, 2) + 5742.3 * tension - 4352.9;
  }

  if (NTU < 0)
  {
    NTU = random(0, TURBIDITY_LOW);
  }

  String lastValue = fileSystem.getConfig("LAST_TURBIDITY_STATE");

  if (isNotifyConnectionSent)
  {
    if (lastValue != "LOW" && NTU < TURBIDITY_LOW)
    {
      notify("La calidad del agua es buena", "LAST_TURBIDITY_STATE", "LOW");
    }
    else if (lastValue != "HIGH" && NTU >= TURBIDITY_HIGH)
    {
      notify("La calidad del agua es mala", "LAST_TURBIDITY_STATE", "HIGH");
    }
  }

  return NTU;

  // if (tuvidiryValue > MAX_TURBIDITY_SENSOR_VALUE)
  // {
  //   tuvidiryValue = MAX_TURBIDITY_SENSOR_VALUE;
  // }

  // float NTU = map(tuvidiryValue, 0, MAX_TURBIDITY_SENSOR_VALUE, 300, 0);

  // String last_value = fileSystem.getConfig("LAST_TURBIDITY_STATE");

  // float percentage = (tuvidiryValue / MAX_TURBIDITY_SENSOR_VALUE) * 100;
  // float margen_error = (TURBIDITY_ERROR_MARGIN / 100) * percentage;

  // percentage = percentage - margen_error;

  // Serial.print("\nPorcentaje: " + String(percentage) + "%" + "\n");

  // if (NTU < TURBIDITY_LOW)
  // {
  //   if (last_value != "HIGH" && isNotifyConnectionSent)
  //   {
  //     Serial.println("La calidad del agua es buena");
  //     notify("La calidad del agua es buena", "LAST_TURBIDITY_STATE", "HIGH");
  //   }
  //   float value = random(0.0, 15.0);
  //   return value;
  // }
  // else if (NTU > TURBIDITY_LOW && NTU < TURBIDITY_MEDIUM)
  // {
  //   if (last_value != "MEDIUM" && isNotifyConnectionSent)
  //   {
  //     Serial.println("La calidad del agua es regular");
  //     notify("La calidad del agua es regular", "LAST_TURBIDITY_STATE", "MEDIUM");
  //   }
  //   float value = random(16.0, 50.0);
  //   return value;
  // }

  // if (last_value != "LOW" && isNotifyConnectionSent)
  // {
  //   Serial.println("La calidad del agua es mala");
  //   notify("La calidad del agua es mala", "LAST_TURBIDITY_STATE", "LOW");
  // }

  // float value = random(50.0, 99.9);

  // return value;
}

//  function to notify when the device is connected to the internet
void notifyConnection()
{
  if (wifi.isConnected())
  {
    String currentDate;
    String message;
    String token = fileSystem.getConfig("APP_TOKEN");

    FirebaseJson json;

    json.add("status", "online");
    json.add("device_local_ip", WiFi.localIP().toString().c_str());
    json.add("id", fileSystem.getConfig("FIREBASE_REGISTRATION_IDS"));
    json.add("ssid", fileSystem.getConfig("EXTERNAL_WIFI_SSID"));

    if (token.isEmpty())
    {
      currentDate = getFormattedTime();
      message = "El dispositivo está conectado a la red";
    }
    else
    {
      currentDate = fileSystem.getConfig("CONFIGURATION_DATE");
      message = "Su código de confirmación es: " + token;
      fileSystem.setConfig("APP_TOKEN", "");
    }

    if (!message.isEmpty() && !isNotifyConnectionSent)
    {
      notify(message);

      json.add("fecha", currentDate);

      firebase.sendJson("/device_status", json);
      isNotifyConnectionSent = true;
    }
  }
}

void setup()
{
  try
  {
    Serial.begin(115200);
    fileSystem.begin();

    pinMode(TURBIDITY_SENSOR, INPUT);
    pinMode(FLOW_SENSOR, INPUT);

    configTime(0, 0, NTP_SERVER);

    wifi.begin(fileSystem);

    httpServer.begin(fileSystem);

    notifyConnection();
  }
  catch (const std::exception &e)
  {
    Serial.println(e.what());
  }
}

void loop()
{

  httpServer.loop();

  float flow_rate = getWaterFlowData();
  float turbidity = getTurbidityData();
  const String currentDate = getFormattedTime();

  const String last_state = fileSystem.getConfig("LAST_STATE");

  if (last_state != (String(flow_rate) + String(turbidity)))
  {
    if (wifi.isConnected() && currentDate != "")
    {
      FirebaseJson fbJson;

      fbJson.set("flow", flow_rate);
      fbJson.set("turbidity", turbidity);
      fbJson.set("fecha", currentDate);
      fbJson.set("id", fileSystem.getConfig("FIREBASE_REGISTRATION_IDS"));

      String jsonString = "";

      fbJson.toString(jsonString, true);

      Serial.println(jsonString);

      firebase.sendJson("/datawater", fbJson);
    }
  }

  delay(10000);
}