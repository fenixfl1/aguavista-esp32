#include "async_web_server_manager.h"
#include <WiFi.h>

AsyncWebServerManager::AsyncWebServerManager(int http_port, int ws_port)
    : http_port(http_port),
      ws_port(ws_port),
      server(http_port),
      webSocket(http_port)
{
}

void AsyncWebServerManager::begin(FileManager fileManager)
{
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
              { this->onIndexRequest(request); });
    server.on("/config", HTTP_POST, [this, &fileManager](AsyncWebServerRequest *request)
              { this->onConfigRequest(request, fileManager); });
    server.on("/style.css", HTTP_GET, std::bind(&AsyncWebServerManager::onCSSRequest, this, std::placeholders::_1));
    server.onNotFound(std::bind(&AsyncWebServerManager::onPageNotFound, this, std::placeholders::_1));
    server.begin();
    webSocket.begin();
    webSocket.onEvent(std::bind(&AsyncWebServerManager::onWebSocketEvent, this, std::placeholders::_1,
                                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void AsyncWebServerManager::loop()
{
    webSocket.loop();
}

void AsyncWebServerManager::onWebSocketEvent(uint8_t client_num,
                                             WStype_t type,
                                             uint8_t *payload,
                                             size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.printf("[%u] Disconnected!\n", client_num);
        break;
    case WStype_CONNECTED:
    {
        IPAddress ip = webSocket.remoteIP(client_num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n",
                      client_num,
                      ip[0],
                      ip[1],
                      ip[2],
                      ip[3],
                      payload);
    }
    break;
    case WStype_TEXT:
        Serial.printf("[%u] get Text: %s\n", client_num, payload);
        break;
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
        Serial.printf("[%u] get binary length: %u\n", client_num, length);
        break;
    default:
        break;
    }
}

void AsyncWebServerManager::onIndexRequest(AsyncWebServerRequest *request)
{
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.print("[" + remote_ip.toString() +
                 "] HTTP GET request of " + request->url());
    request->send(SPIFFS, "/index.html", "text/html");
}

void AsyncWebServerManager::onCSSRequest(AsyncWebServerRequest *request)
{
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.print("[" + remote_ip.toString() +
                 "] HTTP GET request of " + request->url());
    request->send(SPIFFS, "/style.css", "text/css");
}

void AsyncWebServerManager::onPageNotFound(AsyncWebServerRequest *request)
{
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.print("[" + remote_ip.toString() +
                 "] HTTP GET request of " + request->url());
    request->send(404, "text/plain", "Not found");
}

void AsyncWebServerManager::onConfigRequest(AsyncWebServerRequest *request, FileManager fileManager)
{
    try
    {
        // Suponiendo que estás recibiendo los datos del formulario en los parámetros POST
        if (request->method() == HTTP_POST)
        {
            const String ssid = request->getParam("ssid", true)->value();
            const String password = request->getParam("pass", true)->value();
            const String user_id = request->getParam("user_id", true)->value();

            // Realiza aquí la validación de cada uno los datos
            if (ssid.length() == 0)
            {
                request->send(400, "text/plain", "falta: 'ssid'");
            }

            if (password.length() == 0)
            {
                request->send(400, "text/plain", "falta: 'pass'");
            }

            if (user_id.length() == 0)
            {
                request->send(400, "text/plain", "falta: 'user_id'");
            }

            // test the ssid and password with the wifi manager if it's correct then save it
            WiFi.begin(ssid.c_str(), password.c_str());

            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 15)
            {
                delay(1000);
                attempts++;
            }

            if (WiFi.status() == WL_CONNECTED)
            {
                // save the ssid and password to the file system
                fileManager.setConfig("EXTERNAL_WIFI_SSID", ssid.c_str());
                fileManager.setConfig("EXTERNAL_WIFI_PASS", password.c_str());
                fileManager.setConfig("USER_ID", user_id.c_str());
                request->send(200, "text/plain", "Configuración guardada");
            }
            else
            {
                request->send(400, "text/plain", "No se pudo conectar a la red");
            }

            if (request->hasParam("username", false))
            {
                request->send(400, "text/plain", "falta: 'username'");
            }
            if (request->hasParam("password", false))
            {
                request->send(400, "text/plain", "falta: 'password'");
            }

            else
            {
                // Credenciales inválidas, envía un mensaje de error en formato JSON
                request->send(401, "application/json", "{\"error\": \"Credenciales incorrectas\"}");
            }
        }
        else
        {
            // Si la solicitud no es POST
            request->send(405, "text/plain", "Método no permitido...");
        }
    }
    catch (const std::exception &e)
    {
        // send the error messaje as response
        request->send(400, "application/json", "{\"error\": \"Error inesperado. intente nuevamente.\"}");
        // Serial.print(e.what());
    }
}

void AsyncWebServerManager::onDashboardRequest(AsyncWebServerRequest *request)
{
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.print("[" + remote_ip.toString() +
                 "] HTTP GET request of " + request->url());
    request->send(SPIFFS, "/dashboard.html", "text/html");
}
