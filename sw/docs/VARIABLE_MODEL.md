# Shared Variable Model

## Purpose

This document is the current source-of-truth for the shared variables carried through `setget`.
It exists so another programmer or agent can answer these questions without guessing:

- what each variable means
- which task or module produces it
- which unit or scale it uses
- whether the value is stable or still configuration-dependent
- how the value is encoded in the current 32-bit transport model

This document describes the current code as it exists today. Where the code is inconsistent or unit-drifted, that is called out explicitly instead of being silently normalized here.

## Transport Model

All shared variables currently use `globalVar_set()` and `globalVar_get()` with a `long` payload.

On the active ESP32 Arduino target, `long` is a signed 32-bit integer:

- range: `-2147483648` to `2147483647`
- no floating-point transport in `setget`
- any fractional unit must therefore be encoded by scaling into an integer

This makes the shared-state bus compact and task-safe, but it also means every variable must define:

- its physical meaning
- its unit
- its integer scaling

## Variable Taxonomy

The project now uses these tiers:

- `0000` series: calibration and system-state values
- `1000` series: raw sensor values
- `2000` series: calculated or integrated values
- `3000` series: fused decision-ready values
- `4000` series: map and navigation exchange values
- `5000` series: actuator and driver intent or state values

Name prefixes should follow the same structure:

- `raw*`: direct sensor output or near-direct sensor output
- `calc*`: derived from one source or a simple transformation or integration chain
- `fuse*`: derived from multiple inputs and intended to be decision-ready
- `map*`: shared map and pose exchange values
- `driver*`: higher-level vehicle-command intent

## Status Terms

- `Active`: produced and consumed in current code paths
- `Reserved`: present in the enum but not meaningfully driven yet
- `Deferred`: intended concept exists but implementation is deferred
- `Uncertain`: active, but unit or semantics still depend on sensor configuration or historical scaling choices

## Current Variables

### Calibration and System Values

| ID | Name | Tier | Meaning | Unit / Scale | Encoding in `long` | Producer | Status | Notes |
|---|---|---|---|---|---|---|---|---|
| 0001 | `zeroAx` | calibration | Forward-axis accelerometer zero offset used to bias-correct MPU6050 `AcX` | MPU6050 raw counts | signed 32-bit integer holding average startup sample | `ACCsensor::Begin()` and `ACCsensorKalman::Begin()` | Active | The value is an average of startup samples, not a factory-calibrated bias. |
| 0002 | `zeroAy` | calibration | Lateral accelerometer zero offset used to center MPU6050 `AcY` | MPU6050 raw counts | signed 32-bit integer holding average startup sample | `ACCsensor::Begin()` and `ACCsensorKalman::Begin()` | Active | This is a startup average in the truck's initial pose. It reflects both bias and the initial mounting orientation. |
| 0003 | `zeroAz` | calibration | Vertical accelerometer zero offset used to center MPU6050 `AcZ` | MPU6050 raw counts | signed 32-bit integer holding average startup sample | `ACCsensor::Begin()` and `ACCsensorKalman::Begin()` | Active | This startup average includes local gravity in the initial pose, so the centered `rawAccZ` becomes a deviation from startup gravity rather than absolute `1g`. |
| 0004 | `zeroGz` | calibration | Gyro Z zero offset used to bias-correct MPU6050 yaw-rate readings | MPU6050 raw gyro counts | signed 32-bit integer holding average startup sample | `ACCsensor::Begin()` and `ACCsensorKalman::Begin()` | Active | This replaces the older hardcoded yaw-rate bias with a startup average stored in the shared bus. |

### Raw Sensor Values

