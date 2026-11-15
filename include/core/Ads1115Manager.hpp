#pragma once

#include <cstdint>

#include "core/ConfigManager.hpp"

// =============================================================================
// SEMA — Gestor del ADS1115 (ADC 16-bit de 4 canales por I²C)
// =============================================================================
// ADC de precisión (4 entradas analógicas A0-A3, 16 bits). Útil para sensores
// analógicos (humedad de suelo, presión, intensidad de luz) con mayor
// resolución que el ADC nativo del ESP32 (12 bits).

namespace base {

class Ads1115Manager {
public:
  void apply(const Ads1115Config& cfg);
  int16_t readChannel(uint8_t channel);
  float voltage(uint8_t channel, float vref, float gain);

  const Ads1115Config& config() const { return cfg_; }
  bool ready() const { return ready_; }

private:
  Ads1115Config cfg_;
  bool ready_ = false;
};

}  // namespace base
