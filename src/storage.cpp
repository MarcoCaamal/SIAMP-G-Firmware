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

  // Validar que el valor de 'configured' sea razonable (true o false)
  if (credentials.configured != true && credentials.configured != false) {
    memset(&credentials, 0, sizeof(credentials));
    credentials.configured = false;
    Serial.println("❌ Credenciales no válidas. Inicializando...");
    return;
  }

  // Asegurar que las cadenas estén null-terminated
  credentials.ssid[sizeof(credentials.ssid) - 1] = '\0';
  credentials.password[sizeof(credentials.password) - 1] = '\0';

  Serial.println("✅ Credenciales cargadas desde EEPROM:");
  Serial.print("SSID: ");
  Serial.println(credentials.ssid);
  Serial.print("Password: ");
  Serial.println(credentials.password);
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
