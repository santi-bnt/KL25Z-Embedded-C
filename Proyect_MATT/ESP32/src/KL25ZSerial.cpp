#include "KL25ZSerial.h"

#include "Config.h"
#include "Display.h"
#include "FirebaseService.h"
#include "Logger.h"

HardwareSerial RobotSerial(2);
bool modoManualKL25Z = false;
String lineaKL25Z = "";

const unsigned long UART_LETRA_DELAY_MS = 15;
const unsigned long UART_FIN_TEXTO_DELAY_MS = 20;
const unsigned long UART_WAIT_STATUS_MS = 1000;
const unsigned long UART_WAIT_CANCEL_MS = 700;
const unsigned long UART_WAIT_DELETE_MS = 2000;
const unsigned long UART_STOP_ACK_TIMEOUT_MS = 8000;
const unsigned long UART_STOP_SETTLE_MS = 1200;
const size_t UART_TEXTO_MAX_CHARS = 150;

static bool ultimaEsperaCancelada = false;
static String ultimoEstadoKLLinea1 = "";
static String ultimoEstadoKLLinea2 = "";

static bool enviarConAckPorCaracter(const String &mensaje, char terminador, unsigned long timeoutMs, const String &firebaseKey, bool vigilarEliminacion = true);

static void mostrarEstadoKL(const String &linea1, const String &linea2) {
  ultimoEstadoKLLinea1 = linea1;
  ultimoEstadoKLLinea2 = linea2;
  mostrarDisplay(linea1.substring(0, 16), linea2.substring(0, 16));
}

static void mostrarUltimoEstadoKLEsperando(unsigned long segundos) {
  if (ultimoEstadoKLLinea1.length() > 0) {
    mostrarDisplay(ultimoEstadoKLLinea1.substring(0, 16), ultimoEstadoKLLinea2.substring(0, 16));
  } else {
    mostrarDisplay("KL trabajando", String(segundos) + "s");
  }
}

static void prepararEsperaKL(const String &linea1, const String &linea2) {
  ultimoEstadoKLLinea1 = linea1;
  ultimoEstadoKLLinea2 = linea2;
}

static String normalizarTextoParaKL25Z(String texto) {
  String limpio = "";
  limpio.reserve(min(texto.length(), UART_TEXTO_MAX_CHARS));

  for (size_t i = 0; i < texto.length() && limpio.length() < UART_TEXTO_MAX_CHARS; i++) {
    char c = texto[i];

    if (c == '#' || c == '\r' || c == '\n' || c == '\t') {
      limpio += ' ';
    }
    else if ((uint8_t)c < 32 || (uint8_t)c > 126) {
      limpio += ' ';
    }
    else {
      limpio += c;
    }
  }

  return limpio;
}

void iniciarRobotSerial() {
  RobotSerial.begin(UART_BAUD, SERIAL_8N1, UART_RX, UART_TX);
  Serial.println("Serial2 listo para KL25Z");
}

void procesarLineaKL25Z(String linea) {
  linea.trim();

  if (linea.length() == 0) return;

  if (linea == "#MODE:MANUAL") {
    modoManualKL25Z = true;
    logSimple("KL25Z en manual");
    mostrarEstadoKL("Modo manual", "KL25Z manual");
  }
  else if (linea == "#MODE:AUTO") {
    modoManualKL25Z = false;
    logSimple("KL25Z en auto");
    mostrarEstadoKL("Modo auto", "KL25Z auto");
  }
  else if (linea.startsWith("#INPUT:")) {
    String texto = linea.substring(7);
    logSimple("KL25Z recibio: " + texto);
    mostrarEstadoKL("Entrada:", texto);
  }
  else if (linea.startsWith("#WRITING:")) {
    String texto = linea.substring(9);
    logSimple("KL25Z escribiendo: " + texto);
    mostrarEstadoKL("Escribiendo:", texto);
  }
  else if (linea.startsWith("#STATUS:")) {
    String estado = linea.substring(8);
    int separador = estado.indexOf('|');
    String linea1 = estado;
    String linea2 = "";

    if (separador >= 0) {
      linea1 = estado.substring(0, separador);
      linea2 = estado.substring(separador + 1);
    }

    Serial.println("KL25Z STATUS: " + linea1 + " | " + linea2);
    mostrarEstadoKL(linea1, linea2);
  }
  else if (linea == "#DONE") {
    logSimple("KL25Z termino");
    mostrarEstadoKL("Listo", "KL25Z done");
  }
  else if (linea == "OK_CHAR") {
    // ACK por caracter: no se imprime para no hacer lento el flujo UART.
  }
  else if (linea == "OK") {
    logSimple("KL25Z OK");
    mostrarEstadoKL("OK", "KL25Z");
  }
  else if (linea == "BUSY_MANUAL") {
    logSimple("KL25Z ocupada manual");
    mostrarEstadoKL("KL25Z manual", "Espera");
  }
}

