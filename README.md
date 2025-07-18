# SIAMP-G Firmware

Firmware para ESP32 del sistema SIAMP-G con API REST completa en JSON. No incluye interfaces HTML.

## Estructura del Proyecto (Nueva Organización)

```
SIAMP-G-Firmware/
├── src/                    # Carpeta principal del código fuente
│   ├── main.cpp            # Archivo principal (punto de entrada)
│   ├── config.h            # Configuraciones y constantes
│   ├── types.h             # Definiciones de tipos y estructuras
│   ├── storage.cpp/h       # Manejo de almacenamiento EEPROM
│   ├── wifi_manager.cpp/h  # Gestión de WiFi
│   ├── web_server.cpp/h    # Manejo del servidor web
│   └── device_manager.cpp/h # Manejo de estados del dispositivo
├── include/                # Archivos de cabecera adicionales
└── lib/                    # Bibliotecas personalizadas (si las necesitas)
```

## Módulos Principales

### 1. Storage (storage.cpp/h)
Manejo del almacenamiento persistente en EEPROM para guardar las credenciales WiFi y otras configuraciones.

### 2. WiFi Manager (wifi_manager.cpp/h)
Gestión de la conexión WiFi, modo AP, escaneo de redes, etc.

### 3. Web Server (web_server.cpp/h)
Implementación del servidor web y manejo de rutas API.

### 4. Device Manager (device_manager.cpp/h)
Manejo de los diferentes estados del dispositivo y acciones asociadas.

## Características

- **Modo de Configuración (AP)**: El ESP32 crea un punto de acceso WiFi para configuración inicial
- **API REST Completa**: Toda la comunicación se realiza mediante JSON
- **Almacenamiento Persistente**: Las configuraciones se guardan en EEPROM
- **Auto-conexión**: Conecta automáticamente a la red WiFi configurada
- **Escaneo de Redes**: Endpoint para escanear redes WiFi disponibles
- **Información Detallada**: Endpoints con información completa del sistema

## Configuración del Entorno

### Requisitos
- Arduino IDE 2.0+ o PlatformIO
- ESP32 Board Package
- Librerías necesarias:
  - WiFi (incluida en ESP32 Core)
  - WebServer (incluida en ESP32 Core)
  - EEPROM (incluida en ESP32 Core)
  - ArduinoJson (instalar desde Library Manager)

### Instalación en Arduino IDE

1. **Instalar ESP32 Board Package**:
   - Ir a `File > Preferences`
   - Agregar URL: `https://dl.espressif.com/dl/package_esp32_index.json`
   - Ir a `Tools > Board > Boards Manager`
   - Buscar "ESP32" e instalar

2. **Instalar Librerías**:
   - Ir a `Tools > Manage Libraries`
   - Buscar e instalar "ArduinoJson" by Benoit Blanchon

3. **Configurar Board**:
   - Seleccionar `Tools > Board > ESP32 Arduino > ESP32 Dev Module`
   - Configurar puerto COM correspondiente

## Funcionamiento

### Primer Arranque
1. El ESP32 arranca en modo Access Point
2. Red WiFi: `SIAMP-G-Config`
3. Contraseña: `12345678`
4. IP del dispositivo: `192.168.4.1`

### Configuración Inicial via API
1. Conectarse a la red WiFi `SIAMP-G-Config`
2. Hacer una petición GET a `http://192.168.4.1/` para obtener información del dispositivo
3. Enviar configuración con POST `http://192.168.4.1/config`

### Operación Normal
- El ESP32 se conecta automáticamente a la red WiFi configurada
- API REST disponible en la IP asignada por el router
- Endpoints disponibles para comunicación con el servidor

## API Endpoints

### Modo Configuración (AP Mode)
- `GET /` - Información del dispositivo y endpoints disponibles
- `POST /config` - Configurar credenciales WiFi y servidor
- `GET /status` - Estado detallado del dispositivo
- `POST /reset` - Resetear configuración (requiere confirmación)

### Modo Normal (WiFi Conectado)
- `GET /` - Información del dispositivo y endpoints disponibles
- `POST /data` - Recibir y procesar datos JSON
- `GET /info` - Información completa del dispositivo y sistema
- `GET /status` - Estado detallado del dispositivo
- `POST /reset` - Resetear configuración (requiere confirmación)

## Ejemplos de Uso

### 1. Obtener información del dispositivo
```bash
curl -X GET http://192.168.4.1/
```

**Respuesta en modo configuración:**
```json
{
  "device": "SIAMP-G",
  "firmware_version": "1.0.0",
  "state": "CONFIG_MODE",
  "mode": "configuration",
  "message": "Device in configuration mode",
  "ap_ssid": "SIAMP-G-Config",
  "ap_ip": "192.168.4.1",
  "configured": false,
  "endpoints": [
    "/config [POST] - Set WiFi configuration",
    "/status [GET] - Get device status",
    "/networks [GET] - Scan available networks",
    "/reset [POST] - Reset configuration"
  ]
}
```

