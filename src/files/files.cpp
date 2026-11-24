#include "./files.h"

#include "./init/init.h"

void initFS() {
  if (!LittleFS.begin()) {
    if (LittleFS.format()) {
#if VUESP_LOG_LEVEL >= 1
      Serial.println(F("Filesystem formatted!"));
#endif
      LittleFS.begin();
    }
  }
  if (!LittleFS.exists("/tmp")) LittleFS.mkdir("/tmp");
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
  updateEthernetInfo(infoFS);
  return;
}

bool createDir(const char* path) {
  return LittleFS.mkdir(path);
}

uint8_t readFile(const char* path, uint8_t* buf, size_t size) {
  File file = LittleFS.open(path, "r");
  if (!file || file.isDirectory()) {
    return 0;
  } else {
    while (file.available()) {
      file.read(buf, size);
    }
    file.close();
    return 1;
  }
}

void writeFile(const char* path, const uint8_t* buf, size_t size) {
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
  if (!file) return;
  file.write(buf, size);
  delay(1);
  file.close();
}

void getLoadDef(const char* path, uint8_t* buf, size_t size) {
  uint8_t isOk = readFile(path, buf, size);
  if (!isOk) {
    writeFile(path, buf, size);
  }
}

#define SCRIPT_LINE_MAX 384
#define SCRIPT_TEXT_MAX 256

static bool matchScriptLine(char* line, uint8_t id, char* buffer, uint16_t& len) {
  char* start = line;
  while (*start == ' ' || *start == '\t') start++;

  char* end = start + strlen(start);
  while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) end--;
  *end = '\0';
  if (!*start) return false;

  char* firstColon = strchr(start, ':');
  if (!firstColon) return false;
  *firstColon = '\0';
  const int fileId = atoi(start);

  char* rest = firstColon + 1;
  char* secondColon = strchr(rest, ':');
  if (!secondColon) return false;

  const char* script = secondColon + 1;
  if (fileId != id) return false;

  const size_t scriptLen = strlen(script);
  if (scriptLen >= SCRIPT_TEXT_MAX) return false;

  memcpy(buffer, script, scriptLen + 1);
  len = (uint16_t)scriptLen;
  return true;
}

bool loadScriptFromFS(uint8_t id, char* buffer, uint16_t& len) {
#if VUESP_LOG_LEVEL >= 3
  Serial.println("loadScriptFromFS");
  Serial.println(id);
#endif
  File file = LittleFS.open("/www/scripts.txt", "r");
  if (!file) {
    return false;
  }

  static char line[SCRIPT_LINE_MAX];
  uint16_t pos = 0;
  bool overflow = false;
  bool found = false;

  while (file.available()) {
    const int ch = file.read();
    if (ch < 0) break;

    if (ch == '\n') {
      if (!overflow && pos > 0) {
        line[pos] = '\0';
        if (matchScriptLine(line, id, buffer, len)) {
          found = true;
          break;
        }
      }
      pos = 0;
      overflow = false;
      continue;
    }

    if (ch == '\r') continue;
    if (pos < sizeof(line) - 1) {
      line[pos++] = (char)ch;
    } else {
      overflow = true;
    }
  }

  if (!found && !overflow && pos > 0) {
    line[pos] = '\0';
    found = matchScriptLine(line, id, buffer, len);
  }

  file.close();
  return found;
}