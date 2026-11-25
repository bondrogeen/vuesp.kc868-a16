# KC868-A16 — 16 входов, 16 MOSFET-выходов, Ethernet и Modbus

**Коротко:** `vuesp.kc868-a16` — прошивка Vuesp **5.0.0** для платы **KinCony KC868-A16** на ESP32-WROOM-32. 16 опто-входов и 16 MOSFET-выходов подключены через PCF8574 по I2C, 4 аналоговых входа читаются напрямую. Есть Ethernet LAN8720, RS485 с Modbus RTU и локальный веб-интерфейс.

## Что это за устройство

- ESP32-WROOM-32, Wi-Fi 2.4 ГГц, 4 МБ flash, без PSRAM;
- 16 цифровых оптоизолированных входов через PCF8574;
- 16 MOSFET-выходов 12/24 В через PCF8574 (не реле; релейный блок KC868-E16 подключается внешне);
- 4 аналоговых входа 0–5 В: GPIO36, 34, 35, 39;
- Ethernet LAN8720A;
- RS485 MAX13487EESA с автонаправлением;
- 433 МГц: разъёмы RX2/TX15 (поддержка позже);
- HT1–HT2: GPIO32, 33; GPIO14 — вход прерывания `nINT` PCF8574;
- USB Type-C, CH340C, кнопки RESET и DOWNLOAD;
- питание 12/24 В.

## Характеристики

| Параметр | Значение |
| --- | --- |
| MCU | ESP32-WROOM-32, 240 МГц |
| Flash / SRAM | 4 МБ / 520 КБ, PSRAM нет |
| Входы | 16, оптопары, PCF8574 |
| Выходы | 16 MOSFET, PCF8574 |
| ADC | 4 канала: 36, 34, 35, 39 |
| I2C | SDA 4, SCL 5 |
| Ethernet | LAN8720: MDC 23, MDIO 18, CLK 17, PHY 0 |
| RS485 | MAX13487EESA: TX 13, RX 16 |
| 433 МГц | RX 2, TX 15 |
| HT1–HT2 | 32, 33 |
| Среда | PlatformIO + Arduino |

## Распиновка

### I2C и PCF8574

| Сигнал | GPIO | Адрес | Назначение |
| --- | --- | --- | --- |
| SDA | 4 | — | I2C data |
| SCL | 5 | — | I2C clock |
| IN1–IN8 | — | `0x22` | входы 1–8 |
| IN9–IN16 | — | `0x21` | входы 9–16 |
| OUT1–OUT8 | — | `0x24` | выходы 1–8 |
| OUT9–OUT16 | — | `0x25` | выходы 9–16 |

Порядок `0x21`/`0x22` и полярность входов/выходов зависят от ревизии — проверяйте на конкретной плате. В прошивке входы и выходы хранятся как логические маски, где `1` = активен/включён; наружу PCF8574 работает активным низким уровнем.

### Аналоговые входы

| Вход | GPIO |
| --- | --- |
| AI1 | 36 |
| AI2 | 34 |
| AI3 | 35 |
| AI4 | 39 |

### Ethernet LAN8720

| Сигнал | GPIO |
| --- | --- |
| MDC | 23 |
| MDIO | 18 |
| CLK | 17 (`GPIO17_OUT`) |
| PHY address | 0 |
| POWER | не используется (`-1`) |

### RS485 и 433 МГц

| Интерфейс | GPIO |
| --- | --- |
| RS485 TX | 13 |
| RS485 RX | 16 |
| 433 RX | 2 |
| 433 TX | 15 |

RS485 использует MAX13487EESA с автонаправлением, отдельный DE/RE не нужен.

### HT1–HT2 и линия прерывания

| Терминал | GPIO |
| --- | --- |
| HT1 | 32 |
| HT2 | 33 |
| PCF8574 `nINT` | 14 |

HT1–HT2 — входы с подтяжкой. HT3/GPIO14 отдан под `nINT` входных PCF8574; его нельзя использовать как обычный порт.

### Прерывание PCF8574

Входные PCF8574 (`0x21`, `0x22`) имеют выход `nINT` (контакт 13). На A16 он **не разведён** на ESP32, поэтому для работы по прерыванию нужно:

- соединить `nINT` входных PCF8574;
- подтянуть линию к 3.3 В через 10 кОм (если подтяжки нет);
- завести линию на **GPIO14/HT3**;
- в прошивке GPIO14 настроен как `INPUT_PULLUP`, прерывание по спаду.

Если провод не подключён, входы продолжают работать через резервный polling раз в 1 секунду. При подключённой линии чтение PCF выполняется сразу по событию.

## Как устроен проект

- `src/const/protocol.h` — протокол, `Settings`, `Device`, Modbus-структуры;
- `src/config/ConfigStore.*` — настройки портов/устройства/Modbus;
- `src/device/device.cpp` — PCF8574, 16 входов/выходов, ADC, прерывание `nINT` на GPIO14, `input()`/`output()`;
- `src/gpio/gpio.cpp` — HT1–HT2 и ADC как порты Vuesp;
- `src/modbus/*` — Modbus RTU master/slave;
- `src/webserver/webserver.cpp` — HTTP/WebSocket, Ethernet;
- `src/init/init.cpp` — Wi-Fi, Ethernet, Settings;
- `vue/` — веб-интерфейс.

## Веб-интерфейс

