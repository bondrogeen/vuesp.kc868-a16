#define CONFIG_START 0

#define CONFIG_VERSION 0

// Logging: 0 — silent, 1 — errors/boot, 2 — events and script lifecycle,
// 3 — per-port and per-data debug (hot path).
// Local builds default to full logging; CI passes -DVUESP_LOG_LEVEL=0.
#ifndef VUESP_LOG_LEVEL
#define VUESP_LOG_LEVEL 3
#endif

#define DEF_ETH_ENABLED 1
#define DEF_ETH_DHCP 1
#define DEF_ETH_IP \
  {192, 168, 23, 135}
#define DEF_ETH_SUBNET \
  {255, 255, 255, 0}
#define DEF_ETH_GETEWAY \
  {192, 168, 23, 1}
#define DEF_ETH_DNS \
  {192, 168, 23, 1}

// KC868-A16: LAN8720 RMII
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_POWER_PIN -1
#define ETH_TYPE ETH_PHY_LAN8720
#define ETH_ADDR 0
#define ETH_MDC_PIN 23
#define ETH_MDIO_PIN 18

// KC868-A16 I/O
#define GPIO_SDA 4
#define GPIO_SCL 5
#define PCF_INPUT_LOW 0x22
#define PCF_INPUT_HIGH 0x21
#define PCF_OUTPUT_LOW 0x24
#define PCF_OUTPUT_HIGH 0x25

#define RS485_TX_PIN 13
#define RS485_RX_PIN 16
#define RF_RX_PIN 2
#define RF_TX_PIN 15

#define GPIO_HT1 32
#define GPIO_HT2 33
#define PCF_INT_PIN 14
#define GPIO_ADC1 36
#define GPIO_ADC2 34
#define GPIO_ADC3 35
#define GPIO_ADC4 39

#define DEF_WIFI_DHCP 1
#define DEF_WIFI_IP \
  {192, 168, 1, 12}
#define DEF_WIFI_SUBNET \
  {255, 255, 255, 0}
#define DEF_WIFI_GETEWAY \
  {192, 168, 1, 1}
#define DEF_WIFI_DNS \
  {8, 8, 8, 8}

#define DEF_WIFI_MODE WIFI_AP
#define DEF_WIFI_SSID ""
#define DEF_WIFI_PASS ""

#define DEF_AUTH_MODE 1
#define DEF_AUTH_LOGIN "admin"
#define DEF_AUTH_PASS "admin"

#define DEF_DISCOVERY 1
#define DEF_DISCOVERY_INTERVAL 5
#define DEF_DISCOVERY_PORT 12345

#define DEF_HIDE_PASS "********"

#ifndef DEF_DEVICE_NAME
#define DEF_DEVICE_NAME "VUESP-"
#endif

#ifndef DEF_DEVICE_FIRMWARE
#define DEF_DEVICE_FIRMWARE {0, 0, 1}
#endif
#ifndef DEF_BOARD
#define DEF_BOARD 1
#endif