#include <EEPROM.h>
#include "config.h"
#include "storage.h"
#include "types.h"
#include <Arduino.h>

void initStorage() {
  EEPROM.begin(EEPROM_SIZE);
  Serial.println("EEPROM inicializada");
}

void loadCredentials(WiFiCredentials &credentials) {
  EEPROM.get(CREDENTIALS_ADDRESS, credentials);
  
  // Verificar si los datos son válidos
  if (credentials.configured != true && credentials.configured != false) {
    // Datos corruptos, inicializar
    memset(&credentials, 0, sizeof(credentials));
    credentials.configured = false;
    Serial.println("Credenciales no válidas, inicializando...");
  } else {
    Serial.println("Credenciales cargadas desde EEPROM");
  }
}

void saveCredentials(WiFiCredentials &credentials) {
  EEPROM.put(CREDENTIALS_ADDRESS, credentials);
  EEPROM.commit();
  Serial.println("Credenciales guardadas en EEPROM");
}

void resetCredentials() {
  WiFiCredentials emptyCredentials;
  memset(&emptyCredentials, 0, sizeof(emptyCredentials));
  emptyCredentials.configured = false;
  
  EEPROM.put(CREDENTIALS_ADDRESS, emptyCredentials);
  EEPROM.commit();
  Serial.println("Credenciales borradas de EEPROM");
}
