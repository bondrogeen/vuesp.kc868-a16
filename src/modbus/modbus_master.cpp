#include "./modbus_master.h"

#include <Arduino.h>
#include <math.h>
#include <string.h>

#include "../runner/runner.h"
#include "./modbus.h"
#include "./modbus_source.h"
#include "./profile.h"

#define MASTER_IDLE 0
#define MASTER_WAIT 1
#define MASTER_WRITE_WAIT 2

static uint32_t lastPoll[MODBUS_POINTS_MAX];
static uint8_t pointQuality[MODBUS_POINTS_MAX];
static uint8_t rx[256];
static uint16_t rxLen = 0;
static uint32_t lastByte = 0;
static uint32_t waitStart = 0;
static uint16_t expected = 0;
static uint8_t activePoint = 0xFF;
static uint8_t state = MASTER_IDLE;
static uint8_t retriesLeft = 0;
static uint8_t pendingWrite = 0xFF;
static double pendingValue = 0;
static char pendingText[16] = {0};
static uint8_t request[32];
static uint8_t requestLen = 0;

static ModbusDevice* deviceOf(const ModbusPoint& point) {
  if (point.device >= modbusProfile.deviceCount) return nullptr;
  return &modbusProfile.devices[point.device];
}

uint8_t modbusPointRegisters(const ModbusPoint& point) {
  uint8_t minimum = 1;
  if (point.type == MODBUS_TYPE_U32 || point.type == MODBUS_TYPE_S32 || point.type == MODBUS_TYPE_FLOAT32) minimum = 2;
  if (point.type == MODBUS_TYPE_U64 || point.type == MODBUS_TYPE_S64 || point.type == MODBUS_TYPE_FLOAT64) minimum = 4;

  // The editor can override the count for the test frame; the profile must use
  // the same count, otherwise "Test" and polling would read different data.
  uint8_t count = point.registers ? point.registers : minimum;
  if (count < minimum) count = minimum;
  if (count > MODBUS_ENCODE_MAX_REGS) count = MODBUS_ENCODE_MAX_REGS;
  return count;
}

static void setQuality(uint8_t index, uint8_t value) {
  if (index >= MODBUS_POINTS_MAX) return;
  pointQuality[index] = value;
}

void modbusUpdateVarValue(uint8_t varIndex, double value, const char* text, uint8_t quality) {
  if (varIndex >= modbus.varCount) return;

  ModbusVarValue& target = modbus.vars[varIndex];
  const char* next = text ? text : "";
  const bool changed = target.quality != quality || target.value != value || strcmp(target.text, next) != 0;
  target.var = varIndex;
  target.quality = quality;
  target.value = value;
  target.text[0] = '\0';
  if (text) {
    strncpy(target.text, text, sizeof(target.text) - 1);
    target.text[sizeof(target.text) - 1] = '\0';
  }
  modbus.updated = millis() / 1000;
  if (changed) {
    if (quality == MODBUS_Q_OK) scriptRunner.emitEvent("modbus");
    modbusRequestSend();
  }
}

static void setVariable(uint8_t index, double value, const char* text, uint8_t quality) {
  const ModbusPoint& point = modbusProfile.points[index];
  if (point.var >= modbus.varCount) return;
  modbusUpdateVarValue(point.var, value, text, quality);
}

bool modbusSetVarValue(uint8_t varIndex, double value, const char* text) {
  if (varIndex >= modbus.varCount) return false;

  const ModbusVar& def = modbusProfile.vars[varIndex];
  if (def.point < modbusProfile.pointCount) {
    const ModbusPoint& point = modbusProfile.points[def.point];
    if (point.device < modbusProfile.deviceCount) {
      // In slave mode the master queue cannot run; do not leave a stale queued write.
      if (modbus.role == MODBUS_ROLE_MASTER) masterQueueWrite(def.point, value, text);
    }
  } else if (def.source[0]) {
    if (modbusVarSourceWritable(varIndex)) modbusWriteVarSource(varIndex, value);
  }

  modbusUpdateVarValue(varIndex, value, text, MODBUS_Q_OK);
  return true;
}

static uint16_t readRegisterBytes(const uint8_t* data, bool byteLittle) {
  uint16_t value = (uint16_t)((data[0] << 8) | data[1]);
  if (byteLittle) value = (uint16_t)((value >> 8) | (value << 8));
  return value;
}

