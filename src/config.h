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
const char* const DEVICE_ID = "SG-00001"; // ID único del dispositivo (hardcodeado)

// Tiempos y configuraciones de red
const int WIFI_CONNECT_TIMEOUT = 10000; // 10 segundos
const int WIFI_MAX_ATTEMPTS = 20;
const int SERVER_PORT = 80;

// Configuración de hardware
const int STATUS_LED_PIN = 2;  // Pin para el LED de estado (GPIO2 es el LED incorporado en muchos ESP32)
const int CONFIG_MODE_BLINK_INTERVAL = 500;  // Intervalo de parpadeo en modo configuración (ms)
const int CONNECTING_BLINK_INTERVAL = 200;   // Intervalo de parpadeo en modo conexión (ms)
const int CONNECTED_BLINK_INTERVAL = 3000;   // Intervalo de parpadeo en modo conectado (ms)

// Configuración del LED RGB
const int RGB_RED_PIN = 25;    // Pin para el color rojo del LED RGB
const int RGB_GREEN_PIN = 26;  // Pin para el color verde del LED RGB
const int RGB_BLUE_PIN = 27;   // Pin para el color azul del LED RGB
const int PWM_RESOLUTION = 8;  // Resolución PWM (8 bits = 0-255)
const int PWM_FREQUENCY = 5000; // Frecuencia PWM en Hz

// Configuración MQTT
const char* const MQTT_DEFAULT_SERVER = "192.168.0.13";  // IP de tu máquina donde corre el broker MQTT
const int MQTT_PORT = 1883;                                   // Puerto MQTT estándar
const char* const MQTT_TOPIC_PREFIX = "siamp-g/";             // Prefijo para los topics MQTT
const int MQTT_RECONNECT_DELAY = 5000;                        // Tiempo entre intentos de reconexión (ms)
const char* const MQTT_USERNAME = "siamp_server";             // Usuario MQTT del broker
const char* const MQTT_PASSWORD = "S14MP_G_mqtt_2025";        // Contraseña MQTT del broker

#endif // CONFIG_H
