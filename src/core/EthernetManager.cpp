#include "core/EthernetManager.hpp"

#include <Arduino.h>

#include "base/board_config.h"

#if SEMA_USE_ETHERNET

// Selección de variante por directivas del preprocesador (build_flags).
//   BOARD_ESP32_WROOM → MAC Ethernet nativa + LAN8720A (RMII).
//   BOARD_ESP32_S3    → sin MAC nativa, W5500 SPI vía driver ESP-IDF (esp_eth),
//                       integrado a lwIP (la misma pila que WiFi).
#if SEMA_NATIVE_ETH
#include <ETH.h>
#else
#include <esp_eth.h>
#include <esp_eth_mac.h>
#include <esp_eth_phy.h>
#include <esp_netif.h>
#include <esp_eth_netif_glue.h>
#include <driver/spi_master.h>
#endif

namespace base {

#if !SEMA_NATIVE_ETH
static esp_netif_t* g_eth_netif = nullptr;
#endif

void EthernetManager::apply(const EthernetConfig& cfg) {
  cfg_ = cfg;
  ready_ = false;
  if (!cfg_.enabled) {
    return;
  }

#if SEMA_NATIVE_ETH
  // LAN8720A por RMII: reloj de 50 MHz entrando por GPIO0.
  // core 3.x: ETH.begin(type, phy_addr, mdc, mdio, power, clk_mode).
  ETH.begin(ETH_PHY_LAN8720, cfg_.phyAddr, cfg_.mdcPin, cfg_.mdioPin,
            cfg_.powerPin, ETH_CLOCK_GPIO0_IN);
  ready_ = true;

#else
  // --- Dispositivo SPI (W5500: address 16 bits + control 8 bits) ---
  // core 3.x (IDF 5.x): ETH_W5500_DEFAULT_CONFIG(spi_host, &spi_devcfg) delega el
  // bus SPI al driver interno (spi_bus_initialize + spi_bus_add_device con los pines
  // por defecto del host). `static` para que &devcfg siga válido tras apply().
  static spi_device_interface_config_t devcfg = {};
  devcfg.command_bits = 16;
  devcfg.address_bits = 8;
  devcfg.mode = 0;
  devcfg.clock_speed_hz = 20 * 1000 * 1000;  // 20 MHz
  devcfg.spics_io_num = cfg_.csPin;
  devcfg.queue_size = 20;

  // --- MAC W5500 ---
  eth_w5500_config_t w5500 = ETH_W5500_DEFAULT_CONFIG(
      static_cast<spi_host_device_t>(SEMA_ETH_SPI_HOST), &devcfg);
  w5500.int_gpio_num = cfg_.irqPin;
  eth_mac_config_t mac_cfg = ETH_MAC_DEFAULT_CONFIG();
  esp_eth_mac_t* mac = esp_eth_mac_new_w5500(&w5500, &mac_cfg);
  if (mac == nullptr) {
    return;
  }

  // --- PHY W5500 ---
  eth_phy_config_t phy_cfg = ETH_PHY_DEFAULT_CONFIG();
  phy_cfg.phy_addr = cfg_.phyAddr;
  phy_cfg.reset_gpio_num = cfg_.rstPin;
  esp_eth_phy_t* phy = esp_eth_phy_new_w5500(&phy_cfg);
  if (phy == nullptr) {
    return;
  }

  // --- Driver Ethernet ---
  esp_eth_config_t eth_cfg = ETH_DEFAULT_CONFIG(mac, phy);
  esp_eth_handle_t eth_handle = nullptr;
  if (esp_eth_driver_install(&eth_cfg, &eth_handle) != ESP_OK) {
    return;
  }

  // --- Interfaz de red lwIP ---
  esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
  g_eth_netif = esp_netif_new(&netif_cfg);
  esp_netif_attach(g_eth_netif, esp_eth_new_netif_glue(eth_handle));

  // --- Arranque (DHCP) ---
  esp_eth_start(eth_handle);
  ready_ = true;
#endif
}

void EthernetManager::loop() {
  // lwIP (nativo y W5500) gestiona DHCP automáticamente; sin mantenimiento.
}

bool EthernetManager::connected() const {
#if SEMA_NATIVE_ETH
  return ready_ && ETH.linkUp();
#else
  if (!ready_ || g_eth_netif == nullptr) {
    return false;
  }
  esp_netif_ip_info_t ip;
  if (esp_netif_get_ip_info(g_eth_netif, &ip) != ESP_OK) {
    return false;
  }
  return ip.ip.addr != 0;
#endif
  return false;
}

String EthernetManager::localIP() const {
#if SEMA_NATIVE_ETH
  return ETH.localIP().toString();
#else
  if (g_eth_netif == nullptr) {
    return String();
  }
  esp_netif_ip_info_t ip;
  if (esp_netif_get_ip_info(g_eth_netif, &ip) != ESP_OK) {
    return String();
  }
  return IPAddress(ip.ip.addr).toString();
#endif
  return String();
}

}  // namespace base
#endif  // SEMA_USE_ETHERNET
