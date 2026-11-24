#include "./tasks.h"

#include <stddef.h>

#include "../diag/diag.h"
#include "../utils/text.h"

uint8_t tasks[KEY_END];

Scan scan = {KEY_SCAN, 0, 0, 0, 0, 0, ""};
char safePass[32];
Files files = {KEY_FILES, 0, 0, 0, 0, ""};
Port port = {KEY_PORT, 0, 0, 0};
Message message = {KEY_MESSAGE, MESSAGE_TYPE_LOG};

void onWsEventTasks(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  tasks[task] = true;
  if (task == KEY_SETTINGS && info->len == sizeof(settings)) {
    memcpy(&settings, data, sizeof(settings));
    if (strcmp(settings.wifiPass, DEF_HIDE_PASS) == 0) {
      memset(settings.wifiPass, 0, sizeof(settings.wifiPass));
      strcpy(settings.wifiPass, safePass);
    }
    saveSettings(settings);
  }
  if (task == KEY_FILES && info->len == sizeof(files)) {
    memcpy(&files, data, sizeof(files));
  }
  if (task == KEY_PORT && info->len == sizeof(port)) {
    memcpy(&port, data, sizeof(port));
  }
  if (task == KEY_MESSAGE && info->len == sizeof(message)) {
    memcpy(&message, data, sizeof(message));
  }
  if (task == KEY_DISCOVERY && info->len == sizeof(discovery)) {
    memcpy(&discovery, data, sizeof(discovery));
  }
  if (task == KEY_INFO && info->len == sizeof(infoFS)) {
    diagSetDiagnostics(data[offsetof(Info, diagnostics)]);
  }
}

#define FILE_LIST_BATCH 4

#if defined(ESP8266)
static Dir fileDir;
static bool fileListing = false;

static void fileListBegin(const char* name) {
  fileDir = Dir();
  fileDir = LittleFS.openDir(name);
  fileListing = true;
}

static bool fileListPoll() {
  if (!fileListing) return false;

  uint8_t count = 0;
  while (count < FILE_LIST_BATCH && fileDir.next()) {
    memset(files.name, 0, sizeof(files.name));
    fileDir.fileName().toCharArray(files.name, sizeof(files.name));
    files.size = fileDir.fileSize();
    files.isFile = fileDir.isFile();
    files.isDir = fileDir.isDirectory();
    wsSend((uint8_t*)&files, sizeof(files));
    yield();
    count++;
  }

  if (count < FILE_LIST_BATCH) {
    fileListing = false;
    fileDir = Dir();
  }
  return fileListing;
}

#elif defined(ESP32)
static File fileRoot;
static File fileEntry;
static bool fileListing = false;

static void fileListBegin(const char* name) {
  if (fileListing) {
    fileEntry.close();
    fileRoot.close();
  }

  fileRoot = LittleFS.open(name);
  fileEntry = fileRoot.openNextFile();
  fileListing = (bool)fileEntry;
}

static bool fileListPoll() {
  if (!fileListing) return false;

  uint8_t count = 0;
  while (count < FILE_LIST_BATCH && fileEntry) {
    memset(files.name, 0, sizeof(files.name));
    strncpy(files.name, fileEntry.name(), sizeof(files.name) - 1);
    files.size = fileEntry.size();
    files.isFile = !fileEntry.isDirectory();
    files.isDir = fileEntry.isDirectory();
    wsSend((uint8_t*)&files, sizeof(files));
    yield();
    fileEntry = fileRoot.openNextFile();
    count++;
  }

  if (!fileEntry) {
    fileRoot.close();
    fileListing = false;
  }
  return fileListing;
}
#endif

#define SCAN_TIMEOUT_MS 20000
#define SCAN_RESULT_BATCH 4

static bool scanPending = false;
static uint32_t scanStartedAt = 0;
static int scanCount = 0;
static int scanIndex = 0;

