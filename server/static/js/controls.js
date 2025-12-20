// static/js/controls.js - Versión COMPLETA
import { wsManager } from "./websocket.js";
import { notification } from "./notifications.js";

const DEBUG = false;

export class CameraControls {
  constructor() {
    this.elements = this.gatherElements();
    this.currentSettings = this.getDefaultSettings();
    this.bindEvents();
    this.initSliders();
    this.updateUI();
    if (DEBUG) console.log("✅ CameraControls inicializado");
  }

  getDefaultSettings() {
    return {
      resolution: 5,
      quality: 12,
      fps: 20,
      mode: 0,
      brightness: 0,
      contrast: 0,
      exposure: 0,
      gain: 0,
      whitebalance: 0,
      hmirror: 0,
      vflip: 0,
    };
  }

  gatherElements() {
    // Elementos del panel de controles
    const elements = {
      // Camera Configuration
      resolution: document.getElementById("resolution"),
      currentResolution: document.getElementById("currentResolution"),
      resetResolutionBtn: document.getElementById("resetResolutionBtn"),

      quality: document.getElementById("quality"),
      qualityValue: document.getElementById("qualityValue"),

      fps: document.getElementById("fps"),
      fpsValue: document.getElementById("fpsValue"),

      cameraMode: document.getElementById("cameraMode"),
      modeHint: document.getElementById("modeHint"),
      modeHintText: document.getElementById("modeHintText"),

      hmirrorToggle: document.getElementById("hmirrorToggle"),
      vflipToggle: document.getElementById("vflipToggle"),

      // Image Adjustments
      brightness: document.getElementById("brightness"),
      brightnessValue: document.getElementById("brightnessValue"),

      contrast: document.getElementById("contrast"),
      contrastValue: document.getElementById("contrastValue"),

      exposure: document.getElementById("exposure"),
      exposureValue: document.getElementById("exposureValue"),

      gain: document.getElementById("gain"),
      gainValue: document.getElementById("gainValue"),

      whitebalance: document.getElementById("whitebalance"),
      whitebalanceValue: document.getElementById("whitebalanceValue"),

      resetImageAdj: document.getElementById("resetImageAdj"),

      // Image Saver
      saveToggle: document.getElementById("saveToggle"),
      saveToggleThumb: document.getElementById("saveToggleThumb"),
      saveStatus: document.getElementById("saveStatus"),
      saveStats: document.getElementById("saveStats"),
      manualSaveBtn: document.getElementById("manualSaveBtn"),
      viewSavedBtn: document.getElementById("viewSavedBtn"),
      syncSaveStateBtn: document.getElementById("syncSaveStateBtn"),

      // Statistics (por si quieres actualizarlos)
      cameraFrames: document.getElementById("cameraFrames"),
      cameraHeap: document.getElementById("cameraHeap"),
      cameraUptime: document.getElementById("cameraUptime"),
      serverFrames: document.getElementById("serverFrames"),
      serverData: document.getElementById("serverData"),
      serverFps: document.getElementById("serverFps"),
      savedImages: document.getElementById("savedImages"),
      todayImages: document.getElementById("todayImages"),
      lastSaved: document.getElementById("lastSaved"),
      liveFps: document.getElementById("liveFps"),
      latency: document.getElementById("latency"),
      serverClients: document.getElementById("serverClients"),

      // Quick Actions
      captureBtn: document.getElementById("captureBtn"),
      quickCaptureBtn: document.getElementById("quickCaptureBtn"),
      fullscreenBtn: document.getElementById("fullscreenBtn"),
      zoomBtn: document.getElementById("zoomBtn"),
      freezeFrameBtn: document.getElementById("freezeFrameBtn"),
      refreshStatsBtn: document.getElementById("refreshStatsBtn"),

      // System Actions
      resetSensorBtn: document.getElementById("resetSensorBtn"),
      rebootBtn: document.getElementById("rebootBtn"),

      // Connection Info
      connectionDuration: document.getElementById("connectionDuration"),
      cameraIp: document.getElementById("cameraIp"),

      // Header elements
      darkModeToggle: document.getElementById("darkModeToggle"),
    };

    // Debug: Mostrar elementos encontrados
    const found = Object.keys(elements).filter((key) => elements[key]);
    if (DEBUG)
      console.log(
        `📋 Elementos encontrados: ${found.length}/${
          Object.keys(elements).length
        }`
      );

    return elements;
  }

