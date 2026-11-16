#pragma once

#include "base/board_config.h"
#include "base/core_hooks.h"

#include "core/CapabilityManager.hpp"
#include "core/ConfigManager.hpp"
#include "core/alarms/RuleEngine.hpp"
#include "core/events/EventLog.hpp"
#include "core/EventBus.hpp"
#include "core/GpioManager.hpp"
#include "core/ShiftRegisterManager.hpp"
#if SEMA_USE_MCP23S17
#include "core/Mcp23s17Manager.hpp"
#endif
#if SEMA_USE_PCA9685
#include "core/Pca9685Manager.hpp"
#endif
#if SEMA_USE_PCF8574
#include "core/Pcf8574Manager.hpp"
#endif
#if SEMA_USE_SC16IS752
#include "core/Sc16is752Manager.hpp"
#endif
#if SEMA_USE_SC18IS602B
#include "core/Sc18is602bManager.hpp"
#endif
#if SEMA_USE_TCA9548A
#include "core/Tca9548aManager.hpp"
#endif
#if SEMA_USE_ADS1115
#include "core/Ads1115Manager.hpp"
#endif
#if SEMA_USE_MODBUS
#include "core/ModbusManager.hpp"
#endif
#if SEMA_USE_CAN
#include "core/CanManager.hpp"
#endif
#if SEMA_USE_MODBUS_TCP
#include "core/ModbusTcpManager.hpp"
#endif
#if SEMA_USE_BACNET
#include "core/BacnetManager.hpp"
#endif
#if SEMA_USE_MESH_ESPNOW
#include "core/MeshManager.hpp"
#endif
#if SEMA_USE_LORA
#include "core/LoraManager.hpp"
#endif
#if SEMA_USE_ZIGBEE
#include "core/ZigbeeManager.hpp"
#endif
#if SEMA_USE_ETHERNET
#include "core/EthernetManager.hpp"
#endif
#include "core/HealthMonitor.hpp"
#include "core/Measurement.hpp"
#include "core/ModuleRegistry.hpp"
#include "core/sensors/SensorManager.hpp"
#include "core/Scheduler.hpp"
#include "core/network/WiFiManager.hpp"
#include "core/PowerManager.hpp"
#include "core/Watchdog.hpp"
#include "core/publishers/HttpPublisher.hpp"
#include "core/publishers/MqttPublisher.hpp"
#include "core/publishers/PublisherManager.hpp"
#include "core/storage/HistoryStore.hpp"
#include "core/storage/NvsStore.hpp"
#include "core/web/HttpServer.hpp"

// =============================================================================
// ESP32-Base — Núcleo de la plataforma
// =============================================================================
// main.cpp se mantiene pequeño: el Core inicializa los servicios base (storage,
// configuración, capacidades, red, web, scheduler, módulos, eventos) y conduce
// el bucle.
//
// Lo específico del dominio (sensores concretos, magnitudes derivadas,
// pronóstico, catálogo web) se inyecta vía coreHooks() (base/core_hooks.h).

namespace base {

class SemaCore {
public:
  static SemaCore& instance();

  void setup();
  void loop();

  // Re-aplican configuración no hardware-dependiente (sin reinicio).
  void applyRules();
  void applyCalibrations();
  void applyGpio();
  void applyPublishers();
  void applySensors();
  void applyShift();
#if SEMA_USE_MCP23S17
  void applyMcp23s17();
#endif
#if SEMA_USE_PCA9685
  void applyPca9685();
#endif
#if SEMA_USE_PCF8574
  void applyPcf8574();
#endif
#if SEMA_USE_SC16IS752
  void applySc16is752();
#endif
#if SEMA_USE_SC18IS602B
  void applySc18is602b();
#endif
#if SEMA_USE_TCA9548A
  void applyTca9548a();
#endif
#if SEMA_USE_ADS1115
  void applyAds1115();
#endif
#if SEMA_USE_MODBUS
  void applyModbus();
#endif
#if SEMA_USE_CAN
  void applyCan();
#endif
#if SEMA_USE_MODBUS_TCP
  void applyModbusTcp();
#endif
#if SEMA_USE_BACNET
  void applyBacnet();
#endif
#if SEMA_USE_MESH_ESPNOW
  void applyMesh();
#endif
#if SEMA_USE_LORA
  void applyLora();
#endif
#if SEMA_USE_ZIGBEE
  void applyZigbee();
#endif
#if SEMA_USE_ETHERNET
  void applyEthernet();
#endif

