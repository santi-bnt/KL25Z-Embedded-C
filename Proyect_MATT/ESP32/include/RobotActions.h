#ifndef ROBOT_ACTIONS_H
#define ROBOT_ACTIONS_H

#include <Arduino.h>

bool ejecutarEscritura(String id, String texto);
bool ejecutarBorrado(String id);
bool ejecutarBorradoPunto(String id, float x_mm, float y_mm, float r_mm);

#endif
