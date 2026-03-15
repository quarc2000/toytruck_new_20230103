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
| 0002 | `zeroGz` | calibration | Gyro Z zero offset used to bias-correct MPU6050 yaw-rate readings | MPU6050 raw gyro counts | signed 32-bit integer holding average startup sample | `ACCsensor::Begin()` and `ACCsensorKalman::Begin()` | Active | This replaces the older hardcoded yaw-rate bias with a startup average stored in the shared bus. |

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
| 1020 | `rawAccX` | raw | Forward-axis acceleration after zero-offset correction | MPU6050 raw counts after subtracting `zeroAx` | signed integer | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | The current code stores counts, not `m/s^2` or `mg`. Kalman variant stores filtered counts. |
| 1021 | `rawAccY` | raw | Lateral-axis acceleration | MPU6050 raw counts | signed integer | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | Kalman variant stores filtered counts. Unit is still raw sensor scale. |
| 1022 | `rawAccZ` | raw | Vertical-axis acceleration | MPU6050 raw counts | signed integer | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | Kalman variant stores filtered counts. Unit is still raw sensor scale. |
| 1023 | `rawTemp` | raw | MPU6050 temperature estimate | tenths of degrees C | signed integer where `365` means `36.5 C` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | Uses the MPU6050 datasheet conversion in integer form and stores the result as `degC10`. |
| 1030 | `rawGyX` | raw | Gyroscope X axis | tenths of degrees per second | signed integer where `15` means `1.5 dps` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | Uses the MPU6050 default `131 LSB/dps` scale and stores `deg/s * 10`. |
| 1031 | `rawGyY` | raw | Gyroscope Y axis | tenths of degrees per second | signed integer where `15` means `1.5 dps` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | Uses the MPU6050 default `131 LSB/dps` scale and stores `deg/s * 10`. |
| 1032 | `rawGyZ` | raw | Gyroscope Z axis used for yaw-rate updates | tenths of degrees per second | signed integer where `15` means `1.5 dps` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active | Stored in the same `deg/s * 10` unit as `rawGyX` and `rawGyY` after subtracting the startup `zeroGz` calibration. |
| 1040 | `rawLidarFront` | raw | Reserved front lidar distance | intended millimeters | signed integer millimeters | none found in current code | Reserved | Present for a future lidar path; no active producer today. |

### Calculated Values

| ID | Name | Tier | Meaning | Unit / Scale | Encoding in `long` | Producer | Status | Notes |
|---|---|---|---|---|---|---|---|---|
| 2001 | `calcHeading` | calc | Integrated truck heading based on gyro Z over elapsed time | tenths of degrees | signed integer where `900` means `90.0 deg` | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | Heading is integrated from the startup-bias-corrected `rawGyZ` using elapsed milliseconds into a stable `deg10` scale, but overall heading quality still depends on drift and noise. |
| 2002 | `calcSpeed` | calc | Integrated forward speed estimate from acceleration history | code-specific scaled integer, not yet stable SI | signed integer | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | The current code integrates corrected acceleration counts over elapsed milliseconds. Debug output interprets this through `/(8*2048)`, but the stored unit is not yet formally defined. |
| 2003 | `calcDistance` | calc | Integrated forward travel estimate from `calcSpeed` over elapsed time | intended millimeters, but depends on current `calcSpeed` scaling | signed integer | `src/sensors/accsensor.cpp`, `src/sensors/accsensorkalman.cpp` | Active, Uncertain | Comment says millimeters, but this inherits all unit ambiguity from `calcSpeed`. |
| 2004 | `calcXpos` | calc | Reserved X position in world or map frame | intended millimeters | signed integer millimeters | none found in current code | Reserved | Present in the enum, but no active producer or consumer found. |
| 2005 | `calcYpos` | calc | Reserved Y position in world or map frame | intended millimeters | signed integer millimeters | none found in current code | Reserved | Present in the enum, but no active producer or consumer found. |

### Fused Values

There are no active `fuse*` variables in the enum yet.

These IDs are reserved as the first recommended fused outputs:

| ID | Proposed Name | Meaning | Intended Unit / Scale | Intended Producer | Status | Notes |
|---|---|---|---|---|---|---|
| 3001 | `fuseForwardClear` | Decision-ready forward-movement clearance state | boolean-like integer, `0` blocked, `1` clear, negative values optional for unknown or fault | future fusion task | Proposed | Should combine front ultrasonic, front lidar, map occupancy, and confidence policy. |
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

The most important unresolved unit problems are:

1. `calcSpeed`
   Current storage is an integration accumulator in project-specific scale, not yet a stable physical unit such as `mm/s`.

2. `calcDistance`
   Intended to be millimeters, but inherits uncertainty from `calcSpeed`.

## Recommended Next Formalization Steps

1. Freeze explicit units for gyro, heading, speed, and distance.
2. Decide whether `rawGy*` should remain "scaled raw" or be split into a true raw value plus a calculated rate value.
3. Introduce the first `fuse*` variables only after the raw and calc units are stable.
4. Update enum comments in `include/variables/setget.h` so they match this document.
5. Once this document is accepted as the maintained source-of-truth, update `AGENTS.md` so every variable change must update this file in the same work item.

## Recommended Fusion Ownership

The first fusion layer should not be spread across unrelated sensor tasks. A small dedicated package is the cleaner model.

Recommended structure:

- `src/fusion/clearance_fusion.cpp`
  Produces direction-clearance booleans or confidence-coded integers such as `fuseForwardClear`, `fuseReverseClear`, `fuseLeftClear`, and `fuseRightClear`.
- `src/fusion/pose_fusion.cpp`
  Produces fused pose or motion-state values such as `fuseHeadingDeg10`, `fuseSpeedMmPs`, and a later `fusePosePacked`.

Recommended task split:

- fast fusion task, around `20-50 ms`
  Should combine close-range obstacle inputs that gate motion decisions.
- slower pose or map fusion task, around `50-200 ms`
  Should combine motion estimates, map alignment, and later richer range data.

Why:

- obstacle clearance needs quicker refresh for safe control loops
- pose and map fusion usually depend on noisier or slower-changing signals and can tolerate a slower cadence
- separating them keeps each fusion task easier to reason about and test

## First Normalization Targets

If the next step moves from documentation into code semantics, the safest normalization order is:

1. `calcSpeed`
   Recommended target unit is `mm/s`.
2. `calcDistance`
   Recommended target unit is `mm`, but only after `calcSpeed` is stable.

This order keeps the dependencies clear: heading now has a stable unit with a startup bias calibration, and distance still depends on speed scaling.

## Initial Illustration Prompt

Use this prompt for a future architecture illustration:

> Create a clean engineering diagram of the toy-truck firmware data model. Show sensor tasks on the left, the shared `setget` bus in the center, and actuator, map, driver, and future fusion tasks on the right. Group variables into five color-coded bands: calibration (`0000`), raw (`1000`), calculated (`2000`), fused (`3000`), map (`4000`), and driver or actuator (`5000`). Show that every shared value is a signed 32-bit integer. Highlight active flows in solid lines and reserved or deferred flows in dashed lines. Emphasize that `task_safe_wire` is the only direct `Wire` path. Style should feel like a polished embedded-systems architecture poster: precise labels, minimal clutter, balanced whitespace, readable typography, and subtle technical color accents rather than generic flowchart styling.
