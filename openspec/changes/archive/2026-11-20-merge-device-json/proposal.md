# Proposal

## Why

`protocol.json` и `default.json` дублируют доставку данных на устройство и
занимают два блока LittleFS. При блоке 8 КБ это 16 КБ вместо 8 КБ. Обратная
совместимость не требуется: прошивка и парк устройств стартуют с нуля.

## What Changes

- `protocol.json` больше не генерируется и не поставляется.
- `default.json` содержит `struct` вместе с `dashboard`, `menu`, `locales`,
  `suggestions`.
- Hub читает протокол из `default.json.struct`.
- **BREAKING**: устройства, ожидающие отдельный `protocol.json`, не
  поддерживаются.

## Capabilities

### New Capabilities

Нет.

### Modified Capabilities

- `device-profile`: протокол переезжает в `default.json.struct`.
- `web-interface`: сборка больше не создаёт `protocol.json`.

## Impact

- `vuesp`: `vue/scripts/protocol-and-json.js`, `vue/public/*`, `data/www`.
- `vuesp-hub`: `VuespDevice.onInit`, тесты и архитектурная документация.
- OpenSpec-спеки `vuesp`.
