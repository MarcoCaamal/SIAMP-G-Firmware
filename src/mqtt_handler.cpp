#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "mqtt_handler.h"
#include "rgb_controller.h"
#include "config.h"
#include "types.h"
#include "storage.h"

// Prototipo de función para diagnóstico de errores MQTT
void printMQTTError(int errorCode);

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
    Serial.println("MQTT ya está conectado, usando conexión existente");
    return; // Ya está conectado
  }

  Serial.println("Conectando a MQTT...");

  // Verificar el estado actual del cliente MQTT
  int currentState = mqttClient.state();
  Serial.print("Estado actual del cliente MQTT antes de conectar: ");
  printMQTTError(currentState);

  // Crear un ID de cliente basado en el ID de dispositivo
  String clientId = "SIAMP-G-";
  clientId += DEVICE_ID;
  clientId += "-";
  clientId += String(random(0xffff), HEX);

  Serial.print("Usando Client ID: ");
  Serial.println(clientId);

  // Establecer una última voluntad (will message) para notificar cuando se desconecta
  String willTopic = topicPrefix + "status";
  String willMessage = "{\"status\":\"offline\",\"device_id\":\"" + String(DEVICE_ID) + "\"}";

  // Intentar conectar con las credenciales y last will
  Serial.print("Intentando conectar con credenciales al broker MQTT... ");

  bool connectResult;

  // Intentar la conexión con will message
  connectResult = mqttClient.connect(
      clientId.c_str(),
      MQTT_USERNAME,
      MQTT_PASSWORD,
      willTopic.c_str(),
      0,    // QoS 0
      true, // Retain
      willMessage.c_str());

  if (connectResult)
  {
    Serial.println("¡Conectado con éxito al broker MQTT!");
    Serial.print("Servidor: ");
    Serial.println(MQTT_DEFAULT_SERVER);
    Serial.print("Puerto: ");
    Serial.println(MQTT_PORT);
    Serial.print("Usuario: ");
    Serial.println(MQTT_USERNAME);
    Serial.print("Will topic: ");
    Serial.println(willTopic);

    // Publicar mensaje de presencia
    String presenceMessage = "{\"status\":\"online\",\"device_id\":\"" + String(DEVICE_ID) + "\"}";
    mqttClient.publish(willTopic.c_str(), presenceMessage.c_str(), true);
    Serial.println("Mensaje de presencia publicado");

    // Suscripción a los topics relevantes
    subscribeMQTTTopics();

    // Esperar un poco para estabilizar la conexión antes de publicar el estado
    delay(500);

    // Publicar estado actual
    publishRGBState();
  }
  else
  {
    int errorCode = mqttClient.state();
    Serial.print("Fallo en la conexión MQTT, código de error=");
    Serial.print(errorCode);
    printMQTTError(errorCode);
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
    Serial.println("No se puede publicar estado: MQTT no está conectado");
    return;
  }

  // Aumentar el tamaño del documento para asegurar que tiene suficiente espacio
  DynamicJsonDocument doc(1024);
  RGBState state = getRGBState();

  // Información del LED RGB
  doc["on"] = state.isOn;
  doc["brightness"] = state.brightness;

  JsonObject color = doc.createNestedObject("color");
  color["mode"] = "rgb"; // Agregar modo para compatibilidad con NestJS

  // Crear un objeto rgb anidado para el formato que espera el cliente móvil
  JsonObject rgb = color.createNestedObject("rgb");
  rgb["r"] = state.color.r;
  rgb["g"] = state.color.g;
  rgb["b"] = state.color.b;

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

  // Usar currentState como la propiedad esperada por la API
  doc["currentState"] = state.isOn ? "on" : "off";

  String message;
  serializeJson(doc, message);

  // Verificar el tamaño del mensaje antes de enviarlo
  Serial.print("Tamaño del mensaje: ");
  Serial.print(message.length());
  Serial.println(" bytes");

  // Publicar el mensaje - primero con un mensaje reducido por si hay límite de tamaño
  Serial.println("Intentando publicar mensaje reducido primero...");

  // Crear un mensaje mínimo para asegurar la entrega
  DynamicJsonDocument smallerDoc(256);
  smallerDoc["on"] = state.isOn;
  smallerDoc["brightness"] = state.brightness;
  smallerDoc["currentState"] = state.isOn ? "on" : "off";

  // Estructura básica del color
  JsonObject smallColor = smallerDoc.createNestedObject("color");
  smallColor["r"] = state.color.r;
  smallColor["g"] = state.color.g;
  smallColor["b"] = state.color.b;

  // También usar el formato de NestJS para compatibilidad
  smallColor["mode"] = "rgb";
  JsonObject smallRgb = smallColor.createNestedObject("rgb");
  smallRgb["r"] = state.color.r;
  smallRgb["g"] = state.color.g;
  smallRgb["b"] = state.color.b;

  // Solo información esencial del dispositivo
  smallerDoc["device_id"] = DEVICE_ID;

  String smallerMessage;
  serializeJson(smallerDoc, smallerMessage);

  Serial.print("Tamaño del mensaje reducido: ");
  Serial.print(smallerMessage.length());
  Serial.println(" bytes");

  // Intentar publicar el mensaje reducido primero
  bool success = mqttClient.publish(topicState.c_str(), smallerMessage.c_str(), true);

  if (success)
  {
    Serial.println("Estado reducido publicado con éxito");

    // Comprobar estado de la conexión
    Serial.print("Estado del cliente MQTT: ");
    int mqttState = mqttClient.state();
    printMQTTError(mqttState);

    // Verificar que la conexión sigue activa
    if (mqttClient.connected())
    {
      Serial.println("Cliente MQTT sigue conectado");

      // Intenta publicar el mensaje completo con información de diagnóstico
      Serial.println("Publicando información adicional del dispositivo en un topic separado...");
      String diagnosticTopic = topicPrefix + "diagnostic";

      // Crear un documento JSON con datos de diagnóstico
      DynamicJsonDocument diagDoc(256);
      diagDoc["ip"] = WiFi.localIP().toString();
      diagDoc["rssi"] = WiFi.RSSI();
      diagDoc["uptime"] = millis() / 1000;
      diagDoc["mac"] = WiFi.macAddress();
      diagDoc["free_heap"] = ESP.getFreeHeap();
      diagDoc["wifi_ssid"] = WiFi.SSID();
      diagDoc["wifi_status"] = WiFi.status();

      String diagMessage;
      serializeJson(diagDoc, diagMessage);

      // Publicar diagnóstico en un topic separado
      if (mqttClient.publish(diagnosticTopic.c_str(), diagMessage.c_str(), false))
      {
        Serial.println("Información de diagnóstico publicada con éxito");
      }
      else
      {
        Serial.print("Error al publicar diagnóstico: ");
        printMQTTError(mqttClient.state());
      }
    }
    else
    {
      Serial.println("¡Cliente MQTT desconectado después de publicar!");
      connectMQTT(); // Intentar reconectar si se ha desconectado
    }
  }
  else
  {
    Serial.print("Error al publicar el estado reducido. Código de error: ");
    printMQTTError(mqttClient.state());

    // Diagnosticar problemas de conexión
    if (!mqttClient.connected())
    {
      Serial.println("Cliente MQTT desconectado, intentando reconectar");
      delay(1000); // Esperar un segundo antes de intentar reconectar
      connectMQTT();
    }
    else
    {
      // Si está conectado pero no puede publicar, puede ser un problema de formato o permisos
      Serial.println("Conectado pero no puede publicar. Verificando topic y permisos...");
      Serial.print("Topic: ");
      Serial.println(topicState);
      Serial.print("Longitud del mensaje: ");
      Serial.println(smallerMessage.length());

      // Último recurso: intentar publicar un mensaje muy pequeño sin formato JSON
      String miniMessage = "{\"on\":" + String(state.isOn ? "true" : "false") + "}";
      if (mqttClient.publish(topicState.c_str(), miniMessage.c_str()))
      {
        Serial.println("Mensaje mínimo publicado con éxito");
      }
      else
      {
        Serial.println("Fallo total en la publicación, puede ser un problema de permisos o configuración del broker");
      }
    }
  }
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
      
      // Publicar mensaje de estado antes de desemparejar
      DynamicJsonDocument unpairDoc(256);
      unpairDoc["device_id"] = DEVICE_ID;
      unpairDoc["status"] = "unpaired";
      unpairDoc["timestamp"] = millis();
      
      String unpairMessage;
      serializeJson(unpairDoc, unpairMessage);
      
      // Publicar en un topic específico de desemparejamiento
      String unpairTopic = topicPrefix + "unpair";
      mqttClient.publish(unpairTopic.c_str(), unpairMessage.c_str(), true);
      
      Serial.println("Mensaje de desemparejamiento enviado");
      Serial.println("Preparando para reiniciar el dispositivo...");
      
      // Limpiar las credenciales almacenadas
      resetCredentials();
      Serial.println("Credenciales WiFi borradas");
      
      // Pequeña pausa y luego reiniciar
      delay(2000);
      ESP.restart();
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
    Serial.println("Procesando cambio de brillo");
    uint8_t newBrightness = doc["brightness"].as<uint8_t>();
    // Asegurar que está en el rango 0-100
    newBrightness = constrain(newBrightness, 0, 100);

    if (currentState.brightness != newBrightness)
    {
      currentState.brightness = newBrightness;
      stateChanged = true;
    }
  } // Procesar cambio de color
  if (doc.containsKey("color"))
  {
    Serial.println("Procesando cambio de color RGB");

    // Variables para los nuevos valores RGB
    uint8_t r = currentState.color.r;
    uint8_t g = currentState.color.g;
    uint8_t b = currentState.color.b;
    bool colorFound = false;

    // Verificar el formato de NestJS (color.mode y color.rgb)
    if (doc["color"].containsKey("mode") && doc["color"]["mode"] == "rgb" && doc["color"].containsKey("rgb"))
    {
      Serial.println("Formato color.rgb.{r,g,b} detectado");

      // Acceder a los valores RGB dentro del objeto "rgb"
      if (doc["color"]["rgb"].containsKey("r"))
      {
        r = doc["color"]["rgb"]["r"].as<uint8_t>();
        colorFound = true;
      }

      if (doc["color"]["rgb"].containsKey("g"))
      {
        g = doc["color"]["rgb"]["g"].as<uint8_t>();
        colorFound = true;
      }

      if (doc["color"]["rgb"].containsKey("b"))
      {
        b = doc["color"]["rgb"]["b"].as<uint8_t>();
        colorFound = true;
      }

      Serial.printf("Valores RGB extraídos del formato anidado: (%d,%d,%d)\n", r, g, b);
    }
    // Verificar el formato tradicional (color.r, color.g, color.b)
    else if (doc["color"].containsKey("r") || doc["color"].containsKey("g") || doc["color"].containsKey("b"))
    {
      Serial.println("Formato tradicional color.{r,g,b} detectado");

      // Acceder directamente a las propiedades r, g, b del objeto color
      if (doc["color"].containsKey("r"))
      {
        r = doc["color"]["r"].as<uint8_t>();
        colorFound = true;
      }

      if (doc["color"].containsKey("g"))
      {
        g = doc["color"]["g"].as<uint8_t>();
        colorFound = true;
      }

      if (doc["color"].containsKey("b"))
      {
        b = doc["color"]["b"].as<uint8_t>();
        colorFound = true;
      }

      Serial.printf("Valores RGB extraídos del formato tradicional: (%d,%d,%d)\n", r, g, b);
    }
    else
    {
      Serial.println("No se encontró un formato de color RGB válido");
    }

    // Si se encontraron valores de color y son diferentes a los actuales, actualizar
    if (colorFound && (currentState.color.r != r || currentState.color.g != g || currentState.color.b != b))
    {
      currentState.color.r = r;
      currentState.color.g = g;
      currentState.color.b = b;
      stateChanged = true;
      Serial.printf("Nuevo color RGB aplicado: (%d,%d,%d)\n", r, g, b);
    }
  }

  // Aplicar cambios si es necesario
  Serial.println("Verificando cambios en el estado RGB");
  if (stateChanged)
  {
    Serial.println("Aplicando cambios al estado RGB");
    setRGBState(currentState);
    publishRGBState(); // Publicar el nuevo estado
  }
}

