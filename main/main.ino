#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// Configuración del Access Point
const char* ap_ssid = "SIAMP-G-Config";
const char* ap_password = "12345678";

// Configuración del servidor web
WebServer server(80);

// Estructura para almacenar credenciales WiFi
struct WiFiCredentials {
  char ssid[32];
  char password[64];
  char server_url[128];
  bool configured;
};

WiFiCredentials credentials;

// Direcciones EEPROM
const int EEPROM_SIZE = 512;
const int CREDENTIALS_ADDRESS = 0;

// Estados de conexión
enum ConnectionState {
  CONFIG_MODE,
  CONNECTING,
  CONNECTED,
  FAILED
};

ConnectionState currentState = CONFIG_MODE;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("SIAMP-G Firmware iniciando...");
  
  // Inicializar EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // Cargar credenciales guardadas
  loadCredentials();
  
  // Verificar si ya está configurado
  if (credentials.configured) {
    Serial.println("Credenciales encontradas, intentando conectar...");
    if (connectToWiFi()) {
      currentState = CONNECTED;
      setupMainServer();
    } else {
      Serial.println("Fallo al conectar, iniciando modo configuración...");
      setupConfigMode();
    }
  } else {
    Serial.println("Sin configuración, iniciando modo AP...");
    setupConfigMode();
  }
}

void loop() {
  server.handleClient();
  
  switch (currentState) {
    case CONFIG_MODE:
      handleConfigMode();
      break;
    case CONNECTING:
      handleConnecting();
      break;
    case CONNECTED:
      handleConnected();
      break;
    case FAILED:
      handleFailed();
      break;
  }
  
  delay(100);
}

void setupConfigMode() {
  Serial.println("Configurando Access Point...");
  
  // Configurar como Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
    // Configurar rutas del servidor web - Solo JSON API
  server.on("/", HTTP_GET, handleApiInfo);
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/networks", HTTP_GET, handleNetworkScan);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("Servidor web iniciado en modo configuración");
  
  currentState = CONFIG_MODE;
}

void setupMainServer() {  Serial.println("Configurando servidor principal...");
  
  // Configurar rutas para modo normal - Solo JSON API
  server.on("/", HTTP_GET, handleApiInfo);
  server.on("/data", HTTP_POST, handleData);
  server.on("/info", HTTP_GET, handleInfo);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/reset", HTTP_POST, handleReset);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("Servidor principal iniciado");
}

// API Info endpoint - Reemplaza la página principal
void handleApiInfo() {
  DynamicJsonDocument doc(1024);
  doc["device"] = "SIAMP-G";
  doc["firmware_version"] = "1.0.0";
  doc["state"] = getStateString(currentState);
  doc["mode"] = (currentState == CONFIG_MODE) ? "configuration" : "normal";
  
  if (currentState == CONFIG_MODE) {
    doc["message"] = "Device in configuration mode";
    doc["ap_ssid"] = ap_ssid;
    doc["ap_ip"] = WiFi.softAPIP().toString();
    doc["configured"] = credentials.configured;
    
    // Información sobre endpoints disponibles
    JsonArray endpoints = doc.createNestedArray("endpoints");
    endpoints.add("/config [POST] - Set WiFi configuration");
    endpoints.add("/status [GET] - Get device status");
    endpoints.add("/networks [GET] - Scan available networks");
    endpoints.add("/reset [POST] - Reset configuration");
  } else {
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
void handleConfig() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, server.arg("plain"));
    
    String ssid = doc["ssid"];
    String password = doc["password"];
    String server_url = doc["server_url"];
    
    if (ssid.length() > 0 && password.length() > 0 && server_url.length() > 0) {
      // Guardar credenciales
      strncpy(credentials.ssid, ssid.c_str(), sizeof(credentials.ssid) - 1);
      strncpy(credentials.password, password.c_str(), sizeof(credentials.password) - 1);
      strncpy(credentials.server_url, server_url.c_str(), sizeof(credentials.server_url) - 1);
      credentials.configured = true;
      
      saveCredentials();
      
      server.send(200, "application/json", "{\"success\": true, \"message\": \"Configuración guardada\"}");
      
      Serial.println("Nueva configuración recibida:");
      Serial.println("SSID: " + ssid);
      Serial.println("Server URL: " + server_url);
      
      delay(2000);
      ESP.restart();
    } else {
      server.send(400, "application/json", "{\"success\": false, \"message\": \"Datos incompletos\"}");
    }
  } else {
    server.send(400, "application/json", "{\"success\": false, \"message\": \"No se recibieron datos\"}");
  }
}

