# Plan

## Short Term
- Inventory every current `setget` variable with its apparent source, meaning, unit, and uncertainty.
- Define and document a stronger naming convention for variable classes such as `raw*`, `calc*`, and future `fuse*`.
- Define and document a stable variable-ID scheme with reserved numeric ranges for calibration, raw, calc, fuse, map, and driver or actuator values.
- Identify where current comments or units appear inconsistent with actual sensor configuration or scaling in code.
- Write a standalone `docs/` reference for the current shared variables, including meaning, units, source, ID, and 32-bit encoding notes.
- Draft the first fusion-variable set and the likely package/task boundaries that should own those outputs.
- Produce a high-quality illustration prompt for the truck data structures and variable flows.

## Medium Term
- Update the actual variable declarations, comments, and supporting docs so they match the formal model.
 - Normalize the remaining ambiguous current semantics in dependency order: gyro-Z bias handling, `calcSpeed`, then `calcDistance`.
- Introduce the first fusion package or task structure once the variable taxonomy is agreed.
- Restore mapping work on top of the clarified variable model instead of the older ad hoc shared-state meanings.

## Definition Of Done
- Variable naming classes, units, and meanings are documented clearly enough that another programmer or agent can use them correctly.
- Variable documentation IDs and series ranges are defined clearly enough that future additions have an obvious home.
- Existing variable comments and declarations no longer rely on hidden assumptions about sensor configuration.
- A `docs/` variable reference exists and is strong enough to become a maintained source of truth for future variable changes.
- The conceptual place for `fuse*` variables and their producer tasks is documented.
- A reusable illustration prompt exists for communicating the truck data structures visually.

## Longer Term
- Resume mapping work from the existing foundation once cleanup is no longer the limiting factor.
- Revisit logger redesign and other deferred support subsystems once the active firmware baseline is stable.
