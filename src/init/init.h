#ifndef INIT_H
#define INIT_H

#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif

#include "../const/index.h"

void initWiFi();
void loopWiFi(uint32_t now);
void initEthernet();
void updateEthernetInfo(Info* info);
void reboot();
extern Settings settings;
extern Info infoFS;
extern uint8_t isConnected;
extern Event event;

#endif