#ifndef STORAGE_H
#define STORAGE_H

#include "types.h"

// Funciones para manejar el almacenamiento EEPROM
void initStorage();
void loadCredentials(WiFiCredentials &credentials);
void saveCredentials(WiFiCredentials &credentials);
void resetCredentials();

#endif // STORAGE_H
