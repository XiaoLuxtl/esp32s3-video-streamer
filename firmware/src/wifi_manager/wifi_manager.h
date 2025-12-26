#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include "../configuration/secrets.h"

class WifiManager
{
public:
    WifiManager();

    bool connect();
    void checkConnection();
    bool isConnected();
    int getRSSI();
    String getIP();

private:
    Preferences preferences;
    DNSServer dnsServer;
    WebServer webServer;

    bool attemptConnection();
    void printConnectionInfo();
    
    // NVS Methods
    bool connectToSavedNetwork();
    void saveCredentials(String ssid, String password);
    
    // Captive Portal Methods
    void startCaptivePortal();
    void handleRoot();
    void handleSave();
};

#endif