### 2. Escanear redes WiFi disponibles
```bash
curl -X GET http://192.168.4.1/networks
```

**Respuesta:**
```json
{
  "scanning": false,
  "count": 3,
  "message": "Networks found",
  "networks": [
    {
      "ssid": "MiRedWiFi",
      "rssi": -45,
      "encryption": "encrypted",
      "channel": 6
    },
    {
      "ssid": "WiFi_Vecino",
      "rssi": -67,
      "encryption": "encrypted",
      "channel": 11
    },
    {
      "ssid": "Red_Abierta",
      "rssi": -78,
      "encryption": "open",
      "channel": 1
    }
  ]
}
```

### 3. Configurar WiFi y servidor
```bash
curl -X POST http://192.168.4.1/config \
  -H "Content-Type: application/json" \
  -d '{
    "ssid": "MiRedWiFi",
    "password": "MiContraseña123",
    "server_url": "http://192.168.1.100:3000"
  }'
```

**Respuesta:**
```json
{
  "success": true,
  "message": "Configuración guardada"
}
```

### 4. Obtener estado del dispositivo
```bash
curl -X GET http://192.168.1.105/status
```

**Respuesta en modo normal:**
```json
{
  "device": "SIAMP-G",
  "firmware_version": "1.0.0",
  "state": "CONNECTED",
  "uptime": 123456789,
  "free_heap": 250000,
  "configured": true,
  "mode": "normal",
  "wifi_ssid": "MiRedWiFi",
  "wifi_ip": "192.168.1.105",
  "wifi_rssi": -45,
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "server_url": "http://192.168.1.100:3000"
}
```

### 5. Enviar datos al dispositivo
```bash
curl -X POST http://192.168.1.105/data \
  -H "Content-Type: application/json" \
  -d '{
    "command": "update_config",
    "data": {
      "sensor_interval": 5000,
      "led_brightness": 80
    }
  }'
```

**Respuesta:**
```json
{
  "success": true,
  "message": "Data processed successfully",
  "timestamp": 123456789,
  "received_fields": 2,
  "command_received": "update_config",
  "data_size": 2
}
```

### 6. Obtener información completa del sistema
```bash
curl -X GET http://192.168.1.105/info
```

**Respuesta:**
```json
{
  "device": "SIAMP-G",
  "firmware_version": "1.0.0",
  "compile_date": "Jun  8 2025",
  "compile_time": "10:30:00",
  "network": {
    "ip": "192.168.1.105",
    "mac": "AA:BB:CC:DD:EE:FF",
    "ssid": "MiRedWiFi",
    "rssi": -45,
    "gateway": "192.168.1.1",
    "dns": "192.168.1.1"
  },
  "system": {
    "uptime": 123456789,
    "free_heap": 250000,
    "total_heap": 327680,
    "cpu_freq": 240,
    "flash_size": 4194304,
    "chip_model": "ESP32-D0WDQ6",
    "chip_revision": 1
  },
  "configuration": {
    "server_url": "http://192.168.1.100:3000",
    "configured": true
  }
}
```

### 7. Resetear configuración
```bash
curl -X POST http://192.168.1.105/reset \
  -H "Content-Type: application/json" \
  -d '{"confirm": true}'
```

**Respuesta:**
```json
{
  "success": true,
  "message": "Configuration reset successfully",
  "action": "Device will restart in 3 seconds"
}
```

## Códigos de Error

### Errores de Configuración
- `400` - Datos incompletos o formato JSON inválido
- `404` - Endpoint no encontrado

### Ejemplo de respuesta de error:
```json
{
  "success": false,
  "error": "INVALID_JSON",
  "message": "Invalid JSON format"
}
```

### Ejemplo de endpoint no encontrado:
```json
{
  "error": "NOT_FOUND",
  "message": "Endpoint not found",
  "method": "GET",
  "uri": "/invalid-endpoint"
}
```

## Estados del Sistema

- **CONFIG_MODE**: Modo configuración (Access Point activo)
- **CONNECTING**: Intentando conectar a WiFi
- **CONNECTED**: Conectado a WiFi y operativo
- **FAILED**: Fallo de conexión (vuelve a modo configuración)

## Integración con Aplicaciones

### Python Example
```python
import requests
import json

# Configurar dispositivo
config_data = {
    "ssid": "MiRedWiFi",
    "password": "MiContraseña123",
    "server_url": "http://192.168.1.100:3000"
}

response = requests.post('http://192.168.4.1/config', 
                        json=config_data,
                        headers={'Content-Type': 'application/json'})

if response.status_code == 200:
    result = response.json()
    print(f"Configuración: {result['message']}")
```

