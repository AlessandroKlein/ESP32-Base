#pragma once
// chip.hpp — Detección compile-time del SoC ESP32.
//
// Mapea CONFIG_IDF_TARGET_ESP32* (definido por el sistema de build de ESP-IDF
// en sdkconfig.h, disponible en toda unidad vía Arduino.h) a identidad
// (BASE_CHIP_NAME) y capacidades (BASE_HAVE_*) para compilación condicional por
// chip, independiente de la placa elegida en platformio.ini.
//
// Uso:
//   #if BASE_HAVE_ETH_MAC
//     // solo ESP32 clásico y P4 tienen MAC Ethernet nativa
//   #endif
//   log("chip: %s, %u núcleos", BASE_CHIP_NAME, BASE_CORE_COUNT);

#if defined(__has_include)
#  if __has_include("sdkconfig.h")
#    include "sdkconfig.h"
#  endif
#endif

#if defined(CONFIG_IDF_TARGET_ESP32)
  #define BASE_CHIP_NAME       "ESP32"
  #define BASE_HAVE_WIFI       1
  #define BASE_HAVE_BLE        1
  #define BASE_HAVE_BT_CLASSIC 1
  #define BASE_HAVE_802154     0
  #define BASE_HAVE_USB_OTG    0
  #define BASE_HAVE_ETH_MAC    1
  #define BASE_HAVE_DUAL_CORE  1
  #define BASE_CORE_COUNT      2
  #define BASE_IS_RISCV        0

#elif defined(CONFIG_IDF_TARGET_ESP32S2)
  #define BASE_CHIP_NAME       "ESP32-S2"
  #define BASE_HAVE_WIFI       1
  #define BASE_HAVE_BLE        0
  #define BASE_HAVE_BT_CLASSIC 0
  #define BASE_HAVE_802154     0
  #define BASE_HAVE_USB_OTG    1
  #define BASE_HAVE_ETH_MAC    0
  #define BASE_HAVE_DUAL_CORE  0
  #define BASE_CORE_COUNT      1
  #define BASE_IS_RISCV        0

#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  #define BASE_CHIP_NAME       "ESP32-S3"
  #define BASE_HAVE_WIFI       1
  #define BASE_HAVE_BLE        1
  #define BASE_HAVE_BT_CLASSIC 0
  #define BASE_HAVE_802154     0
  #define BASE_HAVE_USB_OTG    1
  #define BASE_HAVE_ETH_MAC    0
  #define BASE_HAVE_DUAL_CORE  1
  #define BASE_CORE_COUNT      2
  #define BASE_IS_RISCV        0

#elif defined(CONFIG_IDF_TARGET_ESP32S31)
  // Pre-release "ESP32-S31" (probable errata de ESP32-S3); tratar como S3.
  #define BASE_CHIP_NAME       "ESP32-S3"  // TODO: confirmar nombre oficial
  #define BASE_HAVE_WIFI       1
  #define BASE_HAVE_BLE        1
  #define BASE_HAVE_BT_CLASSIC 0
  #define BASE_HAVE_802154     0
  #define BASE_HAVE_USB_OTG    1
  #define BASE_HAVE_ETH_MAC    0
  #define BASE_HAVE_DUAL_CORE  1
  #define BASE_CORE_COUNT      2
  #define BASE_IS_RISCV        0

#elif defined(CONFIG_IDF_TARGET_ESP32C2)
  #define BASE_CHIP_NAME       "ESP32-C2"
  #define BASE_HAVE_WIFI       1
  #define BASE_HAVE_BLE        1
  #define BASE_HAVE_BT_CLASSIC 0
  #define BASE_HAVE_802154     0
  #define BASE_HAVE_USB_OTG    0
  #define BASE_HAVE_ETH_MAC    0
  #define BASE_HAVE_DUAL_CORE  0
  #define BASE_CORE_COUNT      1
  #define BASE_IS_RISCV        1