| ID | Name | Tier | Meaning | Unit / Scale | Encoding in `long` | Producer | Status | Notes |
|---|---|---|---|---|---|---|---|---|
| 1001 | `rawDistLeft` | raw | Left ultrasonic measured distance | centimeters, capped at `199` | integer centimeters | `Usensor` trigger or echo path | Active | Age depends on `POLL_INTERVAL` and number of configured sensors. |
| 1002 | `rawDistFront` | raw | Front ultrasonic measured distance | centimeters, capped at `199` | integer centimeters | `Usensor` trigger or echo path | Active | `199` is also used as timeout or out-of-range sentinel. |
| 1003 | `rawDistRight` | raw | Right ultrasonic measured distance | centimeters, capped at `199` | integer centimeters | `Usensor` trigger or echo path | Active | Same timeout behavior as other ultrasonic values. |
| 1004 | `rawDistBack` | raw | Rear ultrasonic measured distance | centimeters, capped at `199` | integer centimeters | `Usensor` trigger or echo path | Active | Same timeout behavior as other ultrasonic values. |
| 1010 | `rawMagX` | raw | Magnetometer X axis | driver-dependent integer magnetometer output | signed integer | legacy `compass.cpp` path | Deferred | Magnetometer path is not active on the truck today. Keep semantics provisional until the active magnetometer path is chosen. |
| 1011 | `rawMagY` | raw | Magnetometer Y axis | driver-dependent integer magnetometer output | signed integer | legacy `compass.cpp` path | Deferred | Same caveat as `rawMagX`. |
| 1012 | `rawMagZ` | raw | Magnetometer Z axis | driver-dependent integer magnetometer output | signed integer | legacy `compass.cpp` path | Deferred | Same caveat as `rawMagX`. |
| 1020 | `rawAccX` | raw | Forward-axis acceleration in the bus's centered raw-data form | MPU6050 raw counts after subtracting `zeroAx` | signed integer | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | The bus stores centered counts, not `m/s^2` or `mg`. `env:accsensor` publishes a locally damped EMA value with deadband and slow stationary zero tracking; `env:accsensorkalman` publishes a Kalman-filtered value. |
| 1021 | `rawAccY` | raw | Lateral-axis acceleration in the bus's centered raw-data form | MPU6050 raw counts after subtracting `zeroAy` | signed integer | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | The bus stores centered counts in the startup pose. `env:accsensor` publishes a locally damped EMA value with deadband and slow stationary zero tracking; `env:accsensorkalman` publishes a Kalman-filtered value. |
| 1022 | `rawAccZ` | raw | Vertical-axis acceleration in the bus's centered raw-data form | MPU6050 raw counts after subtracting `zeroAz` | signed integer | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | The bus stores centered counts relative to startup gravity, not absolute `1g`. `env:accsensor` publishes a locally damped EMA value with deadband and slow stationary zero tracking; `env:accsensorkalman` publishes a Kalman-filtered value. |
| 1023 | `rawTemp` | raw | MPU6050 temperature estimate in the bus's stored raw-data form | tenths of degrees C | signed integer where `365` means `36.5 C` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | The bus stores this raw-data form as `degC * 10` after the datasheet conversion. Both envs currently apply a lightweight EMA before publishing. |
| 1030 | `rawGyX` | raw | Gyroscope X axis in the bus's stored raw-data form | tenths of degrees per second | signed integer where `15` means `1.5 dps` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | The bus stores this raw-data form as `deg/s * 10` using the MPU6050 default `131 LSB/dps` scale. Both envs currently apply a lightweight EMA before publishing. |
| 1031 | `rawGyY` | raw | Gyroscope Y axis in the bus's stored raw-data form | tenths of degrees per second | signed integer where `15` means `1.5 dps` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | The bus stores this raw-data form as `deg/s * 10` using the MPU6050 default `131 LSB/dps` scale. Both envs currently apply a lightweight EMA before publishing. |
| 1032 | `rawGyZ` | raw | Gyroscope Z axis used for yaw-rate updates in the bus's stored raw-data form | tenths of degrees per second | signed integer where `15` means `1.5 dps` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | The bus stores this raw-data form as `deg/s * 10` after subtracting the startup `zeroGz` calibration. Both envs currently apply a lightweight EMA before publishing. |
| 1040 | `rawLidarFront` | raw | Reserved front lidar distance | intended millimeters | signed integer millimeters | none found in current code | Reserved | Present for a future lidar path; no active producer today. |

### Calculated Values

