#include "./ConfigStore.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <stdio.h>
#include <string.h>

#include "../const/define.h"

ConfigStore configStore;

static const char CONFIG_MAGIC[4] = {'V', 'U', 'C', 'F'};
static const char CONFIG_BAD_PATH[] = "/tmp/config.bad";

static void put16(uint8_t* out, uint16_t value) {
  out[0] = (uint8_t)(value & 0xFF);
  out[1] = (uint8_t)((value >> 8) & 0xFF);
}

static uint16_t get16(const uint8_t* in) {
  return (uint16_t)in[0] | ((uint16_t)in[1] << 8);
}

static void put32(uint8_t* out, uint32_t value) {
  out[0] = (uint8_t)(value & 0xFF);
  out[1] = (uint8_t)((value >> 8) & 0xFF);
  out[2] = (uint8_t)((value >> 16) & 0xFF);
  out[3] = (uint8_t)((value >> 24) & 0xFF);
}

static uint32_t get32(const uint8_t* in) {
  return (uint32_t)in[0] | ((uint32_t)in[1] << 8) | ((uint32_t)in[2] << 16) |
         ((uint32_t)in[3] << 24);
}

static uint32_t crc32Update(uint32_t crc, const uint8_t* data, size_t length) {
  while (length--) {
    crc ^= *data++;
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
    }
  }
  return crc;
}

static uint32_t crc32(const uint8_t* data, size_t length) {
  return ~crc32Update(0xFFFFFFFFUL, data, length);
}

// Reads exactly `size` bytes, optionally copies them to `dst` and/or writes
// them to `out`, and updates the CRC. No large buffer is allocated.
static bool copyBytes(File& in, File* out, void* dst, size_t capacity, size_t size,
                      uint32_t* crcOut) {
  uint8_t buffer[64];
  size_t copied = 0;
  size_t remaining = size;
  uint32_t crc = 0xFFFFFFFFUL;

  while (remaining > 0) {
    const size_t chunk = remaining > sizeof(buffer) ? sizeof(buffer) : remaining;
    const size_t got = in.read(buffer, chunk);
    if (got != chunk) return false;

    crc = crc32Update(crc, buffer, chunk);

    if (out) {
      if (out->write(buffer, chunk) != chunk) return false;
    }

    if (dst && copied < capacity) {
      size_t toCopy = chunk;
      if (toCopy > capacity - copied) toCopy = capacity - copied;
      memcpy((uint8_t*)dst + copied, buffer, toCopy);
      copied += toCopy;
    }

    remaining -= chunk;
  }

  if (crcOut) *crcOut = ~crc;
  return true;
}

bool ConfigStore::begin(const char* path) {
  strncpy(_path, path, sizeof(_path) - 1);
  _path[sizeof(_path) - 1] = '\0';
  snprintf(_tmpPath, sizeof(_tmpPath), "%s.tmp", _path);

  _filePresent = false;
  _fileValid = false;
  _dirty = false;
  _lastFlush = 0;
  _pendingCount = 0;
  _arenaUsed = 0;

  if (!LittleFS.exists("/tmp")) LittleFS.mkdir("/tmp");
  LittleFS.remove(_tmpPath);

  if (!LittleFS.exists(_path)) return true;
  _filePresent = true;

  File file = LittleFS.open(_path, "r");
  if (!file) return true;

  uint16_t recordCount = 0;
  uint32_t payloadSize = 0;
  const bool valid = readHeader(file, &recordCount, &payloadSize) &&
                     validateRecords(file, recordCount, payloadSize);
  file.close();

  if (!valid) {
#if CONFIG_STORE_KEEP_BAD
    LittleFS.remove(CONFIG_BAD_PATH);
    if (LittleFS.rename(_path, CONFIG_BAD_PATH)) _filePresent = false;
#endif
    return true;
  }

  _fileValid = true;
  return true;
}

