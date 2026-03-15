# Chip Notes

This directory is the human- and agent-readable knowledge cache for chips used in this project.

## Purpose

Use this directory to capture durable findings extracted from datasheets, experiments, or verified code review so the same PDF does not need to be decoded repeatedly.

This directory is intentionally different from `datasheets/`:

- `datasheets/` stores the original source documents and source URLs
- `chip_notes/` stores distilled project knowledge in Markdown

This directory is also intentionally different from project-specific runtime docs such as `docs/VARIABLE_MODEL.md`:

- `chip_notes/` should stay centered on the chip itself
- project variable names, shared-state mappings, and task-level data contracts belong in the project docs, not here

## What Belongs Here

Keep notes concise, practical, and grounded in the project.

Good contents:

- I2C addresses actually used in this project
- register meanings the code depends on
- conversion formulas and units
- timing or sequencing constraints
- startup or reset behavior
- project-specific quirks or decisions
- facts verified from the datasheet
- facts verified from bench testing

Avoid:

- copying large sections of the datasheet
- speculative claims without labeling them clearly
- project-irrelevant marketing or generic feature lists
- project variable inventories that belong in the variable model or architecture docs

## Format

Each chip should get its own Markdown file when there is enough reusable information to justify it.

Suggested filenames:

- `ESP32.md`
- `MPU6050.md`
- `MCP23017.md`
- `TCA9548A.md`
- `PCA9685.md`

Suggested structure:

```md
# CHIP_NAME

## Summary

Short project-specific description.

## Datasheet

- Local file:
- Original source:

## Facts We Rely On

- ...

## Project Usage

- ...

## Known Quirks

- ...

## Open Questions

- ...
```

## Writing Rule

These notes must stay useful to both humans and agents:

- write in Markdown
- prefer short sections and flat bullet lists
- explain why a fact matters to this project
- distinguish verified facts from inference
- update the note when a datasheet finding changes code, comments, docs, or decisions

## Current Notes

- [MPU6050.md](./MPU6050.md) - current project usage, variable semantics, and known quirks for the accelerometer or gyro path.
- [PCA9685.md](./PCA9685.md) - PWM-controller bring-up notes covering sleep and restart behavior, prescale sequencing, auto-increment requirements, and practical `50 Hz` servo setup facts.
- [VL53L5CX.md](./VL53L5CX.md) - first-bring-up notes for the multizone ToF sensor, including address conventions, `LPn`, ULD-heavy initialization, and a practical minimal first-read strategy.
