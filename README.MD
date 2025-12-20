# 📸 ESP32-S3 Camera WebSocket (Modular)

<p align="center">
  <video 
    src="https://github.com/user-attachments/assets/8f298395-81b6-4b6e-a4ea-9b2534fb09f9" 
    width="800" 
    autoplay 
    muted 
    loop 
    playsinline 
    title="Demo del sistema de streaming y panel de control">
    Tu navegador no soporta videos incrustados. Puedes ver la demo directamente en el enlace de assets.
  </video>
</p>

Sistema de streaming de video de alta eficiencia diseñado para el **ESP32-S3-N16R8** con sensor **OV3660**. Este proyecto destaca por una arquitectura de comunicación desacoplada y un firmware altamente modularizado.
---

## 🏗️ Arquitectura de Comunicación Dual

Para optimizar el rendimiento y evitar latencia en los comandos por el flujo de datos, el sistema utiliza dos canales WebSockets independientes:

* **Puerto 6971 (Control & UI):** Canal para el envío de comandos JSON desde la interfaz web y recepción de telemetría/estadísticas.
* **Puerto 6972 (Binary Stream):** Canal de alta prioridad dedicado exclusivamente a la transferencia de frames binarios desde el ESP32 hacia el servidor.



---

## 📂 Estructura del Proyecto

```text
.
├── firmware                 # Desarrollo C++ (PlatformIO)
│   ├── src
│   │   ├── camera_manager   # Lógica de bajo nivel para el sensor OV3660
│   │   ├── command_processor# Parser de JSON y ejecución de acciones
│   │   ├── configuration    # Configuración de red, pines y secretos
│   │   ├── fps_controller   # Gestión de tiempos y suavidad del stream
│   │   ├── frame_sender     # Empaquetado binario de frames para transporte
│   │   ├── health_monitor   # Telemetría de sistema (Memoria, Uptime)
│   │   ├── websocket_manager# Cliente WebSocket dual (Control/Stream)
│   │   ├── wifi_manager     # Gestión de conexión y watchdog de red
│   │   └── main.cpp         # Orquestador principal del sistema
│   └── platformio.ini       # Dependencias y perfiles de compilación
│
└── server                   # Backend & Frontend (Python/JS)
    ├── app.py               # Servidor Flask y punto de entrada
    ├── camera_server.py     # Procesamiento lógico del stream entrante
    ├── image_saver.py       # Utilidad para persistencia de capturas
    ├── requirements.txt     # Dependencias de Python
    ├── static/
    │   ├── css/             # Estilos modernos del Dashboard
    │   └── js/              # Arquitectura Frontend Modular:
    │       ├── websocket.js # Gestión de túneles de datos
    │       ├── controls.js  # Lógica de la interfaz de usuario
    │       ├── image-saver.js # Interacción con el sistema de guardado
    │       ├── notifications.js # Feedback visual al usuario
    │       └── app.js       # Inicialización y pegamento del frontend
    └── templates/           # Estructura de plantillas Jinja2:
        ├── components/      # Fragmentos reutilizables (Sidebar, Feed, Header)
        └── index.html       # Vista principal
```

---

## 🕹️ Control de Cámara (API de Comandos)

Los comandos se envían vía JSON a través del puerto **6971**:

| Comando | Rango / Valor | Descripción |
| :--- | :--- | :--- |
| `resolution` | 0 - 12 | Desde QQVGA hasta QXGA (3MP) |
| `quality` | 0 - 63 | Factor de compresión JPEG (Menor = Mayor calidad) |
| `brightness` | -2 a 2 | Compensación de exposición |
| `vflip` / `hmirror` | 0 / 1 | Inversión y rotación de imagen |
| `reboot` | - | Reinicio remoto del hardware |

---

## 🚀 Instalación y Despliegue

### 1. Firmware (ESP32-S3)
1. Dirígete a `firmware/src/configuration/`.
2. Renombra `secrets.h.example` a `secrets.h` y completa tus credenciales WiFi e IP del servidor.
3. Carga el código usando **PlatformIO**:
```bash
pio run --target upload
```

### 2. Servidor (Python)
1. Instala las dependencias:
```bash
cd server
pip install -r requirements.txt
```
2. Ejecuta la aplicación:
```bash
python app.py
```

---

## 🛠️ Tecnologías Utilizadas
* **Firmware:** C++, PlatformIO, Arduino Core, ESPAsyncWebServer.
* **Backend:** Python 3.x, Flask, Eventlet, WebSockets.
* **Frontend:** HTML5, CSS3 (Modern UI), Vanilla JavaScript (Modular).

---

## 📄 Licencia
Este proyecto está bajo la Licencia **MIT**.
