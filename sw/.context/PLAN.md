# Plan

## Steps Completed
- Switched the active task from variable-model formalization to the first minimal real fusion implementation.
- Added `fuseForwardClear` to `setget`.
- Added the initial fusion module and verified `hwtest`.
- Refactored fusion into one `fusion` package with helper logic plus one `FusionService`.
- Verified the refactored `hwtest` build.
- Updated the docs and context so they describe the new one-package/two-cadence fusion model.
- Prepared the reusable architecture-illustration prompt for handoff.
- Identified the first live MPU6050 estimator failure mode from hardware testing.
- Fixed heading integration to use task-local timing and a small yaw deadband.
- Made `calcSpeed` and `calcDistance` safe by holding them at `0`.
- Verified `accsensor` and `accsensorkalman` compile.
- Flashed the corrected `accsensor` build to the truck and got a first plausible heading retest.
- Added `zeroAy` and `zeroAz` and changed both MPU6050 envs to center `AX`, `AY`, and `AZ` at startup.
- Verified `accsensor` and `accsensorkalman` still compile after the full 3-axis accelerometer centering change.
- Added stronger accelerometer damping and slow stationary zero tracking.
- Reflashed `accsensor` and confirmed on hardware that the plain path now looks good enough to proceed to forward-motion estimation.
- Implemented a conservative plain-path forward speed and distance estimator in `src/sensors/accsensor.cpp`.
- Verified `env:accsensor` compiles successfully with the new estimator.
- Updated the debug output and variable docs so they describe the plain/Kalman split honestly.
- Reworked the estimator so it uses filtered pre-deadband acceleration for motion estimation while keeping deadbanded values on the bus for display stability.
- Rebuilt and reflashed `accsensor` successfully after that correction.
- Softened the stationary response by requiring sustained stillness before zeroing speed and weakening the leakage.
- Rebuilt and reflashed `accsensor` successfully after the stationary-response tuning.
- Strengthened the sustained-stillness stop decay so speed settles to `0` cleanly after a stop.
- Rebuilt and reflashed `accsensor` successfully after the stop-decay tuning.
- Reduced active-motion underestimation by increasing the motion gain and weakening leakage during real movement.
- Rebuilt and reflashed `accsensor` successfully after the active-motion tuning.
- Reverted the last low-motion and reversal experiment after live testing showed it was worse than the previous version.
- Added the MPU6050-only speed and distance limitation to the backlog as deferred experimental work.
- Rebuilt and reflashed the reverted `accsensor` version successfully.
- Built `env:expander` successfully.
- Flashed `env:expander` successfully to `COM7`.
- Updated `src/z_main_expander.cpp` so pins `8` and `9` blink explicitly for indicator discovery.
- Fixed `src/expander.cpp` so pins `8` and `9` are configured as outputs and GPIO writes preserve the rest of the MCP23017 port.
- Rebuilt and reflashed `env:expander` successfully after the expander implementation fix.
- Extended the simple expander test so brake light pin `1` is asserted during the idle/deceleration part of the blink cycle.
- Rebuilt and reflashed `env:expander` successfully after adding brake light pin `1` to the test pattern.
- Reworked the simple expander test into dedicated phases for pin `0`, pin `1`, pin `8`, and pin `9` so the harness can be verified directly.
- Rebuilt `env:expander` successfully with the explicit four-phase light test.
- Confirmed that the test harness is good enough to move from manual GPIO phasing to a real light-control task.
- Added `include/lights/light_service.h` and `src/lights/light_service.cpp` as the first real expander-backed light-control task.
- Updated `src/expander.cpp` so reverse light pin `11` is also configured as an output.
- Wired the light service into `src/z_main_hw-test.cpp` and `env:hwtest`.
- Verified `env:hwtest` compiles successfully with the new light service.
- Flashed `env:hwtest` successfully to the truck with the integrated light service.
- Added `basic_logger` and `basic_web_server` under `include/basic_telemetry` and `src/basic_telemetry`.
- Added the safe stationary `src/z_main_basic_telemetry.cpp` test entry point.
- Added `env:basictelemetry` and verified it compiles successfully.
- Configured the basic web server to always start a direct debug AP and also attempt station-mode connection.
- Flashed `env:basictelemetry` successfully to the truck.
- Updated `.context/resources/boards/IO_EXPANDER.md` with the recent verified lighting and expander findings.
- Simplified the telemetry Wi‑Fi behavior again to AP-only with fixed SSID `ToyTruckDebug`, and verified that variant compiles successfully.

## In Progress
- Flash the fixed-SSID AP-only `basictelemetry` build to the truck.

## Steps Remaining
- Verify the Wi‑Fi connection path and web outputs on a client device.
- Record the connection details and any follow-up improvements needed for the web UI or logger.

## Definition Of Done
- `calcHeading` has a clear local integration basis and behaves plausibly in live testing.
- The simple expander test builds and flashes successfully.
- The first expander hardware behavior has been observed.
- The next expander-specific follow-up is clear from that observation.
- A real light-control task is running on hardware through the expander path.
- A basic Wi-Fi debug page exists for untethered truck diagnostics.
