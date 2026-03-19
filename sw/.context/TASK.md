# Task

## Active Task
Add a real fused heading path to `env:exploremap` by integrating the GY-271 on expander port `3`, calculating a magnetic course, and publishing a fused course that uses magnetic as the long-term reference while gyro remains the short-term motion source.

## Note
This task may change the live heading source for the explorer, but it should do so conservatively:
- keep raw gyro heading available as `calcHeading`
- keep magnetic heading available as `calculatedMagCourse`
- publish a fused heading as the runtime-facing course
- prefer magnetic as the long-term reference, but reject or soften magnetic corrections when the disturbance signal says the reading is not trustworthy
- longer-term map-based heading correction is still deferred

Definition of done:
- `env:exploremap` starts the GY-271 service on expander port `3`
- a magnetic course is published and visible for diagnostics
- a fused heading is published and used by the explorer instead of pure gyro heading
- the fused heading reduces gyro drift without simply replacing gyro with magnetic
- docs and shared-variable references are updated in the same work item
- keep `.context/PLAN.md` and `.context/STATE.md` aligned with the task
