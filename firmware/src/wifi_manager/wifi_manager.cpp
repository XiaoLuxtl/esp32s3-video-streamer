#include "wifi_manager.h"
#include <Arduino.h>

WifiManager::WifiManager() : webServer(80)
{
    // Constructor
}

bool WifiManager::connect()
{
    if (WiFi.status() == WL_CONNECTED)
        return true;

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);

    // 1. Intentar conectar a redes guardadas (NVS)
    if (connectToSavedNetwork()) {
        return true;
    }

    // 2. Intentar conectar a cada red en la lista hardcoded
    Serial.println("[WiFi] 📂 Probando lista predefinida...");
    for (int i = 0; i < wifiCredentialCount; i++)
    {
        const char *currentSSID = wifiCredentials[i].ssid;
        const char *currentPass = wifiCredentials[i].password;

        Serial.printf("\n[WiFi] 📶 Intentando red %d/%d: %s\n", i + 1, wifiCredentialCount, currentSSID);
        
        // Desconectar intento anterior si lo hubo
        WiFi.disconnect(); 
        delay(100);
        
        WiFi.begin(currentSSID, currentPass);

        if (attemptConnection())
        {
            Serial.printf("[WiFi] ✓ Conexión exitosa a %s\n", currentSSID);
            return true;
        }
        else
        {
            Serial.printf("[WiFi] ✗ Falló conexión a %s\n", currentSSID);
        }
    }

    // 3. Fallback: Portal Captive
    Serial.println("\n[WiFi] ❌ Todas las redes fallaron");
    Serial.println("[WiFi] 🌐 Iniciando Portal Captive...");
    startCaptivePortal();
    
    // startCaptivePortal() bloqueará hasta que se configuren credenciales
    // Después reinicia automáticamente
    return false;
}

bool WifiManager::attemptConnection()
{
    int attempts = 0;
    // Reducir intentos a 20 (aprox 10s) para iterar más rápido entre redes
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        printConnectionInfo();
        return true;
    }

    return false;
}

void WifiManager::checkConnection()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[WiFi] ✗ Desconectado - Reconectando...");
        connect();
    }
}

bool WifiManager::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

int WifiManager::getRSSI()
{
    return WiFi.RSSI();
}

String WifiManager::getIP()
{
    return WiFi.localIP().toString();
}

void WifiManager::printConnectionInfo()
{
    Serial.printf("\n[WiFi] ✓ Conectado\n");
    Serial.printf("   IP: %s\n", getIP().c_str());
    Serial.printf("   RSSI: %d dBm\n", getRSSI());
}

// === NVS METHODS ===

bool WifiManager::connectToSavedNetwork() {
    preferences.begin("wifi", true); // Read-only
    String savedSSID = preferences.getString("ssid", "");
    String savedPass = preferences.getString("password", "");
    preferences.end();

    if (savedSSID != "") {
        Serial.printf("[WiFi] 💾 Red guardada: %s. Conectando...\n", savedSSID.c_str());
        WiFi.disconnect();
        WiFi.begin(savedSSID.c_str(), savedPass.c_str());
        
        if (attemptConnection()) {
            Serial.println("[WiFi] ✓ Conectado con credenciales guardadas");
            return true;
        }
        Serial.println("[WiFi] ✗ No se pudo conectar a red guardada");
    }
    return false;
}

void WifiManager::saveCredentials(String ssid, String password) {
    preferences.begin("wifi", false); // Read-write
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    Serial.println("[WiFi] 💾 Credenciales guardadas en NVS");
}

// === CAPTIVE PORTAL ===

