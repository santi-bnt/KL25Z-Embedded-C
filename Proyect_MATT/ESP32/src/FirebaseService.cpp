#include "FirebaseService.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "Display.h"
#include "Logger.h"
#include "MattWiFi.h"
#include "RobotActions.h"
#include "KL25ZSerial.h"

static const uint16_t FIREBASE_HTTP_TIMEOUT_MS = 3500;

void actualizarHeartbeatRobot(const String &mensaje) {
  if (!internetListo()) return;

  HTTPClient http;
  String url = databaseURL + "/robot/estado.json";

  http.begin(client, url);
  http.setTimeout(FIREBASE_HTTP_TIMEOUT_MS);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"online\":true,\"mensaje\":\"" + mensaje +
                "\",\"lastSeen\":{\".sv\":\"timestamp\"}}";
  int httpCode = http.PATCH(body);
  http.end();

  if (httpCode <= 0 || httpCode >= 400) {
    logSimple("Error heartbeat Firebase: " + String(httpCode));
  }
}

static bool putJsonString(const String &path, const String &valor) {
  if (!internetListo()) {
    logSimple("No actualizo Firebase");
    return false;
  }

  HTTPClient http;
  String url = databaseURL + path + ".json";

  http.begin(client, url);
  http.setTimeout(FIREBASE_HTTP_TIMEOUT_MS);
  http.addHeader("Content-Type", "application/json");

  String body = "\"" + valor + "\"";
  int httpCode = http.PUT(body);
  http.end();

  if (httpCode <= 0 || httpCode >= 400) {
    logSimple("Error actualizando Firebase: " + String(httpCode));
    return false;
  }

  return true;
}

void actualizarCampo(String id, String campo, String valor) {
  putJsonString("/robot/comandos/" + id + "/" + campo, valor);
}

void eliminarComando(String id) {
  if (!internetListo()) {
    logSimple("No elimino comando");
    return;
  }

  HTTPClient http;
  String url = databaseURL + "/robot/comandos/" + id + ".json";

  http.begin(client, url);
  http.setTimeout(FIREBASE_HTTP_TIMEOUT_MS);

  int httpCode = http.sendRequest("DELETE");

  if (httpCode <= 0) {
    logSimple("Error eliminando comando");
  }

  http.end();
}

bool comandoExiste(const String &firebaseKey) {
  if (!internetListo()) return false;

  HTTPClient http;
  String url = databaseURL + "/robot/comandos/" + firebaseKey + ".json";

  http.begin(client, url);
  http.setTimeout(FIREBASE_HTTP_TIMEOUT_MS);
  int httpCode = http.GET();

  if (httpCode != 200) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  return payload != "null" && payload.length() > 0;
}

bool comandoFueEliminado(const String &firebaseKey) {
  if (firebaseKey.length() == 0 || !internetListo()) return false;

  HTTPClient http;
  String url = databaseURL + "/robot/comandos/" + firebaseKey + ".json";

  http.begin(client, url);
  http.setTimeout(FIREBASE_HTTP_TIMEOUT_MS);
  int httpCode = http.GET();

  if (httpCode != 200) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  return payload == "null" || payload.length() == 0;
}

