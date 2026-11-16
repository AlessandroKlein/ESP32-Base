#include "core/MeshManager.hpp"

#include "base/board_config.h"

#if SEMA_USE_MESH_ESPNOW

#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <stdio.h>
#include <string.h>

namespace base {

namespace {

// Instancia activa para que los callbacks C (sin contexto) puedan reenviar.
// Solo hay un MeshManager en SemaCore, así que un puntero global alcanza.
MeshManager* gMesh = nullptr;

}  // namespace

// -----------------------------------------------------------------------------
// Utilidades wire-format
// -----------------------------------------------------------------------------

size_t MeshManager::buildFrame(uint8_t* out, uint8_t type, const uint8_t* srcMac,
                               uint8_t seq, const uint8_t* payload, size_t len) {
  if (len > kMaxPayload) {
    len = kMaxPayload;
  }
  out[0] = 0x53;  // 'S'
  out[1] = 0x4D;  // 'M'
  out[2] = 1;     // version
  out[3] = type;
  memcpy(out + 4, srcMac, 6);
  out[10] = seq;
  out[11] = static_cast<uint8_t>(len);
  if (len) {
    memcpy(out + kHeaderLen, payload, len);
  }
  return kHeaderLen + len;
}

bool MeshManager::parseMac(const char* str, uint8_t mac[6]) {
  if (str == nullptr) {
    return false;
  }
  unsigned m[6] = {0, 0, 0, 0, 0, 0};
  const int n = sscanf(str, "%x:%x:%x:%x:%x:%x", &m[0], &m[1], &m[2], &m[3],
                       &m[4], &m[5]);
  if (n != 6) {
    return false;
  }
  for (uint8_t i = 0; i < 6; ++i) {
    mac[i] = static_cast<uint8_t>(m[i]);
  }
  return true;
}

void MeshManager::macToStr(const uint8_t mac[6], char* out, size_t n) {
  snprintf(out, n, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
}

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

void MeshManager::apply(const MeshConfig& cfg) {
  stop();
  cfg_ = cfg;
  if (cfg_.enabled && cfg_.role != 0) {
    begin(cfg_);
  }
}

void MeshManager::begin(const MeshConfig& cfg) {
  cfg_ = cfg;
  if (!cfg_.enabled || cfg_.role == 0) {
    return;
  }

  // ESP-NOW exige el radio WiFi encendido. Si SemaCore ya lo arrancó
  // (WiFiManager en STA/AP), esp_wifi_start devuelve ESP_ERR_INVALID_STATE;
  // lo tratamos como "ya encendido" y seguimos.
  const esp_err_t ws = esp_wifi_start();
  if (ws != ESP_OK && ws != ESP_ERR_INVALID_STATE) {
    ++errCount_;
    return;
  }
  if (esp_now_init() != ESP_OK) {
    ++errCount_;
    return;
  }

  // MAC de origen de nuestros frames = MAC STA. esp_read_mac no requiere WiFi.
  esp_read_mac(ownMac_, ESP_MAC_WIFI_STA);

  // Los callbacks corren en el task de WiFi; reenvían a la instancia activa.
  gMesh = this;
  esp_now_register_recv_cb(onRecv);
  esp_now_register_send_cb(onSend);

  addPeers();
  seq_ = 0;
  lastSendMs_ = millis();
  ready_ = true;
}

void MeshManager::stop() {
  if (ready_) {
    esp_now_deinit();
  }
  if (gMesh == this) {
    gMesh = nullptr;
  }
  ready_ = false;
  peersAdded_ = 0;
  rxWrite_ = 0;
  rxRead_ = 0;
}

bool MeshManager::addPeer(const uint8_t mac[6], uint8_t channel) {
  if (esp_now_is_peer_exist(mac)) {
    return true;  // ya está (idempotente)
  }
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = channel;      // 0 = canal actual
  peer.ifidx = WIFI_IF_STA;    // ESP-NOW usa la interfaz station por defecto
  peer.encrypt = false;        // sin cifrado en la malla local (simplicidad)
  return esp_now_add_peer(&peer) == ESP_OK;
}

bool MeshManager::addPeers() {
  peersAdded_ = 0;
  const uint8_t n = (cfg_.peerCount > kMaxPeers) ? kMaxPeers : cfg_.peerCount;
  for (uint8_t i = 0; i < n; ++i) {
    uint8_t mac[6] = {0};
    if (!parseMac(cfg_.peerMacs[i].c_str(), mac)) {
      continue;
    }
    if (addPeer(mac, cfg_.channel)) {
      ++peersAdded_;
    }
  }
  // El broadcast no se registra como peer en ESP-NOW: esp_now_send con
  // peer_addr == nullptr ya alcanza a todos los peers añadidos. FF:FF:FF:FF:FF:FF
  // se soporta como destino explícito en send() (esp_now_send lo acepta).
  return peersAdded_ > 0;
}

// -----------------------------------------------------------------------------
// Envío
// -----------------------------------------------------------------------------

bool MeshManager::send(const uint8_t* mac, const uint8_t* data, size_t len) {
  if (!ready_) {
    return false;
  }
  if (len > ESP_NOW_MAX_DATA_LEN) {
    len = ESP_NOW_MAX_DATA_LEN;
  }
  const esp_err_t err = esp_now_send(mac, data, len);
  if (err != ESP_OK) {
    ++errCount_;
    return false;
  }
  ++txCount_;
  return true;
}

bool MeshManager::sendFrame(const uint8_t* mac, uint8_t type,
                            const uint8_t* payload, size_t len) {
  if (!ready_) {
    return false;
  }
  uint8_t frame[kMaxFrame];
  const size_t total = buildFrame(frame, type, ownMac_, seq_, payload, len);
  if (!send(mac, frame, total)) {
    return false;
  }
  ++seq_;  // wrap automático (uint8_t)
  return true;
}

bool MeshManager::broadcast(uint8_t type, const uint8_t* payload, size_t len) {
  return sendFrame(nullptr, type, payload, len);
}

bool MeshManager::sendHeartbeat() {
  return broadcast(HEARTBEAT, nullptr, 0);
}

bool MeshManager::sendAnnounce(const char* boardId, const char* version) {
  // ANNOUNCE: [role:1][boardId: len+chars][version: len+chars]
  uint8_t p[kMaxPayload];
  size_t o = 0;
  p[o++] = cfg_.role;
  const size_t bLen = (boardId == nullptr) ? 0 : strlen(boardId);
  p[o++] = static_cast<uint8_t>(bLen);
  if (bLen) {
    memcpy(p + o, boardId, bLen);
    o += bLen;
  }
  const size_t vLen = (version == nullptr) ? 0 : strlen(version);
  p[o++] = static_cast<uint8_t>(vLen);
  if (vLen) {
    memcpy(p + o, version, vLen);
    o += vLen;
  }
  return broadcast(ANNOUNCE, p, o);
}

// -----------------------------------------------------------------------------
// Recepción (callbacks → cola)
// -----------------------------------------------------------------------------

void MeshManager::onRecv(const esp_now_recv_info_t* info, const uint8_t* data,
                         int len) {
  if (gMesh != nullptr && info != nullptr) {
    gMesh->handleRecv(info->src_addr, data, len);
  }
}

void MeshManager::onSend(const esp_now_send_info_t* tx_info,
                         esp_now_send_status_t status) {
  if (gMesh == nullptr) {
    return;
  }
  if (status == ESP_NOW_SEND_FAIL) {
    ++gMesh->errCount_;
    gMesh->sendOk_ = false;
  } else {
    gMesh->sendOk_ = true;
  }
}

void MeshManager::handleRecv(const uint8_t* srcMac, const uint8_t* data,
                             int len) {
  ++rxCount_;
  // Valida la cabecera del protocolo (magic + version).
  if (data == nullptr || len < kHeaderLen) {
    ++errCount_;
    return;
  }
  if (data[0] != 0x53 || data[1] != 0x4D || data[2] != 1) {
    ++errCount_;  // frame ajeno o de otra versión del protocolo
    return;
  }
  const uint8_t type = data[3];
  const uint8_t seq = data[10];
  const uint8_t payloadLen = data[11];
  if (payloadLen > kMaxPayload || kHeaderLen + payloadLen > len) {
    ++errCount_;
    return;
  }

  // Último peer + última data (acceso inmediato sin drenar la cola).
  memcpy(lastPeer_, srcMac, 6);
  lastType_ = type;
  lastDataLen_ = payloadLen;
  if (payloadLen) {
    memcpy(lastData_, data + kHeaderLen, payloadLen);
  }

  // Cola SPSC best-effort: si está llena se descarta el más nuevo (el maestro
  // debe drenar con popRx en loop()). Sin mutex: el callback y loop() tocan
  // índices distintos; el peor caso es perder/duplicar una lectura, no corromper.
  const uint8_t next = static_cast<uint8_t>((rxWrite_ + 1) % kRxQueue);
  if (next == rxRead_) {
    return;  // llena → drop
  }
  RxMessage& m = rx_[rxWrite_];
  memcpy(m.src, srcMac, 6);
  m.type = type;
  m.seq = seq;
  m.len = payloadLen;
  if (payloadLen) {
    memcpy(m.payload, data + kHeaderLen, payloadLen);
  }
  rxWrite_ = next;
}

bool MeshManager::popRx(RxMessage& out) {
  if (rxRead_ == rxWrite_) {
    return false;  // vacía
  }
  out = rx_[rxRead_];
  rxRead_ = static_cast<uint8_t>((rxRead_ + 1) % kRxQueue);
  return true;
}

// -----------------------------------------------------------------------------
// Loop periódico
// -----------------------------------------------------------------------------

void MeshManager::loop() {
  if (!ready_) {
    return;
  }
  // Latido periódico (ambos roles). El ANNOUNCE lo dispara el esclavo a demanda
  // o Fase 2b desde su firmware; aquí solo mantenemos la presencia con HEARTBEAT.
  const uint16_t intervalSec = cfg_.sendIntervalSec;
  if (intervalSec > 0 &&
      (millis() - lastSendMs_) >= static_cast<uint32_t>(intervalSec) * 1000UL) {
    sendHeartbeat();
    lastSendMs_ = millis();
  }
}

// -----------------------------------------------------------------------------
// Estado para la web
// -----------------------------------------------------------------------------

void MeshManager::statusJson(JsonDocument& doc) {
  doc["ready"] = ready_;
  doc["enabled"] = cfg_.enabled;
  doc["role"] = cfg_.role;
  doc["channel"] = cfg_.channel;
  doc["send_interval"] = cfg_.sendIntervalSec;
  doc["peer_count"] = cfg_.peerCount;
  doc["peers_registered"] = peersAdded_;
  doc["tx_count"] = txCount_;
  doc["rx_count"] = rxCount_;
  doc["err_count"] = errCount_;
  char mac[18];
  macToStr(lastPeer_, mac, sizeof(mac));
  doc["last_peer"] = mac;
  doc["last_type"] = lastType_;
  doc["last_len"] = lastDataLen_;
  doc["queued"] = static_cast<uint8_t>((rxWrite_ - rxRead_ + kRxQueue) % kRxQueue);
}

}  // namespace base

#endif  // SEMA_USE_MESH_ESPNOW
