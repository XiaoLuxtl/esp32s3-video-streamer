#include "command_processor.h"
#include "../fps_controller/fps_controller.h"
#include "../websocket_manager/websocket_manager.h"
#include "../camera_manager/camera_manager.h"
#include "../health_monitor/health_monitor.h"
#include "../frame_sender/frame_sender.h"
#include <Arduino.h>

// Forward declaration del FrameSender global
extern class FrameSender frameSender;

CommandProcessor::CommandProcessor(WebSocketManager *ws, CameraManager *cam, HealthMonitor *health, FPSController *fps)
    : wsManager(ws), camManager(cam), healthMonitor(health), fpsController(fps)
{
}

void CommandProcessor::processMessage(const String &message)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.printf("[CMD] ✗ Error JSON: %s\n", error.c_str());
        return;
    }

    const char *type = doc["type"];
    if (!type || strcmp(type, "command") != 0) {
        return;
    }

    const char *cmd = doc["cmd"];
    const char *val = doc["val"];

    if (!cmd) {
        Serial.println("[CMD] ✗ Comando sin cmd");
        return;
    }

    String command = String(cmd);
    String value = val ? String(val) : "";

    // PRIORIDAD CRÍTICA: REBOOT
    if (command == CMD_REBOOT) {
        Serial.println("\n╔════════════════════════════════╗");
        Serial.println("║  ⚠️  COMANDO DE REINICIO      ║");
        Serial.println("║     PRIORIDAD CRÍTICA          ║");
        Serial.println("╚════════════════════════════════╝");
        handleReboot(value);
        return;  // No procesar nada más
    }

    // Procesar otros comandos
    processCommand(command, value);
}

void CommandProcessor::processCommand(const String &command, const String &value)
{
    Serial.printf("[CMD] Procesando: %s=%s\n", command.c_str(), value.c_str());

    if (command == CMD_RESOLUTION) {
        handleResolution(value);
    }
    else if (command == CMD_QUALITY) {
        handleQuality(value);
    }
    else if (command == CMD_STATS) {
        handleStats(value);
    }
    else if (command == CMD_FPS) {
        handleFPS(value);
    }
    else if (command == CMD_MODE) {
        handleMode(value);
    }
    else if (command == CMD_BRIGHTNESS) {
        handleBrightness(value);
    }
    else if (command == CMD_CONTRAST) {
        handleContrast(value);
    }
    else if (command == CMD_EXPOSURE) {
        handleExposure(value);
    }
    else if (command == CMD_GAIN) {
        handleGain(value);
    }
    else if (command == CMD_WHITEBALANCE) {
        handleWhiteBalance(value);
    }
    else if (command == CMD_HMIRROR) {
        handleHMirror(value);
    }
    else if (command == CMD_VFLIP) {
        handleVFlip(value);
    }
    else {
        sendError(command, "comando desconocido");
        Serial.printf("[CMD] ✗ Comando desconocido: %s\n", command.c_str());
    }
}

void CommandProcessor::handleResolution(const String &value)
{
    int resValue = value.toInt();
    if (camManager->changeResolution(resValue)) {
        String resName = camManager->getResolutionName();
        sendSuccess(CMD_RESOLUTION, value + " (" + resName + ")");
        
        // Pequeña pausa para estabilizar
        delay(DELAY_CAMERA_STABILIZATION);
    } else {
        sendError(CMD_RESOLUTION, "valor no válido (0-12)");
    }
}

void CommandProcessor::handleQuality(const String &value)
{
    int quality = value.toInt();
    if (camManager->setQuality(quality)) {
        sendSuccess(CMD_QUALITY, value);
    } else {
        sendError(CMD_QUALITY, "valor no válido (0-63)");
    }
}

void CommandProcessor::handleReboot(const String &value)
{
    // ⚠️ PRIORIDAD CRÍTICA - NO INTERRUMPIR
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║  🔴 INICIANDO SECUENCIA DE REINICIO  ║");
    Serial.println("╚════════════════════════════════════════╝");
    
    // 1. Notificar al servidor INMEDIATAMENTE
    sendSuccess(CMD_REBOOT, "reiniciando");
    delay(100);  // Asegurar que se envíe
    
    // 2. Enviar mensaje de despedida
    String farewell = "{\"type\":\"status\",\"msg\":\"Dispositivo reiniciando...\"}";
    wsManager->sendText(farewell);
    delay(100);
    
    // 3. Dar tiempo para que los mensajes lleguen
    Serial.println("[REBOOT] ⏳ Enviando notificaciones...");
    for (int i = 3; i > 0; i--) {
        Serial.printf("[REBOOT] Reiniciando en %d...\n", i);
        wsManager->loop();  // Procesar últimos mensajes
        delay(100);
    }
    
    // 4. Limpiar y reiniciar
    Serial.println("[REBOOT] 🔄 Ejecutando reinicio...\n");
    Serial.flush();  // Asegurar que todo se escriba
    
    delay(DELAY_BEFORE_REBOOT);
    
    ESP.restart();
}

