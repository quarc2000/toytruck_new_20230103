# Plan

## Steps Completed
- Defined the shared-variable task scope and switched the active task from general cleanup to variable-model formalization.
- Created `docs/VARIABLE_MODEL.md` as the current source-of-truth for shared-variable IDs, meaning, units, encoding, and future `fuse*` placeholders.
- Updated `include/variables/setget.h` comments to align better with the variable model.
- Normalized MPU6050 temperature semantics so `rawTemp` is stored as `degC10`.
- Normalized MPU6050 gyro semantics so `rawGyX`, `rawGyY`, and `rawGyZ` are stored as `deg/s * 10`.
- Normalized `calcHeading` so it is stored as `deg * 10`.
- Replaced the old hardcoded gyro-Z bias with startup `zeroGz` calibration in both MPU6050 code paths.
- Added datasheet and chip-note resource structure, including the first `MPU6050.md` chip note with tightened chip-only scope.
- Kept the two MPU6050 variants directly comparable by environment only:
  - `env:accsensor` now builds as the plain in-project filtered path
  - `env:accsensorkalman` now builds as the Kalman comparison path
  - both keep the same `ACCsensor` interface, shared-variable outputs, and debug-output shape
- Verified the comparison builds with:
  - `pio run -j1 -e accsensor`
  - `pio run -j1 -e accsensorkalman`
- Decided that the plain-path EMA constants remain internal implementation details for now rather than part of the documented variable contract.
- Normalized `calcSpeed` to `mm/s` in both MPU6050 envs.
- Normalized `calcDistance` to `mm` in both MPU6050 envs.
- Verified the updated speed and distance semantics with:
  - `pio run -j1 -e accsensor`
  - `pio run -j1 -e accsensorkalman`
- Defined the concrete boundary between current `calc*` motion estimates and future `fuse*` motion estimates.
- Documented the first concrete fusion-package split:
  - `clearance_fusion` for fast decision gating
  - `pose_fusion` for slower best-estimate pose and motion outputs

## In Progress
- Close the current variable-model task cleanly in the task files now that the main deliverables are in place.

## Steps Remaining
- Decide in a later task whether heading and motion confidence should be represented explicitly in future shared variables.

## Definition Of Done
- Variable naming classes, units, and meanings are documented clearly enough that another programmer or agent can use them correctly.
- Variable documentation IDs and series ranges are defined clearly enough that future additions have an obvious home.
- Existing variable comments and declarations no longer rely on hidden assumptions about sensor configuration.
- A `docs/` variable reference exists and is strong enough to become a maintained source of truth for future variable changes.
- The conceptual place for `fuse*` variables and their producer tasks is documented.
- A reusable illustration prompt exists for communicating the truck data structures visually.
