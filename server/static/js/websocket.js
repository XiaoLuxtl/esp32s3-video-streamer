// websocket.js - CORREGIDO
import { notification } from "./notifications.js";

const DEBUG = false;

export class WebSocketManager {
  constructor() {
    this.ws = null;
    this.cameraConnected = false;
    this.frameCount = 0;
    this.lastFrameTime = Date.now();
    this.reconnectTimeout = null;
    this.messageHandlers = new Map();

    // Inicializar contador FPS
    this.initFpsCounter();
    this.registerDefaultHandlers();
  }

  gatherElements() {
    const elements = {
      stream: document.getElementById("videoStream"),
      statusDot: document.getElementById("statusDot"),
      statusText: document.getElementById("statusText"),
      connectionTime: document.getElementById("connectionTime"),
      fpsCounter: document.getElementById("fpsCounter"),
      currentResHeader: document.getElementById("currentResHeader"),
      latencyHeader: document.getElementById("latencyHeader"),
      cameraIpHeader: document.getElementById("cameraIpHeader"),
      noStreamOverlay: document.getElementById("videoOverlay"),
      videoFps: document.getElementById("videoFps"),
      videoQuality: document.getElementById("videoQuality"),
      cameraIpSidebar: document.getElementById("cameraIpSidebar"),
      latencySidebar: document.getElementById("latency"), // En sidebar es 'latency'
    };
    if (DEBUG) console.log("Gathered elements:", elements);
    return elements;
  }

  registerDefaultHandlers() {
    this.on("status", this.handleStatusMessage.bind(this));
    this.on("camera_health", this.handleCameraHealth.bind(this));
    this.on("server_stats", this.handleServerStats.bind(this));
  }

  updateHeaderStats(cameraData) {
    // Update camera IP
    if (this.elements.cameraIpHeader && cameraData.ip) {
      this.elements.cameraIpHeader.textContent = cameraData.ip;
    }

    // Update camera model
    if (this.elements.cameraModel && cameraData.model) {
      this.elements.cameraModel.textContent = cameraData.model;
    }

    // Update resolution
    if (this.elements.currentResHeader && cameraData.resolution) {
      this.elements.currentResHeader.textContent = cameraData.resolution;
    }

    // Update latency
    if (this.elements.latencyHeader && cameraData.latency !== undefined) {
      this.elements.latencyHeader.textContent = `${cameraData.latency} ms`;
    }
  }

  updateServerStats(serverData) {
    // Update server uptime
    if (this.elements.serverUptimeHeader && serverData.uptime !== undefined) {
      this.elements.serverUptimeHeader.textContent = this.formatUptime(serverData.uptime);
    }

    // Update memory usage
    if (this.elements.memoryUsage && serverData.memory !== undefined) {
      this.elements.memoryUsage.textContent = `${serverData.memory}%`;
    }

    // Update connected clients
    if (this.elements.connectedClients && serverData.clients !== undefined) {
      this.elements.connectedClients.textContent = serverData.clients;
    }
  }

  on(event, handler) {
    if (!this.messageHandlers.has(event)) {
      this.messageHandlers.set(event, []);
    }
    this.messageHandlers.get(event).push(handler);
  }

  emit(event, data) {
    const handlers = this.messageHandlers.get(event);
    if (handlers) {
      handlers.forEach((handler) => handler(data));
    }
  }

