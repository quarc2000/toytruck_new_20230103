# State

## Current Task Memory
- No active task.
- The retained result from the last completed task is:
  - true `rawAcc*` and `rawGy*` are now preserved
  - `cleanedAcc*` and `cleanedGy*` now carry the filtered or centered MPU6050 motion signals
  - downstream consumers that rely on filtered motion data now use the cleaned layer
  - the affected environments compiled successfully
