#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

extern LiquidCrystal_I2C lcd;

void iniciarDisplay();
void mostrarDisplay(String linea1, String linea2);
void mostrarInicioDisplay();
void mostrarErrorDisplay();
void mostrarTerminadoDisplay();

#endif
