#ifndef KL25Z_SERIAL_H
#define KL25Z_SERIAL_H

#include <Arduino.h>

extern HardwareSerial RobotSerial;
extern bool modoManualKL25Z;
extern String lineaKL25Z;

void iniciarRobotSerial();
void procesarLineaKL25Z(String linea);
void leerMensajesEspontaneosKL25Z();
void limpiarBufferRobot();
bool esperarLineaRobot(String respuestaEsperada, unsigned long timeoutMs, const String &firebaseKey = "", bool vigilarEliminacion = true);
bool ultimoEnvioFueCancelado();
bool enviarTextoPorUART(String texto, const String &firebaseKey = "");
bool enviarComandoPorUART(String comando, const String &firebaseKey = "", bool vigilarEliminacion = true);
bool enviarLineaPorUART(const String &mensaje, unsigned long timeoutMs, const String &firebaseKey = "", bool vigilarEliminacion = true);

#endif
