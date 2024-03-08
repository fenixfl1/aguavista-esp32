#ifndef ASYNC_WEB_SERVER_MANAGER_H
#define ASYNC_WEB_SERVER_MANAGER_H

#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include "file_manager.h"

class AsyncWebServerManager
{
public:
    AsyncWebServerManager(int http_port, int ws_port);
    void begin(FileManager fileManager);
    void loop();
    void onWebSocketEvent(uint8_t client_num,
                          WStype_t type,
                          uint8_t *payload,
                          size_t length);
    void onIndexRequest(AsyncWebServerRequest *request);
    void onCSSRequest(AsyncWebServerRequest *request);
    void onPageNotFound(AsyncWebServerRequest *request);
    void onConfigRequest(AsyncWebServerRequest *request, FileManager fileManager);
    void onDashboardRequest(AsyncWebServerRequest *request);

private:
    int http_port;
    int ws_port;
    AsyncWebServer server;
    WebSocketsServer webSocket;
};

#endif // ASYNC_WEB_SERVER_MANAGER_H
