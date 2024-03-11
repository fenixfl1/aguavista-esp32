#include "firebase_manager.h"

#include "addons/TokenHelper.h"

unsigned long sendDataPreviousMillis = 0;

FirebaseManager::FirebaseManager()
{
    signupOk = false;
}

void FirebaseManager::begin(FileSystem fileSystem)
{
    String api_key = fileSystem.getConfig("FIREBASE_API_KEY");
    String database_url = fileSystem.getConfig("FIREBASE_DATABASE_URL");

    String email = fileSystem.getConfig(" FIREBASE_AUTH_MAIL ");
    String password = fileSystem.getConfig("FIREBASE_AUTH_PASS");

    Serial.printf("API_KEY: %s\n", api_key.c_str());
    Serial.printf("DATABASE_URL: %s\n", database_url.c_str());

    config.api_key = api_key;
    config.database_url = database_url;

    auth.user.email = email;
    auth.user.password = password;

    if (Firebase.signUp(&config, &auth, email, password))
    {
        Serial.println("Sign up succeeded");
        signupOk = true;
    }
    else
    {
        Serial.printf("\n");
        Serial.println(config.signer.signupError.message.c_str());
        signupOk = false;
    }

    config.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

void FirebaseManager::sendNotification(String title, String message, FirebaseJson json, FileSystem fileSystem)
{
    try
    {
        if (!(Firebase.ready() && signupOk && (millis() - sendDataPreviousMillis > 5000) || sendDataPreviousMillis == 0))
        {
            throw std::runtime_error(firebaseData.errorReason().c_str());
        }

        Firebase.FCM.setServerKey(fileSystem.getConfig("FIREBASE_FCM_SERVER_KEY"));

        FCM_HTTPv1_JSON_Message msg;

        msg.token = fileSystem.getConfig("DEVICE_REGISTRATION_ID_TOKEN");

        msg.notification.title = title;
        msg.notification.body = message;

        String payload;

        json.toString(payload);
        msg.data = payload.c_str();

        if (Firebase.FCM.send(&firebaseData, &msg))
        {
            Serial.println("Message sent to FCM backend.");
            Serial.println(Firebase.FCM.payload(&firebaseData));
        }
        else
        {
            throw std::runtime_error(firebaseData.errorReason().c_str());
        }
    }
    catch (const std::exception &e)
    {
        Serial.println(e.what());
    }
}

void FirebaseManager::sendJson(String path, FirebaseJson json)
{
    try
    {
        if (Firebase.ready() && signupOk && (millis() - sendDataPreviousMillis > 5000) || sendDataPreviousMillis == 0)
        {
            sendDataPreviousMillis = millis();

            if (Firebase.RTDB.setJSON(&firebaseData, path, &json))
            {
                Serial.printf("\nJSON data sent to ");
                Serial.println(path);
            }
        }
        else
        {
            throw std::runtime_error(firebaseData.errorReason().c_str());
        }
    }
    catch (std::runtime_error &e)
    {
        Serial.println("Signup failed");
    }
}

bool FirebaseManager::ready()
{
    return Firebase.ready();
}