#include "frame_sender.h"
#include "../websocket_manager/websocket_manager.h"
#include "../camera_manager/camera_manager.h"
#include "../fps_controller/fps_controller.h"
#include <WiFi.h>
#include <Arduino.h>

FrameSender::FrameSender(WebSocketManager *ws, CameraManager *cam, FPSController *fps)
    : wsManager(ws), camManager(cam), fpsController(fps),
      framesSent(0), framesDropped(0), framesFailed(0),
      lastFrameSize(0), successRate(1.0f), lastSendTime(0),
      totalFrameTime(0), frameTimeCount(0), averageFrameTime(0),
      operationMode(DEFAULT_MODE)
{
    sync.waitingForAck = false;
    sync.ackTimeout = 0;
    sync.frameId = 0;
    
    updateDelaysForMode();
}

void FrameSender::setMode(uint8_t mode)
{
    if (mode != MODE_SPEED && mode != MODE_STABILITY) {
        Serial.println("[📷] ⚠️ Modo inválido, usando estabilidad");
        mode = MODE_STABILITY;
    }
    
    operationMode = mode;
    updateDelaysForMode();
    
    Serial.printf("[📷] ✓ Modo cambiado a: %s\n", getModeName().c_str());
}

uint8_t FrameSender::getMode() const
{
    return operationMode;
}

String FrameSender::getModeName() const
{
    return (operationMode == MODE_SPEED) ? "Velocidad" : "Estabilidad";
}

void FrameSender::updateDelaysForMode()
{
    if (operationMode == MODE_SPEED) {
        delays.betweenChunks = DELAY_BETWEEN_CHUNKS_SPEED;
        delays.afterHeader = DELAY_AFTER_HEADER_SPEED;
        delays.afterFrame = DELAY_AFTER_FRAME_SPEED;
        delays.afterFooter = DELAY_AFTER_FOOTER_SPEED;
        delays.smallFrame = DELAY_SMALL_FRAME_SPEED;
    } else {
        delays.betweenChunks = DELAY_BETWEEN_CHUNKS_STABILITY;
        delays.afterHeader = DELAY_AFTER_HEADER_STABILITY;
        delays.afterFrame = DELAY_AFTER_FRAME_STABILITY;
        delays.afterFooter = DELAY_AFTER_FOOTER_STABILITY;
        delays.smallFrame = DELAY_SMALL_FRAME_STABILITY;
    }
    
    Serial.printf("[📷] Delays configurados: chunks=%dms, header=%dms, frame=%dms\n",
                  delays.betweenChunks, delays.afterHeader, delays.afterFrame);
}

size_t FrameSender::getOptimalChunkSize(size_t frameSize)
{
    // Sistema adaptativo de chunks para soportar resoluciones altas
    
    if (operationMode == MODE_SPEED) {
        // Modo Velocidad: Chunks más grandes para mayor throughput
        if (frameSize > THRESHOLD_XXLARGE)  return CHUNK_SIZE_XLARGE;  // >400KB: 16KB chunks (FHD, QXGA)
        if (frameSize > THRESHOLD_XLARGE)   return CHUNK_SIZE_LARGE;   // >200KB: 8KB chunks
        if (frameSize > THRESHOLD_LARGE)    return CHUNK_SIZE_MEDIUM;  // >100KB: 4KB chunks
        if (frameSize > THRESHOLD_MEDIUM)   return CHUNK_SIZE_SMALL;   // >30KB: 2KB chunks
        return CHUNK_SIZE_TINY;                                         // ≤30KB: 1KB chunks
    } 
    else {
        // Modo Estabilidad: Balance entre velocidad y confiabilidad
        if (frameSize > THRESHOLD_XXLARGE)  return CHUNK_SIZE_LARGE;   // >400KB: 8KB chunks
        if (frameSize > THRESHOLD_XLARGE)   return CHUNK_SIZE_MEDIUM;  // >200KB: 4KB chunks
        if (frameSize > THRESHOLD_LARGE)    return CHUNK_SIZE_SMALL;   // >100KB: 2KB chunks
        return CHUNK_SIZE_TINY;                                         // ≤100KB: 1KB chunks
    }
}

void FrameSender::smartDelay(uint16_t maxDelay)
{
    // Solo hacer delay si realmente es necesario
    if (maxDelay > 0) {
        // Procesar WebSocket durante el delay
        unsigned long start = millis();
        while (millis() - start < maxDelay) {
            wsManager->loop();
            delay(DELAY_WS_PROCESSING);
        }
    }
}

