import http.server
import socketserver
import json
import time
import os
import threading
import signal
import sys
from urllib.parse import urlparse

try:
    # Si los archivos están en la misma carpeta
    from mi_entorno.Treaduccion import TraductorCompleto
    print("✅ Módulo Treaduccion importado correctamente")
except ImportError:
    # Intenta importar de forma relativa
    try:
        from .Treaduccion import TraductorCompleto
    except ImportError:
        print("❌ No se puede importar Treaduccion")
        sys.exit(1)
except ImportError as e:
    print(f"❌ Error importando TraductorCompleto: {e}")
    sys.exit(1)

class EnhancedTraductorHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        self.start_time = time.time()
        self.request_count = 0
        try:
            self.traductor = TraductorCompleto()
            print("✅ Traductor inicializado correctamente")
        except Exception as e:
            print(f"❌ Error inicializando traductor: {e}")
            raise
        super().__init__(*args, **kwargs)
    
    def log_message(self, format, *args):
        """Logs personalizados con timestamp y cliente"""
        client_ip = self.client_address[0]
        timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
        print(f"🌐 [{timestamp}] {client_ip} - {format % args}")
    
    def do_GET(self):
        """Manejar solicitudes GET"""
        self.request_count += 1
        parsed_path = urlparse(self.path)
        
        if parsed_path.path == '/status':
            self.send_system_status()
        elif parsed_path.path == '/stats':
            self.send_statistics()
        else:
            self.send_response(404)
            self.end_headers()
    
    def do_POST(self):
        """Manejar solicitudes POST"""
        self.request_count += 1
        parsed_path = urlparse(self.path)
        
        if parsed_path.path == '/upload':
            self.handle_upload()
        else:
            self.send_response(404)
            self.end_headers()
    
    def send_system_status(self):
        """Enviar estado del sistema"""
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        
        status = {
            "status": "online",
            "service": "ESP32-CAM Translator Server",
            "uptime": round(time.time() - self.start_time),
            "timestamp": time.time(),
            "requests_processed": self.request_count
        }
        self.wfile.write(json.dumps(status, indent=2).encode())
    
    def send_statistics(self):
        """Enviar estadísticas detalladas"""
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        
        stats = {
            "server_uptime": round(time.time() - self.start_time),
            "total_requests": self.request_count,
            "current_time": time.strftime('%Y-%m-%d %H:%M:%S')
        }
        self.wfile.write(json.dumps(stats, indent=2).encode())
    
    def handle_upload(self):
        """Manejar subida de imágenes con mejor manejo de errores"""
        try:
            content_length = int(self.headers.get('Content-Length', 0))
            client_ip = self.client_address[0]
            
            if content_length == 0:
                self.send_error(400, "No image data received")
                return
            
            print(f"📥 Recibiendo imagen de {client_ip} ({content_length} bytes)")
            
            # Leer datos en chunks para manejar grandes archivos
            image_data = b''
            remaining = content_length
            chunk_size = 8192
            
            while remaining > 0:
                chunk = self.rfile.read(min(chunk_size, remaining))
                if not chunk:
                    break
                image_data += chunk
                remaining -= len(chunk)
            
            # Verificar que recibimos todos los datos
            if len(image_data) != content_length:
                self.send_error(400, f"Incomplete data: received {len(image_data)} of {content_length} bytes")
                return
            
            # Guardar imagen temporal
            timestamp = int(time.time())
            filename = f"esp32_capture_{timestamp}.jpg"
            
            with open(filename, 'wb') as f:
                f.write(image_data)
            
            print(f"💾 Imagen guardada: {filename} ({len(image_data)} bytes)")
            
            # Procesar en hilo separado
            thread = threading.Thread(
                target=self.process_image_thread, 
                args=(filename, client_ip),
                daemon=True
            )
            thread.start()
            
            # Respuesta inmediata
            response = {
                "status": "processing", 
                "filename": filename,
                "message": "Imagen recibida y en procesamiento",
                "timestamp": timestamp
            }
            
            self.send_response(202)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(json.dumps(response).encode())
                
        except Exception as e:
            print(f"❌ Error en handle_upload: {e}")
            self.send_error(500, f"Server error: {str(e)}")
    
    def process_image_thread(self, filename, client_ip):
        """Procesar imagen en hilo separado"""
        try:
            start_time = time.time()
            resultado = self.procesar_imagen(filename)
            processing_time = round(time.time() - start_time, 2)
            
            print(f"✅ Procesado completado para {client_ip} en {processing_time}s")
            
            # Log del resultado
            if resultado.get('texto_detectado'):
                texto_original = resultado.get('texto_original', '')[:100]
                texto_traducido = resultado.get('texto_traducido', '')[:100]
                print(f"📝 Texto: '{texto_original}...' → '{texto_traducido}...'")
            else:
                print(f"❌ No se detectó texto en la imagen")
                
        except Exception as e:
            print(f"❌ Error en process_image_thread: {e}")
        finally:
            # Limpiar archivo temporal después de 30 segundos
            threading.Timer(30.0, self.limpiar_archivo, args=[filename]).start()
    
    def procesar_imagen(self, imagen_path):
        """Procesar imagen con el sistema de traducción"""
        try:
            if not os.path.exists(imagen_path):
                return {"texto_detectado": False, "mensaje": "Archivo no encontrado"}
            
            # Verificar que el archivo no esté corrupto
            file_size = os.path.getsize(imagen_path)
            if file_size == 0:
                return {"texto_detectado": False, "mensaje": "Archivo vacío"}
            
            print("🔍 Extrayendo texto de la imagen...")
            texto_original = self.traductor.extraer_texto_api(imagen_path)
            
            if not texto_original or texto_original.strip() == "":
                return {"texto_detectado": False, "mensaje": "No se detectó texto en la imagen"}
            
            texto_original = texto_original.strip()
            print(f"✅ Texto detectado ({len(texto_original)} chars): {texto_original[:100]}...")
            
            # Detectar idioma
            idioma_original = self.traductor.detectar_idioma(texto_original)
            nombre_idioma = self.traductor.obtener_nombre_idioma(idioma_original)
            
            print(f"🌍 Idioma detectado: {nombre_idioma}")
            
            # Traducir a español
            texto_traducido = self.traductor.traducir_texto(texto_original, 'es')
            
            if texto_traducido:
                # Guardar en archivo automático
                self.traductor.guardar_traduccion_automatica(
                    texto_original, texto_traducido, idioma_original
                )
                
                print(f"✅ Traducido: {texto_traducido[:100]}...")
                
                return {
                    "texto_detectado": True,
                    "texto_original": texto_original,
                    "texto_traducido": texto_traducido,
                    "idioma_original": idioma_original,
                    "nombre_idioma": nombre_idioma,
                    "mensaje": "Traducción completada exitosamente"
                }
            else:
                return {
                    "texto_detectado": True,
                    "texto_original": texto_original,
                    "mensaje": "Texto detectado pero error en la traducción"
                }
                
        except Exception as e:
            print(f"❌ Error en procesar_imagen: {e}")
            return {"texto_detectado": False, "mensaje": f"Error de procesamiento: {str(e)}"}
    
    def limpiar_archivo(self, filename):
        """Eliminar archivo temporal de forma segura"""
        try:
            if os.path.exists(filename):
                os.remove(filename)
                print(f"🧹 Archivo temporal eliminado: {filename}")
        except Exception as e:
            print(f"⚠️ Error eliminando {filename}: {e}")

