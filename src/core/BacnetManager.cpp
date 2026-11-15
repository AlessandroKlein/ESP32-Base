#include "core/BacnetManager.hpp"

#include "base/board_config.h"

#if SEMA_USE_BACNET
#include <Arduino.h>
#include <lwip/sockets.h>
#include <string.h>

namespace base {

// =============================================================================
// Wire format BACnet/IP (verificado contra bacnet-stack src/portable/BacnetProtocol.cpp).
// Layout de un datagrama (todos big-endian):
//   BVLC : [0]=0x81 type BACnet/IP, [1]=0x0A unicast | 0x0B broadcast,
//          [2..3]=longitud total (incluye los 4 bytes BVLC)
//   NPDU : [4]=0x01 versión, [5]=control (bit7 SNET, bit5 DNET, bit3 SA presente)
//   APDU : [6]=tipo PDU, ... (confirmed 0x00 / unconfirmed 0x10 / complexAck 0x30)
// El tag ASN.1: (tagNumber<<4) | nibble-bajo. Aplicación: 0..4 = longitud del
// valor; contexto primitivo: 0x8..0xC; 0xE apertura / 0xF cierre.

namespace {

// ---- constantes BACnet (ids de objeto/propiedad/error estándar) ----
constexpr uint8_t kObjAnalogInput = 0;
constexpr uint8_t kObjBinaryInput = 3;
constexpr uint8_t kObjDevice = 8;

// Propiedades (identificadores estándar BACnet).
constexpr uint8_t kPropApplicationSoftwareVersion = 12;
constexpr uint8_t kPropFirmwareRevision = 44;
constexpr uint8_t kPropMaxApduLengthAccepted = 62;
constexpr uint8_t kPropModelName = 70;
constexpr uint8_t kPropObjectIdentifier = 75;
constexpr uint8_t kPropObjectList = 76;
constexpr uint8_t kPropObjectName = 77;
constexpr uint8_t kPropObjectType = 79;
constexpr uint8_t kPropPresentValue = 85;
constexpr uint8_t kPropProtocolVersion = 98;
constexpr uint8_t kPropSegmentationSupported = 107;
constexpr uint8_t kPropStatusFlags = 111;
constexpr uint8_t kPropSystemStatus = 112;
constexpr uint8_t kPropUnits = 117;
constexpr uint8_t kPropVendorIdentifier = 120;
constexpr uint8_t kPropVendorName = 121;
constexpr uint8_t kPropProtocolRevision = 139;

// Clases y códigos de error BACnet.
constexpr uint8_t kErrClassObject = 1;
constexpr uint8_t kErrClassProperty = 2;
constexpr uint8_t kErrClassServices = 6;
constexpr uint8_t kErrUnknownObject = 31;
constexpr uint8_t kErrUnknownProperty = 32;
constexpr uint8_t kErrOptionalNotSupported = 45;

// ---- escritores de primitivas (aplicación, contexto y red) ----

// Unsigned/Enumerated de aplicación: tag 2/9, longitud mínima 1..4, big-endian.
void putAppInt(uint8_t* b, size_t& o, uint8_t tag, uint32_t v) {
  uint8_t len = 1;
  if (v > 0xFF) len = 2;
  if (v > 0xFFFF) len = 3;
  if (v > 0xFFFFFF) len = 4;
  b[o++] = static_cast<uint8_t>((tag << 4) | len);
  for (uint8_t i = len; i-- > 0;) {
    b[o++] = static_cast<uint8_t>(v >> (i * 8));
  }
}

// Real de aplicación: 0x44 + 4 bytes IEEE754 big-endian.
void putReal(uint8_t* b, size_t& o, float v) {
  uint32_t raw = 0;
  memcpy(&raw, &v, sizeof(raw));
  b[o++] = 0x44;
  b[o++] = static_cast<uint8_t>(raw >> 24);
  b[o++] = static_cast<uint8_t>(raw >> 16);
  b[o++] = static_cast<uint8_t>(raw >> 8);
  b[o++] = static_cast<uint8_t>(raw);
}

// CharacterString de aplicación: tag 7 + [longitud] + charset 0x00 + texto.
void putCharString(uint8_t* b, size_t& o, const char* s) {
  const size_t n = strlen(s);
  const size_t payload = n + 1;  // +1 byte de charset (0x00 = ANSI X3.4/UTF-8)
  if (payload <= 4) {
    b[o++] = static_cast<uint8_t>((7 << 4) | payload);
  } else {
    b[o++] = 0x75;  // longitud extendida
    b[o++] = static_cast<uint8_t>(payload);
  }
  b[o++] = 0x00;  // character set: ANSI X3.4
  memcpy(&b[o], s, n);
  o += n;
}

// ObjectIdentifier de aplicación: 0xC4 + 4 bytes ((type<<22)|instance).
void putObjectId(uint8_t* b, size_t& o, uint8_t type, uint32_t instance) {
  const uint32_t id = (static_cast<uint32_t>(type) << 22) | (instance & 0x003FFFFF);
  b[o++] = 0xC4;
  b[o++] = static_cast<uint8_t>(id >> 24);
  b[o++] = static_cast<uint8_t>(id >> 16);
  b[o++] = static_cast<uint8_t>(id >> 8);
  b[o++] = static_cast<uint8_t>(id);
}

// ObjectIdentifier de contexto (tag N, 4 bytes): (N<<4)|0x0C + 4 bytes.
void putContextObjectId(uint8_t* b, size_t& o, uint8_t tag, uint8_t type,
                        uint32_t instance) {
  const uint32_t id = (static_cast<uint32_t>(type) << 22) | (instance & 0x003FFFFF);
  b[o++] = static_cast<uint8_t>((tag << 4) | 0x0C);
  b[o++] = static_cast<uint8_t>(id >> 24);
  b[o++] = static_cast<uint8_t>(id >> 16);
  b[o++] = static_cast<uint8_t>(id >> 8);
  b[o++] = static_cast<uint8_t>(id);
}

// Unsigned de contexto (tag N, 1/2/4 bytes): (N<<4)|0x09/0x0A/0x0C.
void putContextUnsigned(uint8_t* b, size_t& o, uint8_t tag, uint32_t v) {
  if (v <= 0xFF) {
    b[o++] = static_cast<uint8_t>((tag << 4) | 0x09);
    b[o++] = static_cast<uint8_t>(v);
  } else if (v <= 0xFFFF) {
    b[o++] = static_cast<uint8_t>((tag << 4) | 0x0A);
    b[o++] = static_cast<uint8_t>(v >> 8);
    b[o++] = static_cast<uint8_t>(v);
  } else {
    b[o++] = static_cast<uint8_t>((tag << 4) | 0x0C);
    b[o++] = static_cast<uint8_t>(v >> 24);
    b[o++] = static_cast<uint8_t>(v >> 16);
    b[o++] = static_cast<uint8_t>(v >> 8);
    b[o++] = static_cast<uint8_t>(v);
  }
}

// BitString de aplicación (tag 8): (8<<4)|(1+nbytes) + bits-sin-usar + datos.
// `bits` y `value` definen un bitstring de `bits` bits (máx 8, una octeto).
void putBitString(uint8_t* b, size_t& o, uint8_t bits, uint8_t value) {
  b[o++] = 0x82;                          // longitud = 2 octetos de contenido
  b[o++] = static_cast<uint8_t>(8 - bits);  // bits sin usar en el último octeto
  b[o++] = value;
}

// Escribe la cabecera BVLC+NPDU de una respuesta unicast y devuelve el offset
// para escribir el APDU; la longitud BVLC se rellena después con `finalize`.
size_t startResponse(uint8_t* b, uint8_t npduControl) {
  size_t o = 0;
  b[o++] = 0x81;          // BVLC type: BACnet/IP
  b[o++] = 0x0A;          // BVLC function: unicast NPDU
  o += 2;                 // longitud BVLC (se rellena al final)
  b[o++] = 0x01;          // NPDU: versión 1
  b[o++] = npduControl;   // 0x00 = sin dirección (respuesta simple)
  return o;
}

void finalize(uint8_t* b, size_t o) {
  b[2] = static_cast<uint8_t>(o >> 8);
  b[3] = static_cast<uint8_t>(o);
}

}  // namespace

uint16_t BacnetManager::engineeringUnits(const char* unit, const char* name) {
  if (unit == nullptr) return 95;
  if (strcmp(unit, "degC") == 0) return 62;     // degrees Celsius
  if (strcmp(unit, "degF") == 0) return 64;     // degrees Fahrenheit
  if (strcmp(unit, "hPa") == 0) return 57;      // millibars (hectopascals)
  if (strcmp(unit, "W/m2") == 0) return 50;     // watts per square meter
  if (strcmp(unit, "lux") == 0) return 36;      // lux
  if (strcmp(unit, "mm") == 0) return 32;       // millimeters
  if (strcmp(unit, "m/s") == 0) return 66;      // meters per second
  if (strcmp(unit, "deg") == 0) return 60;      // degrees angular
  if (strcmp(unit, "V") == 0) return 5;         // volts
  if (strcmp(unit, "km") == 0) return 35;       // kilometers
  if (strcmp(unit, "ppm") == 0) return 96;      // parts per million
  if (strcmp(unit, "ppb") == 0) return 97;      // parts per billion
  if (strcmp(unit, "percent") == 0) {
    // La humedad relativa se expone como %RH (29); el resto es porcentaje (98).
    if (name != nullptr &&
        (strstr(name, "humid") != nullptr || strstr(name, "rh") != nullptr ||
         strstr(name, "humidity") != nullptr)) {
      return 29;  // percent relative humidity
    }
    return 98;  // percent
  }
  return 95;  // No Units
}

void BacnetManager::apply(const BacnetConfig& cfg) {
  cfg_ = cfg;
  ready_ = false;
  if (fd_ >= 0) {
    lwip_close(fd_);
    fd_ = -1;
  }
  if (!cfg_.enabled) {
    return;
  }

  const int fd = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd < 0) {
    return;
  }
  int one = 1;
  lwip_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(cfg_.udpPort);
  addr.sin_addr.s_addr = INADDR_ANY;  // escucha en TODAS las interfaces activas
  if (lwip_bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
    lwip_close(fd);
    return;
  }
  int on = 1;
  lwip_ioctl(fd, FIONBIO, &on);  // recvfrom no bloqueante
  fd_ = fd;
  ready_ = true;
}

