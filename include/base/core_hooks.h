#pragma once

#include <vector>

#include "core/ConfigManager.hpp"
#include "core/Measurement.hpp"
#include "core/alarms/RuleEngine.hpp"
#include "core/sensors/Sensor.hpp"
#include "core/sensors/SensorManager.hpp"

// =============================================================================
// ESP32-Base — Hooks de dominio
// =============================================================================
// La base es genérica. Los comportamientos específicos del dominio (SEMA u otro
// proyecto) se inyectan aquí: catálogo de sensores, magnitudes derivadas,
// pronóstico, etiquetas de medición y endpoints web de dominio.
//
// El proyecto consumidor rellena estos punteros (p. ej. desde su `main.cpp` o un
// archivo de glue de dominio) antes de llamar a SemaCore::setup().

namespace base {

class SemaCore;

struct CoreHooks {
  // ---- Sensores (acoplamiento #3/#4/#6) ----
  Sensor* (*createSensor)(const SensorSpec& spec) = nullptr;
  void (*registerDefaultSensors)(SensorManager& sensors) = nullptr;
  void (*registerDefaultRules)(RuleEngine& rules) = nullptr;
  void (*registerDefaultCalibrations)(SensorManager& sensors) = nullptr;

  // ---- Magnitudes derivadas (acoplamiento #5) ----
  void (*derivedConfigure)(const SystemConfig& sys) = nullptr;
  void (*computeDerived)(std::vector<Measurement>& measurements) = nullptr;

  // ---- Pronóstico (acoplamiento #5) ----
  void (*forecastAddSample)(const char* measurement, uint32_t timestamp,
                            float value) = nullptr;

  // ---- Arranque de dominio (I²C, escaneo, etc.) ----
  void (*domainInit)() = nullptr;

  // ---- Web de dominio (acoplamiento #1/#5/#7) ----
  String (*sensorCatalogJson)() = nullptr;  // array JS TYPES[] (página de sensores)
  String (*measurementLabelsJson)() = nullptr;  // objeto JS MEAS{} (dashboard)
  String (*sensorsJson)(const SemaCore& core, bool imperial) = nullptr;  // /api/v1/sensors
  String (*forecastJson)() = nullptr;  // /api/v1/forecast
  String (*windNorthJson)(SemaCore& core,
                          int& status) = nullptr;  // /api/v1/wind/north
  String (*windResistorsJson)(SemaCore& core, const String& body,
                              int& status) = nullptr;  // /api/v1/wind/resistors
};

inline CoreHooks& coreHooks() {
  static CoreHooks hooks;
  return hooks;
}

}  // namespace base