  bindEvents() {
    if (DEBUG) console.log("🔗 Vinculando eventos...");

    // ===== CAMERA CONFIGURATION =====
    // Resolution
    if (this.elements.resolution) {
      this.elements.resolution.addEventListener("change", (e) => {
        this.handleResolutionChange(e);
      });
    }

    if (this.elements.resetResolutionBtn) {
      this.elements.resetResolutionBtn.addEventListener("click", () => {
        this.resetResolution();
      });
    }

    // Quality
    if (this.elements.quality) {
      this.elements.quality.addEventListener("input", (e) => {
        if (this.elements.qualityValue) {
          this.elements.qualityValue.textContent = e.target.value;
        }
      });
      this.elements.quality.addEventListener("change", (e) => {
        this.sendCommand("quality", parseInt(e.target.value));
      });
    }

    // FPS
    if (this.elements.fps) {
      this.elements.fps.addEventListener("input", (e) => {
        if (this.elements.fpsValue) {
          this.elements.fpsValue.textContent = e.target.value;
        }
      });
      this.elements.fps.addEventListener("change", (e) => {
        this.sendCommand("fps", parseInt(e.target.value));
      });
    }

    // Mode
    if (this.elements.cameraMode) {
      this.elements.cameraMode.addEventListener("change", (e) => {
        this.sendCommand("mode", parseInt(e.target.value));
        this.updateModeHint();
      });
    }

    // Camera Effects
    if (this.elements.hmirrorToggle) {
      this.elements.hmirrorToggle.addEventListener("click", () => {
        this.toggleEffect("hmirror", this.elements.hmirrorToggle);
      });
    }

    if (this.elements.vflipToggle) {
      this.elements.vflipToggle.addEventListener("click", () => {
        this.toggleEffect("vflip", this.elements.vflipToggle);
      });
    }

    // ===== IMAGE ADJUSTMENTS =====
    this.bindImageAdjustments();

    // Reset Image Adjustments
    if (this.elements.resetImageAdj) {
      this.elements.resetImageAdj.addEventListener("click", () => {
        this.resetImageAdjustments();
      });
    }

    // ===== IMAGE SAVER =====
    if (this.elements.saveToggle) {
      this.elements.saveToggle.addEventListener("click", () => {
        this.toggleAutoSave();
      });
    }

    if (this.elements.manualSaveBtn) {
      this.elements.manualSaveBtn.addEventListener("click", () => {
        this.manualSave();
      });
    }

    if (this.elements.viewSavedBtn) {
      this.elements.viewSavedBtn.addEventListener("click", () => {
        this.viewSavedImages();
      });
    }

    if (this.elements.syncSaveStateBtn) {
      this.elements.syncSaveStateBtn.addEventListener("click", () => {
        this.syncSaveState();
      });
    }

    // ===== QUICK ACTIONS =====
    if (this.elements.captureBtn) {
      this.elements.captureBtn.addEventListener("click", () => {
        this.captureImage();
      });
    }

    if (this.elements.quickCaptureBtn) {
      this.elements.quickCaptureBtn.addEventListener("click", () => {
        this.quickCapture();
      });
    }

    if (this.elements.fullscreenBtn) {
      this.elements.fullscreenBtn.addEventListener("click", () => {
        this.toggleFullscreen();
      });
    }

    if (this.elements.zoomBtn) {
      this.elements.zoomBtn.addEventListener("click", () => {
        this.toggleZoom();
      });
    }

    if (this.elements.freezeFrameBtn) {
      this.elements.freezeFrameBtn.addEventListener("click", () => {
        this.toggleFreezeFrame();
      });
    }

    if (this.elements.refreshStatsBtn) {
      this.elements.refreshStatsBtn.addEventListener("click", () => {
        this.refreshStats();
      });
    }

    // ===== SYSTEM ACTIONS =====
    if (this.elements.resetSensorBtn) {
      this.elements.resetSensorBtn.addEventListener("click", () => {
        this.resetSensor();
      });
    }

    if (this.elements.rebootBtn) {
      this.elements.rebootBtn.addEventListener("click", () => {
        this.rebootCamera();
      });
    }

    // ===== HEADER CONTROLS =====
    if (this.elements.darkModeToggle) {
      this.elements.darkModeToggle.addEventListener("click", () => {
        this.toggleDarkMode();
      });
    }

    if (DEBUG) console.log("✅ Eventos vinculados correctamente");
  }