  ModuleRegistry& modules() { return modules_; }
  EventBus& events() { return events_; }
  ConfigManager& config() { return config_; }
  const ConfigManager& config() const { return config_; }
  CapabilityManager& capabilities() { return CapabilityManager::instance(); }
  Scheduler& scheduler() { return scheduler_; }
  WiFiManager& wifi() { return wifi_; }
  PowerManager& power() { return PowerManager::instance(); }
  Watchdog& watchdog() { return watchdog_; }
  HealthMonitor& health() { return health_; }
  SensorManager& sensors() { return sensors_; }
  const SensorManager& sensors() const { return sensors_; }
  HistoryStore& history() { return history_; }
  PublisherManager& publishers() { return publishers_; }
  RuleEngine& rules() { return rules_; }
  EventLog& eventLog() { return eventLog_; }
  GpioManager& gpio() { return gpio_; }
  ShiftRegisterManager& shift() { return shift_; }
#if SEMA_USE_MCP23S17
  Mcp23s17Manager& mcp23s17() { return mcp23s17_; }
#endif
#if SEMA_USE_PCA9685
  Pca9685Manager& pca9685() { return pca9685_; }
#endif
#if SEMA_USE_PCF8574
  Pcf8574Manager& pcf8574() { return pcf8574_; }
#endif
#if SEMA_USE_SC16IS752
  Sc16is752Manager& sc16is752() { return sc16is752_; }
#endif
#if SEMA_USE_SC18IS602B
  Sc18is602bManager& sc18is602b() { return sc18is602b_; }
#endif
#if SEMA_USE_TCA9548A
  Tca9548aManager& tca9548a() { return tca9548a_; }
#endif
#if SEMA_USE_ADS1115
  Ads1115Manager& ads1115() { return ads1115_; }
#endif
  uint32_t restartCount() const { return restartCount_; }
#if SEMA_USE_MODBUS
  ModbusManager& modbus() { return modbus_; }
#endif
#if SEMA_USE_CAN
  CanManager& can() { return can_; }
#endif
#if SEMA_USE_MODBUS_TCP
  ModbusTcpManager& modbusTcp() { return modbusTcp_; }
#endif
#if SEMA_USE_BACNET
  BacnetManager& bacnet() { return bacnet_; }
#endif
#if SEMA_USE_MESH_ESPNOW
  MeshManager& mesh() { return mesh_; }
#endif
#if SEMA_USE_LORA
  LoraManager& lora() { return lora_; }
#endif
#if SEMA_USE_ZIGBEE
  ZigbeeManager& zigbee() { return zigbee_; }
#endif
#if SEMA_USE_ETHERNET
  EthernetManager& ethernet() { return ethernet_; }
#endif

private:
  SemaCore();

  NvsStore store_;
  ModuleRegistry modules_;
  EventBus events_;
  ConfigManager config_;
  SensorManager sensors_;
  HistoryStore history_;
  PublisherManager publishers_;
  HttpPublisher webhook_;
  MqttPublisher mqtt_;
  RuleEngine rules_;
  EventLog eventLog_;
  GpioManager gpio_;
  ShiftRegisterManager shift_;
#if SEMA_USE_MCP23S17
  Mcp23s17Manager mcp23s17_;
#endif
#if SEMA_USE_PCA9685
  Pca9685Manager pca9685_;
#endif
#if SEMA_USE_PCF8574
  Pcf8574Manager pcf8574_;
#endif
#if SEMA_USE_SC16IS752
  Sc16is752Manager sc16is752_;
#endif
#if SEMA_USE_SC18IS602B
  Sc18is602bManager sc18is602b_;
#endif
#if SEMA_USE_TCA9548A
  Tca9548aManager tca9548a_;
#endif
#if SEMA_USE_ADS1115
  Ads1115Manager ads1115_;
#endif
#if SEMA_USE_MODBUS
  ModbusManager modbus_;
#endif
#if SEMA_USE_CAN
  CanManager can_;
#endif
#if SEMA_USE_MODBUS_TCP
  ModbusTcpManager modbusTcp_;
#endif
#if SEMA_USE_BACNET
  BacnetManager bacnet_;
#endif
#if SEMA_USE_MESH_ESPNOW
  MeshManager mesh_;
#endif
#if SEMA_USE_LORA
  LoraManager lora_;
#endif
#if SEMA_USE_ZIGBEE
  ZigbeeManager zigbee_;
#endif
#if SEMA_USE_ETHERNET
  EthernetManager ethernet_;
#endif
  Watchdog watchdog_;
  HealthMonitor health_;
  uint32_t restartCount_ = 0;
  std::vector<Sensor*> ownedSensors_;  // sensores creados por la factoría (D-0042)
  Scheduler scheduler_;
  WiFiManager wifi_;
  HttpServer http_;
};

}  // namespace base
