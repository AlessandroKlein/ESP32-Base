#pragma once

#include <cstdint>

#include "core/ConfigManager.hpp"

// Forward declaration del socket lwIP (definido por <lwip/sockets.h>). Se
// declara aquí para no incluir lwIP en el header; la definición completa llega
// en el .cpp. Debe quedar en el ámbito global (no en base::) para que resuelva
// al mismo tipo que usa <lwip/sockets.h>.
struct sockaddr_in;

// =============================================================================
// SEMA — Servidor BACnet/IP (puente de protocolos, transporte-agnóstico)
// =============================================================================
// Expone un objeto Device + objetos Analog Input (magnitudes de sensores) +
// Binary Input (estados digitales) sobre BACnet/IP (UDP 47808/BAC0). No se ata
// a ningún transporte: escucha en INADDR_ANY con un socket lwIP UDP crudo, así
// sirve igual con Ethernet deshabilitado y WiFi activo (y viceversa), como
// ModbusTcpManager.
//
// Implementación: servidor BACnet/IP MÍNIMO autocontenido (Who-Is→I-Am y
// ReadProperty→ComplexAck). El stack de referencia `vitaly.ruhl/ESP32 BACnet
// Stack` (GPL v2) es CLIENT-ONLY — su `BacnetServer::begin()` es un stub sin
// socket ni responder — así que aquí se codifica el wire format BACnet/IP a
// mano (constantes y layout verificados contra su `src/portable/BacnetProtocol.cpp`).
// No se registra la librería en library.json: para un servidor sería peso muerto.
//
// Licencia: este manager es código propio; el formato BACnet no impone licencia.
// Si más adelante se adopta el stack GPL v2 completo (servidor COV, alarmas,
// BBMD), re-evaluar el cumplimiento GPL de SEMA.
//
// Coste (medido al compilar): ~2.5 KB RAM (tablas de puntos) + ~4 KB flash.

namespace base {

class BacnetManager {
public:
  static constexpr uint16_t kMaxAnalog = 32;  // Analog Input (magnitudes)
  static constexpr uint16_t kMaxBinary = 32;  // Binary Input (estados)
  static constexpr uint16_t kMaxApdu = 1476;  // max APDU aceptado (BACnet típico)

  void apply(const BacnetConfig& cfg);
  void loop();  // recibe datagramas UDP y responde (no bloqueante)

  // Mapping: el índice N (0-based) mapea al objeto Analog/Binary Input de
  // instancia N+1. Lo rellena SemaCore cada ciclo de lectura. `units` es el
  // código de Engineering Units BACnet (ver engineeringUnits()).
  void setAnalogCount(uint16_t n);
  void setAnalogInput(uint16_t idx, const char* name, float value, uint16_t units);
  void setBinaryCount(uint16_t n);
  void setBinaryInput(uint16_t idx, const char* name, bool on);

  // Convierte una unidad SEMA ("degC","percent","hPa",…) al código BACnet de
  // Engineering Units (62=°C, 98=%, 29=%RH, 57=hPa, 95=No Units, …).
  static uint16_t engineeringUnits(const char* unit, const char* name);

  bool ready() const { return ready_; }
  const BacnetConfig& config() const { return cfg_; }

private:
  struct AnalogPoint {
    float value = 0.0f;
    uint16_t units = 95;  // BACnet Engineering Units (95 = No Units)
    char name[33] = {0};  // Object_Name del Analog Input
  };

  bool receive();  // procesa un datagrama pendiente; false = nada/error
  void handlePacket(const uint8_t* buf, uint16_t n,
                    const struct sockaddr_in& src);
  void handleReadProperty(const uint8_t* p, uint16_t len, uint8_t invokeId,
                          const struct sockaddr_in& src);
  void sendIAm(const struct sockaddr_in& src);
  void sendError(const struct sockaddr_in& src, uint8_t invokeId,
                 uint8_t errClass, uint8_t errCode);
  bool sendTo(const struct sockaddr_in& dst, const uint8_t* data, uint16_t len);

  BacnetConfig cfg_;
  bool ready_ = false;
  int fd_ = -1;
  uint16_t analogCount_ = 0;
  uint16_t binaryCount_ = 0;
  AnalogPoint analog_[kMaxAnalog];
  bool binary_[kMaxBinary] = {false};
  char binaryName_[kMaxBinary][33] = {{0}};
};

}  // namespace base