void BacnetManager::loop() {
  if (!ready_ || fd_ < 0) {
    return;
  }
  uint8_t buf[512];
  struct sockaddr_in src = {};
  socklen_t slen = sizeof(src);
  const int n = lwip_recvfrom(fd_, buf, sizeof(buf), 0,
                              reinterpret_cast<struct sockaddr*>(&src), &slen);
  if (n < 6) {
    return;  // sin datagrama pendiente o demasiado corto
  }
  handlePacket(buf, static_cast<uint16_t>(n), src);
}

void BacnetManager::handlePacket(const uint8_t* buf, uint16_t n,
                                 const struct sockaddr_in& src) {
  if (buf[0] != 0x81) return;  // no es BACnet/IP
  const uint16_t bvlcLen = (static_cast<uint16_t>(buf[2]) << 8) | buf[3];
  if (bvlcLen != n) return;  // longitud BVLC inconsistente

  size_t o = 4;
  if (buf[o++] != 0x01) return;  // NPDU versión
  const uint8_t npdu = buf[o++];

  // Salta direcciones NPDU presentes (DNET/DADR, SA) según el control.
  if (npdu & 0x80) return;  // SNET (mensaje de capa de red) → no lo manejamos
  if (npdu & 0x20) {        // DNET+DADR presentes
    if (o + 3 > n) return;
    o += 2;
    const uint8_t alen = buf[o++];
    if (o + alen > n) return;
    o += alen;
  }
  if (npdu & 0x08) {  // SA presente
    if (o + 1 > n) return;
    const uint8_t alen = buf[o++];
    if (o + alen > n) return;
    o += alen;
  }

  if (o >= n) return;
  const uint8_t apdu = buf[o++];
  const uint8_t pduType = apdu >> 4;  // 0=confirmed 1=unconfirmed 3=complexAck 5=error

  if (pduType == 1) {  // Unconfirmed-Request
    if (o >= n) return;
    const uint8_t service = buf[o++];
    if (service == 0x08) {  // Who-Is → responder I-Am
      sendIAm(src);
    }
    return;
  }

  if (pduType == 0) {  // Confirmed-Request
    if (o + 3 > n) return;
    o += 1;                  // max-segments-accepted (ignorado)
    o += 1;                  // max-apdu-length-accepted (ignorado)
    const uint8_t invokeId = buf[o++];
    const uint8_t service = buf[o++];
    if (service == 0x0C) {  // ReadProperty
      handleReadProperty(buf + o, static_cast<uint16_t>(n - o), invokeId, src);
    } else {
      sendError(src, invokeId, kErrClassServices, kErrOptionalNotSupported);
    }
    return;
  }
  // complexAck/error entrantes (no somos cliente) → ignorar.
}

