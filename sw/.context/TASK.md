# Task

## Active Task
Add `VL53L5CX` support to the expander-mux sensor bring-up path.

## Scope
- Build on the successful expander-mux test approach used for `VL53L0X`.
- Add the minimum repo-local code needed to probe and read a `VL53L5CX` through the expander path.
- Do not use an external `VL53L5CX` library for this task.
- Keep I2C access task safe and consistent with the project `task_safe_wire` rule.
- The user plans to connect the `VL53L5CX` to expander port or channel `2`.
- Keep this separate from drivetrain code.

## Definition Of Done
- A dedicated `VL53L5CX` bring-up path exists in the repo.
- The code compiles successfully.
- The code can be flashed successfully.
- The test can probe and read useful data from the connected `VL53L5CX` through the expander-mux path.
