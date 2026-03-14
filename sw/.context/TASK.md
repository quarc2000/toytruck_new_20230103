# Task

## Active Task
Clean up the active firmware codebase and PlatformIO environment set so new development proceeds on a smaller, current, and defensible baseline instead of on top of merged residue, stale build targets, and warning-heavy code.

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

## Task Plan
1. Keep the active PlatformIO environments limited to current, intentional truck firmware paths.
2. Remove or isolate merged residue and stale source paths from active builds without deleting potentially reusable code unless the user confirms removal.
3. Clean the current warning clusters in active code paths, starting with `setget.cpp`, `accsensor.cpp`, `accsensorkalman.cpp`, `usensor.cpp`, `GY271.cpp`, and `z_main_accsensorcalibration.cpp`.
4. Rebuild the active environments after each cleanup increment to separate real code issues from the intermittent archive rename sandbox fault.
5. Defer non-current subsystems such as logger redesign and mapping expansion to `BACKLOG.md` until the active codebase is cleaner.

## Definition Of Done
- The active build set reflects current truck code paths only.
- The main visible warnings in active code have been reduced or documented with intent.
- The repository is in a cleaner state for continued feature work without relying on stale merged code paths.

## Immediate Next Actions
- Update planning context so cleanup is the active task and mapping is explicitly paused.
- Keep the paused task-safety validation work in `BACKLOG.md` unless the user reactivates it.
- Use the latest post-prune build sweep as the baseline for the next cleanup pass.
- Start fixing the active warning clusters one file group at a time.
