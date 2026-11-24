#include "./gpio.h"

#include "../config/ConfigStore.h"

#if defined(ESP8266)
Port ports[6] = {
    {KEY_PORT, 4, 0, PORT_INPUT_PULLUP, GPIO_INTERRUPT_CHANGE, PORT_ALL_NOT_DAC_AND_ADC, GPIO_DISABLED_OFF, GPIO_EVENT_ON},
    {KEY_PORT, 5, 0, PORT_INPUT_PULLUP, GPIO_INTERRUPT_CHANGE, PORT_ALL_NOT_DAC_AND_ADC, GPIO_DISABLED_OFF, GPIO_EVENT_ON},
    {KEY_PORT, 12, 0, PORT_OUTPUT, GPIO_INTERRUPT_OFF, PORT_ALL_NOT_DAC_AND_ADC, GPIO_DISABLED_OFF},
    {KEY_PORT, 13, 255, PORT_PWM, GPIO_INTERRUPT_OFF, PORT_ALL_NOT_DAC_AND_ADC, GPIO_DISABLED_OFF},
    {KEY_PORT, 14, 0, PORT_PWM, GPIO_INTERRUPT_OFF, PORT_ALL_NOT_DAC_AND_ADC, GPIO_DISABLED_OFF},
    {KEY_PORT, 17, 0, PORT_ADC, GPIO_INTERRUPT_OFF, PORT_ADC, GPIO_DISABLED_ON},
};
#elif defined(ARDUINO_ESP32C3_DEV)
Port ports[5] = {
    {KEY_PORT, 0, 0, PORT_PWM, GPIO_INTERRUPT_OFF, PORT_ALL_NOT_DAC, GPIO_DISABLED_ON},
    {KEY_PORT, 1, 0, PORT_PWM, GPIO_INTERRUPT_OFF, PORT_ALL_NOT_DAC, GPIO_DISABLED_ON},
    {KEY_PORT, 2, 0, PORT_PWM, GPIO_INTERRUPT_OFF, PORT_ALL_NOT_DAC, GPIO_DISABLED_ON},
    {KEY_PORT, 3, 0, PORT_PWM, GPIO_INTERRUPT_OFF, PORT_ALL_NOT_DAC, GPIO_DISABLED_ON},
    {KEY_PORT, 4, 0, PORT_PWM, GPIO_INTERRUPT_OFF, PORT_ALL_NOT_DAC, GPIO_DISABLED_ON},
};
#elif defined(ESP32)
// KC868-A16: the 16 inputs/outputs live behind PCF8574. Only the two
// remaining HT sensor terminals and the four ADC inputs are exposed as Vuesp ports.
Port ports[6] = {
    {KEY_PORT, GPIO_HT1, 0, PORT_INPUT_PULLUP, GPIO_INTERRUPT_CHANGE, PORT_ALL_NOT_DAC_AND_ADC, GPIO_DISABLED_OFF, GPIO_EVENT_ON},
    {KEY_PORT, GPIO_HT2, 0, PORT_INPUT_PULLUP, GPIO_INTERRUPT_CHANGE, PORT_ALL_NOT_DAC_AND_ADC, GPIO_DISABLED_OFF, GPIO_EVENT_ON},
    {KEY_PORT, GPIO_ADC1, 0, PORT_ADC, GPIO_INTERRUPT_OFF, PORT_ADC, GPIO_DISABLED_OFF},
    {KEY_PORT, GPIO_ADC2, 0, PORT_ADC, GPIO_INTERRUPT_OFF, PORT_ADC, GPIO_DISABLED_OFF},
    {KEY_PORT, GPIO_ADC3, 0, PORT_ADC, GPIO_INTERRUPT_OFF, PORT_ADC, GPIO_DISABLED_OFF},
    {KEY_PORT, GPIO_ADC4, 0, PORT_ADC, GPIO_INTERRUPT_OFF, PORT_ADC, GPIO_DISABLED_OFF},
};
#endif

