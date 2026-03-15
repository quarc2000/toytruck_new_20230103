# Task

## Active Task
Formalize the shared variable model used by `setget` so variable names, units, meaning, and future fusion outputs are explicit and stable enough to support navigation, planning, and later sensor fusion without guesswork.

## Completed In This Task
- Previous board-analysis task completed with initial board resource files for the TOY controller board and IO expander.
- Known task-safety concern around direct `Wire` usage has now been elevated into the active engineering task.
- Conflict `CON-0001` has been opened and explicitly confirmed by the user as active remediation work.
- First inventory pass of concurrency-sensitive code paths has been completed.
- Proposed I2C wrapper direction has been agreed with the user.
- Reusable I2C modules have now been moved onto the shared `task_safe_wire` wrapper, pending successful build verification.
- Follow-up review identified and remediated a separate `src/sensors/usensor.cpp` task-safety issue around unsynchronized shared state between a task, an ISR, and registration logic.
- The user paused the remaining verification/remediation steps for this task and asked for them to be moved to `BACKLOG.md` for later follow-up.
- The user selected mapping as the next task, with immediate focus on map representation and planning rather than sensor fusion implementation.
- Implemented a first mapping subsystem with a reusable grid map, a programmed-map definition format, a paired programmed or observed map bundle, a sample map, and a focused `maptest` PlatformIO environment.
- Prepared `docs/MAPPING_ARCHITECTURE.md` to present the mapping architecture and explain the code structure.
- Updated the map model so packed position exchange uses one `int` containing four signed bytes for `x`, `y`, `direction`, and `speed`, with `-128` reserved as unknown.
- Reduced the active PlatformIO environment set by removing stale or merged-residue build targets such as `light`, `logger`, `compass`, and `real_main`.
- Recorded that `src/actuators/light.cpp` was deleted because it does not match the active truck hardware path.
- Confirmed that after pruning the stale environments, the remaining active build failures are dominated by the intermittent archive rename permission issue plus a defined set of code warnings in active modules.
- Cleaned the active build set, stabilized sandboxed PlatformIO builds with the archive-retry workaround, confined direct `Wire` usage to `src/task_safe_wire.cpp`, and cleaned the active warning reports.
- Created `docs/VARIABLE_MODEL.md` as the first standalone shared-variable reference. It now documents the current variables in table form with stable documentation IDs, meaning, source, unit intent, encoding notes, reserved or deferred status, and an initial illustration prompt.
- Updated `include/variables/setget.h` comments so the enum no longer claims stale units as if they were settled facts, and added a maintenance rule in `AGENTS.md` requiring future variable changes to update `docs/VARIABLE_MODEL.md` in the same work item.
- Normalized the MPU6050 temperature path so `rawTemp` now has an explicit datasheet-based `degC10` meaning in code rather than an undocumented approximate formula, and cleaned the `z_main_accsensor*` debug output so it no longer prints misleading derived units for the still-ambiguous gyro and `calc*` values.
- Normalized the MPU6050 gyro path so `rawGyX`, `rawGyY`, and `rawGyZ` are now stored consistently as `deg/s * 10`, and `calcHeading` is now stored as `deg * 10`. The remaining MPU6050 ambiguity is the inherited hardcoded gyro Z bias correction, not the unit itself.
- Replaced the inherited hardcoded MPU6050 gyro Z bias correction with a startup `zeroGz` calibration average in both accelerometer paths, keeping the shared variable names stable while removing one of the largest remaining 6050 heuristics.

## Task Plan
1. Inventory the current `setget` variables and capture what each one is supposed to mean today, including unit, sign convention, and whether it is configuration-dependent.
2. Formalize the variable taxonomy so the naming tiers such as `raw`, `calc`, and future `fuse` variables are explicit and consistently defined.
3. Add stable documentation IDs for the shared variables so they can be referenced independently of later renaming.
4. Identify variables whose units or meaning are currently uncertain because of sensor configuration, scaling, or historical drift, and mark those for correction rather than silently trusting old comments.
5. Create a standalone variable reference document under `docs/` that lists the shared variables, their meaning, units, source, IDs, and how they are encoded in the current 32-bit `long` transport model.
6. Define the first fusion-variable concept set, including examples such as `fuseForwardClear`, and outline the likely fusion package and task responsibilities.
7. Produce a reusable prompt for generating a clear visual illustration of the truck data structures and variable flows.

## Definition Of Done
- The shared variable model has a clear naming and unit convention that another programmer or agent can follow without guessing.
- The shared variable model has stable documentation IDs with reserved ranges for current and future variable classes.
- Existing `setget` variables have documented meaning and unit intent, with uncertainties called out explicitly where sensor configuration still affects interpretation.
- A standalone document under `docs/` exists with the current shared variable definitions, units, source, and encoding notes.
- The future `fuse*` layer has an agreed conceptual place in the architecture.
- A reusable illustration prompt exists for visualizing the truck data structures and variable flows.

## Immediate Next Actions
- Review whether the current MPU6050 path needs a simple gyro filter or averaging step in addition to the new startup bias calibration, because drift and noise are now the main remaining issues.
- Tighten any remaining variable names or comments whose current wording still implies stronger unit certainty than the code really provides.
- Prepare the first concrete fusion-package stub plan based on the documented fast-clearance and slower pose-fusion split.
