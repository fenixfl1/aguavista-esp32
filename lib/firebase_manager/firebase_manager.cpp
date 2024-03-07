#include "firebase_manager.h"

FirebaseManager::FirebaseManager()
{
    signupOk = false;
}

void FirebaseManager::begin(FileManager fileManager)
{

    config.api_key = *fileManager.getConfig("FIREBASE_AUTH");
    config.database_url = *fileManager.getConfig("FIREBASE_HOST");

    if (Firebase.signUp(&config, &auth, "", ""))
    {
        Serial.println("Sign up succeeded");
        signupOk = true;
    }
    else
    {
        Serial.println(config.signer.signupError.message.c_str());
        signupOk = false;
    }

    // config.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}