void WifiManager::startCaptivePortal() {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║    🌐 PORTAL CAPTIVE ACTIVADO         ║");
    Serial.println("║                                        ║");
    Serial.println("║  1. Conéctate a 'ESP32-Config'        ║");
    Serial.println("║  2. Abre navegador → 192.168.4.1      ║");
    Serial.println("║  3. Ingresa credenciales WiFi          ║");
    Serial.println("╚════════════════════════════════════════╝\n");
    
    // Configurar como Access Point
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-Config");
    
    IPAddress IP = WiFi.softAPIP();
    Serial.printf("[AP] IP del portal: %s\n", IP.toString().c_str());
    
    // Configurar DNS para redireccionar todas las peticiones
    dnsServer.start(53, "*", IP);
    
    // Configurar rutas del servidor web
    webServer.on("/", [this]() { handleRoot(); });
    webServer.on("/save", HTTP_POST, [this]() { handleSave(); });
    webServer.onNotFound([this]() { handleRoot(); }); // Redirigir todo al portal
    webServer.begin();
    
    Serial.println("[WebServer] Servidor iniciado en puerto 80");
    
    // Loop bloqueante manejando peticiones
    while (true) {
        dnsServer.processNextRequest();
        webServer.handleClient();
        delay(10);
    }
}

void WifiManager::handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 WiFi Config</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 20px;
            padding: 40px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            max-width: 400px;
            width: 100%;
        }
        h1 {
            color: #667eea;
            margin-bottom: 10px;
            font-size: 28px;
            text-align: center;
        }
        p {
            color: #666;
            margin-bottom: 30px;
            text-align: center;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 600;
            font-size: 14px;
        }
        input {
            width: 100%;
            padding: 12px 16px;
            margin-bottom: 20px;
            border: 2px solid #e0e0e0;
            border-radius: 10px;
            font-size: 16px;
            transition: border-color 0.3s;
        }
        input:focus {
            outline: none;
            border-color: #667eea;
        }
        button {
            width: 100%;
            padding: 14px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 10px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s;
        }
        button:hover {
            transform: translateY(-2px);
        }
        button:active {
            transform: translateY(0);
        }
        .info {
            margin-top: 20px;
            padding: 12px;
            background: #f0f0f0;
            border-radius: 8px;
            font-size: 12px;
            color: #666;
            text-align: center;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>📡 ESP32 Config</h1>
        <p>Configura tu red WiFi</p>
        <form action="/save" method="POST">
            <label for="ssid">Nombre de Red (SSID)</label>
            <input type="text" id="ssid" name="ssid" required placeholder="Mi_Red_WiFi">
            
            <label for="password">Contraseña</label>
            <input type="password" id="password" name="password" required placeholder="●●●●●●●●">
            
            <button type="submit">💾 Guardar y Conectar</button>
        </form>
        <div class="info">
            El ESP32 reiniciará después de guardar
        </div>
    </div>
</body>
</html>
)rawliteral";
    
    webServer.send(200, "text/html", html);
}

void WifiManager::handleSave() {
    String ssid = webServer.arg("ssid");
    String password = webServer.arg("password");
    
    Serial.printf("\n[Portal] Credenciales recibidas:\n");
    Serial.printf("  SSID: %s\n", ssid.c_str());
    Serial.printf("  Pass: %s\n", password.c_str());
    
    // Guardar en NVS
    saveCredentials(ssid, password);
    
    // Responder al usuario
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Guardado</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 20px;
            padding: 40px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            max-width: 400px;
            width: 100%;
            text-align: center;
        }
        .success {
            font-size: 60px;
            margin-bottom: 20px;
        }
        h1 {
            color: #667eea;
            margin-bottom: 10px;
        }
        p {
            color: #666;
            line-height: 1.6;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="success">✅</div>
        <h1>¡Configuración Guardada!</h1>
        <p>El ESP32 reiniciará en 3 segundos y se conectará a tu red WiFi.</p>
    </div>
</body>
</html>
)rawliteral";
    
    webServer.send(200, "text/html", html);
    
    // Reiniciar después de 3 segundos
    Serial.println("[Portal] Reiniciando en 3 segundos...");
    delay(3000);
    ESP.restart();
}