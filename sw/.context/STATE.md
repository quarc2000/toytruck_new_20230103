# State

## Current Task Memory
- No active task.
- The shared-variable taxonomy update is complete in the docs.
- The main retained architectural point is that the semantic tiers now use:
  - `1000` raw
  - `2000` cleaned
  - `3000` calculated
  - `4000` fused
  - `7000` map
  - `8000` driver
  - `9000` config or system
- Current code symbols such as `calcHeading` and `fuseForwardClear` were intentionally not renamed in this pass.