### JavaScript/Node.js Example
```javascript
const axios = require('axios');

// Obtener información del dispositivo
async function getDeviceInfo() {
    try {
        const response = await axios.get('http://192.168.1.105/info');
        console.log('Device Info:', response.data);
    } catch (error) {
        console.error('Error:', error.response.data);
    }
}

// Enviar datos al dispositivo
async function sendData(data) {
    try {
        const response = await axios.post('http://192.168.1.105/data', data);
        console.log('Response:', response.data);
    } catch (error) {
        console.error('Error:', error.response.data);
    }
}
```

## Troubleshooting

### Problemas de Compilación y Carga

#### Error "Write timeout" o "Serial exception error"
Este es el error más común al cargar firmware en ESP32. El problema no es de compilación sino de conexión serial.

**Soluciones en orden de prioridad:**

1. **Método del Botón BOOT (Más Efectivo)**:
   - Mantén presionado el botón **BOOT** en el ESP32
   - Haz clic en "Upload" en el Arduino IDE
   - Cuando veas "Connecting..." en la consola, suelta el botón BOOT
   - El firmware debería cargarse correctamente

2. **Verificar Conexiones**:
   - Asegúrate de que el cable USB esté bien conectado
   - Usa un cable USB de datos (no solo de carga)
   - Prueba con un cable USB diferente
   - Verifica que el puerto COM sea correcto en `Tools > Port`

3. **Revisar Drivers**:
   - Instala los drivers CP2102 o CH340 según tu ESP32
   - En Windows: Device Manager debe mostrar el puerto COM sin errores
   - Reinicia el Arduino IDE después de instalar drivers

4. **Configuración del Board**:
   ```
   Board: "ESP32 Dev Module"
   Upload Speed: "115200" (prueba con "921600" si es muy lento)
   CPU Frequency: "240MHz"
   Flash Frequency: "80MHz"
   Flash Mode: "QIO"
   Flash Size: "4MB (32Mb)"
   Partition Scheme: "Default 4MB with spiffs"
   Core Debug Level: "None"
   ```

5. **Método Alternativo - Boot Manual**:
   - Desconecta el ESP32
   - Mantén presionado BOOT
   - Conecta el ESP32 (mantén BOOT presionado)
   - Haz clic en Upload
   - Suelta BOOT cuando veas "Connecting..."

6. **Reset antes de cargar**:
   - Presiona el botón RESET en el ESP32
   - Inmediatamente haz clic en Upload
   - Mantén BOOT presionado si es necesario

#### Error "esptools.py failed" o "Timed out waiting for packet header"
- Reduce la velocidad de carga: `Tools > Upload Speed > 115200`
- Cierra otros programas que puedan usar el puerto serial
- Verifica que no haya monitor serial abierto

#### Compilación exitosa pero no carga
Tu sketch se compiló correctamente (955458 bytes, 72% de espacio usado), el problema es solo de carga:
- El ESP32 tiene suficiente memoria para el firmware
- Sigue los pasos de conexión serial arriba

### El dispositivo no responde después de cargar
- Presiona el botón RESET en el ESP32
- Abre Serial Monitor (Tools > Serial Monitor) a 115200 baudios
- Deberías ver mensajes de inicio del firmware

### El dispositivo no aparece como Access Point
- Verifica que el ESP32 esté alimentado correctamente
- Resetear el dispositivo presionando RESET
- Espera 10-15 segundos después del reset
- Busca la red "SIAMP-G-Config" en tu dispositivo WiFi

### Error de JSON inválido en la API
- Verificar que el Content-Type sea `application/json`
- Verificar que el JSON esté bien formateado
- Usar herramientas como Postman o curl para probar

### El dispositivo no se conecta a WiFi
- Verificar credenciales WiFi con el endpoint `/networks`
- Verificar que la red WiFi esté disponible y en rango
- Usar el endpoint `/reset` para limpiar configuración si es necesario

### Problemas de Memoria
Si ves errores de memoria insuficiente:
- El firmware actual usa 955KB (72%) de espacio de programa
- Usa 46KB (14%) de memoria dinámica
- Hay suficiente espacio para expansiones futuras

### Debugging Serial
Para ver qué está pasando internamente:
1. Abre Serial Monitor después de cargar el firmware
2. Configura a 115200 baudios
3. Presiona RESET en el ESP32
4. Verás mensajes como:
   ```
   SIAMP-G Firmware iniciando...
   Sin configuración, iniciando modo AP...
   Configurando Access Point...
   AP IP address: 192.168.4.1
   Servidor web iniciado en modo configuración
   ```

### Problemas de Conectividad con la API

#### No responde a peticiones HTTP (192.168.4.1)
Si estás conectado a la WiFi "SIAMP-G-Config" pero no puedes hacer peticiones:

