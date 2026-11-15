#pragma once

#include <cstdint>

#include "core/ConfigManager.hpp"

// =============================================================================
// SEMA — Gestor del TCA9548A (mux I²C de 8 canales)
// =============================================================================
// Multiplexa un bus I²C en 8 buses independientes. Permite conectar varios
// dispositivos con la misma dirección I²C (p.ej. varios ADS1115 a 0x48) y
// seleccionar cuál está activo en cada momento.

namespace base {

class Tca9548aManager {
public:
  void apply(const Tca9548aConfig& cfg);
  void selectChannel(uint8_t channel);

  const Tca9548aConfig& config() const { return cfg_; }
  bool ready() const { return ready_; }

private:
  Tca9548aConfig cfg_;
  bool ready_ = false;
};

}  // namespace base
