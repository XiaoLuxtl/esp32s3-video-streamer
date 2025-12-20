#include "camera_manager.h"
#include <Arduino.h>

CameraManager::CameraManager()
    : currentResolution(FRAMESIZE_VGA), currentQuality(DEFAULT_QUALITY)
{
}

bool CameraManager::init()
{
    return initCamera();
}

bool CameraManager::initCamera()
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 10000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = currentResolution;
    config.jpeg_quality = currentQuality;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("[CAM] ✗ Error inicializando: 0x%x\n", err);
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s)
    {
        delay(DELAY_CAMERA_STABILIZATION);
        s->set_framesize(s, currentResolution);
        s->set_vflip(s, 0);
        s->set_hmirror(s, 0);
        s->set_gain_ctrl(s, 1);
        s->set_agc_gain(s, 0);
    }
    return true;
}

framesize_t CameraManager::mapResolution(int resValue)
{
    switch (resValue)
    {
    case RES_QQVGA:
        return FRAMESIZE_QQVGA;
    case RES_QCIF:
        return FRAMESIZE_QCIF;
    case RES_HQVGA:
        return FRAMESIZE_HQVGA;
    case RES_QVGA:
        return FRAMESIZE_QVGA;
    case RES_CIF:
        return FRAMESIZE_CIF;
    case RES_VGA:
        return FRAMESIZE_VGA;
    case RES_SVGA:
        return FRAMESIZE_SVGA;
    case RES_XGA:
        return FRAMESIZE_XGA;
    case RES_HD:
        return FRAMESIZE_HD;
    case RES_SXGA:
        return FRAMESIZE_SXGA;
    case RES_UXGA:
        return FRAMESIZE_UXGA;
    case RES_FHD:
        return FRAMESIZE_FHD;
    case RES_QXGA:
        return FRAMESIZE_QXGA;
    default:
        return FRAMESIZE_VGA;
    }
}

bool CameraManager::resetSensor()
{
    /**
     * Reset completo del sensor de cámara
     * Útil cuando el sensor queda en estado corrupto
     */
    Serial.println("\n[CAM] 🔄 RESETEANDO SENSOR...");

    // 1. Deinicializar cámara
    esp_err_t err = esp_camera_deinit();
    if (err != ESP_OK)
    {
        Serial.printf("[CAM] ⚠️ Error deinit: 0x%x\n", err);
    }

    // 2. Esperar a que se liberen recursos
    delay(500);

    // 3. Reinicializar
    bool success = initCamera();

    if (success)
    {
        Serial.println("[CAM] ✅ Sensor reseteado correctamente");
    }
    else
    {
        Serial.println("[CAM] ❌ Error reseteando sensor");
    }

    return success;
}

bool CameraManager::changeResolution(int resValue)
{
    framesize_t newResolution = mapResolution(resValue);

    // ADVERTENCIA para resoluciones peligrosas
    if (resValue >= RES_FHD)
    {
        Serial.println("\n╔═══════════════════════════════════════╗");
        Serial.println("║  ⚠️  RESOLUCIÓN ALTA DETECTADA       ║");
        Serial.println("║  Puede causar inestabilidad          ║");
        Serial.println("║  Recomendación: UXGA (10) máximo     ║");
        Serial.println("╚═══════════════════════════════════════╝");
    }

    sensor_t *s = esp_camera_sensor_get();

    if (s && newResolution != currentResolution)
    {
        Serial.printf("[CAM] Cambiando resolución de %d a %d\n",
                      currentResolution, resValue);

        // Intentar cambio normal
        esp_err_t err = s->set_framesize(s, newResolution);

        if (err == ESP_OK)
        {
            framesize_t oldResolution = currentResolution;
            currentResolution = newResolution;

            // Delay adaptativo según resolución
            int stabilizationDelay = DELAY_CAMERA_STABILIZATION;
            if (resValue >= RES_FHD)
            {
                stabilizationDelay = 300; // 3x más delay para resoluciones altas
                Serial.printf("[CAM] 🐢 Delay extendido: %dms\n", stabilizationDelay);
            }
            delay(stabilizationDelay);

            // VALIDAR que el cambio funcionó
            bool validated = validateResolutionChange(newResolution);

            if (!validated)
            {
                Serial.println("[CAM] ❌ Validación falló - Intentando recovery...");

                // Intentar volver a resolución anterior
                err = s->set_framesize(s, oldResolution);
                if (err == ESP_OK)
                {
                    currentResolution = oldResolution;
                    delay(DELAY_CAMERA_STABILIZATION);
                    Serial.printf("[CAM] ✓ Revertido a: %s\n", getResolutionName().c_str());
                }
                else
                {
                    // Si no puede revertir, resetear sensor completo
                    Serial.println("[CAM] 🔴 No puede revertir - Reseteando sensor...");
                    resetSensor();
                }
                return false;
            }

            Serial.printf("[CAM] ✓ Resolución validada: %s\n", getResolutionName().c_str());
            return true;
        }
        else
        {
            Serial.printf("[CAM] ✗ Error cambiando resolución: 0x%x\n", err);

            // Intentar resetear sensor si falla
            if (resValue >= RES_FHD)
            {
                Serial.println("[CAM] 🔄 Reseteando sensor después de error...");
                resetSensor();
            }
        }
    }
    return false;
}

