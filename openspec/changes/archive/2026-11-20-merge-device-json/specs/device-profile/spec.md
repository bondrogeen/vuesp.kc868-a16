# device-profile

## MODIFIED Requirements

### Requirement: Protocol description

`default.json` SHALL содержать `struct` с `enums` и `structs` бинарного
протокола устройства; отдельный `protocol.json` SHALL NOT создаваться.
Generated-файлы прошивки и web UI SHALL соответствовать этому описанию.

#### Scenario: Protocol matches sources

- **WHEN** выполняется проверка generated-файлов протокола
- **THEN** расхождение с источниками обнаруживается как ошибка

#### Scenario: Consumers use the published protocol

- **WHEN** клиент читает `default.json` с устройства
- **THEN** он получает `struct` с enums и structs, достаточными для
  декодирования сообщений
