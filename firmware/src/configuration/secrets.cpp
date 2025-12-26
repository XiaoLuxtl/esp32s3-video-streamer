#include "secrets.h"

// === DEFINICIONES REALES ===
const WifiCredential wifiCredentials[] = {
    {"_HolaTengoInternet", "HAHAHAHAHA"},  // Red Principal
    {"_WifiEntrada", "mimamamemima"},      // Red Respaldo 1
    {"_Deco-Craft_2.4", "Deco-Craft-P455"} // Red Respaldo 2
};
const int wifiCredentialCount = sizeof(wifiCredentials) / sizeof(wifiCredentials[0]);

// === SERVIDOR WEBSOCKET ===
const char *server_host = "192.168.100.3";
const int server_port = 6972;