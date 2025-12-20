// Sistema de guardado de imágenes
import { notification } from "./notifications.js";

const DEBUG = false;

export class ImageSaver {
  constructor() {
    this.enabled = false;
    this.saveStats = { total_images: 0, by_date: {} };
    this.elements = null;

    // No inicializar aquí - se hace después con init()
  }

  init() {
    this.elements = this.gatherElements();
    this.bindEvents();
    this.loadState();
  }

  gatherElements() {
    const elements = {
      saveToggle: document.getElementById("saveToggle"),
      saveToggleThumb: document.getElementById("saveToggleThumb"),
      saveStatus: document.getElementById("saveStatus"),
      manualSaveBtn: document.getElementById("manualSaveBtn"),
      saveStatsText: document.getElementById("saveStats"),
      savedImages: document.getElementById("savedImages"),
      todayImages: document.getElementById("todayImages"),
      lastSaved: document.getElementById("lastSaved"),
      saverStatus: document.getElementById("saverStatus"),
    };

    if (DEBUG)
      console.log(
        "ImageSaver elements found:",
        Object.keys(elements).map((key) => `${key}: ${!!elements[key]}`)
      );
    return elements;
  }

  bindEvents() {
    if (!this.elements) return;

    // Toggle
    if (this.elements.saveToggle) {
      this.elements.saveToggle.addEventListener("click", () => {
        this.toggleSaving();
      });
    }

    // Guardado manual
    if (this.elements.manualSaveBtn) {
      this.elements.manualSaveBtn.addEventListener("click", () => {
        this.manualSave();
      });
    }
  }

  async loadState() {
    // Cargar de localStorage primero
    const savedState = localStorage.getItem("imageSavingEnabled");
    if (savedState !== null) {
      this.enabled = JSON.parse(savedState);
      this.updateUI();
    }

    // Sincronizar con servidor
    await this.syncWithServer();
  }

  async syncWithServer() {
    if (!this.elements) {
      return;
    }

    try {
      const response = await fetch("/api/images/stats");
      const data = await response.json();

      if (data.status === "ok") {
        const serverState = data.enabled;

        // Detectar desincronización
        if (serverState !== this.enabled) {
          console.warn(
            `Desincronización: localStorage=${this.enabled}, servidor=${serverState}`
          );

          // Usar estado del servidor por defecto
          this.enabled = serverState;
          this.updateUI();
          notification.show("🔄 Estado sincronizado con el servidor", "info");
        }

        // Actualizar estadísticas
        this.saveStats = data.stats || { total_images: 0, by_date: {} };

        // Actualizar última imagen
        if (data.last_saved && this.elements.lastSaved) {
          const last = data.last_saved;
          this.elements.lastSaved.textContent = last.filename || "--";
          this.elements.lastSaved.title = `Guardado en ${last.date}/${last.hour}`;
        }

        this.updateStats();
      }
    } catch (error) {
      console.error("Error sincronizando con servidor:", error);
      this.saveStats = { total_images: 0, by_date: {} };
      this.updateStats();
    }
  }

  async toggleSaving() {
    if (!this.elements) {
      return;
    }

    const newState = !this.enabled;

    // Actualizar UI inmediatamente
    this.enabled = newState;
    this.updateUI();

    try {
      const response = await fetch("/api/images/toggle", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          Accept: "application/json",
        },
        body: JSON.stringify({ enabled: newState }),
      });

      const data = await response.json();