void BacnetManager::handleReadProperty(const uint8_t* p, uint16_t len,
                                       uint8_t invokeId,
                                       const struct sockaddr_in& src) {
  // object-identifier (context tag 0, 4 bytes) + property-id (context tag 1, 1 byte).
  if (len < 7 || p[0] != 0x0C || p[5] != 0x19) {
    return;  // petición malformada → sin respuesta
  }
  const uint32_t objId = (static_cast<uint32_t>(p[1]) << 24) |
                         (static_cast<uint32_t>(p[2]) << 16) |
                         (static_cast<uint32_t>(p[3]) << 8) | p[4];
  const uint8_t objType = static_cast<uint8_t>(objId >> 22);
  const uint32_t objInst = objId & 0x003FFFFF;
  const uint8_t propId = p[6];

  // array-index opcional (context tag 2). Solo se usa para ObjectList.
  bool hasArrayIndex = false;
  uint32_t arrayIndex = 0;
  size_t o = 7;
  if (o < len && ((p[o] >> 4) == 2) && (p[o] & 0x08)) {
    const uint8_t alen = p[o] & 0x07;
    if (alen == 0 || o + 1 + alen > len) return;
    arrayIndex = 0;
    for (uint8_t i = 0; i < alen; ++i) {
      arrayIndex = (arrayIndex << 8) | p[o + 1 + i];
    }
    hasArrayIndex = true;
    o += 1 + alen;
  }
  (void)o;

  // Resuelve el objeto pedido.
  //   Device      : type 8, instance == cfg_.deviceInstance
  //   AnalogInput : type 0, instance 1..analogCount_
  //   BinaryInput : type 3, instance 1..binaryCount_
  if (objType != kObjDevice && objType != kObjAnalogInput &&
      objType != kObjBinaryInput) {
    sendError(src, invokeId, kErrClassObject, kErrUnknownObject);
    return;
  }
  const int32_t slot = static_cast<int32_t>(objInst) - 1;  // instancia N → índice N-1
  const bool isDevice = (objType == kObjDevice);
  if (isDevice && objInst != cfg_.deviceInstance) {
    sendError(src, invokeId, kErrClassObject, kErrUnknownObject);
    return;
  }
  if (objType == kObjAnalogInput && (slot < 0 || slot >= analogCount_)) {
    sendError(src, invokeId, kErrClassObject, kErrUnknownObject);
    return;
  }
  if (objType == kObjBinaryInput && (slot < 0 || slot >= binaryCount_)) {
    sendError(src, invokeId, kErrClassObject, kErrUnknownObject);
    return;
  }

  // Construye la respuesta ComplexAck: BVLC + NPDU + APDU(0x30 invokeId 0x0C)
  // + context0 object-id + context1 property-id + [context2 array-index]
  // + opening-tag-3 (0x3E) + valor + closing-tag-3 (0x3F).
  uint8_t out[512];
  size_t w = startResponse(out, 0x00);
  out[w++] = 0x30;          // ComplexAck
  out[w++] = invokeId;
  out[w++] = 0x0C;          // service: ReadProperty
  putContextObjectId(out, w, 0, objType, objInst);
  putContextUnsigned(out, w, 1, propId);
  if (hasArrayIndex) {
    putContextUnsigned(out, w, 2, arrayIndex);
  }
  out[w++] = 0x3E;  // opening tag 3 (propertyValue)

  // Encoda el valor de la propiedad. Devuelve false si la propiedad no se
  // soporta para este objeto (→ Unknown Property).
  bool ok = true;
  switch (propId) {
    case kPropObjectIdentifier: {
      if (hasArrayIndex) { ok = false; break; }
      putObjectId(out, w, objType, objInst);
      break;
    }
    case kPropObjectName: {
      if (hasArrayIndex) { ok = false; break; }
      const char* nm = isDevice ? "SEMA"
                       : (objType == kObjAnalogInput ? analog_[slot].name
                                                     : binaryName_[slot]);
      putCharString(out, w, nm);
      break;
    }
    case kPropObjectType: {
      if (hasArrayIndex) { ok = false; break; }
      putAppInt(out, w, 9, objType);  // Enumerated
      break;
    }
    case kPropPresentValue: {
      if (hasArrayIndex) { ok = false; break; }
      if (objType == kObjAnalogInput) {
        putReal(out, w, analog_[slot].value);
      } else if (objType == kObjBinaryInput) {
        putAppInt(out, w, 9, binary_[slot] ? 1u : 0u);  // Enumerated 0/1
      } else {
        ok = false;  // Device no tiene PresentValue
      }
      break;
    }
    case kPropStatusFlags: {
      if (hasArrayIndex) { ok = false; break; }
      if (isDevice) { ok = false; break; }
      putBitString(out, w, 4, 0x00);  // 4 bits, todos en 0 (sin alarma/fallo)
      break;
    }
    case kPropUnits: {
      if (hasArrayIndex) { ok = false; break; }
      if (objType != kObjAnalogInput) { ok = false; break; }
      putAppInt(out, w, 9, analog_[slot].units);  // Enumerated
      break;
    }
    case kPropObjectList: {
      if (!isDevice) { ok = false; break; }
      // ObjectList = BACnetARRAY: opening-tag-0 + ObjectIdentifiers + closing-tag-0.
      if (hasArrayIndex) {
        // Lectura por índice: devuelve solo ese elemento (sin tags de array).
        const uint32_t total = 1 + analogCount_ + binaryCount_;  // Device + AI + BI
        if (arrayIndex == 0) {
          putObjectId(out, w, kObjDevice, cfg_.deviceInstance);
        } else if (arrayIndex >= 1 && arrayIndex <= analogCount_) {
          putObjectId(out, w, kObjAnalogInput, arrayIndex);
        } else if (arrayIndex >= analogCount_ + 1 && arrayIndex <= total - 1) {
          putObjectId(out, w, kObjBinaryInput,
                      arrayIndex - static_cast<uint32_t>(analogCount_));
        } else {
          ok = false;  // índice fuera de rango → tratado como propiedad inválida
        }
      } else {
        out[w++] = 0x0E;  // opening tag 0 (array)
        putObjectId(out, w, kObjDevice, cfg_.deviceInstance);
        for (uint16_t i = 0; i < analogCount_; ++i) {
          putObjectId(out, w, kObjAnalogInput, i + 1);
        }
        for (uint16_t i = 0; i < binaryCount_; ++i) {
          putObjectId(out, w, kObjBinaryInput, i + 1);
        }
        out[w++] = 0x0F;  // closing tag 0
      }
      break;
    }
    case kPropSystemStatus: {
      if (!isDevice || hasArrayIndex) { ok = false; break; }
      putAppInt(out, w, 9, 0);  // operational
      break;
    }
    case kPropVendorIdentifier: {
      if (!isDevice || hasArrayIndex) { ok = false; break; }
      putAppInt(out, w, 2, 0);  // vendor 0 (sin registro formal)
      break;
    }
    case kPropVendorName: {
      if (!isDevice || hasArrayIndex) { ok = false; break; }
      putCharString(out, w, "SEMA");
      break;
    }
    case kPropModelName: {
      if (!isDevice || hasArrayIndex) { ok = false; break; }
      putCharString(out, w, "SEMA Weather Station");
      break;
    }
    case kPropFirmwareRevision: {
      if (!isDevice || hasArrayIndex) { ok = false; break; }
      putCharString(out, w, "1.0");
      break;
    }
    case kPropApplicationSoftwareVersion: {
      if (!isDevice || hasArrayIndex) { ok = false; break; }
      putCharString(out, w, "1.0");
      break;
    }
    case kPropMaxApduLengthAccepted: {
      if (!isDevice || hasArrayIndex) { ok = false; break; }
      putAppInt(out, w, 2, kMaxApdu);
      break;
    }
    case kPropSegmentationSupported: {
      if (!isDevice || hasArrayIndex) { ok = false; break; }
      putAppInt(out, w, 9, 3);  // no-segmentation
      break;
    }
    case kPropProtocolVersion: {
      if (!isDevice || hasArrayIndex) { ok = false; break; }
      putAppInt(out, w, 2, 1);
      break;
    }
    case kPropProtocolRevision: {
      if (!isDevice || hasArrayIndex) { ok = false; break; }
      putAppInt(out, w, 2, 24);  // BACnet protocol revision 24 (version 1)
      break;
    }
    default:
      ok = false;
      break;
  }

  if (!ok) {
    sendError(src, invokeId, kErrClassProperty, kErrUnknownProperty);
    return;
  }
  out[w++] = 0x3F;  // closing tag 3
  finalize(out, w);
  sendTo(src, out, static_cast<uint16_t>(w));
}

