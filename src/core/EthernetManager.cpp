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

// --- Driver SPI custom para W5500 (pines configurables) ---
// El driver default de ESP-IDF inicializa el bus con pines fijos del host, lo que
// impide elegir pines por flag. Este driver custom hace spi_bus_initialize +
// spi_bus_add_device en los pines de cfg_ (SEMA_ETH_SPI_*), para poder "ahorrar
// pines" y ubicar el W5500 donde convenga en cada board.
namespace {
struct W5500SpiConfig {
  spi_device_interface_config_t devcfg;
  int host = -1;
  int sck = -1;
  int miso = -1;
  int mosi = -1;
};
struct W5500SpiContext {
  spi_device_handle_t dev = nullptr;
  int host = -1;
};
void* w5500SpiInit(const void* cfg) {
  const W5500SpiConfig* c = static_cast<const W5500SpiConfig*>(cfg);
  W5500SpiContext* ctx = new W5500SpiContext();
  ctx->host = c->host;
  spi_bus_config_t bus = {};
  bus.mosi_io_num = c->mosi;
  bus.miso_io_num = c->miso;
  bus.sclk_io_num = c->sck;
  bus.quadwp_io_num = -1;
  bus.quadhd_io_num = -1;
  bus.max_transfer_sz = 4092;
  if (spi_bus_initialize(static_cast<spi_host_device_t>(c->host), &bus,
                         SPI_DMA_CH_AUTO) != ESP_OK) {
    delete ctx;
    return nullptr;
  }
  if (spi_bus_add_device(static_cast<spi_host_device_t>(c->host), &c->devcfg,
                         &ctx->dev) != ESP_OK) {
    delete ctx;
    return nullptr;
  }
  return ctx;
}
esp_err_t w5500SpiDeinit(void* ctx) {
  W5500SpiContext* c = static_cast<W5500SpiContext*>(ctx);
  if (c == nullptr) return ESP_OK;
  if (c->dev != nullptr) spi_bus_remove_device(c->dev);
  if (c->host >= 0) spi_bus_free(static_cast<spi_host_device_t>(c->host));
  delete c;
  return ESP_OK;
}
esp_err_t w5500SpiRead(void* ctx, uint32_t cmd, uint32_t addr, void* data,
                       uint32_t len) {
  W5500SpiContext* c = static_cast<W5500SpiContext*>(ctx);
  spi_transaction_t t = {};
  t.cmd = static_cast<uint16_t>(cmd);
  t.addr = addr;
  t.length = len * 8;
  t.rxlength = len * 8;
  t.rx_buffer = data;
  return spi_device_polling_transmit(c->dev, &t);
}
esp_err_t w5500SpiWrite(void* ctx, uint32_t cmd, uint32_t addr, const void* data,
                        uint32_t len) {
  W5500SpiContext* c = static_cast<W5500SpiContext*>(ctx);
  spi_transaction_t t = {};
  t.cmd = static_cast<uint16_t>(cmd);
  t.addr = addr;
  t.length = len * 8;
  t.tx_buffer = data;
  return spi_device_polling_transmit(c->dev, &t);
}
}  // namespace
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
  // --- Dispositivo SPI (W5500: comando 16 bits + dirección 8 bits) ---
  // `static` para que los punteros sigan válidos tras apply() (el driver W5500 los
  // retiene y usa al arrancar). El driver custom honra cfg_.sckPin/misoPin/mosiPin
  // (SEMA_ETH_SPI_*) en lugar de los pines fijos del host.
  static W5500SpiConfig spi_cfg = {};
  spi_cfg.devcfg.command_bits = 16;   // fase de control (BSB+RWB+OM+addrH)
  spi_cfg.devcfg.address_bits = 8;    // registro (addrL)
  spi_cfg.devcfg.mode = 0;
  spi_cfg.devcfg.clock_speed_hz = 20 * 1000 * 1000;  // 20 MHz
  spi_cfg.devcfg.spics_io_num = cfg_.csPin;
  spi_cfg.devcfg.queue_size = 20;
  spi_cfg.host = SEMA_ETH_SPI_HOST;
  spi_cfg.sck = cfg_.sckPin;
  spi_cfg.miso = cfg_.misoPin;
  spi_cfg.mosi = cfg_.mosiPin;

  // --- MAC W5500 ---
  eth_w5500_config_t w5500 = ETH_W5500_DEFAULT_CONFIG(
      static_cast<spi_host_device_t>(SEMA_ETH_SPI_HOST), &spi_cfg.devcfg);
  w5500.int_gpio_num = cfg_.irqPin;
  w5500.custom_spi_driver.config = &spi_cfg;
  w5500.custom_spi_driver.init = w5500SpiInit;
  w5500.custom_spi_driver.deinit = w5500SpiDeinit;
  w5500.custom_spi_driver.read = w5500SpiRead;
  w5500.custom_spi_driver.write = w5500SpiWrite;
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
