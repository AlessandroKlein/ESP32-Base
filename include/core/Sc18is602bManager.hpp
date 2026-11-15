#pragma once

#include <cstdint>
#include <cstddef>

#include "core/ConfigManager.hpp"

// =============================================================================
// SEMA — Gestor del SC18IS602B (puente I²C→SPI)
// =============================================================================
// Expone un bus SPI maestro a través del bus I²C. Útil para conectar periféricos
// SPI (sensores, LCD, SD) cuando los pines SPI nativos ya están ocupados. El
// reloj SPI se mapea a una de las 4 velocidades fijas del chip (58..1843 kHz).

namespace base {

class Sc18is602bManager {
public:
  void apply(const Sc18is602bConfig& cfg);
  bool spiTransfer(uint8_t slaveNum, const uint8_t* tx, size_t len, uint8_t* rx);

  const Sc18is602bConfig& config() const { return cfg_; }
  bool ready() const { return ready_; }

private:
  Sc18is602bConfig cfg_;
  bool ready_ = false;
};

}  // namespace base
