# Task

## Active Task
Refactor `Driver` to a minimal, explicit control model using separate `mode` and `state`, and remove the current mixed-priority steering stack that causes left-right confusion near walls.

## Note
Target model requested by user:
- `mode`: `normal`, `recovery`
- `state`: `idle`, `forward`, `reverse`, `forward_left`, `forward_right`
- persistent `avoidance_direction`: `left` or `right`
- clarified behavior rules from user:
  - `forward_left` and `forward_right` are not only obstacle reactions; they are also valid commanded course-change states while navigating to goal
  - `reverse` is not recovery-only; it must also be available for planned map-navigation maneuvers
  - avoidance priority is always highest over course intent
  - max steering is allowed for any explicit turning state (including user or planner requested sharp turn), not only avoidance
  - straight-driving correction must stay small and smooth when state is straight forward travel

Definition of done:
- `Driver` no longer relies on the current large helper- and constant-heavy arbitration path
- steering sign behavior is explicit and direction-aware:
  - forward near left wall => steer right
  - forward near right wall => steer left
  - reverse uses opposite wheel sign for same yaw intent
- state priority is explicit:
  - avoidance > commanded turn > straight stabilization
- max steering is available in explicit turning states; straight mode uses smooth low-amplitude correction
- `env:exploremap` builds and uploads
- truck-side check confirms no "turn into wall" behavior on left and right wall tests
- `TASK`, `PLAN`, `STATE`, and architecture notes are updated to the final model