      if (data.status === "success") {
        this.enabled = data.enabled;
        this.updateUI();
        notification.show(
          `📸 Guardado ${this.enabled ? "activado" : "desactivado"}`,
          this.enabled ? "success" : "info"
        );

        await this.loadStats();
      } else {
        // Revertir
        this.enabled = !newState;
        this.updateUI();
        notification.show(
          `❌ Error: ${data.message || "Error del servidor"}`,
          "error"
        );
      }
    } catch (error) {
      console.error("❌ Error toggling image save:", error);
      this.enabled = !newState;
      this.updateUI();
      notification.show("❌ Error de conexión con el servidor", "error");
    }
  }

  async manualSave() {
    if (!this.elements) {
      console.log("❌ Elements not initialized yet");
      return;
    }

    if (!this.enabled) {
      notification.show("⚠️ El guardado está desactivado", "warning");
      return;
    }

    // Deshabilitar botón temporalmente
    const originalText = this.elements.manualSaveBtn.textContent;
    this.elements.manualSaveBtn.disabled = true;
    this.elements.manualSaveBtn.textContent = "GUARDANDO...";

    try {
      const response = await fetch("/api/images/save", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          Accept: "application/json",
        },
      });

      const data = await response.json();

      if (data.status === "success") {
        notification.show(`✅ ${data.message}`, "success");
        await this.loadStats();
      } else {
        notification.show(`❌ ${data.message}`, "error");
      }
    } catch (error) {
      console.error("Error saving image:", error);
      notification.show("❌ Error al guardar imagen", "error");
    } finally {
      // Restaurar botón
      setTimeout(() => {
        this.elements.manualSaveBtn.disabled = !this.enabled;
        this.elements.manualSaveBtn.textContent = originalText;
      }, 1000);
    }
  }

  updateUI() {
    if (!this.elements || !this.elements.saveToggle) return;

    if (DEBUG) console.log("Actualizando UI, estado:", this.enabled);

    if (this.enabled) {
      this.elements.saveToggle.classList.remove("bg-gray-700");
      this.elements.saveToggle.classList.add("bg-green-600");
      this.elements.saveToggleThumb.classList.remove("translate-x-1");
      this.elements.saveToggleThumb.classList.add("translate-x-6");
      this.elements.saveStatus.textContent = "ACTIVADO";
      this.elements.saveStatus.className = "text-sm font-medium text-green-400";

      if (this.elements.manualSaveBtn) {
        this.elements.manualSaveBtn.disabled = false;
        this.elements.manualSaveBtn.classList.remove(
          "opacity-50",
          "cursor-not-allowed"
        );
        this.elements.manualSaveBtn.classList.add("hover:bg-cyan-500");
      }
    } else {
      this.elements.saveToggle.classList.remove("bg-green-600");
      this.elements.saveToggle.classList.add("bg-gray-700");
      this.elements.saveToggleThumb.classList.remove("translate-x-6");
      this.elements.saveToggleThumb.classList.add("translate-x-1");
      this.elements.saveStatus.textContent = "DESACTIVADO";
      this.elements.saveStatus.className = "text-sm font-medium text-gray-300";

      if (this.elements.manualSaveBtn) {
        this.elements.manualSaveBtn.disabled = true;
        this.elements.manualSaveBtn.classList.add(
          "opacity-50",
          "cursor-not-allowed"
        );
        this.elements.manualSaveBtn.classList.remove("hover:bg-cyan-500");
      }
    }

    // Guardar en localStorage
    localStorage.setItem("imageSavingEnabled", JSON.stringify(this.enabled));

    this.updateStats();
  }

  async loadStats() {
    try {
      const response = await fetch("/api/images/stats");
      const data = await response.json();

      if (data.status === "ok") {
        this.saveStats = data.stats || { total_images: 0, by_date: {} };

        if (data.last_saved && this.elements.lastSaved) {
          const last = data.last_saved;
          this.elements.lastSaved.textContent = last.filename || "--";
          this.elements.lastSaved.title = `Guardado en ${last.date}/${last.hour}`;
        }

        this.updateStats();
      }
    } catch (error) {
      console.error("Error loading image saver stats:", error);
    }
  }

  updateStats() {
    const today = new Date().toISOString().split("T")[0];
    const todayImages =
      this.saveStats.by_date && this.saveStats.by_date[today]
        ? this.saveStats.by_date[today].images
        : 0;

    // Actualizar texto de estadísticas
    if (this.elements.saveStatsText) {
      this.elements.saveStatsText.textContent = `${
        this.saveStats.total_images || 0
      } imágenes guardadas (${todayImages} hoy)`;
    }

    // Actualizar elementos individuales
    if (this.elements.savedImages) {
      this.elements.savedImages.textContent = this.saveStats.total_images || 0;
    }

    if (this.elements.todayImages) {
      this.elements.todayImages.textContent = todayImages;
    }

    if (this.elements.saverStatus) {
      this.elements.saverStatus.textContent = this.enabled
        ? "Activo"
        : "Inactivo";
      this.elements.saverStatus.className = this.enabled
        ? "text-green-400"
        : "text-gray-400";
    }
  }
}

// Crear instancia y inicializar cuando el DOM esté listo
const imageSaverInstance = new ImageSaver();

// Función para inicializar cuando el DOM esté listo
function initWhenDOMReady() {
  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", () => {
      imageSaverInstance.init();
    });
  } else {
    // DOM ya está listo
    imageSaverInstance.init();
  }
}

// Inicializar
initWhenDOMReady();

// Exportar la instancia directamente
export const imageSaver = imageSaverInstance;

export default imageSaver;
