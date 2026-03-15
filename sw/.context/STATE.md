# State

## Current Task Memory
- Active task: add `VL53L5CX` support to the expander-mux bring-up path.
- Relevant current state:
  - the `VL53L0X` mux test is now a proven working baseline
  - the user plans to connect the `VL53L5CX` to expander port or channel `2`
  - `.context/resources/chip_notes/VL53L5CX.md` now explicitly captures the practical bring-up facts needed before coding: `0x29` 7-bit address versus `0x52` shifted notation, `LPn` and `INT`, the ULD-driven setup model, and the first useful data-read flow
  - the user explicitly rejected the external-library approach; this task must stay repo-local and use `task_safe_wire`
  - `include/sensors/vl53l5cx_basic.h` and `src/sensors/vl53l5cx_basic.cpp` now exist with task-safe probe plus 16-bit register read/write primitives
  - `env:vl53l5cxmuxtest` now builds successfully as a dedicated custom mux test
  - hardware confirmation now exists for the first custom checkpoint: the connected `VL53L5CX` ACKs reliably on mux channel `2` at `0x29`
  - the helper now also reads the public ST device ID register `0x010F`; the updated build compiles successfully but has not yet been flashed
- Current next step:
  - flash the ID-reading build, verify the ID value on hardware, then continue toward the first useful init and read path
