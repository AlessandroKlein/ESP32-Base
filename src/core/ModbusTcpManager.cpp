#include "core/ModbusTcpManager.hpp"

#include "base/board_config.h"

#if SEMA_USE_MODBUS_TCP
#include <Arduino.h>
#include <lwip/sockets.h>
#include <string.h>

namespace base {

// Layout de registros (documentado): bloque fijo de 64 holding registers
// (uint16, big-endian) + 64 coils (bool). Índice N = medición activa N
// (SemaCore escala float→uint16 ×100) / estado digital N. Funciones soportadas:
//   0x01 read coils        0x03 read holding registers
//   0x05 write single coil 0x06 write single register
// ponytail: 0x0F/0x10 (write multiple) diferidos — un puente rara vez los
// necesita; añadir si aparece un cliente que los exija.

namespace {

// Arma la cabecera MBAP (7 bytes) y envía la PDU `pdu` (fn + datos) ya armada.
bool sendPdu(int c, uint16_t tid, uint8_t unit, const uint8_t* pdu,
             uint16_t pduLen) {
  uint8_t hdr[7];
  hdr[0] = static_cast<uint8_t>(tid >> 8);
  hdr[1] = static_cast<uint8_t>(tid & 0xFF);
  hdr[2] = 0;  // protocol id = 0 (Modbus)
  hdr[3] = 0;
  const uint16_t len = static_cast<uint16_t>(pduLen + 1);  // unit id + PDU
  hdr[4] = static_cast<uint8_t>(len >> 8);
  hdr[5] = static_cast<uint8_t>(len & 0xFF);
  hdr[6] = unit;
  if (lwip_send(c, hdr, sizeof(hdr), 0) < 0) {
    return false;
  }
  if (pduLen > 0 && lwip_send(c, pdu, pduLen, 0) < 0) {
    return false;
  }
  return true;
}

}  // namespace

void ModbusTcpManager::apply(const ModbusTcpConfig& cfg) {
  cfg_ = cfg;
  ready_ = false;
  if (listenFd_ >= 0) {
    lwip_close(listenFd_);
    listenFd_ = -1;
  }
  if (!cfg_.enabled) {
    return;
  }

  const int fd = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd < 0) {
    return;
  }
  // Reutiliza el puerto tras un reinicio rápido (evita el TIME_WAIT de lwIP).
  int one = 1;
  lwip_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(cfg_.port);
  addr.sin_addr.s_addr = INADDR_ANY;  // escucha en TODAS las interfaces activas
  if (lwip_bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
    lwip_close(fd);
    return;
  }
  if (lwip_listen(fd, 1) != 0) {
    lwip_close(fd);
    return;
  }
  int on = 1;
  lwip_ioctl(fd, FIONBIO, &on);  // accept no bloqueante
  listenFd_ = fd;
  ready_ = true;
}

void ModbusTcpManager::loop() {
  if (!ready_ || listenFd_ < 0) {
    return;
  }
  struct sockaddr_in client = {};
  socklen_t clen = sizeof(client);
  const int c =
      lwip_accept(listenFd_, reinterpret_cast<struct sockaddr*>(&client), &clen);
  if (c < 0) {
    return;  // sin conexión pendiente
  }
  int on = 1;
  lwip_ioctl(c, FIONBIO, &on);  // el aceptado no hereda no-bloqueante
  // Atiende hasta que la trama quede incompleta o falle el envío. Un cliente
  // Modbus TCP con conexión persistente reenviará/reaabrirá si cerramos antes.
  while (handleClient(c)) {
  }
  lwip_close(c);
}

