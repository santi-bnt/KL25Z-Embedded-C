#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <WiFiClientSecure.h>

// =======================================================
// WIFI
// =======================================================
extern const char* ssid;
extern const char* password;
extern const char* wifiSsids[];
extern const char* wifiPasswords[];
extern const int wifiNetworkCount;

// =======================================================
// FIREBASE
// =======================================================
extern String databaseURL;
extern const char* firebaseHost;
extern WiFiClientSecure client;

// =======================================================
// UART ESP32 <-> KL25Z
// =======================================================
extern const int UART_TX;
extern const int UART_RX;
extern const int UART_BAUD;

extern const unsigned long TIMEOUT_CHAR_MS;
extern const unsigned long TIMEOUT_FINAL_MS;

#endif
