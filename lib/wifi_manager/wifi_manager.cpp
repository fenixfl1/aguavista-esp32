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

    const String ssid = fileManager.getConfig("EXTERNAL_WIFI_SSID"); // "SM-J727P1F1";
    const String pass = fileManager.getConfig("EXTERNAL_WIFI_PASS"); // "12121212";

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

        int numStations = WiFi.softAPgetStationNum();
        const String ssid = fileManager.getConfig("APP_WIFI_SSID");
        const String pass = fileManager.getConfig("APP_WIFI_PASS");

        WiFi.disconnect();
        delay(100);

        // Configurar el ESP32 como un punto de acceso
        WiFi.softAP(ssid, pass);

        Serial.print("\nPunto de acceso iniciado\n");
    }
}