bool obtenerSiguienteComando(ComandoRobot &cmd) {
  if (!internetListo()) return false;

  Serial.println("Buscando comandos en /robot/comandos...");
  mostrarDisplay("Firebase", "Buscando cmd");

  HTTPClient http;
  String url = databaseURL + "/robot/comandos.json";
  http.begin(client, url);
  http.setTimeout(FIREBASE_HTTP_TIMEOUT_MS);

  int httpCode = http.GET();

  if (httpCode <= 0) {
    logSimple("Error leyendo Firebase");
    http.end();
    return false;
  }

  if (httpCode != 200) {
    logSimple("Error HTTP Firebase: " + String(httpCode));
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  if (payload == "null" || payload.length() == 0) {
    mostrarInicioDisplay();
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    logSimple("Error JSON Firebase");
    mostrarDisplay("Error JSON", "Firebase");
    return false;
  }

  JsonObject comandos = doc.as<JsonObject>();
  bool encontrado = false;
  unsigned long long mejorOrden = 0;

  for (JsonPair item : comandos) {
    JsonObject comando = item.value().as<JsonObject>();
    String estado = comando["estado"] | "";

    if (estado != "pendiente") {
      continue;
    }

    unsigned long long orden = comando["ordenEnvio"] | (unsigned long long)0;
    if (orden == 0) {
      orden = comando["fecha"] | (unsigned long long)0;
    }

    if (!encontrado || orden < mejorOrden) {
      encontrado = true;
      mejorOrden = orden;

      cmd.firebaseKey = item.key().c_str();
      cmd.accion = comando["accion"] | "";
      cmd.texto = comando["texto"] | "";
      cmd.x_mm = comando["x_mm"] | 0.0f;
      cmd.y_mm = comando["y_mm"] | 0.0f;
      cmd.radius_mm = comando["radius_mm"] | 30.0f;
    }
  }

  if (!encontrado) {
    mostrarInicioDisplay();
    return false;
  }

  if (cmd.radius_mm <= 0) cmd.radius_mm = 30.0f;

  Serial.println("Comando pendiente encontrado");
  Serial.println("Key: " + cmd.firebaseKey);
  Serial.println("Accion: " + cmd.accion);
  mostrarDisplay("Cmd pendiente", cmd.accion.substring(0, 16));
  if (cmd.texto.length() > 0) {
    Serial.println("Texto: " + cmd.texto);
  }

  return true;
}

bool marcarComandoEnProceso(const String &firebaseKey) {
  if (!comandoExiste(firebaseKey)) {
    logSimple("Comando eliminado antes de iniciar");
    return false;
  }

  actualizarHeartbeatRobot("trabajando");
  bool okEstado = putJsonString("/robot/comandos/" + firebaseKey + "/estado", "en_proceso");
  bool okInicio = putJsonString("/robot/comandos/" + firebaseKey + "/inicioMensaje", "ESP32 tomo el comando");
  return okEstado && okInicio;
}

bool marcarComandoTerminado(const String &firebaseKey) {
  if (!comandoExiste(firebaseKey)) {
    logSimple("Comando eliminado, no marco terminado");
    return true;
  }

  return putJsonString("/robot/comandos/" + firebaseKey + "/estado", "terminado");
}

bool marcarComandoError(const String &firebaseKey, const String &mensaje) {
  if (!comandoExiste(firebaseKey)) {
    logSimple("Comando eliminado, no marco error");
    return true;
  }

  bool okEstado = putJsonString("/robot/comandos/" + firebaseKey + "/estado", "error");
  bool okMensaje = putJsonString("/robot/comandos/" + firebaseKey + "/mensaje", mensaje);
  return okEstado && okMensaje;
}

bool ejecutarComando(const ComandoRobot &cmd) {
  if (cmd.accion == "escribir") {
    return ejecutarEscritura(cmd.firebaseKey, cmd.texto);
  }

  if (cmd.accion == "borrar") {
    return ejecutarBorrado(cmd.firebaseKey);
  }

  if (cmd.accion == "borrar_punto") {
    float x_mm = cmd.x_mm;
    float y_mm = cmd.y_mm;
    float r_mm = cmd.radius_mm;

    if (x_mm < 0) x_mm = 0;
    if (y_mm < 0) y_mm = 0;
    if (r_mm < 10) r_mm = 10;
    if (r_mm > 200) r_mm = 200;

    return ejecutarBorradoPunto(cmd.firebaseKey, x_mm, y_mm, r_mm);
  }

  logSimple("Error: accion desconocida");
  return false;
}

void leerComandosFirebase() {
  ComandoRobot cmd;

  if (!obtenerSiguienteComando(cmd)) {
    return;
  }

  if (!marcarComandoEnProceso(cmd.firebaseKey)) {
    logSimple("No se pudo marcar en_proceso");
    return;
  }

  mostrarDisplay("Ejecutando", cmd.accion.substring(0, 16));
  bool exito = ejecutarComando(cmd);

  if (exito) {
    if (!comandoExiste(cmd.firebaseKey)) {
      Serial.println("Comando eliminado durante ejecucion");
      logSimple("Comando eliminado");
      mostrarInicioDisplay();
      return;
    }

    marcarComandoTerminado(cmd.firebaseKey);
    actualizarHeartbeatRobot("libre");
    Serial.println("Comando terminado");
    logSimple("Esperando comandos...");
    mostrarInicioDisplay();
  } else {
    if (!comandoExiste(cmd.firebaseKey)) {
      Serial.println("Comando eliminado durante ejecucion");
      logSimple("Comando eliminado");
      mostrarInicioDisplay();
      return;
    }

    if (ultimoEnvioFueCancelado()) {
      marcarComandoError(cmd.firebaseKey, "Comando cancelado o detenido por el usuario");
      actualizarHeartbeatRobot("cancelado");
      logSimple("Comando cancelado");
      mostrarDisplay("Cancelado", "Esperando cmd");
      delay(800);
      mostrarInicioDisplay();
      return;
    }

    marcarComandoError(cmd.firebaseKey, "El robot no confirmo OK o accion desconocida");
    actualizarHeartbeatRobot("error");
    logSimple("Comando con error");
    delay(1200);
    mostrarInicioDisplay();
  }
}

static unsigned long long lastMotorTs = 0;
static unsigned long long lastCancelTs = 0;
static bool motorCmdSincronizado = false;
static bool cancelCmdSincronizado = false;

static bool esperarRespuestaUARTSimple(const String &esperada, unsigned long timeoutMs) {
  String respuesta = "";
  unsigned long inicio = millis();

  while (millis() - inicio < timeoutMs) {
    while (RobotSerial.available()) {
      char c = RobotSerial.read();

      if (c == '\n') {
        respuesta.trim();

        if (respuesta == esperada) {
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

    delay(5);
  }

  return false;
}

static bool enviarCancelPorCaracter(void) {
  RobotSerial.write('X');
  RobotSerial.flush();

  if (!esperarRespuestaUARTSimple("OK_CHAR", 3000)) {
    logSimple("Cancel sin OK_CHAR");
    return false;
  }

  RobotSerial.write('\n');
  RobotSerial.flush();
  return true;
}

bool leerCancelCmdFirebase() {
  if (!internetListo()) return false;

  HTTPClient http;
  String url = databaseURL + "/robot/cancel_cmd.json";
  http.begin(client, url);
  http.setTimeout(FIREBASE_HTTP_TIMEOUT_MS);

  int code = http.GET();
  if (code != 200) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  if (payload == "null" || payload.length() == 0) return false;

  JsonDocument doc;
  if (deserializeJson(doc, payload)) return false;

  unsigned long long ts = doc["ts"] | (unsigned long long)0;
  String cancelCmd = doc["cmd"] | "";
  cancelCmd.trim();

  if (ts == 0 || ts == lastCancelTs || cancelCmd.length() == 0) return false;

  if (!cancelCmdSincronizado) {
    lastCancelTs = ts;
    cancelCmdSincronizado = true;
    logSimple("cancel_cmd viejo ignorado");
    return false;
  }

  lastCancelTs = ts;

  if (cancelCmd != "X" && cancelCmd != "M0" && cancelCmd != "STOP") {
    logSimple("cancel_cmd desconocido: " + cancelCmd);
    return false;
  }

  Serial.println("CANCEL prioritario -> KL25Z: X por chars");
  enviarCancelPorCaracter();
  logSimple("Cancel -> KL25Z");
  mostrarDisplay("Cancelando", "KL25Z stop");

  return true;
}

void leerMotorCmdFirebase() {
  if (!internetListo()) return;

  HTTPClient http;
  String url = databaseURL + "/robot/motor_cmd.json";
  http.begin(client, url);
  http.setTimeout(FIREBASE_HTTP_TIMEOUT_MS);

  int code = http.GET();
  if (code != 200) {
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  if (payload == "null" || payload.length() == 0) return;

  JsonDocument doc;
  if (deserializeJson(doc, payload)) return;

  unsigned long long ts = doc["ts"] | (unsigned long long)0;
  String motorCmd = doc["cmd"] | "";
  motorCmd.trim();

  if (ts == 0 || ts == lastMotorTs || motorCmd.length() == 0) return;

  if (!motorCmdSincronizado) {
    lastMotorTs = ts;
    motorCmdSincronizado = true;
    logSimple("motor_cmd viejo ignorado");
    return;
  }

  bool valido = (motorCmd == "W" || motorCmd == "S" || motorCmd == "A" || motorCmd == "D" ||
                 motorCmd == "X" || motorCmd == "FU" || motorCmd == "FD" ||
                 motorCmd == "FL" || motorCmd == "FR" || motorCmd == "F0" ||
                 motorCmd == "M1F" || motorCmd == "M1B" ||
                 motorCmd == "M2F" || motorCmd == "M2B" ||
                 motorCmd == "M3F" || motorCmd == "M3B" ||
                 motorCmd == "M4F" || motorCmd == "M4B" ||
                 motorCmd == "M0");

  if (!valido) {
    logSimple("motor_cmd desconocido: " + motorCmd);
    return;
  }

  lastMotorTs = ts;

  Serial.print("UART chars -> KL25Z: ");
  Serial.println(motorCmd);
  enviarLineaPorUART(motorCmd, 8000, "");

  logSimple("Motor CMD -> KL25Z: " + motorCmd);
}