  connect() {
    // Asegurar que elements estén disponibles
    if (!this.elements) {
      this.elements = this.gatherElements();
    }
    if (DEBUG) console.log("Conectando a WebSocket:", window.APP_CONFIG.WS_URL);

    // Cerrar conexión existente
    if (this.ws) {
      this.ws.close();
    }

    try {
      this.ws = new WebSocket(window.APP_CONFIG.WS_URL);

      this.ws.onopen = () => {
        if (DEBUG) console.log("✓ WebSocket conectado");
        this.updateConnectionStatus("connecting", "CONECTANDO...");

        // Registrarse como navegador
        this.send({
          type: "register",
          device: "browser",
          timestamp: Date.now(),
        });

        notification.show("🔗 WebSocket conectado", "success");

        // Simular status para pruebas (remover en producción)
        setTimeout(() => {
          this.handleMessage({ type: "status", camera_connected: true });
        }, 1000);
      };

      this.ws.onclose = () => {
        if (DEBUG) console.log("✗ WebSocket desconectado");
        this.updateConnectionStatus("disconnected", "DESCONECTADO");
        this.cameraConnected = false;

        // Ocultar video, mostrar overlay
        if (this.elements.stream) {
          this.elements.stream.style.display = "none";
        }
        if (this.elements.noStreamOverlay) {
          this.elements.noStreamOverlay.style.display = "flex";
        }

        // Reconectar después de 3 segundos
        clearTimeout(this.reconnectTimeout);
        this.reconnectTimeout = setTimeout(() => {
          this.connect();
        }, 3000);
      };

      this.ws.onerror = (error) => {
        console.error("WebSocket error:", error);
        notification.show("❌ Error de conexión WebSocket", "error");
      };

      this.ws.onmessage = (event) => {
        if (DEBUG)
          console.log(
            "WebSocket message received, type:",
            typeof event.data,
            event.data instanceof Blob
              ? "Blob"
              : event.data instanceof ArrayBuffer
                ? "ArrayBuffer"
                : "Other"
          );
        // Mensaje binario (frame JPEG)
        if (event.data instanceof Blob) {
          this.processImageFrame(event.data);
        } else if (event.data instanceof ArrayBuffer) {
          // Convertir ArrayBuffer a Blob
          const blob = new Blob([event.data], { type: "image/jpeg" });
          this.processImageFrame(blob);
        }
        // Mensaje de texto (JSON)
        else if (typeof event.data === "string") {
          try {
            const data = JSON.parse(event.data);
            this.handleMessage(data);
          } catch (e) {
            console.error("Error parsing JSON:", e);
          }
        }
      };
    } catch (error) {
      console.error("Error creating WebSocket:", error);
      notification.show("❌ Error creando WebSocket", "error");
      this.reconnect();
    }
  }

  processImageFrame(blob) {
    if (DEBUG) {
      if (DEBUG) console.log("Processing image frame, size:", blob.size);
      if (DEBUG) console.log("Stream element:", this.elements.stream);
      if (DEBUG) console.log("Overlay element:", this.elements.noStreamOverlay);
    }

    // Crear URL para el blob
    const url = URL.createObjectURL(blob);
    if (DEBUG) console.log("Blob URL:", url);

    // Liberar URL anterior si existe
    if (this.elements.stream && this.elements.stream.src) {
      URL.revokeObjectURL(this.elements.stream.src);
    }

    // Actualizar imagen
    if (this.elements.stream) {
      this.elements.stream.src = url;
      this.elements.stream.style.display = "block";
      if (DEBUG) console.log("Image src set to:", url);
      if (DEBUG)
        console.log("Image style.display:", this.elements.stream.style.display);
    } else {
      console.error("Stream element not found!");
    }

    // Ocultar overlay cuando hay frames
    if (this.elements.noStreamOverlay) {
      this.elements.noStreamOverlay.style.display = "none";
      if (DEBUG) console.log("Overlay hidden");
    } else {
      console.error("Overlay element not found!");
    }

    // Simular estado conectado si no lo está
    if (!this.cameraConnected) {
      this.cameraConnected = true;
      this.updateConnectionStatus("connected", "CÁMARA CONECTADA");
    }

    // Incrementar contador de frames
    this.frameCount++;
  }

  handleMessage(data) {
    if (DEBUG) console.log("Mensaje recibido:", data.type);

    switch (data.type) {
      case "status":
        this.emit("status", data);
        break;
      case "camera_health":
        this.emit("camera_health", data.data);
        break;
      case "server_stats":
        this.emit("server_stats", data.data);
        break;
      default:
        if (DEBUG) console.log("Mensaje no manejado:", data.type);
    }
  }

  updateConnectionStatus(status, message) {
    if (!this.elements.statusDot || !this.elements.statusText) return;

    this.elements.statusDot.className = "status-dot status-" + status;
    this.elements.statusText.textContent = message;

    // No cambiar clase de texto, usar CSS
  }

