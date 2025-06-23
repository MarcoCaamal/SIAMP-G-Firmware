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
String topicHeartbeat;
unsigned long lastHeartbeatTime = 0;
const unsigned long HEARTBEAT_INTERVAL = 60000; // 60 segundos

// Callback para mensajes MQTT recibidos
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  String topicStr = String(topic);
  String message = "";

  // Convertir el payload a String
  for (unsigned int i = 0; i < length; i++)
  {
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
void setupMQTTClient(const WiFiCredentials &credentials)
{ // Configurar topics basados en el ID del dispositivo
  topicPrefix = String(MQTT_TOPIC_PREFIX) + String(DEVICE_ID) + "/";
  topicState = topicPrefix + "state";
  topicCommand = topicPrefix + "command";
  topicHeartbeat = topicPrefix + "heartbeat";

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
void connectMQTT()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi no conectado, no se puede establecer conexión MQTT");
    return;
  }

  if (mqttClient.connected())
  {
    return; // Ya está conectado
  }

  Serial.print("Conectando a MQTT...");

  // Crear un ID de cliente basado en el ID de dispositivo
  String clientId = "SIAMP-G-";
  clientId += DEVICE_ID;
  clientId += "-";
  clientId += String(random(0xffff), HEX);
  // Intentar conectar con las credenciales hardcodeadas
  if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD))
  {
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
  }
  else
  {
    Serial.print("Fallo en la conexión MQTT, código de error=");
    Serial.print(mqttClient.state());
    Serial.println(" reintento en 5 segundos");
    Serial.print("Servidor: ");
    Serial.println(MQTT_DEFAULT_SERVER);
    Serial.print("Puerto: ");
    Serial.println(MQTT_PORT);
  }
}

// Envía un heartbeat para indicar que el dispositivo está online
void sendHeartbeat()
{
  if (!mqttClient.connected())
  {
    return;
  }

  DynamicJsonDocument doc(256);
  doc["device_id"] = DEVICE_ID;
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();
  doc["uptime"] = millis() / 1000;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["timestamp"] = millis();

  String message;
  serializeJson(doc, message);

  mqttClient.publish(topicHeartbeat.c_str(), message.c_str(), false);
  Serial.println("Heartbeat enviado: " + message);

  lastHeartbeatTime = millis();
}

// Manejar el loop MQTT
void handleMQTTMessages()
{
  unsigned long now = millis();

  // Verificar conexión
  if (!mqttClient.connected())
  {
    static unsigned long lastReconnectAttempt = 0;

    if (now - lastReconnectAttempt > MQTT_RECONNECT_DELAY)
    {
      lastReconnectAttempt = now;
      connectMQTT();
    }
  }
  else
  {
    // Enviar heartbeat periódicamente si está conectado
    if (now - lastHeartbeatTime > HEARTBEAT_INTERVAL)
    {
      sendHeartbeat();
    }
  }

  // Procesar mensajes MQTT pendientes
  mqttClient.loop();
}

// Suscribirse a los topics MQTT
void subscribeMQTTTopics()
{
  mqttClient.subscribe(topicCommand.c_str());
  Serial.println("Suscrito a " + topicCommand);
}

// Publicar el estado actual del LED RGB y del dispositivo
void publishRGBState()
{
  if (!mqttClient.connected())
  {
    return;
  }

  DynamicJsonDocument doc(512); // Aumentado el tamaño para más datos
  RGBState state = getRGBState();

  // Información del LED RGB
  doc["on"] = state.isOn;
  doc["brightness"] = state.brightness;

  JsonObject color = doc.createNestedObject("color");
  color["r"] = state.color.r;
  color["g"] = state.color.g;
  color["b"] = state.color.b;

  // Información adicional del dispositivo
  JsonObject device = doc.createNestedObject("device");
  device["id"] = DEVICE_ID;
  device["name"] = DEVICE_NAME;
  device["firmware"] = FIRMWARE_VERSION;
  device["ip"] = WiFi.localIP().toString();
  device["rssi"] = WiFi.RSSI();       // Intensidad de la señal WiFi
  device["uptime"] = millis() / 1000; // Tiempo de funcionamiento en segundos
  device["mac"] = WiFi.macAddress();
  device["free_heap"] = ESP.getFreeHeap();

  // Estado de conexión
  device["wifi_ssid"] = WiFi.SSID();
  device["connected"] = WiFi.status() == WL_CONNECTED;

  // Timestamp
  doc["timestamp"] = millis();

  String message;
  serializeJson(doc, message);

  mqttClient.publish(topicState.c_str(), message.c_str(), true);
  Serial.println("Estado completo publicado: " + message);
}

