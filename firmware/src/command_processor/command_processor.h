#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../configuration/config.h"

// Forward declarations
class WebSocketManager;
class CameraManager;
class HealthMonitor;
class FPSController;

class CommandProcessor
{
public:
    CommandProcessor(WebSocketManager *ws, CameraManager *cam, HealthMonitor *health, FPSController *fps);

    void processMessage(const String &message);
    void processCommand(const String &command, const String &value);

private:
    WebSocketManager *wsManager;
    CameraManager *camManager;
    HealthMonitor *healthMonitor;
    FPSController *fpsController;

    // Handlers de comandos (ordenados por prioridad)
    void handleReboot(const String &value);           // PRIORIDAD CRÍTICA
    void handleResolution(const String &value);       // PRIORIDAD ALTA
    void handleQuality(const String &value);          // PRIORIDAD ALTA
    void handleFPS(const String &value);              // PRIORIDAD ALTA
    void handleMode(const String &value);             // PRIORIDAD ALTA (nuevo)
    void handleStats(const String &value);            // PRIORIDAD NORMAL
    void handleBrightness(const String &value);       // PRIORIDAD NORMAL
    void handleContrast(const String &value);         // PRIORIDAD NORMAL
    void handleExposure(const String &value);         // PRIORIDAD NORMAL
    void handleGain(const String &value);             // PRIORIDAD NORMAL
    void handleWhiteBalance(const String &value);     // PRIORIDAD NORMAL
    void handleHMirror(const String &value);          // PRIORIDAD NORMAL
    void handleVFlip(const String &value);            // PRIORIDAD NORMAL

    void sendSuccess(const String &cmd, const String &value = "");
    void sendError(const String &cmd, const String &message = "");
};

#endif