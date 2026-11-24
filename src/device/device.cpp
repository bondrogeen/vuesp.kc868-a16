#include "./device.h"

#include "../config/ConfigStore.h"
#include "../init/init.h"
#include "../tasks/tasks.h"
#include "../webserver/webserver.h"

#define DEVICE_INPUT_COUNT 16
#define DEVICE_OUTPUT_COUNT 16
#define DEVICE_EVENT_INTERVAL_MS 50
#define DEVICE_INPUT_FALLBACK_MS 1000
#define DEVICE_ADC_INTERVAL_MS 1000

Buffer myBuffer = {KEY_BUFFER};

// input/output are logical masks: bit=1 means input active / output on.
Device device = {KEY_DEVICE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "text"};
uint32_t lastTimeDevice = 0;
static uint32_t lastEventPoll = 0;
static uint32_t lastInputRead = 0;
static uint32_t lastAdcPoll = 0;
static uint8_t rawInputLow = 0xFF;
static uint8_t rawInputHigh = 0xFF;
static volatile bool pcfIrq = false;

struct InputState {
  uint8_t value;
  uint8_t valueOld;
  uint8_t count;
  uint8_t isPressed;
  uint32_t pressStart;
};

static InputState inputStates[DEVICE_INPUT_COUNT];

static const uint8_t adcGpios[4] = {GPIO_ADC1, GPIO_ADC2, GPIO_ADC3, GPIO_ADC4};

static bool pcfRead(uint8_t address, uint8_t& value) {
  Wire.requestFrom(address, (uint8_t)1);
  if (!Wire.available()) return false;
  value = (uint8_t)Wire.read();
  return true;
}

static void pcfWrite(uint8_t address, uint8_t value) {
  Wire.beginTransmission(address);
  Wire.write(value);
  Wire.endTransmission();
}

static void IRAM_ATTR pcfIsr() { pcfIrq = true; }

void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  (void)clientId;
  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (task == KEY_DEVICE && info->len == sizeof(device) && len >= sizeof(device)) {
    memcpy(&device, data, sizeof(device));
  }
}

void onSendDevice() {
  wsSendAll((uint8_t*)&device, sizeof(device));
}

void getInput() {
  uint8_t low = rawInputLow;
  uint8_t high = rawInputHigh;
  if (pcfRead(PCF_INPUT_LOW, low)) rawInputLow = low;
  else low = rawInputLow;
  if (pcfRead(PCF_INPUT_HIGH, high)) rawInputHigh = high;
  else high = rawInputHigh;

  // PCF8574 inputs are pulled low by the optocoupler. Store the logical
  // active state so the UI, ScriptRunner and Modbus agree on bit=1=ON.
  device.input = (uint16_t)(~(uint16_t)(low | ((uint16_t)high << 8)));
}

void getOutput() {
  uint8_t low = 0;
  uint8_t high = 0;
  if (!pcfRead(PCF_OUTPUT_LOW, low)) low = (uint8_t)(~device.output);
  if (!pcfRead(PCF_OUTPUT_HIGH, high)) high = (uint8_t)(~device.output >> 8);
  device.output = (uint16_t)(~(uint16_t)(low | ((uint16_t)high << 8)));
}

void setOutput() {
  const uint16_t raw = (uint16_t)~device.output;
  pcfWrite(PCF_OUTPUT_LOW, (uint8_t)raw);
  pcfWrite(PCF_OUTPUT_HIGH, (uint8_t)(raw >> 8));
}

static void readAdc() {
  device.adc1 = (uint16_t)analogRead(adcGpios[0]);
  device.adc2 = (uint16_t)analogRead(adcGpios[1]);
  device.adc3 = (uint16_t)analogRead(adcGpios[2]);
  device.adc4 = (uint16_t)analogRead(adcGpios[3]);
  for (int i = 0; i < ports_len; i++) {
    for (uint8_t a = 0; a < 4; a++) {
      if (ports[i].gpio == adcGpios[a]) {
        ports[i].value = (&device.adc1)[a];
      }
    }
  }
}

static void inputEvent(uint8_t index, uint8_t type, uint8_t value, uint8_t count) {
  switch (type) {
    case EVENT_CLICK:
      scriptRunner.emitEvent("port_c", 3, (int32_t)index, (int32_t)value, (int32_t)count);
      break;
    case EVENT_LONG_PRESS:
      scriptRunner.emitEvent("port_l", 2, (int32_t)index, (int32_t)value);
      break;
    case EVENT_REPEAT:
      scriptRunner.emitEvent("port_r", 2, (int32_t)index, (int32_t)value);
      break;
    default:
      break;
  }
}

