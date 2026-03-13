# Backlog

## Deferred Items
- Define and implement a project-wide task-safe I2C access pattern. Current code uses direct `Wire` calls in multiple modules, so this is a known architectural gap but is deferred until the context and architecture baseline is in place.
- Introduce a formal `RISKS.md` file later if accepted deviations or technical risks grow beyond what is practical to track in `BACKLOG.md` and `CONFLICTS.md`.
- Resolve the onboard magnetometer path. The repository contains multiple magnetometer implementations, but it was not made to work on the truck and is not currently used in the active vehicle setup.
- Resolve the PCA9685 path on the IO expander board. A missed reset pin was previously identified and corrected, but the device still did not work and was deferred because it was not required for the active truck setup.
- Mount and integrate the tested VL53L0X multiplexer setup on the truck when that hardware becomes part of the active build.
