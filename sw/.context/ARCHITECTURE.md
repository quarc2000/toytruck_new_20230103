# Architecture

## Overview
This repository is a PlatformIO-based ESP32 Arduino firmware project for small modular toy trucks with multiple hardware configurations. The physical controller is a bespoke board designed by Par, but for firmware and build purposes it should be treated as an ESP32 DevKit v4 style target. The codebase is organized around reusable modules for actuators, sensors, telemetry, configuration, and robot behavior, with several dedicated `z_main_*.cpp` entry points used to build targeted hardware or subsystem test firmware.

## The use of libraries
- Most libraries from other sources can not be used as we do not know if they are task safe. For instance we cannot use any sensor or display libraries using I2C as we are dependent on the task_safe_wire library for all I2C accesses.


## Current Structure
- `platformio.ini` defines many separate environments instead of one canonical production target.
- The controller hardware is custom, but the PlatformIO board abstraction is `esp32dev`.
- `src/actuators` and `include/actuators` contain motor, steering, and lighting control.
- `src/basic_telemetry` and `include/basic_telemetry` contain the new minimal Wi-Fi debug path with an in-memory logger and simple web server.
- `src/sensors` and `include/sensors` contain ultrasonic, accelerometer, compass, and related filter logic.
- `src/telemetry` and `include/telemetry` contain logging and MQTT support.
- `src/robots` and `include/robots` contain higher-level driver behavior.
- `src/config.cpp`, `src/secrets.cpp`, `src/expander.cpp`, and `src/dynamics.cpp` provide shared supporting logic.
- `src/lights` and `include/lights` now contain expander-backed light-control services.
- `src/variables/setget.cpp` and `include/variables/setget.h` implement the shared information interface used for cross-task data exchange.
- A first mapping subsystem now exists under `include/navigation` and `src/navigation`, and it is the active architecture area for navigation work.

## Module Initialization Pattern
- Hardware-facing packages should use explicit `Begin()` calls for real initialization rather than doing bus, pin, PWM, or config setup implicitly in constructors.
- Constructors should stay cheap and side-effect-light wherever practical.
- `Begin()` must be safe to call more than once; repeated calls should be harmless.
- If a package method is called before `Begin()`, the package should defensively self-initialize rather than assuming startup order was perfect.
- This pattern is preferred because startup order is easier to reason about, easier to debug on embedded targets, and less fragile than hidden constructor-side hardware setup.
- The actuator layer should follow this pattern consistently, including `Motor` and `Steer`.

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
- The current MCP23017 light path is serialized through `task_safe_wire` via `src/expander.cpp`, so lighting is now part of the same task-safe I2C discipline as the other shared-bus devices.
- The new basic Wi-Fi debug path is intentionally separate from the existing MQTT-oriented telemetry files. It is a local-first diagnostic path, not a replacement for later Pi/MQTT integration.
- The current direct magnetometer path is the GY-271/QMC5883L family through the expander-mux path on port `3`, with a dedicated no-mobility service test environment for bench validation.

## Basic Telemetry Architecture
- `basic_telemetry` is the new minimal untethered debug path for moving-truck diagnostics.
- `basic_logger` stores recent runtime log lines in a RAM ring buffer rather than writing files to flash.
- `basic_web_server` exposes:
  - a simple HTML page
  - a plain-text `/logs` view
  - a JSON `/status` view
- The first test path uses a safe stationary env and shows selected `setget` values such as front distance, accelerometer axes, yaw rate, heading, forward-clear fuse, steering command, and desired speed.
- The current Wi-Fi behavior is:
  - always start a dedicated debug AP for direct connection
  - also attempt station connection with the existing credentials from `src/secrets.cpp`
  - continue in AP-only mode if station connection fails
- This path is intentionally minimal and should stay separate from the later MQTT-to-Pi telemetry work.

