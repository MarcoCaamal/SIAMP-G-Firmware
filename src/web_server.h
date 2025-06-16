#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include "types.h"

extern WebServer server;

// Funciones para manejar el servidor web
void setupConfigServer(WiFiCredentials &credentials, ConnectionState &currentState);
void setupMainServer(WiFiCredentials &credentials, ConnectionState &currentState);

// Handlers de solicitudes HTTP
void handleApiInfo(const WiFiCredentials &credentials, ConnectionState currentState);
void handleConfig(WiFiCredentials &credentials);
void handleStatus(const WiFiCredentials &credentials, ConnectionState currentState);
void handleReset(WiFiCredentials &credentials);
void handleData();
void handleInfo(const WiFiCredentials &credentials);
void handleNotFound();

#endif // WEB_SERVER_H
