# Plan

## Steps Completed
- Reviewed the current fusion and driver baseline.
- Confirmed `fuseForwardClear` exists and is driven by the fast fusion task.
- Confirmed `Driver` is still mostly a stub and does not yet own any real runtime behavior.
- Verified the active PAT004 model note and found that front `VL53L0X` mapping must follow:
  - port `1` = front right
  - port `2` = front left
- Confirmed there is no runtime service yet publishing both front `VL53L0X` distances into shared variables.
- Confirmed `hwtest` is the wrong integration point for this task because it directly scripts motor and steering motion in its loop.
- Added the minimum new shared variables:
  - `rawLidarFrontRight`
  - `rawLidarFrontLeft`
  - `fuseTurnBias`
- Added a dedicated front-`VL53L0X` service for PAT004 on expander ports `1` and `2`.
- Extended the fast fusion path so it now publishes:
  - conservative `fuseForwardClear`
  - signed `fuseTurnBias`
- Implemented the first real driver runtime behavior:
  - forward drive when front path is clear
  - slight steering bias toward the better side
  - stop and short reverse recovery when blocked
- Added a dedicated runtime env:
  - `env:frontavoid`
  - entry point `src/z_main_front_avoid.cpp`
- Built `env:frontavoid` successfully with the local PlatformIO cache workflow.
- Extended the reverse recovery choice so it now uses `rawDistLeft` and `rawDistRight` before falling back to `fuseTurnBias`.
- Added `LightService` into `env:frontavoid` so reverse light and steering indicators are active in the avoidance runtime.
- Increased indicator blink cadence from the previous slow setting to a `250 ms` toggle (`2 Hz` blink cycle).
- Restored the actuator layer toward the project's explicit `Begin()` pattern:
  - `Motor` now initializes in `Begin()` instead of its constructor
  - repeated `Begin()` calls are harmless
  - `Motor::driving()` self-initializes if needed
  - `Steer::Begin()` now calls `Config::Begin()` explicitly and `direction()` self-initializes if needed
- Rebuilt `env:frontavoid` successfully after the explicit-init refactor.
- Confirmed on hardware that PAT004 identification now prints explicitly through the corrected init path.
- Extended `env:frontavoid` with the minimum MPU6050 path needed for forward heading hold.
- Added front-clear hysteresis and ignored obviously bogus tiny lidar distances in the fast fusion path.
- Lengthened and stabilized reverse recovery behavior in the driver runtime.
- Rebuilt `env:frontavoid` successfully after the heading-hold and hysteresis changes.

## In Progress
- Refine `frontavoid` after the second floor test:
  - add adaptive straight-driving trim so steering neutral can learn from yaw drift
  - make turn choice sticky so a chosen side is kept until the truck has made real forward progress or becomes blocked again
  - reduce the chance of still hitting front corners by tightening the forward-clear behavior further if needed

## Steps Remaining
- Rebuild `env:frontavoid` and upload it for the next floor test.
