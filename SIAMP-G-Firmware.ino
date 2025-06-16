#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

#include "src/config.h"
#include "src/types.h"
#include "src/storage.h"
#include "src/wifi_manager.h"
#include "src/web_server.h"
#include "src/device_manager.h"

// Variable global para credenciales
WiFiCredentials credentials;

// Estado actual de conexión
ConnectionState currentState = CONFIG_MODE;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("SIAMP-G Firmware iniciando...");
  
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
  server.handleClient();
  
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
