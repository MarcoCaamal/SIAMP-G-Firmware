#ifndef RGB_CONTROLLER_H
#define RGB_CONTROLLER_H

#include <Arduino.h>
#include "types.h"

// Estructuras para el controlador RGB
struct RGBColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct RGBState {
  bool isOn;             // Estado (encendido/apagado)
  uint8_t brightness;    // Brillo (0-100%)
  RGBColor color;        // Color RGB actual
};

// Funciones para el controlador RGB
void setupRGBController();
void setRGBState(const RGBState &state);
RGBState getRGBState();
void turnOnRGB();
void turnOffRGB();
void setRGBColor(uint8_t r, uint8_t g, uint8_t b);
void setRGBBrightness(uint8_t brightness);
void updateRGBOutput();

#endif // RGB_CONTROLLER_H
