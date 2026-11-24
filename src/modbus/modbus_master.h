#ifndef MODBUS_MASTER_H
#define MODBUS_MASTER_H

#include <stddef.h>
#include <stdint.h>

#include "../const/protocol.h"

uint8_t modbusPointRegisters(const ModbusPoint& point);
void modbusEncodeRegisters(const ModbusPoint& point, double value, const char* text, uint8_t* out);
double modbusDecodeRegisters(const ModbusPoint& point, const uint8_t* data, uint8_t count, char* text, size_t textSize);
void modbusUpdateVarValue(uint8_t varIndex, double value, const char* text, uint8_t quality);
bool modbusSetVarValue(uint8_t varIndex, double value, const char* text);

void setupMaster();
void resetMaster();
void loopMaster(uint32_t now);
bool masterQueueWrite(uint8_t pointIndex, double value, const char* text);
bool masterBusy();
void masterAbort();
void registerMasterFunctions();

#endif
