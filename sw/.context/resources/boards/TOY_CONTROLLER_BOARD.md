# TOY Controller Board

## Status
Initial board note derived from current source code. This is code-backed documentation, not yet verified hardware documentation.

## Board Role
- Main embedded controller board for the toy truck platform.
- Treated in firmware and PlatformIO as an `esp32dev` style target.
- Hosts local GPIO-connected actuators and sensors, plus an I2C bus for onboard and expansion devices.

## Implementation Variants
The controller board supports several higher-level vehicle implementations on top of the same base hardware.

### Original Baseline
- One drive motor channel for propulsion
- One motor channel for steering
- Local light output on GPIO32

### Later Variant With IO Expander
- Lights are moved to the IO expander instead of using the local light output
- GPIO32 is then reused for PWM-based servo steering
- Replacing steering motor control with servo steering frees one motor driver channel
- The freed motor driver channel allows separate left and right drive control

### Current Usage
- Separate left and right drive capability exists in the code and hardware mapping
- Current implementation still uses simultaneous left and right drive in practice
- This layout is intended to enable future steering improvement through differential drive assistance

## MCU / Platform
- ESP32-based board.
- PlatformIO board abstraction: `esp32dev`.
- Arduino framework is used throughout the project.

## Configuration Control
The board behavior is controlled in two different layers.

### Build-Time Selection
- `platformio.ini` selects which firmware entry point or test program is built.
- This is done through PlatformIO environments and `build_src_filter`.

### Runtime Vehicle Configuration
- `src/config.cpp` selects the active truck configuration at runtime based on `ESP.getEfuseMac()`.
- `include/config.h` defines the main configuration enums used by the board-level logic.

Current code-defined vehicle mappings:
- `0xE0DE4C08B764` -> `PAT03` -> `SINGLE` drive + `SERVO` steering
- `0xCC328A0A8AB4` -> `PAT04` -> `DIFFERENTIAL` drive + `SERVO` steering
- `0xB4328A0A8AB4` -> `PER01` -> `SINGLE` drive + `MOTOR` steering
- `0xFC318A0A8AB4` -> `PER02` -> `SINGLE` drive + `MOTOR` steering

Interpretation:
- `platformio.ini` determines which program image is built.
- `config.cpp` determines how the controller board behaves on a specific truck at runtime.

## Onboard I2C Bus
- SDA: GPIO21
- SCL: GPIO22

### Devices Seen On The Controller Board I2C Bus
- Accelerometer / gyro: `MPU6050` at `0x68`
- Magnetometer: code evidence is mixed
  - `Compass` implementation uses `Adafruit_HMC5883_Unified`
  - `GY271` implementation uses default address `0x0D`, consistent with a QMC5883L-style module

Current interpretation:
- The controller board definitely has an onboard accelerometer and an onboard magnetometer path in the code.
- The accelerometer identity is clear from code.
- The exact magnetometer hardware currently needs confirmation from the user or from hardware references.
- The magnetometer was not made to work on the truck and is not currently used in the active truck setup.

## GPIO Mapping Inferred From Code

### Propulsion Motor Channel 1
- Enable: GPIO2
- Forward: GPIO4
- Reverse: GPIO5

### Steering Motor
- Enable: GPIO33
- Right: GPIO27
- Left: GPIO23

### Ultrasonic Sensors
- Front trigger: GPIO16
- Front echo: GPIO34
- Right trigger: GPIO17
- Right echo: GPIO35
- Left trigger: GPIO26
- Left echo: GPIO25
- Rear trigger: GPIO19
- Rear echo: GPIO18

### Shared / Mode-Dependent Pin
- GPIO32 is used as `LIGHT_PIN` in `include/atmio.h`
- GPIO32 is also used as `SERVO_Pin` in `src/actuators/steer.cpp`

Interpretation:
- Light output and servo steering are configuration-dependent on this platform.
- In the IO-expander-based variant, lights move off-board and GPIO32 is reused for servo PWM.

### Additional Mode-Dependent Pins
- GPIO33, GPIO27, and GPIO23 are used for steering motor control.
- The same three GPIOs are also used as the second motor channel in differential-drive mode.

Interpretation:
- These pins are not universal fixed assignments across all vehicle configurations.
- The board supports mutually exclusive hardware or firmware modes.
- In the servo-steering variant, these pins can instead be used as the second drive motor channel for left/right separated propulsion.

## Relevant Code Sources
- `include/atmio.h`
- `src/actuators/motor.cpp`
- `src/actuators/steer.cpp`
- `src/sensors/accsensor.cpp`
- `src/sensors/compass.cpp`
- `src/sensors/GY271.cpp`

## Open Questions
- Which magnetometer hardware is actually mounted on the current controller board.
- Whether the current truck hardware still has the magnetometer mounted and connected in the same way as the code assumes.
- Which truck variants currently use steering motor versus servo steering in active deployment.
- Whether differential left/right drive should be documented as "implemented but currently used symmetrically" for the current main truck.