void BacnetManager::sendIAm(const struct sockaddr_in& src) {
  // I-Am (unconfirmed, service 0x00): APDU 0x10 0x00 + ObjectIdentifier(Device)
  // + Unsigned(maxApdu) + Enumerated(segmentation=3) + Unsigned(vendorId).
  uint8_t out[64];
  size_t w = startResponse(out, 0x00);
  out[w++] = 0x10;  // Unconfirmed-Request
  out[w++] = 0x00;  // service: I-Am
  putObjectId(out, w, kObjDevice, cfg_.deviceInstance);
  putAppInt(out, w, 2, kMaxApdu);  // max APDU length accepted
  putAppInt(out, w, 9, 3);         // segmentation: no-segmentation
  putAppInt(out, w, 2, 0);         // vendor id
  finalize(out, w);
  sendTo(src, out, static_cast<uint16_t>(w));
}

void BacnetManager::sendError(const struct sockaddr_in& src, uint8_t invokeId,
                              uint8_t errClass, uint8_t errCode) {
  // Error-PDU: 0x50 invokeId service(0x0C) + Enumerated(errorClass) + Enumerated(errorCode).
  uint8_t out[64];
  size_t w = startResponse(out, 0x00);
  out[w++] = 0x50;  // Error
  out[w++] = invokeId;
  out[w++] = 0x0C;  // service rechazado: ReadProperty
  putAppInt(out, w, 9, errClass);
  putAppInt(out, w, 9, errCode);
  finalize(out, w);
  sendTo(src, out, static_cast<uint16_t>(w));
}

