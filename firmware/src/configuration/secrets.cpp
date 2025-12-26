#include "secrets.h"

// === DEFINICIONES REALES ===
const WifiCredential wifiCredentials[] = {
    {"redPrincipal", "tusupercontrasenhasupersegura"},  // Red Principal
    {"redRespaldo1", "tusupercontrasenhasupersegura"},      // Red Respaldo 1
    {"redRespaldo2", "tusupercontrasenhasupersegura"} // Red Respaldo 2
};
const int wifiCredentialCount = sizeof(wifiCredentials) / sizeof(wifiCredentials[0]);

// === SERVIDOR WEBSOCKET ===
const char *server_host = "192.168.100.3";
const int server_port = 6972;