// Procesar un comando MQTT
void processMQTTCommand(const String &topic, const String &payload)
{
  if (topic != topicCommand)
  {
    return; // No es un comando para este dispositivo
  }

  // Usar un documento JSON más grande para manejar el formato NestJS
  DynamicJsonDocument docOriginal(1024);
  DeserializationError error = deserializeJson(docOriginal, payload);

  if (error)
  {
    Serial.print("Error al parsear JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // Verificar si el mensaje tiene el formato de NestJS (con pattern y data)
  if (docOriginal.containsKey("pattern") && docOriginal.containsKey("data"))
  {
    Serial.println("Formato NestJS detectado");

    // Parsear el contenido del campo "data" que viene como string JSON
    DynamicJsonDocument docData(512);
    error = deserializeJson(docData, docOriginal["data"].as<String>());

    if (error)
    {
      Serial.print("Error al parsear el campo 'data': ");
      Serial.println(error.c_str());
      return;
    }

    // Usar el documento JSON extraído de "data"
    processCommand(docData);
  }
  else
  {
    // Formato tradicional - usar el documento original
    processCommand(docOriginal);
  }
}

// Función para procesar el comando una vez que tenemos el JSON correcto
void processCommand(JsonDocument &doc)
{
  // Verificar si hay un campo "action" que indica el tipo de comando
  if (doc.containsKey("action"))
  {
    String action = doc["action"].as<String>();

    Serial.print("Comando recibido: ");
    Serial.println(action);

    // Comando para solicitar el estado actual
    if (action == "get_status")
    {
      Serial.println("Enviando estado actual por solicitud");
      publishRGBState();
      return;
    }

    // Comando para forzar un reinicio
    if (action == "restart")
    {
      Serial.println("Reiniciando dispositivo por comando MQTT");
      delay(1000);
      ESP.restart();
      return;
    }

    // Comando para solicitar un heartbeat
    if (action == "ping")
    {
      Serial.println("Solicitud de ping recibida");
      sendHeartbeat();
      return;
    }

    // Comando para desemparejar el dispositivo
    if (action == "unpair")
    {
      Serial.println("Comando de desemparejamiento recibido");
      // Aquí podrías resetear credenciales si es necesario
      // resetCredentials();
      publishRGBState(); // Publicar estado actualizado
      return;
    }

    // Comando de control para RGB
    if (action == "control")
    {
      Serial.println("Comando de control RGB recibido");
      handleRGBControl(doc);
      return;
    }
  }
  else
  {
    // Compatibilidad con el formato anterior (sin campo "action")
    handleRGBControl(doc);
  }
}

// Procesar comandos específicos para el control del LED RGB
void handleRGBControl(JsonDocument &doc)
{
  RGBState currentState = getRGBState();
  bool stateChanged = false;

  // Procesar comando de encendido/apagado
  if (doc.containsKey("on"))
  {
    bool newState = doc["on"].as<bool>();
    if (currentState.isOn != newState)
    {
      currentState.isOn = newState;
      stateChanged = true;
    }
  }

  // Procesar cambio de brillo
  if (doc.containsKey("brightness"))
  {
    uint8_t newBrightness = doc["brightness"].as<uint8_t>();
    // Asegurar que está en el rango 0-100
    newBrightness = constrain(newBrightness, 0, 100);

    if (currentState.brightness != newBrightness)
    {
      currentState.brightness = newBrightness;
      stateChanged = true;
    }
  }  // Procesar cambio de color
  if (doc.containsKey("color"))
  {
    // Accedemos a los valores usando el operador || para valores por defecto
    // en lugar del operador | (OR bit a bit) que puede causar mezclas de color no deseadas
    uint8_t r = doc["color"].containsKey("r") ? doc["color"]["r"].as<uint8_t>() : currentState.color.r;
    uint8_t g = doc["color"].containsKey("g") ? doc["color"]["g"].as<uint8_t>() : currentState.color.g;
    uint8_t b = doc["color"].containsKey("b") ? doc["color"]["b"].as<uint8_t>() : currentState.color.b;

    if (currentState.color.r != r || currentState.color.g != g || currentState.color.b != b)
    {
      currentState.color.r = r;
      currentState.color.g = g;
      currentState.color.b = b;
      stateChanged = true;
      Serial.printf("Nuevo color RGB: (%d,%d,%d)\n", r, g, b);
    }
  }

  // Aplicar cambios si es necesario
  if (stateChanged)
  {
    Serial.println("Aplicando cambios al estado RGB");
    setRGBState(currentState);
    publishRGBState(); // Publicar el nuevo estado
  }
}
