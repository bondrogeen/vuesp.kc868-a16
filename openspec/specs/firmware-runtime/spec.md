# firmware-runtime Specification

## Purpose

Описывает запуск и работу прошивки Vuesp на поддерживаемых платах: инициализацию, сеть, HTTP/WebSocket-транспорт, обновление прошивки и восстановление.

## Requirements

### Requirement: Boot and initialization

Прошивка SHALL инициализировать LittleFS и EEPROM при старте, выводить стабильные `id` и имя устройства, применять значения по умолчанию при первом запуске и запускать сеть, GPIO, device runtime, HTTP/WebSocket, discovery и ScriptRunner до входа в основной цикл.

#### Scenario: First boot applies defaults

- **WHEN** на устройстве нет сохранённой конфигурации
- **THEN** прошивка создаёт конфигурацию по умолчанию и запускает подсистемы

#### Scenario: Saved configuration is restored

- **WHEN** в LittleFS и EEPROM есть сохранённые настройки
- **THEN** прошивка загружает их и использует при старте

### Requirement: Non-blocking main loop

После инициализации прошивка SHALL обслуживать discovery, HTTP/WebSocket, задачи, GPIO, device runtime, WiFi, ScriptRunner и serial setup на каждой итерации основного цикла, не блокируя цикл длительными операциями.

#### Scenario: Subsystems are serviced continuously

- **WHEN** устройство работает в основном цикле
- **THEN** перечисленные подсистемы получают управление на каждой итерации

#### Scenario: Uptime advances once per second

- **WHEN** устройство работает непрерывно
- **THEN** счётчик uptime увеличивается примерно раз в секунду

### Requirement: Network modes and provisioning

Устройство SHALL работать в режиме WiFi, заданном настройками (`STA`, `AP` или `AP_STA`), и SHALL принимать настройки WiFi по serial-протоколу `VUESP:PING`, `VUESP:SCAN`, `VUESP:SET`.

#### Scenario: Serial ping responds

- **WHEN** на serial-порт приходит `VUESP:PING`
- **THEN** устройство отвечает `@VUESP:PONG`

#### Scenario: Serial scan lists networks

- **WHEN** приходит `VUESP:SCAN`
- **THEN** устройство печатает `@VUESP:SCAN:BEGIN`, строки `@VUESP:WIFI:<ssid>\t<rssi>\t<secure>` и `@VUESP:SCAN:END`

#### Scenario: Serial setup validates credentials

- **WHEN** приходит `VUESP:SET` с SSID длиннее 31 символа или паролем длиннее 31 символа
- **THEN** устройство отвечает `@VUESP:SET:ERROR` и не сохраняет настройки

#### Scenario: Valid serial setup reboots

- **WHEN** приходит `VUESP:SET` с корректными SSID и паролем
- **THEN** устройство сохраняет настройки, отвечает `@VUESP:SET:OK` и перезагружается

### Requirement: HTTP endpoints and access control

Устройство SHALL отдавать SPA по `/`, статику из `/www`, файловые операции и OTA по `/fs` и `/update`, выполнение скриптов по `/cmd`, recovery-страницу по `/recovery`. При включённой аутентификации все HTTP-маршруты SHALL требовать HTTP Basic credentials.

#### Scenario: Authorization is enforced

- **WHEN** аутентификация включена и запрос приходит без валидных Basic credentials
- **THEN** устройство отвечает отказом авторизации

#### Scenario: Recovery when web UI is missing

- **WHEN** `/www/index.html` отсутствует и запрошен `/`
- **THEN** устройство перенаправляет на `/recovery`

#### Scenario: Recovery can be forced

- **WHEN** запрошен `/recovery?force`
- **THEN** устройство отдаёт встроенную recovery-страницу даже при наличии web UI

### Requirement: Script command validation

`/cmd` SHALL требовать параметры `id` и `script`, отклонять `id = 0` и скрипты длиной 255 символов и более, а при корректных параметрах — выполнять скрипт устройством.

#### Scenario: Invalid command is rejected

- **WHEN** запрос к `/cmd` не содержит `id` или `script`, либо `id = 0`
- **THEN** устройство отвечает статусом 400

#### Scenario: Valid command is accepted

- **WHEN** запрос к `/cmd` содержит положительный `id` и скрипт короче 255 символов
- **THEN** устройство принимает скрипт к выполнению

### Requirement: Firmware and filesystem updates

`/update` SHALL принимать образ прошивки и `littlefs.bin`, сообщать прогресс и перезагружать устройство после успешной записи; при ошибке SHALL возвращать ошибку и не нарушать работу текущего образа.

#### Scenario: Successful update reboots

- **WHEN** образ прошивки или `littlefs.bin` записан без ошибок
- **THEN** устройство отвечает успешным статусом и перезагружается

#### Scenario: Failed update keeps the device running

- **WHEN** запись образа завершается ошибкой
- **THEN** устройство возвращает ошибку и продолжает работу текущей прошивки

### Requirement: WebSocket transport

Устройство SHALL принимать бинарные сообщения на `/esp`, отдавать состояние устройства и принимать команды, отправлять heartbeat и прогресс при подключённых клиентах и освобождать отключившихся клиентов.

#### Scenario: Client receives state after connect

- **WHEN** клиент подключается к `/esp`
- **THEN** устройство отправляет начальное состояние и продолжает обновлять его при изменениях

#### Scenario: Heartbeat is emitted while clients are connected

- **WHEN** к устройству подключён хотя бы один WebSocket-клиент
- **THEN** устройство периодически отправляет heartbeat, а во время обновления — прогресс

#### Scenario: Disconnected clients are cleaned up

- **WHEN** WebSocket-клиент отключается
- **THEN** устройство освобождает его, не прерывая работу остальных клиентов