void leerMensajesEspontaneosKL25Z() {
  while (RobotSerial.available()) {
    char c = RobotSerial.read();

    if (c == '\n') {
      procesarLineaKL25Z(lineaKL25Z);
      lineaKL25Z = "";
    }
    else if (c != '\r') {
      lineaKL25Z += c;

      if (lineaKL25Z.length() > 120) {
        lineaKL25Z = "";
      }
    }
  }
}

void limpiarBufferRobot() {
  while (RobotSerial.available()) {
    RobotSerial.read();
  }
}

static bool esperarStopAck(unsigned long timeoutMs) {
  String respuesta = "";
  unsigned long inicio = millis();

  while (millis() - inicio < timeoutMs) {
    while (RobotSerial.available()) {
      char c = RobotSerial.read();

      if (c == '\n') {
        respuesta.trim();

        if (respuesta.length() == 0) {
          respuesta = "";
          continue;
        }

        procesarLineaKL25Z(respuesta);

        if (respuesta == "OK") {
          unsigned long settleInicio = millis();

          while (millis() - settleInicio < UART_STOP_SETTLE_MS) {
            leerMensajesEspontaneosKL25Z();
            delay(10);
          }

          limpiarBufferRobot();
          return true;
        }

        respuesta = "";
      }
      else if (c != '\r') {
        respuesta += c;

        if (respuesta.length() > 120) {
          respuesta = "";
        }
      }
    }
  }

  return false;
}

static void mandarStopYEsperar(const String &motivo) {
  mostrarDisplay("Cancelando", "Esperando STOP");
  Serial.println(motivo + " -> KL25Z: X por chars");

  if (enviarConAckPorCaracter("X", '\n', UART_STOP_ACK_TIMEOUT_MS, "")) {
    Serial.println("KL25Z confirmo STOP");
  } else {
    Serial.println("KL25Z no confirmo STOP");
    limpiarBufferRobot();
  }
}

bool ultimoEnvioFueCancelado() {
  return ultimaEsperaCancelada;
}