  bindImageAdjustments() {
    // Brightness
    if (this.elements.brightness) {
      this.elements.brightness.addEventListener("input", (e) => {
        if (this.elements.brightnessValue) {
          this.elements.brightnessValue.textContent = e.target.value;
        }
      });
      this.elements.brightness.addEventListener("change", (e) => {
        this.sendCommand("brightness", parseInt(e.target.value));
      });
    }

    // Contrast
    if (this.elements.contrast) {
      this.elements.contrast.addEventListener("input", (e) => {
        if (this.elements.contrastValue) {
          this.elements.contrastValue.textContent = e.target.value;
        }
      });
      this.elements.contrast.addEventListener("change", (e) => {
        this.sendCommand("contrast", parseInt(e.target.value));
      });
    }

    // Exposure
    if (this.elements.exposure) {
      this.elements.exposure.addEventListener("input", (e) => {
        if (this.elements.exposureValue) {
          this.elements.exposureValue.textContent = e.target.value;
        }
      });
      this.elements.exposure.addEventListener("change", (e) => {
        this.sendCommand("exposure", parseInt(e.target.value));
      });
    }

    // Gain
    if (this.elements.gain) {
      this.elements.gain.addEventListener("input", (e) => {
        if (this.elements.gainValue) {
          this.elements.gainValue.textContent = e.target.value;
        }
      });
      this.elements.gain.addEventListener("change", (e) => {
        this.sendCommand("gain", parseInt(e.target.value));
      });
    }

    // White Balance
    if (this.elements.whitebalance) {
      this.elements.whitebalance.addEventListener("input", (e) => {
        const value = parseInt(e.target.value);
        if (this.elements.whitebalanceValue) {
          this.elements.whitebalanceValue.textContent =
            this.getWhiteBalanceName(value);
        }
      });
      this.elements.whitebalance.addEventListener("change", (e) => {
        this.sendCommand("whitebalance", parseInt(e.target.value));
      });
    }
  }

  initSliders() {
    // Inicializar valores visuales de los sliders
    const updateValue = (element, valueElement) => {
      if (element && valueElement) {
        valueElement.textContent = element.value;
      }
    };

    updateValue(this.elements.quality, this.elements.qualityValue);
    updateValue(this.elements.fps, this.elements.fpsValue);
    updateValue(this.elements.brightness, this.elements.brightnessValue);
    updateValue(this.elements.contrast, this.elements.contrastValue);
    updateValue(this.elements.exposure, this.elements.exposureValue);
    updateValue(this.elements.gain, this.elements.gainValue);

    if (this.elements.whitebalance && this.elements.whitebalanceValue) {
      this.elements.whitebalanceValue.textContent = this.getWhiteBalanceName(
        parseInt(this.elements.whitebalance.value)
      );
    }
  }

  // ===== CONTROL METHODS =====

  handleResolutionChange(e) {
    const value = parseInt(e.target.value);
    const name = e.target.options[e.target.selectedIndex].text;

    // Actualizar display
    if (this.elements.currentResolution) {
      this.elements.currentResolution.textContent = name;
    }

    // Enviar comando
    this.sendCommand("resolution", value);
    notification.show(`📐 Resolución: ${name}`, "info");
  }

  resetResolution() {
    if (this.elements.resolution) {
      this.elements.resolution.value = "5"; // VGA
      const event = new Event("change");
      this.elements.resolution.dispatchEvent(event);
    }
  }

  toggleEffect(effect, button) {
    const currentValue = this.currentSettings[effect] || 0;
    const newValue = currentValue === 0 ? 1 : 0;

    // Actualizar UI
    const statusElement = button.querySelector(".effect-status");
    if (statusElement) {
      statusElement.dataset.status = newValue === 1 ? "on" : "off";
      statusElement.textContent = newValue === 1 ? "ON" : "OFF";
      button.classList.toggle("active", newValue === 1);
    }

    // Enviar comando
    this.sendCommand(effect, newValue);
    this.currentSettings[effect] = newValue;
  }

