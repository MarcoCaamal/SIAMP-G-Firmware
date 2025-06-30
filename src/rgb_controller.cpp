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
  // Configurar pines como salidas
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  
  // Asegurarse de que los pines estén inicialmente en nivel bajo
  digitalWrite(RGB_RED_PIN, LOW);
  digitalWrite(RGB_GREEN_PIN, LOW);
  digitalWrite(RGB_BLUE_PIN, LOW);
  
  // Inicialmente apagado
  turnOffRGB();
  
  Serial.println("Controlador RGB inicializado en pines:");
  Serial.printf("R: %d, G: %d, B: %d\n", RGB_RED_PIN, RGB_GREEN_PIN, RGB_BLUE_PIN);
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
    // Calcular el factor de brillo (0-100 a 0-255)
    int pwmMax = 255;
    int pwmFactor = (currentState.brightness * pwmMax) / 100;
    
    // Aplicar el factor de brillo a cada componente RGB
    // Usamos los colores originales para mantener la proporción correcta
    int r = (currentState.color.r * pwmFactor) / 255;
    int g = (currentState.color.g * pwmFactor) / 255;
    int b = (currentState.color.b * pwmFactor) / 255;
    
    // Usar analogWrite para controlar cada componente
    analogWrite(RGB_RED_PIN, r);
    analogWrite(RGB_GREEN_PIN, g);
    analogWrite(RGB_BLUE_PIN, b);
    
    Serial.printf("LED RGB: ON, Color RGB: (%d,%d,%d), Brillo: %d%%, Salida PWM: (%d,%d,%d)\n", 
                 currentState.color.r, currentState.color.g, currentState.color.b, 
                 currentState.brightness, r, g, b);  } else {
    // Apagar todos los LED asegurando que no quede ningún residuo de PWM
    analogWrite(RGB_RED_PIN, 0);
    analogWrite(RGB_GREEN_PIN, 0);
    analogWrite(RGB_BLUE_PIN, 0);
    
    // Asegurarse de que los pines estén completamente apagados
    digitalWrite(RGB_RED_PIN, LOW);
    digitalWrite(RGB_GREEN_PIN, LOW);
    digitalWrite(RGB_BLUE_PIN, LOW);
    
    Serial.println("LED RGB: OFF");
  }
}
