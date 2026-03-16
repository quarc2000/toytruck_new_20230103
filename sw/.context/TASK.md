# Task

## Active Task
No active task.

## Note
The MPU6050 raw-versus-cleaned split is complete. True raw accel and gyro values are now preserved, explicit `cleanedAcc*` and `cleanedGy*` values exist, and downstream consumers that depend on filtered motion data now use the cleaned layer.
