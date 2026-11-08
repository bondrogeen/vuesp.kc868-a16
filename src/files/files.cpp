#include "./files.h"

static_assert(sizeof(ConfigBlockHeader) == 12, "Unexpected ConfigBlockHeader layout");

static uint32_t crc32(const uint8_t* data, size_t size) {
  uint32_t crc = 0xFFFFFFFFUL;
  for (size_t i = 0; i < size; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc >> 1) ^ (0xEDB88320UL & (uint32_t)(-(int32_t)(crc & 1)));
    }
  }
  return ~crc;
}

void initFS() {
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS mount failed; recovery mode"));
  }
}

void getInfo(Info* infoFS) {
#if defined(ESP8266)
  FSInfo fs_info;
  LittleFS.info(fs_info);
  infoFS->totalBytes = fs_info.totalBytes;
  infoFS->usedBytes = fs_info.usedBytes;
#elif defined(ESP32)
  infoFS->totalBytes = LittleFS.totalBytes();
  infoFS->usedBytes = LittleFS.usedBytes();
#endif
  return;
}

bool createDir(const char* path) {
  return LittleFS.mkdir(path);
}

uint8_t readFile(const char* path, uint8_t* buf, size_t size) {
  if (!path || !buf || size == 0) return 0;

  File file = LittleFS.open(path, "r");
  if (!file || file.isDirectory()) {
    return 0;
  }

  // Do not touch buf unless the whole config is present.
  if (file.size() != size) {
    file.close();
    return 0;
  }

  size_t read = file.read(buf, size);
  file.close();

  return read == size ? 1 : 0;
}

bool writeFile(const char* path, const uint8_t* buf, size_t size) {
  if (!path || !buf || size == 0) return false;

  if (!LittleFS.exists(path)) {
    if (strchr(path, '/')) {
      char* pathStr = strdup(path);
      if (pathStr) {
        char* ptr = strchr(pathStr, '/');
        while (ptr) {
          *ptr = 0;
          LittleFS.mkdir(pathStr);
          *ptr = '/';
          ptr = strchr(ptr + 1, '/');
        }
      }
      free(pathStr);
    }
  }

  File file = LittleFS.open(path, "w");
  if (!file) return false;
  size_t written = file.write(buf, size);
  delay(1);
  file.close();
  return written == size;
}

void getLoadDef(const char* path, uint8_t* buf, size_t size) {
  uint8_t isOk = readFile(path, buf, size);
  if (!isOk) {
    writeFile(path, buf, size);
  }
}

bool loadConfigBlock(const char* path, uint8_t* buf, size_t size) {
  if (!path || !buf || size == 0 || size > MAX_CONFIG_BLOCK_SIZE) return false;

  File file = LittleFS.open(path, "r");
  if (!file || file.isDirectory()) return false;

  // Legacy raw configs (before the block format) are migrated in place.
  if (file.size() == size) {
    if (file.read(buf, size) != size) {
      file.close();
      return false;
    }
    file.close();
    saveConfigBlock(path, buf, size);
    return true;
  }

  ConfigBlockHeader header;
  if (file.size() != sizeof(header) + size) {
    file.close();
    return false;
  }
  if (file.read((uint8_t*)&header, sizeof(header)) != sizeof(header)) {
    file.close();
    return false;
  }
  if (header.magic != CONFIG_BLOCK_MAGIC || header.version != CONFIG_BLOCK_VERSION || header.size != size) {
    file.close();
    return false;
  }

  uint8_t payload[MAX_CONFIG_BLOCK_SIZE];
  if (file.read(payload, size) != size) {
    file.close();
    return false;
  }
  file.close();

  if (crc32(payload, size) != header.crc) return false;

  memcpy(buf, payload, size);
  return true;
}

bool saveConfigBlock(const char* path, const uint8_t* buf, size_t size) {
  if (!path || !buf || size == 0 || size > MAX_CONFIG_BLOCK_SIZE) return false;

  uint8_t payload[sizeof(ConfigBlockHeader) + MAX_CONFIG_BLOCK_SIZE];
  ConfigBlockHeader header = {
      CONFIG_BLOCK_MAGIC,
      CONFIG_BLOCK_VERSION,
      (uint16_t)size,
      crc32(buf, size),
  };

  memcpy(payload, &header, sizeof(header));
  memcpy(payload + sizeof(header), buf, size);

  return writeFile(path, payload, sizeof(header) + size);
}

bool loadScriptFromFS(uint8_t id, char* buffer, uint16_t& len) {
  Serial.println("loadScriptFromFS");
  Serial.println(id);
  File file = LittleFS.open("/scripts.txt", "r");
  if (!file) {
    return false;
  }
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    int firstColon = line.indexOf(':');
    if (firstColon == -1) continue;
    int fileId = line.substring(0, firstColon).toInt();
    String rest = line.substring(firstColon + 1);
    int secondColon = rest.indexOf(':');
    if (secondColon == -1) continue;
    String script = rest.substring(secondColon + 1);

    if (fileId == id) {
      const uint16_t maxLen = 255;
      if (script.length() > maxLen) {
        file.close();
        return false;
      }

      strncpy(buffer, script.c_str(), maxLen);
      buffer[maxLen] = '\0';
      len = script.length();
      file.close();
      return true;
    }
  }

  file.close();
  return false;
}