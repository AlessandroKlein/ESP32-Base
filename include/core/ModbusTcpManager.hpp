#pragma once

#include <cstdint>

#include "core/ConfigManager.hpp"

// =============================================================================
// SEMA — Servidor Modbus TCP (puente de protocolos, transporte-agnóstico)
// =============================================================================
// Expone las magnitudes de los sensores como holding registers y los estados
// digitales/alarmas como coils, sobre la pila de red ACTIVA (WiFi, Ethernet o
// AP). No se ata a ningún transporte: escucha en INADDR_ANY con sockets lwIP
// crudos, así sirve igual con Ethernet deshabilitado y WiFi activo (y viceversa).

namespace base {

class ModbusTcpManager {
public:
  static constexpr uint16_t kHoldingCount = 64;  // holding registers (uint16)
  static constexpr uint16_t kCoilCount = 64;     // coils (bool)

  void apply(const ModbusTcpConfig& cfg);
  void loop();  // acepta conexiones y responde tramas (no bloqueante)

  // Mapping: el registro N mapea a la medición activa N (float→uint16, ×100) y
  // el coil N al estado digital/alarma N. Lo rellena SemaCore cada ciclo de
  // lectura; el índice N no depende de ninguna placa concreta.
  void setHoldingRegister(uint16_t idx, uint16_t value);
  uint16_t holdingRegister(uint16_t idx) const;
  void setCoil(uint16_t idx, bool on);
  bool coil(uint16_t idx) const;

  bool ready() const { return ready_; }
  const ModbusTcpConfig& config() const { return cfg_; }

private:
  bool handleClient(int client);  // procesa una PDU; false = cerrar conexión

  ModbusTcpConfig cfg_;
  bool ready_ = false;
  int listenFd_ = -1;
  uint16_t holding_[kHoldingCount] = {0};
  bool coils_[kCoilCount] = {false};
};

}  // namespace base
