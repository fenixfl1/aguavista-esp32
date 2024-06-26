#include "file_system.h"

FileSystem::FileSystem()
{
}

void FileSystem::begin()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
}

void FileSystem::listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void FileSystem::createDir(fs::FS &fs, const char *path)
{
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path))
    {
        Serial.println("Dir created");
    }
    else
    {
        Serial.println("mkdir failed");
    }
}

void FileSystem::removeDir(fs::FS &fs, const char *path)
{
    Serial.printf("Removing Dir: %s\n", path);
    if (fs.rmdir(path))
    {
        Serial.println("Dir removed");
    }
    else
    {
        Serial.println("rmdir failed");
    }
}

void FileSystem::readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path, "r");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while (file.available())
    {
        Serial.write(file.read());
    }
    Serial.println();
    file.close();
}

void FileSystem::writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, "w");
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("File written");
    }
    else
    {
        Serial.println("Write failed");
    }
    file.close();
}

void FileSystem::appendFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, "a");
    if (!file)
    {
        Serial.println("Failed to open file for appending");
        return;
    }
    if (file.print(message))
    {
        Serial.println("Message appended");
    }
    else
    {
        Serial.println("Append failed");
    }
    file.close();
}

void FileSystem::renameFile(fs::FS &fs, const char *path1, const char *path2)
{
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2))
    {
        Serial.println("File renamed");
    }
    else
    {
        Serial.println("Rename failed");
    }
}

void FileSystem::deleteFile(fs::FS &fs, const char *path)
{
    Serial.printf("Deleting file: %s\n", path);
    if (fs.remove(path))
    {
        Serial.println("File deleted");
    }
    else
    {
        Serial.println("Delete failed");
    }
}

const char *FileSystem::getConfig(const char *key, const char *defaultValue)
{
    File file = SPIFFS.open("/config.json", "r");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return defaultValue;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        Serial.println("Failed to read file, using default configuration");
        return defaultValue;
    }

    const char *value = doc[key];

    if (value == NULL)
    {
        Serial.println("Key not found " + String(key) + " or value is not a string");
        return defaultValue;
    }

    return value;
}

const JsonArray FileSystem::getConfigArray(const char *key)
{
    try
    {
        File file = SPIFFS.open("/config.json", "r");
        if (!file)
        {
            Serial.println("Failed to open file for reading");
            return JsonArray();
        }

        JsonDocument doc; // Tamaño del documento depende de la longitud máxima de la cadena JSON
        DeserializationError error = deserializeJson(doc, file);
        if (error)
        {
            Serial.println("Failed to read file, using default configuration");
            return JsonArray();
        }

        if (doc.containsKey(key) && doc[key].is<JsonArray>())
        {
            // Verifica si la clave existe y si el valor es un array
            return doc[key].as<JsonArray>();
        }
        else
        {
            Serial.println("Key not found or value is not an array");
            return JsonArray();
        }
    }
    catch (const std::exception &e)
    {
        Serial.println(e.what());
    }
}

bool FileSystem::setConfig(const char *key, const char *value)
{
    File file = SPIFFS.open("/config.json", "r");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        Serial.println("Failed to read file, using default configuration");
        return false;
    }

    doc[key] = value;

    File file2 = SPIFFS.open("/config.json", "w");
    if (!file2)
    {
        Serial.println("Failed to open file for writing");
        return false;
    }

    if (serializeJson(doc, file2) == 0)
    {
        Serial.println(F("Failed to write to file"));
        return false;
    }

    file2.close();

    return true;
}

#include <ArduinoJson.h>

