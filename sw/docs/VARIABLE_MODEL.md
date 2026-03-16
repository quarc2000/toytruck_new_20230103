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

| Series | Semantic Layer | Preferred Naming | Meaning |
|---|---|---|---|
| `1000` | raw | `raw*` | Direct sensor output or near-direct sensor output. |
| `2000` | cleaned | `cleaned*` | Filtered, damped, centered, or otherwise cleaned values that are still close to one source. |
| `3000` | calculated | `calculated*` | Derived from one source or a simple transformation or integration chain. |
| `4000` | fused | `fused*` | Derived from multiple inputs and intended to be decision-ready. |
| `7000` | map | `map*` | Shared map and pose exchange values. |
| `8000` | driver | `driver*` | Higher-level vehicle-command intent or actuator-facing driver state. |
| `9000` | config or system | `config*` or explicit calibration names | Configuration, calibration, and system support values. |

Current implementation note:

- existing code symbols such as `calcHeading` and `fuseForwardClear` remain valid for now
- this document uses `calculated*` and `fused*` as semantic tier names
- a future rename task may align the code prefixes, but that is not part of this taxonomy update

## Status Terms

- `Active`: produced and consumed in current code paths
- `Reserved`: present in the enum but not meaningfully driven yet
- `Deferred`: intended concept exists but implementation is deferred
- `Uncertain`: active, but unit or semantics still depend on sensor configuration or historical scaling choices

## Current Variables

### Calibration and System Values

| ID | Name | Tier | Meaning | Unit / Scale | Encoding in `long` | Producer | Status | Notes |
|---|---|---|---|---|---|---|---|---|
| 9001 | `zeroAx` | config | Forward-axis accelerometer zero offset used to bias-correct MPU6050 `AcX` | MPU6050 raw counts | signed 32-bit integer holding average startup sample | `ACCsensor::Begin()` and `ACCsensorKalman::Begin()` | Active | The value is an average of startup samples, not a factory-calibrated bias. |
| 9002 | `zeroAy` | config | Lateral accelerometer zero offset used to center MPU6050 `AcY` | MPU6050 raw counts | signed 32-bit integer holding average startup sample | `ACCsensor::Begin()` and `ACCsensorKalman::Begin()` | Active | This is a startup average in the truck's initial pose. It reflects both bias and the initial mounting orientation. |
| 9003 | `zeroAz` | config | Vertical accelerometer zero offset used to center MPU6050 `AcZ` | MPU6050 raw counts | signed 32-bit integer holding average startup sample | `ACCsensor::Begin()` and `ACCsensorKalman::Begin()` | Active | This startup average includes local gravity in the initial pose, so the centered `rawAccZ` becomes a deviation from startup gravity rather than absolute `1g`. |
| 9004 | `zeroGz` | config | Gyro Z zero offset used to bias-correct MPU6050 yaw-rate readings | MPU6050 raw gyro counts | signed 32-bit integer holding average startup sample | `ACCsensor::Begin()` and `ACCsensorKalman::Begin()` | Active | This replaces the older hardcoded yaw-rate bias with a startup average stored in the shared bus. |

### Cleaned Values

This layer is currently architectural rather than fully implemented as a separate set of bus variables.

Its purpose is to hold cleaned single-source values that are more reusable than direct raw measurements:

- filtered accelerometer channels
- centered accelerometer channels
- deadbanded motion channels
- filtered gyro channels
- similar cleaned single-source values from other sensors later

Current implementation note:

- the active MPU6050 paths already perform cleaning before publishing several `raw*` values
- this means some current `raw*` bus values are actually raw-data form after cleaning
- that is acceptable short-term, but the architecture now reserves the `2000` band for a cleaner future split if these cleaned values need to become first-class bus variables

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
| 3001 | `calcHeading` | calculated | Integrated truck heading based on gyro Z over elapsed time | tenths of degrees | signed integer where `900` means `90.0 deg` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | Heading is now integrated from the startup-bias-corrected `rawGyZ` using a task-local sample interval and a small yaw-rate deadband. It has a stable `deg10` scale, but overall quality still depends on drift and noise. |
| 3002 | `calcSpeed` | calculated | Forward speed estimate from the plain MPU6050 path | millimeters per second | signed integer where sign follows forward or reverse acceleration history | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | `env:accsensor` now publishes a conservative estimate derived from centered `rawAccX` with thresholding, leakage, stationary reset, and clamping. `env:accsensorkalman` still holds this at `0` while that path remains parked. |
| 3003 | `calcDistance` | calculated | Forward distance estimate from the plain MPU6050 path | millimeters | signed integer accumulated from the current `calcSpeed` estimate | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | `env:accsensor` now integrates the conservative speed estimate into distance. `env:accsensorkalman` still holds this at `0` while that path remains parked. |
| 3004 | `calcXpos` | calculated | Reserved X position in world or map frame | intended millimeters | signed integer millimeters | none found in current code | Reserved | Present in the enum, but no active producer or consumer found. |
| 3005 | `calcYpos` | calculated | Reserved Y position in world or map frame | intended millimeters | signed integer millimeters | none found in current code | Reserved | Present in the enum, but no active producer or consumer found. |

