# Task

## Active Task
Extend front fusion and add a simple obstacle-avoidance driver.

## Note
Use the three front sensors to produce better immediate driving guidance.

Definition of done:
- keep the existing fused straight-ahead clear signal
- add fused left/right preference indicating which slight turn has more free space ahead
- implement a driver that uses the fused front signals to:
  - drive forward when safe
  - stop before obstacles
  - back out in a smart direction when stuck
- use the side ultrasonic sensors to improve the backing direction choice
- include light behavior in the active avoidance runtime:
  - reverse light when backing
  - indicators for steering turns
  - faster indicator flash rate than the previous slow setting
- no target destination or route-planning logic in this task
