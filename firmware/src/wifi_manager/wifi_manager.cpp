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

    Serial.printf("[WiFi] Conectando a %s...\n", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);

    return attemptConnection();
}

bool WifiManager::attemptConnection()
{
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40)
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

    Serial.println("\n[WiFi] ✗ Falló la conexión");
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