void FrameSender::sendReliable()
{
    if (!wsManager->isConnected()) {
        return;
    }

    unsigned long startTime = millis();

    camera_fb_t *fb = camManager->captureFrame();

    if (!fb || fb->len == 0 || !fb->buf) {
        framesDropped++;
        if (fb) camManager->returnFrame(fb);
        return;
    }

    lastFrameSize = fb->len;

    if (!validateFrame(fb)) {
        framesDropped++;
        camManager->returnFrame(fb);
        return;
    }

    Serial.printf("\n[📷] 🚀 Frame #%lu | %d KB | %dx%d\n", 
                  framesSent + 1, fb->len / 1024, fb->width, fb->height);
    
    // Advertencia para imágenes muy grandes
    if (fb->len > THRESHOLD_XXLARGE) {
        Serial.printf("[📷] ⚠️ IMAGEN MUY GRANDE: %dKB - Usando chunks de ", fb->len / 1024);
        size_t chunkSize = getOptimalChunkSize(fb->len);
        Serial.printf("%dKB\n", chunkSize / 1024);
    }

    bool success = false;

    // Decidir método basado en tamaño
    if (fb->len <= FRAME_SIZE_SMALL) {
        Serial.printf("[📷] Método: Directo (%s)\n", getModeName().c_str());
        success = sendFrameSynchronous(fb);
    }
    else if (fb->len <= FRAME_SIZE_MEDIUM) {
        Serial.printf("[📷] Método: Con ACK (%s)\n", getModeName().c_str());
        success = sendFrameWithAck(fb);
    }
    else {
        size_t chunkSize = getOptimalChunkSize(fb->len);
        Serial.printf("[📷] Método: Chunking (%s, chunks=%dB)\n", 
                     getModeName().c_str(), chunkSize);
        success = sendFrameChunkedReliable(fb, chunkSize);
    }

    unsigned long transferTime = millis() - startTime;

    // Actualizar estadísticas
    if (success) {
        framesSent++;
        lastSendTime = millis();
        
        // Calcular tiempo promedio
        totalFrameTime += transferTime;
        frameTimeCount++;
        averageFrameTime = totalFrameTime / frameTimeCount;
        
        Serial.printf("[📷] ✅ ENVIADO | Tiempo: %lums | Promedio: %lums\n", 
                     transferTime, averageFrameTime);
    } else {
        framesFailed++;
        Serial.printf("[📷] ❌ FALLO | Tiempo: %lums\n", transferTime);
    }

    successRate = (float)framesSent / (framesSent + framesFailed);
    logTransferStats(fb, success, transferTime);

    camManager->returnFrame(fb);

    // NO hacer delay forzado - dejar que el FPS controller maneje el timing
}

bool FrameSender::sendFrameSynchronous(camera_fb_t *fb)
{
    wsManager->sendBinary(fb->buf, fb->len);
    smartDelay(delays.smallFrame);
    return true;
}

bool FrameSender::sendFrameWithAck(camera_fb_t *fb)
{
    uint32_t frameId = ++sync.frameId;

    String header = "{\"type\":\"frame_start\",\"id\":" + String(frameId) +
                    ",\"size\":" + String(fb->len) + "}";
    wsManager->sendText(header);
    
    smartDelay(delays.afterHeader);

    wsManager->sendBinary(fb->buf, fb->len);
    
    smartDelay(delays.afterFrame);

    String footer = "{\"type\":\"frame_end\",\"id\":" + String(frameId) + "}";
    wsManager->sendText(footer);

    smartDelay(delays.afterFooter);

    return true;
}

