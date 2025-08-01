#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#include "src/config.h"
#include "src/types.h"
#include "src/storage.h"
#include "src/wifi_manager.h"
#include "src/web_server.h"
#include "src/device_manager.h"
#include "src/rgb_controller.h"
#include "src/mqtt_handler.h"
#include "monitoring.h"

// Variable global para credenciales
WiFiCredentials credentials;

// Estado actual de conexión
ConnectionState currentState = CONFIG_MODE;

// Variables para control de monitor de estado definidas en monitoring.h
unsigned long lastCheckTime = 0;
const unsigned long CHECK_INTERVAL = 30000; // Revisar cada 30 segundos
ConnectionState previousState = CONFIG_MODE;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("SIAMP-G Firmware iniciando...");
  Serial.print("Versión: ");
  Serial.print(FIRMWARE_VERSION);
  Serial.print(" - ID de dispositivo: ");
  Serial.println(DEVICE_ID);
  
  // Inicializar hardware (LED)
  setupHardware();
  
  // Inicializar EEPROM
  initStorage();
  
  // Cargar credenciales guardadas
  loadCredentials(credentials);
    // Verificar si ya está configurado
  if (credentials.configured) {
    Serial.println("Credenciales encontradas, intentando conectar...");
    if (connectToWiFi(credentials)) {
      currentState = CONNECTED;
      setupMainServer(credentials, currentState);
      
      // Inicializar MQTT después de conectarse a la red WiFi
      setupMQTTClient(credentials);
    } else {
      Serial.println("Fallo al conectar, iniciando modo configuración...");
      setupAccessPoint();
      setupConfigServer(credentials, currentState);
      currentState = CONFIG_MODE;
    }
  } else {
    Serial.println("Sin configuración, iniciando modo AP...");
    setupAccessPoint();
    setupConfigServer(credentials, currentState);
    currentState = CONFIG_MODE;
  }
}

void loop() {
  // Importante: procesar las solicitudes del cliente web
  server.handleClient();
  
  // Comprobar si el estado ha cambiado y actualizar servidor si es necesario
  updateServerIfStateChanged();
  
  // Check periódico del estado de conexión
  unsigned long currentTime = millis();
  if (currentTime - lastCheckTime > CHECK_INTERVAL) {
    lastCheckTime = currentTime;
    
    // Verificar el estado de la conexión WiFi si estamos en modo CONNECTED
    if (currentState == CONNECTED && WiFi.status() != WL_CONNECTED) {
      Serial.println("Conexión WiFi perdida, reconectando...");
      if (connectToWiFi(credentials)) {
        Serial.println("Reconectado a WiFi");
        setupMainServer(credentials, currentState); // Reiniciar el servidor web
      } else {
        Serial.println("No se pudo reconectar, cambiando a modo AP");
        currentState = CONFIG_MODE;
        setupAccessPoint();
        setupConfigServer(credentials, currentState);
      }
    }
    
    // Mostrar diagnóstico cada cierto tiempo
    Serial.println("--- Estado actual ---");
    Serial.print("Modo: ");
    Serial.println(currentState == CONFIG_MODE ? "Configuración" : "Normal");
    if (currentState == CONFIG_MODE) {
      Serial.print("AP IP: ");
      Serial.println(WiFi.softAPIP().toString());
      Serial.print("Clientes conectados: ");
      Serial.println(WiFi.softAPgetStationNum());
    } else {
      Serial.print("STA IP: ");
      Serial.println(WiFi.localIP().toString());
      Serial.print("RSSI: ");
      Serial.println(WiFi.RSSI());
      Serial.print("Estado WiFi: ");
      Serial.println(WiFi.status());
    }
    Serial.print("Servidor web en puerto: ");
    Serial.println(SERVER_PORT);
    Serial.print("Memoria libre: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("Tiempo de ejecución: ");
    Serial.println(currentTime / 1000);
    Serial.println("-------------------------");
  }
  
  switch (currentState) {
    case CONFIG_MODE:
      handleConfigMode();
      break;
    case CONNECTING:
      handleConnecting();
      break;
    case CONNECTED:
      handleConnected();
      break;
    case FAILED:
      handleFailed();
      break;
  }
  
  delay(100);
}
