#include <Arduino.h>
#include "rgb_controller.h"
#include "config.h"

// Estado global del LED RGB
static RGBState currentState = {
  .isOn = false,
  .brightness = 100,
  .color = {255, 255, 255}  // Blanco por defecto
};

// Configuración inicial del controlador RGB
void setupRGBController() {
  // Configurar los canales PWM utilizando la nueva API de ESP32 v3.0.4
  // ledcAttach(pin, frecuencia, resolución)
  ledcAttach(RGB_RED_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttach(RGB_GREEN_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttach(RGB_BLUE_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
  
  // Inicialmente apagado
  turnOffRGB();
  
  Serial.println("Controlador RGB inicializado");
}

// Obtener el estado actual del RGB
RGBState getRGBState() {
  return currentState;
}

// Establecer el estado completo del RGB
void setRGBState(const RGBState &state) {
  currentState = state;
  updateRGBOutput();
}

// Encender el LED RGB
void turnOnRGB() {
  currentState.isOn = true;
  updateRGBOutput();
}

// Apagar el LED RGB
void turnOffRGB() {
  currentState.isOn = false;
  updateRGBOutput();
}

// Establecer el color RGB
void setRGBColor(uint8_t r, uint8_t g, uint8_t b) {
  currentState.color.r = r;
  currentState.color.g = g;
  currentState.color.b = b;
  updateRGBOutput();
}

// Establecer el nivel de brillo
void setRGBBrightness(uint8_t brightness) {
  // Limitar el brillo entre 0 y 100
  currentState.brightness = constrain(brightness, 0, 100);
  updateRGBOutput();
}

// Actualizar la salida física del LED RGB
void updateRGBOutput() {
  if (currentState.isOn) {
    // Aplicar el brillo al color
    float factor = currentState.brightness / 100.0;
    
    uint8_t r = currentState.color.r * factor;
    uint8_t g = currentState.color.g * factor;
    uint8_t b = currentState.color.b * factor;
    
    // Usar la nueva API de ESP32 v3.0.4 para controlar PWM
    // En la nueva API, ledcWrite toma directamente el pin como parámetro
    ledcWrite(RGB_RED_PIN, r);
    ledcWrite(RGB_GREEN_PIN, g);
    ledcWrite(RGB_BLUE_PIN, b);
    
    Serial.printf("LED RGB: ON, Color: (%d,%d,%d), Brillo: %d%%\n", 
                 r, g, b, currentState.brightness);
  } else {
    // Apagar todos los LED
    ledcWrite(RGB_RED_PIN, 0);
    ledcWrite(RGB_GREEN_PIN, 0);
    ledcWrite(RGB_BLUE_PIN, 0);
    
    Serial.println("LED RGB: OFF");
  }
}
