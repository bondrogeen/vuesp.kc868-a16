#include "./tasks.h"

#define MAX_TASK_CLIENTS 4

uint8_t tasks[KEY_END];

// Requests are handled asynchronously, so remember every client that is waiting
// for a response instead of only the last one.
static uint32_t taskClients[KEY_END][MAX_TASK_CLIENTS];
static uint8_t taskClientCount[KEY_END];

static bool isTargetedTask(uint8_t task) {
  return task == KEY_SETTINGS || task == KEY_INFO || task == KEY_FILES || task == KEY_SCAN;
}

static void addTaskClient(uint8_t task, uint32_t clientId) {
  if (!clientId || !isTargetedTask(task)) return;

  for (uint8_t i = 0; i < taskClientCount[task]; i++) {
    if (taskClients[task][i] == clientId) return;
  }

  if (taskClientCount[task] >= MAX_TASK_CLIENTS) {
    // Drop the oldest waiter so a new client is not silently ignored.
    memmove(&taskClients[task][0], &taskClients[task][1], sizeof(taskClients[task][0]) * (MAX_TASK_CLIENTS - 1));
    taskClientCount[task] = MAX_TASK_CLIENTS - 1;
  }

  taskClients[task][taskClientCount[task]++] = clientId;
}

static void sendToTaskClients(uint8_t task, uint8_t* message, size_t len) {
  for (uint8_t i = 0; i < taskClientCount[task]; i++) {
    wsSendTo(taskClients[task][i], message, len);
  }
}

static void clearTaskClients(uint8_t task) {
  taskClientCount[task] = 0;
}

Scan scan = {KEY_SCAN, 0, 0, 0, 0, 0, ""};
char safePass[32];
Files files = {KEY_FILES, 0, 0, 0, 0, ""};
Port port = {KEY_PORT, 0, 0, 0};
Message message = {KEY_MESSAGE, MESSAGE_TYPE_LOG};

static void copyString(char* dst, size_t size, const char* src) {
  if (!dst || size == 0) return;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  strncpy(dst, src, size - 1);
  dst[size - 1] = '\0';
}

void onWsEventTasks(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  if (!arg || !data || len == 0 || task >= KEY_END) return;

  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  tasks[task] = true;
  addTaskClient(task, clientId);

  if (task == KEY_SETTINGS && info->len == sizeof(settings)) {
    memcpy(&settings, data, sizeof(settings));
    settings.wifiSsid[sizeof(settings.wifiSsid) - 1] = '\0';
    settings.wifiPass[sizeof(settings.wifiPass) - 1] = '\0';
    settings.authLogin[sizeof(settings.authLogin) - 1] = '\0';
    settings.authPass[sizeof(settings.authPass) - 1] = '\0';

    if (strcmp(settings.wifiPass, DEF_HIDE_PASS) == 0) {
      copyString(settings.wifiPass, sizeof(settings.wifiPass), safePass);
    }
    saveSettings(settings);
  }
  if (task == KEY_FILES && info->len == sizeof(files)) {
    memcpy(&files, data, sizeof(files));
    files.name[sizeof(files.name) - 1] = '\0';
  }
  if (task == KEY_PORT && info->len == sizeof(port)) {
    memcpy(&port, data, sizeof(port));
  }
  if (task == KEY_MESSAGE && info->len == sizeof(message)) {
    memcpy(&message, data, sizeof(message));
    message.text[sizeof(message.text) - 1] = '\0';
  }
  if (task == KEY_DISCOVERY && info->len == sizeof(discovery)) {
    memcpy(&discovery, data, sizeof(discovery));
  }
}

#if defined(ESP8266)
void getFile(char* name, uint8_t task) {
  Dir dir = LittleFS.openDir(name);
  while (dir.next()) {
    memset(files.name, 0, sizeof(files.name));
    dir.fileName().toCharArray(files.name, 32);
    files.size = dir.fileSize();
    files.isFile = dir.isFile();
    files.isDir = dir.isDirectory();
    sendToTaskClients(task, (uint8_t*)&files, sizeof(files));
  }
}

#elif defined(ESP32)

void getFile(char* name, uint8_t task) {
  File root = LittleFS.open(name);
  File file = root.openNextFile();
  String str = "";
  while (file) {
    memset(files.name, 0, sizeof(files.name));
    copyString(files.name, sizeof(files.name), file.name());
    files.size = file.size();
    files.isFile = !file.isDirectory();
    files.isDir = file.isDirectory();
    sendToTaskClients(task, (uint8_t*)&files, sizeof(files));
    file = root.openNextFile();
  }
}
#endif

void scanWiFi(uint8_t task) {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    scan.id = i;
    memset(scan.ssid, 0, sizeof(scan.ssid));
    WiFi.SSID(i).toCharArray(scan.ssid, 32);
    scan.channel = WiFi.channel(i);
    scan.rssi = WiFi.RSSI(i);
    scan.encryptionType = WiFi.encryptionType(i);
    // scan.isHidden = WiFi.isHidden(i);
    sendToTaskClients(task, (uint8_t*)&scan, sizeof(scan));
  };
}

void sendLog(const char* text) {
  message.type = MESSAGE_TYPE_LOG;
  memset(message.text, 0, sizeof(message.text));
  copyString(message.text, sizeof(message.text), text);
  wsSendAll((uint8_t*)&message, sizeof(message));
}

void loopTask(uint32_t now) {
  if (tasks[KEY_SETTINGS]) {
    if (strcmp(settings.wifiPass, DEF_HIDE_PASS) != 0) {
      memset(safePass, 0, sizeof(safePass));
      copyString(safePass, sizeof(safePass), settings.wifiPass);
      memset(settings.wifiPass, 0, sizeof(settings.wifiPass));
      copyString(settings.wifiPass, sizeof(settings.wifiPass), DEF_HIDE_PASS);
    }
    sendToTaskClients(KEY_SETTINGS, (uint8_t*)&settings, sizeof(settings));
    tasks[KEY_SETTINGS] = false;
    clearTaskClients(KEY_SETTINGS);
  };
  if (tasks[KEY_INFO]) {
    getInfo(&infoFS);
    sendToTaskClients(KEY_INFO, (uint8_t*)&infoFS, sizeof(infoFS));
    tasks[KEY_INFO] = false;
    clearTaskClients(KEY_INFO);
  }
  if (tasks[KEY_FILES]) {
    tasks[KEY_FILES] = false;
    getFile(files.name, KEY_FILES);
    clearTaskClients(KEY_FILES);
  }
  if (tasks[KEY_REBOOT]) reboot();

  if (tasks[KEY_SCAN]) {
    tasks[KEY_SCAN] = false;
    scanWiFi(KEY_SCAN);
    clearTaskClients(KEY_SCAN);
  }
  if (tasks[KEY_MESSAGE]) {
    tasks[KEY_MESSAGE] = false;
    eventRunner();
  }
  if (tasks[KEY_DISCOVERY]) {
    tasks[KEY_DISCOVERY] = false;
    commDiscovery();
  }
}