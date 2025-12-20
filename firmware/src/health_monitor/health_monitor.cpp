#include "health_monitor.h"
#include "../websocket_manager/websocket_manager.h"
#include "../frame_sender/frame_sender.h"
#include "../configuration/config.h" // <-- Añade esta línea
#include <WiFi.h>
#include <Arduino.h>

HealthMonitor::HealthMonitor(WebSocketManager *ws)
    : wsManager(ws), frameSender(nullptr), lastHealthTime(0), systemStartTime(0)
{
}

void HealthMonitor::setStartTime(unsigned long startTime)
{
    systemStartTime = startTime;
}

void HealthMonitor::setFrameSender(FrameSender *fs)
{
    frameSender = fs;
}

void HealthMonitor::sendPeriodic()
{
    unsigned long now = millis();
    if (now - lastHealthTime >= HEALTH_INTERVAL)
    {
        sendImmediate();
        lastHealthTime = now;
    }
}

void HealthMonitor::sendImmediate()
{
    String json = generateHealthJson();
    wsManager->sendText(json);
    Serial.println("[💚] Health enviado");
}

String HealthMonitor::generateHealthJson()
{
    unsigned long uptime = (millis() - systemStartTime) / 1000;

    String json = "{\"type\":\"health\",";

    if (frameSender)
    {
        json += "\"frames\":" + String(frameSender->getFramesSent()) + ",";
        json += "\"dropped\":" + String(frameSender->getFramesDropped()) + ",";
    }
    else
    {
        json += "\"frames\":0,";
        json += "\"dropped\":0,";
    }

    json += "\"heap\":" + String(esp_get_free_heap_size()) + ",";
    json += "\"minHeap\":" + String(esp_get_minimum_free_heap_size()) + ",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"uptime\":\"" + formatUptime(uptime) + "\"";
    json += "}";

    return json;
}

String HealthMonitor::formatUptime(unsigned long seconds)
{
    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    unsigned long secs = seconds % 60;

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%luh %lum %lus", hours, minutes, secs);
    return String(buffer);
}