# web-interface

## Purpose

Описывает встроенный Vue-интерфейс Vuesp: отдачу SPA с устройства, страницы, dashboard, темы, demo/dev-режимы и сборку.

## ADDED Requirements

### Requirement: SPA delivery from LittleFS

Устройство SHALL отдавать `www/index.html` по `/` и статические файлы из `www/`; при отсутствии index SHALL перенаправлять на recovery-страницу. Хешированные файлы из `/assets/` SHALL кешироваться надолго, а index — без кеша.

#### Scenario: SPA is served when installed

- **WHEN** `www/index.html` существует и запрошен `/`
- **THEN** устройство отдаёт SPA с заголовками без кеша

#### Scenario: Assets are cached

- **WHEN** запрошен файл из `/assets/`
- **THEN** устройство отдаёт его с долгим immutable-кешем

#### Scenario: Missing SPA falls back to recovery

- **WHEN** `www/index.html` отсутствует
- **THEN** запрос `/` перенаправляется на `/recovery`

### Requirement: Application routes

Интерфейс SHALL содержать маршруты главной страницы, dashboard, страниц устройства и редактора скриптов; неизвестный путь SHALL отдавать SPA, а маршрутизация в demo-режиме SHALL использовать hash history.

#### Scenario: Known pages open

- **WHEN** пользователь открывает главную, dashboard, страницу устройства или скрипты
- **THEN** отображается соответствующая страница

#### Scenario: Unknown path renders the app

- **WHEN** открыт неизвестный путь
- **THEN** устройство отдаёт SPA, а клиентский роутер показывает страницу-заглушку

### Requirement: Dashboard rendering and control

Dashboard SHALL строить плитки из конфигурации устройства, вычислять отображение через `get`/`getTo` и отправлять изменения через WebSocket-команды. Поддерживаемые типы плиток: `button`, `dimmer`, `info`, `date`, `list`, `input`, `chart`.

#### Scenario: Tile reflects device state

- **WHEN** приходит обновление состояния устройства
- **THEN** соответствующая плитка пересчитывает отображение

#### Scenario: User changes a value

- **WHEN** пользователь нажимает кнопку или меняет значение плитки
- **THEN** интерфейс отправляет команду устройству через WebSocket

#### Scenario: Broken expression does not hide other tiles

- **WHEN** выражение одной плитки завершается ошибкой
- **THEN** остальные плитки продолжают отображаться

### Requirement: Theme and layout

Интерфейс SHALL поддерживать светлую и тёмную темы через `vuesp-components`, сохранять выбор темы и оставаться адаптивным на мобильных и настольных экранах.

#### Scenario: Theme persists

- **WHEN** пользователь переключает тему и перезагружает страницу
- **THEN** выбранная тема сохраняется

#### Scenario: Narrow screen stays usable

- **WHEN** страница открыта на узком экране
- **THEN** навигация и плитки остаются доступными без горизонтального переполнения

### Requirement: Demo and development modes

Сборка SHALL поддерживать demo-режим с mock-устройством и hash-маршрутизацией, а dev-сервер SHALL проксировать запросы к устройству по `VITE_IP`.

#### Scenario: Demo runs without hardware

- **WHEN** собрано demo-приложение
- **THEN** интерфейс работает на mock-данных без реального устройства

#### Scenario: Dev server targets a device

- **WHEN** задан `VITE_IP` и запущен dev-сервер
- **THEN** API-запросы проксируются на указанное устройство

### Requirement: Build outputs

Сборка web UI SHALL складывать результат в `data/www`, а скрипты сборки SHALL обновлять generated-структуры, recovery-страницу и `protocol.json`; проверка `protocol:check` SHALL обнаруживать расхождение generated-файлов с источниками.

#### Scenario: Build produces deployable files

- **WHEN** выполняется `npm run build-web`
- **THEN** готовые файлы появляются в `data/www`

#### Scenario: Protocol drift is detected

- **WHEN** generated-файлы протокола не совпадают с источниками
- **THEN** `protocol:check` завершается с ошибкой
