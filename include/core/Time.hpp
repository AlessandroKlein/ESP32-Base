#pragma once

#include <Arduino.h>
#include <time.h>

// =============================================================================
// SEMA — Reloj (NTP / epoch)
// =============================================================================
// D-0044. `nowEpoch()` devuelve la época **UTC** en segundos (los timestamps del
// Canonical Data Model son UTC). Hasta que el reloj se sincronice por NTP
// (configTzTime en SemaCore), cae a `millis()/1000` (uptime) para no devolver 0.

namespace base {

// Época UTC en segundos. El timbrado de mediciones/histórico/forecast y el valor
// del reloj se consumen como UTC: el JS de la web hace `new Date(ts*1000)`, que
// convierte UTC→local del navegador. Devolver aquí la época local (t + offset)
// desplazaba la hora mostrada según el offset de la zona (p. ej. -3 h en Buenos
// Aires, donde la hora aparecía corrida).
inline uint32_t nowEpoch() {
  const time_t t = time(nullptr);
  // time() vale ~0 antes de la sincronización; un valor > 2001-09-09 indica
  // reloj sincronizado (época real).
  if (t <= 1000000000) {
    return millis() / 1000;
  }
  return static_cast<uint32_t>(t);
}

}  // namespace base