static void writeRegisterBytes(uint8_t* data, uint16_t value, bool byteLittle) {
  if (byteLittle) value = (uint16_t)((value >> 8) | (value << 8));
  data[0] = (uint8_t)(value >> 8);
  data[1] = (uint8_t)(value & 0xFF);
}

void modbusEncodeRegisters(const ModbusPoint& point, double value, const char* text, uint8_t* out) {
  const uint8_t count = modbusPointRegisters(point);
  const bool byteLittle = (point.flags & MODBUS_FLAG_BYTE_LITTLE) != 0;
  const bool wordLittle = (point.flags & MODBUS_FLAG_WORD_LITTLE) != 0;
  const double scale = point.scale != 0 ? point.scale : 1;
  const double raw = (value - point.offset) / scale;

  uint16_t regs[MODBUS_ENCODE_MAX_REGS] = {0};

  switch (point.type) {
    case MODBUS_TYPE_FLOAT32: {
      union {
        float f;
        uint32_t u;
      } data;
      data.f = (float)raw;
      regs[0] = (uint16_t)(data.u >> 16);
      regs[1] = (uint16_t)(data.u & 0xFFFF);
      break;
    }
    case MODBUS_TYPE_U32:
    case MODBUS_TYPE_S32: {
      const uint32_t v = (uint32_t)raw;
      regs[0] = (uint16_t)(v >> 16);
      regs[1] = (uint16_t)(v & 0xFFFF);
      break;
    }
    case MODBUS_TYPE_FLOAT64: {
      union {
        double d;
        uint64_t u;
      } data;
      data.d = raw;
      regs[0] = (uint16_t)(data.u >> 48);
      regs[1] = (uint16_t)(data.u >> 32);
      regs[2] = (uint16_t)(data.u >> 16);
      regs[3] = (uint16_t)(data.u);
      break;
    }
    case MODBUS_TYPE_U64:
    case MODBUS_TYPE_S64: {
      const uint64_t v = (uint64_t)raw;
      regs[0] = (uint16_t)(v >> 48);
      regs[1] = (uint16_t)(v >> 32);
      regs[2] = (uint16_t)(v >> 16);
      regs[3] = (uint16_t)(v);
      break;
    }
    case MODBUS_TYPE_STRING: {
      const size_t len = text ? strlen(text) : 0;
      for (uint8_t i = 0; i < count; i++) {
        const uint8_t hi = i * 2 < len ? (uint8_t)text[i * 2] : 0;
        const uint8_t lo = i * 2 + 1 < len ? (uint8_t)text[i * 2 + 1] : 0;
        regs[i] = (uint16_t)((hi << 8) | lo);
      }
      break;
    }
    default:
      regs[0] = (uint16_t)(int32_t)raw;
      break;
  }

  if (wordLittle) {
    if (count == 2) {
      const uint16_t swap = regs[0];
      regs[0] = regs[1];
      regs[1] = swap;
    } else if (count == 4) {
      for (uint8_t i = 0; i < 2; i++) {
        const uint16_t swap = regs[i];
        regs[i] = regs[3 - i];
        regs[3 - i] = swap;
      }
    }
  }

  for (uint8_t i = 0; i < count; i++) {
    writeRegisterBytes(out + i * 2, regs[i], byteLittle);
  }
}

