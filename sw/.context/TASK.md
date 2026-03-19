# Task

## Active Task
Tune the autonomous `env:exploremap` runtime from live truck testing so it drives straighter, respects side-wall clearance better, and continues forward in open space when the path is credibly clear.

## Note
This is a live behavior-tuning task for the active explorer runtime. Keep the current map and control APIs intact while improving forward heading hold, side-wall clearance behavior, and open-space forward-clearance handling.

Definition of done:
- keep the current map and control APIs working
- reduce early forward heading drift materially from the live baseline
- make the truck hold a safer side-wall stand-off instead of drifting into walls
- allow forward movement in open space when side walls are absent but the front path is still credibly clear
- keep `.context/PLAN.md` and `.context/STATE.md` aligned with the new task
