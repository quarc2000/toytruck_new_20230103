# Decisions

## Decision Register

| decision_id | date | user | context | decision | reason | related_conflict | related_backlog | related_files |
|---|---|---|---|---|---|---|---|---|

---

## Agent Instructions

This section is guidance for agents and maintainers on how to maintain the decision register.

### When To Log A Decision
Add a new entry when:
- A conflict requires user resolution.
- A non-compliance or deviation is explicitly accepted.
- A significant project architecture, behavior, implementation, or workflow choice is made.
- The agent needs direction before proceeding safely.

Do not log:
- Context architecture or documentation housekeeping decisions.
- Simple bookkeeping moves between context files.
- Backlog deferrals by themselves, unless the user also made a significant project decision that should be preserved.

### Entry Rules
- The table must stay at the front of the file.
- Decision IDs must be chronological and sequential using the format `DEC-0001`, `DEC-0002`, and so on.
- New decisions must be appended as new rows at the end of the table.
- Existing decision IDs must not be renumbered after creation.

### Column Meaning
- `decision_id`: Chronological decision identifier used for reference.
- `date`: Date the decision was made.
- `user`: Person who made or approved the decision.
- `context`: Short description of the situation requiring the decision.
- `decision`: The actual decision that was made.
- `reason`: Why that decision was made.
- `related_conflict`: Link or identifier for a related conflict, if any.
- `related_backlog`: Related deferred work item, if any, when the decision caused or clarified it.
- `related_files`: Files materially affected by the decision.
