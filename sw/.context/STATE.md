# State

## Current Task Memory
- Active task: formalize the shared-variable model around `setget`, with current focus on MPU6050 semantics and keeping the two MPU6050 implementations directly comparable by PlatformIO environment.
- `docs/VARIABLE_MODEL.md` is now the source-of-truth for shared-variable IDs, meaning, units, encoding, and proposed future `fuse*` values.
- `rawTemp` is normalized to `degC10`.
- `rawGyX`, `rawGyY`, and `rawGyZ` are normalized to `deg/s * 10`.
- `calcHeading` is normalized to `deg * 10`.
- `calcSpeed` is now normalized to `mm/s`.
- `calcDistance` is now normalized to `mm`.
- `zeroGz` is now an active startup gyro-Z bias calibration value, replacing the old hardcoded bias.
- The current MPU6050 requirement is compatibility between the two sensor envs:
  - `env:accsensor` should be the plain in-project filtered path.
  - `env:accsensorkalman` should be the Kalman-based comparison path.
  - Both must keep the same `ACCsensor` class interface, shared-variable outputs, and comparable debug output so the environment alone selects the variant.
- The MPU6050 compatibility pass is now functionally complete:
  - `env:accsensor` builds successfully with lightweight in-project EMA filtering on all published MPU6050 values.
  - `env:accsensorkalman` builds successfully with Kalman filtering on accel and matching EMA smoothing on temperature and gyro outputs.
  - `src/z_main_accsensor.cpp` and `src/z_main_accsensor_kalman.cpp` now expose comparable debug output.
- The `calcSpeed` / `calcDistance` normalization pass is also now complete and verified in both MPU6050 envs.
- The `calc*` versus `fuse*` boundary is now explicit:
  - `calcHeading`, `calcSpeed`, and `calcDistance` remain fast local inertial estimates
  - later planner-facing or confidence-aware motion outputs should move into `fuseHeadingDeg10`, `fuseSpeedMmPs`, and `fusePosePacked`
- The remaining live work for this task is just to close it cleanly in the task files, because the variable-model source of truth, the stable IDs, the units, the fusion-package concept, and the illustration prompt now all exist.
- The current design decision is that the plain-path EMA constants remain internal implementation details for now, not part of the shared-variable contract.
- Datasheet resources now exist under `.context/resources/datasheets/README.md`.
- Chip notes now exist under `.context/resources/chip_notes/`, and chip notes must stay chip-centered rather than project-centered.
