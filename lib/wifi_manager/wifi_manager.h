#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include "file_manager.h"

class WifiManager
{
public:
    WifiManager();
    bool isConnected();
    void begin(FileManager fileManager);

private:
    bool initialized;
};

#endif // WIFI_MANAGER_H