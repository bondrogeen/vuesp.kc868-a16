#ifndef FILES_H
#define FILES_H

#include <LittleFS.h>
#include <stdint.h>

#define CONFIG_LITTLEFS_SPIFFS_COMPAT 1
#define FORMAT_LITTLEFS_IF_FAILED true

#include "../const/index.h"

#define CONFIG_BLOCK_MAGIC 0x56534550UL  // "VSEP"
#define CONFIG_BLOCK_VERSION 1
#define MAX_CONFIG_BLOCK_SIZE 512

struct ConfigBlockHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t size;
  uint32_t crc;
};

void initFS();
void getInfo(Info* infoFS);
uint8_t readFile(const char* path, uint8_t* buf, size_t size);
bool writeFile(const char* path, const uint8_t* buf, size_t size);
bool createDir(const char* path);
void getLoadDef(const char* path, uint8_t* buf, size_t size);
bool loadConfigBlock(const char* path, uint8_t* buf, size_t size);
bool saveConfigBlock(const char* path, const uint8_t* buf, size_t size);
bool loadScriptFromFS(uint8_t id, char* buffer, uint16_t& len);

#endif