#include "esp_camera.h"
#include <WiFi.h>

// =====================================================
//      SELECCIONA EL MODELO DE TU CÁMARA (DESCOMENTA UNA)
// =====================================================
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER // ¡Esta es la más común! Asegúrate de que esté descomentada.

#include "camera_pins.h"

// =====================================================
//      INGRESA TUS CREDENCIALES DE WIFI AQUÍ
// =====================================================
const char* ssid = "camaraarduino";
const char* password = "skibiditoilet";

// Función para iniciar el servidor de la cámara (viene con el ejemplo)
void startCameraServer();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Configuración de la cámara
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
  
  // Configuración para placas con PSRAM (la mayoría de las ESP32-CAM la tienen)
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // Calidad alta
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA; // Calidad media si no hay PSRAM
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Inicializar la cámara
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Fallo al iniciar la cámara con error 0x%x", err);
    return;
  }

  // Conexión a la red WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado");

  // Iniciar el servidor de streaming
  startCameraServer();

  // Imprimir la dirección IP para acceder a la cámara
  Serial.print("Servidor de la cámara iniciado en: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  // El programa principal se ejecuta aquí, pero para este ejemplo,
  // todo se gestiona en el servidor web que se inicia en el setup.
  delay(10000);
}