# Controller Board

## Status
Initial placeholder for the bespoke controller board designed by Par.

## Current Known Facts
- The board is custom hardware used in the toy-truck platform.
- For PlatformIO and firmware targeting, it is treated as an ESP32 DevKit v4 style board (`esp32dev`).
- The board is intended to host parallel firmware activities, so task-safe access to shared resources is mandatory.

## Pin Mapping Inferred From Code
These mappings are inferred from current source files and should be treated as "code truth for now", not yet verified hardware documentation.

### Base GPIO Assignments
- Motor channel 1 enable: GPIO2
- Motor channel 1 forward: GPIO4
- Motor channel 1 reverse: GPIO5
- Steering motor enable: GPIO33
- Steering motor right: GPIO27
- Steering motor left: GPIO23
- Ultrasonic front trigger: GPIO16
- Ultrasonic front echo: GPIO34
- Ultrasonic right trigger: GPIO17
- Ultrasonic right echo: GPIO35
- Ultrasonic left trigger: GPIO26
- Ultrasonic left echo: GPIO25
- Ultrasonic rear trigger: GPIO19
- Ultrasonic rear echo: GPIO18
- Light or steering-servo pin: GPIO32
- I2C SDA: GPIO21
- I2C SCL: GPIO22

### Notes On Shared / Mode-Dependent Pins
- GPIO33, GPIO27, and GPIO23 are used as steering-motor pins in `src/actuators/steer.cpp`.
- The same GPIO33, GPIO27, and GPIO23 are also used as motor channel 2 pins for differential drive in `src/actuators/motor.cpp`.
- This implies those pins are mode-dependent and cannot serve both steering-motor control and differential-drive motor channel 2 at the same time on one hardware configuration.
- GPIO32 is used as `LIGHT_PIN` in `include/atmio.h` and also as `SERVO_Pin` in `src/actuators/steer.cpp`, which suggests that light output and servo steering are also mutually exclusive or configuration-dependent on current hardware.

### I2C Devices Seen In Code
- Main I2C bus pins are SDA GPIO21 and SCL GPIO22.
- MPU6050 appears at address `0x68`.
- Magnetometer support includes a sensor at address `0x0D` in one implementation.
- TCA9548 I2C switch/multiplexer appears at address `0x70`.
- MCP23017 GPIO expander appears at address `0x20`.
- PCA9685 PWM driver appears at address `0x40` in test code.
- VL53L0X appears at address `0x29` in test code.

### Task-Safety Observation
- The repository contains direct `Wire` calls in multiple modules, including sensor code and test entry points.
- There is a local semaphore inside `EXPANDER`, but there is not yet a project-wide shared I2C wrapper protecting all bus access.
- Because task safety is a hard requirement, this should be treated as an active architectural gap until a project-approved approach is defined.

## To Document
- ESP32 variant and module details.
- Board revision history.
- Verified pin mapping for motors, steering, lights, sensors, expansion connectors, and debug interfaces.
- I2C topology, including the expander and any switch/multiplexer devices.
- Power domains and any constraints relevant to sensors, servo drive, and motor drive.
- Boot, flashing, and recovery notes.

## Constraints
- Do not treat undocumented assumptions here as hardware truth.
- If board facts are uncertain, record them as open questions instead of filling gaps from memory.
