# Tasks

## 1. Baseline specs

- [x] 1.1 Описать `firmware-runtime`: boot, loop, сеть, HTTP/WS, OTA, recovery
- [x] 1.2 Описать `web-interface`: SPA, страницы, dashboard, темы, demo/dev, сборка
- [x] 1.3 Описать `device-profile`: default/dashboard/protocol/gpio/scripts/www

## 2. Validation

- [x] 2.1 Сверить маршруты, файлы и статусы с `src/` и `vue/`
- [x] 2.2 Прогнать `openspec validate establish-vuesp-baseline --strict`

## 3. Baseline

- [x] 3.1 Архивировать change и получить `openspec/specs/*`
- [x] 3.2 Убедиться, что спеки отражают текущее поведение без требований к новому
