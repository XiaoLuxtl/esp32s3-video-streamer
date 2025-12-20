# ESP32-S3 Camera WebSocket (Modular)

Sistema de streaming de video modular para ESP32-S3(esp32-s3-devkitc-1) con cámara OV3660.

## Características

- Streaming de video via WebSocket
- Control completo de cámara (resolución, calidad, ajustes)
- Sistema modular y mantenible
- Estadísticas en tiempo real
- Reconexión automática

## Estructura
src/
├── main.cpp # Punto de entrada principal
├── wifi_manager/ # Gestión de conexión WiFi
├── camera_manager/ # Control de cámara
├── websocket_manager/ # Comunicación WebSocket
├── command_processor/ # Procesamiento de comandos
├── frame_sender/ # Envío de frames
├── health_monitor/ # Estadísticas del sistema
└── configuration/ # Configuraciones y constantes


## Comandos Disponibles

| Comando | Valor | Descripción |
|---------|-------|-------------|
| `resolution` | 0-12 | Cambia resolución |
| `quality` | 0-63 | Calidad JPEG |
| `reboot` | - | Reinicia ESP32 |
| `stats` | - | Solicita estadísticas |
| `brightness` | -2 a 2 | Brillo |
| `contrast` | -2 a 2 | Contraste |
| `exposure` | 0/1 | Control exposición |
| `gain` | 0/1 | Control ganancia |
| `whitebalance` | 0/1 | Balance blancos |
| `hmirror` | 0/1 | Espejo horizontal |
| `vflip` | 0/1 | Volteo vertical |

## Resoluciones

0: QQVGA (160x120)
1: QCIF (176x144)
2: HQVGA (240x176)
3: QVGA (320x240)
4: CIF (400x296)
5: VGA (640x480) - Recomendado
6: SVGA (800x600)
7: XGA (1024x768)
8: HD (1280x720)
9: SXGA (1280x1024)
10: UXGA (1600x1200)
11: FHD (1920x1080)
12: QXGA (2048x1536)

## Configuración

1. Edita `src/configuration/secrets.h` con tus credenciales
2. Compila con PlatformIO
3. Ejecuta el servidor Python

## Servidor

El servidor WebSocket está en `../server/app.py`

```bash
python app.py

# Licencia
MIT