#ifndef GPIO_H
#define GPIO_H

#include "../const/protocol.h"

#define GPIO_EVENT_OFF 0
#define GPIO_EVENT_ON 1

#define GPIO_DISABLED_OFF 0
#define GPIO_DISABLED_ON 1

constexpr uint16_t PORT_ALL_NOT_DAC =
  PORT_INPUT | PORT_INPUT_PULLUP | PORT_OUTPUT | PORT_OUTPUT_OPEN_DRAIN | PORT_PWM | PORT_ONEWIRE | PORT_ADC;
constexpr uint16_t PORT_ALL_NOT_DAC_AND_ADC =
  PORT_INPUT | PORT_INPUT_PULLUP | PORT_OUTPUT | PORT_OUTPUT_OPEN_DRAIN | PORT_PWM | PORT_ONEWIRE;

#define GPIO_INTERRUPT_OFF 0
#define GPIO_INTERRUPT_RISING 1
#define GPIO_INTERRUPT_FALLING 2
#define GPIO_INTERRUPT_CHANGE 3

#define LONG_PRESS_TIME 1000
#define REPEAT_START_TIME 2000
#define REPEAT_INTERVAL 100
#define CLICK_WINDOW 200

#define EVENT_NONE 0
#define EVENT_LONG_PRESS 1
#define EVENT_REPEAT 2
#define EVENT_CLICK 3

#include <OneWire.h>

#include "../init/init.h"
#include "./add/fade.h"
#include "./webserver/webserver.h"

void setupGPIO();
void setupFirstGPIO();
void loopGPIO(uint32_t now);
uint16_t getValue(uint8_t gpio, uint16_t& value);
void setValue(uint8_t gpio, uint16_t value);
void setValue(uint8_t gpio, uint16_t value, uint8_t mode);
void updatePort(uint8_t gpio, uint16_t value);

extern Port ports[];
extern int ports_len;

#endif