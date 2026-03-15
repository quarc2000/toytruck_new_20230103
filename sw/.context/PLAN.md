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

## In Progress
- Keep the two MPU6050 variants directly comparable by environment only:
  - `env:accsensor` as the plain in-project filtered path
  - `env:accsensorkalman` as the Kalman comparison path
  - same `ACCsensor` class interface
  - same shared-variable outputs
  - same debug-output shape
- Finish the current code pass:
  - `src/sensors/accsensor.cpp`: lightweight filtering on all published MPU6050 values
  - `src/sensors/accsensorkalman.cpp`: keep Kalman accel filtering, align temperature and gyro smoothing with the plain path
  - `src/z_main_accsensor_kalman.cpp`: align debug output with `src/z_main_accsensor.cpp`

## Steps Remaining
- Verify the comparison builds with:
  - `pio run -j1 -e accsensor`
  - `pio run -j1 -e accsensorkalman`
- Update `docs/VARIABLE_MODEL.md` so it explicitly documents the intended difference between the two MPU6050 environments as filter strategy rather than interface or variable contract.
- Decide whether the plain-path filtering constants should remain internal implementation details or be documented as tuning parameters.
- Move to the next unresolved variable semantics in dependency order:
  - gyro noise handling review after build verification
  - `calcSpeed`
  - `calcDistance`
- Draft the first concrete fusion-package split for `fuse*` producers once the raw and `calc*` variable semantics are stable enough.

## Definition Of Done
- Variable naming classes, units, and meanings are documented clearly enough that another programmer or agent can use them correctly.
- Variable documentation IDs and series ranges are defined clearly enough that future additions have an obvious home.
- Existing variable comments and declarations no longer rely on hidden assumptions about sensor configuration.
- A `docs/` variable reference exists and is strong enough to become a maintained source of truth for future variable changes.
- The conceptual place for `fuse*` variables and their producer tasks is documented.
- A reusable illustration prompt exists for communicating the truck data structures visually.