## Shared Variable Model
- `setget` is the current shared-variable bus between tasks and remains the approved path for task-safe cross-task integer state exchange.
- The transport payload is currently a signed 32-bit `long` on the active ESP32 target, so all shared values must define explicit integer scaling rather than relying on implicit floating-point meaning.
- Shared variables are now being formalized into taxonomy bands rather than remaining as one flat enum with ad hoc comments.
- The agreed documentation taxonomy is:

| Series | Layer | Preferred Naming | Meaning |
|---|---|---|---|
| `1000` | raw | `raw*` | Direct sensor output or near-direct sensor output. |
| `2000` | cleaned | `cleaned*` | Filtered, damped, centered, or otherwise cleaned values that are still close to one source. |
| `3000` | calculated | `calculated*` | Fast local estimates from one sensor family or one short processing chain. |
| `4000` | fused | `fused*` | Decision-ready estimates that combine multiple sensor families, map context, confidence rules, or planner constraints. |
| `7000` | map | `map*` | Shared map or navigation exchange values. |
| `8000` | driver | `driver*` | Higher-level vehicle command or actuator intent. |
| `9000` | config or system | `config*` or explicit calibration names | Configuration, calibration, or system support values. |

- The rationale for moving configuration or calibration away from `0000` is readability: leading zeros are too easy to lose in normal prose and tables, while `9000` remains visually distinct.
- These IDs are documentation and architecture references first; they do not yet imply a wire protocol or enum-value renumbering.
- The architecture should distinguish clearly between:
  - active variables with current producers
  - reserved variables that have a name but no active producer yet
  - deferred variables whose subsystem concept exists but is not active in the runtime
- Future fusion work should not write directly back into ambiguous `calculated*` meanings if a stable `fused*` layer is more appropriate.
- Current code symbols still use older prefixes such as `calc*` and `fuse*`. Those remain valid implementation names for now, but the architecture language should use `calculated*` and `fused*` as the tier names.
- For the current motion stack, `calcHeading`, `calcSpeed`, and `calcDistance` remain valid members of the `calculated*` layer because they are still single-chain inertial estimates. Later planner-facing motion state should move into `fuseHeadingDeg10`, `fuseSpeedMmPs`, and `fusePosePacked` rather than overloading the current `calc*` names.
- Fusion now lives under one `src/fusion` package rather than being split into separate package owners too early.
- `src/fusion/fusion_service.cpp` owns two cadences:
  - a fast fusion task, currently `10 Hz`, for near-term clearance gating
  - a slow fusion task, currently `10 Hz`, now used for heading fusion and still serving as the home for later pose and map fusion
- Helper files under the same package hold narrow rule logic without owning task lifecycle themselves.
- The live `fused*` implementation now exists:
  - `fuseForwardClear` is published by the fast fusion task in `src/fusion/fusion_service.cpp`
  - `fuseTurnBias` is also now published by the same fast fusion task
  - `fuseHeadingDeg10` is now published by the slow fusion task using gyro heading plus GY-271 magnetic correction
  - `src/fusion/clearance_fusion.cpp` now provides helper logic only
  - current rule set is intentionally minimal and conservative:
    - `fuseForwardClear`
      - blocked if the front ultrasonic or either front `VL53L0X` reports too little clearance
      - clear only when the front ultrasonic and both front `VL53L0X` sensors are all known clear
      - unknown otherwise
    - `fuseTurnBias`
      - left when the front-left `VL53L0X` is meaningfully longer than the front-right one
      - right when the front-right `VL53L0X` is meaningfully longer than the front-left one
      - neutral otherwise
- This first step is deliberately limited to fast forward-clearance gating. The slow fusion task is present as a stub so future pose and map fusion can be added without changing package ownership again.
- The current active PAT004 front sensor mapping used by the runtime is:
  - IO-expander port `1` = front-right `VL53L0X`
  - IO-expander port `2` = front-left `VL53L0X`