static bool readInputs(uint32_t now) {
  const uint16_t previous = device.input;
  getInput();
  const uint16_t changed = (uint16_t)(previous ^ device.input);
  if (!changed) return false;

  for (uint8_t i = 0; i < DEVICE_INPUT_COUNT; i++) {
    if ((changed & (1u << i)) == 0) continue;
    InputState& state = inputStates[i];
    const uint8_t value = (uint8_t)((device.input >> i) & 1);
    state.isPressed = value;
    if (value) {
      state.count++;
      state.pressStart = now;
    }
    state.valueOld = state.value;
    state.value = value;
  }
  return true;
}

static void processInputEvents(uint32_t now) {
  for (uint8_t i = 0; i < DEVICE_INPUT_COUNT; i++) {
    InputState& state = inputStates[i];
    const uint32_t elapsed = now - state.pressStart;

    if (elapsed > REPEAT_START_TIME) {
      if (state.isPressed && state.count == 1) inputEvent(i, EVENT_REPEAT, state.value, state.count);
      else if (!state.isPressed) state.count = 0;
    } else if (elapsed > LONG_PRESS_TIME) {
      if (!state.isPressed && state.count == 1) {
        inputEvent(i, EVENT_LONG_PRESS, state.value, state.count);
        state.count = 0;
      }
    } else if (elapsed > CLICK_WINDOW) {
      if (!state.isPressed && state.count) {
        inputEvent(i, EVENT_CLICK, state.value, state.count);
        state.count = 0;
      }
    }
  }
}

static char displayBuffer[64] = "5";

bool dataProvider(const char* id, DataKind kind, DataValue& value, bool write) {
#if VUESP_LOG_LEVEL >= 3
  Serial.print(id);
  Serial.print(write ? "WRITE" : "READ");
  Serial.print(" kind=");
  Serial.println(kind);
#endif

  if (strcmp(id, "$display") == 0) {
    if (write) {
      if (kind == KIND_STRING) {
        char buf[65];
        uint8_t len = value.stringVal.len;
        if (len > 64) len = 64;
        strncpy(buf, (char*)value.stringVal.data, len);
        buf[len] = '\0';
        strcpy(displayBuffer, buf);
#if VUESP_LOG_LEVEL >= 3
        Serial.print("DISPLAY: ");
        Serial.println(buf);
#endif
      } else if (kind == KIND_INT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%d", value.intVal);
#if VUESP_LOG_LEVEL >= 3
        Serial.print("DISPLAY: ");
        Serial.println(value.intVal);
#endif
      } else if (kind == KIND_UINT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%u", value.uintVal);
#if VUESP_LOG_LEVEL >= 3
        Serial.print("DISPLAY: ");
        Serial.println(value.uintVal);
#endif
      } else if (kind == KIND_FLOAT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%.2f", value.floatVal);
#if VUESP_LOG_LEVEL >= 3
        Serial.print("DISPLAY: ");
        Serial.println(value.floatVal);
#endif
      }
      return true;
    } else {
      if (kind == KIND_STRING) {
        value.stringVal.data = (uint8_t*)displayBuffer;
        value.stringVal.len = strlen(displayBuffer);
      } else if (kind == KIND_INT) {
        value.intVal = atoi(displayBuffer);
      } else if (kind == KIND_UINT) {
        value.uintVal = (uint32_t)atoi(displayBuffer);
      } else if (kind == KIND_FLOAT) {
        value.floatVal = atof(displayBuffer);
      }
      return true;
    }
  }
  return false;
}

bool httpHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 2) return false;

  const char* method = params[0].stringVal.data;
  const char* url = params[1].stringVal.data;
  const int32_t count = params[2].intVal;
  const uint32_t num = params[3].uintVal;

  const uint8_t* data = params[4].arrayVal.data;
  uint8_t len = params[4].arrayVal.len;

#if VUESP_LOG_LEVEL >= 3
  for (uint8_t i = 0; i < len; i++) {
    Serial.print(data[i]);
  }

  Serial.println("method");
  Serial.println(method);
  Serial.println(url);
  Serial.println(count);
  Serial.println(num);
