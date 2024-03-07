#include "http_server_manager.h"

HttpServerManager::HttpServerManager(int http_port, int ws_port)
    : http_port(http_port),
      ws_port(ws_port),
      server(http_port),
      webSocket(http_port)
{
}

void HttpServerManager::begin()
{
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
              { this->onIndexRequest(request); });
    server.on("/login", HTTP_POST, [this](AsyncWebServerRequest *request)
              { this->onLoginRequest(request); });
    server.on("/style.css", HTTP_GET, std::bind(&HttpServerManager::onCSSRequest, this, std::placeholders::_1));
    server.on("/dashboard", HTTP_POST, std::bind(&HttpServerManager::onDashboardRequest, this, std::placeholders::_1));
    server.onNotFound(std::bind(&HttpServerManager::onPageNotFound, this, std::placeholders::_1));
    server.begin();
    webSocket.begin();
    webSocket.onEvent(std::bind(&HttpServerManager::onWebSocketEvent, this, std::placeholders::_1,
                                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void HttpServerManager::loop()
{
    webSocket.loop();
}

void HttpServerManager::onWebSocketEvent(uint8_t client_num,
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

void HttpServerManager::onIndexRequest(AsyncWebServerRequest *request)
{
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.println("[" + remote_ip.toString() +
                   "] HTTP GET request of " + request->url());
    request->send(SPIFFS, "/index.html", "text/html");
}

void HttpServerManager::onCSSRequest(AsyncWebServerRequest *request)
{
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.println("[" + remote_ip.toString() +
                   "] HTTP GET request of " + request->url());
    request->send(SPIFFS, "/style.css", "text/css");
}

void HttpServerManager::onPageNotFound(AsyncWebServerRequest *request)
{
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.println("[" + remote_ip.toString() +
                   "] HTTP GET request of " + request->url());
    request->send(404, "text/plain", "Not found");
}

void HttpServerManager::onLoginRequest(AsyncWebServerRequest *request)
{
    // Suponiendo que estás recibiendo los datos del formulario en los parámetros POST
    if (request->method() == HTTP_POST)
    {
        try
        {
            if (request->hasParam("username", false))
            {
                request->send(400, "text/plain", "falta: 'username'");
            }
            if (request->hasParam("password", false))
            {
                request->send(400, "text/plain", "falta: 'password'");
            }

            String username = request->getParam("username", true)->value();
            String password = request->getParam("password", true)->value();

            // Realiza aquí la validación de las credenciales
            if (validarCredenciales(username, password))
            {
                // Credenciales válidas, envía una respuesta JSON de éxito
                request->send(200, "application/json", "{\"success\": true}");
            }
            else
            {
                // Credenciales inválidas, envía un mensaje de error en formato JSON
                request->send(401, "application/json", "{\"error\": \"Credenciales incorrectas\"}");
            }
        }
        catch (const std::exception &e)
        {
            // send the error messaje as response
            request->send(400, "application/json", "{\"error\": \"Error inesperado. intente nuevamente.\"}");
            Serial.println(e.what());
        }
    }
    else
    {
        // Si la solicitud no es POST
        request->send(405, "text/plain", "Método no permitido...");
    }
}

bool HttpServerManager::validarCredenciales(const String &username, const String &password)
{
    String valid_username = "admin";
    String valid_password = "admin";

    if (username != valid_username || password != valid_password)
    {
        return false;
    }

    return true;
}

void HttpServerManager::onDashboardRequest(AsyncWebServerRequest *request)
{
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.println("[" + remote_ip.toString() +
                   "] HTTP GET request of " + request->url());
    request->send(SPIFFS, "/dashboard.html", "text/html");
}

void HttpServerManager::onConfigRequest(AsyncWebServerRequest *request) 
{
    if (request->method() == HTTP_POST)
    {

    }
    else 
    {
        request->send(405, "plain/text", "Method not allowed.");
    }

}