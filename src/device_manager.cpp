#include <Arduino.h>
#include "device_manager.h"
#include "web_server.h"
#include "wifi_manager.h"

void handleConfigMode() {
  // Parpadear LED para indicar modo configuración
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    // Aquí puedes agregar código para parpadear un LED
  }
}

void handleConnecting() {
  // Código para manejar estado de conexión
  // Por ejemplo, parpadeo de LED más rápido
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 200) {
    lastBlink = millis();
    // Aquí puedes agregar código para parpadear un LED más rápido
  }
}

void handleConnected() {
  // Código para manejar estado conectado
  // Aquí puedes agregar lógica para enviar datos al servidor
  
  // Por ejemplo, un LED encendido fijo o parpadeo lento para indicar actividad
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 3000) {
    lastBlink = millis();
    // Aquí puedes agregar código para parpadear un LED lentamente
  }
  
  // Aquí podrías implementar lógica para enviar datos periódicamente al servidor
}

void handleFailed() {
  // Código para manejar fallo de conexión
  Serial.println("Conexión fallida, volviendo a modo configuración...");
  setupAccessPoint();
  
  // Aquí podrías agregar código para indicar fallo con LED
}
