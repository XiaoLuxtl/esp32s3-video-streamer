#include "health_monitor.h"
#include "../websocket_manager/websocket_manager.h"
#include "../frame_sender/frame_sender.h"
#include "../configuration/config.h" // <-- Añade esta línea
#include <WiFi.h>
#include <Arduino.h>
#include <esp_camera.h>

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
    json += "\"uptime\":\"" + formatUptime(uptime) + "\",";
    
    // Camera metadata
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"model\":\"OV3660\",";
    
    // Resolution (obtenerlo del sensor si es posible)
    sensor_t *s = esp_camera_sensor_get();
    String resolution = "Unknown";
    if (s) {
        switch (s->status.framesize) {
            case FRAMESIZE_QVGA: resolution = "QVGA (320x240)"; break;
            case FRAMESIZE_VGA: resolution = "VGA (640x480)"; break;
            case FRAMESIZE_SVGA: resolution = "SVGA (800x600)"; break;
            case FRAMESIZE_XGA: resolution = "XGA (1024x768)"; break;
            case FRAMESIZE_HD: resolution = "HD (1280x720)"; break;
            case FRAMESIZE_SXGA: resolution = "SXGA (1280x1024)"; break;
            case FRAMESIZE_UXGA: resolution = "UXGA (1600x1200)"; break;
            default: resolution = "Custom"; break;
        }
    }
    json += "\"resolution\":\"" + resolution + "\"";
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