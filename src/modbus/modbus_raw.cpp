#include "./modbus_raw.h"

#include <Arduino.h>
#include <string.h>

#include "../tasks/tasks.h"
#include "../webserver/webserver.h"
#include "./modbus.h"
#include "./modbus_master.h"
#include "./profile.h"

static ModbusRaw incoming;
static ModbusRaw state;
static uint8_t request[80];
static uint8_t requestLen = 0;
static uint8_t response[64];
static uint8_t responseLen = 0;
static uint32_t sentAt = 0;
static uint32_t lastByte = 0;
static uint32_t startAt = 0;
static bool waiting = false;
static bool pendingStart = false;

static void sendRaw() {
  if (wsClientCount() > 0) wsSendAll((uint8_t*)&state, sizeof(state));
}

static uint8_t buildRequest() {
  if (incoming.requestLen > 0) {
    if (incoming.requestLen > 64) return MODBUS_RAW_INVALID;

    memcpy(request, incoming.request, incoming.requestLen);
    requestLen = incoming.requestLen;

    if (incoming.autoCrc) {
      if (requestLen > sizeof(request) - 2) return MODBUS_RAW_INVALID;
      modbusAppendCrc(request, requestLen);
      requestLen += 2;
    }
    return MODBUS_RAW_OK;
  }

  const uint8_t slave = incoming.slave;
  const uint8_t function = incoming.function;
  if (slave < 1 || slave > 247) return MODBUS_RAW_INVALID;

  if (function < 1 || function > 4) return MODBUS_RAW_INVALID;
  if (incoming.length < 1 || incoming.length > 16) return MODBUS_RAW_INVALID;

  request[0] = slave;
  request[1] = function;
  request[2] = (uint8_t)(incoming.address >> 8);
  request[3] = (uint8_t)(incoming.address & 0xFF);
  request[4] = 0;
  request[5] = incoming.length;
  modbusAppendCrc(request, 6);
  requestLen = 8;

  return MODBUS_RAW_OK;
}

static void startRaw(uint8_t status) {
  const bool transmit = status == MODBUS_RAW_OK;

  memset(&state, 0, sizeof(state));
  state.key = KEY_MODBUSRAW;
  state.slave = incoming.slave;
  state.function = incoming.function;
  state.address = incoming.address;
  state.length = incoming.length;
  state.autoCrc = incoming.autoCrc;
  state.status = transmit ? MODBUS_RAW_WAIT : status;

  if (transmit) {
    state.requestLen = requestLen;
    memcpy(state.request, request, requestLen);
    state.elapsed = 0;

    responseLen = 0;
    lastByte = millis();
    sentAt = millis();
    startAt = millis() + 100;
    pendingStart = true;
    waiting = true;
  }

  sendRaw();
}

static void finishRaw(uint8_t status) {
  waiting = false;
  pendingStart = false;
  state.status = status;
  state.responseLen = responseLen;
  memcpy(state.response, response, responseLen);
  state.elapsed = millis() - sentAt;

  modbus.requests++;
  if (status != MODBUS_RAW_OK && status != MODBUS_RAW_EXCEPTION) modbus.errors++;

  sendRaw();
}

static void handleCommand() {
  if (!tasks[KEY_MODBUSRAW]) return;
  tasks[KEY_MODBUSRAW] = 0;

  if (incoming.command == CMD_GET) {
    state.key = KEY_MODBUSRAW;
    sendRaw();
    return;
  }

  if (incoming.command != CMD_SET && incoming.command != CMD_SAVE) return;

  if (modbus.role != MODBUS_ROLE_MASTER) {
    memcpy(&state, &incoming, sizeof(state));
    state.key = KEY_MODBUSRAW;
    state.status = MODBUS_RAW_UNAVAILABLE;
    state.elapsed = 0;
    sendRaw();
    return;
  }

  if (rawBusy()) {
    memcpy(&state, &incoming, sizeof(state));
    state.key = KEY_MODBUSRAW;
    state.status = MODBUS_RAW_BUSY;
    sendRaw();
    return;
  }

  const uint8_t built = buildRequest();
  if (built != MODBUS_RAW_OK) {
    memcpy(&state, &incoming, sizeof(state));
    state.key = KEY_MODBUSRAW;
    state.status = built;
    state.elapsed = 0;
    sendRaw();
    return;
  }

  masterAbort();
  while (Serial1.available() > 0) Serial1.read();
  startRaw(MODBUS_RAW_OK);
}

static uint16_t expectedResponse() {
  if (responseLen < 3) return 0;
  if (response[1] & 0x80) return 5;

  switch (response[1]) {
    case 1:
    case 2:
    case 3:
    case 4:
      return (uint16_t)(5 + response[2]);
    case 5:
    case 6:
    case 15:
    case 16:
      return 8;
    default:
      return 0;
  }
}

static void processResponse() {
  if (responseLen < 4) return finishRaw(MODBUS_RAW_INVALID);

  const uint16_t crc = (uint16_t)((response[responseLen - 1] << 8) | response[responseLen - 2]);
  if (modbusCrc16(response, responseLen - 2) != crc) return finishRaw(MODBUS_RAW_CRC);
  if (response[1] & 0x80) return finishRaw(MODBUS_RAW_EXCEPTION);

  finishRaw(MODBUS_RAW_OK);
}

void setupRaw() {
  memset(&incoming, 0, sizeof(incoming));
  memset(&state, 0, sizeof(state));
  state.key = KEY_MODBUSRAW;
  waiting = false;
  pendingStart = false;
}

bool rawBusy() { return waiting || pendingStart; }

void loopRaw(uint32_t now) {
  handleCommand();

  if (pendingStart) {
    // Let the response of the aborted master request finish and the bus go
    // quiet, otherwise it collides with the test frame.
    while (Serial1.available() > 0) {
      Serial1.read();
      lastByte = now;
    }

    if ((int32_t)(now - startAt) >= 0 && (uint32_t)(now - lastByte) >= 20) {
      while (Serial1.available() > 0) Serial1.read();
      pendingStart = false;
      sentAt = now;
      modbusTransmit(request, requestLen);
    }
    return;
  }

  if (!waiting) return;

  while (responseLen < sizeof(response) && Serial1.available() > 0) {
    const int value = Serial1.read();
    if (value < 0) break;
    response[responseLen++] = (uint8_t)value;
    lastByte = now;
  }

  const uint16_t expected = expectedResponse();
  const bool complete = responseLen > 0 && ((expected > 0 && responseLen >= expected) || (uint32_t)(now - lastByte) >= MODBUS_RAW_GAP_MS);

  if (complete) {
    processResponse();
    return;
  }

  if ((uint32_t)(now - sentAt) >= MODBUS_RAW_TIMEOUT_MS) finishRaw(MODBUS_RAW_TIMEOUT);
}

void onWsEventModbusRaw(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  if (!arg || !data || task != KEY_MODBUSRAW) return;

  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (info->len != sizeof(ModbusRaw)) return;

  memcpy(&incoming, data, sizeof(incoming));
  incoming.key = KEY_MODBUSRAW;
  tasks[KEY_MODBUSRAW] = 1;
}