bool FileSystem::setConfigArray(const char *key, const char *value)
{
    try
    {
        // Abrir el archivo JSON en modo de lectura
        File file = SPIFFS.open("/config.json", "r+");
        if (!file)
        {
            Serial.println("Failed to open file for reading and writing");
            return false;
        }

        // Crear un documento JSON y cargar el contenido del archivo
        JsonDocument doc; // Ajusta el tamaño según el tamaño máximo de tu archivo JSON
        DeserializationError error = deserializeJson(doc, file);
        if (error)
        {
            Serial.println("Failed to parse file");
            file.close();
            return false;
        }

        // Verificar si la clave existe y si el valor es un array
        if (doc.containsKey(key) && doc[key].is<JsonArray>())
        {
            // Obtener el array
            JsonArray jsonArray = doc[key].as<JsonArray>();

            // Agregar el nuevo valor al array
            jsonArray.add(value);

            // Cerrar el archivo antes de escribir los datos actualizados
            file.close();

            // Reabrir el archivo en modo de escritura, que truncará automáticamente el archivo
            file = SPIFFS.open("/config.json", "w");
            if (!file)
            {
                Serial.println("Failed to reopen file for writing");
                return false;
            }

            // Serializar el documento JSON y escribir los datos actualizados en el archivo
            if (serializeJson(doc, file) == 0)
            {
                Serial.println("Failed to write to file");
                file.close();
                return false;
            }

            // Cerrar el archivo
            file.close();
            return true;
        }
        else
        {
            Serial.println("Key not found or value is not an array");
            file.close();
            return false;
        }
    }
    catch (const std::exception &e)
    {
        Serial.println(e.what());
        return false;
    }
}
void FileSystem::defaultConfig()
{
    JsonDocument doc;

    doc["APP_WIFI_SSID"] = "Aguavista";
    doc["APP_SERVER_PORT"] = "80";
    doc["APP_TOKEN"] = "";
    doc["APP_WIFI_GATEWAY"] = "192.168.4.1";
    doc["APP_WIFI_IP"] = "192.168.4.1";
    doc["APP_WIFI_MASK"] = "255.255.255.0";
    doc["APP_WIFI_PASS"] = "12345678";
    doc["CONFIG_PAGE_PASSWORD"] = "admin";
    doc["CONFIG_PAGE_USERNAME"] = "admin";
    doc["DEVICE_REGISTRATION_ID_TOKEN"] = "frXi8jszRsC4W2c-xzBFJ5:APA91bEynkUC5kE5u1Zzj4wKktPYxOfJltiCPs3gB5cLZJAJPVq4XcHwsVFF10wPZ2D9CFBefyvLpOu1VGmIr-H1uaJiz9IKT9Z8B99991urH8vI2NdUp80E1Njfx--ggEZCIlDYHvOU";
    doc["EXTERNAL_WIFI_PASS"] = "";
    doc["EXTERNAL_WIFI_SSID"] = "";
    doc["FIREBASE_API_KEY"] = "AIzaSyBrKc8hQ4wXaH6eKWj9Yblua2i8cOKmnZQ";
    doc["FIREBASE_AUTH_MAIL"] = "solojuegosfl119@gmail.com";
    doc["FIREBASE_AUTH_PASS"] = "adminfl119";
    doc["FIREBASE_DATABASE_URL"] = "https://aquavista-12cf5-default-rtdb.firebaseio.com/";
    doc["FIREBASE_FCM_SERVER_KEY"] = "key=AAAAw-1Vpac:APA91bG-ECTVuP3AZmiW7HM7X7lpbrtWi-AAjpQuCi_HHSYfPJC0ukda2g3kHBjDkhUHzRoQJR7vcNexmASW_Rsi-cA5B4Xx4C1TkPgJlHEX9sM-41qnyQlDdMDP7m3T0WeTNPwvvvEF";
    doc["FIREBASE_NOTIFICATION_URL"] = "https://fcm.googleapis.com/fcm/send";
    doc["FIREBASE_REGISTRATION_IDS"] = "";
    doc["LOCAL_SERVER_STATE"] = "ON";
    doc["NOTIFICATION_INTERVAL"] = "1";
    doc["LAST_DATA_SENT"] = "";
    doc["LAST_STATE"] = "";
    doc["LAST_TURBIDITY_STATE"] = "";
    doc["LAST_FLOW_STATE"] = "";
    doc["LAST_NOTIFICATION_TIME"] = "";
    doc["CONFIGURATION_DATE"] = "";

    File file = SPIFFS.open("/config.json", "w");
    if (!file)
    {
        Serial.printf("\nFailed to open file for writing\n");
        return;
    }

    if (serializeJson(doc, file) == 0)
    {
        Serial.printf("\nFailed to write to file\n");
    }
    else
    {
        Serial.printf("\nFile written\n");
    }

    file.close();
}