| ID | Name | Tier | Meaning | Unit / Scale | Encoding in `long` | Producer | Status | Notes |
|---|---|---|---|---|---|---|---|---|
| 2001 | `calcHeading` | calc | Integrated truck heading based on gyro Z over elapsed time | tenths of degrees | signed integer where `900` means `90.0 deg` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | Heading is now integrated from the startup-bias-corrected `rawGyZ` using a task-local sample interval and a small yaw-rate deadband. It has a stable `deg10` scale, but overall quality still depends on drift and noise. |
| 2002 | `calcSpeed` | calc | Forward speed estimate from the plain MPU6050 path | millimeters per second | signed integer where sign follows forward or reverse acceleration history | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | `env:accsensor` now publishes a conservative estimate derived from centered `rawAccX` with thresholding, leakage, stationary reset, and clamping. `env:accsensorkalman` still holds this at `0` while that path remains parked. |
| 2003 | `calcDistance` | calc | Forward distance estimate from the plain MPU6050 path | millimeters | signed integer accumulated from the current `calcSpeed` estimate | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | `env:accsensor` now integrates the conservative speed estimate into distance. `env:accsensorkalman` still holds this at `0` while that path remains parked. |
| 2004 | `calcXpos` | calc | Reserved X position in world or map frame | intended millimeters | signed integer millimeters | none found in current code | Reserved | Present in the enum, but no active producer or consumer found. |
| 2005 | `calcYpos` | calc | Reserved Y position in world or map frame | intended millimeters | signed integer millimeters | none found in current code | Reserved | Present in the enum, but no active producer or consumer found. |

### Fused Values

The first live `fuse*` variable now exists in the enum and runtime.

| ID | Name | Meaning | Intended Unit / Scale | Intended Producer | Status | Notes |
|---|---|---|---|---|---|---|
| 3001 | `fuseForwardClear` | Decision-ready forward-movement clearance state | integer state: `-1` unknown, `0` blocked, `1` clear | `src/fusion/clearance_fusion.cpp` | Active | Current minimal rule uses `rawDistFront` and `rawLidarFront` only. A known blocked reading wins; any known clear reading wins if none are blocked; otherwise the result is unknown. |
| 3002 | `fuseReverseClear` | Decision-ready reverse clearance state | boolean-like integer | future fusion task | Proposed | Same structure as `fuseForwardClear`, but for the rear path. |
| 3003 | `fuseLeftClear` | Decision-ready left-side clearance state | boolean-like integer | future fusion task | Proposed | Useful for path and turn feasibility. |
| 3004 | `fuseRightClear` | Decision-ready right-side clearance state | boolean-like integer | future fusion task | Proposed | Useful for path and turn feasibility. |
| 3010 | `fuseHeadingDeg10` | Best current heading estimate after combining gyro, map alignment, and later other sensors | tenths of degrees | future fusion task | Proposed | This is a likely eventual replacement for the current ambiguous `calcHeading`. |
| 3011 | `fuseSpeedMmPs` | Best current speed estimate | millimeters per second | future fusion task | Proposed | This is a likely eventual replacement for the current ambiguous `calcSpeed`. |
| 3012 | `fusePosePacked` | Compact fused pose in grid space | same packed-byte layout as `map*PosePacked` | future fusion task | Proposed | Useful if observed and programmed map pose should be complemented by a best fused pose. |

### Map and Navigation Values

| ID | Name | Tier | Meaning | Unit / Scale | Encoding in `long` | Producer | Status | Notes |
|---|---|---|---|---|---|---|---|---|
| 4001 | `mapObservedPosePacked` | map | Observed pose exchanged as one packed 32-bit value | packed bytes: `x`, `y`, `direction`, `speed` | one signed 32-bit integer containing four signed bytes | mapping subsystem intended; not yet wired into `setget` producers | Reserved | `x` and `y` are grid cells, `direction` is 5-degree clockwise steps from north, `speed` is signed cm/s, `-128` per field means unknown. |
| 4002 | `mapProgrammedPosePacked` | map | Programmed-map pose exchanged as one packed 32-bit value | packed bytes: `x`, `y`, `direction`, `speed` | one signed 32-bit integer containing four signed bytes | mapping subsystem intended; not yet wired into `setget` producers | Reserved | Same packing contract as `mapObservedPosePacked`. |
| 4010 | `mapObservedCellSizeMm` | map | Cell size used by the observed grid map | millimeters per cell | signed integer millimeters | mapping subsystem intended | Reserved | Mirrors `MapGeometry::cell_size_mm`. |
| 4011 | `mapProgrammedCellSizeMm` | map | Cell size used by the programmed grid map | millimeters per cell | signed integer millimeters | mapping subsystem intended | Reserved | Mirrors `MapGeometry::cell_size_mm`. |