bool ConfigStore::readHeader(File& file, uint16_t* recordCount, uint32_t* payloadSize) {
  uint8_t header[HEADER_SIZE];
  file.seek(0);
  if (file.read(header, sizeof(header)) != sizeof(header)) return false;
  if (memcmp(header, CONFIG_MAGIC, sizeof(CONFIG_MAGIC)) != 0) return false;
  if (header[4] != CONFIG_STORE_FORMAT_VERSION) return false;

  const uint32_t storedCrc = get32(header + 12);
  if (storedCrc != crc32(header, 12)) return false;

  const uint16_t count = get16(header + 6);
  const uint32_t payload = get32(header + 8);
  if ((size_t)HEADER_SIZE + payload > file.size()) return false;

  *recordCount = count;
  *payloadSize = payload;
  return true;
}

bool ConfigStore::validateRecords(File& file, uint16_t recordCount, uint32_t payloadSize) {
  uint32_t sum = 0;

  for (uint16_t i = 0; i < recordCount; i++) {
    uint8_t header[RECORD_HEADER_SIZE];
    if (file.read(header, sizeof(header)) != sizeof(header)) return false;

    const uint16_t size = get16(header + 14);
    const size_t position = file.position();
    const size_t fileSize = file.size();
    if (position > fileSize || size > fileSize - position) return false;
    if (!file.seek(position + size)) return false;

    sum += RECORD_HEADER_SIZE + size;
  }

  return sum == payloadSize;
}

ConfigStore::Pending* ConfigStore::findPending(const char* key) {
  for (uint8_t i = 0; i < _pendingCount; i++) {
    if (strncmp(_pending[i].key, key, KEY_SIZE) == 0) return &_pending[i];
  }
  return nullptr;
}

TConfigStatus ConfigStore::read(const char* key, void* data, size_t capacity,
                                uint16_t version, size_t* actual) {
  if (actual) *actual = 0;
  if (!_filePresent) return TConfigStatus::NotFound;
  if (!_fileValid) return TConfigStatus::Corrupt;

  File file = LittleFS.open(_path, "r");
  if (!file) return TConfigStatus::IoError;

  uint16_t recordCount = 0;
  uint32_t payloadSize = 0;
  if (!readHeader(file, &recordCount, &payloadSize)) {
    file.close();
    return TConfigStatus::Corrupt;
  }

  for (uint16_t i = 0; i < recordCount; i++) {
    uint8_t header[RECORD_HEADER_SIZE];
    if (file.read(header, sizeof(header)) != sizeof(header)) {
      file.close();
      return TConfigStatus::Corrupt;
    }

    const char* recordKey = (const char*)header;
    const uint16_t recordVersion = get16(header + 12);
    const uint16_t recordSize = get16(header + 14);
    const uint32_t recordCrc = get32(header + 16);

    if (strncmp(recordKey, key, KEY_SIZE) == 0) {
      if (recordVersion != version) {
        file.close();
        return TConfigStatus::VersionMismatch;
      }
      if (recordSize > capacity) {
        file.close();
        return TConfigStatus::TooLarge;
      }

      uint32_t crc = 0;
      if (!copyBytes(file, nullptr, data, capacity, recordSize, &crc)) {
        file.close();
        return TConfigStatus::Corrupt;
      }
      if (crc != recordCrc) {
        file.close();
        return TConfigStatus::Corrupt;
      }

      if (actual) *actual = recordSize;
      file.close();
      return TConfigStatus::Ok;
    }

    if (!file.seek(file.position() + recordSize)) {
      file.close();
      return TConfigStatus::Corrupt;
    }
  }

  file.close();
  return TConfigStatus::NotFound;
}

TConfigStatus ConfigStore::write(const char* key, const void* data, size_t size,
                                 uint16_t version) {
  if (!key || strlen(key) >= KEY_SIZE) return TConfigStatus::Invalid;
  if (size > 0xFFFF) return TConfigStatus::TooLarge;
  if (size > 0 && !data) return TConfigStatus::Invalid;

  Pending* pending = findPending(key);
  if (!pending) {
    if (_pendingCount >= CONFIG_STORE_MAX_PENDING) return TConfigStatus::NoSpace;
    pending = &_pending[_pendingCount++];
    memset(pending, 0, sizeof(*pending));
    strncpy(pending->key, key, KEY_SIZE - 1);
  }

  if (size > (size_t)(CONFIG_STORE_ARENA_SIZE - _arenaUsed)) {
    return TConfigStatus::NoSpace;
  }

  const uint16_t offset = _arenaUsed;
  if (size > 0) memcpy(_arena + offset, data, size);
  _arenaUsed += (uint16_t)size;

  pending->offset = offset;
  pending->size = (uint16_t)size;
  pending->version = version;
  pending->remove = false;
  pending->handled = false;
  _dirty = true;
  return TConfigStatus::Ok;
}

