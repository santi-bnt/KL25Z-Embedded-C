#include "Config.h"

// =======================================================
// WIFI
// =======================================================
const char* ssid = "AQUI_VA_EL_NOMBRE_DEL_WIFI";
const char* password = "AQUI_VA_LA_CONTRASENA_DEL_WIFI";

const char* wifiSsids[] = {
  "AQUI_VA_WIFI_1",
  "AQUI_VA_WIFI_2",
  "AQUI_VA_WIFI_3",
  "AQUI_VA_WIFI_4"
};

const char* wifiPasswords[] = {
  "AQUI_VA_CONTRASENA_WIFI_1",
  "AQUI_VA_CONTRASENA_WIFI_2",
  "AQUI_VA_CONTRASENA_WIFI_3",
  "AQUI_VA_CONTRASENA_WIFI_4"
};

const int wifiNetworkCount = sizeof(wifiSsids) / sizeof(wifiSsids[0]);

// =======================================================
// FIREBASE
// =======================================================
String databaseURL = "https://AQUI_VA_TU_PROYECTO.firebaseio.com";
const char* firebaseHost = "AQUI_VA_TU_PROYECTO.firebaseio.com";
WiFiClientSecure client;

// =======================================================
// UART ESP32 <-> KL25Z
// =======================================================
const int UART_TX = 18;
const int UART_RX = 19;
const int UART_BAUD = 9600;

const unsigned long TIMEOUT_CHAR_MS  =   4000;
const unsigned long TIMEOUT_FINAL_MS = 900000;  // 15 min: cubre escritura real con servos y barridos largos
