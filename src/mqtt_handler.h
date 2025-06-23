#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include "types.h"
#include "rgb_controller.h"
#include "config.h" // Incluir config.h para acceder a las constantes MQTT

// Funciones para el manejo de MQTT
void setupMQTTClient(const WiFiCredentials &credentials);
void connectMQTT();
void handleMQTTMessages();
void publishRGBState();
void subscribeMQTTTopics();
void processMQTTCommand(const String &topic, const String &payload);

#endif // MQTT_HANDLER_H
