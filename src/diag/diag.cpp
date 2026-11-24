#include "./diag.h"

#if VUESP_DIAGNOSTICS

#include <Arduino.h>

#include "../const/protocol.h"
#include "../discovery/discovery.h"
#include "../init/init.h"
#include "../runner/runner.h"
#include "../webserver/webserver.h"

#define DIAG_SAMPLES 64

// `enabled` and `enableSeq` are written from the WebSocket callback, which on
// ESP32 runs in a separate task; keep the cross-task state volatile and apply
// the window reset only from the loop context.
static volatile bool enabled = false;
static volatile uint16_t enableSeq = 0;
static volatile uint32_t lastRequest = 0;

static uint16_t seenSeq = 0;
static bool inLoop = false;
static uint32_t loopStartedAt = 0;
static uint32_t loopCount = 0;
static uint32_t loopLastUs = 0;
static uint32_t loopMaxUs = 0;
static uint32_t loopSamples[DIAG_SAMPLES];
static uint8_t loopSampleCount = 0;
static uint8_t loopSampleIndex = 0;
static uint32_t minFreeHeap = 0xFFFFFFFF;

static uint32_t freeHeapNow() {
  return (uint32_t)ESP.getFreeHeap();
}

static void resetWindow() {
  loopStartedAt = 0;
  loopCount = 0;
  loopLastUs = 0;
  loopMaxUs = 0;
  loopSampleCount = 0;
  loopSampleIndex = 0;
  minFreeHeap = freeHeapNow();
}

void diagInit() {
  enabled = false;
  enableSeq = 0;
  lastRequest = 0;
  seenSeq = 0;
  inLoop = false;
  resetWindow();
}

void diagSetDiagnostics(uint8_t on) {
  if (on) {
    if (!enabled) enableSeq++;
    enabled = true;
    lastRequest = millis();
    return;
  }

  enabled = false;
}

void diagLoopBegin() {
  if (!enabled) return;

  // Start a fresh window only from the loop context: a WS callback may enable
  // collection halfway through an iteration.
  if (seenSeq != enableSeq) {
    resetWindow();
    seenSeq = enableSeq;
  }

  loopStartedAt = micros();
  inLoop = true;
}

void diagLoopEnd() {
  if (!inLoop) return;
  inLoop = false;
  if (!enabled) return;

  const uint32_t elapsed = (uint32_t)(micros() - loopStartedAt);
  if (elapsed > 60000UL) return;  // 60 s: scans/OTA can legitimately be long

  loopCount++;
  loopLastUs = elapsed;
  if (elapsed > loopMaxUs) loopMaxUs = elapsed;

  // Heap APIs take a lock; sampling and the timeout check every 64 loops keep
  // the hot path cheap.
  if ((loopCount & 0x3F) == 0) {
    const uint32_t heap = freeHeapNow();
    if (heap < minFreeHeap) minFreeHeap = heap;
    if ((uint32_t)millis() - lastRequest > DIAG_TIMEOUT_MS) enabled = false;
  }

  loopSamples[loopSampleIndex] = elapsed;
  loopSampleIndex = (uint8_t)((loopSampleIndex + 1) % DIAG_SAMPLES);
  if (loopSampleCount < DIAG_SAMPLES) loopSampleCount++;
}

static uint32_t percentile95() {
  if (!loopSampleCount) return 0;
  uint32_t copy[DIAG_SAMPLES];
  for (uint8_t i = 0; i < loopSampleCount; i++) copy[i] = loopSamples[i];
  for (uint8_t i = 1; i < loopSampleCount; i++) {
    const uint32_t value = copy[i];
    int8_t j = (int8_t)(i - 1);
    while (j >= 0 && copy[j] > value) {
      copy[j + 1] = copy[j];
      j--;
    }
    copy[j + 1] = value;
  }
  uint8_t index = (uint8_t)(((uint16_t)loopSampleCount * 95) / 100);
  if (index >= loopSampleCount) index = loopSampleCount - 1;
  return copy[index];
}

void diagFillInfo(Info& info) {
  uint32_t maxBlock = 0;
  uint32_t stackFree = 0;
  uint16_t fragmentation = 0;

#if defined(ESP8266)
  maxBlock = (uint32_t)ESP.getMaxFreeBlockSize();
  fragmentation = (uint16_t)ESP.getHeapFragmentation();
  stackFree = (uint32_t)ESP.getFreeContStack();
#elif defined(ESP32)
  maxBlock = (uint32_t)ESP.getMaxAllocHeap();
  stackFree = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
#endif

  info.freeHeap = freeHeapNow();
  info.maxBlock = maxBlock;
  info.fragmentation = fragmentation;
  info.stackFree = stackFree;
  info.clients = wsClientCount();
  info.scripts = scriptRunner.getUsedSlotsCount();
  info.peers = (uint8_t)protocol.getPeerCount();
  info.wifi = isConnected;
  info.diagnostics = enabled ? 1 : 0;

  // Until the loop applies the reset for a new enable, report zeros instead of
  // values left over from the previous window.
  if (!enabled || seenSeq != enableSeq) {
    info.minFreeHeap = 0;
    info.loopLastUs = 0;
    info.loopMaxUs = 0;
    info.loopP95Us = 0;
    info.loopCount = 0;
    return;
  }

  info.minFreeHeap = minFreeHeap;
  info.loopLastUs = loopLastUs;
  info.loopMaxUs = loopMaxUs;
  info.loopP95Us = percentile95();
  info.loopCount = loopCount;
}

#endif
