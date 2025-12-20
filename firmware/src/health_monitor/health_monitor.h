#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include <Arduino.h>

// Forward declarations
class WebSocketManager;
class FrameSender;

class HealthMonitor
{
public:
    HealthMonitor(WebSocketManager *ws);

    void sendPeriodic();
    void sendImmediate();
    void setStartTime(unsigned long startTime);
    void setFrameSender(FrameSender *fs);

private:
    WebSocketManager *wsManager;
    FrameSender *frameSender;
    unsigned long lastHealthTime;
    unsigned long systemStartTime;

    String generateHealthJson();
    String formatUptime(unsigned long seconds);
};

#endif