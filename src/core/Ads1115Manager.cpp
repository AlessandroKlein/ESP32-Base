#include "core/Ads1115Manager.hpp"

#include "base/board_config.h"

#if SEMA_USE_ADS1115
#include <Arduino.h>
#include <Wire.h>
#include <ADS1X15.h>

namespace base {

// Puntero porque la dirección I²C se fija en el constructor.
static ADS1115* g_adc = nullptr;

void Ads1115Manager::apply(const Ads1115Config& cfg) {
  cfg_ = cfg;
  ready_ = false;
  delete g_adc;
  g_adc = nullptr;
  if (cfg_.address == 0) {
    return;  // sin chip conectado
  }
  Wire.begin();  // la librería ADS1X15 no inicializa el bus I²C (begin() solo chequea conexión)
  g_adc = new ADS1115(cfg_.address);
  ready_ = g_adc->begin();  // begin() == isConnected() (rango 0x48..0x4B)
}

int16_t Ads1115Manager::readChannel(uint8_t channel) {
  if (!ready_ || g_adc == nullptr || channel > 3) {
    return 0;
  }
  return g_adc->readADC(channel);
}

float Ads1115Manager::voltage(uint8_t channel, float vref, float gain) {
  if (!ready_ || gain <= 0.0f) {
    return 0.0f;
  }
  const int16_t raw = readChannel(channel);
  // ADS1115: 16 bits, rango ±FSR con FSR = vref / gain.
  return (static_cast<float>(raw) / 32768.0f) * (vref / gain);
}

}  // namespace base
#endif  // SEMA_USE_ADS1115
