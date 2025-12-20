// static/js/app.js - Versión mejorada con UI/UX profesional
import { wsManager } from "./websocket.js";
import { cameraControls } from "./controls.js";
import { imageSaver } from "./image-saver.js";
import { notification } from "./notifications.js";

const DEBUG = false;

class CameraApp {
  constructor() {
    this.statsInterval = null;
    this.latency = 0;
    this.frameDrops = 0;
    this.connectionStart = null;
    this.isFullscreen = false;
    this.isStreamFrozen = false;
    this.frozenFrame = null;

    this.init();
  }

  init() {
    if (DEBUG) console.log("🚀 ESP32 Vision v2.1 - Inicializando");

    // Inicializar iconos de Lucide
    if (typeof lucide !== "undefined") {
      lucide.createIcons();
      if (DEBUG) console.log("✓ Lucide Icons inicializados");
    }

    // Ocultar loading overlay
    this.hideLoadingOverlay();

    // Mostrar información del sistema
    this.showSystemInfo();

    // Configurar event listeners globales
    this.setupGlobalListeners();

    // Inicializar temas
    this.initTheme();

    // Inicializar WebSocket DESPUÉS de configurar listeners
    setTimeout(() => {
      wsManager.connect();
    }, 100);

    // Iniciar monitoreo de rendimiento
    this.startPerformanceMonitoring();

    // Verificar compatibilidad
    this.checkCompatibility();
  }

  hideLoadingOverlay() {
    const loadingOverlay = document.getElementById("loadingOverlay");
    if (loadingOverlay) {
      setTimeout(() => {
        loadingOverlay.style.opacity = "0";
        setTimeout(() => {
          loadingOverlay.style.display = "none";
          if (DEBUG) console.log("✓ Loading overlay oculto");
        }, 300);
      }, 1000);
    }
  }

  showSystemInfo() {
    if (DEBUG) console.log("🌐 WebSocket:", window.APP_CONFIG.WS_URL);
    if (DEBUG) console.log("🔗 API:", window.APP_CONFIG.API_BASE);
    if (DEBUG) console.log("📱 User Agent:", navigator.userAgent);
    if (DEBUG) console.log("💻 Plataforma:", navigator.platform);

    // Mostrar en UI si hay elementos para ello
    const systemInfo = document.getElementById("system-info");
    if (systemInfo) {
      systemInfo.textContent = `${
        navigator.platform
      } | ${this.getBrowserName()}`;
    }
  }

  getBrowserName() {
    const userAgent = navigator.userAgent;
    if (userAgent.includes("Chrome")) return "Chrome";
    if (userAgent.includes("Firefox")) return "Firefox";
    if (userAgent.includes("Safari")) return "Safari";
    if (userAgent.includes("Edge")) return "Edge";
    return "Navegador";
  }

  setupGlobalListeners() {
    // Pantalla completa
    document.addEventListener("fullscreenchange", () => {
      this.isFullscreen = !this.isFullscreen;
    });

    // Teclas rápidas
    document.addEventListener("keydown", (e) => {
      this.handleKeyboardShortcuts(e);
    });

    // Conexión WebSocket
    wsManager.on("status", (data) => {
      this.handleConnectionStatus(data);
    });

    wsManager.on("camera_health", (data) => {
      this.updateCameraHealth(data);
    });

    wsManager.on("server_stats", (data) => {
      this.updateServerStats(data);
    });

    // Detectar cierre de página
    window.addEventListener("beforeunload", () => {
      this.cleanup();
    });

    // Mobile restart button
    const mobileRestartBtn = document.getElementById("mobileRestartBtn");
    if (mobileRestartBtn) {
      mobileRestartBtn.addEventListener("click", () => {
        this.rebootCamera();
      });
    }
  }

  handleKeyboardShortcuts(e) {
    // Ignorar si está en un campo de entrada
    if (e.target.tagName === "INPUT" || e.target.tagName === "TEXTAREA") {
      return;
    }

    switch (e.key.toLowerCase()) {
      case "f":
        this.toggleFullscreen();
        break;
      case " ":
        e.preventDefault();
        this.toggleFreezeFrame();
        break;
      case "c":
        cameraControls.captureImage();
        break;
      case "s":
        imageSaver.manualSave();
        break;
      case "r":
        this.refreshAll();
        break;
      case "escape":
        if (this.isFullscreen) {
          document.exitFullscreen();
        }
        break;
    }
  }