### Fused Values

The first live `fused*` variable now exists in the enum and runtime.

| ID | Name | Meaning | Intended Unit / Scale | Intended Producer | Status | Notes |
|---|---|---|---|---|---|---|
| 4001 | `fuseForwardClear` | Decision-ready forward-movement clearance state | integer state: `-1` unknown, `0` blocked, `1` clear | `src/fusion/clearance_fusion.cpp` | Active | Current minimal rule uses `rawDistFront` and `rawLidarFront` only. A known blocked reading wins; any known clear reading wins if none are blocked; otherwise the result is unknown. |
| 4002 | `fuseReverseClear` | Decision-ready reverse clearance state | boolean-like integer | future fusion task | Proposed | Same structure as `fuseForwardClear`, but for the rear path. |
| 4003 | `fuseLeftClear` | Decision-ready left-side clearance state | boolean-like integer | future fusion task | Proposed | Useful for path and turn feasibility. |
| 4004 | `fuseRightClear` | Decision-ready right-side clearance state | boolean-like integer | future fusion task | Proposed | Useful for path and turn feasibility. |
| 4010 | `fuseHeadingDeg10` | Best current heading estimate after combining gyro, map alignment, and later other sensors | tenths of degrees | future fusion task | Proposed | This is a likely eventual replacement for the current ambiguous `calcHeading`. |
| 4011 | `fuseSpeedMmPs` | Best current speed estimate | millimeters per second | future fusion task | Proposed | This is a likely eventual replacement for the current ambiguous `calcSpeed`. |
| 4012 | `fusePosePacked` | Compact fused pose in grid space | same packed-byte layout as `map*PosePacked` | future fusion task | Proposed | Useful if observed and programmed map pose should be complemented by a best fused pose. |

### Map and Navigation Values

| ID | Name | Tier | Meaning | Unit / Scale | Encoding in `long` | Producer | Status | Notes |
|---|---|---|---|---|---|---|---|---|
| 7001 | `mapObservedPosePacked` | map | Observed pose exchanged as one packed 32-bit value | packed bytes: `x`, `y`, `direction`, `speed` | one signed 32-bit integer containing four signed bytes | mapping subsystem intended; not yet wired into `setget` producers | Reserved | `x` and `y` are grid cells, `direction` is 5-degree clockwise steps from north, `speed` is signed cm/s, `-128` per field means unknown. |
| 7002 | `mapProgrammedPosePacked` | map | Programmed-map pose exchanged as one packed 32-bit value | packed bytes: `x`, `y`, `direction`, `speed` | one signed 32-bit integer containing four signed bytes | mapping subsystem intended; not yet wired into `setget` producers | Reserved | Same packing contract as `mapObservedPosePacked`. |
| 7010 | `mapObservedCellSizeMm` | map | Cell size used by the observed grid map | millimeters per cell | signed integer millimeters | mapping subsystem intended | Reserved | Mirrors `MapGeometry::cell_size_mm`. |
| 7011 | `mapProgrammedCellSizeMm` | map | Cell size used by the programmed grid map | millimeters per cell | signed integer millimeters | mapping subsystem intended | Reserved | Mirrors `MapGeometry::cell_size_mm`. |

### Actuator and Driver Values

