#include "wifi_manager.h"
#include <Arduino.h>

WifiManager::WifiManager()
{
    // Constructor
}

bool WifiManager::connect()
{
    if (WiFi.status() == WL_CONNECTED)
        return true;

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);

    // Intentar conectar a cada red en la lista
    for (int i = 0; i < wifiCredentialCount; i++)
    {
        const char *currentSSID = wifiCredentials[i].ssid;
        const char *currentPass = wifiCredentials[i].password;

        Serial.printf("\n[WiFi] 📶 Intentando red %d/%d: %s\n", i + 1, wifiCredentialCount, currentSSID);
        
        // Desconectar intento anterior si lo hubo
        WiFi.disconnect(); 
        delay(100);
        
        WiFi.begin(currentSSID, currentPass);

        if (attemptConnection())
        {
            Serial.printf("[WiFi] ✓ Conexión exitosa a %s\n", currentSSID);
            return true;
        }
        else
        {
            Serial.printf("[WiFi] ✗ Falló conexión a %s\n", currentSSID);
        }
    }

    Serial.println("\n[WiFi] ❌ Todas las redes fallaron");
    return false;
}

bool WifiManager::attemptConnection()
{
    int attempts = 0;
    // Reducir intentos a 20 (aprox 10s) para iterar más rápido entre redes
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        printConnectionInfo();
        return true;
    }

    return false;
}

void WifiManager::checkConnection()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[WiFi] ✗ Desconectado - Reconectando...");
        connect();
    }
}

bool WifiManager::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

int WifiManager::getRSSI()
{
    return WiFi.RSSI();
}

String WifiManager::getIP()
{
    return WiFi.localIP().toString();
}

void WifiManager::printConnectionInfo()
{
    Serial.printf("\n[WiFi] ✓ Conectado\n");
    Serial.printf("   IP: %s\n", getIP().c_str());
    Serial.printf("   RSSI: %d dBm\n", getRSSI());
}