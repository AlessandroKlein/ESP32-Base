#include "core/Mcp23s17Manager.hpp"

#include "base/board_config.h"

#if SEMA_USE_MCP23S17
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_MCP23X17.h>

namespace base {

// Instancia única: SemaCore es singleton (un solo chip SPI por configuración).
static Adafruit_MCP23X17 g_mcp;

void Mcp23s17Manager::apply(const Mcp23s17Config& cfg) {
  cfg_ = cfg;
  ready_ = false;
  if (cfg_.csPin == 0) {
    return;  // sin chip conectado
  }
  // Dirección SPI 0x00 (A0/A1 a GND): puertos A (pines 0-7) y B (pines 8-15).
  ready_ = g_mcp.begin_SPI(cfg_.csPin, &SPI);
  if (!ready_) {
    return;
  }
  for (uint8_t i = 0; i < 16; ++i) {
    if (cfg_.pinModes[i] == 0) {
      continue;
    }
    const uint8_t mode = (cfg_.pinModes[i] == 1) ? OUTPUT : INPUT;
    g_mcp.pinMode(i, mode);
    if (mode == OUTPUT) {
      g_mcp.digitalWrite(i, LOW);  // estado inicial seguro (relés/válvulas apagados)
    }
  }
}

void Mcp23s17Manager::pinMode(uint8_t pin, uint8_t mode) {
  if (ready_ && pin < 16) {
    g_mcp.pinMode(pin, mode);
  }
}

void Mcp23s17Manager::digitalWrite(uint8_t pin, uint8_t value) {
  if (ready_ && pin < 16) {
    g_mcp.digitalWrite(pin, value ? HIGH : LOW);
  }
}

int Mcp23s17Manager::digitalRead(uint8_t pin) const {
  if (ready_ && pin < 16) {
    return g_mcp.digitalRead(pin);
  }
  return 0;
}

}  // namespace base
#endif  // SEMA_USE_MCP23S17
