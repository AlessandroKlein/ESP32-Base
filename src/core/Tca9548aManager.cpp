#include "core/Tca9548aManager.hpp"

#include "base/board_config.h"

#if SEMA_USE_TCA9548A
#include <Arduino.h>
#include <Wire.h>
#include <TCA9548.h>

namespace base {

// Puntero porque la dirección I²C se fija en el constructor.
static TCA9548* g_mux = nullptr;

void Tca9548aManager::apply(const Tca9548aConfig& cfg) {
  cfg_ = cfg;
  ready_ = false;
  delete g_mux;
  g_mux = nullptr;
  if (cfg_.address == 0) {
    return;  // sin chip conectado
  }
  Wire.begin();  // la librería TCA9548 no inicializa el bus I²C (begin() solo chequea conexión)
  g_mux = new TCA9548(cfg_.address);
  ready_ = g_mux->begin();  // begin() == isConnected()
  if (ready_ && cfg_.activeChannel <= 7) {
    g_mux->selectChannel(cfg_.activeChannel);
  }
}

void Tca9548aManager::selectChannel(uint8_t channel) {
  if (ready_ && g_mux != nullptr && channel <= 7) {
    g_mux->selectChannel(channel);
    cfg_.activeChannel = channel;
  }
}

}  // namespace base
#endif  // SEMA_USE_TCA9548A
