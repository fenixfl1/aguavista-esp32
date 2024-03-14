#include "wifi_manager.h"

WifiManager::WifiManager()
{
    initialized = false;
}

bool WifiManager::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

void WifiManager::begin(FileSystem fileSystem)
{

    const String ssid = fileSystem.getConfig("EXTERNAL_WIFI_SSID"); // "SM-J727P1F1";
    const String pass = fileSystem.getConfig("EXTERNAL_WIFI_PASS"); // "12121212";

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

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.print("\Local ip: ");
            Serial.println(WiFi.localIP());
        }
        else
        {
            startSoftApp(fileSystem);
        }
    }
    else
    {
        startSoftApp(fileSystem);
    }
}

void WifiManager::startSoftApp(FileSystem fileSystem)
{
    try
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            WiFi.disconnect(true, true);
        }

        int numStations = WiFi.softAPgetStationNum();
        const String ssid = fileSystem.getConfig("APP_WIFI_SSID");
        const String pass = fileSystem.getConfig("APP_WIFI_PASS");

        WiFi.disconnect();
        delay(100);

        // Configurar el ESP32 como un punto de acceso
        WiFi.softAP(ssid, pass);

        Serial.print("\nPunto de acceso iniciado\n");
    }
    catch (const std::exception &e)
    {
        Serial.printf("\n");
        Serial.printf(e.what());
    }
}