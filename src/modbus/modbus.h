#ifndef MODBUS_H
#define MODBUS_H

#include <stddef.h>
#include <stdint.h>

#include "../const/protocol.h"

#define MODBUS_TX_PIN RS485_TX_PIN
#define MODBUS_RX_PIN RS485_RX_PIN
#define MODBUS_DE_PIN -1

#define MODBUS_ROLE_OFF 0
#define MODBUS_ROLE_SLAVE 1
#define MODBUS_ROLE_MASTER 2

#define MODBUS_DEFAULT_ADDRESS 1
#define MODBUS_DEFAULT_BAUD 9600
#define MODBUS_DEFAULT_ROLE MODBUS_ROLE_SLAVE

#define MODBUS_ERR_NONE 0
#define MODBUS_ERR_CRC 1
#define MODBUS_ERR_ADDRESS 2
#define MODBUS_ERR_FUNCTION 3
#define MODBUS_ERR_RANGE 4
#define MODBUS_ERR_FRAME 5

#define MODBUS_DATA_BITS 8

#define MODBUS_PARITY_NONE 0
#define MODBUS_PARITY_EVEN 1
#define MODBUS_PARITY_ODD 2

#define MODBUS_TYPE_U16 0
#define MODBUS_TYPE_S16 1
#define MODBUS_TYPE_U32 2
#define MODBUS_TYPE_S32 3
#define MODBUS_TYPE_U64 4
#define MODBUS_TYPE_S64 5
#define MODBUS_TYPE_FLOAT32 6
#define MODBUS_TYPE_FLOAT64 7
#define MODBUS_TYPE_BOOL 8
#define MODBUS_TYPE_STRING 9
#define MODBUS_TYPE_BIT 10
#define MODBUS_TYPE_ENUM 11
#define MODBUS_TYPE_BCD 12

#define MODBUS_FLAG_WORD_LITTLE (1 << 0)
#define MODBUS_FLAG_BYTE_LITTLE (1 << 1)
#define MODBUS_FLAG_WRITABLE (1 << 2)

#define MODBUS_Q_OK 0
#define MODBUS_Q_STALE 1
#define MODBUS_Q_ERROR 2
#define MODBUS_Q_DISABLED 3

// ModbusPoint.device marker: normal points use a device index (0..devices-1).
#define MODBUS_DEVICE_SLAVE 0xFF  // slave export point: serves a variable

// Dynamic slave register bank: 100..131. Unmapped addresses read as 0.
#define MODBUS_SLAVE_ADDR_START 100
#define MODBUS_SLAVE_REG_COUNT 32

#define MODBUS_ENCODE_MAX_REGS 16

#include "./profile.h"

extern Modbus modbus;
extern ModbusProfile modbusProfile;

uint16_t modbusCrc16(const uint8_t* data, size_t len);
void modbusAppendCrc(uint8_t* data, size_t len);
bool modbusTransmit(const uint8_t* data, size_t len);
void modbusRequestSend();

void setupFirstModbus();
void setupModbus();
void loopModbus(uint32_t now);
void onWsEventModbus(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task);
void onWsEventModbusProfile(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task);

#endif
