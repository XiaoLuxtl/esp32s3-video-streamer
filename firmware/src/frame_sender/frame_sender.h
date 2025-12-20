#ifndef FRAME_SENDER_H
#define FRAME_SENDER_H

#include <Arduino.h>
#include <esp_camera.h>
#include "../configuration/config.h"

class WebSocketManager;
class CameraManager;
class FPSController;

class FrameSender
{
public:
    FrameSender(WebSocketManager *ws, CameraManager *cam, FPSController *fps);

    void sendReliable();
    void sendHighQuality();

    // Gestión de modos
    void setMode(uint8_t mode);
    uint8_t getMode() const;
    String getModeName() const;

    // Estadísticas
    unsigned long getFramesSent();
    unsigned long getFramesDropped();
    size_t getLastFrameSize();
    float getSuccessRate() const;
    unsigned long getLastSendTime() const;
    unsigned long getAverageFrameTime() const;

private:
    WebSocketManager *wsManager;
    CameraManager *camManager;
    FPSController *fpsController;

    // Estadísticas
    unsigned long framesSent;
    unsigned long framesDropped;
    unsigned long framesFailed;
    size_t lastFrameSize;
    float successRate;
    unsigned long lastSendTime;
    
    // Tiempos de procesamiento
    unsigned long totalFrameTime;
    unsigned long frameTimeCount;
    unsigned long averageFrameTime;

    // Modo de operación
    uint8_t operationMode;

    // Delays configurables según modo
    struct DelayConfig {
        uint16_t betweenChunks;
        uint16_t afterHeader;
        uint16_t afterFrame;
        uint16_t afterFooter;
        uint16_t smallFrame;
    } delays;

    // Sistema de confirmación
    struct {
        bool waitingForAck;
        unsigned long ackTimeout;
        uint32_t frameId;
    } sync;

    // Métodos de envío
    bool sendFrameSynchronous(camera_fb_t *fb);
    bool sendFrameWithAck(camera_fb_t *fb);
    bool sendFrameChunkedReliable(camera_fb_t *fb, size_t chunkSize);

    // Métodos auxiliares
    bool validateFrame(camera_fb_t *fb);
    void logTransferStats(camera_fb_t *fb, bool success, unsigned long duration);
    void updateDelaysForMode();
    size_t getOptimalChunkSize(size_t frameSize);
    void smartDelay(uint16_t maxDelay);
};

#endif