#include "RobotActions.h"

#include "Config.h"
#include "Display.h"
#include "KL25ZSerial.h"
#include "Logger.h"

bool ejecutarEscritura(String id, String texto) {
  logSimple("");
  logSimple("Comando recibido");
  logSimple("Accion: escribir");
  logSimple("Texto: " + texto);

  mostrarDisplay("Texto recibido", texto.substring(0, 16));
  delay(1000);

  Serial.println("Enviando a KL25Z...");
  bool exito = enviarTextoPorUART(texto, id);

  if (exito) logSimple("Comando terminado");
  else if (ultimoEnvioFueCancelado()) logSimple("Comando cancelado/eliminado");
  else logSimple("Comando con error");

  return exito;
}

bool ejecutarBorrado(String id) {
  logSimple("");
  logSimple("Comando recibido");
  logSimple("Accion: borrar");

  mostrarDisplay("Borrando...", "Esperando OK");

  Serial.println("Enviando a KL25Z...");
  if (enviarLineaPorUART("ERASE", TIMEOUT_FINAL_MS, id)) {
    logSimple("Borrado terminado");
    mostrarDisplay("Borrado listo", "OK KL25Z");
    return true;
  }

  if (ultimoEnvioFueCancelado()) {
    logSimple("Borrado cancelado/eliminado");
    mostrarDisplay("Cancelado", "Borrado stop");
    return false;
  }

  logSimple("Error borrado");
  mostrarErrorDisplay();
  return false;
}

bool ejecutarBorradoPunto(String id, float x_mm, float y_mm, float r_mm) {
  logSimple("");
  logSimple("Comando recibido");
  logSimple("Accion: borrar_punto");
  logSimple("x_mm=" + String(x_mm, 1) +
            " y_mm=" + String(y_mm, 1) +
            " r_mm=" + String(r_mm, 1));

  if (modoManualKL25Z) {
    logSimple("No enviado: KL manual");
    mostrarDisplay("KL25Z manual", "No enviar");
    return false;
  }

  String linea1 = "Borrar punto";
  String linea2 = "(" + String((int)x_mm) + "," + String((int)y_mm) + ")";
  mostrarDisplay(linea1, linea2);
  delay(500);

  String cmd = "E" + String((int)x_mm) + "," +
               String((int)y_mm) + "," +
               String((int)r_mm);

  Serial.println("Enviando a KL25Z...");
  if (enviarComandoPorUART(cmd, id, false)) {
    logSimple("KL25Z confirmo borrado puntual");
    mostrarDisplay("Punto borrado", "OK KL25Z");
    return true;
  }

  if (ultimoEnvioFueCancelado()) {
    logSimple("Borrado punto cancelado/eliminado");
    mostrarDisplay("Cancelado", "Punto stop");
    return false;
  }

  logSimple("Error: KL no confirmo OK final");
  mostrarErrorDisplay();
  return false;
}