bool ModbusTcpManager::handleClient(int c) {
  uint8_t buf[260];
  const int n = lwip_recv(c, buf, sizeof(buf), 0);
  if (n < 8) {
    return false;  // MBAP(7) + fn(1) mínimo; trama incompleta/EOF → cerrar
  }
  const uint16_t tid = (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
  const uint8_t unit = buf[6];
  const uint8_t fn = buf[7];

  uint8_t pdu[258];
  memset(pdu, 0, sizeof(pdu));
  uint16_t plen = 0;

  switch (fn) {
    case 0x01: {  // Read Coils
      if (n < 12) {
        return false;
      }
      const uint16_t start = (static_cast<uint16_t>(buf[8]) << 8) | buf[9];
      const uint16_t qty = (static_cast<uint16_t>(buf[10]) << 8) | buf[11];
      if (qty == 0 || qty > 0x07D0 || start + qty > kCoilCount) {
        pdu[0] = 0x81;
        pdu[1] = 0x02;  // ILLEGAL DATA ADDRESS
        plen = 2;
        break;
      }
      const uint8_t byteCount = static_cast<uint8_t>((qty + 7) / 8);
      pdu[0] = 0x01;
      pdu[1] = byteCount;
      for (uint16_t i = 0; i < qty; ++i) {
        if (coils_[start + i]) {
          pdu[2 + (i >> 3)] |= static_cast<uint8_t>(1u << (i & 7));
        }
      }
      plen = static_cast<uint16_t>(2 + byteCount);
      break;
    }
    case 0x03: {  // Read Holding Registers
      if (n < 12) {
        return false;
      }
      const uint16_t start = (static_cast<uint16_t>(buf[8]) << 8) | buf[9];
      const uint16_t qty = (static_cast<uint16_t>(buf[10]) << 8) | buf[11];
      if (qty == 0 || qty > 0x007D || start + qty > kHoldingCount) {
        pdu[0] = 0x83;
        pdu[1] = 0x02;  // ILLEGAL DATA ADDRESS
        plen = 2;
        break;
      }
      const uint8_t byteCount = static_cast<uint8_t>(qty * 2);
      pdu[0] = 0x03;
      pdu[1] = byteCount;
      for (uint16_t i = 0; i < qty; ++i) {
        const uint16_t v = holding_[start + i];
        pdu[2 + i * 2] = static_cast<uint8_t>(v >> 8);
        pdu[3 + i * 2] = static_cast<uint8_t>(v & 0xFF);
      }
      plen = static_cast<uint16_t>(2 + byteCount);
      break;
    }
    case 0x05: {  // Write Single Coil
      if (n < 12) {
        return false;
      }
      const uint16_t addr = (static_cast<uint16_t>(buf[8]) << 8) | buf[9];
      const uint16_t val = (static_cast<uint16_t>(buf[10]) << 8) | buf[11];
      if (addr >= kCoilCount || (val != 0x0000 && val != 0xFF00)) {
        pdu[0] = 0x85;
        pdu[1] = 0x02;  // ILLEGAL DATA ADDRESS
        plen = 2;
        break;
      }
      coils_[addr] = (val == 0xFF00);
      pdu[0] = 0x05;
      pdu[1] = buf[8];
      pdu[2] = buf[9];
      pdu[3] = buf[10];
      pdu[4] = buf[11];  // eco
      plen = 5;
      break;
    }
    case 0x06: {  // Write Single Register
      if (n < 12) {
        return false;
      }
      const uint16_t addr = (static_cast<uint16_t>(buf[8]) << 8) | buf[9];
      const uint16_t val = (static_cast<uint16_t>(buf[10]) << 8) | buf[11];
      if (addr >= kHoldingCount) {
        pdu[0] = 0x86;
        pdu[1] = 0x02;  // ILLEGAL DATA ADDRESS
        plen = 2;
        break;
      }
      holding_[addr] = val;
      pdu[0] = 0x06;
      pdu[1] = buf[8];
      pdu[2] = buf[9];
      pdu[3] = buf[10];
      pdu[4] = buf[11];  // eco
      plen = 5;
      break;
    }
    default: {  // función no soportada
      pdu[0] = static_cast<uint8_t>(fn | 0x80);
      pdu[1] = 0x01;  // ILLEGAL FUNCTION
      plen = 2;
      break;
    }
  }
  return sendPdu(c, tid, unit, pdu, plen);
}

void ModbusTcpManager::setHoldingRegister(uint16_t idx, uint16_t value) {
  if (idx < kHoldingCount) {
    holding_[idx] = value;
  }
}

uint16_t ModbusTcpManager::holdingRegister(uint16_t idx) const {
  return (idx < kHoldingCount) ? holding_[idx] : 0;
}

void ModbusTcpManager::setCoil(uint16_t idx, bool on) {
  if (idx < kCoilCount) {
    coils_[idx] = on;
  }
}

bool ModbusTcpManager::coil(uint16_t idx) const {
  return (idx < kCoilCount) ? coils_[idx] : false;
}

}  // namespace base
#endif  // SEMA_USE_MODBUS_TCP
