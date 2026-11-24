# Proposal

## Why

У `vuesp` есть подробные README/DOCS и работающая прошивка, но нет baseline-спек
OpenSpec. Из-за этого локальные изменения не могут ссылаться на стабильные
контракты, а кросс-репозиторные задачи (hub, UI, device-проекты) не имеют
якоря в этом репозитории.

## What Changes

- Добавляются три baseline-спеки, описывающие текущее наблюдаемое поведение:
  - `firmware-runtime` — запуск, конфигурация, сеть, HTTP/WS-транспорт, OTA;
  - `web-interface` — встроенный Vue UI, страницы, dashboard, темы, demo/dev;
  - `device-profile` — JSON-файлы профиля устройства в LittleFS.
- Код прошивки и web UI не меняется.
- Общие кросс-репозиторные контракты (протокол, dashboard schema) остаются
  в referenced store `vuesp-workspace`; здесь фиксируется только локальное
  поведение и реализация.

## Capabilities

### New Capabilities

- `firmware-runtime`: запуск и инициализация устройства, конфигурация и
  идентичность, сеть, HTTP/WebSocket-транспорт, OTA и recovery.
- `web-interface`: встроенное Vue-приложение, страницы и навигация, dashboard,
  темы, demo/dev-режимы, сборка.
- `device-profile`: контракт файлов LittleFS (`default.json`, `dashboard.json`,
  `protocol.json`, `gpio.json`, `scripts.txt`, `www/`).

### Modified Capabilities

Нет: существующих спек в репозитории ещё нет.

## Impact

- Только `openspec/`: новые спеки и этот change.
- Прошивка, web UI, CI и артефакты не затрагиваются.
- После архивации спеки станут доступны для ссылок из локальных changes.