double modbusDecodeRegisters(const ModbusPoint& point, const uint8_t* data, uint8_t count, char* text, size_t textSize) {
  const bool byteLittle = (point.flags & MODBUS_FLAG_BYTE_LITTLE) != 0;
  const bool wordLittle = (point.flags & MODBUS_FLAG_WORD_LITTLE) != 0;
  uint16_t regs[MODBUS_ENCODE_MAX_REGS] = {0};

  for (uint8_t i = 0; i < count && i < MODBUS_ENCODE_MAX_REGS; i++) {
    regs[i] = readRegisterBytes(data + i * 2, byteLittle);
  }

  if (wordLittle) {
    if (count == 2) {
      const uint16_t swap = regs[0];
      regs[0] = regs[1];
      regs[1] = swap;
    } else if (count == 4) {
      for (uint8_t i = 0; i < 2; i++) {
        const uint16_t swap = regs[i];
        regs[i] = regs[3 - i];
        regs[3 - i] = swap;
      }
    }
  }

  double raw = 0;

  switch (point.type) {
    case MODBUS_TYPE_S16:
      raw = (int16_t)regs[0];
      break;
    case MODBUS_TYPE_U32:
      raw = (double)(((uint32_t)regs[0] << 16) | regs[1]);
      break;
    case MODBUS_TYPE_S32:
      raw = (double)(int32_t)(((uint32_t)regs[0] << 16) | regs[1]);
      break;
    case MODBUS_TYPE_FLOAT32: {
      union {
        float f;
        uint32_t u;
      } value;
      value.u = ((uint32_t)regs[0] << 16) | regs[1];
      raw = value.f;
      break;
    }
    case MODBUS_TYPE_U64:
      raw = (double)(((uint64_t)regs[0] << 48) | ((uint64_t)regs[1] << 32) | ((uint64_t)regs[2] << 16) | regs[3]);
      break;
    case MODBUS_TYPE_S64:
      raw = (double)(int64_t)(((uint64_t)regs[0] << 48) | ((uint64_t)regs[1] << 32) | ((uint64_t)regs[2] << 16) | regs[3]);
      break;
    case MODBUS_TYPE_FLOAT64: {
      union {
        double d;
        uint64_t u;
      } value;
      value.u = ((uint64_t)regs[0] << 48) | ((uint64_t)regs[1] << 32) | ((uint64_t)regs[2] << 16) | regs[3];
      raw = value.d;
      break;
    }
    case MODBUS_TYPE_STRING: {
      if (!text || textSize == 0) return 0;
      size_t out = 0;
      for (uint8_t i = 0; i < count && out + 1 < textSize; i++) {
        const uint8_t hi = (uint8_t)(regs[i] >> 8);
        const uint8_t lo = (uint8_t)(regs[i] & 0xFF);
        if (hi) text[out++] = (char)hi;
        if (lo && out + 1 < textSize) text[out++] = (char)lo;
      }
      text[out] = '\0';
      return 0;
    }
    case MODBUS_TYPE_BCD: {
      const uint16_t v = regs[0];
      raw = ((v >> 12) & 0xF) * 1000 + ((v >> 8) & 0xF) * 100 + ((v >> 4) & 0xF) * 10 + (v & 0xF);
      break;
    }
    default:
      raw = regs[0];
      break;
  }

  const double scale = point.scale != 0 ? point.scale : 1;
  return raw * scale + point.offset;
}

static bool buildRead(const ModbusPoint& point, uint8_t* out) {
  const ModbusDevice* device = deviceOf(point);
  if (!device) return false;

  const uint8_t count = modbusPointRegisters(point);
  out[0] = device->address;
  out[1] = point.fc;
  out[2] = (uint8_t)(point.address >> 8);
  out[3] = (uint8_t)(point.address & 0xFF);
  out[4] = 0;
  out[5] = count;
  modbusAppendCrc(out, 6);

  requestLen = 8;
  expected = (point.fc == 1 || point.fc == 2) ? (uint16_t)(5 + ((count + 7) / 8)) : (uint16_t)(5 + count * 2);
  return true;
}

static bool buildWrite(const ModbusPoint& point, uint8_t* out) {
  const ModbusDevice* device = deviceOf(point);
  if (!device) return false;

  if (point.fc == 1) {
    out[0] = device->address;
    out[1] = 0x05;
    out[2] = (uint8_t)(point.address >> 8);
    out[3] = (uint8_t)(point.address & 0xFF);
    out[4] = pendingValue != 0 ? 0xFF : 0x00;
    out[5] = 0x00;
    modbusAppendCrc(out, 6);
    requestLen = 8;
    expected = 8;
    return true;
  }

  if (point.fc != 3) return false;

  const uint8_t count = modbusPointRegisters(point);
  if (count == 1) {
    uint8_t data[2] = {0, 0};
    modbusEncodeRegisters(point, pendingValue, pendingText, data);
    out[0] = device->address;
    out[1] = 0x06;
    out[2] = (uint8_t)(point.address >> 8);
    out[3] = (uint8_t)(point.address & 0xFF);
    out[4] = data[0];
    out[5] = data[1];
    modbusAppendCrc(out, 6);
    requestLen = 8;
    expected = 8;
    return true;
  }

  out[0] = device->address;
  out[1] = 0x10;
  out[2] = (uint8_t)(point.address >> 8);
  out[3] = (uint8_t)(point.address & 0xFF);
  out[4] = 0;
  out[5] = count;
  out[6] = (uint8_t)(count * 2);
  modbusEncodeRegisters(point, pendingValue, pendingText, out + 7);
  modbusAppendCrc(out, (size_t)7 + count * 2);
  requestLen = (uint8_t)(9 + count * 2);
  expected = 8;
  return true;
}

