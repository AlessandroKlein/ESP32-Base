#pragma once

#include <cstdint>

#include "core/ConfigManager.hpp"

// =============================================================================
// SEMA — Gestor del MCP23S17 (expansor GPIO por SPI, 16 pines)
// =============================================================================
// Mismo rol que GpioManager (entradas/salidas digitales) pero por bus SPI en
// lugar de I²C. El bus SPI debe estar ya inicializado (SemaCore lo hace una
// única vez cuando hay algún dispositivo SPI).

namespace base {

class Mcp23s17Manager {
public:
  void apply(const Mcp23s17Config& cfg);
  void pinMode(uint8_t pin, uint8_t mode);
  void digitalWrite(uint8_t pin, uint8_t value);
  int digitalRead(uint8_t pin) const;

  const Mcp23s17Config& config() const { return cfg_; }
  bool ready() const { return ready_; }

private:
  Mcp23s17Config cfg_;
  bool ready_ = false;
};

}  // namespace base
