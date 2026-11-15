#pragma once

#include <cstdint>
#include <cstddef>

#include "core/ConfigManager.hpp"

// =============================================================================
// SEMA — Gestor del SC16IS752 (UART dual por I²C, 2 puertos)
// =============================================================================
// Convierte el bus I²C en dos UARTs independientes (canal A y B). Útil para
// conectar periféricos serie (RS232/RS485, GPS, terminales) sin consumir los
// UART nativos del ESP32. Los dos canales comparten el mismo baudrate.

namespace base {

class Sc16is752Manager {
public:
  void apply(const Sc16is752Config& cfg);
  size_t write(uint8_t channel, const uint8_t* buf, size_t len);
  int available(uint8_t channel);
  int read(uint8_t channel);

  const Sc16is752Config& config() const { return cfg_; }
  bool ready() const { return ready_; }

private:
  Sc16is752Config cfg_;
  bool ready_ = false;
};

}  // namespace base
