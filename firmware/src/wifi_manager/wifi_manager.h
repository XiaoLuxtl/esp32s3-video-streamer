#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
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
    bool attemptConnection();
    void printConnectionInfo();
};

#endif