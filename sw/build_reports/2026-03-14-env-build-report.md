# PlatformIO Environment Build Report - 2026-03-14

Built using the repo-local helper workflow:

```powershell
. .\pio-local.ps1
pio run ...
```

## Summary
- Environments checked: 20
- Successful builds: 2
- Failed builds: 18
- Successful in the refreshed sweep: `usensor`, `motor`

## Main Failure Groups
1. Archive rename `Permission denied` under `.pio\build\...`
   - Still affected in the refreshed sweep: `maptest`, `config`, `steer`, `compass`, `hwtest`, `driver`, `i2cscan`, `expander`, `accsensor`, `accsensorcalibration`, `accsensorkalman`, `ioexpandertest`, `light`, `gy271`, `gy271monolith`
   - No longer affected in the refreshed sweep: `usensor`, `motor`
   - Typical failure text:
     - `xtensa-esp32-elf-ar: unable to rename ... reason: Permission denied`
     - `xtensa-esp32-elf-ranlib: unable to rename ... reason: Permission denied`
   - After the sandbox permission change, the problem became intermittent rather than universal.
   - A fresh cleanup-and-retry test on `maptest`, `config`, and `steer` still failed the same way, so the remaining cases are not just stale output in `.pio\build`.

2. Missing library headers or missing library declarations
   - `esp32s3box`: missing `FastLED.h`
   - `light`: missing `FastLED.h`
   - `logger`: missing `FastLED.h`, missing `Adafruit_Sensor.h`, and also missing `logger.h`
   - `real_main`: missing `Adafruit_Sensor.h`

3. Likely include-path issue
   - `logger`: `src/z_main_logger.cpp` includes `logger.h`, but the repository header is at `include/telemetry/logger.h`. This likely needs either `#include "telemetry/logger.h"` or an include-path adjustment.

## Warnings Worth Reviewing
- Repeated deprecation warning in `src/variables/setget.cpp`:
  - `taskENTER_CRITICAL(mux)` is deprecated in the ESP-IDF used by this toolchain.
- Seen in: `accsensor`, `accsensorkalman`, `compass`, `driver`, `esp32s3box`, `gy271`, `gy271monolith`, `hwtest`, `logger`, `real_main`, `steer`

## Suggested Next Discussion Order
1. Decide whether to solve the `.pio\build` archive rename restriction first, since it blocks most environments before code-level validation finishes.
2. Add or correct missing library dependencies for `FastLED` and `Adafruit Unified Sensor`.
3. Fix the `logger.h` include path mismatch.
4. Decide whether the `taskENTER_CRITICAL` deprecation should be cleaned up now or deferred.

## Artifacts
- Machine-readable summary: `build_reports/summary.json`
- Human summary table: `build_reports/summary.txt`
- Per-environment logs: `build_reports/*.log`
