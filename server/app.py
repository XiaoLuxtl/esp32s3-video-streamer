"""
ESP32 Camera Server - Versión Modular
Servidor robusto para streaming de video con WebSocket
"""
from flask import Flask, render_template, Response, jsonify, send_from_directory, request
import threading
import time
from datetime import datetime
import json
import asyncio

# Importar configuración
from config import HTTP_PORT, WS_PORT, RESOLUTIONS, DEFAULT_RESOLUTION, COMMANDS, logger
from camera_server import camera_server
from image_saver import image_saver

# === FLASK APP ===
app = Flask(__name__, template_folder='templates', static_folder='static')

# === RUTAS PRINCIPALES ===
@app.route('/')
def index():
    """Página principal con interfaz de video"""
    return render_template('index.html', 
                          resolutions=RESOLUTIONS,
                          default_resolution=DEFAULT_RESOLUTION,
                          commands=COMMANDS,
                          http_port=HTTP_PORT)

@app.route('/mjpeg')
def mjpeg():
    """Stream MJPEG para <img> tag"""
    def generate():
        while True:
            frame = getattr(camera_server, 'latest_frame', None)
            
            if frame:
                yield (b'--frame\r\n'
                       b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
            else:
                time.sleep(0.05)
    
    return Response(generate(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/static/<path:path>')
def send_static(path):
    """Servir archivos estáticos"""
    return send_from_directory('static', path)

# === API ENDPOINTS ===
@app.route('/api/stats')
def get_stats():
    """API endpoint para estadísticas del servidor"""
    stats = getattr(camera_server, 'get_stats', lambda: {})()
    browsers_connected = len(getattr(camera_server, 'browser_clients', set()))
    
    return jsonify({
        'status': 'ok',
        'camera_connected': getattr(camera_server, 'camera_client', None) is not None,
        'stats': stats,
        'browsers_connected': browsers_connected,
        'image_saver': {
            'enabled': getattr(image_saver, 'enabled', False),
            'stats': getattr(image_saver, 'get_saved_images_count', lambda: {})()
        }
    })

@app.route('/api/health')
def health():
    """Health check endpoint"""
    start_time = getattr(getattr(camera_server, 'stats', {}), 'start_time', time.time())
    
    return jsonify({
        'status': 'healthy',
        'uptime': time.time() - start_time,
        'timestamp': datetime.now().isoformat(),
        'version': '2.0.0'
    })

@app.route('/api/resolutions')
def get_resolutions():
    """API para obtener resoluciones soportadas"""
    return jsonify({
        'status': 'ok',
        'resolutions': RESOLUTIONS,
        'default': DEFAULT_RESOLUTION
    })

@app.route('/api/commands')
def get_commands():
    """API para obtener comandos disponibles"""
    return jsonify({
        'status': 'ok',
        'commands': COMMANDS
    })

@app.route('/api/command/<cmd>/<val>')
def api_command(cmd, val):
    """API para enviar comandos a la cámara via HTTP"""
    camera_client = getattr(camera_server, 'camera_client', None)
    
    if camera_client:
        command = {
            'type': 'command',
            'cmd': cmd,
            'val': val,
            'source': 'http_api',
            'timestamp': time.time()
        }
        
        try:
            loop = asyncio.new_event_loop()
            asyncio.set_event_loop(loop)
            
            async def send_command():
                await camera_client.send(json.dumps(command))
            
            loop.run_until_complete(send_command())
            logger.info(f"✅ Comando API enviado: {cmd}={val}")
            return jsonify({'status': 'success', 'command': cmd, 'value': val})
        except Exception as e:
            logger.error(f"❌ Error enviando comando: {e}")
            return jsonify({'status': 'error', 'message': str(e)}), 500
    else:
        return jsonify({'status': 'error', 'message': 'No camera connected'}), 400

@app.route('/api/command/<cmd>')
def api_command_no_val(cmd):
    """API para enviar comandos sin valor"""
    return api_command(cmd, "")

# ========== ENDPOINTS PARA GUARDADO DE IMÁGENES ==========
@app.route('/api/images/stats')
def images_stats():
    """Estadísticas de imágenes guardadas"""
    try:
        stats_func = getattr(image_saver, 'get_saved_images_count', lambda: {})
        enabled = getattr(image_saver, 'enabled', False)
        last_saved = getattr(image_saver, 'get_last_saved_image', lambda: None)()
        today_images = getattr(image_saver, 'get_today_images_count', lambda: 0)()
        
        return jsonify({
            'status': 'ok',
            'enabled': enabled,
            'stats': stats_func(),
            'last_saved': last_saved,
            'today_images': today_images
        })
    except Exception as e:
        logger.error(f"❌ Error en images_stats: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/images/toggle', methods=['POST'])
def toggle_image_saving():
    """
    Activa / desactiva el guardado de imágenes
    - Si viene {"enabled": true/false} → SET
    - Si no viene enabled → TOGGLE
    """
    try:
        data = request.get_json(silent=True) or {}

        logger.info(f"📝 Toggle request received: {data}")

        # Puede venir explícito o no
        enabled = data.get('enabled', None)

        if enabled is None:
            # Toggle REAL
            new_state = image_saver.toggle_saving()
            logger.info("🔁 Toggle aplicado (invertido)")
        else:
            # Set explícito
            new_state = image_saver.set_enabled(enabled)
            logger.info(f"🎯 Estado forzado a: {new_state}")

        logger.info(f"✅ Guardado de imágenes: {new_state}")

        return jsonify({
            'status': 'success',
            'enabled': new_state,
            'message': f'Guardado de imágenes {"activado" if new_state else "desactivado"}'
        })

    except Exception as e:
        logger.exception("❌ Error en toggle_image_saving")
        return jsonify({
            'status': 'error',
            'message': 'Error interno al cambiar estado'
        }), 500

@app.route('/api/images/save', methods=['POST'])
def save_current_image():
    """Guardar la imagen actual"""
    try:
        logger.info("💾 Save request received")
        
        if not hasattr(image_saver, 'enabled'):
            return jsonify({
                'status': 'error',
                'message': 'Image saver not initialized'
            }), 500
        
        if not image_saver.enabled:
            return jsonify({
                'status': 'error',
                'message': 'Guardado de imágenes desactivado'
            }), 400
        
        latest_frame = getattr(camera_server, 'latest_frame', None)
        if not latest_frame:
            return jsonify({
                'status': 'error',
                'message': 'No hay imagen disponible'
            }), 404
        
        logger.info(f"💾 Intentando guardar imagen de {len(latest_frame)} bytes")
        saved_path = image_saver.save_image(latest_frame)
        
        if saved_path:
            logger.info(f"✅ Imagen guardada: {saved_path}")
            return jsonify({
                'status': 'success',
                'message': f'Imagen guardada: {saved_path.name}',
                'path': str(saved_path),
                'filename': saved_path.name
            })
        else:
            logger.warning("⚠️ Image save returned None")
            return jsonify({
                'status': 'error',
                'message': 'Error al guardar imagen'
            }), 500
            
    except Exception as e:
        logger.error(f"❌ Error saving image: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/images/state')
def get_image_saver_state():
    """Obtener solo el estado del guardado de imágenes"""
    try:
        return jsonify({
            'status': 'ok',
            'enabled': getattr(image_saver, 'enabled', False)
        })
    except Exception as e:
        logger.error(f"❌ Error obteniendo estado del guardado: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/images/latest')
def latest_image():
    """Obtener la última imagen recibida"""
    latest_frame = getattr(camera_server, 'latest_frame', None)
    if latest_frame:
        return Response(latest_frame, mimetype='image/jpeg')
    return jsonify({'status': 'error', 'message': 'No image available'}), 404

# ========== ENDPOINT PARA FAVICON (evitar error 404) ==========
@app.route('/favicon.ico')
def favicon():
    """Evitar error 404 del favicon"""
    return '', 204  # No Content

# === FUNCIONES DE INICIALIZACIÓN ===
def run_websocket_server():
    """Ejecutar WebSocket en thread separado"""
    import asyncio
    from camera_server import camera_server
    
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(camera_server.start_server())

# === MAIN ===
if __name__ == '__main__':
    print("\n" + "=" * 60)
    print("🎥  ESP32 CAMERA SERVER - VERSIÓN MODULAR")
    print("=" * 60)
    print(f"📡 HTTP Server:  http://0.0.0.0:{HTTP_PORT}")
    print(f"📡 WebSocket:    ws://0.0.0.0:{WS_PORT}")
    print(f"📡 MJPEG Stream: http://0.0.0.0:{HTTP_PORT}/mjpeg")
    print(f"📡 API Stats:    http://0.0.0.0:{HTTP_PORT}/api/stats")
    print("=" * 60)
    print("\n✅ Características:")
    print("   • Guardado MANUAL de imágenes (toggle)")
    print("   • Nombres secuenciales: 0.jpg, 1.jpg, 2.jpg...")
    print("   • Soporte completo de resoluciones")
    print("   • API REST para control")
    print("   • WebSocket con chunking")
    print("   • Interface web moderna")
    print("=" * 60)
    print("\n🚀 Iniciando servidores...\n")
    
    # Iniciar WebSocket server en thread
    ws_thread = threading.Thread(target=run_websocket_server, daemon=True)
    ws_thread.start()
    
    # Esperar un momento para que WebSocket inicie
    time.sleep(1)
    
    # Iniciar Flask server
    logger.info("🌐 Flask HTTP Server iniciando...")
    app.run(
        host='0.0.0.0',
        port=HTTP_PORT,
        debug=False,
        use_reloader=False,
        threaded=True
    )