# web-interface

## MODIFIED Requirements

### Requirement: Build outputs

Сборка web UI SHALL складывать результат в `data/www`, а скрипты сборки SHALL
обновлять generated-структуры и recovery-страницу; `protocol:check` SHALL
обнаруживать расхождение generated-файлов с источниками и SHALL считать
наличие `protocol.json` ошибкой.

#### Scenario: Build produces deployable files

- **WHEN** выполняется `npm run build-web`
- **THEN** готовые файлы появляются в `data/www`, а `protocol.json` отсутствует

#### Scenario: Protocol drift is detected

- **WHEN** generated-файлы протокола не совпадают с источниками
- **THEN** `protocol:check` завершается с ошибкой