#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  #define BASE_CHIP_NAME       "ESP32-C3"
  #define BASE_HAVE_WIFI       1
  #define BASE_HAVE_BLE        1
  #define BASE_HAVE_BT_CLASSIC 0
  #define BASE_HAVE_802154     0
  #define BASE_HAVE_USB_OTG    0
  #define BASE_HAVE_ETH_MAC    0
  #define BASE_HAVE_DUAL_CORE  0
  #define BASE_CORE_COUNT      1
  #define BASE_IS_RISCV        1

#elif defined(CONFIG_IDF_TARGET_ESP32C5)
  #define BASE_CHIP_NAME       "ESP32-C5"
  #define BASE_HAVE_WIFI       1
  #define BASE_HAVE_BLE        1
  #define BASE_HAVE_BT_CLASSIC 0
  #define BASE_HAVE_802154     1
  #define BASE_HAVE_USB_OTG    0
  #define BASE_HAVE_ETH_MAC    0
  #define BASE_HAVE_DUAL_CORE  0
  #define BASE_CORE_COUNT      1
  #define BASE_IS_RISCV        1

#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  #define BASE_CHIP_NAME       "ESP32-C6"
  #define BASE_HAVE_WIFI       1
  #define BASE_HAVE_BLE        1
  #define BASE_HAVE_BT_CLASSIC 0
  #define BASE_HAVE_802154     1
  #define BASE_HAVE_USB_OTG    0
  #define BASE_HAVE_ETH_MAC    0
  #define BASE_HAVE_DUAL_CORE  0
  #define BASE_CORE_COUNT      1
  #define BASE_IS_RISCV        1

#elif defined(CONFIG_IDF_TARGET_ESP32C61)
  #define BASE_CHIP_NAME       "ESP32-C61"
  #define BASE_HAVE_WIFI       1
  #define BASE_HAVE_BLE        1
  #define BASE_HAVE_BT_CLASSIC 0
  #define BASE_HAVE_802154     0
  #define BASE_HAVE_USB_OTG    0
  #define BASE_HAVE_ETH_MAC    0
  #define BASE_HAVE_DUAL_CORE  0
  #define BASE_CORE_COUNT      1
  #define BASE_IS_RISCV        1

#elif defined(CONFIG_IDF_TARGET_ESP32H2)
  #define BASE_CHIP_NAME       "ESP32-H2"
  #define BASE_HAVE_WIFI       0
  #define BASE_HAVE_BLE        1
  #define BASE_HAVE_BT_CLASSIC 0
  #define BASE_HAVE_802154     1
  #define BASE_HAVE_USB_OTG    0
  #define BASE_HAVE_ETH_MAC    0
  #define BASE_HAVE_DUAL_CORE  0
  #define BASE_CORE_COUNT      1
  #define BASE_IS_RISCV        1

#elif defined(CONFIG_IDF_TARGET_ESP32P4)
  #define BASE_CHIP_NAME       "ESP32-P4"
  #define BASE_HAVE_WIFI       0
  #define BASE_HAVE_BLE        0
  #define BASE_HAVE_BT_CLASSIC 0
  #define BASE_HAVE_802154     0
  #define BASE_HAVE_USB_OTG    1
  #define BASE_HAVE_ETH_MAC    1
  #define BASE_HAVE_DUAL_CORE  1
  #define BASE_CORE_COUNT      2
  #define BASE_IS_RISCV        1

#else
  #warning "chip.hpp: target ESP32 desconocido — capacidades por defecto en 0"
  #define BASE_CHIP_NAME       "ESP32-unknown"
  #define BASE_HAVE_WIFI       0
  #define BASE_HAVE_BLE        0
  #define BASE_HAVE_BT_CLASSIC 0
  #define BASE_HAVE_802154     0
  #define BASE_HAVE_USB_OTG    0
  #define BASE_HAVE_ETH_MAC    0
  #define BASE_HAVE_DUAL_CORE  0
  #define BASE_CORE_COUNT      1
  #define BASE_IS_RISCV        0

#endif
