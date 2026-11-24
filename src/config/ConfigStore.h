#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <stddef.h>
#include <stdint.h>

#define CONFIG_STORE_PATH "/tmp/config.bin"
#define CONFIG_LEGACY_DEVICE_PATH "/tmp/device.io"
#define CONFIG_LEGACY_GPIO_PATH "/tmp/gpio.bin"

#define CONFIG_STORE_FORMAT_VERSION 1
#define CONFIG_DEVICE_VERSION 1
#define CONFIG_MODBUS_VERSION 1
#define CONFIG_PORTS_VERSION 1

#ifndef CONFIG_STORE_MAX_PENDING
#define CONFIG_STORE_MAX_PENDING 5
#endif

// Fixed arena for data copied by write(); no heap, no lifetime requirements.
#ifndef CONFIG_STORE_ARENA_SIZE
#define CONFIG_STORE_ARENA_SIZE 384
#endif

// Do not write the config file more often than this.
#ifndef CONFIG_STORE_MIN_FLUSH_INTERVAL_MS
#define CONFIG_STORE_MIN_FLUSH_INTERVAL_MS 1000
#endif

// During testing a corrupt file is kept as /tmp/config.bad; disable for release.
#ifndef CONFIG_STORE_KEEP_BAD
#define CONFIG_STORE_KEEP_BAD 0
#endif

enum class TConfigStatus : uint8_t {
  Ok = 0,
  NotFound,
  VersionMismatch,
  TooLarge,
  Corrupt,
  IoError,
  Invalid,
  NoSpace,
  Deferred
};

namespace fs {
class File;
}

class ConfigStore {
 public:
  bool begin(const char* path = CONFIG_STORE_PATH);

  TConfigStatus read(const char* key, void* data, size_t capacity, uint16_t version,
                     size_t* actual = nullptr);
  TConfigStatus write(const char* key, const void* data, size_t size, uint16_t version);
  TConfigStatus remove(const char* key);
  bool exists(const char* key);

  TConfigStatus flush();
  bool dirty() const { return _dirty; }

 private:
  static const size_t KEY_SIZE = 12;
  static const size_t HEADER_SIZE = 16;
  static const size_t RECORD_HEADER_SIZE = 20;

  struct Pending {
    char key[KEY_SIZE];
    uint16_t offset;
    uint16_t size;
    uint16_t version;
    bool remove;
    bool handled;
  };

  char _path[24];
  char _tmpPath[28];
  bool _filePresent;
  bool _fileValid;
  bool _dirty;
  uint32_t _lastFlush;
  uint8_t _pendingCount;
  Pending _pending[CONFIG_STORE_MAX_PENDING];
  uint8_t _arena[CONFIG_STORE_ARENA_SIZE];
  uint16_t _arenaUsed;

  Pending* findPending(const char* key);
  bool readHeader(fs::File& file, uint16_t* recordCount, uint32_t* payloadSize);
  bool validateRecords(fs::File& file, uint16_t recordCount, uint32_t payloadSize);
  bool writeRecord(fs::File& file, const char* key, const void* data, size_t size,
                   uint16_t version);
};

extern ConfigStore configStore;

#endif
