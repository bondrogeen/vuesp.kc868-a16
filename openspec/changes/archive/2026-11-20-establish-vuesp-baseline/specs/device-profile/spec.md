# device-profile

## Purpose

Описывает контракт файлов профиля устройства в LittleFS: конфигурацию dashboard, меню, локали, протокол и пользовательские данные.

## ADDED Requirements

### Requirement: Default device profile

`default.json` SHALL содержать `dashboard`, `menu`, `locales` и `suggestions` и SHALL использоваться как базовый профиль, когда отдельный профиль устройства отсутствует или невалиден.

#### Scenario: Default profile is complete

- **WHEN** устройство отдаёт базовый профиль
- **THEN** в нём есть dashboard, menu, локали и suggestions

#### Scenario: Default profile is the fallback

- **WHEN** `dashboard.json` отсутствует или не может быть разобран
- **THEN** интерфейс использует `default.json`

### Requirement: Per-device dashboard override

`dashboard.json` SHALL переопределять базовый dashboard и состоять из списка плиток с уникальными `id`, полями `name`, `type`, `args` и опциональными `icon`, `opts`, `get`, `getTo`, `set`.

#### Scenario: Override wins over default

- **WHEN** `dashboard.json` существует и валиден
- **THEN** интерфейс показывает плитки из него

#### Scenario: Duplicate ids are rejected

- **WHEN** в `dashboard.json` встречаются плитки с одинаковым `id`
- **THEN** профиль считается невалидным и применяется базовый dashboard

### Requirement: Protocol description

`protocol.json` SHALL описывать `enums` и `structs` бинарного протокола устройства и SHALL поставляться вместе с web UI; generated-файлы прошивки и web UI SHALL соответствовать этому описанию.

#### Scenario: Protocol matches sources

- **WHEN** выполняется проверка generated-файлов протокола
- **THEN** расхождение с источниками обнаруживается как ошибка

#### Scenario: Consumers use the published protocol

- **WHEN** клиент читает `protocol.json` с устройства
- **THEN** он получает enums и structs, достаточные для декодирования сообщений

### Requirement: GPIO configuration

`gpio.json` SHALL хранить конфигурацию портов устройства и SHALL сохраняться между перезагрузками.

#### Scenario: Port configuration survives reboot

- **WHEN** конфигурация GPIO сохранена и устройство перезагружено
- **THEN** порты восстанавливаются из `gpio.json`

### Requirement: Scripts and user files

Пользовательские скрипты SHALL храниться в `scripts.txt` и/или `scripts/`, а файловые операции через `/fs` SHALL работать только внутри LittleFS устройства.

#### Scenario: Scripts persist

- **WHEN** пользователь сохраняет скрипт и перезагружает устройство
- **THEN** скрипт остаётся доступным

#### Scenario: Files stay inside LittleFS

- **WHEN** выполняется файловая операция через `/fs`
- **THEN** она ограничена файловой системой устройства

### Requirement: Web assets and recovery

`www/` SHALL содержать собранный web UI; отсутствие `www/index.html` SHALL переводить устройство в режим recovery без потери профиля и пользовательских файлов.

#### Scenario: UI assets are present

- **WHEN** устройство прошито с web UI
- **THEN** `www/` содержит index и assets

#### Scenario: Recovery preserves data

- **WHEN** index отсутствует и открыта recovery-страница
- **THEN** конфигурация и пользовательские файлы остаются на месте
