#ifndef TYPES_H
#define TYPES_H

// Estructura para almacenar credenciales WiFi
struct WiFiCredentials {
  char ssid[32];
  char password[64];
  char server_url[128];
  bool configured;
};

// Estados de conexión
enum ConnectionState {
  CONFIG_MODE,
  CONNECTING,
  CONNECTED,
  FAILED
};

#endif // TYPES_H