static void sendActive() {
  rxLen = 0;
  lastByte = 0;
  waitStart = millis();
  modbus.lastFunction = request[1];
  modbus.lastAddress = (uint16_t)((request[2] << 8) | request[3]);
  modbus.requests++;
  modbusTransmit(request, requestLen);
}

static void finishPoint(uint8_t quality, uint8_t error) {
  if (activePoint < MODBUS_POINTS_MAX) {
    lastPoll[activePoint] = millis();
    setQuality(activePoint, quality);
    if (quality != MODBUS_Q_OK) {
      setVariable(activePoint, 0, nullptr, quality);
      modbus.errors++;
      modbus.lastError = error;
    }
  }
  activePoint = 0xFF;
  state = MASTER_IDLE;
}

static void failActive(uint8_t error) {
  const ModbusPoint& point = modbusProfile.points[activePoint];
  const ModbusDevice* device = deviceOf(point);

  if (device && retriesLeft > 0) {
    retriesLeft--;
    sendActive();
    return;
  }

  if (state == MASTER_WRITE_WAIT) {
    pendingWrite = 0xFF;
    pendingText[0] = '\0';
  }
  finishPoint(MODBUS_Q_ERROR, error);
}

static void processRead() {
  const ModbusPoint& point = modbusProfile.points[activePoint];
  const uint8_t count = modbusPointRegisters(point);

  if (point.fc == 1 || point.fc == 2) {
    const uint8_t bits = rx[3];
    const bool on = (bits & 0x01) != 0;
    setVariable(activePoint, on ? 1 : 0, nullptr, MODBUS_Q_OK);
    finishPoint(MODBUS_Q_OK, MODBUS_ERR_NONE);
    return;
  }

  if (rx[2] < count * 2) {
    failActive(MODBUS_ERR_FRAME);
    return;
  }

  char text[sizeof(((ModbusVarValue*)nullptr)->text)] = {0};
  const double value = modbusDecodeRegisters(point, rx + 3, count, text, sizeof(text));
  setVariable(activePoint, value, point.type == MODBUS_TYPE_STRING ? text : nullptr, MODBUS_Q_OK);
  finishPoint(MODBUS_Q_OK, MODBUS_ERR_NONE);
}

static void processResponse() {
  if (rxLen < 5) {
    failActive(MODBUS_ERR_FRAME);
    return;
  }

  if (modbusCrc16(rx, rxLen - 2) != (uint16_t)((rx[rxLen - 1] << 8) | rx[rxLen - 2])) {
    failActive(MODBUS_ERR_CRC);
    return;
  }

  const ModbusPoint& point = modbusProfile.points[activePoint];
  const ModbusDevice* device = deviceOf(point);
  if (!device || rx[0] != device->address) {
    failActive(MODBUS_ERR_ADDRESS);
    return;
  }

  if (rx[1] & 0x80) {
    failActive(rx[2] == 2 ? MODBUS_ERR_RANGE : MODBUS_ERR_FUNCTION);
    return;
  }

  if (state == MASTER_WRITE_WAIT) {
    setVariable(activePoint, pendingValue, pendingText, MODBUS_Q_OK);
    pendingWrite = 0xFF;
    pendingText[0] = '\0';
    finishPoint(MODBUS_Q_OK, MODBUS_ERR_NONE);
    return;
  }

  if (rx[1] != point.fc || rxLen < expected) {
    failActive(MODBUS_ERR_FRAME);
    return;
  }

  processRead();
}

