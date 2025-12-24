#ifndef SECRETS_H
#define SECRETS_H

// === CREDENCIALES WIFI ===
struct WifiCredential {
    const char* ssid;
    const char* password;
};

extern const WifiCredential wifiCredentials[];
extern const int wifiCredentialCount;

// === SERVIDOR WEBSOCKET ===
extern const char *server_host;
extern const int server_port;

#endif