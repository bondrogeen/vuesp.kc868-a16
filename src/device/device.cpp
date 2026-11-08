#include "./device.h"

#include <Wire.h>

Buffer myBuffer = {KEY_BUFFER};
Device device = {KEY_DEVICE, 0, 0, 0, 0, "", 255, 255, 0, 0, 0, 0, 0, 0};
uint32_t lastTimeDevice = 0;

void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  if (!arg || !data || len != sizeof(device)) return;

  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (task == KEY_DEVICE && info->len == sizeof(device)) {
    memcpy(&device, data, sizeof(device));
  }
}

void onSendDevice() {
  wsSendAll((uint8_t*)&device, sizeof(device));
}

void sendDeviceTo(uint32_t clientId) {
  wsSendTo(clientId, (uint8_t*)&device, sizeof(device));
}

void deviceGPIO(Port* port, uint8_t type) {
  Serial.printf("gpio:%d, value:%d", port->gpio, port->value);
  if (type == EVENT_LONG_PRESS) {
    Serial.print(", type:long");
  } else if (type == EVENT_REPEAT) {
    Serial.print(", type:repeat");
  } else if (type == EVENT_CLICK) {
    Serial.printf(", type:click, count:%d", port->count);
  }
  Serial.println("");
}

static char displayBuffer[65] = "5";

bool dataProvider(const char* id, DataKind kind, DataValue& value, bool write) {
  Serial.print(id);
  Serial.print(write ? "WRITE" : "READ");
  Serial.print(" kind=");
  Serial.println(kind);

  if (strcmp(id, "$display") == 0) {
    if (write) {
      if (kind == KIND_STRING) {
        if (!value.stringVal.data) return false;
        char buf[65];
        uint8_t len = value.stringVal.len;
        if (len > 64) len = 64;
        strncpy(buf, (char*)value.stringVal.data, len);
        buf[len] = '\0';
        strncpy(displayBuffer, buf, sizeof(displayBuffer) - 1);
        displayBuffer[sizeof(displayBuffer) - 1] = '\0';
        Serial.print("DISPLAY: ");
        Serial.println(buf);
      } else if (kind == KIND_INT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%d", value.intVal);
      } else if (kind == KIND_UINT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%u", value.uintVal);
      } else if (kind == KIND_FLOAT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%.2f", value.floatVal);
      }
      return true;
    }

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
  return false;
}

bool httpHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (!params || paramCount < 5) return false;
  if (!params[0].stringVal.data || !params[1].stringVal.data) return false;
  if (params[4].arrayVal.len > 0 && !params[4].arrayVal.data) return false;

  const char* method = params[0].stringVal.data;
  const char* url = params[1].stringVal.data;
  const int32_t count = params[2].intVal;
  const uint32_t num = params[3].uintVal;

  const uint8_t* data = params[4].arrayVal.data;
  uint8_t len = params[4].arrayVal.len;

  for (uint8_t i = 0; i < len; i++) {
    Serial.print(data[i]);
  }

  Serial.println("method");
  Serial.println(method);
  Serial.println(url);
  Serial.println(count);
  Serial.println(num);

  result.type = VAL_INT;
  result.intVal = 200;
  return true;
}

void readInputs() {
  Wire.requestFrom(0x21, 1);
  while (Wire.available()) {
    device.portIn1 = Wire.read();
  }
  Wire.requestFrom(0x22, 1);
  while (Wire.available()) {
    device.portIn2 = Wire.read();
  }

  device.adc1 = analogRead(GPIO_ADC1);
  device.adc2 = analogRead(GPIO_ADC2);
  device.adc3 = analogRead(GPIO_ADC3);
  device.adc4 = analogRead(GPIO_ADC4);
}

void writeOutputs() {
  Wire.beginTransmission(0x24);
  Wire.write(device.portOut1);
  Wire.endTransmission();

  Wire.beginTransmission(0x25);
  Wire.write(device.portOut2);
  Wire.endTransmission();
}

void setupDevice() {
  Wire.begin(GPIO_SDA, GPIO_SCL);
  scriptRunner.setDataProvider(dataProvider);
  scriptRunner.registerFunction("http", httpHandler);
}

void setupFirstDevice() {
  if (!loadConfigBlock(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device))) {
    saveConfigBlock(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
  }
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 2000) {
    lastTimeDevice = now;
    readInputs();
    onSendDevice();
  }

  if (tasks[KEY_BUFFER]) {
    wsSendAll((uint8_t*)&myBuffer, sizeof(myBuffer));
    tasks[KEY_BUFFER] = 0;
  }

  if (tasks[KEY_DEVICE]) {
    if (device.command == COMMAND_SAVE) saveConfigBlock(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));

    writeOutputs();
    device.command = 0;
    tasks[KEY_DEVICE] = 0;
    onSendDevice();
  }
}
