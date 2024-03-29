#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

class FileSystem
{
public:
    FileSystem();
    void begin();
    void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
    void createDir(fs::FS &fs, const char *path);
    void removeDir(fs::FS &fs, const char *path);
    void readFile(fs::FS &fs, const char *path);
    void writeFile(fs::FS &fs, const char *path, const char *message);
    void appendFile(fs::FS &fs, const char *path, const char *message);
    void renameFile(fs::FS &fs, const char *path1, const char *path2);
    void deleteFile(fs::FS &fs, const char *path);
    bool setConfigArray(const char *key, const char *value);
    const char *getConfig(const char *key, const char *defaultValue = "");
    const JsonArray getConfigArray(const char *key);

    // setConfig is use to set a value to a key in the config.json file in the SPIFFS and return true if the operation is successful and false if it is not
    bool setConfig(const char *key, const char *value);
    void defaultConfig();
};

#endif // FILE_SYSTEM_H