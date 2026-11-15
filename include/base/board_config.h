#pragma once

// =============================================================================
// ESP32-Base — Configuración de board (defaults con #ifndef)
// =============================================================================
// La base no impone hardware. Si el proyecto consumidor define su propio
// "hw/HwProfile.hpp" (identidad de board, pines, features) y/o "core/Version.hpp"
// (macros de versión), se incluyen y sus macros ganan. Si no existen, se aplican
// estos defaults neutros (todo feature deshabilitado, versión 0.0.0).
//
//   -D SEMA_USE_*  /  BOARD_*  en build_flags siguen funcionando igual que antes.
// =============================================================================

#if defined(__has_include)
#  if __has_include("hw/HwProfile.hpp")
#    include "hw/HwProfile.hpp"
#  endif
#  if __has_include("core/Version.hpp")
#    include "core/Version.hpp"
#  endif
#  if __has_include("sdkconfig.h")
#    include "sdkconfig.h"  // CONFIG_IDF_TARGET_ESP32* (detección de chip)
#  endif
#endif

// --- Identidad de la board ---
#ifndef SEMA_BOARD_ID
#define SEMA_BOARD_ID "base-unknown"
#endif
#ifndef SEMA_FLASH_MB
#define SEMA_FLASH_MB 4
#endif
#ifndef SEMA_NATIVE_ETH
// Fallback sin perfil de proyecto: detectar el chip. CONFIG_IDF_TARGET_ESP32* lo
// define sdkconfig.h (incluido arriba vía __has_include). El ESP32 clásico es el
// único con EMAC nativa; S2/S3/C3/C6/H2/P4 usan W5500 por SPI.
#if defined(CONFIG_IDF_TARGET_ESP32)
#define SEMA_NATIVE_ETH 1
#else
#define SEMA_NATIVE_ETH 0
#endif
#endif

#ifndef SEMA_ON
#define SEMA_ON 1
#endif
#ifndef SEMA_OFF
#define SEMA_OFF 0
#endif

// --- Features (por defecto deshabilitadas en la base; el proyecto las activa) ---
#ifndef SEMA_USE_ETHERNET
#define SEMA_USE_ETHERNET SEMA_OFF
#endif
#ifndef SEMA_USE_LORA
#define SEMA_USE_LORA SEMA_OFF
#endif
#ifndef SEMA_USE_MODBUS
#define SEMA_USE_MODBUS SEMA_OFF
#endif
#ifndef SEMA_USE_CAN
#define SEMA_USE_CAN SEMA_OFF
#endif
#ifndef SEMA_USE_ZIGBEE
#define SEMA_USE_ZIGBEE SEMA_OFF
#endif
#ifndef SEMA_USE_SHIFT
#define SEMA_USE_SHIFT SEMA_OFF
#endif
#ifndef SEMA_USE_MCP23S17
#define SEMA_USE_MCP23S17 SEMA_OFF
#endif
#ifndef SEMA_USE_PCA9685
#define SEMA_USE_PCA9685 SEMA_OFF
#endif
#ifndef SEMA_USE_PCF8574
#define SEMA_USE_PCF8574 SEMA_OFF
#endif

// --- Transceiver RS485 ---
#ifndef SEMA_MODBUS_ISOLATED
#define SEMA_MODBUS_ISOLATED SEMA_ON
#endif
#ifndef SEMA_MODBUS_TRANSCEIVER
#define SEMA_MODBUS_TRANSCEIVER "generic"
#endif

// --- Origen de pines / demo ---
#ifndef SEMA_PINS_FROM_FILE
#define SEMA_PINS_FROM_FILE SEMA_OFF
#endif
#ifndef SEMA_DEMO
#define SEMA_DEMO SEMA_OFF
#endif

// --- Bus SPI (Arduino) ---
#ifndef SEMA_SPI_SCK
#define SEMA_SPI_SCK 18
#endif
#ifndef SEMA_SPI_MISO
#define SEMA_SPI_MISO 19
#endif
#ifndef SEMA_SPI_MOSI
#define SEMA_SPI_MOSI 23
#endif
#ifndef SEMA_SPI_CS
#define SEMA_SPI_CS 5
#endif

// --- Chip-select por dispositivo ---
#ifndef SEMA_CS_LORA
#define SEMA_CS_LORA 10
#endif
#ifndef SEMA_CS_ETHERNET_W5500
#define SEMA_CS_ETHERNET_W5500 5
#endif
#ifndef SEMA_PIN_SD_CS
#define SEMA_PIN_SD_CS 4
#endif