  toggleFullscreen() {
    const videoContainer =
      document.querySelector(".video-container") ||
      document.querySelector(".video-section");

    if (!this.isFullscreen) {
      if (videoContainer.requestFullscreen) {
        videoContainer.requestFullscreen();
      } else if (videoContainer.webkitRequestFullscreen) {
        videoContainer.webkitRequestFullscreen();
      } else if (videoContainer.msRequestFullscreen) {
        videoContainer.msRequestFullscreen();
      }
      notification.show("🖥️ Pantalla completa", "info");
    } else {
      if (document.exitFullscreen) {
        document.exitFullscreen();
      } else if (document.webkitExitFullscreen) {
        document.webkitExitFullscreen();
      } else if (document.msExitFullscreen) {
        document.msExitFullscreen();
      }
      notification.show("📱 Pantalla normal", "info");
    }
  }

  toggleFreezeFrame() {
    const stream = document.getElementById("videoStream");
    const freezeBtn = document.getElementById("freezeFrameBtn");

    if (!this.isStreamFrozen) {
      // Congelar
      this.frozenFrame = stream.src;
      this.isStreamFrozen = true;

      if (freezeBtn) {
        const icon = freezeBtn.querySelector("i");
        if (icon) {
          icon.setAttribute("data-lucide", "play");
          lucide.createIcons();
        }
      }

      notification.show("⏸️ Video pausado", "info");
    } else {
      // Reanudar
      this.isStreamFrozen = false;
      this.frozenFrame = null;

      if (freezeBtn) {
        const icon = freezeBtn.querySelector("i");
        if (icon) {
          icon.setAttribute("data-lucide", "pause");
          lucide.createIcons();
        }
      }

      notification.show("▶️ Video reanudado", "info");
    }
  }

  toggleZoom() {
    const stream = document.getElementById("videoStream");

    if (stream.classList.contains("zoom-100")) {
      stream.classList.remove("zoom-100");
      stream.style.objectFit = "contain";
      notification.show("🔍 Zoom normal", "info");
    } else {
      stream.classList.add("zoom-100");
      stream.style.objectFit = "none";
      notification.show("🔎 Zoom 100%", "info");
    }
  }

  handleConnectionStatus(data) {
    const overlay = document.getElementById("noStreamOverlay");
    const statusDot = document.getElementById("statusDot");
    const statusRing = document.getElementById("statusRing");
    const statusText = document.getElementById("statusText");
    const cameraStateBadge = document.getElementById("camera-state-badge");

    if (data.camera_connected) {
      // Cámara conectada
      if (overlay) overlay.style.display = "none";

      if (statusDot) {
        statusDot.className = "status-dot status-connected";
      }

      if (statusRing) {
        statusRing.style.opacity = "0.75";
      }

      if (statusText) {
        statusText.textContent = "CÁMARA CONECTADA";
        statusText.style.color = "var(--success)";
      }

      if (cameraStateBadge) {
        cameraStateBadge.textContent = "ONLINE";
        cameraStateBadge.className = "stat-badge badge-online";
      }

      // Iniciar temporizador de conexión
      if (!this.connectionStart) {
        this.connectionStart = Date.now();
        this.startConnectionTimer();
      }
    } else if (data.server_connected) {
      // Servidor conectado, esperando cámara
      if (overlay) overlay.style.display = "flex";

      if (statusDot) {
        statusDot.className = "status-dot status-connecting";
      }

      if (statusRing) {
        statusRing.style.opacity = "0.75";
      }

      if (statusText) {
        statusText.textContent = "ESPERANDO CÁMARA";
        statusText.style.color = "var(--warning)";
      }
    } else {
      // Desconectado
      if (overlay) overlay.style.display = "flex";

      if (statusDot) {
        statusDot.className = "status-dot status-disconnected";
      }

      if (statusRing) {
        statusRing.style.opacity = "0";
      }

      if (statusText) {
        statusText.textContent = "DESCONECTADO";
        statusText.style.color = "var(--danger)";
      }

      if (cameraStateBadge) {
        cameraStateBadge.textContent = "OFFLINE";
        cameraStateBadge.className = "stat-badge badge-offline";
      }

      this.connectionStart = null;
    }
  }

