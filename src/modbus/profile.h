#ifndef MODBUS_PROFILE_H
#define MODBUS_PROFILE_H

#include "../const/protocol.h"

bool loadModbusProfile(ModbusProfile& profile);
bool saveModbusProfile(const ModbusProfile& profile);
void defaultModbusProfile(ModbusProfile& profile);
bool validateModbusProfile(const ModbusProfile& profile);

#endif
