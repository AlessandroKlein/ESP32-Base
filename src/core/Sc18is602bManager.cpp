#include "core/Sc18is602bManager.hpp"

#include "base/board_config.h"

#if SEMA_USE_SC18IS602B
#include <Arduino.h>
#include <Wire.h>
#include <SC18IS602B.h>

namespace base {

// Puntero porque la dirección I²C se fija en el constructor.
static SC18IS602B* g_bridge = nullptr;

// Mapea el reloj SPI pedido (Hz) a la velocidad fija más cercana del chip.
// ponytail: 4 velocidades fijas (58/115/461/1843 kHz); redondeo al más cercano.
static SC18IS601B_SPI_Speed mapClock(uint32_t hz) {
  if (hz >= 1152000) return SC18IS601B_SPICLK_1843_kHz;  // >= ~1.15 MHz
  if (hz >= 288000) return SC18IS601B_SPICLK_461_kHz;    // >= ~288 kHz
  if (hz >= 86500) return SC18IS601B_SPICLK_115_kHz;     // >= ~86.5 kHz
  return SC18IS601B_SPICLK_58_kHz;
}

void Sc18is602bManager::apply(const Sc18is602bConfig& cfg) {
  cfg_ = cfg;
  ready_ = false;
  delete g_bridge;
  g_bridge = nullptr;
  if (cfg_.address == 0) {
    return;  // sin chip conectado
  }
  // La librería deriva la dirección (0x28..0x2F, 7-bit) de los pines A0/A1/A2;
  // no acepta la dirección como entero. Se extraen los 3 bits bajos.
  g_bridge = new SC18IS602B(-1,
                            (cfg_.address >> 0) & 0x01,
                            (cfg_.address >> 1) & 0x01,
                            (cfg_.address >> 2) & 0x01);
  g_bridge->begin();
  g_bridge->configureSPI(false,
                         static_cast<SC18IS601B_SPI_Mode>(cfg_.spiMode & 0x03),
                         mapClock(cfg_.spiClock));
  ready_ = true;
}

bool Sc18is602bManager::spiTransfer(uint8_t slaveNum, const uint8_t* tx, size_t len, uint8_t* rx) {
  if (!ready_ || g_bridge == nullptr || tx == nullptr) {
    return false;
  }
  return g_bridge->spiTransfer(slaveNum, tx, len, rx);
}

}  // namespace base
#endif  // SEMA_USE_SC18IS602B
