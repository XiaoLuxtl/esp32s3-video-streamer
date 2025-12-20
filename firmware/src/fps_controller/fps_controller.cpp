#include "fps_controller.h"
#include <Arduino.h>

FPSController::FPSController()
    : targetFPS(DEFAULT_FPS),
      frameInterval(1000 / DEFAULT_FPS),
      lastFrameTime(0),
      enabled(true),
      frameIndex(0),
      totalFrameTime(0)
{

    // Inicializar array de tiempos
    for (int i = 0; i < 10; i++)
    {
        frameTimes[i] = frameInterval;
    }
}

void FPSController::setFPS(int fps)
{
    if (fps < MIN_FPS)
        fps = MIN_FPS;
    if (fps > MAX_FPS)
        fps = MAX_FPS;

    targetFPS = fps;
    frameInterval = 1000 / fps;

    Serial.printf("[FPS] ✓ Objetivo: %d FPS (intervalo: %lums)\n", fps, frameInterval);
}

int FPSController::getFPS()
{
    return targetFPS;
}

unsigned long FPSController::getFrameInterval()
{
    return frameInterval;
}

bool FPSController::shouldSendFrame()
{
    if (!enabled)
        return true;

    unsigned long now = millis();
    return (now - lastFrameTime >= frameInterval);
}

void FPSController::frameSent()
{
    if (!enabled)
        return;

    unsigned long now = millis();
    unsigned long frameTime = now - lastFrameTime;
    lastFrameTime = now;

    // Calcular FPS real (promedio de últimos 10 frames)
    totalFrameTime -= frameTimes[frameIndex];
    frameTimes[frameIndex] = frameTime;
    totalFrameTime += frameTime;

    frameIndex = (frameIndex + 1) % 10;
}

void FPSController::setEnabled(bool enabled)
{
    this->enabled = enabled;
    Serial.printf("[FPS] %s\n", enabled ? "✓ Activado" : "✗ Desactivado");
}

bool FPSController::isEnabled()
{
    return enabled;
}

float FPSController::getActualFPS()
{
    if (totalFrameTime == 0)
        return 0;

    float avgFrameTime = totalFrameTime / 10.0;
    return 1000.0 / avgFrameTime;
}