# Plan

## Steps Completed
- Closed the dedicated VL53L0X-through-expander bring-up task with working hardware confirmation on channels `1` and `2`.
- Confirmed there is no existing `VL53L5CX` code in the repository, so this must be treated as a fresh bring-up rather than a minor extension.
- Reviewed the local `VL53L5CX` references and added a first chip note at `.context/resources/chip_notes/VL53L5CX.md` covering address conventions, `LPn`, ULD-style initialization, and a minimal first-read strategy.
- Tightened `.context/resources/chip_notes/VL53L5CX.md` so it now explicitly captures the practical address notation, `LPn` / `INT` role, the fact that setup is ULD-driven rather than a tiny register list, and the first useful data-read flow.
- Inspected the existing dedicated `VL53L0X` mux test and confirmed it is the right structural baseline: isolated env, explicit mux channel selection, no drivetrain code, and serial output focused on clear bring-up status.
- Re-scoped the implementation after user direction: no external `VL53L5CX` library; the bring-up must use repo-local code and `task_safe_wire`.
- Added a first repo-local custom helper at `include/sensors/vl53l5cx_basic.h` and `src/sensors/vl53l5cx_basic.cpp` with task-safe `0x29` probe plus 16-bit register read and write primitives.
- Added a dedicated `env:vl53l5cxmuxtest` and updated `src/z_main_vl53l5cx_mux_test.cpp` so it selects mux channel `2` and repeatedly probes the sensor through the expander path using only repo-local code.
- Verified `pio run -j1 -e vl53l5cxmuxtest` succeeds with the custom no-library path.
- Flashed `env:vl53l5cxmuxtest` successfully to `COM7` and confirmed on hardware that the connected `VL53L5CX` acknowledges reliably on expander mux channel `2` at address `0x29`.
- Extended the custom helper and test beyond plain `ACK`: `Vl53l5cxBasic` now reads the public ST device ID register at `0x010F`, and `pio run -j1 -e vl53l5cxmuxtest` still succeeds with that change.

## In Progress
- Use the new task-safe custom helper as the base bring-up path and determine the next smallest step beyond plain I2C ACK toward actual ranging.

## Steps Remaining
- Extract or reconstruct the minimum additional `VL53L5CX` init and readout sequence needed for a first real frame without introducing an external library.
- Flash the updated ID-reading build and confirm the `0x010F` device ID value on hardware.
- Extend the custom helper from ID-check to useful ranging output.
- Rebuild, flash, and confirm useful readout through the expander-mux path.

## Definition Of Done
- A dedicated `VL53L5CX` bring-up path exists.
- The code builds successfully.
- The code flashes successfully.
- The test reads useful data from the connected `VL53L5CX` through the expander-mux path.
