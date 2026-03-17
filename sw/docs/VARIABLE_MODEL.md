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

This layer is now first-class for the active MPU6050 accel and gyro path.

| ID | Name | Tier | Meaning | Unit / Scale | Encoding in `long` | Producer | Status | Notes |
|---|---|---|---|---|---|---|---|---|
| 2020 | `cleanedAccX` | cleaned | Forward-axis accelerometer after centering and filter cleanup | MPU6050 counts after zero-offset correction and env-specific filtering | signed integer | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | `env:accsensor` publishes EMA-smoothed centered counts. `env:accsensorkalman` publishes Kalman-filtered centered counts. Downstream motion consumers should use this instead of `rawAccX`. |
| 2021 | `cleanedAccY` | cleaned | Lateral-axis accelerometer after centering and filter cleanup | MPU6050 counts after zero-offset correction and env-specific filtering | signed integer | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | Same env difference as `cleanedAccX`. |
| 2022 | `cleanedAccZ` | cleaned | Vertical-axis accelerometer after centering and filter cleanup | MPU6050 counts after zero-offset correction and env-specific filtering | signed integer | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | Same env difference as `cleanedAccX`. |
| 2030 | `cleanedGyX` | cleaned | Gyroscope X after smoothing | tenths of degrees per second | signed integer where `15` means `1.5 dps` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | Both envs publish lightweight EMA-smoothed gyro X here. |
| 2031 | `cleanedGyY` | cleaned | Gyroscope Y after smoothing | tenths of degrees per second | signed integer where `15` means `1.5 dps` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | Both envs publish lightweight EMA-smoothed gyro Y here. |
| 2032 | `cleanedGyZ` | cleaned | Gyroscope Z after startup bias correction, smoothing, and deadband | tenths of degrees per second | signed integer where `15` means `1.5 dps` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | This is the intended downstream yaw-rate input for heading integration and other consumers. |

### Raw Sensor Values

| ID | Name | Tier | Meaning | Unit / Scale | Encoding in `long` | Producer | Status | Notes |
|---|---|---|---|---|---|---|---|---|
| 1001 | `rawDistLeft` | raw | Left ultrasonic measured distance | centimeters, capped at `199` | integer centimeters | `Usensor` trigger or echo path | Active | Age depends on `POLL_INTERVAL` and number of configured sensors. |
| 1002 | `rawDistFront` | raw | Front ultrasonic measured distance | centimeters, capped at `199` | integer centimeters | `Usensor` trigger or echo path | Active | `199` is also used as timeout or out-of-range sentinel. |
| 1003 | `rawDistRight` | raw | Right ultrasonic measured distance | centimeters, capped at `199` | integer centimeters | `Usensor` trigger or echo path | Active | Same timeout behavior as other ultrasonic values. |
| 1004 | `rawDistBack` | raw | Rear ultrasonic measured distance | centimeters, capped at `199` | integer centimeters | `Usensor` trigger or echo path | Active | Same timeout behavior as other ultrasonic values. |
| 1010 | `rawMagX` | raw | Magnetometer X axis raw sample from the active QMC5883L-compatible path | sensor LSB | signed integer | `src/sensors/gy271_service.cpp` | Active, Uncertain | Current active path uses the GY-271/QMC5883L service on expander port `4`. Orientation and absolute field interpretation are still under validation. |
| 1011 | `rawMagY` | raw | Magnetometer Y axis raw sample from the active QMC5883L-compatible path | sensor LSB | signed integer | `src/sensors/gy271_service.cpp` | Active, Uncertain | Same caveat as `rawMagX`. |
| 1012 | `rawMagZ` | raw | Magnetometer Z axis raw sample from the active QMC5883L-compatible path | sensor LSB | signed integer | `src/sensors/gy271_service.cpp` | Active, Uncertain | Same caveat as `rawMagX`. |
| 1020 | `rawAccX` | raw | Forward-axis accelerometer raw register sample | MPU6050 raw counts | signed integer | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | This is now kept as the nearest honest bus copy of the sensor reading. Centering and filtering move to `cleanedAccX`. |
| 1021 | `rawAccY` | raw | Lateral-axis accelerometer raw register sample | MPU6050 raw counts | signed integer | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | Centering and filtering move to `cleanedAccY`. |
| 1022 | `rawAccZ` | raw | Vertical-axis accelerometer raw register sample | MPU6050 raw counts | signed integer | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | Centering and filtering move to `cleanedAccZ`. |
| 1023 | `rawTemp` | raw | MPU6050 temperature estimate in the bus's stored raw-data form | tenths of degrees C | signed integer where `365` means `36.5 C` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | The bus stores this raw-data form as `degC * 10` after the datasheet conversion. Both envs currently apply a lightweight EMA before publishing. |
| 1030 | `rawGyX` | raw | Gyroscope X axis in the bus's raw converted sensor-domain form | tenths of degrees per second | signed integer where `15` means `1.5 dps` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | The bus stores the nearest honest sensor-domain form as `deg/s * 10` using the MPU6050 default `131 LSB/dps` scale. Smoothing moves to `cleanedGyX`. |
| 1031 | `rawGyY` | raw | Gyroscope Y axis in the bus's raw converted sensor-domain form | tenths of degrees per second | signed integer where `15` means `1.5 dps` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | Smoothing moves to `cleanedGyY`. |
| 1032 | `rawGyZ` | raw | Gyroscope Z axis in the bus's raw converted sensor-domain form | tenths of degrees per second | signed integer where `15` means `1.5 dps` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | This is the uncleaned yaw-rate publication before startup-bias correction, smoothing, and deadband. Downstream yaw-rate users should prefer `cleanedGyZ`. |
| 1040 | `rawLidarFront` | raw | Minimum known front-lidar distance from the active front-left and front-right `VL53L0X` pair | millimeters | signed integer millimeters | `src/sensors/front_vl53l0x_service.cpp` | Active | This is a conservative aggregate used to keep one forward-facing lidar distance available on the bus. If both side lidars are known, this value is the smaller of the two. If only one is known, it mirrors that one. |
| 1041 | `rawLidarFrontRight` | raw | Front-right `VL53L0X` distance | millimeters | signed integer millimeters | `src/sensors/front_vl53l0x_service.cpp` | Active | On the active PAT004 truck model this uses IO-expander port `1`. |
| 1042 | `rawLidarFrontLeft` | raw | Front-left `VL53L0X` distance | millimeters | signed integer millimeters | `src/sensors/front_vl53l0x_service.cpp` | Active | On the active PAT004 truck model this uses IO-expander port `2`. |