  updateModeHint() {
    if (!this.elements.cameraMode || !this.elements.modeHintText) return;

    const mode = parseInt(this.elements.cameraMode.value);
    const hints = {
      0: "Prioriza velocidad y baja latencia. Ideal para control en tiempo real.",
      1: "Balance entre velocidad y calidad. Recomendado para uso general.",
    };

    this.elements.modeHintText.textContent = hints[mode] || "";

    if (this.elements.modeHint) {
      this.elements.modeHint.classList.toggle("hidden", !hints[mode]);
    }
  }

  resetImageAdjustments() {
    const defaults = {
      brightness: 0,
      contrast: 0,
      exposure: 0,
      gain: 0,
      whitebalance: 0,
    };

    Object.keys(defaults).forEach((setting) => {
      const element = this.elements[setting];
      const valueElement = this.elements[`${setting}Value`];

      if (element && valueElement) {
        element.value = defaults[setting];

        if (setting === "whitebalance") {
          valueElement.textContent = this.getWhiteBalanceName(
            defaults[setting]
          );
        } else {
          valueElement.textContent = defaults[setting];
        }

        this.sendCommand(setting, defaults[setting]);
        this.currentSettings[setting] = defaults[setting];
      }
    });

    notification.show("⚙️ Ajustes de imagen restablecidos", "info");
  }

  getWhiteBalanceName(value) {
    const whiteBalanceModes = {
      0: "Auto",
      1: "Sol",
      2: "Nublado",
      3: "Tungsteno",
      4: "Fluor",
    };
    return whiteBalanceModes[value] || "Auto";
  }

  // ===== IMAGE SAVER METHODS =====

  toggleAutoSave() {
    const toggle = this.elements.saveToggle;
    if (!toggle) return;

    const isActive = toggle.classList.contains("active");
    toggle.classList.toggle("active", !isActive);

    if (this.elements.saveToggleThumb) {
      this.elements.saveToggleThumb.style.transform = !isActive
        ? "translateX(20px)"
        : "translateX(0)";
    }

    if (this.elements.saveStatus) {
      this.elements.saveStatus.textContent = !isActive
        ? "ACTIVADO"
        : "DESACTIVADO";
      this.elements.saveStatus.style.color = !isActive
        ? "var(--success)"
        : "var(--text-secondary)";
    }

    if (this.elements.manualSaveBtn) {
      this.elements.manualSaveBtn.disabled = !isActive;
    }

    notification.show(
      `💾 Auto-guardado ${!isActive ? "ACTIVADO" : "DESACTIVADO"}`,
      !isActive ? "success" : "info"
    );
  }

  manualSave() {
    this.captureImage();
  }

  viewSavedImages() {
    notification.show("📁 Función en desarrollo...", "info");
  }

  syncSaveState() {
    notification.show("🔄 Sincronizando estado...", "info");
  }

  // ===== QUICK ACTIONS METHODS =====

  captureImage() {
    const stream = document.getElementById("videoStream");
    if (!stream || !stream.src || !stream.src.startsWith("blob:")) {
      notification.show("⚠️ No hay imagen disponible para capturar", "warning");
      return;
    }

    const link = document.createElement("a");
    const timestamp = new Date()
      .toISOString()
      .replace(/[:.]/g, "-")
      .replace("T", "_")
      .slice(0, 19);

    link.download = `esp32_capture_${timestamp}.jpg`;
    link.href = stream.src;
    link.click();

    notification.show("📸 Imagen capturada y descargada", "success");
  }

  quickCapture() {
    this.captureImage();
  }

  toggleFullscreen() {
    const videoContainer = document.querySelector(".video-container");
    if (!videoContainer) return;

    if (!document.fullscreenElement) {
      videoContainer.requestFullscreen().catch((err) => {
        notification.show("⚠️ Error al activar pantalla completa", "error");
      });
    } else {
      document.exitFullscreen();
    }
  }

  toggleZoom() {
    const stream = document.getElementById("videoStream");
    if (!stream) return;

    const isZoomed = stream.style.objectFit === "contain";
    stream.style.objectFit = isZoomed ? "cover" : "contain";

    notification.show(
      isZoomed ? "🔍 Zoom 1:1 activado" : "🔍 Vista completa",
      "info"
    );
  }