// --- W5500 (driver ESP-IDF, SPI dedicado) ---
#ifndef SEMA_ETH_SPI_HOST
#define SEMA_ETH_SPI_HOST 2
#endif
#ifndef SEMA_ETH_SPI_SCK
#define SEMA_ETH_SPI_SCK 18
#endif
#ifndef SEMA_ETH_SPI_MISO
#define SEMA_ETH_SPI_MISO 19
#endif
#ifndef SEMA_ETH_SPI_MOSI
#define SEMA_ETH_SPI_MOSI 21
#endif

// --- Pines fijos (solo SEMA_PINS_FROM_FILE == 1) ---
#ifndef SEMA_PIN_CAN_TX
#define SEMA_PIN_CAN_TX 5
#endif
#ifndef SEMA_PIN_CAN_RX
#define SEMA_PIN_CAN_RX 4
#endif
#ifndef SEMA_PIN_MODBUS_RX
#define SEMA_PIN_MODBUS_RX 16
#endif
#ifndef SEMA_PIN_MODBUS_TX
#define SEMA_PIN_MODBUS_TX 17
#endif
#ifndef SEMA_PIN_MODBUS_DERE
#define SEMA_PIN_MODBUS_DERE 0
#endif
#ifndef SEMA_PIN_ZIGBEE_RX
#define SEMA_PIN_ZIGBEE_RX 18
#endif
#ifndef SEMA_PIN_ZIGBEE_TX
#define SEMA_PIN_ZIGBEE_TX 19
#endif
#ifndef SEMA_PIN_LORA_RST
#define SEMA_PIN_LORA_RST 32
#endif
#ifndef SEMA_PIN_LORA_DIO1
#define SEMA_PIN_LORA_DIO1 26
#endif
#ifndef SEMA_PIN_LORA_BUSY
#define SEMA_PIN_LORA_BUSY 27
#endif

// --- Ethernet LAN8720A (RMII) ---
#ifndef SEMA_PIN_ETH_MDC
#define SEMA_PIN_ETH_MDC 23
#endif
#ifndef SEMA_PIN_ETH_MDIO
#define SEMA_PIN_ETH_MDIO 18
#endif
#ifndef SEMA_PIN_ETH_PHY_ADDR
#define SEMA_PIN_ETH_PHY_ADDR 1
#endif
#ifndef SEMA_PIN_ETH_POWER
#define SEMA_PIN_ETH_POWER -1
#endif
#ifndef SEMA_PIN_ETH_TXD0
#define SEMA_PIN_ETH_TXD0 19
#endif
#ifndef SEMA_PIN_ETH_TXD1
#define SEMA_PIN_ETH_TXD1 22
#endif
#ifndef SEMA_PIN_ETH_TX_EN
#define SEMA_PIN_ETH_TX_EN 21
#endif
#ifndef SEMA_PIN_ETH_RXD0
#define SEMA_PIN_ETH_RXD0 25
#endif
#ifndef SEMA_PIN_ETH_RXD1
#define SEMA_PIN_ETH_RXD1 26
#endif
#ifndef SEMA_PIN_ETH_CRS_DV
#define SEMA_PIN_ETH_CRS_DV 27
#endif
#ifndef SEMA_PIN_ETH_RX_ER
#define SEMA_PIN_ETH_RX_ER 13
#endif
#ifndef SEMA_PIN_ETH_REF_CLK
#define SEMA_PIN_ETH_REF_CLK 0
#endif
#ifndef SEMA_PIN_ETH_W5500_RST
#define SEMA_PIN_ETH_W5500_RST -1
#endif
#ifndef SEMA_PIN_ETH_W5500_IRQ
#define SEMA_PIN_ETH_W5500_IRQ 4
#endif

// --- Versión (se sobrescribe con core/Version.hpp si el proyecto lo define) ---
#ifndef SEMA_FW_VERSION
#define SEMA_FW_VERSION "0.0.0"
#endif
#ifndef SEMA_HW_VERSION
#define SEMA_HW_VERSION "rev0"
#endif
#ifndef SEMA_CONFIG_SCHEMA_VERSION
#define SEMA_CONFIG_SCHEMA_VERSION 1
#endif
#ifndef SEMA_PROTOCOL_VERSION
#define SEMA_PROTOCOL_VERSION 1
#endif
