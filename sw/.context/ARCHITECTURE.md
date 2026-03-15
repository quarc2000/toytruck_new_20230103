# Architecture

## Overview
This repository is a PlatformIO-based ESP32 Arduino firmware project for small modular toy trucks with multiple hardware configurations. The physical controller is a bespoke board designed by Par, but for firmware and build purposes it should be treated as an ESP32 DevKit v4 style target. The codebase is organized around reusable modules for actuators, sensors, telemetry, configuration, and robot behavior, with several dedicated `z_main_*.cpp` entry points used to build targeted hardware or subsystem test firmware.

## Current Structure
- `platformio.ini` defines many separate environments instead of one canonical production target.
- The controller hardware is custom, but the PlatformIO board abstraction is `esp32dev`.
- `src/actuators` and `include/actuators` contain motor, steering, and lighting control.
- `src/sensors` and `include/sensors` contain ultrasonic, accelerometer, compass, and related filter logic.
- `src/telemetry` and `include/telemetry` contain logging and MQTT support.
- `src/robots` and `include/robots` contain higher-level driver behavior.
- `src/config.cpp`, `src/secrets.cpp`, `src/expander.cpp`, and `src/dynamics.cpp` provide shared supporting logic.
- `src/variables/setget.cpp` and `include/variables/setget.h` implement the shared information interface used for cross-task data exchange.
- A first mapping subsystem now exists under `include/navigation` and `src/navigation`, and it is the active architecture area for navigation work.

## Build Model
- The project uses `build_src_filter` heavily to select one entry point per PlatformIO environment.
- Several environments appear to be hardware bring-up or subsystem diagnostics: `usensor`, `motor`, `config`, `logger`, `steer`, `accsensor`, `accsensorkalman`, `compass`, `hwtest`, `light`, `gy271`, `i2cscan`, and `expander`.
- `env:esp32s3box` is configured with a broader dependency set and may represent a newer or less-finished integration target.
- `env:real_main` looks like the closest thing to an integrated runtime, but that should be verified against actual usage.

## Concurrency Model
- The firmware is intended to run multiple activities in parallel under FreeRTOS.
- Shared runtime data is exchanged through the `setget` interface rather than by directly sharing mutable state between tasks.
- `setget` is not the only theoretically possible inter-task communication method, but it is the only task-safe shared-state mechanism currently available in this project.
- `setget` is therefore the strongly recommended pattern because it keeps tasks loosely coupled while still allowing multiple tasks to poll the same variables independently.
- `setget` currently stores integer-valued state (`long` in the current implementation) and protects each variable with its own mutex/semaphore.
- Architectural requirement: all code must be task safe. If any module is found to bypass task-safe access patterns, work must pause and the user must be asked for guidance before proceeding.
- I2C access is a known architectural concern area and should be treated as requiring explicit task-safe wrapping or serialization.

## Shared Variable Model
- `setget` is the current shared-variable bus between tasks and remains the approved path for task-safe cross-task integer state exchange.
- The transport payload is currently a signed 32-bit `long` on the active ESP32 target, so all shared values must define explicit integer scaling rather than relying on implicit floating-point meaning.
- Shared variables are now being formalized into taxonomy bands rather than remaining as one flat enum with ad hoc comments.
- The agreed documentation series are:
  - `0000` calibration or system
  - `1000` raw sensor
  - `2000` calculated
  - `3000` fused
  - `4000` map or navigation
  - `5000` driver or actuator
- These IDs are documentation and architecture references first; they do not yet imply a wire protocol or enum-value renumbering.
- The architecture should distinguish clearly between:
  - active variables with current producers
  - reserved variables that have a name but no active producer yet
  - deferred variables whose subsystem concept exists but is not active in the runtime
- Future fusion work should not write directly back into ambiguous `calc*` meanings if a stable `fuse*` layer is more appropriate.