### Calculated Values

| ID | Name | Tier | Meaning | Unit / Scale | Encoding in `long` | Producer | Status | Notes |
|---|---|---|---|---|---|---|---|---|
| 3001 | `calcHeading` | calculated | Integrated truck heading based on gyro Z over elapsed time | tenths of degrees | signed integer where `900` means `90.0 deg` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | Heading is now integrated from `cleanedGyZ` using a task-local sample interval and a small yaw-rate deadband. It has a stable `deg10` scale, but overall quality still depends on drift and noise. |
| 3002 | `calcSpeed` | calculated | Forward speed estimate from the plain MPU6050 path | millimeters per second | signed integer where sign follows forward or reverse acceleration history | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | `env:accsensor` now publishes a conservative estimate derived from `cleanedAccX` with thresholding, leakage, stationary reset, and clamping. `env:accsensorkalman` still holds this at `0` while that path remains parked. |
| 3003 | `calcDistance` | calculated | Forward distance estimate from the plain MPU6050 path | millimeters | signed integer accumulated from the current `calcSpeed` estimate | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | `env:accsensor` now integrates the conservative speed estimate into distance. `env:accsensorkalman` still holds this at `0` while that path remains parked. |
| 3004 | `calcXpos` | calculated | Reserved X position in world or map frame | intended millimeters | signed integer millimeters | none found in current code | Reserved | Present in the enum, but no active producer or consumer found. |
| 3005 | `calcYpos` | calculated | Reserved Y position in world or map frame | intended millimeters | signed integer millimeters | none found in current code | Reserved | Present in the enum, but no active producer or consumer found. |
| 3010 | `calculatedMagCourse` | calculated | Magnetic course (compass heading) from GY-271, 0=north, 900=east, etc. | tenths of degrees | signed integer | `src/sensors/gy271_service.cpp` | Active, Uncertain | Current service assumes truck forward aligns with sensor `-Y` and therefore derives heading from `atan2(X, -Y)`. This is functional for bring-up but still needs final mounting validation. |
| 3011 | `calculatedMagDisturbance` | calculated | Boolean: 1 if magnetic disturbance detected, 0 otherwise | 0 or 1 | integer | `src/sensors/gy271_service.cpp` | Active, Uncertain | Current service sets this from overflow or large magnitude deviation from expected Earth field until a proper calibration model exists. |

### Fused Values

The first live `fused*` variable now exists in the enum and runtime.