- `/` — главная;
- `/dashboard/` — 16 информационных входов, 16 переключателей выходов, 4 ADC, Ethernet и HT1–HT2;
- `/device/gpio/` — HT1–HT2 и ADC;
- `/device/modbus/` — Modbus RTU master/slave;
- `/device/settings/` — Wi-Fi, безопасность, **Ethernet**, discovery, диагностика;
- `/device/storage/` — LittleFS;
- `/script/` — ScriptRunner.

### Ethernet

Ethernet настраивается отдельной карточкой на `/device/settings/`:

- включение/выключение;
- **DHCP по умолчанию**;
- статический IP, маска, шлюз, DNS;
- статус link, скорость, текущий IP;
- сохранение с перезагрузкой.

Настройки Ethernet хранятся в `Settings` вместе с Wi-Fi. `SETTINGS_VERSION` обновлён, поэтому после первой прошивки настройки создаются заново.

### Modbus RTU

- master/slave на RS485 (TX13/RX16);
- slave address по умолчанию `1`, 9600 8N1;
- входы: discrete inputs 0–15;
- выходы: coils 0–15;
- holding registers:
  - `0` — маска входов;
  - `1` — маска выходов;
  - `2–5` — ADC1–ADC4;
  - `8–10` — версия прошивки;
  - `11–12` — uptime;
  - `13` — адрес;
  - `14` — baud index;
  - `15` — роль.
- master работает по профилю устройств/точек из веб-интерфейса.

## ScriptRunner

Дополнительно к стандартным возможностям:

- `input(n)` — вход 1–16, `1` = активен;
- `output(n, value)` — выход 1–16, `1` = включён;
- `modbus(...)` — операции Modbus master;
- `udp(deviceId, text)` — отправить текст другому устройству через Discovery;
- события входов: `port_c`, `port_l`, `port_r` (`$e0` — индекс 0–15).

### Примеры скриптов

Стартовый набор лежит в `vue/public/scripts.txt` и после `build-web` попадает на устройство как `/www/scripts.txt`:

| ID | Имя | Что делает |
| ---: | --- | --- |
| 0 | `boot` | пишет приветствие в лог и регистрирует `input_event` |
| 1 | `output1` | включает OUT1 на 1 секунду и выключает |
| 2 | `output_walk` | поочерёдно включает все 16 выходов по 500 мс |
| 3 | `input_scan` | выводит в лог состояние всех 16 входов |
| 4 | `input_event` | логирует `port_c`: индекс входа, значение и счётчик |
| 5 | `input_toggle` | переключает OUT1 коротким нажатием на IN1 |

`boot` не включает выходы: скрипты 1, 2 и 5 запускаются вручную из редактора. Полярность логическая: `input()` возвращает `1` для активного входа, `output(..., 1)` включает выход.

Подсказки для редактора: `input`, `output`, `var`, `modbus` и события `port_c/port_l/port_r` лежат в `vue/json/suggestions.json`. Глобальные `fade()`, `udp()` и `http()` приходят из `vuesp-components` и в проекте не дублируются.

## Первичная настройка по USB

Если устройство ещё не в сети, его можно настроить через USB-serial (115200):

| Команда | Действие |
| --- | --- |
| `VUESP:PING` | проверить связь (`@VUESP:PONG`) |
| `VUESP:INFO` | `@VUESP:INFO:<имя>,<плата>,<id>,<ip STA>,<ip AP>` |
| `VUESP:SCAN` | список Wi-Fi сетей |
| `VUESP:SET<TAB>ssid<TAB>pass` | подключить Wi-Fi STA и перезагрузить |
| `VUESP:ETH` | link/скорость/текущий IP Ethernet |
| `VUESP:ETHSET<TAB>ip<TAB>gateway<TAB>mask` | задать статический Ethernet и перезагрузить |
| `VUESP:ETHDHCP` | вернуть Ethernet на DHCP |
| `VUESP:OUT<TAB>n<TAB>0/1` | включить/выключить выход n |

После `VUESP:ETHSET` или `VUESP:ETHDHCP` устройство перезагружается.

## Сборка и прошивка

```bash
cd vue
npm ci
npm run protocol
npm run build-web

cd ..
~/.platformio/penv/bin/pio run -e esp32
~/.platformio/penv/bin/pio run -e esp32 -t buildfs
```

Заливка:

```bash
~/.platformio/penv/bin/pio run -e esp32 -t upload
~/.platformio/penv/bin/pio run -e esp32 -t uploadfs
```

OTA: прошивка через `/update`, веб-файлы через `/fs` или полный `littlefs.bin`.

## Ограничения

- Нет PSRAM; приложение занимает около 80% слота 1.25 МБ.
- 433 МГц пока не обрабатывается в UI/скриптах — только пины.
- ADC ESP32 нелинейный и шумит; для точных измерений нужна калибровка.
- PCF8574 и внешние I2C-модули должны иметь разные адреса.
- MOSFET-выходы рассчитаны на определённый ток; паспорт платы важнее общего правила.
- Старые настройки A16 4.x не мигрируются: версия Settings новая.

## Источники

- [KinCony KC868-A16 — схема](https://www.kincony.com/download/KC868-A16-schematic.pdf)
- [KinCony KC868-A16 — описание платы](https://www.kincony.com/esp32-board-16-channel-relay-hardware.html)
- [ESPHome KC868-A16 pinout](https://devices.esphome.io/devices/kincony-kc868-a16)
- [PCF8574 datasheet](https://www.ti.com/lit/ds/symlink/pcf8574.pdf)
- [ESP32 Ethernet LAN8720](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_eth.html)
- [Документация Vuesp](https://docs.vuesp.ru)