### Actuator and Driver Values

| ID | Name | Tier | Meaning | Unit / Scale | Encoding in `long` | Producer | Status | Notes |
|---|---|---|---|---|---|---|---|---|
| 5001 | `steerDirection` | actuator | Requested steering direction normalized for the steering subsystem | integer percent-like scale from `-100` to `100` | signed integer | `src/actuators/steer.cpp` | Active | This is the currently commanded steering target, not a measured wheel angle. |
| 5100 | `driver_driverActivity` | driver | High-level driver mode or command category | intended small enum integer | signed integer | no active producer in current code | Deferred | `driver.cpp` currently uses file-local state instead of the shared variable. |
| 5101 | `driver_desired_direction` | driver | Requested absolute direction for the driver abstraction | intended degrees or `deg10`, not yet formalized | signed integer | no active producer in current code | Deferred | Existing `driver.cpp` keeps this in file-local state only. |
| 5102 | `driver_desired_turn` | driver | Requested relative turn for the driver abstraction | intended degrees or `deg10`, not yet formalized | signed integer | no active producer in current code | Deferred | Existing `driver.cpp` keeps this in file-local state only. |
| 5103 | `driver_desired_speed` | driver | Requested travel speed | intended millimeters per second or similar, not yet formalized | signed integer | no active producer in current code | Deferred | Existing `driver.cpp` keeps this in file-local state only. |
| 5104 | `driver_desired_distance` | driver | Requested travel distance | intended millimeters | signed integer | no active producer in current code | Deferred | Existing `driver.cpp` keeps this in file-local state only. |

## Current Semantic Gaps To Fix Later

The most important remaining semantic gaps are now about quality and fusion rather than base units:

1. Gyro drift and heading quality
   `calcHeading` now uses a task-local time base and a small yaw deadband, but long-term accuracy still depends on bias stability, mounting alignment, and motion outside the simplified model.

2. Motion estimation beyond heading
   `env:accsensor` now publishes a conservative accelerometer-only forward speed and distance estimate, but it is still a simplified 2D model without tilt compensation. `env:accsensorkalman` remains parked with both values held at `0`.

The current MPU6050 env difference is now intentional and should stay narrow:

- `env:accsensor`
  Uses a local in-project damping chain on the accelerometer channels and a conservative forward speed or distance estimator.
- `env:accsensorkalman`
  Uses Kalman filtering on the accelerometer channels and the same lightweight EMA on temperature and gyro channels, but still keeps speed and distance at `0`.

This difference is implementation strategy only. Both environments should keep the same shared-variable contract and debug-output shape so they can be compared by environment selection alone.

## Recommended Next Formalization Steps

1. Keep the stored `raw*` naming honest by describing the bus value as raw-data form plus explicit integer scaling.
2. Decide later whether `rawGy*` should remain scaled raw-data form or be split into a true register-level raw value plus a calculated rate value.
3. Introduce the first `fuse*` variables only after the raw and calc units are stable.
4. Update enum comments in `include/variables/setget.h` so they match this document.
5. Once this document is accepted as the maintained source-of-truth, update `AGENTS.md` so every variable change must update this file in the same work item.

## Recommended Fusion Ownership

The first fusion layer should not be spread across unrelated sensor tasks. One dedicated `fusion` package is now the approved model.

### Concrete `calc*` vs `fuse*` Boundary

Use this rule going forward:

- `calc*`
  Fast, local estimates derived from one sensor family or one short processing chain.
- `fuse*`
  Decision-ready estimates that combine multiple sensor families, map context, confidence rules, or planner constraints.

