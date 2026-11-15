#pragma once

#include <cstdint>

#include "core/ConfigManager.hpp"

// =============================================================================
// SEMA — Gestor del PCA9685 (expansor PWM por I²C, 16 canales)
// =============================================================================
// Para válvulas, dimmers o servos. Una sola frecuencia PWM compartida por los
// 16 canales; cada canal es un duty de 12 bits (0..4095).

namespace base {

class Pca9685Manager {
public:
  void apply(const Pca9685Config& cfg);
  void setChannel(uint8_t channel, uint16_t value);  // duty 0..4095

  const Pca9685Config& config() const { return cfg_; }
  bool ready() const { return ready_; }

private:
  Pca9685Config cfg_;
  bool ready_ = false;
};

}  // namespace base