| ID | Name | Meaning | Intended Unit / Scale | Intended Producer | Status | Notes |
|---|---|---|---|---|---|---|
| 4001 | `fuseForwardClear` | Decision-ready forward-movement clearance state | integer state: `-1` unknown, `0` blocked, `1` clear | `src/fusion/clearance_fusion.cpp` via `src/fusion/fusion_service.cpp` | Active | Current rule uses front ultrasonic plus both front `VL53L0X` sensors. A known blocked reading on any of the three front sensors wins. Forward is only considered clear when the front ultrasonic and both front lidars are all known clear. |
| 4002 | `fuseTurnBias` | Decision-ready preference for which slight front turn has more free space | integer state: `-1` favor left, `0` neutral or unknown, `1` favor right | `src/fusion/clearance_fusion.cpp` via `src/fusion/fusion_service.cpp` | Active | Current rule compares the two front `VL53L0X` distances and only declares a bias when one side is meaningfully longer than the other. |
| 4003 | `fuseReverseClear` | Decision-ready reverse clearance state | boolean-like integer | future fusion task | Proposed | Same structure as `fuseForwardClear`, but for the rear path. |
| 4004 | `fuseLeftClear` | Decision-ready left-side clearance state | boolean-like integer | future fusion task | Proposed | Useful if the project later needs explicit per-side fused states in addition to `fuseTurnBias`. |
| 4005 | `fuseRightClear` | Decision-ready right-side clearance state | boolean-like integer | future fusion task | Proposed | Useful if the project later needs explicit per-side fused states in addition to `fuseTurnBias`. |
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
| 8100 | `driver_driverActivity` | driver | High-level driver mode or command category | small runtime enum integer | signed integer | `src/robots/driver.cpp` | Active, Uncertain | Current obstacle-avoidance runtime uses this for internal states such as forward drive and recovery phases. The code is now active, but the numeric mode table is not yet formalized as a stable external contract. |
| 8101 | `driver_desired_direction` | driver | Requested absolute direction for the driver abstraction | intended degrees or `deg10`, not yet formalized | signed integer | no active producer in current code | Deferred | Existing `driver.cpp` keeps this in file-local state only. |
| 8102 | `driver_desired_turn` | driver | Requested steering-turn command for the current driver runtime | normalized signed steering command | signed integer in the current `-100` to `100` steering scale | `src/robots/driver.cpp` | Active, Uncertain | In the current obstacle-avoidance runtime this mirrors the live steering command chosen from the fused turn bias and recovery behavior. |
| 8103 | `driver_desired_speed` | driver | Requested longitudinal travel speed command | normalized signed command today, likely physical speed later | signed integer | `src/robots/driver.cpp`; older bench use also exists in `src/z_main_hw-test.cpp` | Active, Uncertain | Positive means forward command, negative means reverse command, and the present scale is still the normalized `-100` to `100` actuator command rather than a formal physical unit. |
| 8104 | `driver_desired_distance` | driver | Requested travel distance | intended millimeters | signed integer | no active producer in current code | Deferred | Existing `driver.cpp` keeps this in file-local state only. |

## Current Semantic Gaps To Fix Later

The most important remaining semantic gaps are now about quality and fusion rather than base units:

1. Gyro drift and heading quality
   `calcHeading` now uses a task-local time base and a small yaw deadband, but long-term accuracy still depends on bias stability, mounting alignment, and motion outside the simplified model.

2. Motion estimation beyond heading
   `env:accsensor` now publishes a conservative accelerometer-only forward speed and distance estimate from `cleanedAccX`, but it is still a simplified 2D model without tilt compensation. `env:accsensorkalman` remains parked with both values held at `0`.

The current MPU6050 env difference is now intentional and should stay narrow:

- `env:accsensor`
  Uses a local in-project damping chain on the accelerometer channels and a conservative forward speed or distance estimator.
- `env:accsensorkalman`
  Uses Kalman filtering on the accelerometer channels and the same lightweight EMA on temperature and gyro channels, but still keeps speed and distance at `0`.

This difference is implementation strategy only. Both environments should keep the same shared-variable contract and debug-output shape so they can be compared by environment selection alone.

## Recommended Next Formalization Steps

1. Keep the stored `raw*` naming honest by describing the bus value as raw-data form plus explicit integer scaling.
2. Decide later whether temperature should also get an explicit `cleanedTemp` path or remain a single raw converted sensor-domain value.
3. Decide later whether `rawGy*` should remain scaled raw-data form or be split again into register-level counts plus converted sensor-domain values.
4. Keep the decision-ready layer under `fused*` ownership rather than letting more planner-facing meaning accumulate under `calc*`.
5. Update any remaining debug or telemetry surfaces so they expose both raw and cleaned values where comparison is useful.

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
  Implemented as helper logic for `fuseForwardClear` and `fuseTurnBias`.
- slow fusion logic
  Task exists as a stub so the architecture and runtime cadence are already in place before heavier fusion rules are added.

Current task split:

- fast fusion task, `100 ms`
  Updates `fuseForwardClear` and `fuseTurnBias`, and is the home for later near-term clearance gating.
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