OneWire* ds = nullptr;
Fade fade;
Dallas ht1 = {KEY_DALLAS};

int ports_len = sizeof(ports) / sizeof(ports[0]);

static constexpr size_t PORTS_MAX = sizeof(ports) / sizeof(ports[0]);

struct PortsBlob {
  uint8_t count;
  Port ports[PORTS_MAX];
} __attribute__((packed));

static PortsBlob portsBlob;

static void sanitizePort(Port& port) {
  port.command = 0;
  port.isPressed = 0;
  port.count = 0;
  port.empty = 0;
  port.valueOld = 0;
  port.pressStart = 0;
}

volatile uint8_t btnStatus = 0;
uint8_t timeDebounce = 0;
uint32_t debounce = 0;
uint32_t lastLoopInterrupt = 0;
uint32_t lastTimeGPIO = 0;
uint32_t lastTimeADC = 0;
uint32_t isOneWire = 0;
uint32_t isADC = 0;

void ICACHE_RAM_ATTR btnIsr() {
  btnStatus = 1;
}

uint8_t pinModeOf(uint8_t mode) {
  switch (mode) {
    case PORT_OUTPUT:
      return OUTPUT;
    case PORT_INPUT_PULLUP:
      return INPUT_PULLUP;
    case PORT_OUTPUT_OPEN_DRAIN:
      return OUTPUT_OPEN_DRAIN;
    default:
      return INPUT;
  }
}

void initGPIO() {
  for (uint8_t i = 0; i < ports_len; i++) {
    if (ports[i].mode == PORT_ONEWIRE) {
      if (!isOneWire) {
        if (ds) delete ds;
        ds = new OneWire(ports[i].gpio);
        isOneWire = 1;
      }
      continue;
    }
    if (ports[i].mode == PORT_PWM) {
      pinMode(ports[i].gpio, OUTPUT);
      analogWrite(ports[i].gpio, ports[i].value);
    } else if (ports[i].mode == PORT_ADC) {
      pinMode(ports[i].gpio, INPUT);
      ports[i].value = analogRead(ports[i].gpio);
      isADC = 1;
    } else if (ports[i].mode == PORT_DAC) {
#if defined(ESP32) && !defined(ARDUINO_ESP32C3_DEV)
      dacWrite(ports[i].gpio, ports[i].value);
#endif
    } else {
      pinMode(ports[i].gpio, pinModeOf(ports[i].mode));
      if (ports[i].mode == PORT_OUTPUT || ports[i].mode == PORT_OUTPUT_OPEN_DRAIN) digitalWrite(ports[i].gpio, ports[i].value);

      ports[i].value = digitalRead(ports[i].gpio);
      ports[i].valueOld = ports[i].value;
      if ((ports[i].mode == PORT_INPUT || ports[i].mode == PORT_INPUT_PULLUP) && ports[i].interrupt) {
        attachInterrupt(digitalPinToInterrupt(ports[i].gpio), btnIsr, ports[i].interrupt);
      }
    }
  }
}

void getAll() {
  for (int i = 0; i < ports_len; i++) {
    if (ports[i].mode == PORT_PWM || ports[i].mode == PORT_ONEWIRE || ports[i].mode == PORT_DAC) {
    } else if (ports[i].mode == PORT_ADC) {
      ports[i].value = analogRead(ports[i].gpio);
    } else {
      ports[i].value = digitalRead(ports[i].gpio);
    }
    wsSendAll((uint8_t*)&ports[i], sizeof(ports[i]));
  }
}

void getADC() {
  for (int i = 0; i < ports_len; i++) {
    if (ports[i].mode == PORT_ADC) {
      ports[i].value = analogRead(ports[i].gpio);
      wsSendAll((uint8_t*)&ports[i], sizeof(ports[i]));
    }
  }
}

void updatePort() {
  for (int i = 0; i < ports_len; i++) {
    if (ports[i].gpio == port.gpio) {
      ports[i] = port;
    }
  }
  wsSendAll((uint8_t*)&port, sizeof(port));
}

