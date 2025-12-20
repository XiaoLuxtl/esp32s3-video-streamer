#ifndef FPS_CONTROLLER_H
#define FPS_CONTROLLER_H

#include <Arduino.h>
#include "../configuration/config.h"

class FPSController
{
public:
    FPSController();

    void setFPS(int fps);
    int getFPS();
    unsigned long getFrameInterval();

    bool shouldSendFrame();
    void frameSent();

    void setEnabled(bool enabled);
    bool isEnabled();

    float getActualFPS();

private:
    int targetFPS;
    unsigned long frameInterval;
    unsigned long lastFrameTime;
    bool enabled;

    unsigned long frameTimes[10];
    int frameIndex;
    unsigned long totalFrameTime;
};

#endif