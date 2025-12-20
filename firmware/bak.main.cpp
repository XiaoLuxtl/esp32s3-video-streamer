#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h> // ¡Añade esta librería!

#include "secrets.h"

// === PINES ESP32-S3 OV3660 ===
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 15
#define SIOD_GPIO_NUM 4
#define SIOC_GPIO_NUM 5
#define Y9_GPIO_NUM 16
#define Y8_GPIO_NUM 17
#define Y7_GPIO_NUM 18
#define Y6_GPIO_NUM 12
#define Y5_GPIO_NUM 10
#define Y4_GPIO_NUM 8
#define Y3_GPIO_NUM 9
#define Y2_GPIO_NUM 11
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM 7
#define PCLK_GPIO_NUM 13

// === CONFIGURACIÓN ===
#define FRAME_INTERVAL 50      // 20 FPS
#define HEALTH_INTERVAL 10000  // 10s
#define CONNECTION_CHECK 30000 // Verificar conexión cada 30s

// === ESTADO ===
WebSocketsClient webSocket;
bool wsConnected = false;
unsigned long lastFrameTime = 0;
unsigned long lastHealthTime = 0;
unsigned long lastConnectionCheck = 0;
unsigned long framesSent = 0;
unsigned long framesDropped = 0;
unsigned long systemStartTime = 0;
framesize_t currentResolution = FRAMESIZE_VGA;

// === CÁMARA ===
bool initCamera()
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
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = currentResolution;
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.fb_location = CAMERA_FB_IN_PSRAM;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("[CAM] ✗ Error: 0x%x\n", err);
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s)
    {
        s->set_brightness(s, 0);
        s->set_contrast(s, 0);
        s->set_whitebal(s, 1);
        s->set_awb_gain(s, 1);
        s->set_exposure_ctrl(s, 1);
        s->set_gain_ctrl(s, 1);
        s->set_hmirror(s, 0);
        s->set_vflip(s, 0);
    }

    Serial.println("[CAM] ✓ OK");
    return true;
}

// === WIFI ===
bool connectWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
        return true;

    Serial.printf("[WiFi] Conectando a %s...\n", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("\n[WiFi] ✓ IP: %s | RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        return true;
    }

    Serial.println("\n[WiFi] ✗ Failed");
    return false;
}

// === ENVIAR FRAME ===
void sendFrame()
{
    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb)
    {
        framesDropped++;
        return;
    }

    if (fb->len == 0 || !fb->buf)
    {
        esp_camera_fb_return(fb);
        framesDropped++;
        return;
    }

    // Enviar frame via WebSocket
    webSocket.sendBIN(fb->buf, fb->len);
    framesSent++;

    // Log cada 50 frames
    if (framesSent % 50 == 0)
    {
        Serial.printf("[📊] Frames:%lu Drop:%lu Size:%dKB Heap:%luKB RSSI:%ddBm\n",
                      framesSent, framesDropped, fb->len / 1024,
                      esp_get_free_heap_size() / 1024, WiFi.RSSI());
    }

    esp_camera_fb_return(fb);
}

// === ENVIAR HEALTH ===
void sendHealth()
{
    if (!wsConnected)
        return;

    unsigned long uptime = (millis() - systemStartTime) / 1000;

    String json = "{\"type\":\"health\",";
    json += "\"frames\":" + String(framesSent) + ",";
    json += "\"dropped\":" + String(framesDropped) + ",";
    json += "\"heap\":" + String(esp_get_free_heap_size()) + ",";
    json += "\"minHeap\":" + String(esp_get_minimum_free_heap_size()) + ",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"uptime\":\"" + String(uptime / 3600) + "h " + String((uptime % 3600) / 60) + "m\"";
    json += "}";

    webSocket.sendTXT(json);
    Serial.println("[💚] Health enviado");
}

// === FUNCIÓN PARA CAMBIAR RESOLUCIÓN ===
bool changeResolution(int resValue)
{
    framesize_t newResolution;

    // Mapear valores a resoluciones
    switch (resValue)
    {
    case 5:
        newResolution = FRAMESIZE_QQVGA;
        break; // 160x120
    case 6:
        newResolution = FRAMESIZE_QVGA;
        break; // 320x240
    case 7:
        newResolution = FRAMESIZE_VGA;
        break; // 640x480
    case 8:
        newResolution = FRAMESIZE_SVGA;
        break; // 800x600
    case 9:
        newResolution = FRAMESIZE_XGA;
        break; // 1024x768
    case 10:
        newResolution = FRAMESIZE_SXGA;
        break; // 1280x1024
    case 11:
        newResolution = FRAMESIZE_UXGA;
        break; // 1600x1200
    default:
        return false; // Valor no válido
    }

    if (newResolution != currentResolution)
    {
        // Primero, detener la cámara
        esp_camera_deinit();

        // Cambiar la resolución actual
        currentResolution = newResolution;

        // Reinicializar la cámara con nueva resolución
        if (initCamera())
        {
            Serial.printf("[⚡] Resolución cambiada a: %d\n", resValue);
            return true;
        }
        else
        {
            Serial.println("[✗] Error al cambiar resolución");
            return false;
        }
    }
    return true;
}