void updatePort(uint8_t gpio, uint16_t value) {
  for (int i = 0; i < ports_len; i++) {
    if (ports[i].gpio == gpio) {
      ports[i].value = value;
      port = ports[i];
    }
  }
  wsSendAll((uint8_t*)&port, sizeof(port));
}

void setValue(uint8_t gpio, uint16_t value, uint8_t mode) {
  if (mode == PORT_PWM) {
    analogWrite(gpio, value);
  } else {
    if (!(mode == PORT_INPUT || mode == PORT_INPUT_PULLUP)) digitalWrite(gpio, value);
  }
}

void setValue(uint8_t gpio, uint16_t value) {
  for (int i = 0; i < ports_len; i++) {
    if (ports[i].gpio == gpio) {
      setValue(gpio, value, ports[i].mode);
      break;
    }
  }
}

uint16_t getValue(uint8_t gpio, uint16_t& value) {
  for (int i = 0; i < ports_len; i++) {
    if (ports[i].gpio == gpio) {
      value = ports[i].value;
      return true;
    }
  }
  return false;
}

void setValueUpdate() {
  setValue(port.gpio, port.value, port.mode);
  if (port.mode == PORT_PWM || port.mode == PORT_ONEWIRE) {
  } else if (port.mode == PORT_ADC) {
    port.value = analogRead(port.gpio);
  } else if (port.mode == PORT_DAC) {
#if defined(ARDUINO_ESP32C3_DEV)
#elif defined(ESP32)
    dacWrite(port.gpio, port.value);
#endif
  } else {
    port.value = digitalRead(port.gpio);
  }
  updatePort();
}

void loopInterrupt(uint32_t now) {
  for (int i = 0; i < ports_len; i++) {
    if (!(ports[i].mode == PORT_INPUT || ports[i].mode == PORT_INPUT_PULLUP) && !ports[i].interrupt) continue;
    uint32_t time = now - ports[i].pressStart;
    if (time > REPEAT_START_TIME) {
      if (ports[i].isPressed && ports[i].count == 1) {
        if (ports[i].isButton) {
          deviceGPIO(&ports[i], EVENT_REPEAT);
          scriptRunner.emitEvent("btn_r", 2, ports[i].gpio, ports[i].value);
        }
      } else {
        ports[i].count = 0;
      }
    }
    if (time > LONG_PRESS_TIME && time < REPEAT_START_TIME) {
      if (!ports[i].isPressed && ports[i].count == 1) {
        if (ports[i].isButton) {
          deviceGPIO(&ports[i], EVENT_LONG_PRESS);
          scriptRunner.emitEvent("btn_l", 2, ports[i].gpio, ports[i].value);
        }
        ports[i].count = 0;
      }
    }
    if (time > CLICK_WINDOW) {
      if (!ports[i].isPressed && ports[i].count) {
        if (ports[i].isButton) {
          deviceGPIO(&ports[i], EVENT_CLICK);
          scriptRunner.emitEvent("btn_c", 3, ports[i].gpio, ports[i].value, ports[i].count);
        }
        ports[i].count = 0;
      }
    }
  }
}

void checkInterrupt(uint32_t now) {
  for (int i = 0; i < ports_len; i++) {
    if (!(ports[i].mode == PORT_INPUT || ports[i].mode == PORT_INPUT_PULLUP) && !ports[i].interrupt) continue;

    port = ports[i];
    port.value = digitalRead(port.gpio);
    if (ports[i].interrupt == GPIO_INTERRUPT_CHANGE && ports[i].value != port.value) {
      ports[i].isPressed = ports[i].valueOld != port.value;
      if (ports[i].isPressed) {
        ports[i].count++;
        ports[i].pressStart = now;
      }
      ports[i].value = port.value;
      scriptRunner.emitEvent("btn", 2, ports[i].gpio, ports[i].value);
      deviceGPIO(&ports[i], EVENT_NONE);
    }
    wsSendAll((uint8_t*)&ports[i], sizeof(ports[i]));
  }
}

