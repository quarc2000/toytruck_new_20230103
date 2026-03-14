# Plan

## Short Term
- Keep only current truck firmware environments active in `platformio.ini`.
- Remove or isolate stale merged code paths from active builds without over-deleting still-useful code.
- Clean the visible warning clusters in active modules: deprecated critical-section macros, unused locals, possible uninitialized pin use, `PI` redefinition, and ambiguous expressions.
- Rebuild the active environment set after each cleanup increment and update the findings.

## Medium Term
- Decide whether dormant merged code such as telemetry/logger should be redesigned, archived, or removed.
- Revisit deferred task-safety follow-up once the active codebase is cleaner and easier to validate.
- Restore feature work such as mapping only after the baseline active code paths are stable enough to build and reason about.

## Definition Of Done
- The active build targets match current project intent.
- Cleanup findings are tracked, acted on, and revalidated with repeatable builds.
- New feature work can continue on a cleaner baseline with less stale merge residue.

## Longer Term
- Resume mapping work from the existing foundation once cleanup is no longer the limiting factor.
- Revisit logger redesign and other deferred support subsystems once the active firmware baseline is stable.
