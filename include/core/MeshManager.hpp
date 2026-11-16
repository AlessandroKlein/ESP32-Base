#pragma once

#include <ArduinoJson.h>
#include <cstdint>
#include <esp_now.h>  // esp_now_recv_info_t / esp_now_send_info_t (callbacks privados)

#include "core/ConfigManager.hpp"

// =============================================================================
// SEMA — MeshManager (capa de malla ESP-NOW, maestro/esclavo)
// =============================================================================
// Comunicación de corto alcance entre nodos ESP32 sobre ESP-NOW (radio WiFi sin
// IP). No depende de Ethernet ni de la pila TCP/IP: ESP-NOW transmite frames
// 802.11 directamente entre MACs, así que funciona con el radio encendido aunque
// no haya red WiFi configurada. Es el transporte de la malla de nodos (Fase 3).
//
// Roles (MeshConfig.role): 0=off, 1=maestro (agrega esclavos), 2=esclavo
// (publica mediciones/estado al maestro). El maestro y los esclavos se
// descubren por ANNOUNCE broadcast (MAC FF:FF:FF:FF:FF:FF); la lista `peerMacs`
// fija los pares conocidos sin necesidad de descubrimiento.
//
// Callbacks: ESP-NOW invoca los callbacks en el task de WiFi (no en loop()), así
// que la recepción solo copia a una cola ring SPSC (best-effort) y loop() la
// drena. Ver popRx().
//
// Coste (medido al compilar): ver resultado de pio run; el protocolo es ~250 B
// de cola por slot (8 slots) + tablas de peers (16).
//
// Licencia: ESP-NOW es parte del framework arduino-esp32 (Apache-2.0). Este
// manager es código propio; sin dependencias externas.

namespace base {

// =============================================================================
// Protocolo de mensajes (binario, versionado y extensible)
// =============================================================================
// Cabecera fija de 12 bytes + payload variable (máx 238 → frame de 250 bytes,
// el límite ESP_NOW v1.0). Todo es byte-level y little-endian; los dos targets
// (Xtensa ESP32 y RISC-V C6/H2) son little-endian, y el `float` se serializa
// como IEEE754 little-endian por copia de bits (memcpy), nunca por reinterpret
// directo de un puntero no alineado.
//
//   offset 0..1   : magic "SM" (0x53 0x4D)
//   offset 2      : version (1)
//   offset 3      : type (ver Type)
//   offset 4..9   : MAC origen (6 bytes, orden de impresión "AA:BB:CC:DD:EE:FF")
//   offset 10     : seq (monotónico por emisor, 0..255, wrap)
//   offset 11     : longitud del payload (N, 0..238)
//   offset 12..   : payload (formato según el tipo)
//
// Payloads:
//   ANNOUNCE    (0x01): [role:1][boardId: string 1B len + chars][version: string]
//                       lo emite el esclavo para darse a conocer (descubrimiento).
//   SENSOR_DATA (0x02): secuencia de entradas, hasta llenar el payload:
//                       [nameLen:1][name chars][value: float32 LE 4B][unitLen:1][unit chars]
//                       esclavo → maestro.
//   COMMAND     (0x03): [cmdLen:1][cmd chars][args: resto del payload como texto]
//                       maestro → esclavo.
//   ACK         (0x04): [seqAcked:1][status:1]  (0=ok, !=0 error)
//   HEARTBEAT   (0x05): sin payload (0 bytes). Latido periódico de ambos roles.
// =============================================================================

class MeshManager {
public:
  static constexpr uint8_t kHeaderLen = 12;   // magic(2)+ver(1)+type(1)+mac(6)+seq(1)+len(1)
  static constexpr uint8_t kMaxFrame = 250;   // ESP_NOW_MAX_DATA_LEN (v1.0)
  static constexpr uint8_t kMaxPayload = kMaxFrame - kHeaderLen;  // 238
  static constexpr uint8_t kMaxPeers = 16;    // coincide con MeshConfig.peerMacs[16]
  static constexpr uint8_t kRxQueue = 8;      // slots de la cola de recepción

  enum Type : uint8_t {
    ANNOUNCE = 0x01,     // esclavo → maestro: rol, board id, versión
    SENSOR_DATA = 0x02,  // esclavo → maestro: {magnitud, valor float, unidad}
    COMMAND = 0x03,      // maestro → esclavo: comando + args
    ACK = 0x04,          // confirmación de recepción
    HEARTBEAT = 0x05     // latido periódico (ambos roles)
  };

