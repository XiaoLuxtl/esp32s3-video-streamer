"""
Servidor WebSocket para manejo de conexiones de cámara
Con sistema de prioridades y manejo robusto de resoluciones altas
"""

import asyncio
import websockets
import threading
import json
import time
from datetime import datetime
from collections import defaultdict, deque
from typing import Set, Optional
import logging

from config import (
    logger,
    WS_PORT,
    MAX_FRAME_SIZE,
    WS_PING_INTERVAL,
    WS_PING_TIMEOUT,
    CHUNK_TIMEOUT,
    CHUNK_CLEANUP_INTERVAL,
    COMMANDS,
    PRIORITY_CRITICAL,
    PRIORITY_HIGH,
    PRIORITY_NORMAL,
)
from image_saver import image_saver


class FPSCounter:
    """Contador de FPS en tiempo real"""

    def __init__(self, window_size: float = 1.0):
        self.timestamps = []
        self.window_size = window_size

    def tick(self) -> float:
        """Registra un nuevo frame y devuelve FPS"""
        now = time.time()
        self.timestamps.append(now)

        cutoff = now - self.window_size
        self.timestamps = [t for t in self.timestamps if t > cutoff]

        return len(self.timestamps)

    def get_fps(self) -> float:
        """Obtiene FPS actual sin registrar nuevo frame"""
        now = time.time()
        cutoff = now - self.window_size
        recent = [t for t in self.timestamps if t > cutoff]
        return len(recent)


class CommandQueue:
    """Cola de comandos con prioridades"""

    def __init__(self):
        self.queue = []
        self.lock = asyncio.Lock()

    async def add(self, command: dict, priority: int = PRIORITY_NORMAL):
        """Agrega comando con prioridad"""
        async with self.lock:
            self.queue.append((priority, time.time(), command))
            # Ordenar por prioridad (menor número = mayor prioridad)
            self.queue.sort(key=lambda x: (x[0], x[1]))

            if priority == PRIORITY_CRITICAL:
                logger.warning(
                    f"🔴 COMANDO CRÍTICO ENCOLADO: {command.get('cmd', 'unknown')}"
                )

    async def get(self):
        """Obtiene siguiente comando"""
        async with self.lock:
            if self.queue:
                return self.queue.pop(0)[2]  # Retornar solo el comando
            return None

    async def clear(self):
        """Limpia la cola"""
        async with self.lock:
            self.queue.clear()

    def size(self):
        """Tamaño de la cola"""
        return len(self.queue)


