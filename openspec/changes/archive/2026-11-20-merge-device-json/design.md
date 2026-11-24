# Design

## Approach

Один JSON-файл на устройство. `default.json` остаётся точкой входа web UI и
дополнительно несёт `struct`; hub берёт его оттуда же.

## Decisions

1. **Ключ `struct`.** Совпадает с device-проектами и текущим `/default.json`.
2. **Без fallback.** `protocol.json` удаляется сразу; старые прошивки не
   поддерживаются.
3. **Генерация.** `protocol-and-json.js` пишет `default.json` и удаляет
   устаревший `public/protocol.json`, а `--check` считает его наличие ошибкой.

## Non-goals

- Совместимость со старыми прошивками и hub’ами.
- Переименование `default.json` в `device.json`.
