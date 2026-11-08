#include "./eeprom.h"

#include <stddef.h>

static_assert(sizeof(Settings) <= 256, "Settings must fit EEPROM");
static_assert(offsetof(Settings, version) == 4, "Settings.version offset changed");

void initEEprom() {
  EEPROM.begin(256);
}

bool saveSettings(Settings& settings) {
  EEPROM.put(CONFIG_START, settings);
  return EEPROM.commit();
}

void loadConfig(Settings& settings, uint32_t id) {
  uint16_t version = 0;
  EEPROM.get(CONFIG_START + 4, version);
  if (version == SETTINGS_VERSION) {
    EEPROM.get(CONFIG_START, settings);
    settings.wifiSsid[sizeof(settings.wifiSsid) - 1] = '\0';
    settings.wifiPass[sizeof(settings.wifiPass) - 1] = '\0';
    settings.authLogin[sizeof(settings.authLogin) - 1] = '\0';
    settings.authPass[sizeof(settings.authPass) - 1] = '\0';
  } else {
    char nameDevice[20];
    sprintf(nameDevice, "%s%02X", DEF_DEVICE_NAME, id);
    strcpy(settings.wifiSsid, nameDevice);
    settings.version = SETTINGS_VERSION;
    saveSettings(settings);
  }
}
