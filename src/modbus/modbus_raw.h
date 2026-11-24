#ifndef MODBUS_RAW_H
#define MODBUS_RAW_H

#include <stddef.h>
#include <stdint.h>

#include "../const/protocol.h"

// Raw Modbus RTU frame test. Runs only in master mode and is mutually
// exclusive with the polling engine: while a frame is in flight the master
// scheduler is paused.

#define MODBUS_RAW_GAP_MS 8
#define MODBUS_RAW_TIMEOUT_MS 1000

#define MODBUS_RAW_IDLE 0
#define MODBUS_RAW_WAIT 1
#define MODBUS_RAW_OK 2
#define MODBUS_RAW_TIMEOUT 3
#define MODBUS_RAW_CRC 4
#define MODBUS_RAW_EXCEPTION 5
#define MODBUS_RAW_UNAVAILABLE 6
#define MODBUS_RAW_INVALID 7
#define MODBUS_RAW_BUSY 8

void setupRaw();
void loopRaw(uint32_t now);
bool rawBusy();
void onWsEventModbusRaw(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task);

#endif