  startConnectionTimer() {
    const timerElement = document.getElementById("connectionTime");
    const durationElement = document.getElementById("connectionDuration");

    if (!timerElement && !durationElement) return;

    const updateTimer = () => {
      if (!this.connectionStart) return;

      const duration = Date.now() - this.connectionStart;
      const hours = Math.floor(duration / 3600000);
      const minutes = Math.floor((duration % 3600000) / 60000);
      const seconds = Math.floor((duration % 60000) / 1000);

      const timeString = `${hours.toString().padStart(2, "0")}:${minutes
        .toString()
        .padStart(2, "0")}:${seconds.toString().padStart(2, "0")}`;

      if (timerElement) timerElement.textContent = timeString;
      if (durationElement) durationElement.textContent = timeString;
    };

    setInterval(updateTimer, 1000);
    updateTimer();
  }

  updateCameraHealth(health) {
    // Actualizar elementos de salud de la cámara
    const elements = {
      cameraFrames: document.getElementById("cameraFrames"),
      cameraHeap: document.getElementById("cameraHeap"),
      cameraUptime: document.getElementById("cameraUptime"),
      cameraIp: document.getElementById("cameraIp"),
      cameraIpHeader: document.getElementById("cameraIpHeader"),
    };

    if (elements.cameraFrames && health.frames) {
      elements.cameraFrames.textContent = health.frames.toLocaleString();
    }

    if (elements.cameraHeap && health.heap) {
      const heapKB = (health.heap / 1024).toFixed(1);
      elements.cameraHeap.textContent = `${heapKB} KB`;
    }

    if (elements.cameraUptime && health.uptime) {
      elements.cameraUptime.textContent = this.formatUptime(health.uptime);
    }

    // Actualizar IP en múltiples lugares
    if (health.ip) {
      if (elements.cameraIp) elements.cameraIp.textContent = health.ip;
      if (elements.cameraIpHeader)
        elements.cameraIpHeader.textContent = health.ip;
    }
  }

  updateServerStats(stats) {
    // Actualizar elementos del servidor
    const elements = {
      serverFrames: document.getElementById("serverFrames"),
      serverData: document.getElementById("serverData"),
      serverFps: document.getElementById("serverFps"),
      serverUptime: document.getElementById("serverUptime"),
      serverClients: document.getElementById("serverClients"),
      fpsCounter: document.getElementById("fpsCounter"),
      videoFps: document.getElementById("videoFps"),
      currentResHeader: document.getElementById("currentResHeader"),
      latencyHeader: document.getElementById("latencyHeader"),
    };

    if (elements.serverFrames && stats.frames_received) {
      elements.serverFrames.textContent =
        stats.frames_received.toLocaleString();
    }

    if (elements.serverData && stats.total_bytes) {
      const totalMB = (stats.total_bytes / (1024 * 1024)).toFixed(2);
      elements.serverData.textContent = `${totalMB} MB`;
    }

    if (elements.serverFps && stats.fps) {
      elements.serverFps.textContent = stats.fps;
      if (elements.fpsCounter) elements.fpsCounter.textContent = stats.fps;
      if (elements.videoFps) elements.videoFps.textContent = stats.fps;
    }

    if (elements.serverUptime && stats.uptime) {
      elements.serverUptime.textContent = this.formatUptime(stats.uptime);
    }

    if (elements.serverClients && stats.clients) {
      elements.serverClients.textContent = stats.clients;
    }

    // Actualizar uso de memoria si está disponible
    if (stats.memory_usage) {
      const memoryElement = document.getElementById("memory-usage");
      if (memoryElement) {
        memoryElement.textContent = `${stats.memory_usage}%`;
        memoryElement.className = `info-value ${this.getPerformanceClass(
          stats.memory_usage,
          80,
          90
        )}`;
      }
    }

    // Actualizar latencia
    if (stats.latency) {
      this.updateLatency(stats.latency);
    }
  }

  formatUptime(seconds) {
    if (!seconds) return "--:--:--";

    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = Math.floor(seconds % 60);

    return `${hours.toString().padStart(2, "0")}:${minutes
      .toString()
      .padStart(2, "0")}:${secs.toString().padStart(2, "0")}`;
  }

  getPerformanceClass(value, warningThreshold, dangerThreshold) {
    if (value >= dangerThreshold) return "perf-poor";
    if (value >= warningThreshold) return "perf-good";
    return "perf-excellent";
  }

