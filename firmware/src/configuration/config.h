#ifndef CONFIG_H
#define CONFIG_H

// === CONFIGURACIÓN GENERAL ===
#define FRAME_INTERVAL_MIN 33     // 30 FPS máximo (1000ms/30 = 33ms)
#define FRAME_INTERVAL_MAX 1000   // 1 FPS mínimo (1000ms/1 = 1000ms)
#define FRAME_INTERVAL_DEFAULT 50 // 20 FPS por defecto
#define HEALTH_INTERVAL 10000     // 10s
#define CONNECTION_CHECK 30000    // Verificar conexión cada 30s

// === MODOS DE OPERACIÓN ===
#define MODE_SPEED 0     // Velocidad: FPS altos, menos pausas
#define MODE_STABILITY 1 // Estabilidad: Transferencias confiables (DEFAULT)
#define DEFAULT_MODE MODE_STABILITY

// === TIMEOUTS Y DELAYS (CONFIGURABLES) ===
// Modo Estabilidad
#define DELAY_BETWEEN_CHUNKS_STABILITY 30 // ms entre chunks (estabilidad)
#define DELAY_AFTER_HEADER_STABILITY 150  // ms después de enviar header
#define DELAY_AFTER_FRAME_STABILITY 200   // ms después de frame completo
#define DELAY_AFTER_FOOTER_STABILITY 200  // ms después de footer
#define DELAY_SMALL_FRAME_STABILITY 50    // ms para frames pequeños

// Modo Velocidad (OPTIMIZADO para FPS altos)
#define DELAY_BETWEEN_CHUNKS_SPEED 1 // ms entre chunks
#define DELAY_AFTER_HEADER_SPEED 2   // ms después de enviar heade
#define DELAY_AFTER_FRAME_SPEED 2    // ms después de frame completo
#define DELAY_AFTER_FOOTER_SPEED 5   // ms después de footer
#define DELAY_SMALL_FRAME_SPEED 1    // ms para frames pequeños

// Delays mínimos del sistema (siempre activos)
#define DELAY_MAIN_LOOP 5              // ms en el loop principal
#define DELAY_WS_PROCESSING 2          // ms para procesamiento WS
#define DELAY_CAMERA_STABILIZATION 100 // ms después de cambiar resolución
#define DELAY_BEFORE_REBOOT 500        // ms antes de reiniciar (URGENTE)

// === CHUNK SIZES ADAPTATIVOS ===
// Para imágenes pequeñas (<30KB)
#define CHUNK_SIZE_TINY 1024 // 1KB

// Para imágenes medianas (30-100KB)
#define CHUNK_SIZE_SMALL 2048 // 2KB

// Para imágenes medianas-grandes (100-200KB)
#define CHUNK_SIZE_MEDIUM 6144 // 6KB (era 4KB, +50% para velocidad)

// Para imágenes grandes (200-400KB) - HD, SXGA
#define CHUNK_SIZE_LARGE 12288 // 12KB (era 8KB, +50% para velocidad)

// Para imágenes muy grandes (>400KB) - FHD, QXGA
#define CHUNK_SIZE_XLARGE 24576 // 24KB (era 16KB, +50% para velocidad)

// === UMBRALES DE TAMAÑO DE FRAME ===
#define FRAME_SIZE_SMALL 10000  // ≤10KB - Envío directo
#define FRAME_SIZE_MEDIUM 30000 // 10-30KB - Con ACK
// 30-200KB - Chunking normal
// 200KB+ - Chunking con chunks grandes

// === UMBRALES PARA CHUNK SIZES ===
#define THRESHOLD_MEDIUM 30000   // 30KB
#define THRESHOLD_LARGE 100000   // 100KB
#define THRESHOLD_XLARGE 200000  // 200KB
#define THRESHOLD_XXLARGE 400000 // 400KB

// === RESOLUCIONES DISPONIBLES PARA OV3660/ESP32-S3 ===
#define RES_QQVGA 0 // 160x120     - Muy rápida, baja calidad
#define RES_QCIF 1  // 176x144     - QCIF standard
#define RES_HQVGA 2 // 240x176     - Half QVGA
#define RES_QVGA 3  // 320x240     - Buena para streaming
#define RES_CIF 4   // 400x296     - CIF standard
#define RES_VGA 5   // 640x480     - Recomendado (balance calidad/velocidad)
#define RES_SVGA 6  // 800x600     - Buena calidad
#define RES_XGA 7   // 1024x768    - Alta calidad
#define RES_HD 8    // 1280x720    - HD (720p)
#define RES_SXGA 9  // 1280x1024   - SXGA
#define RES_UXGA 10 // 1600x1200   - UXGA (máximo recomendado)
#define RES_FHD 11  // 1920x1080   - Full HD (requiere modo estabilidad)
#define RES_QXGA 12 // 2048x1536   - 3MP nativo (requiere modo estabilidad)

// === CALIDAD JPEG ===
#define MIN_QUALITY 5
#define MAX_QUALITY 63
#define DEFAULT_QUALITY 15

// === FPS ===
#define MIN_FPS 1
#define MAX_FPS 30
#define DEFAULT_FPS 20

// === COMANDOS DISPONIBLES ===
#define CMD_RESOLUTION "resolution"
#define CMD_REBOOT "reboot"
#define CMD_STATS "stats"
#define CMD_QUALITY "quality"
#define CMD_FPS "fps"
#define CMD_BRIGHTNESS "brightness"
#define CMD_CONTRAST "contrast"
#define CMD_EXPOSURE "exposure"
#define CMD_GAIN "gain"
#define CMD_WHITEBALANCE "whitebalance"
#define CMD_HMIRROR "hmirror"
#define CMD_VFLIP "vflip"
#define CMD_FRAMESIZE "framesize"
#define CMD_MODE "mode"

// === PRIORIDADES DE COMANDOS ===
#define PRIORITY_CRITICAL 0 // Reboot, emergencias
#define PRIORITY_HIGH 1     // Cambios de resolución, calidad
#define PRIORITY_NORMAL 2   // Stats, ajustes menores
#define PRIORITY_LOW 3      // Info, health

#endif