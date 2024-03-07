#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_

#include <Firebase_ESP_Client.h>
#include "file_manager.h"

class FirebaseManager
{
public:
    FirebaseManager();
    void begin(FileManager fileManager);
    bool signupOk;

private:
    FirebaseData firebaseData;
    FirebaseConfig config;
    FirebaseAuth auth;
};

#endif // FIREBASE_MANAGER_H