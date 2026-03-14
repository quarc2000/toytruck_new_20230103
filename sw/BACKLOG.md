# Backlog

## Deferred Items
- Define and implement a project-wide task-safe I2C access pattern. Current code uses direct `Wire` calls in multiple modules, so this is a known architectural gap but is deferred until the context and architecture baseline is in place.
- Introduce a formal `RISKS.md` file later if accepted deviations or technical risks grow beyond what is practical to track in `BACKLOG.md` and `CONFLICTS.md`.
- Resolve the onboard magnetometer path. The repository contains multiple magnetometer implementations, but it was not made to work on the truck and is not currently used in the active vehicle setup.
- Rework the merged logger path into a project-appropriate logging solution. The current logger appears to come from another repo, likely using Wi-Fi or MQTT/server logging, and should be preserved for later redesign but kept out of active builds for now because logging is still needed for debugging.
- Resolve the PCA9685 path on the IO expander board. A missed reset pin was previously identified and corrected, but the device still did not work and was deferred because it was not required for the active truck setup.
- Mount and integrate the tested VL53L0X multiplexer setup on the truck when that hardware becomes part of the active build.
- Rerun focused PlatformIO builds for `accsensor`, `accsensorkalman`, `compass`, `gy271`, `expander`, and `hwtest` after runtime access or environment permissions allow full archive packaging, because current sandbox runs still fail during `.a` archive rename operations under `.pio/build`.
- Recheck full `usensor` validation on hardware or in an unrestricted local PlatformIO environment. Current evidence shows `src/sensors/usensor.cpp` compiles, but complete firmware packaging could not be finished in the sandbox.
- Review whether `src/dynamics.cpp` is part of the active runtime and, if so, move its I2C usage onto the approved task-safe wrapper model.
- Resume the mapping task after cleanup: review the existing mapping foundation, decide the next increment, and continue only once the active firmware baseline is clean enough that new feature work will not build on stale or misleading code paths.
- Decide the first operational mapping grid resolution for the truck runtime, with explicit choice between 5 cm, 10 cm, or a hybrid approach.
- Add a real programmed-map loading path beyond the built-in sample map, such as firmware-embedded generated data or a host-provided format.
- Implement the first observed-map update pipeline from live sensors after the mapping foundation is reviewed.
- Define and implement pose tracking and alignment updates between observed and programmed maps.
- Add VL53L5 point-cloud projection rules into the 2D occupancy map once that hardware path is active.
