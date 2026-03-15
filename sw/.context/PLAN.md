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

## In Progress
- Rebuild and reflash the reverted `accsensor` version.

## Steps Remaining
- Confirm the truck is back on the reverted `accsensor` build.
- Switch the active task to the expander path and define the first narrow verification step there.

## Definition Of Done
- `calcHeading` has a clear local integration basis and behaves plausibly in live testing.
- The last acceptable `accsensor` version is restored on the truck.
- The inertial-only speed and distance limitation is explicitly deferred.
- The active work is ready to move on to the expander path.
