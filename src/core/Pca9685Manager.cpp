#include "core/Pca9685Manager.hpp"

#include "base/board_config.h"

#if SEMA_USE_PCA9685
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

namespace base {

// Puntero porque la dirección I²C se fija en el constructor (no hay setAddr).
static Adafruit_PWMServoDriver* g_pwm = nullptr;

void Pca9685Manager::apply(const Pca9685Config& cfg) {
  cfg_ = cfg;
  ready_ = false;
  delete g_pwm;
  g_pwm = nullptr;
  if (cfg_.address == 0) {
    return;  // sin chip conectado
  }
  g_pwm = new Adafruit_PWMServoDriver(cfg_.address);
  g_pwm->begin();
  g_pwm->setPWMFreq(cfg_.frequency);
  ready_ = true;
  // Aplica el duty inicial de cada canal (0 = salida apagada).
  for (uint8_t i = 0; i < 16; ++i) {
    if (cfg_.channels[i] > 0) {
      g_pwm->setPin(i, cfg_.channels[i]);
    }
  }
}

void Pca9685Manager::setChannel(uint8_t channel, uint16_t value) {
  if (ready_ && g_pwm != nullptr && channel < 16) {
    g_pwm->setPin(channel, value > 4095 ? 4095 : value);
  }
}

}  // namespace base
#endif  // SEMA_USE_PCA9685