  toggleFreezeFrame() {
    const stream = document.getElementById("videoStream");
    if (!stream) return;

    const isFrozen = stream.style.animationPlayState === "paused";
    stream.style.animationPlayState = isFrozen ? "running" : "paused";

    // Cambiar icono
    const icon = this.elements.freezeFrameBtn?.querySelector("i");
    if (icon) {
      icon.setAttribute("data-lucide", isFrozen ? "pause" : "play");
      lucide.createIcons();
    }

    notification.show(
      isFrozen ? "▶️ Vídeo reanudado" : "⏸️ Vídeo pausado",
      "info"
    );
  }

  refreshStats() {
    if (wsManager.isConnected) {
      wsManager.send({ type: "request_stats" });
      notification.show("🔄 Actualizando estadísticas...", "info");
    } else {
      notification.show("⚠️ No hay conexión con la cámara", "warning");
    }
  }

  // ===== SYSTEM METHODS =====

  resetSensor() {
    if (
      confirm(
        "¿Resetear el sensor de la cámara?\n\nEsto puede ayudar a recuperar frames negros o corrupción."
      )
    ) {
      this.sendCommand("sensor_reset", "");
      notification.show("🔧 Reseteando sensor...", "warning");
    }
  }

  rebootCamera() {
    if (
      confirm(
        "¿Estás seguro de reiniciar la cámara?\n\nLa conexión se perderá temporalmente."
      )
    ) {
      this.sendCommand("reboot", 1);
      notification.show("⚡ Reiniciando cámara...", "warning");
    }
  }

  // ===== HEADER METHODS =====

  toggleDarkMode() {
    const html = document.documentElement;
    const currentTheme = html.getAttribute("data-theme");
    const newTheme = currentTheme === "dark" ? "light" : "dark";

    html.setAttribute("data-theme", newTheme);

    // Cambiar icono
    const icon = this.elements.darkModeToggle?.querySelector("i");
    if (icon) {
      icon.setAttribute("data-lucide", newTheme === "dark" ? "moon" : "sun");
      lucide.createIcons();
    }

    notification.show(
      `🌓 Modo ${newTheme === "dark" ? "oscuro" : "claro"} activado`,
      "info"
    );
  }

  // ===== CORE METHODS =====

  sendCommand(cmd, val) {
    if (!wsManager.isConnected) {
      notification.show("⚠️ No hay conexión con la cámara", "warning");
      return false;
    }

    const command = {
      type: "command",
      cmd: cmd,
      val: val,
      timestamp: Date.now(),
    };

    // Guardar en configuración actual
    this.currentSettings[cmd] = val;

    // Enviar a través de WebSocket
    const sent = wsManager.send(command);

    if (sent) {
      if (DEBUG) console.log(`✅ Comando enviado: ${cmd}=${val}`);
      return true;
    }

    return false;
  }

  updateUI() {
    // Actualizar modo actual
    this.updateModeHint();

    // Inicializar sliders
    this.initSliders();
  }

  handleStatusUpdate(status) {
    // Actualizar IP de cámara
    if (status.camera_ip && this.elements.cameraIp) {
      this.elements.cameraIp.textContent = status.camera_ip;
    }

    // Actualizar configuración actual si viene del servidor
    if (status.settings) {
      Object.keys(status.settings).forEach((key) => {
        if (this.currentSettings.hasOwnProperty(key)) {
          this.currentSettings[key] = status.settings[key];
        }
      });
      this.updateUI();
    }

    // Actualizar estadísticas
    this.updateStatistics(status);
  }

  updateStatistics(status) {
    // Actualizar estadísticas de cámara
    if (status.frames && this.elements.cameraFrames) {
      this.elements.cameraFrames.textContent = status.frames;
    }

    if (status.heap && this.elements.cameraHeap) {
      this.elements.cameraHeap.textContent = `${status.heap} KB`;
    }

    // Actualizar estadísticas de servidor
    if (status.server_frames && this.elements.serverFrames) {
      this.elements.serverFrames.textContent = status.server_frames;
    }

    // Actualizar FPS en vivo
    if (status.live_fps && this.elements.liveFps) {
      this.elements.liveFps.textContent = status.live_fps.toFixed(1);
    }

    // Actualizar latencia
    if (status.latency && this.elements.latency) {
      this.elements.latency.textContent = `${status.latency}ms`;
    }
  }
}

// Exportar instancia única
export const cameraControls = new CameraControls();
export default cameraControls;
