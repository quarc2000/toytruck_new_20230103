# Plan

## Steps Completed
- Confirmed the current `Driver` implementation has grown into a large mixed-priority steering path with many constants and helper functions.
- Confirmed user-selected simplification direction:
  - explicit `mode`: `normal`, `recovery`
  - explicit `state`: `idle`, `forward`, `reverse`, `forward_left`, `forward_right`
  - explicit persistent avoidance direction: `left` or `right`
- Captured the requirement that quick fixes are disallowed and root-cause fixes must stay in the correct control layer.
- Captured final steering-authority clarification:
  - max steering is allowed for any explicit turn command (planner/user), not only obstacle avoidance.
- Locked behavior contract in architecture and task context:
  - avoidance priority remains highest
  - explicit turn states are separate from straight stabilization
  - reverse steering sign inversion for same yaw intent is retained
- Replaced `src/robots/driver.cpp` with a compact explicit mode/state implementation:
  - `mode`: `normal` and `recovery`
  - `state`: `idle`, `forward`, `reverse`, `forward_left`, `forward_right`
  - single steering output path per loop, no summed competing arbitration branches
  - wall-follow correction sign for left/right side made explicit in straight-forward state
  - recovery keeps a persistent `avoidance_direction` and applies post-recovery forward turn boost
  - max steering allowed in explicit turn states regardless of avoidance origin
- Applied requested forward-speed policy cleanup in `src/robots/driver.cpp`:
  - `DEFAULT_FORWARD_SPEED` set to `100`
  - `FORWARD_SLOW_SPEED` set to `90`
  - added concise constant-purpose comments to reduce ambiguity during tuning
- Fixed forward-control regression where straight driving could enter max-turn behavior:
  - `-1/0/1` turn bias no longer triggers explicit max-turn states
  - max-turn states remain for explicit sharp command turns and recovery
  - added side-wall guard steering in straight-forward state to push away from close side walls
- Fixed first-avoidance direction and indecisive flipping:
  - avoidance chooser now treats unknown side readings as potentially open and avoids known very-close sides
  - added decision hysteresis (`AVOID_FLIP_DELTA_CM`) to avoid noisy side swapping
  - replaced time-based commit with open-space-based release of committed direction
- Fixed targeted sign-direction regressions only in the two requested algorithms:
  - obstacle-avoidance turn-side chooser (`chooseAvoidanceDirection` plus `maybeFlipAvoidanceDirection`)
  - close-wall away-from-wall steering (`wallFollowSteer`, `sideGuardSteer`, `sideEmergencySteer`)

## In Progress
- Verify the refactor in `env:exploremap` and upload for truck-side behavior checks.

## Steps Remaining
- Step 1: verify:
  - build `env:exploremap` (done via `pio-local.ps1; pio run -e exploremap -j1`)
  - upload and run focused live checks: (upload succeeded to `COM7` after avoidance-direction fix)
    - left wall approach while moving forward
    - right wall approach while moving forward
    - corner recovery (forward-reverse-forward sequence)
    - straight run before obstacle
- Step 2: finalize docs:
  - append concise summary to `.context/PROJECT_HISTORY.md`
