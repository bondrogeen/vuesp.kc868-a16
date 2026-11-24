#include "./modbus_source.h"

#include <string.h>

#include "./modbus.h"

#include "../device/device.h"
#include "../gpio/gpio.h"
#include "../init/init.h"

static bool splitPath(const char* path, char* root, size_t rootSize, char* key, size_t keySize, char* field, size_t fieldSize) {
  if (!path || !root || !key || !field || rootSize == 0 || keySize == 0 || fieldSize == 0) return false;
  if (!path[0] || strlen(path) >= 32) return false;

  const char* first = strchr(path, '.');
  if (!first || first == path) return false;
  const size_t rootLen = (size_t)(first - path);
  if (rootLen >= rootSize) return false;
  memcpy(root, path, rootLen);
  root[rootLen] = '\0';

  const char* rest = first + 1;
  if (!rest[0]) return false;
  const char* second = strchr(rest, '.');
  if (second) {
    if (second == rest) return false;
    const size_t keyLen = (size_t)(second - rest);
    if (keyLen >= keySize) return false;
    memcpy(key, rest, keyLen);
    key[keyLen] = '\0';
    const char* tail = second + 1;
    if (!tail[0] || strchr(tail, '.')) return false;
    if (strlen(tail) >= fieldSize) return false;
    strcpy(field, tail);
  } else {
    if (strlen(rest) >= fieldSize) return false;
    strcpy(field, rest);
    key[0] = '\0';
  }
  return true;
}

struct ParsedSource {
  char root[12];
  char key[20];
  char field[16];
  bool valid;
};

static ParsedSource sourceCache[MODBUS_VARS_MAX];

void modbusSourceRebuild() {
  for (uint8_t i = 0; i < MODBUS_VARS_MAX; i++) {
    memset(&sourceCache[i], 0, sizeof(sourceCache[i]));
    sourceCache[i].valid = splitPath(modbusProfile.vars[i].source, sourceCache[i].root, sizeof(sourceCache[i].root),
                                     sourceCache[i].key, sizeof(sourceCache[i].key),
                                     sourceCache[i].field, sizeof(sourceCache[i].field));
  }
}

static Port* findPortByGpio(uint8_t gpio) {
  for (int i = 0; i < ports_len; i++) {
    if (ports[i].gpio == gpio) return &ports[i];
  }
  return nullptr;
}

static bool parseRom(const char* text, uint8_t rom[8]) {
  if (!text || strlen(text) != 16) return false;
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t value = 0;
    for (uint8_t j = 0; j < 2; j++) {
      const char c = text[i * 2 + j];
      uint8_t digit = 0;
      if (c >= '0' && c <= '9') digit = (uint8_t)(c - '0');
      else if (c >= 'a' && c <= 'f') digit = (uint8_t)(c - 'a' + 10);
      else if (c >= 'A' && c <= 'F') digit = (uint8_t)(c - 'A' + 10);
      else return false;
      value = (uint8_t)((value << 4) | digit);
    }
    rom[i] = value;
  }
  return true;
}

static bool readPortField(const Port& port, const char* field, double& value) {
  if (strcmp(field, "key") == 0) value = port.key;
  else if (strcmp(field, "gpio") == 0) value = port.gpio;
  else if (strcmp(field, "value") == 0) value = port.value;
  else if (strcmp(field, "mode") == 0) value = port.mode;
  else if (strcmp(field, "interrupt") == 0) value = port.interrupt;
  else if (strcmp(field, "list") == 0) value = port.list;
  else if (strcmp(field, "disabled") == 0) value = port.disabled;
  else if (strcmp(field, "isButton") == 0) value = port.isButton;
  else if (strcmp(field, "command") == 0) value = port.command;
  else if (strcmp(field, "isPressed") == 0) value = port.isPressed;
  else if (strcmp(field, "count") == 0) value = port.count;
  else if (strcmp(field, "empty") == 0) value = port.empty;
  else if (strcmp(field, "valueOld") == 0) value = port.valueOld;
  else if (strcmp(field, "pressStart") == 0) value = port.pressStart;
  else return false;
  return true;
}

static bool readDeviceField(const char* field, double& value) {
  if (strcmp(field, "command") == 0) value = device.command;
  else if (strcmp(field, "input") == 0) value = device.input;
  else if (strcmp(field, "output") == 0) value = device.output;
  else if (strcmp(field, "analog") == 0) value = device.analog;
  else if (strcmp(field, "now") == 0) value = device.now;
  else if (strcmp(field, "list") == 0) value = device.list;
  else return false;
  return true;
}

static bool readInfoField(const char* field, double& value) {
  if (strcmp(field, "totalBytes") == 0) value = infoFS.totalBytes;
  else if (strcmp(field, "usedBytes") == 0) value = infoFS.usedBytes;
  else if (strcmp(field, "id") == 0) value = infoFS.id;
  else if (strcmp(field, "uptime") == 0) value = (double)infoFS.uptime;
  else if (strcmp(field, "board") == 0) value = infoFS.board;
  else if (strcmp(field, "freeHeap") == 0) value = infoFS.freeHeap;
  else if (strcmp(field, "minFreeHeap") == 0) value = infoFS.minFreeHeap;
  else if (strcmp(field, "maxBlock") == 0) value = infoFS.maxBlock;
  else if (strcmp(field, "stackFree") == 0) value = infoFS.stackFree;
  else if (strcmp(field, "loopLastUs") == 0) value = infoFS.loopLastUs;
  else if (strcmp(field, "loopMaxUs") == 0) value = infoFS.loopMaxUs;
  else if (strcmp(field, "loopP95Us") == 0) value = infoFS.loopP95Us;
  else if (strcmp(field, "loopCount") == 0) value = infoFS.loopCount;
  else if (strcmp(field, "fragmentation") == 0) value = infoFS.fragmentation;
  else if (strcmp(field, "clients") == 0) value = infoFS.clients;
  else if (strcmp(field, "scripts") == 0) value = infoFS.scripts;
  else if (strcmp(field, "peers") == 0) value = infoFS.peers;
  else if (strcmp(field, "wifi") == 0) value = infoFS.wifi;
  else if (strcmp(field, "diagnostics") == 0) value = infoFS.diagnostics;
  else return false;
  return true;
}