static bool startNextRead(uint32_t now) {
  for (uint8_t p = 0; p < modbusProfile.pointCount; p++) {
    const ModbusPoint& point = modbusProfile.points[p];
    const ModbusDevice* device = deviceOf(point);
    if (!device || !device->enabled) continue;
    if (point.var >= modbus.varCount) continue;
    if (now - lastPoll[p] < device->interval) continue;

    if (!buildRead(point, request)) continue;

    activePoint = p;
    retriesLeft = device->retries;
    state = MASTER_WAIT;
    sendActive();
    return true;
  }
  return false;
}

static bool startWrite() {
  if (pendingWrite >= modbusProfile.pointCount) {
    pendingWrite = 0xFF;
    return false;
  }

  const ModbusPoint& point = modbusProfile.points[pendingWrite];
  if (!(point.flags & MODBUS_FLAG_WRITABLE) || !buildWrite(point, request)) {
    pendingWrite = 0xFF;
    pendingText[0] = '\0';
    return false;
  }

  activePoint = pendingWrite;
  retriesLeft = deviceOf(point)->retries;
  state = MASTER_WRITE_WAIT;
  sendActive();
  return true;
}

void resetMaster() {
  memset(lastPoll, 0, sizeof(lastPoll));
  memset(pointQuality, MODBUS_Q_STALE, sizeof(pointQuality));
  state = MASTER_IDLE;
  activePoint = 0xFF;
  pendingWrite = 0xFF;
  pendingText[0] = '\0';
  rxLen = 0;
  modbus.varCount = modbusProfile.varCount;
  modbusSourceRebuild();

  for (uint8_t i = 0; i < modbus.varCount; i++) {
    modbus.vars[i].var = i;
    modbus.vars[i].quality = MODBUS_Q_STALE;
    modbus.vars[i].value = 0;
    modbus.vars[i].text[0] = '\0';
  }
}

void setupMaster() { resetMaster(); }

bool masterBusy() { return state != MASTER_IDLE || pendingWrite != 0xFF; }

void masterAbort() {
  // Raw frame test takes the bus: drop the in-flight transaction, keep the
  // pending write so it is retried after the test.
  state = MASTER_IDLE;
  activePoint = 0xFF;
  rxLen = 0;
}

bool masterQueueWrite(uint8_t pointIndex, double value, const char* text) {
  if (pointIndex >= modbusProfile.pointCount) return false;
  if (!(modbusProfile.points[pointIndex].flags & MODBUS_FLAG_WRITABLE)) return false;

  pendingWrite = pointIndex;
  pendingValue = value;
  pendingText[0] = '\0';
  if (text) {
    strncpy(pendingText, text, sizeof(pendingText) - 1);
    pendingText[sizeof(pendingText) - 1] = '\0';
  }
  return true;
}

void loopMaster(uint32_t now) {
  while (Serial1.available() > 0) {
    const int value = Serial1.read();
    if (value < 0) break;
    if (rxLen < sizeof(rx)) rx[rxLen++] = (uint8_t)value;
    lastByte = now;
  }

  if (state == MASTER_WAIT || state == MASTER_WRITE_WAIT) {
    const bool exception = rxLen >= 5 && (rx[1] & 0x80);
    const bool complete = exception || rxLen >= expected;
    if (complete) {
      processResponse();
      return;
    }

    const ModbusPoint& point = modbusProfile.points[activePoint];
    const ModbusDevice* device = deviceOf(point);
    if (!device || now - waitStart > device->timeout) {
      failActive(MODBUS_ERR_FRAME);
    }
    return;
  }

  if (pendingWrite != 0xFF) {
    if (startWrite()) return;
  }

  startNextRead(now);
}

static int findVar(const char* id, size_t len) {
  if (!id) return -1;
  for (uint8_t i = 0; i < modbusProfile.varCount; i++) {
    if (strlen(modbusProfile.vars[i].id) == len && strncmp(modbusProfile.vars[i].id, id, len) == 0) return i;
  }
  return -1;
}

static int findDevice(const char* id, size_t len) {
  if (!id) return -1;
  for (uint8_t i = 0; i < modbusProfile.deviceCount; i++) {
    if (strlen(modbusProfile.devices[i].id) == len && strncmp(modbusProfile.devices[i].id, id, len) == 0) return i;
  }
  return -1;
}