bool BacnetManager::sendTo(const struct sockaddr_in& dst, const uint8_t* data,
                           uint16_t len) {
  return lwip_sendto(fd_, data, len, 0,
                     reinterpret_cast<const struct sockaddr*>(&dst),
                     sizeof(dst)) >= 0;
}

void BacnetManager::setAnalogCount(uint16_t n) {
  analogCount_ = (n > kMaxAnalog) ? kMaxAnalog : n;
}

void BacnetManager::setAnalogInput(uint16_t idx, const char* name, float value,
                                   uint16_t units) {
  if (idx >= analogCount_) return;
  AnalogPoint& p = analog_[idx];
  p.value = value;
  p.units = units;
  p.name[0] = '\0';
  if (name != nullptr) {
    strncpy(p.name, name, sizeof(p.name) - 1);
    p.name[sizeof(p.name) - 1] = '\0';
  }
}

void BacnetManager::setBinaryCount(uint16_t n) {
  binaryCount_ = (n > kMaxBinary) ? kMaxBinary : n;
}

void BacnetManager::setBinaryInput(uint16_t idx, const char* name, bool on) {
  if (idx >= binaryCount_) return;
  binary_[idx] = on;
  binaryName_[idx][0] = '\0';
  if (name != nullptr) {
    strncpy(binaryName_[idx], name, sizeof(binaryName_[idx]) - 1);
    binaryName_[idx][sizeof(binaryName_[idx]) - 1] = '\0';
  }
}

}  // namespace base
#endif  // SEMA_USE_BACNET
