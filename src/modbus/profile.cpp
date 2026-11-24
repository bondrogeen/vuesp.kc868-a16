#include "./profile.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <string.h>

#include "./modbus.h"

#define MODBUS_PROFILE_PATH "/tmp/modbus.bin"
#define MODBUS_PROFILE_TMP "/tmp/modbus.tmp"
#define MODBUS_PROFILE_MAGIC 0x504D4456  // "VDMP" in little-endian
#define MODBUS_PROFILE_VERSION 2

struct ModbusProfileHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t headerSize;
  uint32_t payloadSize;
  uint32_t crc;
};

static uint32_t crc32(const uint8_t* data, size_t len) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : (crc >> 1);
    }
  }
  return ~crc;
}

static bool validText(const char* text, size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (text[i] == '\0') return true;
  }
  return false;
}

void defaultModbusProfile(ModbusProfile& profile) {
  memset(&profile, 0, sizeof(profile));
  profile.key = KEY_MODBUSPROFILE;
  profile.version = MODBUS_PROFILE_VERSION;
}

bool validateModbusProfile(const ModbusProfile& profile) {
  if (profile.deviceCount > MODBUS_DEVICES_MAX) return false;
  if (profile.pointCount > MODBUS_POINTS_MAX) return false;
  if (profile.varCount > MODBUS_VARS_MAX) return false;

  for (uint8_t i = 0; i < profile.deviceCount; i++) {
    const ModbusDevice& device = profile.devices[i];
    if (device.address < 1 || device.address > 247) return false;
    if (!validText(device.id, sizeof(device.id))) return false;
    if (!validText(device.name, sizeof(device.name))) return false;
    if (device.interval < 100) return false;
    if (device.timeout < 50) return false;
    if (device.retries > 5) return false;
  }

  for (uint8_t i = 0; i < profile.pointCount; i++) {
    const ModbusPoint& point = profile.points[i];
    if (point.type > MODBUS_TYPE_BCD) return false;
    if (!validText(point.id, sizeof(point.id))) return false;
    if (!validText(point.name, sizeof(point.name))) return false;
    if (!validText(point.unit, sizeof(point.unit))) return false;
    if (point.var != 0xFF && point.var >= profile.varCount) return false;

    if (point.device == MODBUS_DEVICE_SLAVE) {
      if (point.fc != 3 && point.fc != 4) return false;
      if (point.type != MODBUS_TYPE_U16 && point.type != MODBUS_TYPE_S16 &&
          point.type != MODBUS_TYPE_U32 && point.type != MODBUS_TYPE_S32 &&
          point.type != MODBUS_TYPE_FLOAT32) return false;
      if (point.address < MODBUS_SLAVE_ADDR_START) return false;
      if (point.var == 0xFF) return false;

      uint8_t minWidth = 1;
      if (point.type == MODBUS_TYPE_U32 || point.type == MODBUS_TYPE_S32 ||
          point.type == MODBUS_TYPE_FLOAT32) minWidth = 2;
      const uint8_t width = point.registers ? point.registers : minWidth;
      if (width < minWidth || width > MODBUS_ENCODE_MAX_REGS) return false;
      if ((uint32_t)point.address + width > MODBUS_SLAVE_ADDR_START + MODBUS_SLAVE_REG_COUNT) return false;
    } else {
      if (point.device >= profile.deviceCount) return false;
      if (point.fc < 1 || point.fc > 4) return false;
    }
  }

  for (uint8_t i = 0; i < profile.pointCount; i++) {
    const ModbusPoint& a = profile.points[i];
    if (a.device != MODBUS_DEVICE_SLAVE) continue;
    uint8_t minA = 1;
    if (a.type == MODBUS_TYPE_U32 || a.type == MODBUS_TYPE_S32 ||
        a.type == MODBUS_TYPE_FLOAT32) minA = 2;
    const uint8_t widthA = a.registers ? a.registers : minA;
    for (uint8_t j = i + 1; j < profile.pointCount; j++) {
      const ModbusPoint& b = profile.points[j];
      if (b.device != MODBUS_DEVICE_SLAVE) continue;
      uint8_t minB = 1;
      if (b.type == MODBUS_TYPE_U32 || b.type == MODBUS_TYPE_S32 ||
          b.type == MODBUS_TYPE_FLOAT32) minB = 2;
      const uint8_t widthB = b.registers ? b.registers : minB;
      if (a.address < (uint32_t)b.address + widthB && b.address < (uint32_t)a.address + widthA) return false;
    }
  }

  uint8_t sourceCount[MODBUS_VARS_MAX] = {0};
  for (uint8_t i = 0; i < profile.pointCount; i++) {
    const ModbusPoint& point = profile.points[i];
    if (point.var == 0xFF || point.device == MODBUS_DEVICE_SLAVE) continue;
    if (sourceCount[point.var]++ > 0) return false;
    if (profile.vars[point.var].source[0]) return false;
  }

  for (uint8_t i = 0; i < profile.varCount; i++) {
    const ModbusVar& var = profile.vars[i];
    if (var.type > MODBUS_TYPE_BCD) return false;
    if (!validText(var.id, sizeof(var.id))) return false;
    if (!validText(var.name, sizeof(var.name))) return false;
    if (!validText(var.unit, sizeof(var.unit))) return false;
    if (!validText(var.source, sizeof(var.source))) return false;
    if (var.point != 0xFF && var.point >= profile.pointCount) return false;
    if (var.source[0] && var.point != 0xFF) return false;
  }

  return true;
}