## Reactive Driver Runtime
- `src/robots/driver.cpp` now contains the first real driver-owned runtime behavior instead of remaining a pure stub.
- The current driver behavior is intentionally minimal and non-navigational:
  - drive forward when `fuseForwardClear` is clear
  - bias steering slightly toward `fuseTurnBias` while moving forward
  - stop when the forward path is not trusted
  - if blocking persists, reverse briefly with steering biased toward the better side
  - recovery direction now prefers the freer side ultrasonic (`rawDistLeft` or `rawDistRight`) and falls back to `fuseTurnBias` only when side context is weak
  - use the rear ultrasonic only as a safety stop while reversing
- This is not yet target-directed navigation. It is a local obstacle-avoidance behavior only.
- The dedicated build env for this path is `env:frontavoid`, with `src/z_main_front_avoid.cpp` as the runtime entry point.
- `env:frontavoid` now also includes `LightService`, so reverse-light and steering-indicator behavior stay active during the obstacle-avoidance runtime.

## Lighting Architecture
- Vehicle lighting is now moving from scripted GPIO bring-up into a real service layer.
- The current light output ownership is:
  - pin `0`: main light
  - pin `1`: brake light
  - pin `8`: left indicator 
  - pin `9`: right indicator 
  - pin `11`: reverse light
- `LightService` is the current owner of these outputs through one expander instance and one FreeRTOS task.
- The current control inputs are:
  - `rawAccX` for first-pass brake detection from longitudinal deceleration
  - `steerDirection` for indicator flashing demand
  - `driver_desired_speed` for reverse-light demand
- The current indicator cadence is `250 ms` per toggle, which gives a `2 Hz` blink cycle.
- Main-light policy is intentionally left undecided for now and is kept off in the first automated light-service pass so brake, indicator, and reverse behavior remain easy to verify.

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
- The first operational exploration baseline now uses:
  - `100 x 100` cells
  - `100 mm` cell size
  - start in the center cell
  - initial heading aligned so the truck starts pointing toward positive `Y`
- A finer `50 mm` grid remains a later option, not the current active baseline.
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

## Observed Exploration Runtime
- A dedicated exploration runtime now exists as `env:exploremap` with `src/z_main_explore_map.cpp` as the entry point.
- This runtime is intentionally separate from `env:frontavoid` so the reactive avoidance baseline remains available as a stable test path.
- The new runtime currently adds:
  - `ObservedExplorerService`
  - `ExploreWebServerService`
- `ObservedExplorerService` owns the first operational exploration loop:
  - reads `fuseForwardClear` and `fuseTurnBias`
  - reads side and rear ultrasonic distances
  - reads `calcHeading` as the local heading reference
  - projects the front `VL53L0X` pair and ultrasonic data into an observed occupancy grid
  - marks visited cells
  - searches reachable frontier cells
  - drives until no reachable frontier remains, then stops
- The active PAT004 front `VL53L0X` pair remains:
  - forward-facing
  - `105 mm` center-to-center spacing
  - expander port `1` = front-right
  - expander port `2` = front-left
- The exploration web path is separate from the basic telemetry page and currently serves:
  - a simple HTML map view
  - `/status` JSON
  - `/map` JSON
  - `/control` JSON plus remote-control command endpoints
  - `/logs`
- The exploration web/API path now also includes a first supervised remote-control layer:
  - manual control is explicitly enabled rather than always active
  - the explorer yields actuator ownership while remote control is active
  - remote commands are watchdog-timed and fail safe to stop if updates expire
  - disabling remote control returns actuator ownership to the autonomous explorer
- The first pose update model is intentionally lightweight:
  - integrate movement from commanded forward or reverse motion
  - use repeated front or rear obstacle observations to shrink over-optimistic travel estimates
  - use fused heading as the runtime-facing course:
    - gyro provides the short-term turn delta
    - GY-271 magnetic course provides long-term reference when not disturbed
    - later map alignment is still expected to refine heading further
- The current exploration runtime is still an early autonomous baseline, not a finished navigation stack. It does not yet load a real programmed map, perform robust pose alignment, or do full route planning.

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
