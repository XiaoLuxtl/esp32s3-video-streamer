// Utilidades generales

/**
 * Formatea segundos a HH:MM:SS
 */
export function formatUptime(seconds) {
    if (!seconds) return '--';

    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = Math.floor(seconds % 60);

    return `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
}

/**
 * Obtiene la fecha actual en formato YYYY-MM-DD
 */
export function getTodayDate() {
    return new Date().toISOString().split('T')[0];
}

/**
 * Crea un elemento DOM con atributos
 */
export function createElement(tag, attributes = {}, text = '') {
    const element = document.createElement(tag);

    Object.keys(attributes).forEach(key => {
        element.setAttribute(key, attributes[key]);
    });

    if (text) {
        element.textContent = text;
    }

    return element;
}

/**
 * Debounce function para optimizar eventos
 */
export function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

export default {
    formatUptime,
    getTodayDate,
    createElement,
    debounce
};