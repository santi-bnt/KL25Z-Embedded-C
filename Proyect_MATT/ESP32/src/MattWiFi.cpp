#include "MattWiFi.h"

#include <WiFi.h>
#include "Config.h"
#include "Display.h"
#include "Logger.h"

static const uint8_t DNS_REINTENTOS = 3;
static const unsigned long DNS_REINTENTO_DELAY_MS = 250;
static const unsigned long DNS_CACHE_MS = 30000;
static const bool USAR_IP_FIJA = false;

static unsigned long ultimaPruebaDns = 0;
static bool dnsOk = false;

static bool reconectarConDnsFijo(const char* ssidActual, const char* passActual) {
  IPAddress localIP = WiFi.localIP();
  IPAddress gateway = WiFi.gatewayIP();
  IPAddress subnet = WiFi.subnetMask();
  IPAddress dns1(8, 8, 8, 8);
  IPAddress dns2(1, 1, 1, 1);

  if (localIP == IPAddress(0, 0, 0, 0) || gateway == IPAddress(0, 0, 0, 0)) {
    return false;
  }

  logSimple("DNS 0.0.0.0, fijando DNS...");
  mostrarDisplay("DNS manual", "Reintentando");

  WiFi.disconnect(false);
  delay(300);
  WiFi.config(localIP, gateway, subnet, dns1, dns2);
  WiFi.begin(ssidActual, passActual);

  for (uint8_t intento = 0; intento < 12 && WiFi.status() != WL_CONNECTED; intento++) {
    delay(250);
    Serial.print(".");
  }

  Serial.println();
  logSimple("IP: " + WiFi.localIP().toString());
  logSimple("Gateway: " + WiFi.gatewayIP().toString());
  logSimple("DNS: " + WiFi.dnsIP().toString());

  return WiFi.status() == WL_CONNECTED && WiFi.dnsIP() != IPAddress(0, 0, 0, 0);
}

bool probarUnDNS(const char* host) {
  for (uint8_t intento = 0; intento < DNS_REINTENTOS; intento++) {
    IPAddress ip;

    if (WiFi.hostByName(host, ip) && ip != IPAddress(0, 0, 0, 0)) {
      return true;
    }

    delay(DNS_REINTENTO_DELAY_MS);
  }

  return false;
}

bool probarDNSCompleto() {
  return probarUnDNS(firebaseHost);
}

bool conectarWiFi() {
  static bool wifiConfigurado = false;

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.persistent(false);

  if (!wifiConfigurado) {
    if (USAR_IP_FIJA) {
      IPAddress localIP(192, 168, 137, 216);
      IPAddress gateway(192, 168, 137, 1);
      IPAddress subnet(255, 255, 255, 0);
      IPAddress dns1(8, 8, 8, 8);
      IPAddress dns2(1, 1, 1, 1);

      WiFi.config(localIP, gateway, subnet, dns1, dns2);
    } else {
      WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    }

    wifiConfigurado = true;
  }

  logSimple("Conectando WiFi...");
  mostrarDisplay("Conectando WiFi", "");

  int redConectada = -1;

  for (int red = 0; red < wifiNetworkCount && WiFi.status() != WL_CONNECTED; red++) {
    WiFi.disconnect(false);
    delay(100);

    logSimple("Probando WiFi: " + String(wifiSsids[red]));
    mostrarDisplay("Probando WiFi", String(wifiSsids[red]).substring(0, 16));

    WiFi.begin(wifiSsids[red], wifiPasswords[red]);

    int intentos = 0;

    while (WiFi.status() != WL_CONNECTED && intentos < 8) {
      delay(250);
      Serial.print(".");

      String puntos = "";
      for (int i = 0; i < (intentos % 4); i++) puntos += ".";
      mostrarDisplay(String(wifiSsids[red]).substring(0, 16), puntos);

      intentos++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      redConectada = red;
    }
  }

  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    logSimple("Error WiFi");
    mostrarDisplay("WiFi apagado?", "Reintentando");
    return false;
  }

  logSimple("WiFi conectado");
  logSimple("IP: " + WiFi.localIP().toString());
  logSimple("Gateway: " + WiFi.gatewayIP().toString());
  logSimple("DNS: " + WiFi.dnsIP().toString());
  mostrarDisplay("WiFi OK", WiFi.localIP().toString());
  delay(300);

  if (WiFi.dnsIP() == IPAddress(0, 0, 0, 0) && redConectada >= 0) {
    if (!reconectarConDnsFijo(wifiSsids[redConectada], wifiPasswords[redConectada])) {
      logSimple("No se pudo fijar DNS");
      mostrarDisplay("DNS error", "Cambia red");
      return false;
    }
  }

  logSimple("Probando Firebase...");
  mostrarDisplay("Probando", "Firebase...");

  if (!probarDNSCompleto()) {
    logSimple("Error Firebase/red");
    logSimple("IP: " + WiFi.localIP().toString());
    logSimple("Gateway: " + WiFi.gatewayIP().toString());
    logSimple("DNS: " + WiFi.dnsIP().toString());
    mostrarDisplay("DNS lento", "Sigo intentando");
    delay(300);
  }

  dnsOk = true;
  ultimaPruebaDns = millis();

  logSimple("Firebase conectado");
  mostrarDisplay("Firebase OK", "Conectado");
  delay(300);

  logSimple("Esperando comandos...");
  mostrarInicioDisplay();

  return true;
}

bool internetListo() {
  if (WiFi.status() != WL_CONNECTED) {
    logSimple("WiFi desconectado");
    dnsOk = false;
    return false;
  }

  if (dnsOk && millis() - ultimaPruebaDns < DNS_CACHE_MS) {
    return true;
  }

  ultimaPruebaDns = millis();

  if (!probarDNSCompleto()) {
    logSimple("Error: Firebase no disponible");
    dnsOk = false;
    return false;
  }

  dnsOk = true;
  return true;
}
