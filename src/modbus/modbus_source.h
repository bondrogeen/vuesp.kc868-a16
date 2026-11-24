#ifndef MODBUS_SOURCE_H
#define MODBUS_SOURCE_H

#include <stddef.h>
#include <stdint.h>

void modbusSourceRebuild();
bool modbusReadVarSource(uint8_t varIndex, double& value, char* text, size_t textSize, uint8_t& quality);
bool modbusWriteVarSource(uint8_t varIndex, double value);
bool modbusVarSourceWritable(uint8_t varIndex);

#endif