  updateLatency(latency) {
    this.latency = latency;

    // Actualizar elementos de latencia
    const latencyValue = document.getElementById("latency-value");
    const latencyHeader = document.getElementById("latencyHeader");
    const latency2 = document.getElementById("latency");
    const latencyFill = document.getElementById("latency-fill");

    const latencyText = `${latency} ms`;

    if (latencyValue) {
      latencyValue.textContent = latencyText;
      latencyValue.className = this.getPerformanceClass(latency, 100, 200);
    }

    if (latencyHeader) {
      latencyHeader.textContent = latencyText;
      latencyHeader.className = `stat-value ${this.getPerformanceClass(
        latency,
        100,
        200
      )}`;
    }

    if (latency2) {
      latency2.textContent = latencyText;
    }

    if (latencyFill) {
      // Calcular porcentaje (0-200ms = 0-100%)
      const percentage = Math.min((latency / 200) * 100, 100);
      latencyFill.style.width = `${percentage}%`;
      latencyFill.style.background = this.getLatencyColor(percentage);
    }
  }

  getLatencyColor(percentage) {
    if (percentage <= 50)
      return "linear-gradient(90deg, var(--success), var(--success-dark))";
    if (percentage <= 75)
      return "linear-gradient(90deg, var(--warning), var(--warning-dark))";
    return "linear-gradient(90deg, var(--danger), var(--danger-dark))";
  }

  startPerformanceMonitoring() {
    // Monitorear FPS en tiempo real
    setInterval(() => {
      this.updatePerformanceStats();
    }, 1000);

    // Monitorear caída de frames
    this.startFrameDropMonitoring();

    // Monitorear uso de memoria del navegador
    this.startMemoryMonitoring();
  }

  updatePerformanceStats() {
    const liveFps = document.getElementById("liveFps");
    if (liveFps) {
      // Obtener FPS del WebSocket manager
      liveFps.textContent = wsManager.frameCount || 0;
    }
  }

  startFrameDropMonitoring() {
    let lastFrameTime = Date.now();
    let expectedFps = 20; // Valor inicial

    const checkFrameDrops = () => {
      const now = Date.now();
      const elapsed = now - lastFrameTime;
      const expectedInterval = 1000 / expectedFps;

      // Si pasó más del doble del intervalo esperado, contamos como frame drop
      if (elapsed > expectedInterval * 2) {
        this.frameDrops++;
        this.showFrameDropWarning();
      }

      lastFrameTime = now;

      // Actualizar FPS esperado basado en configuración actual
      const fpsSlider = document.getElementById("fps");
      if (fpsSlider) {
        expectedFps = parseInt(fpsSlider.value);
      }
    };

    setInterval(checkFrameDrops, 100);
  }

  showFrameDropWarning() {
    const dropIndicator = document.getElementById("frame-drop-indicator");
    const dropCount = document.getElementById("drop-count");

    if (dropIndicator && dropCount) {
      dropCount.textContent = this.frameDrops;
      dropIndicator.classList.remove("hidden");

      // Ocultar después de 5 segundos si no hay más drops
      clearTimeout(this.dropTimeout);
      this.dropTimeout = setTimeout(() => {
        dropIndicator.classList.add("hidden");
      }, 5000);
    }
  }

  startMemoryMonitoring() {
    if (performance.memory) {
      setInterval(() => {
        const memory = performance.memory;
        const usedMB = (memory.usedJSHeapSize / 1048576).toFixed(1);
        const totalMB = (memory.totalJSHeapSize / 1048576).toFixed(1);
        const percent = (
          (memory.usedJSHeapSize / memory.totalJSHeapSize) *
          100
        ).toFixed(0);

        const memoryElement = document.getElementById("memory-usage");
        if (memoryElement) {
          memoryElement.textContent = `${percent}% (${usedMB}/${totalMB} MB)`;
        }
      }, 5000);
    }
  }