static bool readModbusField(const char* field, double& value) {
  if (strcmp(field, "role") == 0) value = modbus.role;
  else if (strcmp(field, "address") == 0) value = modbus.address;
  else if (strcmp(field, "baud") == 0) value = modbus.baud;
  else if (strcmp(field, "dataBits") == 0) value = modbus.dataBits;
  else if (strcmp(field, "parity") == 0) value = modbus.parity;
  else if (strcmp(field, "stopBits") == 0) value = modbus.stopBits;
  else if (strcmp(field, "status") == 0) value = modbus.status;
  else if (strcmp(field, "lastFunction") == 0) value = modbus.lastFunction;
  else if (strcmp(field, "lastAddress") == 0) value = modbus.lastAddress;
  else if (strcmp(field, "lastError") == 0) value = modbus.lastError;
  else if (strcmp(field, "requests") == 0) value = modbus.requests;
  else if (strcmp(field, "errors") == 0) value = modbus.errors;
  else if (strcmp(field, "varCount") == 0) value = modbus.varCount;
  else if (strcmp(field, "updated") == 0) value = modbus.updated;
  else if (strcmp(field, "point") == 0) value = modbus.point;
  else if (strcmp(field, "ok") == 0) value = modbus.ok;
  else if (strcmp(field, "value") == 0) value = modbus.value;
  else return false;
  return true;
}

static bool readSource(const ParsedSource& source, double& value, uint8_t& quality, bool& writable) {
  const char* root = source.root;
  const char* key = source.key;
  const char* field = source.field;
  quality = MODBUS_Q_DISABLED;
  writable = false;

  if (!source.valid) return false;

  if (strcmp(root, "ports") == 0) {
    if (!key[0]) return false;
    const int gpio = atoi(key);
    if (gpio < 0 || gpio > 255) return false;
    Port* port = findPortByGpio((uint8_t)gpio);
    if (!port) return false;
    if (!readPortField(*port, field, value)) return false;
    quality = MODBUS_Q_OK;
    writable = strcmp(field, "value") == 0;
    return true;
  }

  if (strcmp(root, "device") == 0) {
    if (key[0]) return false;
    if (!readDeviceField(field, value)) return false;
    quality = MODBUS_Q_OK;
    return true;
  }

  if (strcmp(root, "info") == 0) {
    if (key[0]) return false;
    if (!readInfoField(field, value)) return false;
    quality = MODBUS_Q_OK;
    return true;
  }

  if (strcmp(root, "modbus") == 0) {
    if (key[0]) return false;
    if (!readModbusField(field, value)) return false;
    quality = MODBUS_Q_OK;
    return true;
  }

  if (strcmp(root, "vars") == 0) {
    if (!key[0] || strcmp(field, "value") != 0) return false;
    for (uint8_t i = 0; i < modbusProfile.varCount; i++) {
      if (strcmp(modbusProfile.vars[i].id, key) != 0) continue;
      value = i < modbus.varCount ? modbus.vars[i].value : 0;
      quality = i < modbus.varCount ? (uint8_t)modbus.vars[i].quality : MODBUS_Q_STALE;
      return true;
    }
    quality = MODBUS_Q_STALE;
    return false;
  }

  (void)parseRom;
  return false;
}

bool modbusReadVarSource(uint8_t varIndex, double& value, char* text, size_t textSize, uint8_t& quality) {
  value = 0;
  if (text && textSize) text[0] = '\0';
  if (varIndex >= MODBUS_VARS_MAX) {
    quality = MODBUS_Q_DISABLED;
    return false;
  }
  bool writable = false;
  const bool ok = readSource(sourceCache[varIndex], value, quality, writable);
  if (!ok && quality == MODBUS_Q_DISABLED) value = 0;
  return ok;
}

bool modbusVarSourceWritable(uint8_t varIndex) {
  if (varIndex >= MODBUS_VARS_MAX) return false;
  double value = 0;
  uint8_t quality = MODBUS_Q_DISABLED;
  bool writable = false;
  readSource(sourceCache[varIndex], value, quality, writable);
  return quality == MODBUS_Q_OK && writable;
}

bool modbusWriteVarSource(uint8_t varIndex, double value) {
  if (varIndex >= MODBUS_VARS_MAX) return false;
  const ParsedSource& source = sourceCache[varIndex];
  if (!source.valid) return false;
  if (strcmp(source.root, "ports") != 0 || strcmp(source.field, "value") != 0 || !source.key[0]) return false;

  const int gpio = atoi(source.key);
  if (gpio < 0 || gpio > 255) return false;
  Port* port = findPortByGpio((uint8_t)gpio);
  if (!port) return false;

  uint16_t raw = value > 65535 ? 65535 : (value < 0 ? 0 : (uint16_t)value);
  setValue((uint8_t)gpio, raw);
  updatePort((uint8_t)gpio, raw);
  return true;
}