// Función para diagnosticar errores MQTT
void printMQTTError(int errorCode)
{
  Serial.print("MQTT Error: ");
  switch (errorCode)
  {
  case -4:
    Serial.println("MQTT_CONNECTION_TIMEOUT - El servidor no respondió dentro del intervalo de keep-alive");
    break;
  case -3:
    Serial.println("MQTT_CONNECTION_LOST - La conexión de red se interrumpió");
    break;
  case -2:
    Serial.println("MQTT_CONNECT_FAILED - La conexión de red falló");
    break;
  case -1:
    Serial.println("MQTT_DISCONNECTED - El cliente está desconectado");
    break;
  case 0:
    Serial.println("MQTT_CONNECTED - El cliente está conectado");
    break;
  case 1:
    Serial.println("MQTT_CONNECT_BAD_PROTOCOL - El servidor no soporta la versión solicitada del protocolo MQTT");
    break;
  case 2:
    Serial.println("MQTT_CONNECT_BAD_CLIENT_ID - El servidor rechazó el identificador del cliente");
    break;
  case 3:
    Serial.println("MQTT_CONNECT_UNAVAILABLE - El servidor no pudo aceptar la conexión");
    break;
  case 4:
    Serial.println("MQTT_CONNECT_BAD_CREDENTIALS - El usuario/contraseña fueron rechazados");
    break;
  case 5:
    Serial.println("MQTT_CONNECT_UNAUTHORIZED - El cliente no está autorizado a conectarse");
    break;
  default:
    Serial.println("Código de error MQTT desconocido");
  }
}
