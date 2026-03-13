# IO Expander Board

## Status
Initial board note derived from current source code and expander test code. This is functional documentation from firmware, not yet verified schematic-level documentation.

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
- `EXPANDER` class is constructed with:
  - `switchAddr`: TCA9548 address
  - `gpioAddr`: MCP23017 address
- The expander code includes a local semaphore intended to protect I2C access inside that class.
- The current implementation is not yet the project-wide approved task-safe I2C solution.

## Channel / Function Notes

### TCA9548
- Default channel selection is channel `0`.
- The code supports selecting channels `0` through `7`.
- `pushChannel()` and `popChannel()` are intended to support temporary channel switching.

### MCP23017
- `initGPIO()` configures:
  - GPA0 as output for MOSFET 1 control
  - GPA1 as output for MOSFET 2 control
  - GPA7 as output for LED control
  - GPIO pins `8` through `14` are also configured in initialization

Current interpretation from user input:
- The two MOSFET-controlled outputs are currently used for brake lights and main lights.
- The second bank of eight GPIOs is intended for direct LED drive outputs.

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

## Relevant Code Sources
- `include/expander.h`
- `src/expander.cpp`
- `src/z_main_expander.cpp`
- `src/z_main_23017_test.cpp`

## Open Questions
- Which IO expander board functions are present in the production hardware versus only in lab test setups.
- Whether the PCA9685 is always present on the IO expander board or only on certain variants.
- The exact per-pin mapping of the direct-drive LED outputs on the second GPIO bank.
- Which TCA9548 channels are reserved for which downstream peripherals in the intended final architecture.