TConfigStatus ConfigStore::remove(const char* key) {
  if (!key || strlen(key) >= KEY_SIZE) return TConfigStatus::Invalid;

  Pending* pending = findPending(key);
  if (!pending) {
    if (_pendingCount >= CONFIG_STORE_MAX_PENDING) return TConfigStatus::NoSpace;
    pending = &_pending[_pendingCount++];
    memset(pending, 0, sizeof(*pending));
    strncpy(pending->key, key, KEY_SIZE - 1);
  }

  pending->offset = 0;
  pending->size = 0;
  pending->version = 0;
  pending->remove = true;
  pending->handled = false;
  _dirty = true;
  return TConfigStatus::Ok;
}

bool ConfigStore::exists(const char* key) {
  if (!_fileValid) return false;

  File file = LittleFS.open(_path, "r");
  if (!file) return false;

  uint16_t recordCount = 0;
  uint32_t payloadSize = 0;
  if (!readHeader(file, &recordCount, &payloadSize)) {
    file.close();
    return false;
  }

  for (uint16_t i = 0; i < recordCount; i++) {
    uint8_t header[RECORD_HEADER_SIZE];
    if (file.read(header, sizeof(header)) != sizeof(header)) break;
    const uint16_t recordSize = get16(header + 14);
    if (strncmp((const char*)header, key, KEY_SIZE) == 0) {
      file.close();
      return true;
    }
    if (!file.seek(file.position() + recordSize)) break;
  }

  file.close();
  return false;
}

bool ConfigStore::writeRecord(File& file, const char* key, const void* data, size_t size,
                              uint16_t version) {
  if (size > 0xFFFF) return false;

  uint8_t header[RECORD_HEADER_SIZE];
  memset(header, 0, sizeof(header));
  strncpy((char*)header, key, KEY_SIZE - 1);
  put16(header + 12, version);
  put16(header + 14, (uint16_t)size);
  put32(header + 16, crc32((const uint8_t*)data, size));

  if (file.write(header, sizeof(header)) != sizeof(header)) return false;
  if (size > 0 && file.write((const uint8_t*)data, size) != size) return false;
  return true;
}

