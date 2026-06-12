#ifndef FIREBASE_SERVICE_H
#define FIREBASE_SERVICE_H

#include <Arduino.h>

struct ComandoRobot {
  String firebaseKey;
  String accion;
  String texto;
  float x_mm = 0.0f;
  float y_mm = 0.0f;
  float radius_mm = 30.0f;
};

void actualizarCampo(String id, String campo, String valor);
void eliminarComando(String id);
bool comandoExiste(const String &firebaseKey);
bool comandoFueEliminado(const String &firebaseKey);
bool obtenerSiguienteComando(ComandoRobot &cmd);
bool marcarComandoEnProceso(const String &firebaseKey);
bool marcarComandoTerminado(const String &firebaseKey);
bool marcarComandoError(const String &firebaseKey, const String &mensaje);
bool ejecutarComando(const ComandoRobot &cmd);
void leerComandosFirebase();
void leerMotorCmdFirebase();
bool leerCancelCmdFirebase();
void actualizarHeartbeatRobot(const String &mensaje);

#endif