| ID | Name | Tier | Meaning | Unit / Scale | Encoding in `long` | Producer | Status | Notes |
|---|---|---|---|---|---|---|---|---|
| 8001 | `steerDirection` | actuator | Requested steering direction normalized for the steering subsystem | integer percent-like scale from `-100` to `100` | signed integer | `src/actuators/steer.cpp` | Active | This is the currently commanded steering target, not a measured wheel angle. |
| 8100 | `driver_driverActivity` | driver | High-level driver mode or command category | intended small enum integer | signed integer | no active producer in current code | Deferred | `driver.cpp` currently uses file-local state instead of the shared variable. |
| 8101 | `driver_desired_direction` | driver | Requested absolute direction for the driver abstraction | intended degrees or `deg10`, not yet formalized | signed integer | no active producer in current code | Deferred | Existing `driver.cpp` keeps this in file-local state only. |
| 8102 | `driver_desired_turn` | driver | Requested relative turn for the driver abstraction | intended degrees or `deg10`, not yet formalized | signed integer | no active producer in current code | Deferred | Existing `driver.cpp` keeps this in file-local state only. |
| 8103 | `driver_desired_speed` | driver | Requested longitudinal travel speed command | normalized signed command today, likely physical speed later | signed integer | `src/z_main_hw-test.cpp` in the current light-task integration path | Active, Uncertain | Currently used as a signed desired-speed hint for reverse-light behavior. Positive means forward command, negative means reverse command, and the present scale is still the normalized `-100` to `100` actuator command rather than a formal physical unit. |
| 8104 | `driver_desired_distance` | driver | Requested travel distance | intended millimeters | signed integer | no active producer in current code | Deferred | Existing `driver.cpp` keeps this in file-local state only. |

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
2. Decide where cleaned single-source values should become explicit `cleaned*` bus variables rather than remaining folded into the current published `raw*` values.
3. Decide later whether `rawGy*` should remain scaled raw-data form or be split into a true register-level raw value plus a calculated rate value.
4. Keep the decision-ready layer under `fused*` ownership rather than letting more planner-facing meaning accumulate under `calc*`.
5. Update enum comments in `include/variables/setget.h` so they match this document.

## Recommended Fusion Ownership

The first fusion layer should not be spread across unrelated sensor tasks. One dedicated `fusion` package is now the approved model.

### Concrete `calculated*` vs `fused*` Boundary

Use this rule going forward:

- `calculated*`
  Fast, local estimates derived from one sensor family or one short processing chain.
- `fused*`
  Decision-ready estimates that combine multiple sensor families, map context, confidence rules, or planner constraints.

Applied to the current motion variables:

| Current Variable | Keep As | Why |
|---|---|---|
| `calcHeading` | `calculated*` for now | It is still a fast gyro-driven heading estimate from one sensor chain. |
| `calcSpeed` | `calculated*` for now | In the plain env it is now a conservative forward accelerometer-integration estimate from one sensor chain. |
| `calcDistance` | `calculated*` for now | In the plain env it is derived directly from `calcSpeed` and stays part of the same local motion chain. |
| `fuseHeadingDeg10` | future `fused*` | This should become the best heading after combining gyro integration, map alignment, and later other sensors. |
| `fuseSpeedMmPs` | future `fused*` | This should become the best speed after combining inertial estimates, wheel or motor knowledge, and later map or obstacle cues. |
| `fusePosePacked` | future `fused*` | This should become the compact decision-ready pose for navigation. |

This keeps the current `calc*` variables useful as members of the `calculated*` layer while making it clear that later planner-facing or safety-facing motion decisions should prefer `fused*` ownership rather than continuously overloading `calc*`.

Recommended structure:

- `src/fusion/fusion_service.cpp`
  Owns one fast fusion task and one slow fusion task.
- helper files under `src/fusion/`
  Hold narrow fusion rules such as forward-clearance logic without owning task lifecycle themselves.

Recommended first task responsibilities:

| Package Area | Cadence | Inputs | Outputs |
|---|---|---|---|
| fast fusion task | `100 ms` today, may later tighten | `rawDist*`, future lidar distances, map occupancy near the truck | `fuseForwardClear`, later other direction-clearance outputs |
| slow fusion task | `1000 ms` today, likely faster later | `calcHeading`, `calcSpeed`, `calcDistance`, map alignment, future magnetometer or wheel cues | future pose and navigation-facing `fused*` outputs |

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

> Create a clean engineering diagram of the toy-truck firmware data model. Show sensor tasks on the left, the shared `setget` bus in the center, and actuator, map, driver, and one unified `fusion` package on the right. Inside the fusion package, show `FusionService` owning two cadences: a fast `10 Hz` task for near-term clearance fusion and a slow `1 Hz` task for pose and map fusion. Group variables into color-coded bands: raw (`1000`), cleaned (`2000`), calculated (`3000`), fused (`4000`), map (`7000`), driver or actuator (`8000`), and config or calibration (`9000`). Show that every shared value is a signed 32-bit integer. Highlight active flows in solid lines and reserved or deferred flows in dashed lines. Emphasize that `task_safe_wire` is the only direct `Wire` path. Style should feel like a polished embedded-systems architecture poster: precise labels, minimal clutter, balanced whitespace, readable typography, and subtle technical color accents rather than generic flowchart styling.