Applied to the current motion variables:

| Current Variable | Keep As | Why |
|---|---|---|
| `calcHeading` | `calc*` for now | It is still a fast gyro-driven heading estimate from one sensor chain. |
| `calcSpeed` | `calc*` for now | In the plain env it is now a conservative forward accelerometer-integration estimate from one sensor chain. |
| `calcDistance` | `calc*` for now | In the plain env it is derived directly from `calcSpeed` and stays part of the same local motion chain. |
| `fuseHeadingDeg10` | future `fuse*` | This should become the best heading after combining gyro integration, map alignment, and later other sensors. |
| `fuseSpeedMmPs` | future `fuse*` | This should become the best speed after combining inertial estimates, wheel or motor knowledge, and later map or obstacle cues. |
| `fusePosePacked` | future `fuse*` | This should become the compact decision-ready pose for navigation. |

This keeps the current `calc*` variables useful for fast local estimation while making it clear that later planner-facing or safety-facing motion decisions should prefer `fuse*` ownership rather than continuously overloading `calc*`.

Recommended structure:

- `src/fusion/fusion_service.cpp`
  Owns one fast fusion task and one slow fusion task.
- helper files under `src/fusion/`
  Hold narrow fusion rules such as forward-clearance logic without owning task lifecycle themselves.

Recommended first task responsibilities:

| Package Area | Cadence | Inputs | Outputs |
|---|---|---|---|
| fast fusion task | `100 ms` today, may later tighten | `rawDist*`, future lidar distances, map occupancy near the truck | `fuseForwardClear`, later other direction-clearance outputs |
| slow fusion task | `1000 ms` today, likely faster later | `calcHeading`, `calcSpeed`, `calcDistance`, map alignment, future magnetometer or wheel cues | future pose and navigation-facing `fuse*` outputs |

Current implementation status:

- `fusion_service`
  Implemented and owns both task cadences.
- `clearance_fusion`
  Implemented as helper logic for `fuseForwardClear` only.
- slow fusion logic
  Task exists as a stub so the architecture and runtime cadence are already in place before heavier fusion rules are added.

Current task split:

- fast fusion task, `100 ms`
  Updates `fuseForwardClear` and is the home for later near-term clearance gating.
- slow fusion task, `1000 ms`
  Exists as the home for later pose and map fusion without changing package ownership again.

Why:

- obstacle clearance needs quicker refresh for safe control loops
- pose and map fusion usually depend on noisier or slower-changing signals and can tolerate a slower cadence
- separating them keeps each fusion task easier to reason about and test

## Current MPU6050 Comparison Rule

For the two active MPU6050 environments, the intended comparison model is now:

- `env:accsensor`
  Plain in-project filtering using EMA damping, deadbanding, and slow stationary zero tracking on the accelerometer channels, plus the current conservative speed and distance estimator.
- `env:accsensorkalman`
  Kalman filtering on the accelerometer channels, with the same lightweight EMA on temperature and gyro channels. This path is currently parked and still holds speed and distance at `0`.

This is an implementation-strategy difference only. The shared-variable contract should stay the same between the two environments.

## Initial Illustration Prompt

Use this prompt for a future architecture illustration:

> Create a clean engineering diagram of the toy-truck firmware data model. Show sensor tasks on the left, the shared `setget` bus in the center, and actuator, map, driver, and one unified `fusion` package on the right. Inside the fusion package, show `FusionService` owning two cadences: a fast `10 Hz` task for near-term clearance fusion and a slow `1 Hz` task for pose and map fusion. Group variables into five color-coded bands: calibration (`0000`), raw (`1000`), calculated (`2000`), fused (`3000`), map (`4000`), and driver or actuator (`5000`). Show that every shared value is a signed 32-bit integer. Highlight active flows in solid lines and reserved or deferred flows in dashed lines. Emphasize that `task_safe_wire` is the only direct `Wire` path. Style should feel like a polished embedded-systems architecture poster: precise labels, minimal clutter, balanced whitespace, readable typography, and subtle technical color accents rather than generic flowchart styling.
