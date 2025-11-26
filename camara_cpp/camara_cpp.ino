#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_camera.h"
#include <WiFiClientSecure.h>

// =============================================
// CONFIGURACIÓN - RELLENA CON TUS DATOS
// =============================================
const char* ssid = "TU_RED_WIFI";
const char* password = "TU_PASSWORD_WIFI";
const char* serverURL = "http://IP_DE_TU_PC:5000/upload";
const char* serverHost = "IP_DE_TU_PC"; // Solo IP, sin http://
const int serverPort = 5000;

// =============================================
// CONFIGURACIÓN PINES ESP32-CAM (AI-Thinker)
// =============================================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// =============================================
// VARIABLES GLOBALES
// =============================================
unsigned long lastCaptureTime = 0;
const unsigned long CAPTURE_INTERVAL = 10000; // 10 segundos
int failedAttempts = 0;
const int MAX_FAILED_ATTEMPTS = 5;

// =============================================
// SETUP
// =============================================
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("🚀 Iniciando ESP32-CAM Traductor...");
  
  // Conectar WiFi con reintentos
  if (!connectWiFi()) {
    Serial.println("❌ Error crítico: No se pudo conectar a WiFi");
    return;
  }
  
  // Inicializar cámara
  if (!initCamera()) {
    Serial.println("❌ Error crítico: No se pudo inicializar la cámara");
    return;
  }
  
  Serial.println("✅ ESP32-CAM inicializada correctamente");
  Serial.println("📡 Lista para enviar imágenes al servidor");
}

// =============================================
// CONEXIÓN WiFi CON REINTENTOS
// =============================================
bool connectWiFi() {
  Serial.print("📡 Conectando a WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✅ WiFi conectado!");
    Serial.print("📍 IP Address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println();
    Serial.println("❌ Fallo la conexión WiFi");
    return false;
  }
}

// =============================================
// INICIALIZACIÓN DE CÁMARA
// =============================================
bool initCamera() {
  Serial.println("📷 Inicializando cámara...");
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Configuración según memoria disponible
  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA;  // 800x600
    config.jpeg_quality = 10;
    config.fb_count = 2;
    Serial.println("✅ PSRAM detectada - Calidad alta");
  } else {
    config.frame_size = FRAMESIZE_VGA;   // 640x480
    config.jpeg_quality = 12;
    config.fb_count = 1;
    Serial.println("⚠️  Sin PSRAM - Calidad media");
  }
  
  // Inicializar cámara
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("❌ Error inicializando cámara: 0x%x\n", err);
    return false;
  }
  
  // Test rápido de la cámara
  camera_fb_t * test_fb = esp_camera_fb_get();
  if (!test_fb) {
    Serial.println("❌ Error en test de cámara - No se pudo capturar imagen");
    return false;
  }
  Serial.printf("✅ Test cámara OK - Imagen: %d bytes\n", test_fb->len);
  esp_camera_fb_return(test_fb);
  
  return true;
}

// =============================================
// LOOP PRINCIPAL
// =============================================
void loop() {
  unsigned long currentTime = millis();
  
  // Verificar conexión WiFi periódicamente
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️  WiFi desconectado, reconectando...");
    if (!connectWiFi()) {
      delay(5000);
      return;
    }
  }
  
  // Tomar y enviar foto cada intervalo
  if (currentTime - lastCaptureTime >= CAPTURE_INTERVAL || lastCaptureTime == 0) {
    tomarYEnviarFoto();
    lastCaptureTime = currentTime;
  }
  
  delay(1000); // Pequeña pausa entre verificaciones
}

// =============================================
// CAPTURA Y ENVÍO DE FOTOS
// =============================================
void tomarYEnviarFoto() {
  Serial.println("\n📸 Iniciando ciclo de captura...");
  
  // Capturar imagen
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Error: No se pudo capturar imagen");
    failedAttempts++;
    return;
  }
  
  Serial.printf("✅ Imagen capturada: %d bytes\n", fb->len);
  
  // Intentar enviar con múltiples métodos
  bool enviada = false;
  
  // Método 1: HTTPClient estándar
  if (!enviada) {
    Serial.println("🔄 Intentando envío con HTTPClient...");
    enviada = enviarConHTTPClient(fb->buf, fb->len);
  }
  
  // Método 2: WiFiClient directo
  if (!enviada) {
    Serial.println("🔄 Intentando envío con WiFiClient...");
    enviada = enviarConWiFiClient(fb->buf, fb->len);
  }
  
  // Método 3: Stream
  if (!enviada) {
    Serial.println("🔄 Intentando envío con Stream...");
    enviada = enviarConStream(fb->buf, fb->len);
  }
  
  if (enviada) {
    Serial.println("✅ Foto enviada correctamente");
    failedAttempts = 0; // Resetear contador de fallos
  } else {
    failedAttempts++;
    Serial.printf("❌ Fallo en envío (Intentos fallidos: %d)\n", failedAttempts);
    
    if (failedAttempts >= MAX_FAILED_ATTEMPTS) {
      Serial.println("🔄 Reiniciando después de múltiples fallos...");
      ESP.restart();
    }
  }
  
  // Liberar buffer de la cámara
  esp_camera_fb_return(fb);
}

