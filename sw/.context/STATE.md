# State

## Current Task Memory
- Active subtask: tune live `env:exploremap` behavior from truck test feedback.
- Latest confirmed truck behavior:
  - reverse and forward steering sign semantics are much better
  - corner escape still fails because the truck loses turn intent too easily and sometimes does not show enough reverse steering authority
- Current hypothesis:
  - steering endpoint units needed to be clarified because the current config values are PWM controller units, not physical angles
  - repeated block events were still able to re-decide the recovery side too easily from noisy side readings
- Current work:
  - keep one recovery-turn decision sticky across repeated block-recover cycles
  - document PAT004 steering endpoint calibration in readable units
- Latest code change now in tree:
  - PAT003 and PAT004 servo range widened from `60..105` to `55..110`
  - PAT004 truck-model note now documents those steering endpoint values explicitly as `50 Hz`, `10-bit` LEDC duty counts
  - committed reverse and close-front forward turns now use full steering authority
  - committed forward turns near a close wall now enforce a minimum turn so heading-hold and wall terms cannot cancel them back toward straight
  - `ExplorerRecoverPause` now keeps the forward escape steering preloaded instead of centering the wheels
  - repeated block events now keep the current committed escape side unless that side becomes impossible or the opposite side becomes decisively more open
- Verification:
  - `pio run -e exploremap -j1` succeeded after the turning-consistency pass
- Upload status:
  - latest turning-consistency build uploaded successfully to `COM7`
- Remaining manual cleanup outside this blocked session:
  - delete stray cache directories `p3`, `p4`, and `p5`
- Companion-display handoff docs intentionally kept:
  - `docs/COMPANION_DISPLAY_API.md`
  - `docs/COMPANION_DISPLAY_ARCHITECTURE.md`
