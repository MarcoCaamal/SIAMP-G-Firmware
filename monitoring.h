#ifndef MONITORING_H
#define MONITORING_H

#include "src/types.h"

// Referencias a variables globales definidas en el archivo principal
extern WiFiCredentials credentials;
extern ConnectionState currentState;

// Variables para control de monitor de estado
extern unsigned long lastCheckTime;
extern const unsigned long CHECK_INTERVAL; // Revisar cada 30 segundos
extern ConnectionState previousState;

void updateServerIfStateChanged() {
  // Si el estado cambió, reconfiguramos el servidor web
  if (previousState != currentState) {
    Serial.print("Estado cambiado de ");
    Serial.print(getStateString(previousState));
    Serial.print(" a ");
    Serial.println(getStateString(currentState));
    
    // Asegurar que siempre se establezcan las referencias globales correctamente
    if (currentState == CONNECTED) {
      Serial.println("Configurando servidor en modo normal (CONNECTED)");
      setupMainServer(credentials, currentState);
    } else {
      Serial.println("Configurando servidor en modo configuración (CONFIG_MODE)");
      setupConfigServer(credentials, currentState);
    }
    
    // Imprimir información de estado para depuración
    Serial.print("Servidor web corriendo en puerto: ");
    Serial.println(SERVER_PORT);
    if (currentState == CONFIG_MODE) {
      Serial.print("IP AP: ");
      Serial.println(WiFi.softAPIP().toString());
    } else {
      Serial.print("IP STA: ");
      Serial.println(WiFi.localIP().toString());
    }
    
    previousState = currentState;
  }
}

#endif // MONITORING_H
