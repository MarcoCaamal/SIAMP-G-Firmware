#include <WiFi.h>
#include <ArduinoJson.h>
#include "wifi_manager.h"
#include "config.h"
#include "types.h"

void setupAccessPoint() {
  Serial.println("Configurando Access Point...");
  
  // Configurar como Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

bool connectToWiFi(const WiFiCredentials &credentials) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(credentials.ssid, credentials.password);
  
  Serial.println("Conectando a WiFi...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < WIFI_MAX_ATTEMPTS) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi conectado!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println();
    Serial.println("Fallo al conectar a WiFi");
    return false;
  }
}

String getStateString(ConnectionState state) {
  switch (state) {
    case CONFIG_MODE: return "CONFIG_MODE";
    case CONNECTING: return "CONNECTING";
    case CONNECTED: return "CONNECTED";
    case FAILED: return "FAILED";
    default: return "UNKNOWN";
  }
}

void scanNetworks(JsonDocument &doc) {
  Serial.println("Scanning WiFi networks...");
  
  doc["scanning"] = true;
  doc["message"] = "Scanning for available networks...";
  
  int networks = WiFi.scanNetworks();
  
  if (networks == 0) {
    doc["scanning"] = false;
    doc["count"] = 0;
    doc["message"] = "No networks found";
  } else {
    doc["scanning"] = false;
    doc["count"] = networks;
    doc["message"] = "Networks found";
    
    JsonArray networkArray = doc.createNestedArray("networks");
    
    for (int i = 0; i < networks; i++) {
      JsonObject network = networkArray.createNestedObject();
      network["ssid"] = WiFi.SSID(i);
      network["rssi"] = WiFi.RSSI(i);
      network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "encrypted";
      network["channel"] = WiFi.channel(i);
    }
  }
  
  WiFi.scanDelete(); // Limpiar resultados del scan
}
