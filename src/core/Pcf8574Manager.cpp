#include "core/Pcf8574Manager.hpp"

#include "base/board_config.h"

#if SEMA_USE_PCF8574
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PCF8574.h>
#include <Adafruit_PCF8575.h>

namespace base {

// Instancias únicas: SemaCore es singleton. Se usa una u otra según el tipo.
static Adafruit_PCF8574 g_pcf8;
static Adafruit_PCF8575 g_pcf16;

void Pcf8574Manager::apply(const Pcf8574Config& cfg) {
  cfg_ = cfg;
  ready_ = false;
  is16_ = (cfg_.type == "PCF8575");
  if (cfg_.address == 0) {
    return;  // sin chip conectado
  }
  ready_ = is16_ ? g_pcf16.begin(cfg_.address) : g_pcf8.begin(cfg_.address);
  if (!ready_) {
    return;
  }
  const uint8_t n = is16_ ? 16 : 8;
  for (uint8_t i = 0; i < n; ++i) {
    if (cfg_.pinModes[i] == 1) {
      // Salida: el pin queda bajo por defecto (transistor/relé apagado).
      if (is16_) g_pcf16.pinMode(i, OUTPUT);
      else g_pcf8.pinMode(i, OUTPUT);
      digitalWrite(i, LOW);
    } else if (cfg_.pinModes[i] == 2) {
      // Entrada: pin en alto para poder leerlo (cuasi-bidireccional).
      if (is16_) g_pcf16.pinMode(i, INPUT);
      else g_pcf8.pinMode(i, INPUT);
    }
  }
}

void Pcf8574Manager::digitalWrite(uint8_t pin, uint8_t value) {
  if (!ready_) return;
  if (is16_) g_pcf16.digitalWrite(pin, value ? HIGH : LOW);
  else g_pcf8.digitalWrite(pin, value ? HIGH : LOW);
}

int Pcf8574Manager::digitalRead(uint8_t pin) const {
  if (!ready_) return 0;
  return is16_ ? (g_pcf16.digitalRead(pin) ? 1 : 0)
               : (g_pcf8.digitalRead(pin) ? 1 : 0);
}

}  // namespace base
#endif  // SEMA_USE_PCF8574