// Estado del dispositivo - Mejorado con más información
void handleStatus() {
  DynamicJsonDocument doc(1024);
  doc["device"] = "SIAMP-G";
  doc["firmware_version"] = "1.0.0";
  doc["state"] = getStateString(currentState);
  doc["uptime"] = millis();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["configured"] = credentials.configured;
  
  if (currentState == CONFIG_MODE) {
    doc["mode"] = "configuration";
    doc["ap_ssid"] = ap_ssid;
    doc["ap_ip"] = WiFi.softAPIP().toString();
    doc["connected_clients"] = WiFi.softAPgetStationNum();
  } else {
    doc["mode"] = "normal";
    doc["wifi_ssid"] = credentials.ssid;
    doc["wifi_ip"] = WiFi.localIP().toString();
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["mac_address"] = WiFi.macAddress();
    doc["server_url"] = credentials.server_url;
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// Reset de configuración - Mejorado con confirmación
void handleReset() {
  DynamicJsonDocument requestDoc(256);
  
  if (server.hasArg("plain")) {
    deserializeJson(requestDoc, server.arg("plain"));
  }
  
  // Verificar confirmación (opcional pero recomendado)
  bool confirmed = requestDoc["confirm"] | false;
  
  if (confirmed || !server.hasArg("plain")) {
    credentials.configured = false;
    memset(&credentials, 0, sizeof(credentials));
    saveCredentials();
    
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
  } else {
    DynamicJsonDocument responseDoc(256);
    responseDoc["success"] = false;
    responseDoc["message"] = "Reset requires confirmation";
    responseDoc["required"] = "Send POST with {\"confirm\": true}";
    
    String response;
    serializeJson(responseDoc, response);
    server.send(400, "application/json", response);
  }
}

// Escaneo de redes WiFi disponibles
void handleNetworkScan() {
  Serial.println("Scanning WiFi networks...");
  
  DynamicJsonDocument doc(2048);
  doc["scanning"] = true;
  doc["message"] = "Scanning for available networks...";
  
  int networks = WiFi.scanNetworks();
  
  if (networks == 0) {
    doc["scanning"] = false;
    doc["count"] = 0;
    doc["message"] = "No networks found";
  } else {
    doc["scanning"] = false;
    doc["count"] = networks;
    doc["message"] = "Networks found";
    
    JsonArray networkArray = doc.createNestedArray("networks");
    
    for (int i = 0; i < networks; i++) {
      JsonObject network = networkArray.createNestedObject();
      network["ssid"] = WiFi.SSID(i);
      network["rssi"] = WiFi.RSSI(i);
      network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "encrypted";
      network["channel"] = WiFi.channel(i);
    }
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
  
  WiFi.scanDelete(); // Limpiar resultados del scan
}

// Manejar datos en modo normal - Mejorado
void handleData() {
  if (!server.hasArg("plain")) {
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
  
  if (error) {
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
  if (receivedDoc.containsKey("command")) {
    responseDoc["command_received"] = receivedDoc["command"];
  }
  if (receivedDoc.containsKey("data")) {
    responseDoc["data_size"] = receivedDoc["data"].size();
  }
  
  String response;
  serializeJson(responseDoc, response);
  server.send(200, "application/json", response);
}

// Información del dispositivo - Expandida
void handleInfo() {
  DynamicJsonDocument doc(1024);
  doc["device"] = "SIAMP-G";
  doc["firmware_version"] = "1.0.0";
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

void handleNotFound() {
  server.send(404, "text/plain", "404: Página no encontrada");
}

bool connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(credentials.ssid, credentials.password);
  
  Serial.println("Conectando a WiFi...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi conectado!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println();
    Serial.println("Fallo al conectar a WiFi");
    return false;
  }
}

void loadCredentials() {
  EEPROM.get(CREDENTIALS_ADDRESS, credentials);
  
  // Verificar si los datos son válidos
  if (credentials.configured != true && credentials.configured != false) {
    // Datos corruptos, inicializar
    memset(&credentials, 0, sizeof(credentials));
    credentials.configured = false;
  }
}

void saveCredentials() {
  EEPROM.put(CREDENTIALS_ADDRESS, credentials);
  EEPROM.commit();
  Serial.println("Credenciales guardadas en EEPROM");
}

String getStateString(ConnectionState state) {
  switch (state) {
    case CONFIG_MODE: return "CONFIG_MODE";
    case CONNECTING: return "CONNECTING";
    case CONNECTED: return "CONNECTED";
    case FAILED: return "FAILED";
    default: return "UNKNOWN";
  }
}

void handleConfigMode() {
  // Parpadear LED para indicar modo configuración
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    // Aquí puedes agregar código para parpadear un LED
  }
}

void handleConnecting() {
  // Código para manejar estado de conexión
}

void handleConnected() {
  // Código para manejar estado conectado
  // Aquí puedes agregar lógica para enviar datos al servidor
}

void handleFailed() {
  // Código para manejar fallo de conexión
  Serial.println("Conexión fallida, volviendo a modo configuración...");
  setupConfigMode();
}