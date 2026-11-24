# vuesp.kc868-a16

KinCony **KC868-A16**: 16 опто-входов, 16 MOSFET-выходов, 4 ADC, Ethernet, RS485 и локальный веб-интерфейс Vuesp.

Проект собран на базе [Vuesp](https://github.com/bondrogeen/vuesp) 5.0.x: прошивка **5.0.0**, web UI **5.0.0** на Vue 3 и `vuesp-components`.

## Возможности

- 16 входов и 16 выходов через PCF8574 по I2C;
- 4 аналоговых входа: GPIO36, 34, 35, 39;
- Ethernet LAN8720: DHCP или статический IP, настройка в отдельной карточке Settings;
- RS485 (MAX13487EESA) и Modbus RTU master/slave;
- 2 терминала HT1–HT2: GPIO32, 33; GPIO14 занят линией `nINT` PCF8574;
- 433 МГц: пины RX2/TX15 зарезервированы, поддержка будет добавлена позже;
- dashboard: 16 информационных карточек входов и 16 переключателей выходов, 4 ADC;
- ScriptRunner: `input()`, `output()`, `modbus()`, `udp()` и готовые примеры в Scripts;
- ConfigStore, диагностика загрузки/loop, OTA, файловая система.

## Документация

Подробное описание платы, распиновка, Ethernet, Modbus, ScriptRunner и ограничения — в [DOCS.md](./DOCS.md).

## Плата и окружение

- PlatformIO env: **esp32**
- Прошивка: `platformio.ini`
- Web UI: `vue/`
- Файловая система: LittleFS, веб-сборка попадает в `data/www`
- Настройки устройства и портов: `ConfigStore` (`/tmp/config.bin`)
- Настройки Wi-Fi/Ethernet: EEPROM (`Settings`)

## Сборка

```bash
cd vue
npm ci
npm run protocol
npm run build-web

cd ..
~/.platformio/penv/bin/pio run -e esp32
~/.platformio/penv/bin/pio run -e esp32 -t buildfs
```

Прошивка и файловая система:

```bash
~/.platformio/penv/bin/pio run -e esp32 -t upload
~/.platformio/penv/bin/pio run -e esp32 -t uploadfs
```

## Ethernet

По умолчанию Ethernet включён и использует **DHCP**. Статический режим и адреса задаются на странице **Settings → Ethernet**. После сохранения устройство перезагружается.

Аппаратная часть зафиксирована: LAN8720, MDC 23, MDIO 18, CLK 17, PHY address 0, POWER не используется.

## Ограничения

- Прошивка 5.0.0 — новая конфигурация; старые настройки не мигрируются.
- 433 МГц пока только пины/документация.
- ADC ESP32 нелинейный; для точных измерений нужна калибровка.
- Питание платы 12/24 В; логика 3.3 В.

## Репозиторий

<https://github.com/bondrogeen/vuesp.kc868-a16>
