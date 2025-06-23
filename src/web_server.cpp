#include <WebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESP.h>
#include "web_server.h"
#include "config.h"
#include "wifi_manager.h"
#include "storage.h"
#include "types.h"
#include "rgb_controller.h"
#include "mqtt_handler.h"

// Declaramos estas funciones para poder usarlas desde web_server.cpp
extern void publishRGBState();

// Forward declarations para funciones dentro de este archivo
void handleRGBControlWeb();

// Definir la variable global del servidor web (declarada como extern en el .h)
WebServer server(SERVER_PORT);

// Variable para mantener una referencia a las credenciales y al estado actual
WiFiCredentials *globalCredentials;
ConnectionState *globalState;

// Configuración del servidor web en modo configuración
void setupConfigServer(WiFiCredentials &credentials, ConnectionState &currentState)
{
  // Guardar referencias a las variables globales
  globalCredentials = &credentials;
  globalState = &currentState;
  server.on("/", HTTP_GET, []()
            { handleApiInfo(*globalCredentials, *globalState); });
  server.on("/config", HTTP_POST, []()
            { handleConfig(*globalCredentials); });
  server.on("/status", HTTP_GET, []()
            { handleStatus(*globalCredentials, *globalState); });
  server.on("/reset", HTTP_POST, []()
            { handleReset(*globalCredentials); });
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Servidor web iniciado en modo configuración");
}

// Configuración del servidor web en modo normal
void setupMainServer(WiFiCredentials &credentials, ConnectionState &currentState)
{
  // Guardar referencias a las variables globales
  globalCredentials = &credentials;
  globalState = &currentState;

  Serial.println("Configurando servidor principal...");
  server.on("/", HTTP_GET, []()
            { handleApiInfo(*globalCredentials, *globalState); });
  server.on("/data", HTTP_POST, handleData);
  server.on("/info", HTTP_GET, []()
            { handleInfo(*globalCredentials); });
  server.on("/status", HTTP_GET, []()
            { handleStatus(*globalCredentials, *globalState); });  
  server.on("/reset", HTTP_POST, []()
            { handleReset(*globalCredentials); });
  server.on("/rgb", HTTP_POST, []()
            { handleRGBControlWeb(); });
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Servidor principal iniciado");
}

