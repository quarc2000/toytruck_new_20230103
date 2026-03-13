# Project History

## 2026-03-13
- Created the local `AGENTS.md` by copying the MCP-served project agent context into the repository root.
- Bootstrapped the local `.context` directory structure required by `AGENTS.md`.
- Added initial project-specific context for guardrails, architecture, state, task, tools, and planning.
- Mirrored shared resource index files into `.context/resources` and `.context/resources/policies`.
- Refined the architecture notes to describe the bespoke controller board, the `setget` task-safety model, and the current concurrency expectations.
- Added an initial controller-board document populated from code-inferred GPIO and I2C assignments.
- Recorded the currently visible task-safety concern that I2C access is not yet protected by one project-wide shared wrapper.
- Restored the uppercase context files and placed `BACKLOG.md` and `CONFLICTS.md` at repository root for visibility.
- Shifted the active task from context setup to code-driven board analysis.
- Added initial board resource files for the TOY controller board and IO expander based on current source-code evidence.
- Recorded that the magnetometer path is currently unresolved and not used on the truck, and moved follow-up work to `BACKLOG.md`.
- Updated the controller-board notes to describe the progression from steering-motor plus local lights to servo steering plus IO-expander lights, including the future differential-drive intent.
- Added a configuration-control section to the controller-board notes to distinguish PlatformIO build selection from runtime truck configuration by ESP32 device ID.
- Updated the IO-expander notes with actual light usage, PCA9685 non-working status, and the tested-but-not-mounted VL53L0X multiplexer setup, and added the deferred follow-up items to `BACKLOG.md`.
- Switched the active task to task-safety analysis and planning, centered on the known `Wire` / I2C concurrency concern.
- Recorded user confirmation that the known I2C task-safety gap is the current remediation task under conflict `CON-0001`.
- Completed the first task-safety inventory pass, identifying direct `Wire` usage in reusable sensor and expander modules as the primary remediation target and noting `usensor.cpp` as a second concurrency-review candidate.
- Recorded the agreed I2C wrapper direction: explicit begin/write/restart/request_from/read/end operations guarded by one shared bus semaphore, avoiding exposed `endTransmission(false)` semantics.
- Added an explicit task-safety definition of done to the task and plan context.
- Confirmed by source-level sweep that reusable I2C modules now use `task_safe_wire`, leaving direct `Wire` use only in `src/dynamics.cpp` and `src/z_main_*` bring-up paths outside the wrapper.
- Attempted focused PlatformIO validation for `accsensor`, `accsensorkalman`, `compass`, `gy271`, and `expander`, but local runs were blocked by a permissions error on `C:\Users\patbwm\.platformio\platforms.lock`.
- Recorded in `.context/TOOLS.md` that PlatformIO exists locally at `C:\Users\patbwm\.platformio\penv\Scripts\platformio.exe` / `pio.exe`, but is not on `PATH` in this shell.
- Reviewed `src/sensors/usensor.cpp`, opened conflict `CON-0002`, and paused further remediation because the module shares mutable file-scope state across a task, an ISR, and registration without an approved synchronization model.
