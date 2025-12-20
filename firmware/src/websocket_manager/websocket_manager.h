#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <Arduino.h>
#include <WebSocketsClient.h>

class WebSocketManager
{
private:
    WebSocketsClient webSocket;
    bool connected;

public:
    WebSocketManager();
    void init(); // Sin parámetros, porque los toma de secrets.h
    void loop();
    bool isConnected();
    void setConnected(bool connected);
    void setEventCallback(void (*callback)(WStype_t, uint8_t *, size_t));

    void sendBinary(const uint8_t *data, size_t length);
    void sendText(const String &text);
    void sendCommandResponse(const String &cmd, const String &status, const String &value = "");
};

#endif