// API Info endpoint - Reemplaza la página principal
void handleApiInfo(const WiFiCredentials &credentials, ConnectionState currentState)
{
  DynamicJsonDocument doc(1024);
  doc["device"] = DEVICE_NAME;
  doc["firmware_version"] = FIRMWARE_VERSION;
  doc["device_id"] = DEVICE_ID;
  doc["state"] = getStateString(currentState);
  doc["mode"] = (currentState == CONFIG_MODE) ? "configuration" : "normal";

  if (currentState == CONFIG_MODE)
  {
    doc["message"] = "Device in configuration mode";
    doc["ap_ssid"] = AP_SSID;
    doc["ap_ip"] = WiFi.softAPIP().toString();
    doc["configured"] = credentials.configured;
    // Información sobre endpoints disponibles
    JsonArray endpoints = doc.createNestedArray("endpoints");
    endpoints.add("/config [POST] - Set WiFi configuration");
    endpoints.add("/status [GET] - Get device status");
    endpoints.add("/reset [POST] - Reset configuration");
  }
  else
  {
    doc["message"] = "Device in normal operation mode";
    doc["ip"] = WiFi.localIP().toString();
    doc["mac"] = WiFi.macAddress();
    doc["ssid"] = credentials.ssid;
    doc["server_url"] = credentials.server_url;
    doc["uptime"] = millis();
    doc["free_heap"] = ESP.getFreeHeap();

    // Información sobre endpoints disponibles
    JsonArray endpoints = doc.createNestedArray("endpoints");
    endpoints.add("/data [POST] - Send data to device");
    endpoints.add("/info [GET] - Get device information");
    endpoints.add("/status [GET] - Get device status");
    endpoints.add("/reset [POST] - Reset configuration");
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// Manejar configuración recibida
void handleConfig(WiFiCredentials &credentials)
{
  if (server.hasArg("plain"))
  {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, server.arg("plain"));

    String ssid = doc["ssid"];
    String password = doc["password"];
    String server_url = doc["server_url"];

    if (ssid.length() > 0 && password.length() > 0 && server_url.length() > 0)
    {
      // Guardar credenciales
      strncpy(credentials.ssid, ssid.c_str(), sizeof(credentials.ssid) - 1);
      strncpy(credentials.password, password.c_str(), sizeof(credentials.password) - 1);
      strncpy(credentials.server_url, server_url.c_str(), sizeof(credentials.server_url) - 1);
      credentials.configured = true;

      saveCredentials(credentials);

      server.send(200, "application/json", "{\"success\": true, \"message\": \"Configuración guardada\"}");

      Serial.println("Nueva configuración recibida:");
      Serial.println("SSID: " + ssid);
      Serial.println("Server URL: " + server_url);

      delay(2000);
      ESP.restart();
    }
    else
    {
      server.send(400, "application/json", "{\"success\": false, \"message\": \"Datos incompletos\"}");
    }
  }
  else
  {
    server.send(400, "application/json", "{\"success\": false, \"message\": \"No se recibieron datos\"}");
  }
}

// Estado del dispositivo - Mejorado con más información
void handleStatus(const WiFiCredentials &credentials, ConnectionState currentState)
{
  DynamicJsonDocument doc(1024);
  doc["device"] = DEVICE_NAME;
  doc["firmware_version"] = FIRMWARE_VERSION;
  doc["device_id"] = DEVICE_ID;
  doc["state"] = getStateString(currentState);
  doc["uptime"] = millis();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["configured"] = credentials.configured;

  if (currentState == CONFIG_MODE)
  {
    doc["mode"] = "configuration";
    doc["ap_ssid"] = AP_SSID;
    doc["ap_ip"] = WiFi.softAPIP().toString();
    doc["connected_clients"] = WiFi.softAPgetStationNum();
  }
  else
  {
    doc["mode"] = "normal";
    doc["wifi_ssid"] = credentials.ssid;
    doc["wifi_ip"] = WiFi.localIP().toString();
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["mac_address"] = WiFi.macAddress();
    doc["server_url"] = credentials.server_url;

    // Agregar estado del LED RGB
    RGBState rgbState = getRGBState();
    JsonObject rgb = doc.createNestedObject("rgb");
    rgb["on"] = rgbState.isOn;
    rgb["brightness"] = rgbState.brightness;

    JsonObject color = rgb.createNestedObject("color");
    color["r"] = rgbState.color.r;
    color["g"] = rgbState.color.g;
    color["b"] = rgbState.color.b;
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// Reset de configuración - Mejorado con confirmación
void handleReset(WiFiCredentials &credentials)
{
  DynamicJsonDocument requestDoc(256);

  if (server.hasArg("plain"))
  {
    deserializeJson(requestDoc, server.arg("plain"));
  }

  // Verificar confirmación (opcional pero recomendado)
  bool confirmed = requestDoc["confirm"] | false;

  if (confirmed || !server.hasArg("plain"))
  {
    credentials.configured = false;
    memset(&credentials, 0, sizeof(credentials));
    saveCredentials(credentials);

    DynamicJsonDocument responseDoc(256);
    responseDoc["success"] = true;
    responseDoc["message"] = "Configuration reset successfully";
    responseDoc["action"] = "Device will restart in 3 seconds";

    String response;
    serializeJson(responseDoc, response);
    server.send(200, "application/json", response);

    Serial.println("Configuration reset requested");
    delay(3000);
    ESP.restart();
  }
  else
  {
    DynamicJsonDocument responseDoc(256);
    responseDoc["success"] = false;
    responseDoc["message"] = "Reset requires confirmation";
    responseDoc["required"] = "Send POST with {\"confirm\": true}";

    String response;
    serializeJson(responseDoc, response);
    server.send(400, "application/json", response);
  }
}

// Manejar datos en modo normal - Mejorado
void handleData()
{
  if (!server.hasArg("plain"))
  {
    DynamicJsonDocument errorDoc(256);
    errorDoc["success"] = false;
    errorDoc["error"] = "NO_DATA";
    errorDoc["message"] = "No JSON data received";

    String response;
    serializeJson(errorDoc, response);
    server.send(400, "application/json", response);
    return;
  }

  DynamicJsonDocument receivedDoc(1024);
  DeserializationError error = deserializeJson(receivedDoc, server.arg("plain"));

  if (error)
  {
    DynamicJsonDocument errorDoc(256);
    errorDoc["success"] = false;
    errorDoc["error"] = "INVALID_JSON";
    errorDoc["message"] = "Invalid JSON format";

    String response;
    serializeJson(errorDoc, response);
    server.send(400, "application/json", response);
    return;
  }

  // Procesar datos recibidos
  Serial.println("Data received:");
  serializeJsonPretty(receivedDoc, Serial);

  // Respuesta exitosa
  DynamicJsonDocument responseDoc(512);
  responseDoc["success"] = true;
  responseDoc["message"] = "Data processed successfully";
  responseDoc["timestamp"] = millis();
  responseDoc["received_fields"] = receivedDoc.size();

  // Echo de algunos campos importantes si están presentes
  if (receivedDoc.containsKey("command"))
  {
    responseDoc["command_received"] = receivedDoc["command"];
  }
  if (receivedDoc.containsKey("data"))
  {
    responseDoc["data_size"] = receivedDoc["data"].size();
  }

  String response;
  serializeJson(responseDoc, response);
  server.send(200, "application/json", response);
}

// Información del dispositivo - Expandida
void handleInfo(const WiFiCredentials &credentials)
{
  DynamicJsonDocument doc(1024);
  doc["device"] = DEVICE_NAME;
  doc["firmware_version"] = FIRMWARE_VERSION;
  doc["device_id"] = DEVICE_ID;
  doc["compile_date"] = __DATE__;
  doc["compile_time"] = __TIME__;

  // Información de red
  JsonObject network = doc.createNestedObject("network");
  network["ip"] = WiFi.localIP().toString();
  network["mac"] = WiFi.macAddress();
  network["ssid"] = WiFi.SSID();
  network["rssi"] = WiFi.RSSI();
  network["gateway"] = WiFi.gatewayIP().toString();
  network["dns"] = WiFi.dnsIP().toString();

  // Información del sistema
  JsonObject system = doc.createNestedObject("system");
  system["uptime"] = millis();
  system["free_heap"] = ESP.getFreeHeap();
  system["total_heap"] = ESP.getHeapSize();
  system["cpu_freq"] = ESP.getCpuFreqMHz();
  system["flash_size"] = ESP.getFlashChipSize();
  system["chip_model"] = ESP.getChipModel();
  system["chip_revision"] = ESP.getChipRevision();

  // Configuración
  JsonObject config = doc.createNestedObject("configuration");
  config["server_url"] = credentials.server_url;
  config["configured"] = credentials.configured;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// Control del LED RGB a través de API web
void handleRGBControl()
{
  if (!server.hasArg("plain"))
  {
    server.send(400, "application/json", "{\"success\": false, \"message\": \"No se recibieron datos\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error)
  {
    server.send(400, "application/json", "{\"success\": false, \"message\": \"Error en formato JSON\"}");
    return;
  }

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
  }

  // Procesar cambio de color
  if (doc.containsKey("color"))
  {
    JsonObject color = doc["color"];
    uint8_t r = color["r"] | currentState.color.r;
    uint8_t g = color["g"] | currentState.color.g;
    uint8_t b = color["b"] | currentState.color.b;

    if (currentState.color.r != r || currentState.color.g != g || currentState.color.b != b)
    {
      currentState.color.r = r;
      currentState.color.g = g;
      currentState.color.b = b;
      stateChanged = true;
    }
  }

  // Aplicar cambios si es necesario
  if (stateChanged)
  {
    setRGBState(currentState);
    // Publicar el nuevo estado por MQTT
    publishRGBState();
  }

  // Devolver el estado actual
  DynamicJsonDocument responseDoc(256);
  responseDoc["success"] = true;
  responseDoc["on"] = currentState.isOn;
  responseDoc["brightness"] = currentState.brightness;

  JsonObject responseColor = responseDoc.createNestedObject("color");
  responseColor["r"] = currentState.color.r;
  responseColor["g"] = currentState.color.g;
  responseColor["b"] = currentState.color.b;

  String response;
  serializeJson(responseDoc, response);
  server.send(200, "application/json", response);
}

// Manejar control RGB desde la API web
void handleRGBControlWeb()
{
  if (server.hasArg("plain"))
  {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error) {
      server.send(400, "application/json", "{\"success\": false, \"message\": \"JSON inválido\"}");
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
      uint8_t r = doc["color"]["r"] | currentState.color.r;
      uint8_t g = doc["color"]["g"] | currentState.color.g;
      uint8_t b = doc["color"]["b"] | currentState.color.b;
      
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
      publishRGBState(); // Publicar cambios por MQTT
    }
    
    // Enviar respuesta
    DynamicJsonDocument responseDoc(256);
    responseDoc["success"] = true;
    responseDoc["message"] = "Control RGB aplicado";
    
    String response;
    serializeJson(responseDoc, response);
    server.send(200, "application/json", response);
  }
  else {
    server.send(400, "application/json", "{\"success\": false, \"message\": \"No se recibieron datos\"}");
  }
}

void handleNotFound()
{
  server.send(404, "text/plain", "404: Página no encontrada");
}
