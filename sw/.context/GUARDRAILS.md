# Guardrails

## Preflight Gate
- Read [AGENTS.md](/c:/Users/patbwm/Documents/PlatformIO/Projects/toytruck_new_20230103/sw/AGENTS.md) before substantial work.
- Check this `.context` directory plus the root `BACKLOG.md` and `CONFLICTS.md` files for current architecture, state, task, plan, deferred work, and instruction issues.
- If local context conflicts with MCP-served guidance, stop and ask the user which to prioritize.
- Prefer small, reversible changes and verify with the narrowest relevant PlatformIO environment.
- If you are about to update a file that has been thorougly tested and known to work, you have to Stop and ask the user if it is ok to modify that file.
- Always be conservative of the use of memory or computing. We are on an embedded platform with very limited resources. Only use what is needed to achive the task.


## Working Rules
- Treat this project as embedded firmware for ESP32-based toy-truck variants; avoid assumptions that all hardware is present on every build target.
- Preserve the current multi-entry-point workflow in `src/z_main_*.cpp` unless the task explicitly consolidates it.
- Avoid changing secrets, hardware IDs, or pin mappings without explicit user intent.
- When architecture, state-machine, or hardware integration understanding changes, update `.context/ARCHITECTURE.md` and `.context/STATE.md`.
- Append meaningful milestones or discoveries to `.context/PROJECT_HISTORY.md`.
- Add deferred but relevant work items to `BACKLOG.md`.
- All shared state access MUST be task safe. No variable may be read or written from multiple tasks without an approved synchronization or serialization mechanism.
- Functions that may be called from multiple tasks MUST be reentrant or protected by an approved task-safe access mechanism.
- If the agent finds code that appears non-task-safe or non-reentrant where concurrent use is possible, it MUST create an entry in `CONFLICTS.md`, stop, and ask the user for guidance before continuing.

## Safety
- Ask before changing anything under `.context/resources` unless the change is only mirroring MCP-served shared content into a local copy.
- If hardware behavior is uncertain, prefer documenting assumptions in `.context/TASK.md` or `BACKLOG.md` over silently encoding them in code.