static void scanWiFiStart() {
  if (scanPending) return;

#if defined(ESP32)
  // max_ms_per_chan × 20 = core scan timeout; the default 300 ms gives 6 s,
  // which is too short for a busy 2.4 GHz band.
  const int started = WiFi.scanNetworks(true, false, false, 1000);
#else
  const int started = WiFi.scanNetworks(true);
#endif
  if (started == WIFI_SCAN_RUNNING || started >= 0) {
    scanPending = true;
    scanStartedAt = millis();
  }
}

static void scanWiFiPoll() {
  if (!scanPending) return;

  // Send results in small batches so a dense RF environment does not produce a
  // single long iteration.
  if (scanCount > 0) {
    uint8_t sent = 0;
    while (scanIndex < scanCount && sent < SCAN_RESULT_BATCH) {
      scan.id = scanIndex;
      memset(scan.ssid, 0, sizeof(scan.ssid));
      WiFi.SSID(scanIndex).toCharArray(scan.ssid, 32);
      textSanitizeSsid(scan.ssid);
      scan.channel = WiFi.channel(scanIndex);
      scan.rssi = WiFi.RSSI(scanIndex);
      scan.encryptionType = WiFi.encryptionType(scanIndex);
      wsSend((uint8_t*)&scan, sizeof(scan));
      yield();
      scanIndex++;
      sent++;
    }

    if (scanIndex >= scanCount) {
      WiFi.scanDelete();
      scanPending = false;
      scanCount = 0;
      scanIndex = 0;
    }
    return;
  }

  const int count = WiFi.scanComplete();
  if (count == WIFI_SCAN_RUNNING) {
    if (millis() - scanStartedAt > SCAN_TIMEOUT_MS) {
      WiFi.scanDelete();
      scanPending = false;
    }
    return;
  }

  if (count > 0) {
    scanCount = count;
    scanIndex = 0;
    return;
  }

  WiFi.scanDelete();
  scanPending = false;
}

void sendLog(const char* text) {
  message.type = MESSAGE_TYPE_LOG;
  memset(message.text, 0, sizeof(message.text));
  strcpy(message.text, text);
  wsSendAll((uint8_t*)&message, sizeof(message));
}

void loopTask(uint32_t now) {
  if (tasks[KEY_SETTINGS]) {
    if (strcmp(settings.wifiPass, DEF_HIDE_PASS) != 0) {
      memset(safePass, 0, sizeof(safePass));
      strcpy(safePass, settings.wifiPass);
      memset(settings.wifiPass, 0, sizeof(settings.wifiPass));
      strcpy(settings.wifiPass, DEF_HIDE_PASS);
    }
    wsSend((uint8_t*)&settings, sizeof(settings));
    tasks[KEY_SETTINGS] = false;
  };
  if (tasks[KEY_INFO]) {
    getInfo(&infoFS);
    diagFillInfo(infoFS);
    wsSend((uint8_t*)&infoFS, sizeof(infoFS));
    tasks[KEY_INFO] = false;
  }
  if (tasks[KEY_FILES]) {
    tasks[KEY_FILES] = false;
    fileListBegin(files.name);
  }
  fileListPoll();
  if (tasks[KEY_REBOOT]) {
#if VUESP_LOG_LEVEL >= 1
    Serial.println("[boot] restart requested");
#endif
    // Give the async web task time to flush the OTA/reboot HTTP response
    // before restarting. On ESP32 the handler runs in a separate task, so an
    // immediate reboot loses the reply.
    delay(1000);
    reboot();
  }

  if (tasks[KEY_SCAN]) {
    tasks[KEY_SCAN] = false;
    scanWiFiStart();
  }
  scanWiFiPoll();
  if (tasks[KEY_MESSAGE]) {
    tasks[KEY_MESSAGE] = false;
    eventRunner();
  }
  if (tasks[KEY_DISCOVERY]) {
    tasks[KEY_DISCOVERY] = false;
    commDiscovery();
  }
}