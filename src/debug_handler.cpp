#include <WebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESP.h>
#include "debug_handler.h"
#include "config.h"
#include "wifi_manager.h"
#include "storage.h"
#include "types.h"
#include "rgb_controller.h"

// Referencia al servidor web global definido en web_server.cpp
extern WebServer server;

// Handler para el endpoint de diagnóstico
void handleDebug(const WiFiCredentials &credentials, ConnectionState currentState)
{
  DynamicJsonDocument doc(2048);
  doc["device"] = DEVICE_NAME;
  doc["device_id"] = DEVICE_ID;
  doc["firmware_version"] = FIRMWARE_VERSION;
  doc["current_state"] = getStateString(currentState);
  doc["millis"] = millis();
  
  // Información de sistema
  JsonObject system = doc.createNestedObject("system");
  system["free_heap"] = ESP.getFreeHeap();
  system["total_heap"] = ESP.getHeapSize();
  system["cpu_freq"] = ESP.getCpuFreqMHz();
  system["core_version"] = ESP.getSdkVersion();
  
  // Información de estado y configuración
  JsonObject config = doc.createNestedObject("config");
  config["configured"] = credentials.configured;
  config["ssid"] = credentials.ssid;
  config["server_url"] = credentials.server_url;
  
  // Información del servidor web
  JsonObject webServer = doc.createNestedObject("web_server");
  webServer["active"] = true;
  webServer["port"] = SERVER_PORT;
  
  // Toda la información de WiFi
  JsonObject wifi = doc.createNestedObject("wifi");
  if (currentState == CONFIG_MODE) {
    wifi["mode"] = "AP";
    wifi["ap_ssid"] = AP_SSID;
    wifi["ap_ip"] = WiFi.softAPIP().toString();
    wifi["station_count"] = WiFi.softAPgetStationNum();
  } else {
    wifi["mode"] = "STA";
    wifi["sta_ssid"] = WiFi.SSID();
    wifi["sta_ip"] = WiFi.localIP().toString();
    wifi["gateway"] = WiFi.gatewayIP().toString();
    wifi["subnet"] = WiFi.subnetMask().toString();
    wifi["dns1"] = WiFi.dnsIP(0).toString();
    wifi["dns2"] = WiFi.dnsIP(1).toString();
    wifi["rssi"] = WiFi.RSSI();
    wifi["status_code"] = WiFi.status();
    
    switch (WiFi.status()) {
      case WL_CONNECTED:
        wifi["status"] = "CONNECTED";
        break;
      case WL_DISCONNECTED:
        wifi["status"] = "DISCONNECTED";
        break;
      case WL_CONNECT_FAILED:
        wifi["status"] = "CONNECT_FAILED";
        break;
      case WL_IDLE_STATUS:
        wifi["status"] = "IDLE";
        break;
      case WL_NO_SSID_AVAIL:
        wifi["status"] = "NO_SSID_AVAIL";
        break;
      case WL_CONNECTION_LOST:
        wifi["status"] = "CONNECTION_LOST";
        break;
      default:
        wifi["status"] = "UNKNOWN";
    }
  }
  
  // Lista de rutas registradas
  JsonArray routes = doc.createNestedArray("registered_endpoints");
  routes.add("/");
  routes.add("/status");
  routes.add("/reset");
  routes.add("/debug");
  
  if (currentState == CONFIG_MODE) {
    routes.add("/config");
  } else {
    routes.add("/data");
    routes.add("/info");
    routes.add("/rgb");
  }
  
  // Estado del LED RGB
  RGBState rgbState = getRGBState();
  JsonObject rgb = doc.createNestedObject("rgb_state");
  rgb["on"] = rgbState.isOn;
  rgb["brightness"] = rgbState.brightness;
  
  JsonObject color = rgb.createNestedObject("color");
  color["r"] = rgbState.color.r;
  color["g"] = rgbState.color.g;
  color["b"] = rgbState.color.b;
  
  // Request info
  JsonObject request = doc.createNestedObject("request");
  request["method"] = server.method() == HTTP_GET ? "GET" : "POST";
  request["uri"] = server.uri();
  request["args"] = server.args();
  
  JsonArray headers = request.createNestedArray("headers");
  for (int i = 0; i < server.headers(); i++) {
    JsonObject header = headers.createNestedObject();
    header["name"] = server.headerName(i);
    header["value"] = server.header(i);
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}
