# Task

## Active Task
Analyse the code for task safety and make sure the codebase moves toward a fully task-safe model, with immediate focus on shared-resource access and especially `Wire` / I2C usage.

## Completed In This Task
- Previous board-analysis task completed with initial board resource files for the TOY controller board and IO expander.
- Known task-safety concern around direct `Wire` usage has now been elevated into the active engineering task.
- Conflict `CON-0001` has been opened and explicitly confirmed by the user as active remediation work.
- First inventory pass of concurrency-sensitive code paths has been completed.
- Proposed I2C wrapper direction has been agreed with the user.
- Reusable I2C modules have now been moved onto the shared `task_safe_wire` wrapper, pending successful build verification.
- Follow-up review identified a separate active conflict in `src/sensors/usensor.cpp` around unsynchronized shared state between a task, an ISR, and registration logic.

## Task Plan
1. Inventory all shared-resource access patterns that may be unsafe across tasks.
2. Classify the findings by type: I2C / `Wire`, global state, static state, driver initialization, and cross-task function reentrancy.
3. Define the project-approved task-safe pattern for I2C access and document the intended wrapper or serialization model.
4. Implement a shared I2C wrapper built around `task_safe_wire_begin`, `task_safe_wire_write`, `task_safe_wire_restart`, `task_safe_wire_request_from`, `task_safe_wire_read`, and `task_safe_wire_end`.
5. Refactor direct `Wire` call sites toward the approved pattern, starting with reusable modules before test-only entry points.
6. Re-check the codebase for remaining unsafe access paths and update `CONFLICTS.md`, `BACKLOG.md`, and architecture notes accordingly.
7. Resolve or explicitly accept the `usensor.cpp` concurrency model before further task-safety remediation continues.

## Definition Of Done
- No remaining unprotected concurrent access path exists to shared resources in the active codebase.
- Shared resources include at least the I2C bus, shared mutable state, shared static or file-scope state used across tasks, and any driver or service API callable from multiple tasks.
- Reusable runtime modules must follow the approved task-safe access pattern.
- Any remaining exceptions must either be removed, proven non-concurrent, or explicitly tracked in governance files.

## Immediate Next Actions
- Confirm how you want to handle `CON-0002` in `src/sensors/usensor.cpp`: remediate now, explicitly accept/defer it, or narrow the active task scope.
- Once `CON-0002` is resolved, rerun focused PlatformIO builds for `accsensor`, `accsensorkalman`, `compass`, `gy271`, and `expander`.
