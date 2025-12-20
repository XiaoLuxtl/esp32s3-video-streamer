// Manejo de resoluciones peligrosas
import { notification } from './notifications.js';

export class DangerousResolutions {
    constructor() {
        this.DANGEROUS_RESOLUTIONS = {
            11: { name: "FHD (1920x1080)", risk: "high" },
            12: { name: "QXGA (2048x1536)", risk: "critical" }
        };
    }

    isDangerous(value) {
        return this.DANGEROUS_RESOLUTIONS.hasOwnProperty(value);
    }

    getDangerLevel(value) {
        const danger = this.DANGEROUS_RESOLUTIONS[value];
        return danger ? danger.risk : null;
    }

    confirmDangerousResolution(value) {
        const danger = this.DANGEROUS_RESOLUTIONS[value];
        if (!danger) return true;

        let warningMsg = '';
        let warningIcon = '';

        if (danger.risk === 'critical') {
            warningIcon = '🔴';
            warningMsg = `${warningIcon} ADVERTENCIA CRÍTICA\n\n` +
                `${danger.name} puede causar:\n` +
                `• Frames negros/corruptos\n` +
                `• Sensor trabado\n` +
                `• Requiere reinicio para recuperar\n\n` +
                `Recomendación: Usar UXGA (1600x1200) como máximo.\n\n` +
                `¿Estás SEGURO de continuar?`;
        } else if (danger.risk === 'high') {
            warningIcon = '⚠️';
            warningMsg = `${warningIcon} ADVERTENCIA\n\n` +
                `${danger.name} puede causar inestabilidad.\n\n` +
                `Configuración requerida:\n` +
                `• Modo: Estabilidad\n` +
                `• FPS: 3-5 (máximo)\n` +
                `• Calidad: 10-15\n\n` +
                `¿Continuar?`;
        }

        return confirm(warningMsg);
    }

    applySafeSettings() {
        const elements = {
            cameraMode: document.getElementById('camera-mode'),
            fps: document.getElementById('fps'),
            fpsValue: document.getElementById('fps-value'),
            quality: document.getElementById('quality'),
            qualityValue: document.getElementById('quality-value')
        };

        // Forzar modo estabilidad
        if (elements.cameraMode) {
            elements.cameraMode.value = '1';
            this.sendCommand('mode', 1);
        }

        // Reducir FPS
        if (elements.fps) {
            elements.fps.value = '3';
            if (elements.fpsValue) elements.fpsValue.textContent = '3';
            this.sendCommand('fps', 3);
        }

        // Ajustar calidad
        if (elements.quality) {
            elements.quality.value = '15';
            if (elements.qualityValue) elements.qualityValue.textContent = '15';
            this.sendCommand('quality', 15);
        }

        notification.show(
            `⚙️ Configuración automática:\n` +
            `Modo: Estabilidad | FPS: 3 | Calidad: 15`,
            'info',
            5000
        );
    }

    sendCommand(cmd, val) {
        // Esta función sería provista por el módulo de controles
        // Por ahora, usamos una implementación simple
        if (window.cameraControls && window.cameraControls.sendCommand) {
            window.cameraControls.sendCommand(cmd, val);
        }
    }

    markInDropdown() {
        const resolutionSelect = document.getElementById('resolution');
        if (!resolutionSelect) return;

        Array.from(resolutionSelect.options).forEach(option => {
            const value = parseInt(option.value);

            if (this.isDangerous(value)) {
                const danger = this.DANGEROUS_RESOLUTIONS[value];

                if (danger.risk === 'critical') {
                    option.textContent = `🔴 ${option.textContent} (RIESGO)`;
                    option.style.color = '#ef4444';
                    option.style.fontWeight = 'bold';
                } else if (danger.risk === 'high') {
                    option.textContent = `⚠️ ${option.textContent} (Cuidado)`;
                    option.style.color = '#f59e0b';
                }
            }
        });
    }
}

export default DangerousResolutions;