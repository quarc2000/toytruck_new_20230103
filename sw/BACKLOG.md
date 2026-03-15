# Backlog

## Deferred Items
- Define and implement a project-wide task-safe I2C access pattern. Current code uses direct `Wire` calls in multiple modules, so this is a known architectural gap but is deferred until the context and architecture baseline is in place.
- Introduce a formal `RISKS.md` file later if accepted deviations or technical risks grow beyond what is practical to track in `BACKLOG.md` and `CONFLICTS.md`.
- Resolve the onboard magnetometer path. The repository contains multiple magnetometer implementations, but it was not made to work on the truck and is not currently used in the active vehicle setup.
- Rework the merged logger path into a project-appropriate logging solution. The current logger appears to come from another repo, likely using Wi-Fi or MQTT/server logging, and should be preserved for later redesign but kept out of active builds for now because logging is still needed for debugging.
- Review the active project source for missing intent-level comments and add concise explanations where the why or control flow is not obvious, with priority on concurrency, task-safe I2C, shared-state flows, calibration logic, and deferred architecture stubs such as `Driver`.
- Finish the `Driver` abstraction so high-level code can command the vehicle in distances and directions while low-level code handles steering, motor control, and pose feedback. The current `driver.cpp` is only a stub and should remain deferred until the active cleanup pass is complete.
- Add the later "higher intelligence" layer that reads maps, current pose, and target position and then issues movement commands through the `Driver` abstraction. This depends on both the mapping work and the driver layer being in a cleaner state first.
- Resolve the PCA9685 path on the IO expander board. A missed reset pin was previously identified and corrected, but the device still did not work and was deferred because it was not required for the active truck setup.
- Mount and integrate the tested VL53L0X multiplexer setup on the truck when that hardware becomes part of the active build.
- Review whether `src/dynamics.cpp` is part of the active runtime and, if so, move its I2C usage onto the approved task-safe wrapper model.
- Resume the mapping task after cleanup: review the existing mapping foundation, decide the next increment, and continue only once the active firmware baseline is clean enough that new feature work will not build on stale or misleading code paths.
- Decide the first operational mapping grid resolution for the truck runtime, with explicit choice between 5 cm, 10 cm, or a hybrid approach.
- Add a real programmed-map loading path beyond the built-in sample map, such as firmware-embedded generated data or a host-provided format.
- Implement the first observed-map update pipeline from live sensors after the mapping foundation is reviewed.
- Define and implement pose tracking and alignment updates between observed and programmed maps.
- Add VL53L5 point-cloud projection rules into the 2D occupancy map once that hardware path is active.
- Explore an indoor ceiling-map layer for navigation using a long-range TOF sensor, analogous to an echosounder map in a boat. This could provide an additional overhead reference map for indoor driving and later fusion with the floor-plane or wall map.
- Revisit MPU6050-only longitudinal speed and distance estimation later as experimental work. Recent tuning improved some cases but remained non-linear, directionally unstable under reversals, and too sensitive to acceleration transients. For now, heading is useful; true speed and distance should later come from fusion or another sensor path rather than trusting inertial-only integration.
