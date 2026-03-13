# State

## Current Task Memory
- Active task: task-safety analysis and remediation planning, with immediate focus on `Wire` / I2C access.
- Known active conflict: `CON-0001` tracks the current architecture-level task-safety gap around direct `Wire` usage without a project-wide approved synchronization model.
- Additional active conflict: `CON-0002` tracks shared mutable state in `src/sensors/usensor.cpp` across a task, ISR, and registration path without an approved synchronization model.
- The repository uses multiple firmware entry points and reusable modules, so task-safety review must cover both reusable runtime modules and test or bring-up paths.
- `setget` is the current approved shared-state mechanism for loosely coupled parallel tasks.
- Existing evidence already shows direct `Wire` access in multiple modules, while only `EXPANDER` has a local I2C semaphore.
- The production build target is still not confirmed, so task-safety work should prefer reusable cross-cutting fixes over environment-specific assumptions.
- First inventory pass found direct `Wire` usage in reusable modules: `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp`, `src/sensors/compass.cpp`, `src/sensors/GY271.cpp`, and `src/expander.cpp`.
- Additional `Wire` usage exists in test or bring-up paths: `src/z_main_23017_test.cpp`, `src/z_main_accsensorcalibration.cpp`, `src/z_main_gy-271_monolith.cpp`, `src/z_main_i2cscan.cpp`, and `src/z_main_expander.cpp`.
- Source-level review now confirms the reusable I2C modules have been refactored onto `task_safe_wire`, while remaining direct `Wire` usage outside the wrapper is in `src/dynamics.cpp` and `src/z_main_*` bring-up paths.
- Build verification could not be completed in this session because local PlatformIO runs failed with a permissions error opening `C:\Users\patbwm\.platformio\platforms.lock`.
- `src/sensors/usensor.cpp` is now confirmed as a separate task-safety issue because it uses file-scope mutable state shared by `TriggerTask`, `echoInterrupt`, and `Usensor::open()`.
- Agreed direction for I2C safety: use a shared bus semaphore and a wrapper API shaped around `task_safe_wire_begin(addr)`, `task_safe_wire_write(...)`, `task_safe_wire_restart()`, `task_safe_wire_request_from(addr, len)`, `task_safe_wire_read()`, and `task_safe_wire_end()`.
- Reason for this API shape: keep call-site structure close to current `Wire` usage, avoid exposing `endTransmission(false)`, preserve explicit repeated-start behavior, and avoid hidden state that could make reentrant behavior harder to reason about.
