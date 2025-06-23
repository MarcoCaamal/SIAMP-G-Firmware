#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "mqtt_handler.h"
#include "rgb_controller.h"
#include "config.h"
#include "types.h"

// Cliente MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Topics MQTT
String topicPrefix;
String topicState;
String topicCommand;

// Callback para mensajes MQTT recibidos
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  String message = "";
  
  // Convertir el payload a String
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
  
  // Procesar el mensaje
  processMQTTCommand(topicStr, message);
}

// Configurar cliente MQTT
void setupMQTTClient(const WiFiCredentials &credentials) {
  // Configurar topics basados en el ID del dispositivo
  topicPrefix = String(MQTT_TOPIC_PREFIX) + String(DEVICE_ID) + "/";
  topicState = topicPrefix + "state";
  topicCommand = topicPrefix + "command";
  
  // Siempre usar el servidor MQTT hardcodeado
  Serial.println("Configurando cliente MQTT con valores hardcodeados:");
  Serial.print("Servidor MQTT: ");
  Serial.println(MQTT_DEFAULT_SERVER);
  Serial.print("Puerto MQTT: ");
  Serial.println(MQTT_PORT);
  Serial.print("Usuario MQTT: ");
  Serial.println(MQTT_USERNAME);
  
  // Configurar cliente MQTT
  mqttClient.setServer(MQTT_DEFAULT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  
  // Intentar conexión inicial
  connectMQTT();
}

// Conectar al servidor MQTT
void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi no conectado, no se puede establecer conexión MQTT");
    return;
  }
  
  if (mqttClient.connected()) {
    return; // Ya está conectado
  }
  
  Serial.print("Conectando a MQTT...");
  
  // Crear un ID de cliente basado en el ID de dispositivo
  String clientId = "SIAMP-G-";
  clientId += DEVICE_ID;
  clientId += "-";
  clientId += String(random(0xffff), HEX);
    // Intentar conectar con las credenciales hardcodeadas
  if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.println("Conectado al broker MQTT");
    Serial.print("Servidor: ");
    Serial.println(MQTT_DEFAULT_SERVER);
    Serial.print("Puerto: ");
    Serial.println(MQTT_PORT);
    Serial.print("Usuario: ");
    Serial.println(MQTT_USERNAME);
    
    // Suscripción a los topics relevantes
    subscribeMQTTTopics();
    
    // Publicar estado actual
    publishRGBState();
  } else {
    Serial.print("Fallo en la conexión MQTT, código de error=");
    Serial.print(mqttClient.state());
    Serial.println(" reintento en 5 segundos");
    Serial.print("Servidor: ");
    Serial.println(MQTT_DEFAULT_SERVER);
    Serial.print("Puerto: ");
    Serial.println(MQTT_PORT);
  }
}

// Manejar el loop MQTT
void handleMQTTMessages() {
  // Verificar conexión
  if (!mqttClient.connected()) {
    static unsigned long lastReconnectAttempt = 0;
    unsigned long now = millis();
    
    if (now - lastReconnectAttempt > MQTT_RECONNECT_DELAY) {
      lastReconnectAttempt = now;
      connectMQTT();
    }
  }
  
  // Procesar mensajes MQTT pendientes
  mqttClient.loop();
}

// Suscribirse a los topics MQTT
void subscribeMQTTTopics() {
  mqttClient.subscribe(topicCommand.c_str());
  Serial.println("Suscrito a " + topicCommand);
}

// Publicar el estado actual del LED RGB
void publishRGBState() {
  if (!mqttClient.connected()) {
    return;
  }
  
  DynamicJsonDocument doc(256);
  RGBState state = getRGBState();
  
  doc["on"] = state.isOn;
  doc["brightness"] = state.brightness;
  
  JsonObject color = doc.createNestedObject("color");
  color["r"] = state.color.r;
  color["g"] = state.color.g;
  color["b"] = state.color.b;
  
  String message;
  serializeJson(doc, message);
  
  mqttClient.publish(topicState.c_str(), message.c_str(), true);
  Serial.println("Estado publicado: " + message);
}

// Procesar un comando MQTT
void processMQTTCommand(const String &topic, const String &payload) {
  if (topic != topicCommand) {
    return; // No es un comando para este dispositivo
  }
  
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    Serial.print("Error al parsear JSON: ");
    Serial.println(error.c_str());
    return;
  }
  
  RGBState currentState = getRGBState();
  bool stateChanged = false;
  
  // Procesar comando de encendido/apagado
  if (doc.containsKey("on")) {
    bool newState = doc["on"].as<bool>();
    if (currentState.isOn != newState) {
      currentState.isOn = newState;
      stateChanged = true;
    }
  }
  
  // Procesar cambio de brillo
  if (doc.containsKey("brightness")) {
    uint8_t newBrightness = doc["brightness"].as<uint8_t>();
    // Asegurar que está en el rango 0-100
    newBrightness = constrain(newBrightness, 0, 100);
    
    if (currentState.brightness != newBrightness) {
      currentState.brightness = newBrightness;
      stateChanged = true;
    }
  }
  
  // Procesar cambio de color
  if (doc.containsKey("color")) {
    JsonObject color = doc["color"];
    uint8_t r = color["r"] | currentState.color.r;
    uint8_t g = color["g"] | currentState.color.g;
    uint8_t b = color["b"] | currentState.color.b;
    
    if (currentState.color.r != r || currentState.color.g != g || currentState.color.b != b) {
      currentState.color.r = r;
      currentState.color.g = g;
      currentState.color.b = b;
      stateChanged = true;
    }
  }
  
  // Aplicar cambios si es necesario
  if (stateChanged) {
    setRGBState(currentState);
    publishRGBState(); // Publicar el nuevo estado
  }
}
