#pragma once

#include "base/board_config.h"

#include <WebServer.h>
#include <WebSocketsServer.h>
#include <vector>

#include "core/Measurement.hpp"

// =============================================================================
// SEMA — Servidor HTTP local (REST /api/v1)
// =============================================================================
// D-0041 / D-0006. Expone la API local sin depender de ningún servidor.
// WebSocket (/ws) se añadirá en una iteración posterior.

namespace base {

class SemaCore;

class HttpServer {
public:
  void begin(SemaCore& core);
  void loop();
  void broadcastMeasurements(const std::vector<Measurement>& measurements);

private:
  void onRoot();
  void onNetworkPage();
  void onSecurityPage();
  void onSystemPage();
  void onWindPage();
  void onSensorsPage();
  void onSensorsViewPage();
  void onEventsPage();
  void onStatus();
  void onHealth();
  void onSystem();
  void onConfig();
  void onConfigPut();
  void onConfigNetwork();
  void onConfigSystem();
  void onConfigSensors();
  void onConfigIo();
  void onConfigBuses();
  void onWifiScan();
  void onApiKeys();
  void onUpdateCheck();
  void onBackup();
  void onLoginPost();
  void onLoginPage();
  void onLogout();
  void onRestart();
  void onOta();
  void onOtaUpload();
  void onCapabilities();
  void onNetwork();
  void onEnergy();
  void onDiagnostics();
  void onSensors();
  void onForecast();
  void onWindNorth();
  void onWindResistors();
  void onDashboardLayout();
  void onDashboardLayoutGet();
  void onHistory();
  void onEvents();
  void onAlarms();
  void onGpio();
  void onGpioWrite();
  void onShift();
  void onShiftWrite();
#if SEMA_USE_MCP23S17
  void onMcp23s17();
  void onMcp23s17Write();
#endif
#if SEMA_USE_PCA9685
  void onPca9685();
  void onPca9685Write();
#endif
#if SEMA_USE_PCF8574
  void onPcf8574();
  void onPcf8574Write();
#endif
#if SEMA_USE_SC16IS752
  void onSc16is752();
  void onSc16is752Write();
#endif
#if SEMA_USE_SC18IS602B
  void onSc18is602b();
  void onSc18is602bWrite();
#endif
#if SEMA_USE_TCA9548A
  void onTca9548a();
  void onTca9548aWrite();
#endif
#if SEMA_USE_ADS1115
  void onAds1115();
  void onAds1115Write();
#endif
#if SEMA_USE_MODBUS
  void onModbus();
#endif
#if SEMA_USE_MODBUS_TCP
  void onModbusTcp();
#endif
#if SEMA_USE_BACNET
  void onBacnet();
#endif
#if SEMA_USE_MESH_ESPNOW
  void onMesh();
#endif
#if SEMA_USE_CAN
  void onCan();
  void onCanWrite();
#endif
#if SEMA_USE_LORA
  void onLora();
  void onLoraWrite();
#endif
#if SEMA_USE_ZIGBEE
  void onZigbee();
  void onZigbeeWrite();
#endif
  void onNotFound();

  bool authorized();
  bool sessionAuthorized();
  bool webAuthed();  // acceso web: sin login configurado, o sesión, o API key
  bool pinsLocked() const;  // pines no editables: variante PCB o no estamos en AP

  ::WebServer server_;
  ::WebSocketsServer ws_{81};
  SemaCore* core_ = nullptr;
  bool otaAuthorized_ = false;
  String otaExpectedSha_;  // SHA-256 esperado del firmware (cabecera X-SHA256)
  bool otaShaOk_ = true;   // false = checksum no coincide → no reiniciar
  String sessionToken_;
  uint32_t failedLogins_ = 0;
  uint32_t lockoutUntilMs_ = 0;
  uint32_t sessionStartMs_ = 0;  // 0 = sin sesión activa
};

}  // namespace base
