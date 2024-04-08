#include "async_web_server_manager.h"
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

AsyncWebServerManager::AsyncWebServerManager(int http_port, int ws_port)
    : http_port(http_port),
      ws_port(ws_port),
      server(http_port),
      webSocket(http_port),
      httpClient()
{
}

void AsyncWebServerManager::begin(FileSystem fileSystem)
{
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    // server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
    //           { this->onIndexRequest(request); });
    server.on("/", HTTP_GET, std::bind(&AsyncWebServerManager::onIndexRequest, this, std::placeholders::_1));
    server.on("/style.css", HTTP_GET, std::bind(&AsyncWebServerManager::onCSSRequest, this, std::placeholders::_1));
    server.on("/reset", HTTP_GET, std::bind(&AsyncWebServerManager::onResetRequest, this, std::placeholders::_1, fileSystem));
    server.onNotFound(std::bind(&AsyncWebServerManager::onPageNotFound, this, std::placeholders::_1));

    // Config endpoint. This endpoint is used to configure the device, it use the async callback handler to manage the json data in the request
    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/config",
                                                                           std::bind(&AsyncWebServerManager::onConfigRequest, this, std::placeholders::_1, std::placeholders::_2, fileSystem));
    server.addHandler(handler);

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
                 "] HTTP GET request of " + "\n" + request->url());

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

void AsyncWebServerManager::onConfigRequest(AsyncWebServerRequest *request, JsonVariant &json, FileSystem fileSystem)
{
    Serial.println("\nRequest from: " + request->client()->remoteIP().toString() + " to " + request->url() + "\n");
    try
    {
        if (!json.is<JsonObject>())
        {
            request->send(400, "text/plain", "El cuerpo de la solicitud no es un objeto JSON válido");
            return;
        }

        // Obtener los datos del JSON
        JsonObject jsonObj = json.as<JsonObject>();

        // mostrar el json en consola
        serializeJsonPretty(jsonObj, Serial);

        if (!jsonObj.containsKey("ssid") || !jsonObj.containsKey("pass") || !jsonObj.containsKey("user_id"))
        {
            request->send(400, "text/plain", "El objeto JSON debe contener las claves 'ssid', 'pass' y 'user_id'");
            return;
        }

        const String ssid = jsonObj["ssid"].as<String>();
        const String password = jsonObj["pass"].as<String>();
        const String user_id = jsonObj["user_id"].as<String>();
        const String token = jsonObj["token"].as<String>();
        const String notif = jsonObj["notif"].as<String>();
        const String date = jsonObj["fecha"].as<String>();

        // Realizar validaciones de los datos
        if (ssid.isEmpty() || password.isEmpty() || user_id.isEmpty())
        {
            request->send(400, "text/plain", "Los campos 'ssid', 'pass' y 'user_id' no pueden estar vacíos");
            return;
        }

        // Guardar los datos en el sistema de archivos
        bool ssidSaved = fileSystem.setConfig("EXTERNAL_WIFI_SSID", ssid.c_str());
        bool passSaved = fileSystem.setConfig("EXTERNAL_WIFI_PASS", password.c_str());
        bool savedNotif = fileSystem.setConfig("DEVICE_REGISTRATION_ID_TOKEN", notif.c_str());
        bool userIdSaved = fileSystem.setConfig("FIREBASE_REGISTRATION_IDS", user_id.c_str());
        bool appToken = fileSystem.setConfig("APP_TOKEN", token.c_str());
        bool dateSaved = fileSystem.setConfig("CONFIGURATION_DATE", date.c_str());

        if (!ssidSaved || !passSaved || !userIdSaved || !appToken || !savedNotif || !dateSaved)
        {
            request->send(500, "text/plain", "Error al guardar la configuración");
            return;
        }

        request->send(200, "text/plain", "Configuración guardada");
        // Reiniciar el dispositivo

        delay(2000);
        ESP.restart();
    }
    catch (const std::exception &e)
    {
        // Si hay un error inesperado
        request->send(500, "text/plain", "Error interno del servidor");
        Serial.print(e.what());
    }
}

void AsyncWebServerManager::on(const char *uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest)
{
    try
    {
        server.on(uri, method, onRequest);
    }
    catch (const std::exception &e)
    {
        Serial.print(e.what());
    }
}

void AsyncWebServerManager::onResetRequest(AsyncWebServerRequest *request, FileSystem fileSystem)
{
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.print("[" + remote_ip.toString() +
                 "] HTTP GET request of " + request->url());

    if (resetDevice(fileSystem) == false)
    {
        request->send(500, "text/plain", "Error al reiniciar el dispositivo");
        return;
    }

    request->send(200, "text/plain", "Dispositivo restablecido a la configuración predeterminada.");
}

bool AsyncWebServerManager::resetDevice(FileSystem fileSystem)
{
    try
    {
        Serial.println("\nRestarting device...\n");

        fileSystem.defaultConfig();

        ESP.restart();
        return true;
    }
    catch (const std::exception &e)
    {
        Serial.print(e.what());
        return false;
    }
}

void AsyncWebServerManager::sendNotification(String title, String message, FileSystem fileSystem)
{
    // send a post request to the server using HTTPClient.h
    try
    {
        String fcm_api_key = fileSystem.getConfig("FIREBASE_FCM_SERVER_KEY");
        String fcm_url = fileSystem.getConfig("FIREBASE_NOTIFICATION_URL");
        String toke = fileSystem.getConfig("DEVICE_REGISTRATION_ID_TOKEN");

        httpClient.begin(fcm_url);

        httpClient.addHeader("Authorization", fcm_api_key);
        httpClient.addHeader("Content-Type", "application/json");

        JsonDocument doc;

        FirebaseJson json;

        String data = "{\"registration_ids\": [\"" + toke + "\"], \"notification\": {\"body\":\"" + message + "\", \"title\":\"" + title + "\"}}";

        int httpCode = httpClient.POST(data);

        if (httpCode > 0)
        {
            String response = httpClient.getString();
        }
        else
        {
            Serial.println("Error on HTTP request");
        }

        httpClient.end();
    }
    catch (const std::exception &e)
    {
        Serial.print("Error on HTTP request:");
        Serial.println(e.what());
    }
}
