#include "wifi_manager.h"

WifiManager::WifiManager()
{
    initialized = false;
}

bool WifiManager::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

void WifiManager::begin(FileManager fileManager)
{
    String ssid = fileManager.getConfig("EXTERNAL_WIFI_SSID"); // "SM-J727P1F1";
    String pass = fileManager.getConfig("EXTERNAL_WIFI_PASS"); // "12121212";

    if (ssid != NULL)
    {
        WiFi.begin(ssid.c_str(), pass.c_str());
        Serial.print("Conectando a WiFi");
        int attempts = 0;

        // si esta conectado a una red Wifi, apagar el modo AP
        if (WiFi.status() == WL_CONNECTED)
        {
            WiFi.softAPdisconnect(true);
        }

        // Esperar hasta que se conecte o se exceda el número máximo de intentos
        while (WiFi.status() != WL_CONNECTED && attempts < 10)
        {
            delay(1000);
            Serial.print(".");
            attempts++;
        }
    }
    else
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            int numStations = WiFi.softAPgetStationNum();
            String ssid = fileManager.getConfig("APP_WIFI_SSID");
            String pass = fileManager.getConfig("APP_WIFI_PASS");

            Serial.println("\n");
            Serial.println("No se pudo conectar a WiFi. Iniciando modo AP...");

            WiFi.disconnect();
            delay(100);

            // Configurar el ESP32 como un punto de acceso
            WiFi.softAP(ssid, pass);

            Serial.print("Punto de acceso iniciado");

            Serial.println("\n");
            Serial.println(ssid);
            Serial.println(WiFi.softAPIP());

            // Si hay estaciones conectadas al ESP32, mostrar sus direcciones IP
            if (numStations > 0)
            {
                Serial.println("Estaciones conectadas al ESP32:");
                for (int i = 0; i < numStations; i++)
                {
                    Serial.print("Estación: \n");
                    Serial.println(WiFi.softAPBroadcastIP() + i);
                }
            }
        }
        else
        {
            Serial.println("Conectado a WiFi");
            Serial.println(fileManager.getConfig("EXTERNAL_WIFI_SSID"));
        }
    }
}