// === PROCESAR COMANDOS ===
void processCommand(const String &command, const String &value)
{
    Serial.printf("[⚡] Comando: %s, Valor: %s\n", command.c_str(), value.c_str());

    if (command == "resolution")
    {
        int resValue = value.toInt();
        if (changeResolution(resValue))
        {
            // Enviar confirmación
            String response = "{\"type\":\"response\",\"cmd\":\"resolution\",\"status\":\"ok\",\"value\":" + value + "}";
            webSocket.sendTXT(response);
        }
        else
        {
            String response = "{\"type\":\"response\",\"cmd\":\"resolution\",\"status\":\"error\",\"value\":\"invalid\"}";
            webSocket.sendTXT(response);
        }
    }
    else if (command == "reboot")
    {
        Serial.println("[⚡] Reiniciando en 1 segundo...");
        String response = "{\"type\":\"response\",\"cmd\":\"reboot\",\"status\":\"ok\"}";
        webSocket.sendTXT(response);
        delay(1000);
        ESP.restart();
    }
    else if (command == "stats")
    {
        // Enviar estadísticas inmediatas
        sendHealth();
        Serial.println("[⚡] Estadísticas enviadas");
    }
    else if (command == "quality")
    {
        int quality = value.toInt();
        if (quality >= 0 && quality <= 63)
        {
            sensor_t *s = esp_camera_sensor_get();
            if (s)
            {
                s->set_quality(s, quality);
                Serial.printf("[⚡] Calidad JPEG cambiada a: %d\n", quality);
                String response = "{\"type\":\"response\",\"cmd\":\"quality\",\"status\":\"ok\",\"value\":" + value + "}";
                webSocket.sendTXT(response);
            }
        }
    }
    else
    {
        Serial.printf("[⚡] Comando desconocido: %s\n", command.c_str());
        String response = "{\"type\":\"response\",\"cmd\":\"" + command + "\",\"status\":\"unknown\"}";
        webSocket.sendTXT(response);
    }
}

// === WEBSOCKET EVENTS ===
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.println("[WS] ✗ Desconectado");
        wsConnected = false;
        break;

    case WStype_CONNECTED:
        Serial.printf("[WS] ✓ Conectado: %s\n", payload);
        wsConnected = true;

        // Registrar como cámara
        webSocket.sendTXT("{\"type\":\"register\",\"device\":\"camera\"}");
        Serial.println("[WS] ✓ Registrado como cámara");

        // Health inicial
        delay(500);
        sendHealth();
        break;

    case WStype_TEXT:
    {
        // Enclose this case in braces to create a scope
        Serial.printf("[📩] Mensaje: %s\n", payload);

        // Convertir payload a String
        String message = String((char *)payload);

        // Usar ArduinoJson para parsear el mensaje (versión actualizada)
        JsonDocument doc; // Cambiado de StaticJsonDocument a JsonDocument
        DeserializationError error = deserializeJson(doc, message);

        if (!error)
        {
            // Verificar si es un comando (forma actualizada)
            const char *typeStr = doc["type"];
            if (typeStr && strcmp(typeStr, "command") == 0)
            {
                const char *cmdStr = doc["cmd"];
                const char *valStr = doc["val"];

                if (cmdStr && valStr)
                {
                    String command = String(cmdStr);
                    String value = String(valStr);
                    processCommand(command, value);
                }
                else
                {
                    Serial.println("[✗] Comando sin cmd o val");
                }
            }
            else
            {
                Serial.println("[ℹ] Mensaje no es un comando");
            }
        }
        else
        {
            Serial.printf("[✗] Error parseando JSON: %s\n", error.c_str());
        }
    }
    break;

    case WStype_BIN:
        Serial.println("[WS] Binary recibido (no esperado)");
        break;

    case WStype_ERROR:
        Serial.println("[WS] ✗ Error");
        break;

    case WStype_PING:
    case WStype_PONG:
        // Ping/Pong automático
        break;
    }
}

// === SETUP ===
void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║   ESP32-S3 CAMERA - ROBUSTO       ║");
    Serial.println("║   Versión 5.1 con Comandos        ║");
    Serial.println("╚════════════════════════════════════╝\n");

    systemStartTime = millis();

    // Inicializar cámara
    if (!initCamera())
    {
        Serial.println("[ERROR] ✗ Cámara falló - RESTART en 3s");
        delay(3000);
        ESP.restart();
    }

    // Conectar WiFi
    if (!connectWiFi())
    {
        Serial.println("[ERROR] ✗ WiFi falló - RESTART en 3s");
        delay(3000);
        ESP.restart();
    }

    // Configurar WebSocket
    Serial.printf("[WS] Conectando a %s:%d\n", server_host, server_port);
    webSocket.begin(server_host, server_port, "/");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
    webSocket.enableHeartbeat(15000, 3000, 2);

    Serial.println("[✓] SETUP Completo - Sistema listo\n");
}

// === LOOP PRINCIPAL ===
void loop()
{
    unsigned long now = millis();

    // 1. SIEMPRE procesar WebSocket primero
    webSocket.loop();

    // 2. Verificar WiFi periódicamente
    if (now - lastConnectionCheck >= CONNECTION_CHECK)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("[WiFi] ✗ Desconectado - Reconectando...");
            connectWiFi();
        }
        lastConnectionCheck = now;
    }

    // 3. Enviar frames continuamente
    if (now - lastFrameTime >= FRAME_INTERVAL)
    {
        sendFrame();
        lastFrameTime = now;
    }

    // 4. Enviar health periódicamente
    if (wsConnected && now - lastHealthTime >= HEALTH_INTERVAL)
    {
        sendHealth();
        lastHealthTime = now;
    }

    // Pequeño delay para estabilidad
    delay(1);
}