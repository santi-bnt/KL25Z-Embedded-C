#include "Config.h"

// =======================================================
// WIFI
// =======================================================
const char* ssid = "SAZER 2432";
const char* password = "12345678";

const char* wifiSsids[] = {
  "SAZER 2432",
  "iPhone Joshua",
  "KOKITO 2852",
  "Hamiosh"
};

const char* wifiPasswords[] = {
  "12345678",
  "12345678",
  "23456789",
  "12345678"
};

const int wifiNetworkCount = sizeof(wifiSsids) / sizeof(wifiSsids[0]);

// =======================================================
// FIREBASE
// =======================================================
String databaseURL = "https://matt-205c1-default-rtdb.firebaseio.com";
const char* firebaseHost = "matt-205c1-default-rtdb.firebaseio.com";
WiFiClientSecure client;

// =======================================================
// UART ESP32 <-> KL25Z
// =======================================================
const int UART_TX = 18;
const int UART_RX = 19;
const int UART_BAUD = 9600;

const unsigned long TIMEOUT_CHAR_MS  =   4000;
const unsigned long TIMEOUT_FINAL_MS = 900000;  // 15 min: cubre escritura real con servos y barridos largos