static bool readProfileFile(const char* path, ModbusProfile& profile) {
  File file = LittleFS.open(path, "r");
  if (!file) return false;

  ModbusProfileHeader header;
  if (file.read((uint8_t*)&header, sizeof(header)) != sizeof(header)) {
    file.close();
    return false;
  }

  if (header.magic != MODBUS_PROFILE_MAGIC ||
      header.version != MODBUS_PROFILE_VERSION ||
      header.headerSize != sizeof(header) ||
      header.payloadSize != sizeof(ModbusProfile)) {
    file.close();
    return false;
  }

  if (file.read((uint8_t*)&profile, sizeof(profile)) != sizeof(profile)) {
    file.close();
    return false;
  }
  file.close();

  if (crc32((const uint8_t*)&profile, sizeof(profile)) != header.crc) return false;
  if (!validateModbusProfile(profile)) return false;

  profile.key = KEY_MODBUSPROFILE;
  profile.command = 0;
  return true;
}

bool loadModbusProfile(ModbusProfile& profile) {
  return readProfileFile(MODBUS_PROFILE_PATH, profile);
}

bool saveModbusProfile(const ModbusProfile& profile) {
  if (!validateModbusProfile(profile)) return false;

  ModbusProfileHeader header = {
      MODBUS_PROFILE_MAGIC,
      MODBUS_PROFILE_VERSION,
      (uint16_t)sizeof(header),
      (uint32_t)sizeof(profile),
      crc32((const uint8_t*)&profile, sizeof(profile)),
  };

  LittleFS.remove(MODBUS_PROFILE_TMP);
  File file = LittleFS.open(MODBUS_PROFILE_TMP, "w");
  if (!file) return false;

  const size_t headerWritten = file.write((const uint8_t*)&header, sizeof(header));
  const size_t payloadWritten = file.write((const uint8_t*)&profile, sizeof(profile));
  file.close();

  if (headerWritten != sizeof(header) || payloadWritten != sizeof(profile)) {
    LittleFS.remove(MODBUS_PROFILE_TMP);
    return false;
  }

  LittleFS.remove(MODBUS_PROFILE_PATH);
  if (!LittleFS.rename(MODBUS_PROFILE_TMP, MODBUS_PROFILE_PATH)) {
    LittleFS.remove(MODBUS_PROFILE_TMP);
    return false;
  }

  return true;
}
