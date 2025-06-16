#include <Arduino.h>
#include "device_manager.h"
#include "web_server.h"
#include "wifi_manager.h"
#include "config.h"

// Variable para controlar el estado del LED
static bool ledState = false;

void setupHardware() {
  // Configurar el pin del LED como salida
  pinMode(STATUS_LED_PIN, OUTPUT);
  
  // Inicialmente apagar el LED
  digitalWrite(STATUS_LED_PIN, LOW);
  
  Serial.println("Hardware inicializado");
}

void handleConfigMode() {
  // Parpadear LED para indicar modo configuración
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > CONFIG_MODE_BLINK_INTERVAL) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(STATUS_LED_PIN, ledState);
  }
}

void handleConnecting() {
  // Parpadeo rápido de LED para indicar intento de conexión
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > CONNECTING_BLINK_INTERVAL) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(STATUS_LED_PIN, ledState);
  }
}

void handleConnected() {
  // LED con parpadeo muy lento para indicar estado conectado
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > CONNECTED_BLINK_INTERVAL) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(STATUS_LED_PIN, ledState);
  }
  
  // Aquí podrías implementar lógica para enviar datos periódicamente al servidor
}

void handleFailed() {
  // Para fallo de conexión, mantener el LED apagado
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // Código para manejar fallo de conexión
  Serial.println("Conexión fallida, volviendo a modo configuración...");
  setupAccessPoint();
}
