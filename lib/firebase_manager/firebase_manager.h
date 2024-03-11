#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Firebase_ESP_Client.h>
#include "file_system.h"

class FirebaseManager
{
public:
    FirebaseManager();
    void begin(FileSystem fileSystem);
    void sendNotification(String title, String message, FirebaseJson json, FileSystem fileSystem);
    void sendJson(String path, FirebaseJson json);
    bool ready();
    bool signupOk;

private:
    FirebaseData firebaseData;
    FirebaseConfig config;
    FirebaseAuth auth;
};

#endif // FIREBASE_MANAGER_H