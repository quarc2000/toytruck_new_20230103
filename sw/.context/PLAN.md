# Plan

## Steps Completed
- Closed the previous active explorer implementation pass at the correct boundary:
  - `env:exploremap` exists
  - desk-side implementation and docs are complete
  - remaining work is hardware upload and truck validation, moved to backlog
- Switched the active task to the ESP32-LCD-4.3 companion display client so work can continue independently of truck availability.
- Captured the first companion-display architecture baseline and added the first board-specific scaffold:
  - `docs/COMPANION_DISPLAY_ARCHITECTURE.md`
  - `env:lcd43clientboot`
  - `src/display_client/lcd43_client_bootstrap.cpp`
- Added `docs/COMPANION_DISPLAY_API.md` as the truck-owned API handoff document for `/status`, `/map`, and `/logs`.
- Removed the LCD experiment remnants from the truck repo while keeping the two display documents:
  - removed `env:lcd43clientboot`
  - removed the display bootstrap code
  - removed `tools/pio_force_s3_toolchain.py`
  - removed the LCD-specific section from `.context/ARCHITECTURE.md`
  - kept `docs/COMPANION_DISPLAY_API.md`
  - kept `docs/COMPANION_DISPLAY_ARCHITECTURE.md`
- Improved the live `/map` API for `env:exploremap` with additive metadata:
  - `apiVersion`
  - `state`
  - `frontierReachable`
  - `frontierBias`
  - `posePacked`
  - `robot`
- Added a first safe remote-control API for `env:exploremap`:
  - new control endpoints:
    - `GET /control`
    - `POST /control/enable`
    - `POST /control/drive`
    - `POST /control/stop`
    - `POST /control/disable`
  - explorer actuator ownership now yields cleanly while remote control is active
  - remote control now uses a watchdog timeout and fail-safe stop
  - `docs/COMPANION_DISPLAY_API.md`, `docs/COMPANION_DISPLAY_ARCHITECTURE.md`, and `.context/ARCHITECTURE.md` were updated to match
  - verified with `pio run -e exploremap -j1`
- Tuned live `env:exploremap` behavior from truck-side testing:
  - fixed the forward heading-hold bug where heading correction was computed but not applied to steering
  - added immediate yaw-rate damping from `cleanedGyZ`
  - refreshed the forward heading target at actual forward-motion start
  - replaced neutral-only side centering with side-wall stand-off and emergency wall-avoidance correction
  - relaxed forward-clear fusion so open space can still count as clear when both front lidars agree but the front ultrasonic is only out of range
  - verified with `pio run -e exploremap -j1`
- Added a wall-angle-aware forward correction pass for PAT004:
  - increased heading and yaw correction strength for faster straightening in open runs
  - derived a relative front-wall angle from the two forward-facing `VL53L0X` sensors using the PAT004 `105 mm` lidar spacing
  - applied extra forward steering correction when approaching a wall at an angle, especially once the wall is inside roughly one truck length plus margin
  - verified with `pio run -e exploremap -j1`
  - uploaded successfully to `COM7`

## In Progress
- Run the next truck-side validation pass for the wall-angle-aware `env:exploremap` runtime:
  - retest open-area straightness and front-wall approach angle
  - verify that forward motion now keeps turning away from an angled wall after the first turn decision

## Steps Remaining
- upload to the truck and validate:
  - straightness over the first `15-20 cm`
  - side-wall stand-off behavior near `10 cm`
  - emergency response before side distance drops below `5 cm`
  - open-space forward motion with no nearby side walls
  - front-wall approach angle correction in the `8 x 8 m` area
- decide whether the next API increment after remote control should add:
  - a dedicated control-status endpoint
  - explicit command source reporting in more responses
  - a compact changed-cells or viewport path later
- Remove stray local cache directories `p3`, `p4`, and `p5` once destructive shell commands are available outside the current blocked session policy.
