#ifndef HTTP_SERVER_MANAGER_H
#define HTTP_SERVER_MANAGER_H

#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>

class HttpServerManager
{
public:
    HttpServerManager(int http_port, int ws_port);
    void begin();
    void loop();
    void onWebSocketEvent(uint8_t client_num,
                          WStype_t type,
                          uint8_t *payload,
                          size_t length);
    void onIndexRequest(AsyncWebServerRequest *request);
    void onCSSRequest(AsyncWebServerRequest *request);
    void onPageNotFound(AsyncWebServerRequest *request);
    void onLoginRequest(AsyncWebServerRequest *request);
    void onDashboardRequest(AsyncWebServerRequest *request);
    void onConfigRequest(AsyncWebServerRequest *request);

private:
    int http_port;
    int ws_port;
    AsyncWebServer server;
    WebSocketsServer webSocket;
    bool validarCredenciales(const String &username, const String &password);
};

#endif // HTTP_SERVER_MANAGER_H
