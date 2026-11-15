# ESP32-Base

Librería base reutilizable para firmware **ESP32** (PlatformIO + framework Arduino).

Separa la infraestructura genérica (runtime, configuración, red, web local,
almacenamiento, alarmas, publicadores) de todo lo específico de cada proyecto
(sensores, lógica de dominio). Es la base extraída de
[SEMA](https://github.com/AlessandroKlein/SEMA) (estación meteorológica), pensada
para que cualquier proyecto nuevo haga `git clone → nuevo proyecto` y la consuma
como dependencia **siempre actualizada**.

## Qué incluye

- **Runtime modular**: `SemaCore` (singleton con `setup()`/`loop()`), `ModuleRegistry`,
  `Scheduler` + tareas, `EventBus` + `EventLog`.
- **Configuración**: `ConfigManager` (JSON en NVS, esquema versionado, pines por
  perfil de hardware).
- **Red**: `WiFiManager`, `EthernetManager` (LAN8720A nativa / W5500 SPI).
- **Buses**: `CanManager`, `ModbusManager`, `LoraManager`, `ZigbeeManager`.
- **Web local**: `HttpServer` (servida desde PROGMEM, dashboard + API REST `/api/v1/*`
  + WebSocket en vivo, auth `X-API-Key`, OTA).
- **Almacenamiento**: `NvsStore` (NVS), `HistoryStore` (SD + downsample).
- **Alarmas**: `RuleEngine` (reglas umbral → eventos).
- **Publicadores**: `HttpPublisher`, `MqttPublisher`, `PublisherManager`.
- **Infraestructura**: `Watchdog`, `HealthMonitor`, `PowerManager`,
  `CapabilityManager`, `Time`, `Calibration`, `GpioManager` (MCP23017),
  `ShiftRegisterManager`.
- **Modelo**: `Measurement` (medición + calidad).

## Cómo se consume

En `platformio.ini` del proyecto:

```ini
lib_deps =
    https://github.com/AlessandroKlein/ESP32-Base.git#v0.1.0
```

### Puntos de extensión (hooks de dominio)

La base no conoce los sensores ni la lógica de tu dominio. El proyecto registra
sus hooks en `base::coreHooks()` (ver `include/base/core_hooks.h`):

| Hook | Qué provee el proyecto |
|------|------------------------|
| `createSensor` | crear un sensor desde `SensorSpec` |
| `registerDefaultSensors` / `registerDefaultRules` / `registerDefaultCalibrations` | catálogo por defecto |
| `derivedConfigure` / `computeDerived` | magnitudes derivadas |
| `forecastAddSample` | muestras para predicción |
| `domainInit` | inicialización del dominio (p. ej. escaneo I²C) |
| `sensorCatalogJson` / `measurementLabelsJson` | catálogo y etiquetas para la web |
| `sensorsJson` / `forecastJson` / `windNorthJson` / `windResistorsJson` | endpoints de dominio |

### Perfil de hardware

La base no define la placa. El proyecto provee `hw/HwProfile.hpp` (macros `SEMA_*`
de pines/features) y `core/Version.hpp`. `include/base/board_config.h` los
descubre vía `__has_include` y cae a defaults neutros si no existen.

## Estructura

```
include/base/board_config.h    # defaults de pines/features (__has_include)
include/base/core_hooks.h      # puntos de extensión de dominio
include/core/...               # runtime, config, red, web, storage, alarmas, publishers
src/core/...                   # implementaciones
```

## Licencia

MIT. Ver [LICENSE](LICENSE).
