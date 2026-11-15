#include "core/PowerManager.hpp"

#include "driver/gpio.h"
#include "esp_sleep.h"

namespace base {

PowerManager& PowerManager::instance() {
  static PowerManager mgr;
  return mgr;
}

void PowerManager::sleep(uint64_t seconds) {
  // Wake por timer RTC (D-0021).
  esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
  esp_deep_sleep_start();
}

void PowerManager::enableRainWakeup(uint8_t pin) {
  // Wake por GPIO (D-0022): pluviómetro de cangilones. Nivel HIGH en el pulso.
#if SOC_PM_SUPPORT_EXT0_WAKEUP
  // ESP32/S2/S3: wakeup EXT0 de un único pin RTC.
  esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(pin), 1);
#else
  // C2/C3/C6/H2/C5/C61/P4 (sin EXT0): wakeup GPIO por máscara de 1 bit.
  esp_deep_sleep_enable_gpio_wakeup(1ULL << pin, ESP_GPIO_WAKEUP_GPIO_HIGH);
#endif
}

uint32_t PowerManager::wakeReason() const {
  return static_cast<uint32_t>(esp_sleep_get_wakeup_cause());
}

}  // namespace base
