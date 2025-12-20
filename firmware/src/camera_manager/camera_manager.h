#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <Arduino.h>
#include <esp_camera.h>
#include "../configuration/pins.h"
#include "../configuration/config.h"

class CameraManager
{
public:
    CameraManager();

    bool init();
    bool changeResolution(int resValue);
    bool setQuality(int quality);
    bool setBrightness(int value);
    bool setContrast(int value);
    bool setExposure(bool enable);
    bool setGain(bool enable);
    bool setWhiteBalance(bool enable);
    bool setHMirror(bool enable);
    bool setVFlip(bool enable);

    camera_fb_t *captureFrame();
    void returnFrame(camera_fb_t *fb);

    framesize_t getCurrentResolution();
    int getCurrentQuality();
    String getResolutionName();
    String getSupportedResolutions();

    // NUEVO: Recovery y validación
    bool resetSensor();                                  // Reset completo del sensor
    bool validateResolutionChange(framesize_t expected); // Validar cambio de resolución
    bool isFrameBlack(camera_fb_t *fb);                  // Detectar frames negros

private:
    bool initCamera();
    framesize_t mapResolution(int resValue);

    framesize_t currentResolution;
    int currentQuality;
};

#endif