// =============================================
// MÉTODOS DE ENVÍO (MÚLTIPLES ALTERNATIVAS)
// =============================================

// Método 1: HTTPClient estándar
bool enviarConHTTPClient(uint8_t* buffer, size_t length) {
  if (WiFi.status() != WL_CONNECTED) return false;
  
  HTTPClient http;
  bool success = false;
  
  try {
    Serial.printf("🔗 Conectando a: %s\n", serverURL);
    http.begin(serverURL);
    http.addHeader("Content-Type", "image/jpeg");
    http.addHeader("X-ESP32-CAM", "Traductor");
    http.setTimeout(15000);
    
    Serial.println("📤 Enviando datos...");
    int httpCode = http.POST(buffer, length);
    
    if (httpCode > 0) {
      Serial.printf("✅ HTTP Code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        Serial.printf("📨 Respuesta: %s\n", response.c_str());
        success = true;
      }
    } else {
      Serial.printf("❌ HTTP Error: %s\n", http.errorToString(httpCode).c_str());
    }
  } catch (...) {
    Serial.println("❌ Excepción en HTTPClient");
  }
  
  http.end();
  return success;
}

// Método 2: WiFiClient directo
bool enviarConWiFiClient(uint8_t* buffer, size_t length) {
  WiFiClient client;
  bool success = false;
  
  Serial.printf("🔗 Conectando a %s:%d...\n", serverHost, serverPort);
  
  if (client.connect(serverHost, serverPort)) {
    Serial.println("✅ Conexión TCP establecida");
    
    // Construir request HTTP manualmente
    client.println("POST /upload HTTP/1.1");
    client.printf("Host: %s:%d\r\n", serverHost, serverPort);
    client.println("Content-Type: image/jpeg");
    client.println("X-ESP32-CAM: Traductor");
    client.printf("Content-Length: %d\r\n", length);
    client.println("Connection: close");
    client.println();
    
    // Enviar datos de imagen
    size_t sent = client.write(buffer, length);
    Serial.printf("📤 Bytes enviados: %d/%d\n", sent, length);
    
    if (sent == length) {
      // Esperar respuesta
      unsigned long timeout = millis();
      while (client.connected() && millis() - timeout < 10000) {
        if (client.available()) {
          String line = client.readStringUntil('\n');
          if (line.startsWith("HTTP")) {
            Serial.printf("📨 Status: %s\n", line.c_str());
            if (line.indexOf("200") > 0) {
              success = true;
            }
          }
        }
      }
    }
    
    client.stop();
  } else {
    Serial.println("❌ Falló conexión TCP");
  }
  
  return success;
}

// Método 3: Usando Stream (alternativa)
bool enviarConStream(uint8_t* buffer, size_t length) {
  if (WiFi.status() != WL_CONNECTED) return false;
  
  HTTPClient http;
  bool success = false;
  
  try {
    http.begin(serverURL);
    http.addHeader("Content-Type", "image/jpeg");
    
    // Crear un stream con los datos
    WiFiClient *stream = http.getStreamPtr();
    if (stream) {
      int httpCode = http.POST((uint8_t*)buffer, length);
      if (httpCode > 0) {
        success = (httpCode == HTTP_CODE_OK);
        Serial.printf("📨 Stream Response: %d\n", httpCode);
      }
    }
  } catch (...) {
    Serial.println("❌ Excepción en Stream");
  }
  
  http.end();
  return success;
}

// =============================================
// MONITOREO DE ESTADO
// =============================================
void printSystemStatus() {
  Serial.println("\n=== SISTEMA ESP32-CAM ===");
  Serial.printf("WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "CONECTADO" : "DESCONECTADO");
  Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Intentos fallidos: %d/%d\n", failedAttempts, MAX_FAILED_ATTEMPTS);
  Serial.printf("Última captura: %lu ms\n", millis() - lastCaptureTime);
  Serial.println("==========================\n");
}