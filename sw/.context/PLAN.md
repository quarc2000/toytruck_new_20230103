# Plan

## Steps Completed
- Closed the active GY-271 / QMC5883L task and moved the unresolved work to `BACKLOG.md`.
- Added the requested temporary XY bias offsets to the live GY271 service:
  - `X += 400`
  - `Y -= 150`

## In Progress
- Rebuild and flash `env:gy271service` so the user can test the adjusted output immediately.

## Steps Remaining
- Confirm whether the bias-corrected values are easier to interpret.
- Close this small retuning task and return to no active task.
