#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>

#include "Config.h"
#include "Display.h"
#include "FirebaseService.h"
#include "KL25ZSerial.h"
#include "Logger.h"
#include "MattWiFi.h"

static const unsigned long POLL_MOTOR_MS = 400;
static const unsigned long POLL_CANCEL_MS = 400;
static const unsigned long POLL_COMANDOS_MS = 1000;
static const unsigned long HEARTBEAT_MS = 5000;
static const bool ENABLE_LIVE_MOTOR_CMDS = false;

// =======================================================
// SETUP
// =======================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(21, 22);
  Wire.setClock(50000);
  iniciarDisplay();

  mostrarDisplay("ESP32 MATT", "Iniciando...");
  delay(800);

  iniciarRobotSerial();

  logSimple("ESP32 MATT cola Firebase");
  logSimple("UART listo");

  client.setInsecure();

  conectarWiFi();
  Serial.println("Firebase listo si WiFi/DNS respondieron correctamente");
}

// =======================================================
// LOOP
// =======================================================
void loop() {
  leerMensajesEspontaneosKL25Z();

  if (modoManualKL25Z) {
    delay(200);
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long ultimoIntentoWiFi = 0;

    if (millis() - ultimoIntentoWiFi > 5000) {
      logSimple("WiFi perdido");
      mostrarDisplay("WiFi perdido", "Reintentando");
      conectarWiFi();
      ultimoIntentoWiFi = millis();
    }

    delay(100);
    return;
  }

  static int fallosFirebase = 0;

  if (!internetListo()) {
    fallosFirebase++;

    if (fallosFirebase >= 3) {
      static unsigned long ultimoIntentoRed = 0;
      logSimple("Reconectando WiFi...");
      mostrarDisplay("Reconectando", "WiFi...");

      if (millis() - ultimoIntentoRed > 5000) {
        conectarWiFi();
        ultimoIntentoRed = millis();
      }

      fallosFirebase = 0;
    }

    delay(250);
    return;
  }

  fallosFirebase = 0;

  static unsigned long ultimoMotor = 0;
  static unsigned long ultimoCancel = 0;
  static unsigned long ultimoComandos = 0;
  static unsigned long ultimoHeartbeat = 0;
  unsigned long ahora = millis();

  if (ahora - ultimoHeartbeat >= HEARTBEAT_MS) {
    actualizarHeartbeatRobot("libre");
    ultimoHeartbeat = ahora;
  }

  if (ahora - ultimoComandos >= POLL_COMANDOS_MS) {
    leerComandosFirebase();
    ultimoComandos = ahora;
  }

  if (ENABLE_LIVE_MOTOR_CMDS && ahora - ultimoMotor >= POLL_MOTOR_MS) {
    leerMotorCmdFirebase();
    ultimoMotor = ahora;
  }

  if (ENABLE_LIVE_MOTOR_CMDS && ahora - ultimoCancel >= POLL_CANCEL_MS) {
    leerCancelCmdFirebase();
    ultimoCancel = ahora;
  }

  delay(100);
}