## Integration Status
- Some capabilities such as OTA and related infrastructure may exist in branches or prior work by other contributors, but they are not yet considered integrated into the working architecture for this local development flow.
- Until those features are deliberately integrated, the baseline architecture should be described in terms of the currently maintained modules and build targets in this repository.

## Key Architectural Observations
- Hardware identity and per-vehicle configuration are likely centralized in configuration code rather than compile-time board variants.
- The firmware supports multiple sensing modalities and optional I2C expansion hardware, so runtime capability may differ per truck.
- The repository appears to mix prototype/test entry points with reusable library-style modules, which is practical for embedded bring-up but increases the need for task-specific build verification.
- The `setget` layer is a core architectural boundary because it is the current mechanism for task-safe communication across parallel activities.
- Task safety is not optional quality work here; it is a hard architectural requirement.
- The controller platform has evolved through multiple hardware-use variants: an earlier steering-motor plus local-light setup, and a later servo-steering plus IO-expander-light setup that frees a motor channel for split left/right propulsion.
- Differential left/right propulsion support is important architecturally even where current runtime behavior still drives both sides together, because it is the basis for later steering enhancement.
- Navigation now needs an explicit map architecture rather than only direct sensor-reactive behavior.
- The emerging mapping design has at least two distinct map views: a programmed or intended map, and an observed or runtime map built from live sensing.
- The observed map must be alignable with the programmed map using vehicle pose estimates and later sensor-fusion outputs rather than assuming they start perfectly registered.
- Because future sensors include VL53L5 devices that can provide richer spatial measurements than single-distance sensors, the map representation should be designed to accept projected point or occupancy updates into a shared 2D grid.

## Planned Mapping Direction
- The current direction is a 2D grid map where each cell corresponds to a physical square in the environment.
- Candidate resolutions explicitly under consideration are 10 cm and 5 cm. This choice must balance path precision against ESP32 RAM and CPU limits.
- Cell storage should likely support bit flags rather than only a single code value, because later navigation may need to represent occupancy, certainty, observed-versus-programmed origin, path annotations, start or goal markers, and temporary navigation state at the same time.
- The programmed map format must support at least dimensions, cell data, start location, and destination location.
- The observed map should be a separate runtime structure that can later be updated from fused sensor inputs and aligned back to the programmed map frame.
- The implemented map foundation now consists of:
  - `GridMap` for reusable occupancy-grid storage and coordinate conversion
  - `ProgrammedMapDefinition` for static map loading
  - `MapBundle` for keeping programmed and observed maps plus alignment metadata together
  - `MapCell` as a 4-byte cell with `flags` and `confidence`
- The current cell model treats unknown as the absence of `CellKnown`, which leaves room for overlapping meanings such as blocked, programmed, observed, start, goal, and temporary navigation annotations.
- The current code keeps map resolution configurable per map instance through `cell_size_mm` rather than baking one resolution into the type system.
- The current position-exchange direction is now grid-based rather than millimeter-based. One signed 32-bit integer is used as a packed pose container with four signed bytes: `x`, `y`, `direction`, and `speed`.
- `-128` is the reserved unknown sentinel for each packed pose component.
- Physical distance is derived from grid delta times the relevant map's `cell_size_mm`, rather than by exchanging millimeter position directly.

## Open Questions
- Which PlatformIO environment is the authoritative production target today.
- Whether there is an intended migration path from the older `z_main_*` layout to a more unified application entry point.
- Which sensor and actuator combinations are currently deployed on real trucks versus only used in experiments.
- What the project-approved task-safe wrapper should be for shared I2C access across modules.
- Which of the currently inferred controller-board pin assignments are guaranteed board facts versus only current firmware conventions.
- Which grid resolution is the project baseline for mapping: 5 cm, 10 cm, or a hybrid strategy.
- What exact cell-flag semantics should be reserved from the start so the map format remains stable as fusion and planning grow.
- Whether programmed maps will be embedded in firmware, loaded from serial or telemetry, or generated by a host-side tool.
- What coordinate system and origin conventions should be used for map cells and vehicle pose.