TConfigStatus ConfigStore::flush() {
  if (!_dirty) return TConfigStatus::Ok;

  const uint32_t now = millis();
  if (_lastFlush != 0 &&
      (uint32_t)(now - _lastFlush) < CONFIG_STORE_MIN_FLUSH_INTERVAL_MS) {
    return TConfigStatus::Deferred;
  }
  _lastFlush = now;

  bool hasWrite = false;
  for (uint8_t i = 0; i < _pendingCount; i++) {
    if (!_pending[i].remove) hasWrite = true;
    _pending[i].handled = false;
  }

  // Only removals and no file yet: nothing to create.
  if (!_fileValid && !hasWrite) {
    _dirty = false;
    _pendingCount = 0;
    _arenaUsed = 0;
    return TConfigStatus::Ok;
  }

  File oldFile;
  bool oldValid = _fileValid;
  if (oldValid) {
    oldFile = LittleFS.open(_path, "r");
    if (!oldFile) oldValid = false;
  }

  uint16_t oldCount = 0;
  uint32_t oldPayload = 0;
  if (oldValid && !readHeader(oldFile, &oldCount, &oldPayload)) {
    oldFile.close();
    oldValid = false;
  }

  LittleFS.remove(_tmpPath);
  File out = LittleFS.open(_tmpPath, "w");
  if (!out) {
    if (oldValid) oldFile.close();
    return TConfigStatus::IoError;
  }

  uint8_t zeroHeader[HEADER_SIZE];
  memset(zeroHeader, 0, sizeof(zeroHeader));
  if (out.write(zeroHeader, sizeof(zeroHeader)) != sizeof(zeroHeader)) {
    out.close();
    if (oldValid) oldFile.close();
    LittleFS.remove(_tmpPath);
    return TConfigStatus::IoError;
  }

  uint16_t recordCount = 0;
  uint32_t payloadSize = 0;

  if (oldValid) {
    for (uint16_t i = 0; i < oldCount; i++) {
      uint8_t header[RECORD_HEADER_SIZE];
      if (oldFile.read(header, sizeof(header)) != sizeof(header)) {
        oldFile.close();
        out.close();
        LittleFS.remove(_tmpPath);
        return TConfigStatus::Corrupt;
      }

      const char* key = (const char*)header;
      const uint16_t size = get16(header + 14);
      const uint32_t storedCrc = get32(header + 16);
      Pending* pending = findPending(key);

      if (pending) {
        pending->handled = true;
        if (!pending->remove) {
          if (pending->offset > _arenaUsed ||
              pending->offset + pending->size > _arenaUsed ||
              !writeRecord(out, pending->key, _arena + pending->offset, pending->size,
                           pending->version)) {
            oldFile.close();
            out.close();
            LittleFS.remove(_tmpPath);
            return TConfigStatus::Corrupt;
          }
          recordCount++;
          payloadSize += RECORD_HEADER_SIZE + pending->size;
        }
        if (!oldFile.seek(oldFile.position() + size)) {
          oldFile.close();
          out.close();
          LittleFS.remove(_tmpPath);
          return TConfigStatus::Corrupt;
        }
        continue;
      }

      const size_t dataPosition = oldFile.position();
      uint32_t crc = 0;
      if (!copyBytes(oldFile, nullptr, nullptr, 0, size, &crc)) {
        oldFile.close();
        out.close();
        LittleFS.remove(_tmpPath);
        return TConfigStatus::Corrupt;
      }

      if (crc != storedCrc) {
#if VUESP_LOG_LEVEL >= 1
        Serial.printf("[config] drop corrupt record %.12s\n", key);
#endif
        continue;
      }

      if (!oldFile.seek(dataPosition)) {
        oldFile.close();
        out.close();
        LittleFS.remove(_tmpPath);
        return TConfigStatus::Corrupt;
      }

      if (out.write(header, sizeof(header)) != sizeof(header)) {
        oldFile.close();
        out.close();
        LittleFS.remove(_tmpPath);
        return TConfigStatus::IoError;
      }

      if (!copyBytes(oldFile, &out, nullptr, 0, size, nullptr)) {
        oldFile.close();
        out.close();
        LittleFS.remove(_tmpPath);
        return TConfigStatus::IoError;
      }

      recordCount++;
      payloadSize += RECORD_HEADER_SIZE + size;
    }

    oldFile.close();
  }

  for (uint8_t i = 0; i < _pendingCount; i++) {
    Pending& pending = _pending[i];
    if (pending.handled || pending.remove) continue;
    if (pending.offset > _arenaUsed ||
        pending.offset + pending.size > _arenaUsed ||
        !writeRecord(out, pending.key, _arena + pending.offset, pending.size,
                     pending.version)) {
      out.close();
      LittleFS.remove(_tmpPath);
      return TConfigStatus::Corrupt;
    }
    recordCount++;
    payloadSize += RECORD_HEADER_SIZE + pending.size;
  }

  // Only removals on a fresh store: do not create an empty file.
  if (!oldValid && recordCount == 0) {
    out.close();
    LittleFS.remove(_tmpPath);
    _dirty = false;
    _pendingCount = 0;
    _arenaUsed = 0;
    return TConfigStatus::Ok;
  }

  uint8_t header[HEADER_SIZE];
  memset(header, 0, sizeof(header));
  memcpy(header, CONFIG_MAGIC, sizeof(CONFIG_MAGIC));
  header[4] = CONFIG_STORE_FORMAT_VERSION;
  header[5] = 0;
  put16(header + 6, recordCount);
  put32(header + 8, payloadSize);
  put32(header + 12, crc32(header, 12));

  if (!out.seek(0) || out.write(header, sizeof(header)) != sizeof(header)) {
    out.close();
    LittleFS.remove(_tmpPath);
    return TConfigStatus::IoError;
  }

  out.close();

  if (!LittleFS.rename(_tmpPath, _path)) {
    LittleFS.remove(_tmpPath);
    return TConfigStatus::IoError;
  }

  _filePresent = true;
  _fileValid = true;
  _dirty = false;
  _pendingCount = 0;
  _arenaUsed = 0;
  return TConfigStatus::Ok;
}
