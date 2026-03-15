# Datasheets

This directory indexes datasheets for the chips that are explicitly identifiable from the current codebase and board notes.

## Purpose

Keep one self-contained datasheet index here so future agents do not need to rediscover chip names, source URLs, or local mirrored files.

The focus is:

- chips clearly present in active code or board notes
- direct PDF or primary product-page sources where possible
- explicit status so future agents know which devices matter now versus later

## Current State

Official or primary-source datasheet URLs have been identified for the main chips in use or referenced by the project.

Binary PDF mirroring into this repository was attempted on 2026-03-15, but the current shell runtime failed during HTTPS retrieval:

- `Invoke-WebRequest`: `The underlying connection was closed: An unexpected error occurred on a receive.`
- `curl.exe`: `schannel: AcquireCredentialsHandle failed: SEC_E_NO_CREDENTIALS`

So this directory should be treated as:

- a local index of exact source URLs
- a place to keep mirrored PDFs when they are manually added or downloaded in a less restricted session

## Active or Strongly Referenced Chips

| Chip | Role In This Project | Status | Source Type | Local File | Source URL |
|---|---|---|---|---|---|
| `ESP32` | Main MCU on the truck controller | Active | Espressif PDF | [esp32.pdf](./esp32_datasheet_en.pdf) | [Original source](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf) |
| `MPU6050` | Accelerometer and gyro on the truck controller | Active | TDK/InvenSense PDF | [MPU-6000.pdf](./MPU-6000-Datasheet.pdf) | [Original source](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf) |
| `MCP23017` | GPIO expander on the IO expander board | Active | Microchip PDF | [MCP23017.pdf](./MCP23017-Data-Sheet-DS20001952.pdf) | [Original source](https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP23017-Data-Sheet-DS20001952.pdf) |
| `TCA9548A` | I2C switch or multiplexer on the IO expander board | Active | Texas Instruments PDF | [TCA9548A.pdf](./tca9548a.pdf) | [Original source](https://www.ti.com/lit/gpn/tca9548a) |
| `PCA9685` | PWM driver path on the IO expander board | Present in board notes and test code, deferred in runtime | NXP PDF | [PCA9685.pdf](./PCA9685.pdf) | [Original source](https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf) |
| `VL53L0X` | TOF distance sensor path on the IO expander board | Tested but not mounted on active truck | ST PDF | [VL53L0X.pdf](./vl53l0x.pdf) | [Original source](https://www.st.com/resource/en/datasheet/vl53l0x.pdf) |
| `VL53L5CX` | Future richer range sensor for mapping | Planned or future | ST PDF | [VL53L5CX.pdf](./vl53l5cx.pdf) | [Original source](https://www.st.com/resource/en/datasheet/vl53l5cx.pdf) |
| `QMC5883L` | Likely chip behind the `GY271` path | Deferred magnetometer path | QST PDF | [QMC5883L.pdf](./13-52-04%20QMC5883L%20Datasheet%20Rev.%20B.pdf) | [Original source](https://www.qstcorp.com/upload/pdf/202512/13-52-04%20QMC5883L%20Datasheet%20Rev.%20B.pdf) |

## Legacy or Alternate Paths

| Chip | Role In This Project | Status | Source Type | Local File | Source URL |
|---|---|---|---|---|---|
| `HMC5883L` | Legacy compass path used by `compass.cpp` and older board notes | Legacy or deferred | Honeywell PDF mirror surfaced by search | not mirrored | [Original source](https://www.mouser.com/datasheet/2/187/hmc5883l-347289.pdf) |

## Notes

- `GY271` is a module name, not a chip name. Current code and board notes suggest a `QMC5883L`-style device at `0x0D`.
- `HMC5883L` still appears in legacy code paths and board notes, so both magnetometer datasheet paths are retained here until the project fully settles on one implementation.
- `VL53L0X` is referenced in test or bring-up code on the IO expander path; `VL53L5CX` is the more important future mapping sensor.
- The active runtime today does not imply every listed chip is mounted on the current truck.

## Retrieval Note

Local PDF mirroring was attempted from the current shell on 2026-03-15 but failed due HTTPS credential or receive errors in the shell runtime. The URLs above were still verified as the intended sources through web search and product-page discovery.