  testConnection() {
    notification.show("🔍 Probando conexión...", "info");

    // Test WebSocket
    const wsStart = Date.now();
    const wsTest = new WebSocket(window.APP_CONFIG.WS_URL);

    wsTest.onopen = () => {
      const wsLatency = Date.now() - wsStart;
      wsTest.close();

      // Test API
      fetch(`${window.APP_CONFIG.API_BASE}/api/stats`)
        .then((response) => {
          const apiLatency = Date.now() - wsStart - wsLatency;

          notification.show(
            `✅ Conexión OK\nWebSocket: ${wsLatency}ms\nAPI: ${apiLatency}ms`,
            "success",
            5000
          );
        })
        .catch((error) => {
          notification.show(`❌ Error en API: ${error.message}`, "error");
        });
    };

    wsTest.onerror = () => {
      notification.show("❌ WebSocket no disponible", "error");
    };

    setTimeout(() => {
      if (wsTest.readyState === WebSocket.CONNECTING) {
        wsTest.close();
        notification.show("❌ Timeout en WebSocket", "error");
      }
    }, 3000);
  }

  refreshAll() {
    // Refrescar estadísticas
    cameraControls.refreshStats();

    // Sincronizar guardado de imágenes
    imageSaver.syncWithServer();

    // Forzar recarga de frame si está congelado
    if (this.isStreamFrozen) {
      this.isStreamFrozen = false;
      this.frozenFrame = null;
    }

    notification.show("🔄 Actualizando todo...", "info");
  }

  resetSensor() {
    if (
      confirm(
        "¿Resetear el sensor de la cámara?\n\nEsto puede ayudar a recuperar frames negros o corrupción."
      )
    ) {
      cameraControls.sendCommand("reset", "");
      notification.show("🔧 Reseteando sensor...", "warning");
    }
  }

  rebootCamera() {
    if (
      confirm(
        "¿Estás seguro de reiniciar la cámara?\n\nLa conexión se perderá temporalmente."
      )
    ) {
      cameraControls.sendCommand("reboot", 1);
      notification.show("⚡ Reiniciando cámara...", "warning");
    }
  }

  checkCompatibility() {
    const issues = [];

    // Verificar WebSocket
    if (!window.WebSocket) {
      issues.push("WebSocket no soportado");
    }

    // Verificar APIs necesarias
    if (!window.URL || !window.URL.createObjectURL) {
      issues.push("URL API no soportada");
    }

    if (!window.Blob) {
      issues.push("Blob API no soportada");
    }

    // Mostrar advertencias si hay issues
    if (issues.length > 0) {
      notification.show(
        `⚠️ Problemas de compatibilidad: ${issues.join(", ")}`,
        "warning",
        10000
      );
    } else {
      if (DEBUG) console.log("✓ Compatibilidad verificada - OK");
    }
  }

  initTheme() {
    // Cargar tema guardado
    const savedTheme = localStorage.getItem("theme") || "dark";
    document.documentElement.setAttribute("data-theme", savedTheme);

    // Configurar toggle
    const themeToggle = document.getElementById("darkModeToggle");
    if (themeToggle) {
      // Actualizar icono inicial
      const icon = themeToggle.querySelector("i");
      if (icon) {
        icon.setAttribute(
          "data-lucide",
          savedTheme === "dark" ? "moon" : "sun"
        );
        lucide.createIcons();
      }

      themeToggle.addEventListener("click", () => {
        const currentTheme =
          document.documentElement.getAttribute("data-theme");
        const newTheme = currentTheme === "dark" ? "light" : "dark";

        document.documentElement.setAttribute("data-theme", newTheme);
        localStorage.setItem("theme", newTheme);

        // Actualizar icono
        const icon = themeToggle.querySelector("i");
        if (icon) {
          icon.setAttribute(
            "data-lucide",
            newTheme === "dark" ? "moon" : "sun"
          );
          lucide.createIcons();
        }

        notification.show(
          newTheme === "dark" ? "🌙 Modo oscuro" : "☀️ Modo claro",
          "info"
        );
      });
    }
  }

  cleanup() {
    // Limpiar intervalos
    if (this.statsInterval) {
      clearInterval(this.statsInterval);
    }

    // Desconectar WebSocket
    wsManager.disconnect();

    // Limpiar URLs de objetos
    const stream = document.getElementById("videoStream");
    if (stream && stream.src && stream.src.startsWith("blob:")) {
      URL.revokeObjectURL(stream.src);
    }

    if (DEBUG) console.log("👋 ESP32 Vision - Limpiando recursos");
  }
}

// Inicializar aplicación
let cameraApp;

export default CameraApp;
