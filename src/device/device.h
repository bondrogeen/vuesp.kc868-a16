#ifndef DEVICE_H
#define DEVICE_H

#include <Wire.h>

#include "../libs/UnixTime.h"
#include "./gpio/gpio.h"
#include "./runner/runner.h"

#define GPIO_SDA 4
#define GPIO_SCL 5

#define GPIO_ADC1 36
#define GPIO_ADC2 34
#define GPIO_ADC3 35
#define GPIO_ADC4 39

void setupDevice();
void setupFirstDevice();
void deviceGPIO(Port* port, uint8_t type);
void loopDevice(uint32_t now);
void sendDeviceTo(uint32_t clientId);
void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task);

#endif