#else
  (void)data;
  (void)len;
  (void)method;
  (void)url;
  (void)count;
  (void)num;
#endif

  result.type = VAL_INT;
  result.intVal = 200;
  return true;
}


void deviceGPIO(Port* port, uint8_t type) {
#if VUESP_LOG_LEVEL >= 2
  Serial.printf("gpio:%d, value:%d, event:%d\n", port->gpio, port->value, type);
#else
  (void)port;
  (void)type;
#endif
}

bool inputHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  (void)userData;
  if (paramCount != 1) return false;
  const uint32_t pin = params[0].uintVal;
  if (pin < 1 || pin > DEVICE_INPUT_COUNT) return false;
  result.type = VAL_UINT;
  result.uintVal = (device.input >> (pin - 1)) & 1;
  return true;
}

bool outputHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  (void)userData;
  if (paramCount < 1) return false;
  const uint32_t pin = params[0].uintVal;
  if (pin < 1 || pin > DEVICE_OUTPUT_COUNT) return false;

  if (paramCount >= 2) {
    if (params[1].uintVal) bitSet(device.output, pin - 1);
    else bitClear(device.output, pin - 1);
    setOutput();
    onSendDevice();
  }

  result.type = VAL_UINT;
  result.uintVal = (device.output >> (pin - 1)) & 1;
  return true;
}

void setupDevice() {
  scriptRunner.setDataProvider(dataProvider);
  scriptRunner.registerFunction("http", httpHandler);
  scriptRunner.registerFunction("input", inputHandler);
  scriptRunner.registerFunction("output", outputHandler);

  Wire.begin(GPIO_SDA, GPIO_SCL);
  pinMode(PCF_INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PCF_INT_PIN), pcfIsr, FALLING);
  setOutput();
  getOutput();
  getInput();

  for (uint8_t i = 0; i < DEVICE_INPUT_COUNT; i++) {
    const uint8_t value = (uint8_t)((device.input >> i) & 1);
    inputStates[i] = {value, value, 0, 0, 0};
  }
  readAdc();
}

void setupFirstDevice() {
  const TConfigStatus status =
      configStore.read("device", &device, sizeof(device), CONFIG_DEVICE_VERSION);
  if (status == TConfigStatus::Ok) {
    device.command = 0;
  }
#if VUESP_LOG_LEVEL >= 1
  else if (status != TConfigStatus::NotFound) {
    Serial.printf("[config] device read: %d\n", (int)status);
  }
#endif
}

void loopDevice(uint32_t now) {
  if (now - lastEventPoll >= DEVICE_EVENT_INTERVAL_MS) {
    lastEventPoll = now;
    processInputEvents(now);
  }

  // The PCF8574 nINT line wakes this read. The 1 s fallback keeps the inputs
  // working if the wire is missing or the interrupt is missed.
  if (pcfIrq || now - lastInputRead >= DEVICE_INPUT_FALLBACK_MS) {
    pcfIrq = false;
    lastInputRead = now;
    if (readInputs(now)) onSendDevice();
  }

  if (now - lastAdcPoll >= DEVICE_ADC_INTERVAL_MS) {
    lastAdcPoll = now;
    const uint16_t previous[4] = {device.adc1, device.adc2, device.adc3, device.adc4};
    readAdc();
    if (previous[0] != device.adc1 || previous[1] != device.adc2 ||
        previous[2] != device.adc3 || previous[3] != device.adc4) {
      onSendDevice();
    }
  }

  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    getOutput();
    onSendDevice();
  }

  if (tasks[KEY_BUFFER]) {
    wsSendAll((uint8_t*)&myBuffer, sizeof(myBuffer));
    tasks[KEY_BUFFER] = 0;
  }

  if (tasks[KEY_DEVICE]) {
    if (device.command == CMD_SET) {
      setOutput();
    } else if (device.command == CMD_SAVE) {
      const TConfigStatus status =
          configStore.write("device", &device, sizeof(device), CONFIG_DEVICE_VERSION);
#if VUESP_LOG_LEVEL >= 1
      if (status != TConfigStatus::Ok) {
        Serial.printf("[config] device write: %d\n", (int)status);
      }
#endif
    } else {
      getInput();
      getOutput();
      readAdc();
    }

    device.command = 0;
    tasks[KEY_DEVICE] = 0;
    onSendDevice();
  }
}