static int findPoint(uint8_t device, const char* id, size_t len) {
  if (!id) return -1;
  for (uint8_t i = 0; i < modbusProfile.pointCount; i++) {
    if (modbusProfile.points[i].device != device) continue;
    if (strlen(modbusProfile.points[i].id) == len && strncmp(modbusProfile.points[i].id, id, len) == 0) return i;
  }
  return -1;
}

static double paramNumber(const Value& value) {
  switch (value.type) {
    case VAL_INT:
      return value.intVal;
    case VAL_UINT:
      return value.uintVal;
    case VAL_FLOAT:
      return value.floatVal;
    default:
      return 0;
  }
}

static bool varHandler(uint8_t count, const Value* params, Value& result, void*) {
  if (count < 1 || count > 2 || params[0].type != VAL_STRING) return false;

  const int index = findVar(params[0].stringVal.data, params[0].stringVal.len);
  if (index < 0) return false;

  const ModbusVar& var = modbusProfile.vars[index];

  if (count == 1) {
    const ModbusVarValue& value = modbus.vars[index];
    if (var.type == MODBUS_TYPE_STRING) {
      result.type = VAL_STRING;
      result.stringVal.data = value.text;
      result.stringVal.len = (uint8_t)strlen(value.text);
    } else {
      result.type = VAL_FLOAT;
      result.floatVal = (float)value.value;
    }
    return true;
  }

  if (var.type == MODBUS_TYPE_STRING && params[1].type == VAL_STRING) {
    modbusSetVarValue((uint8_t)index, 0, params[1].stringVal.data);
  } else {
    modbusSetVarValue((uint8_t)index, paramNumber(params[1]), nullptr);
  }

  result.type = VAL_INT;
  result.intVal = 1;
  return true;
}

static bool paramAddress(const Value& value, uint16_t& address) {
  if (value.type != VAL_INT && value.type != VAL_UINT && value.type != VAL_FLOAT) return false;

  const double number = paramNumber(value);
  if (number < 0 || number > 65535) return false;

  const uint16_t candidate = (uint16_t)number;
  if ((double)candidate != number) return false;

  address = candidate;
  return true;
}

static int findDeviceParam(const Value& param) {
  if (param.type == VAL_STRING) return findDevice(param.stringVal.data, param.stringVal.len);

  uint16_t address = 0;
  if (!paramAddress(param, address)) return -1;
  for (uint8_t i = 0; i < modbusProfile.deviceCount; i++) {
    if (modbusProfile.devices[i].address == address) return i;
  }
  return -1;
}

static int findPointParam(uint8_t device, const Value& param, bool writable) {
  if (param.type == VAL_STRING) return findPoint(device, param.stringVal.data, param.stringVal.len);

  uint16_t address = 0;
  if (!paramAddress(param, address)) return -1;

  int fallback = -1;
  for (uint8_t i = 0; i < modbusProfile.pointCount; i++) {
    const ModbusPoint& point = modbusProfile.points[i];
    if (point.device != device || point.address != address) continue;
    if (!writable || (point.flags & MODBUS_FLAG_WRITABLE)) return i;
    if (fallback < 0) fallback = i;
  }
  return fallback;
}

static bool modbusHandler(uint8_t count, const Value* params, Value& result, void*) {
  if (count < 2 || count > 3) return false;

  const int device = findDeviceParam(params[0]);
  if (device < 0) return false;
  const int point = findPointParam((uint8_t)device, params[1], count == 3);
  if (point < 0) return false;

  if (count == 2) {
    const ModbusPoint& p = modbusProfile.points[point];
    if (p.var >= modbus.varCount) return false;
    result.type = VAL_FLOAT;
    result.floatVal = (float)modbus.vars[p.var].value;
    return true;
  }

  const char* text = params[2].type == VAL_STRING ? params[2].stringVal.data : nullptr;
  const bool ok = masterQueueWrite((uint8_t)point, paramNumber(params[2]), text);
  result.type = VAL_INT;
  result.intVal = ok ? 1 : 0;
  return true;
}

void registerMasterFunctions() {
  scriptRunner.registerFunction("var", varHandler);
  scriptRunner.registerFunction("modbus", modbusHandler);
}