bool CameraManager::validateResolutionChange(framesize_t expectedResolution)
{
    /**
     * Valida que el sensor realmente cambió de resolución
     * capturando frames de prueba
     */
    Serial.println("[CAM] 🔍 Validando cambio de resolución...");

    // Limpiar buffer viejo
    for (int i = 0; i < 3; i++)
    {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb)
            esp_camera_fb_return(fb);
        delay(50);
    }

    // Capturar frame de validación
    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb)
    {
        Serial.println("[CAM] ✗ No se pudo capturar frame de validación");
        return false;
    }

    bool valid = true;

    // Verificar tamaño mínimo
    if (fb->len < 1000)
    {
        Serial.printf("[CAM] ✗ Frame muy pequeño: %d bytes\n", fb->len);
        valid = false;
    }

    // Verificar que no es todo negro
    if (isFrameBlack(fb))
    {
        Serial.println("[CAM] ✗ Frame completamente negro");
        valid = false;
    }

    // Verificar marcadores JPEG
    if (fb->buf[0] != 0xFF || fb->buf[1] != 0xD8)
    {
        Serial.println("[CAM] ✗ No es JPEG válido (sin SOI)");
        valid = false;
    }

    esp_camera_fb_return(fb);

    return valid;
}

bool CameraManager::isFrameBlack(camera_fb_t *fb)
{
    /**
     * Detecta si un frame es completamente negro
     * (indica sensor corrupto o mal configurado)
     */
    if (!fb || !fb->buf || fb->len < 100)
    {
        return true;
    }

    // Muestrear primeros 1KB del frame
    size_t sampleSize = (fb->len < 1024) ? fb->len : 1024;
    int nonZeroBytes = 0;

    for (size_t i = 0; i < sampleSize; i++)
    {
        if (fb->buf[i] != 0x00 && fb->buf[i] != 0xFF)
        {
            nonZeroBytes++;
        }
    }

    // Si menos del 10% son bytes no-negro, considerar frame negro
    float nonZeroRatio = (float)nonZeroBytes / sampleSize;

    if (nonZeroRatio < 0.1)
    {
        Serial.printf("[CAM] ⚫ Frame negro detectado (%.1f%% datos)\n",
                      nonZeroRatio * 100);
        return true;
    }

    return false;
}

bool CameraManager::setQuality(int quality)
{
    if (quality >= MIN_QUALITY && quality <= MAX_QUALITY)
    {
        sensor_t *s = esp_camera_sensor_get();
        if (s)
        {
            s->set_quality(s, quality);
            currentQuality = quality;
            Serial.printf("[CAM] ✓ Calidad: %d\n", quality);
            return true;
        }
    }
    return false;
}

bool CameraManager::setBrightness(int value)
{
    sensor_t *s = esp_camera_sensor_get();
    if (s && value >= -2 && value <= 2)
    {
        s->set_brightness(s, value);
        Serial.printf("[CAM] ✓ Brillo: %d\n", value);
        return true;
    }
    return false;
}

bool CameraManager::setContrast(int value)
{
    sensor_t *s = esp_camera_sensor_get();
    if (s && value >= -2 && value <= 2)
    {
        s->set_contrast(s, value);
        Serial.printf("[CAM] ✓ Contraste: %d\n", value);
        return true;
    }
    return false;
}

bool CameraManager::setExposure(bool enable)
{
    sensor_t *s = esp_camera_sensor_get();
    if (s)
    {
        s->set_exposure_ctrl(s, enable ? 1 : 0);
        Serial.printf("[CAM] ✓ Exposición %s\n", enable ? "ON" : "OFF");
        return true;
    }
    return false;
}

bool CameraManager::setGain(bool enable)
{
    sensor_t *s = esp_camera_sensor_get();
    if (s)
    {
        s->set_gain_ctrl(s, enable ? 1 : 0);
        Serial.printf("[CAM] ✓ Ganancia %s\n", enable ? "ON" : "OFF");
        return true;
    }
    return false;
}