  // Mensaje recibido (drenado con popRx). `payload` es el cuerpo crudo del frame.
  struct RxMessage {
    uint8_t src[6];          // MAC origen
    uint8_t type;            // Type
    uint8_t seq;             // secuencia del emisor
    uint8_t len;             // longitud útil del payload (0..238)
    uint8_t payload[kMaxPayload];
  };

  // Reconfiguración idempotente: detiene (si corría) y arranca si enabled.
  void apply(const MeshConfig& cfg);

  void begin(const MeshConfig& cfg);  // init esp_now + callbacks + peers
  void stop();                        // esp_now_deinit + des-registra
  void loop();                        // heartbeat/announce periódico

  // Envío crudo por ESP-NOW. `mac == nullptr` envía a TODOS los peers añadidos.
  // `data` son los bytes completos del frame (o cualquier payload si el caller
  // ya construyó el header). Devuelve false solo si la cola de ESP-NOW está llena.
  bool send(const uint8_t* mac, const uint8_t* data, size_t len);

  // Construye un frame MeshProtocol (magic+version+type+MAC+seq+len+payload) y lo
  // envía a `mac` (nullptr = broadcast a peers). Es el punto de entrada típico.
  bool sendFrame(const uint8_t* mac, uint8_t type, const uint8_t* payload, size_t len);
  bool broadcast(uint8_t type, const uint8_t* payload, size_t len);
  bool sendHeartbeat();                                  // HEARTBEAT broadcast
  bool sendAnnounce(const char* boardId, const char* version);  // ANNOUNCE broadcast

  // Recepción: drena un mensaje de la cola; false = cola vacía.
  bool popRx(RxMessage& out);

  const uint8_t* lastPeer() const { return lastPeer_; }   // MAC del último emisor
  const uint8_t* lastData() const { return lastData_; }   // payload del último frame
  uint8_t lastDataLen() const { return lastDataLen_; }
  uint8_t lastType() const { return lastType_; }

  uint32_t rxCount() const { return rxCount_; }
  uint32_t txCount() const { return txCount_; }
  uint32_t errCount() const { return errCount_; }
  bool ready() const { return ready_; }
  const MeshConfig& config() const { return cfg_; }

  // Rellena el estado para la web (/api/v1/mesh). `doc` ya debe existir.
  void statusJson(JsonDocument& doc);

  // Utilidades wire-format (públicas para Fase 2b/3 y tests).
  static size_t buildFrame(uint8_t* out, uint8_t type, const uint8_t* srcMac,
                           uint8_t seq, const uint8_t* payload, size_t len);
  static bool parseMac(const char* str, uint8_t mac[6]);  // "AA:BB:CC:DD:EE:FF"
  static void macToStr(const uint8_t mac[6], char* out, size_t n);

private:
  // Callbacks estáticos (C ABI) que reenvían a la instancia activa.
  static void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len);
  static void onSend(const esp_now_send_info_t* tx_info, esp_now_send_status_t status);

  void handleRecv(const uint8_t* srcMac, const uint8_t* data, int len);
  bool addPeer(const uint8_t mac[6], uint8_t channel);
  bool addPeers();

  MeshConfig cfg_;
  bool ready_ = false;
  uint8_t ownMac_[6] = {0};       // MAC STA (origen de nuestros frames)
  uint8_t seq_ = 0;               // secuencia monotónica de envío
  uint8_t peersAdded_ = 0;        // peers realmente registrados
  uint32_t rxCount_ = 0;
  uint32_t txCount_ = 0;
  uint32_t errCount_ = 0;
  uint8_t lastPeer_[6] = {0};
  uint8_t lastData_[kMaxPayload] = {0};
  uint8_t lastDataLen_ = 0;
  uint8_t lastType_ = 0;
  // Cola SPSC: el callback (task WiFi) escribe en rxWrite_, loop() lee rxRead_.
  RxMessage rx_[kRxQueue];
  volatile uint8_t rxWrite_ = 0;
  volatile uint8_t rxRead_ = 0;
  uint32_t lastSendMs_ = 0;       // último envío periódico (millis)
  bool sendOk_ = true;            // último estado del send callback
};

}  // namespace base
