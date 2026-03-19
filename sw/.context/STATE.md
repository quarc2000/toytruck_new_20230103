# State

## Current Task Memory
- Active subtask: tune live `env:exploremap` behavior from truck test feedback.
- Implemented in code:
  - forward heading correction is now actually applied into the steering command
  - added immediate yaw damping from `cleanedGyZ`
  - forward heading target now refreshes at real forward-motion start
  - side-wall correction now tries to hold about `10 cm` stand-off and reacts strongly by `5 cm`
  - forward-clear fusion now accepts open space when both front lidars are clearly open and the front ultrasonic is only unknown
- Latest tuning increment:
  - stronger heading and yaw correction for faster straightening
  - front-wall angle estimated from the PAT004 front lidar pair using `105 mm` spacing
  - extra steering correction now stays active when approaching a wall at an angle
- Local verification complete:
  - `pio run -e exploremap -j1` succeeded after the tuning pass
- Uploaded successfully to `COM7`.
- Next real check is truck-side retest in:
  - the small enclosure for straightness and side-wall behavior
  - the larger open space for forward motion, wall-approach angle, and turn-away behavior after the first wall decision
- Remaining manual cleanup outside this blocked session:
  - delete stray cache directories `p3`, `p4`, and `p5`
- Companion-display handoff docs intentionally kept:
  - `docs/COMPANION_DISPLAY_API.md`
  - `docs/COMPANION_DISPLAY_ARCHITECTURE.md`
