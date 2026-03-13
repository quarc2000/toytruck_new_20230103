# Plan

## Short Term
- Inventory concurrency-sensitive code paths across the repository.
- Focus first on `Wire` / I2C usage and define the approved project-wide task-safe access model.
- Use a shared bus semaphore with an explicit wrapper API shaped around `task_safe_wire_begin`, `task_safe_wire_write`, `task_safe_wire_restart`, `task_safe_wire_request_from`, `task_safe_wire_read`, and `task_safe_wire_end`.
- Prioritize reusable runtime modules before test-only or bring-up code.

## Medium Term
- Refactor unsafe shared-resource access toward the approved task-safe model.
- Verify that shared state and cross-task callable functions meet the guardrail requirements.
- Document the final task-safety model for shared state access and I2C usage.

## Definition Of Done
- No remaining unprotected concurrent access path exists to shared resources in the active codebase.
- Reusable runtime modules use approved task-safe access patterns for shared resources.
- Any remaining exceptions are either removed, proven non-concurrent, or explicitly tracked.

## Longer Term
- Maintain the context files as lightweight operational documentation for future debugging and feature work.
- Capture architectural decisions when consolidating entry points, changing hardware abstractions, or altering telemetry behavior.
