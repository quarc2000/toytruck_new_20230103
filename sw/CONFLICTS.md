# Conflicts

## Conflict Register

| conflict_id | rule_reference | source_document | type | control_outcome | status | description | decision | owner | decision_date | evidence_or_justification | linked_backlog_item | review_or_expiry_date |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| CON-0001 | Shared state and concurrent function access must be task safe | `.context/GUARDRAILS.md` | architecture_conflict | FAIL | Open | Multiple modules perform direct `Wire` I2C access without a project-wide approved synchronization or serialization mechanism, so concurrent task-safe behavior is not assured. | User confirmed this is the active remediation task. Treat as work in progress under the current task until a project-wide approach is implemented and verified. | patbwm | 2026-03-13 | Direct `Wire` use was previously identified in sensor modules and test paths, while only the `EXPANDER` class has a local semaphore. This indicates a cross-module concurrency gap rather than one isolated driver issue. | Define and implement a project-wide task-safe I2C access pattern |  |
| CON-0002 | Shared state and concurrent function access must be task safe | `.context/GUARDRAILS.md` | architecture_conflict | FAIL | Open | `src/sensors/usensor.cpp` uses shared file-scope state across the trigger task, the echo ISR, and sensor registration without an approved synchronization or serialization model, so task-safe behavior is not assured. | Pending user guidance. Guardrails require execution to stop until the concurrency model for this module is explicitly accepted or remediated. | patbwm | 2026-03-13 | `sensor[]`, `num_sensors`, `current_sensor`, `pulse_active`, and `startTime` are shared across `TriggerTask`, `echoInterrupt`, and `Usensor::open()`. The task is started in the constructor before registration finishes, and the ISR/task pair both access mutable module state directly. | Review and remediate `usensor.cpp` shared-state model |  |

---

## Agent Instructions

This file is the conflict register for the repository. It exists to make conflicts visible, reviewable, and deliberately resolved when using LLMs or autonomous agents in development workflows.

### Location
`CONFLICTS.md` must be located in the repository root, not under `.context`.

Reason:
- Conflicts represent governance issues.
- They must be immediately visible to maintainers and reviewers.
- They often involve external policies or cross-repository standards.

### When This File Must Be Updated
The agent must create a new entry in this file when any of the following occur:
- A rule in `.context/GUARDRAILS.md` would be violated.
- A decision recorded in `DECISIONS.md` would be contradicted.
- Two instructions conflict.
- A central policy conflicts with a local rule.
- An architectural constraint would be violated.
- The request exceeds the defined task scope.
- Required information is missing and prevents safe execution.

### Required Behavior On Conflict
When a conflict is detected the agent must:
1. Append a new entry to `CONFLICTS.md`.
2. Stop execution.
3. Request a decision from the user or responsible owner.

Work must not continue until the conflict has been resolved or explicitly accepted.

### Conflict Types
The `type` field must use one of the following values and must not change after creation.

| type | meaning |
|---|---|
| `conflicting_instructions` | Two instructions contradict each other. |
| `rule_violation` | A rule in `.context/GUARDRAILS.md` would be broken. |
| `decision_violation` | A decision recorded in `DECISIONS.md` would be contradicted. |
| `policy_conflict` | Central and local policies disagree. |
| `architecture_conflict` | Architectural constraints would be violated. |
| `scope_conflict` | Request exceeds the defined task scope. |
| `missing_information` | Required information is unavailable. |

### Conflict Status Lifecycle
Each conflict entry must include a `status` field.

Allowed values:

| status | meaning |
|---|---|
| `Open` | Conflict detected and awaiting user decision. |
| `Accepted` | Deviation explicitly accepted. |
| `Backlogged` | Remediation planned but deferred. |
| `Resolved` | Issue corrected and no longer active. |
| `Dismissed` | Conflict determined to be invalid. |

Typical lifecycle:
- Conflict detected.
- Entry added to `CONFLICTS.md`.
- User decision.
- Status updated.

Possible outcomes:
- `Resolved` when the issue is fixed.
- `Accepted` when a deviation is explicitly accepted.
- `Backlogged` when remediation is deferred.
- `Dismissed` when the conflict was based on an incorrect interpretation.

### Table Field Meaning
- `conflict_id`: Unique identifier.
- `rule_reference`: Rule, policy, or decision affected.
- `source_document`: Where the rule originated.
- `type`: Conflict classification.
- `control_outcome`: `PASS`, `FAIL`, or `NOT_APPLICABLE`.
- `status`: Lifecycle state.
- `description`: Explanation of the issue.
- `decision`: User or owner resolution.
- `owner`: Responsible person.
- `decision_date`: Date of resolution or acceptance.
- `evidence_or_justification`: Supporting information.
- `linked_backlog_item`: Related remediation item in `BACKLOG.md`, if any.
- `review_or_expiry_date`: Optional follow-up or review date.

Entries should be appended rather than rewritten, except when updating status and resolution fields.

### Relationship To Other Governance Files

| file | relationship |
|---|---|
| `.context/GUARDRAILS.md` | Rule violations originate here. |
| `DECISIONS.md` | Decision violations originate here. |
| `.context/ARCHITECTURE.md` | Architecture conflicts originate here. |
| `BACKLOG.md` | Remediation work may be deferred here. |
| `RISKS.md` | Accepted deviations may generate risk entries. |
| `.context/PROJECT_HISTORY.md` | Major conflict resolutions should be recorded here. |

Typical governance loop:
- Conflict detected.
- Entry added to `CONFLICTS.md`.
- User decision.
- Remediation, acceptance, or backlog.
- Status updated.

### Why This File Exists
Without a dedicated conflict register, LLM-assisted workflows can:
- Silently ignore rules.
- Work around guardrails.
- Lose traceability of deviations.
- Repeatedly trigger the same governance problems.

`CONFLICTS.md` is therefore a key mechanism for maintaining safety, compliance, and traceability.
