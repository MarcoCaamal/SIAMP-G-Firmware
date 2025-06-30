#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "types.h"

// Inicialización del hardware
void setupHardware();

// Funciones para manejar los estados del dispositivo
void handleConfigMode();
void handleConnecting();
void handleConnected();
void handleFailed();

#endif // DEVICE_MANAGER_H