  handleStatusMessage(data) {
    this.cameraConnected = data.camera_connected;

    if (data.camera_connected) {
      this.updateConnectionStatus("connected", "CÁMARA CONECTADA");
      if (this.elements.noStreamOverlay) {
        this.elements.noStreamOverlay.style.display = "none";
      }
    }
  }

  handleCameraHealth(health) {
    // Actualizar elementos de salud si existen
    const elements = {
      cameraFrames: document.getElementById("cameraFrames"),
      cameraHeap: document.getElementById("cameraHeap"),
      cameraUptime: document.getElementById("cameraUptime"),
    };

    if (elements.cameraFrames)
      elements.cameraFrames.textContent = health.frames || 0;
    if (elements.cameraHeap && health.heap) {
      elements.cameraHeap.textContent = `${(health.heap / 1024).toFixed(1)} KB`;
    }
    if (elements.cameraUptime && health.uptime) {
      elements.cameraUptime.textContent = this.formatUptime(health.uptime);
    }

    // Update header with camera data
    this.updateHeaderStats({
      ip: health.ip,
      model: health.model || "OV3660",
      resolution: health.resolution,
      latency: health.latency
    });

    // Update sidebar IP if exists
    if (this.elements.cameraIpSidebar && health.ip) {
      this.elements.cameraIpSidebar.textContent = health.ip;
    }

    // Update Quality overlay on video feed
    if (this.elements.videoQuality && health.quality !== undefined) {
      this.elements.videoQuality.textContent = health.quality;
    }
  }

  handleServerStats(stats) {
    // Actualizar elementos de estadísticas si existen
    const elements = {
      serverFrames: document.getElementById("serverFrames"),
      serverData: document.getElementById("serverData"),
      serverFps: document.getElementById("serverFps"),
      serverClients: document.getElementById("serverClients"),
      latency: document.getElementById("latency"),
    };

    if (elements.serverFrames)
      elements.serverFrames.textContent = stats.frames_received || 0;
    if (elements.serverData && stats.total_bytes) {
      elements.serverData.textContent = `${(
        stats.total_bytes /
        1024 /
        1024
      ).toFixed(2)} MB`;
    }
    if (elements.serverFps) elements.serverFps.textContent = stats.fps || 0;
    if (elements.serverClients)
      elements.serverClients.textContent = stats.clients || 1;
    if (elements.latency && stats.latency !== undefined) {
      elements.latency.textContent = `${stats.latency} ms`;
    }

    // Sync sidebar latency redundant element if exists
    if (this.elements.latencySidebar && stats.latency !== undefined) {
      this.elements.latencySidebar.textContent = `${stats.latency} ms`;
    }

    // Update header server stats
    this.updateServerStats({
      uptime: stats.uptime,
      memory: stats.memory,
      clients: stats.clients || 1
    });
  }

  formatUptime(seconds) {
    if (!seconds) return "--";

    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = Math.floor(seconds % 60);

    return `${hours.toString().padStart(2, "0")}:${minutes
      .toString()
      .padStart(2, "0")}:${secs.toString().padStart(2, "0")}`;
  }

  send(data) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      if (typeof data === "object") {
        this.ws.send(JSON.stringify(data));
      } else {
        this.ws.send(data);
      }
      return true;
    }
    return false;
  }

  initFpsCounter() {
    setInterval(() => {
      const now = Date.now();
      const fps = Math.round(
        (this.frameCount * 1000) / (now - this.lastFrameTime)
      );

      if (this.elements.fpsCounter) this.elements.fpsCounter.textContent = fps;
      if (this.elements.videoFps) this.elements.videoFps.textContent = fps;

      this.frameCount = 0;
      this.lastFrameTime = now;
    }, 1000);
  }

  get isConnected() {
    return this.ws && this.ws.readyState === WebSocket.OPEN;
  }

  disconnect() {
    if (this.ws) {
      this.ws.close();
    }
    if (this.reconnectTimeout) {
      clearTimeout(this.reconnectTimeout);
    }
  }

  reconnect() {
    clearTimeout(this.reconnectTimeout);
    this.reconnectTimeout = setTimeout(() => {
      if (DEBUG) console.log("🔄 Intentando reconexión...");
      this.connect();
    }, 3000);
  }
}

// Exportar instancia única
export const wsManager = new WebSocketManager();

export default wsManager;
