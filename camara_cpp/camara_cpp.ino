#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_camera.h"

// =============================================
// CONFIGURACIÓN SIMPLE Y DIRECTA
// =============================================
const char* ssid = "IPM-Wifi";
const char* password = "ipm-wifi";
const char* serverIP = "172.16.1.149";  // Tu IP actual
const int serverPort = 5000;

// =============================================
// PINES ESP32-CAM
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
const unsigned long CAPTURE_INTERVAL = 15000; // 15 segundos
int failedAttempts = 0;

// =============================================
// SETUP
// =============================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n🚀 ESP32-CAM TRADUCTOR");
  Serial.println("=====================\n");
  
  // 1. Conectar WiFi
  Serial.println("📡 Conectando WiFi...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi CONECTADO");
    Serial.print("📍 IP Local: ");
    Serial.println(WiFi.localIP());
    Serial.print("🌐 Servidor: ");
    Serial.print(serverIP);
    Serial.print(":");
    Serial.println(serverPort);
  } else {
    Serial.println("\n❌ WiFi FALLÓ");
    ESP.restart();
  }
  
  // 2. Inicializar cámara
  Serial.println("\n📷 Inicializando cámara...");
  if (!initCamera()) {
    Serial.println("❌ Cámara falló - Reiniciando...");
    ESP.restart();
  }
  
  // 3. Probar servidor
  Serial.println("\n🔍 Probando servidor...");
  testServer();
  
  Serial.println("\n✅ Sistema listo!");
  Serial.println("=====================\n");
}

// =============================================
// LOOP PRINCIPAL
// =============================================
void loop() {
  static unsigned long lastStatus = 0;
  
  // Verificar WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️  Reconectando WiFi...");
    WiFi.reconnect();
    delay(3000);
    return;
  }
  
  // Enviar foto cada intervalo
  if (millis() - lastCaptureTime >= CAPTURE_INTERVAL) {
    captureAndSend();
    lastCaptureTime = millis();
  }
  
  // Mostrar estado cada 10 segundos
  if (millis() - lastStatus >= 10000) {
    Serial.print("📊 Estado: WiFi=");
    Serial.print(WiFi.status() == WL_CONNECTED ? "OK" : "NO");
    Serial.print(", RSSI=");
    Serial.print(WiFi.RSSI());
    Serial.print("dBm, Próxima=");
    Serial.print((CAPTURE_INTERVAL - (millis() - lastCaptureTime)) / 1000);
    Serial.println("s");
    lastStatus = millis();
  }
  
  delay(1000);
}

// =============================================
// INICIALIZAR CÁMARA
// =============================================
bool initCamera() {
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
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 15;
    config.fb_count = 1;
  }
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Error cámara: 0x%x\n", err);
    return false;
  }
  
  return true;
}

// =============================================
// PROBAR SERVIDOR
// =============================================
void testServer() {
  Serial.print("Probando conexión a ");
  Serial.print(serverIP);
  Serial.print(":");
  Serial.print(serverPort);
  Serial.println("...");
  
  WiFiClient client;
  client.setTimeout(3000);
  
  if (client.connect(serverIP, serverPort)) {
    Serial.println("✅ Conexión TCP exitosa");
    
    // Enviar request GET
    client.print("GET /status HTTP/1.1\r\n");
    client.print("Host: ");
    client.print(serverIP);
    client.print(":");
    client.print(serverPort);
    client.print("\r\nConnection: close\r\n\r\n");
    
    // Esperar respuesta
    unsigned long timeout = millis();
    while (!client.available() && millis() - timeout < 5000) {
      delay(10);
    }
    
    if (client.available()) {
      String response = client.readString();
      if (response.indexOf("200") > 0 || response.indexOf("online") > 0) {
        Serial.println("✅ Servidor respondió correctamente");
      } else {
        Serial.println("⚠️  Servidor respondió pero no es nuestro");
        Serial.println(response.substring(0, 200));
      }
    } else {
      Serial.println("⚠️  Servidor no respondió (pero conexión OK)");
    }
    
    client.stop();
  } else {
    Serial.println("❌ No se pudo conectar al servidor");
    Serial.println("💡 Verifica:");
    Serial.println("   1. Servidor Python corriendo");
    Serial.println("   2. Firewall desactivado");
    Serial.println("   3. IP correcta: 172.16.1.149");
  }
}

// =============================================
// CAPTURAR Y ENVIAR FOTO
// =============================================
void captureAndSend() {
  Serial.println("\n📸 Capturando imagen...");
  
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Error capturando");
    failedAttempts++;
    return;
  }
  
  Serial.printf("✅ Imagen: %d bytes\n", fb->len);
  
  String url = "http://" + String(serverIP) + ":" + String(serverPort) + "/upload";
  Serial.print("📤 Enviando a: ");
  Serial.println(url);
  
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "image/jpeg");
  http.addHeader("X-ESP32-CAM", "Traductor");
  http.setTimeout(10000);
  
  int httpCode = http.POST(fb->buf, fb->len);
  
  if (httpCode > 0) {
    Serial.printf("📨 HTTP Code: %d\n", httpCode);
    
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_ACCEPTED) {
      String response = http.getString();
      Serial.print("✅ Éxito! Respuesta: ");
      Serial.println(response);
      failedAttempts = 0;
    } else {
      Serial.printf("❌ Error HTTP: %s\n", http.errorToString(httpCode).c_str());
      failedAttempts++;
    }
  } else {
    Serial.printf("❌ Error conexión: %s\n", http.errorToString(httpCode).c_str());
    failedAttempts++;
  }
  
  http.end();
  esp_camera_fb_return(fb);
  
  // Si muchos fallos, reiniciar
  if (failedAttempts >= 5) {
    Serial.println("🔄 Demasiados fallos - Reiniciando...");
    ESP.restart();
  }
}
