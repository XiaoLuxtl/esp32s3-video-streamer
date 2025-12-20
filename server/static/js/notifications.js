// Sistema de notificaciones - Versión corregida

const DEBUG = false;

class NotificationSystem {
  constructor() {
    this.notification = document.getElementById("notification");
    this.icon = document.getElementById("notificationIcon"); // Cambiado
    this.message = document.getElementById("notificationMessage"); // Cambiado

    if (!this.notification) {
      this.createNotificationElement();
    } else {
      if (DEBUG)
        console.log("✅ Elementos de notificación encontrados:", {
          notification: !!this.notification,
          icon: !!this.icon,
          message: !!this.message,
        });
    }
  }

  createNotificationElement() {
    console.warn("⚠️ Creando elemento de notificación dinámicamente");

    const container = document.createElement("div");
    container.id = "notification";
    container.className = "toast-notification";
    container.style.cssText = `
            position: fixed;
            top: 20px;
            right: 20px;
            background: var(--bg-surface);
            color: var(--text-primary);
            padding: 16px;
            border-radius: 8px;
            border-left: 4px solid var(--primary);
            box-shadow: var(--shadow-lg);
            z-index: 1000;
            max-width: 300px;
            transition: opacity 0.3s, transform 0.3s;
            opacity: 0;
            transform: translateX(100%);
            display: none;
        `;

    const content = document.createElement("div");
    content.className = "toast-content";
    content.style.display = "flex";
    content.style.alignItems = "center";
    content.style.gap = "12px";

    this.icon = document.createElement("i");
    this.icon.id = "notificationIcon";
    this.icon.setAttribute("data-lucide", "info");
    this.icon.className = "toast-icon";

    this.message = document.createElement("span");
    this.message.id = "notificationMessage";
    this.message.className = "toast-message";
    this.message.style.flex = "1";

    content.appendChild(this.icon);
    content.appendChild(this.message);
    container.appendChild(content);
    document.body.appendChild(container);

    this.notification = container;

    // Inicializar icono de Lucide
    if (window.lucide) {
      lucide.createIcons();
    }
  }

  show(message, type = "info", duration = 3000) {
    // Asegurarse de que los elementos existen
    if (!this.notification) {
      this.createNotificationElement();
    }

    if (!this.icon || !this.message) {
      console.error("❌ Elementos de notificación no disponibles");
      return;
    }

    if (DEBUG) console.log(`📢 Mostrando notificación: ${type} - ${message}`);

    // Configurar tipo
    let iconName = "";
    let borderColor = "";
    let iconColor = "";

    switch (type) {
      case "success":
        iconName = "check-circle";
        borderColor = "var(--success)";
        iconColor = "var(--success)";
        break;
      case "error":
        iconName = "x-circle";
        borderColor = "var(--error)";
        iconColor = "var(--error)";
        break;
      case "warning":
        iconName = "alert-triangle";
        borderColor = "var(--warning)";
        iconColor = "var(--warning)";
        break;
      default:
        iconName = "info";
        borderColor = "var(--primary)";
        iconColor = "var(--primary)";
    }

    // Actualizar icono
    this.icon.setAttribute("data-lucide", iconName);
    this.icon.style.color = iconColor;

    // Actualizar mensaje
    this.message.textContent = message;

    // Actualizar estilo
    this.notification.style.borderLeftColor = borderColor;
    this.notification.style.display = "block";

    // Recrear iconos de Lucide
    if (window.lucide) {
      lucide.createIcons();
    }

    // Animar entrada
    requestAnimationFrame(() => {
      this.notification.style.opacity = "1";
      this.notification.style.transform = "translateX(0)";
    });

    // Ocultar después de la duración
    if (duration > 0) {
      setTimeout(() => {
        this.hide();
      }, duration);
    }
  }

  hide() {
    if (!this.notification) return;

    this.notification.style.opacity = "0";
    this.notification.style.transform = "translateX(100%)";

    setTimeout(() => {
      if (this.notification) {
        this.notification.style.display = "none";
      }
    }, 300);
  }
}

// Exportar instancia única
export const notification = new NotificationSystem();

// Función helper global
window.showNotification = (message, type = "info", duration = 3000) => {
  notification.show(message, type, duration);
};

// Depuración: exponer globalmente para pruebas
window.notification = notification;

export default notification;