void CommandProcessor::handleStats(const String &value)
{
    healthMonitor->sendImmediate();
    sendSuccess(CMD_STATS);
}

void CommandProcessor::handleFPS(const String &value)
{
    int fps = value.toInt();

    if (fps < MIN_FPS || fps > MAX_FPS) {
        sendError(CMD_FPS, "valor no válido (1-30)");
        return;
    }

    fpsController->setFPS(fps);
    float currentFPS = fpsController->getActualFPS();

    String response = String(fps) + " (actual: " + String(currentFPS, 1) + ")";
    sendSuccess(CMD_FPS, response);

    Serial.printf("[CMD] ✓ FPS cambiado a: %d\n", fps);
}

void CommandProcessor::handleMode(const String &value)
{
    uint8_t mode;
    
    if (value == "0" || value == "speed" || value == "velocidad") {
        mode = MODE_SPEED;
    } 
    else if (value == "1" || value == "stability" || value == "estabilidad") {
        mode = MODE_STABILITY;
    }
    else {
        sendError(CMD_MODE, "valor no válido (0=velocidad, 1=estabilidad)");
        return;
    }
    
    frameSender.setMode(mode);
    String modeName = frameSender.getModeName();
    sendSuccess(CMD_MODE, modeName);
    
    Serial.printf("[CMD] ✓ Modo cambiado a: %s\n", modeName.c_str());
}

void CommandProcessor::handleBrightness(const String &value)
{
    int brightness = value.toInt();
    if (camManager->setBrightness(brightness)) {
        sendSuccess(CMD_BRIGHTNESS, value);
    } else {
        sendError(CMD_BRIGHTNESS, "valor no válido (-2 a 2)");
    }
}

void CommandProcessor::handleContrast(const String &value)
{
    int contrast = value.toInt();
    if (camManager->setContrast(contrast)) {
        sendSuccess(CMD_CONTRAST, value);
    } else {
        sendError(CMD_CONTRAST, "valor no válido (-2 a 2)");
    }
}

void CommandProcessor::handleExposure(const String &value)
{
    bool enable = (value == "1" || value == "true" || value == "on");
    if (camManager->setExposure(enable)) {
        sendSuccess(CMD_EXPOSURE, enable ? "on" : "off");
    } else {
        sendError(CMD_EXPOSURE);
    }
}

void CommandProcessor::handleGain(const String &value)
{
    bool enable = (value == "1" || value == "true" || value == "on");
    if (camManager->setGain(enable)) {
        sendSuccess(CMD_GAIN, enable ? "on" : "off");
    } else {
        sendError(CMD_GAIN);
    }
}

void CommandProcessor::handleWhiteBalance(const String &value)
{
    bool enable = (value == "1" || value == "true" || value == "on");
    if (camManager->setWhiteBalance(enable)) {
        sendSuccess(CMD_WHITEBALANCE, enable ? "on" : "off");
    } else {
        sendError(CMD_WHITEBALANCE);
    }
}

void CommandProcessor::handleHMirror(const String &value)
{
    bool enable = (value == "1" || value == "true" || value == "on");
    if (camManager->setHMirror(enable)) {
        sendSuccess(CMD_HMIRROR, enable ? "on" : "off");
    } else {
        sendError(CMD_HMIRROR);
    }
}

void CommandProcessor::handleVFlip(const String &value)
{
    bool enable = (value == "1" || value == "true" || value == "on");
    if (camManager->setVFlip(enable)) {
        sendSuccess(CMD_VFLIP, enable ? "on" : "off");
    } else {
        sendError(CMD_VFLIP);
    }
}

void CommandProcessor::sendSuccess(const String &cmd, const String &value)
{
    wsManager->sendCommandResponse(cmd, "ok", value);
}

void CommandProcessor::sendError(const String &cmd, const String &message)
{
    wsManager->sendCommandResponse(cmd, "error", message);
}