# 🖥️ ESP32 Camera Backend & Frontend (Python)

Este es el núcleo de control y visualización del sistema. Se encarga de procesar el flujo binario de la cámara, servir la interfaz web y gestionar la mensajería bidireccional mediante WebSockets.

---

## 🏗️ Arquitectura del Servidor

El servidor está diseñado bajo un modelo híbrido para maximizar la eficiencia en el manejo de datos y comandos:

1.  **Flask (HTTP/API):** Servido en el puerto `6971`. Maneja los archivos estáticos (Dashboard), el API de estadísticas y el health-check.
2.  **WebSockets (Stream & Control):**
    *   **Control (6971):** Túnel JSON para comandos (resolución, calidad) y telemetría.
    *   **Binary Stream (6972):** Protocolo de alta velocidad para recibir frames JPEG, manejando segmentación (chunking) para resoluciones de hasta 3MP.
3.  **Image Saver Worker:** Módulo asíncrono que permite capturar y persistir frames en disco sin bloquear el flujo de video en tiempo real.

---

## 📂 Estructura de Directorios

```text
server/
├── app.py               # Punto de entrada (Inicia Flask y WebSockets)
├── camera_server.py     # Lógica central del servidor WebSocket y ruteo de datos
├── image_saver.py       # Lógica de persistencia de imágenes capturadas
├── config.py            # Configuración de red, puertos y mapeo de cámaras
├── requirements.txt     # Dependencias necesarias
├── config/
│   └── config.ini       # Archivo de configuración persistente
├── static/              # Recursos del Frontend
│   ├── css/             # Estilos del Dashboard moderno
│   └── js/              # Lógica modular (websocket.js, controls.js, etc.)
└── templates/           # Plantillas HTML (Jinja2)
    └── components/      # Componentes UI reutilizables
```

---

## 🚀 Inicio Rápido

### 1. Requisitos
Asegúrate de tener Python 3.9+ instalado. Es recomendable usar un entorno virtual:

```bash
# Crear y activar venv
python -m venv venv
source venv/bin/activate  # En Windows: venv\Scripts\activate
```

### 2. Instalación
Instala las dependencias críticas:
```bash
pip install -r requirements.txt
```

### 3. Ejecución
Inicia el servidor orquestador:
```bash
python app.py
```

El servidor estará disponible en `http://localhost:6971`.

---

## 📊 Endpoints de la API

| Endpoint | Método | Descripción |
| :--- | :--- | :--- |
| `/` | `GET` | Interfaz Principal (Dashboard) |
| `/api/stats` | `GET` | Estadísticas detalladas, clientes y estado de cámara |
| `/api/health` | `GET` | Health check y Uptime del servidor |
| `/api/resolutions`| `GET` | Listado de resoluciones soportadas |
| `/mjpeg` | `GET` | Stream compatible para integración con terceras apps |

---

## 🛠️ Tecnologías Backend

*   **Flask:** Framework web ligero para el Dashboard.
*   **Websockets:** Librería para comunicación asíncrona de alto rendimiento.
*   **Asyncio:** Orquestación de tareas no bloqueantes.
*   **Jinja2:** Motor de plantillas dinámico.
