"""
Configuración centralizada del servidor de cámara
"""
from pathlib import Path
import logging

# === PATHS ===
BASE_DIR = Path(__file__).parent
TEMPLATES_DIR = BASE_DIR / "templates"
STATIC_DIR = BASE_DIR / "static"
IMAGES_DIR = BASE_DIR / "images"

# Crear directorios si no existen
IMAGES_DIR.mkdir(exist_ok=True)
STATIC_DIR.mkdir(exist_ok=True)
(STATIC_DIR / "css").mkdir(exist_ok=True)

# === PORTS ===
HTTP_PORT = 6971
WS_PORT = 6972

# === RESOLUTION MAPPING ===
RESOLUTIONS = {
    0: "QQVGA (160x120)",
    1: "QCIF (176x144)", 
    2: "HQVGA (240x176)",
    3: "QVGA (320x240)",
    4: "CIF (400x296)",
    5: "VGA (640x480)",
    6: "SVGA (800x600)",
    7: "XGA (1024x768)",
    8: "HD (1280x720)",
    9: "SXGA (1280x1024)",
    10: "UXGA (1600x1200)",
    11: "FHD (1920x1080)",
    12: "QXGA (2048x1536)"
}

# Resolución por defecto
DEFAULT_RESOLUTION = 5  # VGA

# === CAMERA COMMANDS ===
COMMANDS = {
    "resolution": {
        "type": "int",
        "range": (0, 12),
        "priority": 1,  # HIGH
        "description": "Cambiar resolución"
    },
    "quality": {
        "type": "int", 
        "range": (10, 63),
        "priority": 1,  # HIGH
        "description": "Calidad JPEG (10=mejor, 63=peor)"
    },
    "fps": {
        "type": "int",
        "range": (1, 30),
        "priority": 1,  # HIGH
        "description": "Frames por segundo"
    },
    "mode": {
        "type": "int",
        "range": (0, 1),
        "priority": 1,  # HIGH
        "description": "0=Velocidad, 1=Estabilidad"
    },
    "reboot": {
        "type": "trigger",
        "priority": 0,  # CRITICAL - Máxima prioridad
        "description": "Reiniciar cámara"
    },
    "stats": {
        "type": "trigger",
        "priority": 2,  # NORMAL
        "description": "Solicitar estadísticas"
    },
    "flash": {
        "type": "int",
        "range": (0, 255),
        "priority": 2,  # NORMAL
        "description": "Intensidad flash LED"
    },
    "brightness": {
        "type": "int",
        "range": (-2, 2),
        "priority": 2,  # NORMAL
        "description": "Brillo"
    },
    "contrast": {
        "type": "int",
        "range": (-2, 2),
        "priority": 2,  # NORMAL
        "description": "Contraste"
    },
    "saturation": {
        "type": "int", 
        "range": (-2, 2),
        "priority": 2,  # NORMAL
        "description": "Saturación"
    }
}

# === LOGGING ===
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    datefmt='%H:%M:%S'
)
logger = logging.getLogger("CameraServer")

# === STREAM SETTINGS ===
MAX_CHUNKS = 200  # Aumentado para imágenes grandes
MAX_FRAME_SIZE = 5 * 1024 * 1024  # 5MB
WS_PING_INTERVAL = 20
WS_PING_TIMEOUT = 10

# === CHUNKING TIMEOUTS ===
CHUNK_TIMEOUT = 10  # segundos - Para completar una imagen chunked
CHUNK_CLEANUP_INTERVAL = 30  # segundos - Limpiar buffers viejos

# === PRIORITY LEVELS ===
PRIORITY_CRITICAL = 0  # Reboot, emergencias
PRIORITY_HIGH = 1      # Resolución, FPS, modo
PRIORITY_NORMAL = 2    # Stats, ajustes menores
PRIORITY_LOW = 3       # Info