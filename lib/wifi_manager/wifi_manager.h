#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include "file_system.h"

class WifiManager
{
public:
    WifiManager();
    void startSoftApp(FileSystem fileSystem);
    bool isConnected();
    void begin(FileSystem fileSystem);

private:
    bool initialized;
};

#endif // WIFI_MANAGER_H