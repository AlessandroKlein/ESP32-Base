#pragma once

#include <cstdint>

#include "core/ConfigManager.hpp"

// =============================================================================
// SEMA — Gestor del PCF8574/PCF8575 (expansor GPIO por I²C, 8/16 pines)
// =============================================================================
// PCF8574 = 8 pines; PCF8575 = 16 pines. Son cuasi-bidireccionales: para leer
// un pin hay que dejarlo en alto (pinMode INPUT lo hace).

namespace base {

class Pcf8574Manager {
public:
  void apply(const Pcf8574Config& cfg);
  void digitalWrite(uint8_t pin, uint8_t value);
  int digitalRead(uint8_t pin) const;

  const Pcf8574Config& config() const { return cfg_; }
  bool ready() const { return ready_; }
  uint8_t pinCount() const { return is16_ ? 16 : 8; }

private:
  Pcf8574Config cfg_;
  bool ready_ = false;
  bool is16_ = false;
};

}  // namespace base
