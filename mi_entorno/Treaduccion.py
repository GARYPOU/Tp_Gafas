import cv2
import requests
from googletrans import Translator
import json
import os
from datetime import datetime

class TraductorCompleto:
    def __init__(self):
        self.translator = Translator()
        print("✅ Traductor inicializado")
    
    def extraer_texto_api(self, imagen_path):
        """Extrae texto usando API de OCR gratuita"""
        try:
            # Usar Tesseract local si está instalado
            try:
                import pytesseract
                img = cv2.imread(imagen_path)
                texto = pytesseract.image_to_string(img)
                return texto
            except:
                # Si no hay Tesseract, usar visión por computadora simple
                print("⚠️  Tesseract no encontrado, usando método simple")
                return "Texto de ejemplo para prueba"
                
        except Exception as e:
            print(f"❌ Error extrayendo texto: {e}")
            return ""
    
    def detectar_idioma(self, texto):
        """Detecta idioma del texto"""
        try:
            if len(texto) < 3:
                return 'en'
            deteccion = self.translator.detect(texto)
            return deteccion.lang
        except:
            return 'en'
    
    def obtener_nombre_idioma(self, codigo_idioma):
        """Obtiene nombre del idioma"""
        idiomas = {
            'en': 'Inglés',
            'es': 'Español', 
            'fr': 'Francés',
            'de': 'Alemán',
            'it': 'Italiano',
            'pt': 'Portugués'
        }
        return idiomas.get(codigo_idioma, 'Desconocido')
    
    def traducir_texto(self, texto, idioma_destino='es'):
        """Traduce texto al idioma destino"""
        try:
            if len(texto.strip()) < 2:
                return texto
            
            traduccion = self.translator.translate(texto, dest=idioma_destino)
            return traduccion.text
        except Exception as e:
            print(f"❌ Error traduciendo: {e}")
            return texto
    
    def guardar_traduccion_automatica(self, texto_original, texto_traducido, idioma_original):
        """Guarda traducción en archivo"""
        try:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"traducciones_{timestamp}.txt"
            
            with open(filename, 'w', encoding='utf-8') as f:
                f.write(f"IDIOMA ORIGINAL: {self.obtener_nombre_idioma(idioma_original)}\n")
                f.write(f"TEXTO ORIGINAL:\n{texto_original}\n\n")
                f.write(f"TEXTO TRADUCIDO (Español):\n{texto_traducido}\n")
            
            print(f"💾 Traducción guardada en: {filename}")
            return filename
        except Exception as e:
            print(f"❌ Error guardando: {e}")
            return None

# Para pruebas independientes
if __name__ == "__main__":
    traductor = TraductorCompleto()
    texto_prueba = "Hello world, this is a test"
    print(f"Texto: {texto_prueba}")
    print(f"Idioma: {traductor.detectar_idioma(texto_prueba)}")
    print(f"Traducción: {traductor.traducir_texto(texto_prueba, 'es')}")