class CameraWebSocketServer:
    """Servidor WebSocket para streaming de cámara con manejo robusto"""

    def __init__(self):
        self.camera_client = None
        self.browser_clients: Set = set()
        self.latest_frame = None
        self.frame_lock = threading.Lock()

        # Cola de comandos con prioridades
        self.command_queue = CommandQueue()

        # Estadísticas
        self.stats = {
            "connected": False,
            "frames_received": 0,
            "frames_chunked": 0,
            "frames_failed": 0,
            "total_bytes": 0,
            "fps": 0,
            "last_frame_time": None,
            "camera_ip": None,
            "browsers_connected": 0,
            "start_time": time.time(),
            "pending_commands": 0,
        }

        # Para chunking
        self.chunk_buffers = defaultdict(bytearray)
        self.chunk_metadata = {}

        # Contador FPS
        self.fps_counter = FPSCounter()

        # Task para limpiar buffers viejos
        self.cleanup_task = None

        logger.info("📡 WebSocket Server inicializado con sistema de prioridades")

    def get_stats(self) -> dict:
        """Obtiene estadísticas actualizadas"""
        self.stats["browsers_connected"] = len(self.browser_clients)
        self.stats["uptime"] = time.time() - self.stats["start_time"]
        self.stats["pending_commands"] = self.command_queue.size()

        # Formatear uptime
        uptime = self.stats["uptime"]
        hours, remainder = divmod(int(uptime), 3600)
        minutes, seconds = divmod(remainder, 60)
        self.stats["uptime_formatted"] = f"{hours:02d}:{minutes:02d}:{seconds:02d}"

        return self.stats.copy()

    async def cleanup_old_chunks(self):
        """Limpia buffers de chunking viejos periódicamente"""
        while True:
            try:
                await asyncio.sleep(CHUNK_CLEANUP_INTERVAL)

                now = time.time()
                to_remove = []

                for client_id, metadata in self.chunk_metadata.items():
                    if now - metadata["start_time"] > CHUNK_TIMEOUT:
                        to_remove.append(client_id)
                        logger.warning(f"⏱️ Timeout de chunking para {client_id}")

                for client_id in to_remove:
                    self._cleanup_client_buffers(client_id)
                    self.stats["frames_failed"] += 1

            except Exception as e:
                logger.error(f"❌ Error en cleanup de chunks: {e}")

    async def process_command_queue(self):
        """Procesa comandos de la cola cuando hay cámara conectada"""
        while True:
            try:
                await asyncio.sleep(0.1)  # Check cada 100ms

                if self.camera_client and self.command_queue.size() > 0:
                    command = await self.command_queue.get()
                    if command:
                        try:
                            await self.camera_client.send(json.dumps(command))
                            cmd_name = command.get("cmd", "unknown")
                            logger.info(f"✅ Comando enviado desde cola: {cmd_name}")
                        except Exception as e:
                            logger.error(f"❌ Error enviando comando desde cola: {e}")

            except Exception as e:
                logger.error(f"❌ Error en process_command_queue: {e}")

    async def handle_websocket(self, websocket):
        """Maneja conexiones WebSocket entrantes"""
        client_ip = websocket.remote_address[0]
        client_port = websocket.remote_address[1]
        client_id = f"{client_ip}:{client_port}"

        logger.info(f"🔌 Nueva conexión: {client_id}")

        # Limpiar buffers anteriores
        self._cleanup_client_buffers(client_id)

        try:
            async for message in websocket:
                # Mensaje binario (frame o chunk)
                if isinstance(message, bytes):
                    await self._handle_binary_message(
                        message, websocket, client_id, client_ip
                    )
                # Mensaje de texto (JSON)
                else:
                    await self._handle_text_message(
                        message, websocket, client_id, client_ip
                    )

        except websockets.exceptions.ConnectionClosed:
            logger.info(f"🔌 Conexión cerrada: {client_id}")

        except Exception as e:
            logger.error(f"❌ Error en conexión {client_id}: {e}")

        finally:
            await self._cleanup_connection(websocket, client_id)

    async def _handle_binary_message(
        self, message: bytes, websocket, client_id: str, client_ip: str
    ):
        """Maneja mensajes binarios (frames o chunks)"""
        try:
            # Modo chunking activo
            if client_id in self.chunk_metadata:
                metadata = self.chunk_metadata[client_id]
                self.chunk_buffers[client_id].extend(message)
                metadata["received"] += len(message)

                # Log cada 20% de progreso para imágenes grandes
                progress = (metadata["received"] / metadata["size"]) * 100
                if progress % 20 < 5:  # Aproximadamente cada 20%
                    logger.debug(
                        f"📦 Progreso chunking: {progress:.0f}% ({metadata['received']}/{metadata['size']} bytes)"
                    )

                # Verificar si se completó la imagen
                if metadata["received"] >= metadata["size"]:
                    full_image = bytes(
                        self.chunk_buffers[client_id][: metadata["size"]]
                    )
                    await self._process_complete_image(
                        full_image, websocket, client_id, client_ip
                    )

                    # Limpiar buffers
                    self._cleanup_client_buffers(client_id)

            # Modo normal: frame completo
            else:
                # Validar JPEG
                if len(message) > 2 and message[0] == 0xFF and message[1] == 0xD8:
                    await self._process_complete_image(
                        message, websocket, client_id, client_ip
                    )
                else:
                    logger.warning(
                        f"⚠️ Datos binarios no válidos (no JPEG), tamaño: {len(message)}"
                    )
                    self.stats["frames_failed"] += 1

        except Exception as e:
            logger.error(f"❌ Error procesando mensaje binario: {e}")
            self.stats["frames_failed"] += 1
            self._cleanup_client_buffers(client_id)

    async def _handle_text_message(
        self, message: str, websocket, client_id: str, client_ip: str
    ):
        """Maneja mensajes de texto JSON"""
        try:
            data = json.loads(message)
            msg_type = data.get("type", "")

            # Registrar dispositivo
            if msg_type == "register":
                await self._handle_register(data, websocket, client_id, client_ip)

            # Inicio de chunking
            elif msg_type == "img_start":
                await self._handle_img_start(data, websocket, client_id)

            # Fin de chunking
            elif msg_type == "img_end":
                await self._handle_img_end(data, client_id)

            # Comando para cámara
            elif msg_type == "command":
                await self._handle_command(data, websocket)

            # Health check
            elif msg_type == "health":
                await self._handle_health(data)

            # Solicitar estadísticas
            elif msg_type == "request_stats":
                await websocket.send(
                    json.dumps(
                        {
                            "type": "server_stats",
                            "data": self.get_stats(),
                            "timestamp": time.time(),
                        }
                    )
                )

        except json.JSONDecodeError:
            logger.warning(f"⚠️ JSON inválido: {message[:100]}")
        except Exception as e:
            logger.error(f"❌ Error procesando mensaje: {e}")

    async def _handle_register(
        self, data: dict, websocket, client_id: str, client_ip: str
    ):
        """Maneja registro de dispositivos"""
        device = data.get("device", "")

        if device == "camera":
            self.camera_client = websocket
            self.stats["connected"] = True
            self.stats["camera_ip"] = client_ip
            logger.info(f"📷 Cámara registrada: {client_id}")

            await websocket.send(
                json.dumps(
                    {
                        "status": "ok",
                        "message": "Camera registered",
                        "server_time": datetime.now().isoformat(),
                    }
                )
            )

            # Broadcast status a navegadores
            await self._broadcast_to_browsers(
                json.dumps(
                    {"type": "status", "camera_connected": True, "stats": self.stats}
                )
            )

            # Procesar comandos pendientes en la cola
            pending = self.command_queue.size()
            if pending > 0:
                logger.info(f"📋 Procesando {pending} comandos pendientes...")

        elif device == "browser":
            self.browser_clients.add(websocket)
            logger.info(f"🌐 Navegador registrado: {client_id}")

            # Enviar último frame si existe
            with self.frame_lock:
                if self.latest_frame:
                    await websocket.send(self.latest_frame)

            # Enviar estado actual
            await websocket.send(
                json.dumps(
                    {
                        "type": "status",
                        "camera_connected": self.stats["connected"],
                        "stats": self.stats,
                    }
                )
            )

    async def _handle_img_start(self, data: dict, websocket, client_id: str):
        """Maneja inicio de transferencia chunked"""
        size = data.get("size", 0)
        chunks = data.get("chunks", 0)

        if size > 0 and chunks > 0:
            # Limpiar buffer anterior si existe
            if client_id in self.chunk_buffers:
                logger.warning(f"⚠️ Limpiando buffer previo para {client_id}")
                self._cleanup_client_buffers(client_id)

            self.chunk_buffers[client_id] = bytearray()
            self.chunk_metadata[client_id] = {
                "size": size,
                "chunks": chunks,
                "received": 0,
                "start_time": time.time(),
            }

            self.stats["frames_chunked"] += 1
            logger.info(f"📦 Iniciando chunking: {size/1024:.1f}KB en {chunks} chunks")

            await websocket.send(
                json.dumps(
                    {"type": "img_ack", "status": "ready", "timestamp": time.time()}
                )
            )

    async def _handle_img_end(self, data: dict, client_id: str):
        """Maneja fin de transferencia chunked"""
        if client_id in self.chunk_metadata:
            size = data.get("size", 0)
            success = data.get("success", False)

            if success:
                logger.info(f"✅ Transferencia chunked completada: {size/1024:.1f}KB")
            else:
                logger.warning(f"⚠️ Transferencia chunked fallida")
                self._cleanup_client_buffers(client_id)
                self.stats["frames_failed"] += 1

    async def _handle_command(self, data: dict, websocket):
        """Maneja comandos para la cámara con sistema de prioridades"""
        cmd = data.get("cmd", "").lower()
        val = data.get("val", "")

        # Convertir valor a string si es numérico
        if isinstance(val, (int, float)):
            val = str(val)

        # Obtener prioridad del comando
        priority = PRIORITY_NORMAL
        if cmd in COMMANDS:
            priority = COMMANDS[cmd].get("priority", PRIORITY_NORMAL)

        command = {"type": "command", "cmd": cmd, "val": val, "timestamp": time.time()}

        # Si es comando crítico (reboot), logearlo especialmente
        if priority == PRIORITY_CRITICAL:
            logger.warning(f"🔴 COMANDO CRÍTICO: {cmd}")

        # Si hay cámara conectada, enviar inmediatamente si es crítico
        if self.camera_client and self.camera_client != websocket:
            if priority == PRIORITY_CRITICAL:
                # Comandos críticos se envían INMEDIATAMENTE
                try:
                    await self.camera_client.send(json.dumps(command))
                    logger.warning(f"🔴 Comando crítico enviado inmediatamente: {cmd}")
                except Exception as e:
                    logger.error(f"❌ Error enviando comando crítico: {e}")
                    # Si falla, encolar de todas formas
                    await self.command_queue.add(command, priority)
            else:
                # Comandos normales se envían directamente
                try:
                    await self.camera_client.send(json.dumps(command))
                    logger.info(f"⚡ Comando enviado: {cmd}={val}")
                except Exception as e:
                    logger.error(f"❌ Error enviando comando: {e}")
                    await self.command_queue.add(command, priority)
        else:
            # No hay cámara, encolar con prioridad
            await self.command_queue.add(command, priority)
            if priority == PRIORITY_CRITICAL:
                logger.warning(f"🔴 Comando crítico encolado (sin cámara): {cmd}")
            else:
                logger.info(f"📋 Comando encolado: {cmd}={val} (prioridad: {priority})")

    async def _handle_health(self, data: dict):
        """Maneja health check de la cámara"""
        logger.info(f"💚 Health check: Frames={data.get('frames', 0)}")

        # Broadcast a navegadores
        health_msg = json.dumps(
            {
                "type": "camera_health",
                "data": data,
                "server_time": datetime.now().isoformat(),
            }
        )

        await self._broadcast_to_browsers(health_msg)

    async def _process_complete_image(
        self, image_data: bytes, websocket, client_id: str, client_ip: str
    ):
        """Procesa una imagen JPEG completa"""
        try:
            # Validar que sea JPEG completo
            if len(image_data) < 100:
                logger.warning(f"⚠️ Imagen demasiado pequeña: {len(image_data)} bytes")
                self.stats["frames_failed"] += 1
                return

            # Verificar marcadores JPEG
            if image_data[0] != 0xFF or image_data[1] != 0xD8:
                logger.warning("⚠️ No tiene SOI (Start of Image)")
                self.stats["frames_failed"] += 1
                return

            # Buscar EOI (End of Image)
            has_eoi = False
            for i in range(len(image_data) - 2, max(0, len(image_data) - 100), -1):
                if image_data[i] == 0xFF and image_data[i + 1] == 0xD9:
                    has_eoi = True
                    break

            if not has_eoi:
                logger.warning("⚠️ No tiene EOI (End of Image)")
                self.stats["frames_failed"] += 1
                return

            # Auto-registrar cámara si no está registrada
            if self.camera_client != websocket:
                self.camera_client = websocket
                self.stats["connected"] = True
                self.stats["camera_ip"] = client_ip
                logger.info(f"📷 Cámara auto-registrada: {client_id}")

            # Actualizar estadísticas
            self.stats["frames_received"] += 1
            self.stats["total_bytes"] += len(image_data)
            self.stats["fps"] = self.fps_counter.tick()
            self.stats["last_frame_time"] = datetime.now().isoformat()

            # Guardar frame más reciente
            with self.frame_lock:
                self.latest_frame = image_data

            # Guardar imagen SOLO SI está habilitado
            if image_saver.enabled:
                try:
                    saved_path = image_saver.save_image(image_data)
                    if saved_path:
                        logger.debug(f"💾 Imagen guardada: {saved_path.name}")
                except Exception as e:
                    logger.error(f"❌ Error guardando imagen: {e}")

            # Broadcast a navegadores
            await self._broadcast_to_browsers(image_data)

            # Log periódico solo cada 50 frames (menos spam)
            if self.stats["frames_received"] % 50 == 0:
                logger.info(
                    f"📊 Frames={self.stats['frames_received']} | "
                    f"Failed={self.stats['frames_failed']} | "
                    f"FPS={self.stats['fps']} | "
                    f"Size={len(image_data)/1024:.1f}KB | "
                    f"Browsers={len(self.browser_clients)}"
                )

        except Exception as e:
            logger.error(f"❌ Error procesando imagen: {e}")
            self.stats["frames_failed"] += 1

    async def _broadcast_to_browsers(self, data):
        """Envía datos a todos los navegadores conectados"""
        disconnected = []

        for browser in list(self.browser_clients):
            try:
                await browser.send(data)
            except Exception:
                disconnected.append(browser)

        # Limpiar desconectados
        for client in disconnected:
            self.browser_clients.discard(client)

    def _cleanup_client_buffers(self, client_id: str):
        """Limpia buffers de chunking para un cliente"""
        if client_id in self.chunk_buffers:
            del self.chunk_buffers[client_id]
        if client_id in self.chunk_metadata:
            del self.chunk_metadata[client_id]

    async def _cleanup_connection(self, websocket, client_id: str):
        """Limpia recursos al desconectar"""
        # Limpiar buffers
        self._cleanup_client_buffers(client_id)

        # Limpiar registro
        if websocket == self.camera_client:
            logger.warning("📷 Cámara desconectada")
            self.camera_client = None
            self.stats["connected"] = False
            self.stats["camera_ip"] = None

            # Broadcast status a navegadores
            await self._broadcast_to_browsers(
                json.dumps(
                    {"type": "status", "camera_connected": False, "stats": self.stats}
                )
            )

        if websocket in self.browser_clients:
            self.browser_clients.discard(websocket)
            logger.info(
                f"🌐 Navegador desconectado (activos: {len(self.browser_clients)})"
            )

    async def start_server(self):
        """Inicia el servidor WebSocket"""
        logger.info(f"🚀 WebSocket Server iniciando en ws://0.0.0.0:{WS_PORT}")
        logger.info(f"📦 MAX_FRAME_SIZE: {MAX_FRAME_SIZE/1024/1024:.1f}MB")

        # Iniciar task de limpieza de chunks
        self.cleanup_task = asyncio.create_task(self.cleanup_old_chunks())

        # Iniciar task de procesamiento de cola de comandos
        queue_task = asyncio.create_task(self.process_command_queue())

        async with websockets.serve(
            self.handle_websocket,
            "0.0.0.0",
            WS_PORT,
            ping_interval=WS_PING_INTERVAL,
            ping_timeout=WS_PING_TIMEOUT,
            max_size=MAX_FRAME_SIZE,
        ):
            await asyncio.Future()  # Ejecutar indefinidamente


# Instancia global
camera_server = CameraWebSocketServer()
