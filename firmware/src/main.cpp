#include <Arduino.h>
#include "configuration/config.h"
#include "wifi_manager/wifi_manager.h"
#include "camera_manager/camera_manager.h"
#include "websocket_manager/websocket_manager.h"
#include "frame_sender/frame_sender.h"
#include "health_monitor/health_monitor.h"
#include "command_processor/command_processor.h"
#include "fps_controller/fps_controller.h"

// === VARIABLES GLOBALES ===
unsigned long lastConnectionCheck = 0;
unsigned long systemStartTime = 0;

// === INSTANCIAS ===
WifiManager wifiManager;
CameraManager cameraManager;
WebSocketManager wsManager;
FPSController fpsController;
FrameSender frameSender(&wsManager, &cameraManager, &fpsController);
HealthMonitor healthMonitor(&wsManager);
CommandProcessor commandProcessor(&wsManager, &cameraManager, &healthMonitor, &fpsController);

// === FUNCIÓN DE EVENTOS WEBSOCKET ===
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.println("[WS] ✗ Desconectado del servidor");
        wsManager.setConnected(false);
        break;

    case WStype_CONNECTED:
    {
        Serial.printf("[WS] ✓ CONECTADO: %s:%d\n", server_host, server_port);
        wsManager.setConnected(true);

        // Secuencia de registro optimizada
        delay(50);  // Pausa mínima inicial

        // 1. Registrar como cámara
        String registerMsg = "{\"type\":\"register\",\"device\":\"camera\"}";
        wsManager.sendText(registerMsg);
        Serial.printf("[WS] 📝 Registro: %s\n", registerMsg.c_str());
        
        delay(100);

        // 2. Enviar información de configuración
        String resolutions = cameraManager.getSupportedResolutions();
        String infoMsg = "{\"type\":\"info\",\"resolutions\":\"" + resolutions + 
                        "\",\"mode\":\"" + frameSender.getModeName() + 
                        "\",\"fps\":" + String(fpsController.getFPS()) + "}";
        wsManager.sendText(infoMsg);
        Serial.printf("[WS] 📋 Info enviada\n");

        delay(100);

        // 3. Health inicial
        healthMonitor.sendImmediate();

        Serial.println("[WS] ✅ Registro completo");
    }
    break;

    case WStype_TEXT:
    {
        String message = String((char *)payload);
        Serial.printf("[WS] 📩 RX: %s\n", payload);
        commandProcessor.processMessage(message);
    }
    break;

    case WStype_ERROR:
        Serial.printf("[WS] ✗ Error: %s\n", payload);
        break;

    case WStype_PING:
        Serial.println("[WS] 🏓 Ping");
        break;

    case WStype_PONG:
        Serial.println("[WS] 🏓 Pong");
        break;
    }
}

// === SETUP ===
void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║  ESP32-S3 CAMERA STREAMING v7.0    ║");
    Serial.println("║  Sistema Inteligente Optimizado    ║");
    Serial.println("╚════════════════════════════════════╝\n");

    systemStartTime = millis();
    healthMonitor.setStartTime(systemStartTime);
    healthMonitor.setFrameSender(&frameSender);

    // Configurar sistema por defecto
    fpsController.setFPS(DEFAULT_FPS);
    frameSender.setMode(DEFAULT_MODE);

    // Inicializar cámara
    Serial.println("[INIT] Inicializando cámara...");
    if (!cameraManager.init()) {
        Serial.println("[ERROR] ✗ Cámara falló - RESTART en 3s");
        delay(3000);
        ESP.restart();
    }
    Serial.println("[INIT] ✓ Cámara iniciada");

    // Conectar WiFi
    Serial.println("[INIT] Conectando WiFi...");
    if (!wifiManager.connect()) {
        Serial.println("[ERROR] ✗ WiFi falló - RESTART en 3s");
        delay(3000);
        ESP.restart();
    }
    Serial.println("[INIT] ✓ WiFi conectado");

    // Configurar WebSocket
    Serial.println("[INIT] Configurando WebSocket...");
    wsManager.setEventCallback(webSocketEvent);
    wsManager.init();
    Serial.println("[INIT] ✓ WebSocket configurado");

    // Resumen del sistema
    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║        CONFIGURACIÓN ACTUAL        ║");
    Serial.println("╠════════════════════════════════════╣");
    Serial.printf("║ Resolución: %-22s ║\n", cameraManager.getResolutionName().c_str());
    Serial.printf("║ Calidad JPEG: %-19d ║\n", cameraManager.getCurrentQuality());
    Serial.printf("║ Modo: %-28s ║\n", frameSender.getModeName().c_str());
    Serial.printf("║ FPS objetivo: %-19d ║\n", fpsController.getFPS());
    Serial.printf("║ IP: %-30s ║\n", wifiManager.getIP().c_str());
    Serial.printf("║ RSSI: %-26d dBm ║\n", wifiManager.getRSSI());
    Serial.println("╚════════════════════════════════════╝\n");

    Serial.println("[✓] Sistema listo - Iniciando streaming\n");
}

// === LOOP OPTIMIZADO ===
void loop()
{
    unsigned long now = millis();

    // 1. Procesar WebSocket (siempre prioritario)
    wsManager.loop();

    // 2. Verificar conexión periódicamente
    if (now - lastConnectionCheck >= CONNECTION_CHECK) {
        wifiManager.checkConnection();
        lastConnectionCheck = now;
    }

    // 3. Envío de frames con control inteligente
    static unsigned long lastFrameAttempt = 0;
    
    // Usar el intervalo del FPS controller
    unsigned long frameInterval = fpsController.getFrameInterval();
    
    if (now - lastFrameAttempt >= frameInterval) {
        if (WiFi.status() == WL_CONNECTED && wsManager.isConnected()) {
            frameSender.sendReliable();
            lastFrameAttempt = now;
        } else {
            // Log estado solo cada 5 segundos
            static unsigned long lastStatusLog = 0;
            if (now - lastStatusLog >= 5000) {
                Serial.printf("[STATUS] WiFi: %s, WS: %s\n",
                             WiFi.status() == WL_CONNECTED ? "✅" : "❌",
                             wsManager.isConnected() ? "✅" : "❌");
                lastStatusLog = now;
            }
        }
    }

    // 4. Health periódico
    static unsigned long lastHealth = 0;
    if (wsManager.isConnected() && now - lastHealth >= HEALTH_INTERVAL) {
        healthMonitor.sendPeriodic();
        lastHealth = now;
    }

    // 5. Delay mínimo del sistema
    delay(DELAY_MAIN_LOOP);
}