bool FrameSender::sendFrameChunkedReliable(camera_fb_t *fb, size_t chunkSize)
{
    uint32_t frameId = ++sync.frameId;
    size_t totalSize = fb->len;
    size_t numChunks = (totalSize + chunkSize - 1) / chunkSize;

    Serial.printf("[📷] 📦 %d chunks de %dB (Total: %dKB)\n", 
                  numChunks, chunkSize, totalSize / 1024);

    // Delay adaptativo: imágenes grandes necesitan más tiempo
    uint16_t adaptiveChunkDelay = delays.betweenChunks;
    if (totalSize > THRESHOLD_XXLARGE) {
        adaptiveChunkDelay = delays.betweenChunks * 1.5;  // 50% más delay para imágenes >400KB
        Serial.printf("[📷] 🐢 Usando delay adaptativo: %dms (imagen grande)\n", adaptiveChunkDelay);
    }

    // HEADER
    String header = "{\"type\":\"img_start\",\"id\":" + String(frameId) +
                    ",\"size\":" + String(totalSize) +
                    ",\"chunks\":" + String(numChunks) +
                    ",\"chunkSize\":" + String(chunkSize) +
                    ",\"width\":" + String(fb->width) +
                    ",\"height\":" + String(fb->height) + "}";

    wsManager->sendText(header);
    smartDelay(delays.afterHeader);

    // CHUNKS
    size_t sent = 0;
    int chunkNum = 0;
    unsigned long lastProgressLog = millis();
    unsigned long chunkStartTime = millis();

    while (sent < totalSize) {
        size_t remaining = totalSize - sent;
        size_t currentChunkSize = (remaining < chunkSize) ? remaining : chunkSize;

        wsManager->sendBinary(fb->buf + sent, currentChunkSize);
        sent += currentChunkSize;
        chunkNum++;

        // Delay inteligente adaptativo
        smartDelay(adaptiveChunkDelay);

        // Log de progreso cada 500ms o cada 20%
        unsigned long now = millis();
        int percent = (sent * 100) / totalSize;
        
        if (now - lastProgressLog > 500 || percent % 20 == 0) {
            float speed = (float)sent / ((now - chunkStartTime) / 1000.0) / 1024.0;  // KB/s
            Serial.printf("[📷] 📦 %d%% (%d/%d KB) | %.1f KB/s | Chunk #%d/%d\n",
                         percent, sent / 1024, totalSize / 1024, speed, chunkNum, numChunks);
            lastProgressLog = now;
        }
    }

    // Calcular velocidad final
    unsigned long transferTime = millis() - chunkStartTime;
    float avgSpeed = (float)totalSize / (transferTime / 1000.0) / 1024.0;
    Serial.printf("[📷] 📊 Transferencia completada: %.1f KB/s promedio\n", avgSpeed);

    // FOOTER
    String footer = "{\"type\":\"img_end\",\"id\":" + String(frameId) +
                    ",\"size\":" + String(totalSize) +
                    ",\"success\":true}";

    wsManager->sendText(footer);
    smartDelay(delays.afterFooter);

    Serial.println("[📷] ✅ Chunks completos");

    return true;
}

bool FrameSender::validateFrame(camera_fb_t *fb)
{
    if (fb->len < 100) {
        Serial.println("[📷] ✗ Frame muy pequeño");
        return false;
    }

    if (fb->buf[0] != 0xFF || fb->buf[1] != 0xD8) {
        Serial.println("[📷] ✗ No es JPEG (sin SOI)");
        return false;
    }

    bool hasEOI = false;
    for (size_t i = fb->len - 2; i > fb->len - 100 && i > 0; i--) {
        if (fb->buf[i] == 0xFF && fb->buf[i + 1] == 0xD9) {
            hasEOI = true;
            break;
        }
    }

    if (!hasEOI) {
        Serial.println("[📷] ✗ JPEG incompleto (sin EOI)");
        return false;
    }

    return true;
}

void FrameSender::logTransferStats(camera_fb_t *fb, bool success, unsigned long duration)
{
    if (framesSent % 10 == 0) {  // Log detallado cada 10 frames
        Serial.println("\n[📊] ===== ESTADÍSTICAS =====");
        Serial.printf("    Frames: %lu exitosos, %lu fallos\n", framesSent, framesFailed);
        Serial.printf("    Tasa éxito: %.1f%%\n", successRate * 100);
        Serial.printf("    Tiempo promedio: %lums\n", averageFrameTime);
        Serial.printf("    Modo: %s\n", getModeName().c_str());
        Serial.printf("    RSSI: %d dBm\n", WiFi.RSSI());
        Serial.printf("    Heap: %lu KB\n", esp_get_free_heap_size() / 1024);
        Serial.println("============================\n");
    }
}

void FrameSender::sendHighQuality()
{
    static unsigned long lastFrameTime = 0;
    unsigned long now = millis();

    if (now - lastFrameTime < 2000) {
        return;
    }

    sendReliable();
    lastFrameTime = now;
}

unsigned long FrameSender::getFramesSent() { return framesSent; }
unsigned long FrameSender::getFramesDropped() { return framesDropped; }
size_t FrameSender::getLastFrameSize() { return lastFrameSize; }
float FrameSender::getSuccessRate() const { return successRate; }
unsigned long FrameSender::getLastSendTime() const { return lastSendTime; }
unsigned long FrameSender::getAverageFrameTime() const { return averageFrameTime; }