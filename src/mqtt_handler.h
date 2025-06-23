#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
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
void processCommand(JsonDocument &doc);
void sendHeartbeat();
void handleRGBControl(JsonDocument &doc);

#endif // MQTT_HANDLER_H