bool esperarLineaRobot(String respuestaEsperada, unsigned long timeoutMs, const String &firebaseKey, bool vigilarEliminacion) {
  String respuesta = "";
  unsigned long inicio = millis();
  unsigned long ultimaRevisionCancel = 0;
  unsigned long ultimaRevisionComando = 0;
  unsigned long ultimoDisplay = 0;
  unsigned long ultimoHeartbeat = 0;
  ultimaEsperaCancelada = false;

  while (millis() - inicio < timeoutMs) {
    unsigned long ahora = millis();

    if (ahora - ultimoHeartbeat > 5000) {
      actualizarHeartbeatRobot("trabajando");
      ultimoHeartbeat = ahora;
    }

    if (ahora - ultimaRevisionCancel > UART_WAIT_CANCEL_MS) {
      if (leerCancelCmdFirebase()) {
        ultimaEsperaCancelada = true;
        mostrarDisplay("Cancelando", "Esperando STOP");
        esperarStopAck(UART_STOP_ACK_TIMEOUT_MS);
        return false;
      }
      ultimaRevisionCancel = millis();
    }

    if (vigilarEliminacion && firebaseKey.length() > 0 && ahora - ultimaRevisionComando > UART_WAIT_DELETE_MS) {
      if (comandoFueEliminado(firebaseKey)) {
        ultimaEsperaCancelada = true;
        mostrarDisplay("Cmd eliminado", "Parando KL25Z");
        mandarStopYEsperar("Comando eliminado en Firebase");
        return false;
      }
      ultimaRevisionComando = millis();
    }

    if (ahora - ultimoDisplay > UART_WAIT_STATUS_MS) {
      unsigned long segundos = (ahora - inicio) / 1000UL;
      mostrarUltimoEstadoKLEsperando(segundos);
      ultimoDisplay = ahora;
    }

    while (RobotSerial.available()) {
      char c = RobotSerial.read();

      if (c == '\n') {
        respuesta.trim();

        if (respuesta.length() == 0) {
          respuesta = "";
          continue;
        }

        procesarLineaKL25Z(respuesta);

        if (respuesta == respuestaEsperada) {
          return true;
        }

        respuesta = "";
      }
      else if (c != '\r') {
        respuesta += c;

        if (respuesta.length() > 120) {
          respuesta = "";
        }
      }
    }
  }

  mostrarDisplay("Timeout KL25Z", "Sin OK");
  return false;
}

static bool enviarConAckPorCaracter(const String &mensaje, char terminador, unsigned long timeoutMs, const String &firebaseKey, bool vigilarEliminacion) {
  if (modoManualKL25Z) {
    logSimple("No enviado: KL manual");
    mostrarDisplay("KL25Z manual", "No enviar");
    return false;
  }

  limpiarBufferRobot();
  prepararEsperaKL("KL trabajando", mensaje);

  Serial.print("UART rapido -> KL25Z: ");
  Serial.println(mensaje);

  for (int i = 0; i < mensaje.length(); i++) {
    RobotSerial.write(mensaje[i]);
    RobotSerial.flush();

    if (UART_LETRA_DELAY_MS > 0) {
      delay(UART_LETRA_DELAY_MS);
    }
  }

  delay(UART_FIN_TEXTO_DELAY_MS);
  Serial.print("Mandando fin: ");
  if (terminador == '\n') Serial.println("\\n");
  else Serial.println(terminador);

  RobotSerial.write(terminador);
  RobotSerial.flush();

  bool okFinal = esperarLineaRobot("OK", timeoutMs, firebaseKey, vigilarEliminacion);

  if (okFinal) {
    logSimple("KL25Z confirmo OK");
    mostrarTerminadoDisplay();
    return true;
  }

  logSimple("Error: KL25Z no confirmo final");
  mostrarErrorDisplay();
  return false;
}

bool enviarComandoPorUART(String comando, const String &firebaseKey, bool vigilarEliminacion) {
  return enviarConAckPorCaracter(comando, '#', TIMEOUT_FINAL_MS, firebaseKey, vigilarEliminacion);
}

bool enviarTextoPorUART(String texto, const String &firebaseKey) {
  String textoSeguro = normalizarTextoParaKL25Z(texto);

  if (textoSeguro.length() == 0) {
    logSimple("Texto vacio");
    mostrarDisplay("Texto vacio", "Nada que enviar");
    return true;
  }

  if (textoSeguro != texto) {
    logSimple("Texto normalizado para KL25Z");
  }

  logSimple("Enviando WRITE por UART: " + textoSeguro);
  mostrarDisplay("Enviando texto", textoSeguro.substring(0, 16));

  return enviarComandoPorUART(textoSeguro, firebaseKey);
}

bool enviarLineaPorUART(const String &mensaje, unsigned long timeoutMs, const String &firebaseKey, bool vigilarEliminacion) {
  return enviarConAckPorCaracter(mensaje, '\n', timeoutMs, firebaseKey, vigilarEliminacion);
}