bool CameraManager::setWhiteBalance(bool enable)
{
    sensor_t *s = esp_camera_sensor_get();
    if (s)
    {
        s->set_whitebal(s, enable ? 1 : 0);
        Serial.printf("[CAM] ✓ Balance blancos %s\n", enable ? "ON" : "OFF");
        return true;
    }
    return false;
}

bool CameraManager::setHMirror(bool enable)
{
    sensor_t *s = esp_camera_sensor_get();
    if (s)
    {
        s->set_hmirror(s, enable ? 1 : 0);
        Serial.printf("[CAM] ✓ Espejo H %s\n", enable ? "ON" : "OFF");
        return true;
    }
    return false;
}

bool CameraManager::setVFlip(bool enable)
{
    sensor_t *s = esp_camera_sensor_get();
    if (s)
    {
        s->set_vflip(s, enable ? 1 : 0);
        Serial.printf("[CAM] ✓ Volteo V %s\n", enable ? "ON" : "OFF");
        return true;
    }
    return false;
}

camera_fb_t *CameraManager::captureFrame()
{
    // Limpiar buffer para obtener frame fresco
    for (int i = 0; i < 2; i++)
    {
        camera_fb_t *fb_old = esp_camera_fb_get();
        if (fb_old)
            esp_camera_fb_return(fb_old);
    }

    // Capturar frame actual
    camera_fb_t *fb = esp_camera_fb_get();

    // DETECTAR FRAMES NEGROS (sensor corrupto)
    static int consecutiveBlackFrames = 0;

    if (fb && isFrameBlack(fb))
    {
        consecutiveBlackFrames++;

        if (consecutiveBlackFrames >= 3)
        {
            Serial.println("\n╔═══════════════════════════════════════╗");
            Serial.println("║  🔴 SENSOR CORRUPTO DETECTADO        ║");
            Serial.println("║  3+ frames negros consecutivos       ║");
            Serial.println("║  Ejecutando auto-recovery...         ║");
            Serial.println("╚═══════════════════════════════════════╝");

            esp_camera_fb_return(fb);

            // Intentar resetear sensor
            if (resetSensor())
            {
                consecutiveBlackFrames = 0;
                // Capturar nuevo frame después de reset
                return esp_camera_fb_get();
            }
            else
            {
                // Si reset falla, reiniciar ESP32
                Serial.println("[CAM] 🔴 Reset falló - REINICIANDO ESP32 en 2s...");
                delay(2000);
                ESP.restart();
            }
        }
    }
    else
    {
        // Frame válido, resetear contador
        consecutiveBlackFrames = 0;
    }

    return fb;
}

void CameraManager::returnFrame(camera_fb_t *fb)
{
    esp_camera_fb_return(fb);
}

framesize_t CameraManager::getCurrentResolution()
{
    return currentResolution;
}

int CameraManager::getCurrentQuality()
{
    return currentQuality;
}

String CameraManager::getResolutionName()
{
    switch (currentResolution)
    {
    case FRAMESIZE_QQVGA:
        return "QQVGA (160x120)";
    case FRAMESIZE_QCIF:
        return "QCIF (176x144)";
    case FRAMESIZE_HQVGA:
        return "HQVGA (240x176)";
    case FRAMESIZE_QVGA:
        return "QVGA (320x240)";
    case FRAMESIZE_CIF:
        return "CIF (400x296)";
    case FRAMESIZE_VGA:
        return "VGA (640x480)";
    case FRAMESIZE_SVGA:
        return "SVGA (800x600)";
    case FRAMESIZE_XGA:
        return "XGA (1024x768)";
    case FRAMESIZE_HD:
        return "HD (1280x720)";
    case FRAMESIZE_SXGA:
        return "SXGA (1280x1024)";
    case FRAMESIZE_UXGA:
        return "UXGA (1600x1200)";
    case FRAMESIZE_FHD:
        return "FHD (1920x1080)";
    case FRAMESIZE_QXGA:
        return "QXGA (2048x1536)";
    default:
        return "Unknown";
    }
}

String CameraManager::getSupportedResolutions()
{
    return "0:QQVGA(160x120),1:QCIF(176x144),2:HQVGA(240x176),3:QVGA(320x240),"
           "4:CIF(400x296),5:VGA(640x480),6:SVGA(800x600),7:XGA(1024x768),"
           "8:HD(1280x720),9:SXGA(1280x1024),10:UXGA(1600x1200),11:FHD(1920x1080),"
           "12:QXGA(2048x1536)";
}