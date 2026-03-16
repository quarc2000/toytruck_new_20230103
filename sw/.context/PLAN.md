# Plan

## Steps Completed
- Closed the VL53L5CX bring-up task and moved the unresolved work to `BACKLOG.md`.
- Updated the shared-variable taxonomy docs to use:
  - `1000` raw
  - `2000` cleaned
  - `3000` calculated
  - `4000` fused
  - `7000` map
  - `8000` driver
  - `9000` config or system
- Replaced the separate series and naming bullets with aligned taxonomy tables in the architecture and variable-model docs.
- Kept current `calc*` and `fuse*` code symbols stable while documenting them as members of the broader `calculated*` and `fused*` layers.
- Split the MPU6050 accel and gyro publications so true `raw*` values stay available while `cleanedAcc*` and `cleanedGy*` carry the filtered or centered motion signals.
- Updated downstream consumers to use the cleaned layer where filtered motion data is intended, including light-service logic, MPU6050 debug views, `hwtest`, and the basic telemetry pages.
- Verified the affected environments compile:
  - `accsensor`
  - `accsensorkalman`
  - `hwtest`
  - `basictelemetry`
  - `basictelemetrymin`

## In Progress
- No active plan. Waiting for the next task.

## Steps Remaining
- None for the current task, because there is no active task.
