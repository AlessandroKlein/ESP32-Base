#include "core/Sc16is752Manager.hpp"

#include "base/board_config.h"

#if SEMA_USE_SC16IS752
#include <Arduino.h>
#include <Wire.h>
#include <SC16IS752.h>

namespace base {

// Puntero porque la dirección I²C se fija en el constructor (no hay setAddr).
static SC16IS752* g_uart = nullptr;

void Sc16is752Manager::apply(const Sc16is752Config& cfg) {
  cfg_ = cfg;
  ready_ = false;
  delete g_uart;
  g_uart = nullptr;
  if (cfg_.address == 0) {
    return;  // sin chip conectado
  }
  // La librería acepta la dirección 7-bit (0x48..0x57) tal cual; si se pasa
  // una 8-bit la desplaza. Config guarda la 7-bit.
  g_uart = new SC16IS752(SC16IS750_PROTOCOL_I2C, cfg_.address);
  g_uart->begin(cfg_.baudrate, cfg_.baudrate);
  ready_ = (g_uart->ping() != 0);  // ping devuelve 0 si el chip no responde
}

size_t Sc16is752Manager::write(uint8_t channel, const uint8_t* buf, size_t len) {
  if (!ready_ || g_uart == nullptr || buf == nullptr) {
    return 0;
  }
  // La API real escribe de a un byte: SC16IS752::write(channel, uint8_t).
  size_t n = 0;
  for (size_t i = 0; i < len; ++i) {
    n += g_uart->write(channel, buf[i]);
  }
  return n;
}

int Sc16is752Manager::available(uint8_t channel) {
  if (!ready_ || g_uart == nullptr) {
    return 0;
  }
  return g_uart->available(channel);
}

int Sc16is752Manager::read(uint8_t channel) {
  if (!ready_ || g_uart == nullptr) {
    return -1;
  }
  return g_uart->read(channel);
}

}  // namespace base
#endif  // SEMA_USE_SC16IS752
