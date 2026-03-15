# Task

## Active Task
Restore the last acceptable plain `accsensor` MPU6050 build, defer inertial-only speed and distance work to the backlog, and then switch active attention to the expander path for the next hardware bring-up step.

## Completed In This Task
- The shared-variable model was formalized with stable IDs, clearer units, a variable reference, and an initial `calc*` versus `fuse*` boundary.
- The MPU6050 paths were normalized and made directly comparable by PlatformIO environment.
- The first live fused variable, `fuseForwardClear`, was implemented and verified.
- The fusion code was refactored into one `fusion` package with helper logic plus one `FusionService`.
- `env:hwtest` builds successfully with the unified fusion service.
- The docs now describe one fusion package with two task cadences.
- `calcHeading` now uses task-local timing plus a small yaw deadband in both MPU6050 envs.
- `calcSpeed` and `calcDistance` are now intentionally held safe at `0`.
- `accsensor` and `accsensorkalman` both compile after the estimator change.
- The updated `accsensor` build was flashed to the truck, and the first retest looked plausible.
- `zeroAy` and `zeroAz` were added so all three accelerometer axes are now centered at startup in both MPU6050 envs.
- Stronger accelerometer damping and slow stationary zero tracking were added, and the updated `accsensor` build was flashed successfully.
- Live testing now shows the plain path heading behaves plausibly enough to move on to forward-motion estimation.
- A first conservative forward speed and distance estimator was implemented in `src/sensors/accsensor.cpp` using thresholding, leakage, stationary reset, and clamping.
- `env:accsensor` compiles successfully with the new estimator.
- The plain debug view and `docs/VARIABLE_MODEL.md` now describe the new plain/Kalman split honestly.
- The later low-motion and reversal experiment was judged worse in live testing and has now been rolled back.
- MPU6050-only speed and distance are being deferred to the backlog as experimental rather than treated as a near-term deliverable.

## Task Plan
1. Rebuild and reflash the reverted `accsensor` version so the truck is back on the last acceptable behavior.
2. Record the inertial-only speed and distance limitation honestly in `BACKLOG.md` and current task memory.
3. Move the active task toward the expander path.

## Definition Of Done
- The truck is back on the last acceptable `accsensor` firmware version.
- The MPU6050-only speed and distance problem is explicitly deferred.
- The active task is no longer blocked on inertial distance tuning and is redirected to the expander path.

## Immediate Next Actions
- Rebuild and reflash the reverted `accsensor` version, then switch the active task to the expander path.
