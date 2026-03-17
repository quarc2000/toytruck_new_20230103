# Shared Knowledge Root

This directory is the configured MCP resource root for local development.

## Conventions

- Keep content read-only from MCP clients.
- Include a `README.md` in each subdirectory.
- Use short, stable file names for discoverability.
- Prefer Markdown for agent-consumable references.

## Example domains

- `central_requirements/` (typically mandatory requirements)
- `Scania policies`       (for more infromation in mandatory requirements)
- `boards/esp32-s3-lcd-4.3/`

## Project Resources

- `CONTROLLER_BOARD.md` for the bespoke truck controller board documentation.
- `AGENTS_AND_PLATFORMIO.md` for the repo-local PlatformIO setup, sandbox build workaround, and the expected build workflow for future agents.
- `datasheets/README.md` for the current chip datasheet index and direct source URLs for the chips identified from code and board notes.
- `chip_notes/README.md` for distilled per-chip Markdown notes that capture reusable findings from datasheets, experiments, and verified code review.
- `boards/README.md` for board-level and module-level hardware notes such as the IO expander and the Waveshare TOF Laser Range Sensor Mini.
- `truck_models/README.md` for whole-vehicle variant notes such as dimensions, drive layout, installed sensor sets, and model-specific hardware assumptions.
