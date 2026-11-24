#include "./bootlog.h"

#if VUESP_LOG_LEVEL >= 1

#include <Arduino.h>

#include "../const/define.h"

#if defined(ESP8266)
extern "C" uint8_t system_upgrade_userbin_check(void);
#elif defined(ESP32)
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#endif

static void printFirmware() {
  const uint8_t fw[] = DEF_DEVICE_FIRMWARE;
  Serial.printf("[boot] firmware %u.%u.%u\n", (unsigned)fw[0], (unsigned)fw[1], (unsigned)fw[2]);
}

#if defined(ESP32)
static const char* resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:
      return "power-on";
    case ESP_RST_EXT:
      return "external";
    case ESP_RST_SW:
      return "software";
    case ESP_RST_PANIC:
      return "panic";
    case ESP_RST_INT_WDT:
      return "interrupt-wdt";
    case ESP_RST_TASK_WDT:
      return "task-wdt";
    case ESP_RST_WDT:
      return "wdt";
    case ESP_RST_DEEPSLEEP:
      return "deep-sleep";
    case ESP_RST_BROWNOUT:
      return "brownout";
    case ESP_RST_SDIO:
      return "sdio";
    default:
      return "unknown";
  }
}

static const char* otaStateName(esp_ota_img_states_t state) {
  switch (state) {
    case ESP_OTA_IMG_NEW:
      return "new";
    case ESP_OTA_IMG_PENDING_VERIFY:
      return "pending-verify";
    case ESP_OTA_IMG_VALID:
      return "valid";
    case ESP_OTA_IMG_INVALID:
      return "invalid";
    case ESP_OTA_IMG_ABORTED:
      return "aborted";
    default:
      return "unknown";
  }
}
#endif

void bootLog() {
  Serial.println();
  printFirmware();

#if defined(ESP8266)
  Serial.printf("[boot] reset: %s\n", ESP.getResetReason().c_str());
  Serial.printf("[boot] info: %s\n", ESP.getResetInfo().c_str());
  Serial.printf("[boot] userbin: %s\n", system_upgrade_userbin_check() == 0 ? "bin1" : "bin2");
#elif defined(ESP32)
  const esp_reset_reason_t reason = esp_reset_reason();
  Serial.printf("[boot] reset: %s (%d)\n", resetReasonName(reason), (int)reason);

  const esp_partition_t* running = esp_ota_get_running_partition();
  if (running) {
    Serial.printf("[boot] partition: %s\n", running->label);
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
      Serial.printf("[boot] ota state: %s\n", otaStateName(state));
    }
  }
#endif

  Serial.printf("[boot] free heap: %u\n", (unsigned)ESP.getFreeHeap());
}

#endif
