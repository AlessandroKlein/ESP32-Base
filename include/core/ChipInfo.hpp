#pragma once
// ChipInfo.hpp — Wrappers runtime de identidad del SoC (namespace base::).
// Finos sobre EspClass (Arduino core); útiles para logs y para la web local.

#include <Arduino.h>
#include "base/chip.hpp"

namespace base {

inline const char* chipModel()    { return ESP.getChipModel(); }
inline uint8_t     chipRevision() { return ESP.getChipRevision(); }
inline uint32_t    flashSize()    { return ESP.getFlashChipSize(); }
inline uint32_t    psramSize()    { return ESP.getPsramSize(); }
inline uint8_t     coreCount()    { return ESP.getChipCores(); }

}  // namespace base
