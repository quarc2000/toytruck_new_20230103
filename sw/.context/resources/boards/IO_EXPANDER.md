# IO Expander Board

## Status
Board note updated from current firmware plus recent live truck testing. This is still firmware-and-bench documentation rather than schematic-level documentation, but several light-path details are now hardware-verified.

## Board Role
- External expansion board connected over the controller board I2C bus.
- Provides GPIO expansion and I2C channel switching.
- Appears intended to support lights and additional external I2C peripherals such as distance sensors and PWM-driven devices.

## I2C Devices Seen In Code
- TCA9548 I2C switch / multiplexer at `0x70`
- MCP23017 GPIO expander at `0x20`
- PCA9685 PWM driver at `0x40` in test code
- VL53L0X distance sensor at `0x29` in test code, accessed through the multiplexer

## Software Model
- `EXPANDER` is constructed with:
  - `switchAddr`: TCA9548 address
  - `gpioAddr`: MCP23017 address
- The current approved access path is through `task_safe_wire`, not the earlier ad hoc expander-local locking idea.
- `src/expander.cpp` now uses task-safe I2C transactions and read-modify-write behavior for MCP23017 registers, so single-pin writes preserve the rest of the port state.

## Channel / Function Notes

### TCA9548
- Default channel selection is channel `0`.
- The code supports selecting channels `0` through `7`.
- `pushChannel()` and `popChannel()` are intended to support temporary channel switching.

### MCP23017
- `initGPIO()` configures:
  - GPA0 as output for MOSFET-controlled main light
  - GPA1 as output for MOSFET-controlled brake light
  - GPA7 as output for LED control
  - GPB0 and GPB1 (logical pins `8` and `9`) as output candidates for left and right indicators
  - GPB3 (logical pin `11`) as output candidate for reverse light
  - remaining higher pins are currently left available as inputs unless explicitly promoted to outputs

Current interpretation from user input and recent live tests:
- logical pin `0` is the main light path
- logical pin `1` is the brake light path
- logical pins `8` and `9` are the indicator outputs
- logical pin `11` is intended as the reverse-light output
- the brake and main light paths drive MOSFETs
- the indicator electrical setup expects drive-high behavior

### PCA9685
- Used in test code as a PWM generator for servo-style pulse output.
- Test code drives channel `0` and channel `15`.
- It is currently not working in the project.
- A missed reset pin was previously found and corrected, but the device still did not work afterward.
- Because it was not needed for the active truck setup, work on it was deferred.

### VL53L0X
- Test code initializes and reads a VL53L0X through TCA9548 channel `1`.
- The purpose of the TCA9548 is to allow multiple VL53L0X sensors with the same fixed I2C address.
- This setup has been tested successfully in the lab.
- The VL53L0X sensors are not yet mounted on the truck.

## Likely Responsibilities
- Lighting control through MCP23017 outputs
- Additional switched outputs such as MOSFET-controlled loads
- Optional PWM generation through PCA9685
- Multiplexed attachment of additional I2C sensors, including time-of-flight distance sensors

## Verified Firmware Findings
- The MCP23017 port-B direction bug that originally left indicator outputs configured as inputs has been corrected in `src/expander.cpp`.
- The MCP23017 write path now preserves unrelated bits on the same port; earlier code overwrote the entire port latch when changing one pin.
- A simple scripted test successfully identified the light outputs well enough to move on to a real light-control task.
- A first `LightService` now drives the board through the expander path:
  - brake from longitudinal deceleration
  - indicators from steering demand
  - reverse light from negative desired speed
- This means the expander board is no longer only a bring-up target; it now has an active runtime role in the truck firmware.

## Relevant Code Sources
- `include/expander.h`
- `src/expander.cpp`
- `src/z_main_expander.cpp`
- `src/z_main_23017_test.cpp`

## Open Questions
- Which IO expander board functions are present in the production hardware versus only in lab test setups.
- Whether the PCA9685 is always present on the IO expander board or only on certain variants.
- Final confirmation of which physical side is pin `8` versus pin `9` for the indicators.
- Which TCA9548 channels are reserved for which downstream peripherals in the intended final architecture.
