#ifndef TYPES_H
#define TYPES_H

// Estructura para almacenar credenciales WiFi
struct WiFiCredentials {
  char ssid[33];
  char password[65];
  char server_url[129];
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
