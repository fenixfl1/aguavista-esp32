#ifndef ASYNC_WEB_SERVER_MANAGER_H
#define ASYNC_WEB_SERVER_MANAGER_H

#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <HTTPClient.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "file_system.h"

class AsyncWebServerManager
{
public:
    AsyncWebServerManager(int http_port, int ws_port);
    void sendNotification(String title, String message, FileSystem fileSystem);
    void begin(FileSystem fileSystem);
    void loop();
    void on(const char *uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest);

private:
    int http_port;
    int ws_port;
    AsyncWebServer server;
    WebSocketsServer webSocket;
    HTTPClient httpClient;

    void onIndexRequest(AsyncWebServerRequest *request);
    void onCSSRequest(AsyncWebServerRequest *request);
    void onPageNotFound(AsyncWebServerRequest *request);
    void onConfigRequest(AsyncWebServerRequest *request, JsonVariant &json, FileSystem fileSystem);
    bool resetDevice(FileSystem fileSystem);
    void onResetRequest(AsyncWebServerRequest *request, FileSystem fileSystem);
    void onWebSocketEvent(uint8_t client_num,
                          WStype_t type,
                          uint8_t *payload,
                          size_t length);
};

#endif // ASYNC_WEB_SERVER_MANAGER_H
