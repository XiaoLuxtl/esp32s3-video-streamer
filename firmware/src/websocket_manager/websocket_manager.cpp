#include "websocket_manager.h"
#include "../configuration/secrets.h" // <--- Aquí es donde viven los valores reales

WebSocketManager::WebSocketManager() : connected(false) {}

void WebSocketManager::init()
{
    Serial.println("\n[WS] === INICIALIZANDO WEBSOCKET ===");
    Serial.printf("[WS] 🔌 Conectando a: %s:%d\n", server_host, server_port);

    // Limpiar cualquier conexión previa
    webSocket.disconnect();

    // Configurar WebSocket
    webSocket.begin(server_host, server_port, "/");
    webSocket.setReconnectInterval(3000); // 3 segundos
    webSocket.enableHeartbeat(15000, 3000, 2);

    // Timeouts más cortos para debugging
    // webSocket.setTimeout(1000); // 1 segundo timeout

    Serial.println("[WS] ✓ Configuración WebSocket completada");
    Serial.printf("[WS] 📍 Ruta: /\n");
    Serial.printf("[WS] ⏱️  Timeout: 1s, Reconnect: 3s\n");
}

void WebSocketManager::loop()
{
    static unsigned long lastLoopLog = 0;
    unsigned long now = millis();

    // Log cada 10 segundos
    if (now - lastLoopLog > 10000)
    {
        Serial.printf("[WS] 🔄 Loop activo. Estado: %s\n",
                      connected ? "Conectado" : "Desconectado");
        lastLoopLog = now;
    }

    webSocket.loop();
}

bool WebSocketManager::isConnected()
{
    // Es vital verificar ambos para que no intente enviar a un socket muerto
    return (connected && webSocket.isConnected());
}

void WebSocketManager::setConnected(bool connected)
{
    this->connected = connected;
}

void WebSocketManager::setEventCallback(void (*callback)(WStype_t, uint8_t *, size_t))
{
    webSocket.onEvent(callback);
}

void WebSocketManager::sendBinary(const uint8_t *data, size_t length)
{
    if (isConnected())
    {
        webSocket.sendBIN(data, length);
    }
}

void WebSocketManager::sendText(const String &text)
{
    if (isConnected())
    {
        webSocket.sendTXT(text.c_str());
    }
}

void WebSocketManager::sendCommandResponse(const String &cmd, const String &status, const String &value)
{
    // Construcción manual de JSON para ahorrar memoria de la pila
    String response = "{\"type\":\"response\",\"cmd\":\"" + cmd + "\",\"status\":\"" + status + "\"";
    if (value.length() > 0)
    {
        response += ",\"value\":\"" + value + "\"";
    }
    response += "}";

    sendText(response);
}