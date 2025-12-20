"""
Módulo para guardar imágenes automáticamente - Versión con estructura por hora
"""

import os
import time
from datetime import datetime
from pathlib import Path
import json
import configparser
from typing import Optional
import threading
from config import logger, IMAGES_DIR


class ImageSaver:
    """Gestiona el guardado de imágenes con nombres secuenciales en estructura por hora"""

    def __init__(self, enabled: bool = False):  # Por defecto desactivado
        self.enabled = enabled
        self.image_counter = {}
        self.lock = threading.Lock()
        self.current_hour_dir = None  # Para evitar crear múltiples directorios

        # Cargar contadores si existen
        self._load_counters()

        # Cargar configuración
        self._load_config()

        if self.enabled:
            logger.info("🖼️  ImageSaver habilitado (persistido)")
        else:
            logger.info("🖼️  ImageSaver deshabilitado")

    def _get_current_path(self) -> Path:
        """Obtiene la ruta actual basada en fecha y hora (HH-00-00)"""
        now = datetime.now()

        # Estructura: /images/YYYY-MM-DD/HH-00-00/
        date_str = now.strftime("%Y-%m-%d")
        hour_str = now.strftime("%H-00-00")

        date_path = IMAGES_DIR / date_str / hour_str

        # Solo crear directorio si es diferente al actual
        if self.current_hour_dir != str(date_path):
            date_path.mkdir(parents=True, exist_ok=True)
            self.current_hour_dir = str(date_path)
            logger.info(f"📁 Directorio de guardado: {date_path}")

        return date_path

    def _get_counter_file(self) -> Path:
        """Ruta al archivo de contadores"""
        return IMAGES_DIR / "counters.json"

    def _get_config_file(self) -> Path:
        """Ruta al archivo de configuración INI"""
        config_dir = Path(__file__).parent / "config"
        config_dir.mkdir(exist_ok=True)
        return config_dir / "config.ini"

    def _load_config(self):
        """Carga la configuración desde archivo INI"""
        try:
            config_file = self._get_config_file()
            if config_file.exists():
                config = configparser.ConfigParser()
                config.read(config_file)

                # Cargar configuración de imágenes
                if config.has_section("images"):
                    self.enabled = config.getboolean(
                        "images", "auto_save", fallback=False
                    )

                logger.debug(f"📋 Configuración cargada: auto_save={self.enabled}")
            else:
                logger.debug(
                    "📄 Archivo de configuración no existe, usando valores por defecto"
                )
        except Exception as e:
            logger.warning(f"⚠️ Error cargando configuración: {e}")

    def _save_config(self):
        """Guarda la configuración actual en archivo INI"""
        try:
            config_file = self._get_config_file()
            config = configparser.ConfigParser()

            # Leer configuración existente si existe
            if config_file.exists():
                config.read(config_file)

            # Asegurar que existe la sección images
            if not config.has_section("images"):
                config.add_section("images")

            # Guardar configuración
            config.set("images", "auto_save", str(self.enabled))

            with open(config_file, "w") as f:
                config.write(f)

            logger.debug(f"💾 Configuración guardada: auto_save={self.enabled}")
        except Exception as e:
            logger.error(f"❌ Error guardando configuración: {e}")

    def _load_counters(self):
        """Carga contadores desde archivo"""
        counter_file = self._get_counter_file()
        if counter_file.exists():
            try:
                with open(counter_file, "r") as f:
                    self.image_counter = json.load(f)
                logger.info(
                    f"📄 Contadores cargados: {len(self.image_counter)} directorios"
                )
            except Exception as e:
                logger.error(f"❌ Error cargando contadores: {e}")
                self.image_counter = {}

    def _save_counters(self):
        """Guarda contadores en archivo"""
        try:
            with open(self._get_counter_file(), "w") as f:
                json.dump(self.image_counter, f, indent=2)
        except Exception as e:
            logger.error(f"❌ Error guardando contadores: {e}")

    def get_next_image_number(self, path: Path) -> int:
        """Obtiene el siguiente número de imagen para un directorio"""
        path_str = str(path)

        with self.lock:
            if path_str not in self.image_counter:
                # Contar archivos existentes en este directorio
                try:
                    existing = list(path.glob("*.jpg"))
                    # Extraer números de los nombres de archivo
                    numbers = []
                    for file in existing:
                        try:
                            # Intentar extraer número del nombre (ej: "5.jpg" -> 5)
                            name = file.stem
                            if name.isdigit():
                                numbers.append(int(name))
                        except:
                            pass

                    if numbers:
                        self.image_counter[path_str] = max(numbers)
                    else:
                        self.image_counter[path_str] = 0
                except Exception as e:
                    logger.error(f"❌ Error contando archivos en {path}: {e}")
                    self.image_counter[path_str] = 0

            # Incrementar
            self.image_counter[path_str] += 1
            self._save_counters()

            return self.image_counter[path_str]

    def save_image(self, image_data: bytes) -> Optional[Path]:
        """
        Guarda una imagen con nombre secuencial en directorio por hora

        Args:
            image_data: Bytes de la imagen JPEG

        Returns:
            Path de la imagen guardada o None
        """
        if not self.enabled or not image_data:
            return None

        try:
            # Obtener ruta con estructura por hora
            save_path = self._get_current_path()

            # Obtener siguiente número secuencial para este directorio
            image_num = self.get_next_image_number(save_path)

            # Crear nombre de archivo simple (ej: "5.jpg")
            filename = f"{image_num}.jpg"
            filepath = save_path / filename

            # Guardar imagen
            with open(filepath, "wb") as f:
                f.write(image_data)

            logger.info(
                f"✅ Imagen guardada: {filepath} ({len(image_data)/1024:.1f} KB)"
            )
            return filepath

        except Exception as e:
            logger.error(f"❌ Error guardando imagen: {e}")
            import traceback

            traceback.print_exc()
            return None

    def toggle_saving(self, enabled: bool = None):
        """Activa/desactiva el guardado de imágenes"""
        if enabled is None:
            self.enabled = not self.enabled
        else:
            self.enabled = enabled

        self._save_config()
        status = "activado" if self.enabled else "desactivado"
        logger.info(f"🔄 Guardado de imágenes {status}")

        # Resetear directorio actual cuando se activa
        if self.enabled:
            self.current_hour_dir = None

        return self.enabled

    def set_enabled(self, value: bool):
        """
        Fuerza el estado exacto enviado por el cliente
        """
        self.enabled = bool(value)
        self._save_config()
        return self.enabled

    def get_saved_images_count(self) -> dict:
        """Obtiene estadísticas de imágenes guardadas con estructura por hora"""
        stats = {"total_dirs": 0, "total_images": 0, "by_date": {}, "by_hour": {}}

        try:
            for date_dir in IMAGES_DIR.glob("*"):
                if date_dir.is_dir():
                    date_stats = {"hour_dirs": 0, "images": 0, "hours": {}}

                    for hour_dir in date_dir.glob("*"):
                        if hour_dir.is_dir():
                            images = list(hour_dir.glob("*.jpg"))
                            hour_stats = {"images": len(images)}

                            date_stats["hours"][hour_dir.name] = hour_stats
                            date_stats["hour_dirs"] += 1
                            date_stats["images"] += len(images)

                    if date_stats["images"] > 0:
                        stats["by_date"][date_dir.name] = date_stats
                        stats["total_dirs"] += date_stats["hour_dirs"]
                        stats["total_images"] += date_stats["images"]

                        # Agregar a by_hour
                        for hour, hour_data in date_stats["hours"].items():
                            key = f"{date_dir.name}/{hour}"
                            stats["by_hour"][key] = hour_data

        except Exception as e:
            logger.error(f"❌ Error contando imágenes: {e}")

        return stats

    def get_last_saved_image(self) -> Optional[dict]:
        """Obtiene información de la última imagen guardada"""
        try:
            # Buscar el directorio más reciente
            date_dirs = sorted(IMAGES_DIR.glob("*"), key=lambda x: x.name, reverse=True)

            for date_dir in date_dirs:
                if date_dir.is_dir():
                    hour_dirs = sorted(
                        date_dir.glob("*"), key=lambda x: x.name, reverse=True
                    )

                    for hour_dir in hour_dirs:
                        if hour_dir.is_dir():
                            images = sorted(
                                hour_dir.glob("*.jpg"),
                                key=lambda x: int(x.stem) if x.stem.isdigit() else -1,
                                reverse=True,
                            )

                            if images:
                                last_image = images[0]
                                return {
                                    "path": str(last_image),
                                    "filename": last_image.name,
                                    "number": (
                                        int(last_image.stem)
                                        if last_image.stem.isdigit()
                                        else 0
                                    ),
                                    "date": date_dir.name,
                                    "hour": hour_dir.name,
                                    "size": (
                                        last_image.stat().st_size
                                        if last_image.exists()
                                        else 0
                                    ),
                                }

        except Exception as e:
            logger.error(f"❌ Error obteniendo última imagen: {e}")

        return None

    def get_today_images_count(self) -> int:
        """Obtiene el número de imágenes guardadas hoy"""
        try:
            today = datetime.now().strftime("%Y-%m-%d")
            today_path = IMAGES_DIR / today

            if not today_path.exists():
                return 0

            total = 0
            for hour_dir in today_path.glob("*"):
                if hour_dir.is_dir():
                    images = list(hour_dir.glob("*.jpg"))
                    total += len(images)
            return total

        except Exception as e:
            logger.error(f"❌ Error contando imágenes de hoy: {e}")
            import traceback

            traceback.print_exc()
            return 0


# Instancia global - Inicialmente DESACTIVADA
image_saver = ImageSaver(enabled=False)
