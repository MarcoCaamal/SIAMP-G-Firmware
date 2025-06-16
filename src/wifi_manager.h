#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include "types.h"

// Funciones para gestionar WiFi
void setupAccessPoint();
bool connectToWiFi(const WiFiCredentials &credentials);
String getStateString(ConnectionState state);
void scanNetworks(JsonDocument &doc);

#endif // WIFI_MANAGER_H
