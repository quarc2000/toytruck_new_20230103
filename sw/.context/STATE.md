# State

## Current Task Memory
- Active subtask:
  - verify the new simplified `Driver` mode/state refactor on `env:exploremap` and hardware
- Current confirmed pain point:
  - need fresh truck-side validation after side-authoritative remap and 60 cm front-corner avoidance reset update
- Current agreed design direction:
  - `mode`: `normal`, `recovery`
  - `state`: `idle`, `forward`, `reverse`, `forward_left`, `forward_right`
  - persistent `avoidance_direction` used during recovery and immediate post-recovery
  - explicit turn states may use max steering even when not in avoidance
- Immediate next action:
  - run focused floor checks on latest uploaded `env:exploremap` build (COM7): side-based avoidance direction, wall-follow precedence, and reset when turning-corner front lidar is >60 cm