float getTemperature(uint8_t* address1) {
  if (!ds) return -127.0f;
  uint16_t temp;
  ds->reset();
  ds->select(address1);
  ds->write(0xBE);
  temp = (ds->read() | ds->read() << 8);

  ds->reset();
  ds->select(address1);
  ds->write(0x44, 1);
  return (float)temp / 16.0;
}

void findDallas() {
  if (!ds) return;
  while (ds->search(ht1.address) == 1) {
    ht1.temp = getTemperature(ht1.address);
    wsSendAll((uint8_t*)&ht1, sizeof(ht1));
  }
}

void stateChangeProvider(uint8_t gpio, uint16_t value) {
  updatePort(gpio, value);
}

bool portProvider(uint8_t gpio, uint8_t action, uint16_t& value) {
  switch (action) {
    case PORT_READ:
      return getValue(gpio, value);
    case PORT_WRITE:
      setValue(gpio, value);
      return true;
  }
  return false;
}

bool fadeHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 3) return false;
  const uint32_t port = params[0].uintVal;
  const uint32_t value = params[1].uintVal;
  const uint32_t time = params[2].uintVal;
  result.type = VAL_INT;
  result.intVal = fade.start(port, value, time);
  return true;
}

void setupGPIO() {
  scriptRunner.setStateChangeProvider(stateChangeProvider);
  scriptRunner.setPortProvider(portProvider);
  scriptRunner.registerFunction("fade", fadeHandler);

  fade.init(5, 3);
  fade.setDataProvider(portProvider);
  fade.setStateChangeProvider(stateChangeProvider);
}

void setupFirstGPIO() {
  const TConfigStatus status =
      configStore.read("ports", &portsBlob, sizeof(portsBlob), CONFIG_PORTS_VERSION);
  if (status == TConfigStatus::Ok && portsBlob.count <= PORTS_MAX) {
    for (uint8_t i = 0; i < portsBlob.count; i++) {
      Port saved = portsBlob.ports[i];
      sanitizePort(saved);

      for (int j = 0; j < ports_len; j++) {
        if (ports[j].gpio == saved.gpio) {
          ports[j] = saved;
          break;
        }
      }
    }
  }
#if VUESP_LOG_LEVEL >= 1
  else if (status != TConfigStatus::NotFound) {
    Serial.printf("[config] ports read: %d\n", (int)status);
  }
#endif
  initGPIO();
}

void loopGPIO(uint32_t now) {
  fade.loop();

  if (btnStatus == 1) {
    btnStatus = 2;
    debounce = now;
  }
  if (btnStatus == 2 && now - debounce > timeDebounce) {
    btnStatus = 0;
    checkInterrupt(now);
  }

  if (now - lastLoopInterrupt > REPEAT_INTERVAL) {
    lastLoopInterrupt = now;
    loopInterrupt(now);
  }

  if (now - lastTimeGPIO > 10000) {
    lastTimeGPIO = now;
    if (isOneWire) {
      findDallas();
    }
  }

  if (isADC && now - lastTimeADC > 5000) {
    lastTimeADC = now;
    getADC();
  }

  if (tasks[KEY_PORT]) {
    if (port.command == CMD_SAVE) {
      portsBlob.count = (uint8_t)ports_len;
      for (int i = 0; i < ports_len; i++) {
        portsBlob.ports[i] = ports[i];
        sanitizePort(portsBlob.ports[i]);
      }
      const TConfigStatus status =
          configStore.write("ports", &portsBlob, 1 + (size_t)ports_len * sizeof(Port),
                            CONFIG_PORTS_VERSION);
#if VUESP_LOG_LEVEL >= 1
      if (status != TConfigStatus::Ok) {
        Serial.printf("[config] ports write: %d\n", (int)status);
      }
#endif
    }
    if (port.command == CMD_SET) setValueUpdate();

    if (port.command == CMD_GET_ALL) getAll();
    if (port.command == CMD_CHANGE) updatePort();
    port.command = 0;
    tasks[KEY_PORT] = 0;
  };
}