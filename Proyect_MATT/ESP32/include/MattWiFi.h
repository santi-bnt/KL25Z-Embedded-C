#ifndef MATT_WIFI_H
#define MATT_WIFI_H

#include <Arduino.h>

bool probarUnDNS(const char* host);
bool probarDNSCompleto();
bool conectarWiFi();
bool internetListo();

#endif
