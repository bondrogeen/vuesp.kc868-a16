#include "./config/ConfigStore.h"
#include "./diag/bootlog.h"
#include "./diag/diag.h"
#include "./device/device.h"
#include "./discovery/discovery.h"
#include "./files/files.h"
#include "./gpio/gpio.h"
#include "./init/init.h"
#include "./modbus/modbus.h"
#include "./runner/runner.h"
#include "./setup/serialSetup.h"
#include "./tasks/tasks.h"
#include "./webserver/webserver.h"

#if defined(ESP8266)
uint32_t id = ESP.getChipId();
#elif defined(ESP32)
uint32_t id = (uint32_t)(ESP.getEfuseMac() >> 24);
#endif

uint32_t now;
uint32_t lastTimeMain;

uint8_t isSetup = false;
uint8_t isInit = false;
uint8_t isFirst = false;

void setup() {
  Serial.begin(115200);
  bootLog();
  diagInit();
  initFS();
  initEEprom();
}

void setupFirst() {
  configStore.begin();
  setupFirstDevice();
  setupFirstGPIO();
  setupFirstModbus();
}

void setupDelay() {
  infoFS.id = id;
  infoFS.board = DEF_BOARD;
  char nameDevice[40];
  snprintf(nameDevice, sizeof(nameDevice), "%s%02X", DEF_DEVICE_NAME, id);
  strncpy(infoFS.name, nameDevice, sizeof(infoFS.name) - 1);
  infoFS.name[sizeof(infoFS.name) - 1] = 0;

  getInfo(&infoFS);
  loadConfig(settings, id);
  initWiFi();
  initEthernet();
  setupGPIO();
  setupDevice();
  setupModbus();
  setupServer();
  setupDiscovery();
  setupRunner();

}

void loop() {
  diagLoopBegin();
  now = millis();
  if (!isFirst) {
    setupFirst();
    isFirst = true;
  }

  if (!isSetup && now > 100) {
    setupDelay();
    isSetup = true;
  }

  if (isSetup) {
    loopDiscovery(now);
    loopServer(now);
    loopTask(now);
    loopGPIO(now);
    loopDevice(now);
    loopModbus(now);
    loopWiFi(now);
    loopRunner(now);
    loopSerialSetup();
  }

  if (configStore.dirty()) {
    const TConfigStatus status = configStore.flush();
    if (status != TConfigStatus::Ok && status != TConfigStatus::Deferred) {
#if VUESP_LOG_LEVEL >= 1
      Serial.printf("[config] flush: %d\n", (int)status);
#endif
    }
  }

  if (now - lastTimeMain > 1000) {
    lastTimeMain = now;
    infoFS.uptime++;
  }
  diagLoopEnd();

  // Yield the rest of the slot to Wi-Fi and system tasks. All timers here are
  // 10 ms or slower, so 1 ms does not change behaviour but stops the busy loop.
  delay(1);
}