**1. Verificaciones Básicas:**
- Confirma que estás conectado a "SIAMP-G-Config" (no a tu WiFi normal)
- La contraseña de la red es: `12345678`
- La IP del ESP32 debe ser exactamente: `192.168.4.1`

**2. Diagnóstico de Red:**
```bash
# Verificar conectividad básica
ping 192.168.4.1

# Ver tu configuración de red actual
ipconfig /all    # Windows
ifconfig         # Linux/Mac
```

**3. Verificar que el servidor web esté funcionando:**
- Abre Serial Monitor en Arduino IDE (115200 baudios)
- Presiona RESET en el ESP32
- Deberías ver:
```
SIAMP-G Firmware iniciando...
Configurando Access Point...
AP IP address: 192.168.4.1
Servidor web iniciado en modo configuración
```

**4. Problemas Comunes y Soluciones:**

**A) El ESP32 no arrancó en modo AP:**
- Presiona RESET en el ESP32
- Espera 10-15 segundos
- Verifica en Serial Monitor que dice "modo configuración"

**B) Conflicto de IP:**
- Desconéctate de todas las redes WiFi
- Conéctate solo a "SIAMP-G-Config"
- Verifica tu IP: debe estar en rango 192.168.4.x

**C) Firewall/Antivirus bloqueando:**
- Temporalmente desactiva Windows Firewall
- Prueba desde el navegador: `http://192.168.4.1/`

**D) El ESP32 se quedó en loop o crash:**
- Verifica en Serial Monitor si hay errores
- Si ves reinicios constantes, hay un problema en el código

**5. Configuración Postman Correcta:**
```
Method: GET
URL: http://192.168.4.1/
Headers: (ninguno necesario para GET /)

Para POST /config:
Method: POST  
URL: http://192.168.4.1/config
Headers: Content-Type: application/json
Body (raw JSON):
{
  "ssid": "TuRedWiFi",
  "password": "TuContraseña", 
  "server_url": "http://192.168.1.100:3000"
}
```

**6. Alternativas de Testing:**

**Navegador Web:**
- Abre: `http://192.168.4.1/`
- Deberías ver respuesta JSON con información del dispositivo

**PowerShell (Windows):**
```powershell
# Verificar conectividad
Test-NetConnection -ComputerName 192.168.4.1 -Port 80

# Petición HTTP básica
Invoke-RestMethod -Uri "http://192.168.4.1/" -Method GET
```

**cURL (si lo tienes instalado):**
```bash
curl -v http://192.168.4.1/
```

### Comandos de Diagnóstico Paso a Paso

**Paso 1 - Verificar conexión de red:**
```bash
ping 192.168.4.1
```
**Resultado esperado:** Respuestas exitosas (tiempo < 50ms)

**Paso 2 - Verificar puerto HTTP:**
```bash
telnet 192.168.4.1 80
```
**Resultado esperado:** Conexión exitosa al puerto 80

**Paso 3 - Petición HTTP básica:**
```bash
curl -v http://192.168.4.1/
```
**Resultado esperado:** JSON con información del dispositivo

**Paso 4 - Probar configuración:**
```bash
curl -X POST http://192.168.4.1/config \
  -H "Content-Type: application/json" \
  -d '{"ssid":"test","password":"test","server_url":"http://test.com"}' \
  -v
```

**Paso 5 - Ver estado detallado:**
```bash
curl -s http://192.168.4.1/status | python -m json.tool
```

### Debugging Serial en Tiempo Real

Si nada funciona, abre Serial Monitor y observa qué pasa cuando haces peticiones:

**Lo que deberías ver:**
```
SIAMP-G Firmware iniciando...
Sin configuración, iniciando modo AP...
Configurando Access Point...
AP IP address: 192.168.4.1
Servidor web iniciado en modo configuración
```

**Si ves errores repetitivos:**
- El ESP32 está crasheando
- Posible problema de memoria o configuración
- Contacta para revisar el código

**Si no ves nada:**
- El ESP32 no está ejecutando el código
- Verifica que se cargó correctamente el firmware

## Desarrollo

### Agregar Nuevos Endpoints
1. Definir nueva ruta en `setup()` o `setupMainServer()`
2. Implementar función handler correspondiente
3. Seguir el patrón JSON para respuestas
4. Actualizar documentación

### Ejemplo de nuevo endpoint:
```cpp
server.on("/custom", HTTP_POST, handleCustom);

void handleCustom() {
  DynamicJsonDocument doc(512);
  doc["message"] = "Custom endpoint response";
  doc["timestamp"] = millis();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}
```

## Integración con el Sistema SIAMP-G

Este firmware está diseñado para trabajar con:
- **SIAMP-G-Server**: Backend NestJS que se comunica via API REST
- **SIAMP-G-Mobile**: App móvil React Native para control y monitoreo

## Versiones

- **v1.0.0**: Versión inicial con API REST completa en JSON
