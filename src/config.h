#ifndef CONFIG_H
#define CONFIG_H

// Configuración del Access Point
const char* const AP_SSID = "SIAMP-G-Config";
const char* const AP_PASSWORD = "12345678";

// Direcciones EEPROM
const int EEPROM_SIZE = 512;
const int CREDENTIALS_ADDRESS = 0;

// Configuración del dispositivo
const char* const DEVICE_NAME = "SIAMP-G";
const char* const FIRMWARE_VERSION = "1.0.0";

// Tiempos y configuraciones de red
const int WIFI_CONNECT_TIMEOUT = 10000; // 10 segundos
const int WIFI_MAX_ATTEMPTS = 20;
const int SERVER_PORT = 80;

// Configuración de hardware
const int STATUS_LED_PIN = 2;  // Pin para el LED de estado (GPIO2 es el LED incorporado en muchos ESP32)
const int CONFIG_MODE_BLINK_INTERVAL = 500;  // Intervalo de parpadeo en modo configuración (ms)
const int CONNECTING_BLINK_INTERVAL = 200;   // Intervalo de parpadeo en modo conexión (ms)
const int CONNECTED_BLINK_INTERVAL = 3000;   // Intervalo de parpadeo en modo conectado (ms)

#endif // CONFIG_H
