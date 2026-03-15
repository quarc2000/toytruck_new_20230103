# State

## Current Task Memory
- Active task: formalize the shared-variable model around `setget`, with current focus on MPU6050 semantics and keeping the two MPU6050 implementations directly comparable by PlatformIO environment.
- `docs/VARIABLE_MODEL.md` is now the source-of-truth for shared-variable IDs, meaning, units, encoding, and proposed future `fuse*` values.
- `rawTemp` is normalized to `degC10`.
- `rawGyX`, `rawGyY`, and `rawGyZ` are normalized to `deg/s * 10`.
- `calcHeading` is normalized to `deg * 10`.
- `zeroGz` is now an active startup gyro-Z bias calibration value, replacing the old hardcoded bias.
- The current MPU6050 requirement is compatibility between the two sensor envs:
  - `env:accsensor` should be the plain in-project filtered path.
  - `env:accsensorkalman` should be the Kalman-based comparison path.
  - Both must keep the same `ACCsensor` class interface, shared-variable outputs, and comparable debug output so the environment alone selects the variant.
- The current in-progress code change is:
  - `src/sensors/accsensor.cpp`: add lightweight filtering on all published MPU6050 values.
  - `src/sensors/accsensorkalman.cpp`: keep Kalman filtering on accel, but align temperature and gyro smoothing with the plain path.
  - `src/z_main_accsensor_kalman.cpp`: align debug output with `src/z_main_accsensor.cpp`.
- Datasheet resources now exist under `.context/resources/datasheets/README.md`.
- Chip notes now exist under `.context/resources/chip_notes/`, and chip notes must stay chip-centered rather than project-centered.
