#include "secrets.h"

// === DEFINICIONES REALES ===
const WifiCredential wifiCredentials[] = {
    {"TUSSID5214", "SuperContrasenha"},     // Red Principal
    {"Red_Secundaria", "password123"},      // Red Respaldo 1
    {"Red_Oficina", "oficina2024"}          // Red Respaldo 2
};
const int wifiCredentialCount = sizeof(wifiCredentials) / sizeof(wifiCredentials[0]);

// === SERVIDOR WEBSOCKET ===
const char *server_host = "192.168.100.3";
const int server_port = 6972;