def obtener_ip_local():
    """Obtener la IP local de la PC"""
    import socket
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except:
        return "127.0.0.1"

def iniciar_servidor():
    """Iniciar el servidor HTTP mejorado"""
    PORT = 5000
    ip = obtener_ip_local()
    
    def signal_handler(sig, frame):
        print(f"\n🛑 Servidor detenido por el usuario")
        print(f"📊 Total de solicitudes procesadas: ...")  # Podrías agregar un contador global
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    
    with socketserver.TCPServer(("", PORT), EnhancedTraductorHandler) as httpd:
        print("🚀 SERVVIDOR TRADUCTOR ESP32-CAM INICIADO")
        print("=" * 60)
        print(f"📍 URL principal: http://{ip}:{PORT}/upload")
        print(f"📊 Estado del sistema: http://{ip}:{PORT}/status")
        print(f"📈 Estadísticas: http://{ip}:{PORT}/stats")
        print("=" * 60)
        print("📡 Esperando conexiones de ESP32-CAM...")
        print("⏹️  Presiona Ctrl+C para detener el servidor")
        print("-" * 60)
        
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n🛑 Servidor detenido por el usuario")
        except Exception as e:
            print(f"\n❌ Error en el servidor: {e}")

if __name__ == "__main__":
    # Verificar dependencias
    try:
        from googletrans import Translator
        import requests
        import cv2
        print("✅ Dependencias verificadas - Servidor listo")
    except ImportError as e:
        print(f"❌ Error: {e}")
        print("\n💡 Instala las dependencias:")
        print("   source traductor_env/bin/activate")
        print("   pip install requests googletrans==3.1.0a0 opencv-python")
        sys.exit(1)
    
    iniciar_servidor()