# Task

## Active Task
Implement a minimal `basic_telemetry` Wi-Fi debug path with an in-memory logger and a simple web page that shows recent log lines plus selected `setget` values.

## Completed In This Task
- The shared-variable model was formalized with stable IDs, clearer units, a variable reference, and an initial `calc*` versus `fuse*` boundary.
- The MPU6050 paths were normalized and made directly comparable by PlatformIO environment.
- The first live fused variable, `fuseForwardClear`, was implemented and verified.
- The fusion code was refactored into one `fusion` package with helper logic plus one `FusionService`.
- `env:hwtest` builds successfully with the unified fusion service.
- The docs now describe one fusion package with two task cadences.
- `calcHeading` now uses task-local timing plus a small yaw deadband in both MPU6050 envs.
- `calcSpeed` and `calcDistance` are now intentionally held safe at `0`.
- `accsensor` and `accsensorkalman` both compile after the estimator change.
- The updated `accsensor` build was flashed to the truck, and the first retest looked plausible.
- `zeroAy` and `zeroAz` were added so all three accelerometer axes are now centered at startup in both MPU6050 envs.
- Stronger accelerometer damping and slow stationary zero tracking were added, and the updated `accsensor` build was flashed successfully.
- Live testing now shows the plain path heading behaves plausibly enough to move on to forward-motion estimation.
- A first conservative forward speed and distance estimator was implemented in `src/sensors/accsensor.cpp` using thresholding, leakage, stationary reset, and clamping.
- `env:accsensor` compiles successfully with the new estimator.
- The plain debug view and `docs/VARIABLE_MODEL.md` now describe the new plain/Kalman split honestly.
- The later low-motion and reversal experiment was judged worse in live testing and has now been rolled back.
- MPU6050-only speed and distance are being deferred to the backlog as experimental rather than treated as a near-term deliverable.
- The reverted `accsensor` build has been rebuilt and reflashed, so the active work can move on to the expander path.
- `env:expander` compiles successfully and has been flashed to the truck.
- `src/z_main_expander.cpp` now blinks pins `8` and `9` explicitly for indicator discovery.
- `src/expander.cpp` now preserves the rest of the MCP23017 port when writing a single GPIO and configures pins `8` and `9` as outputs.
- `src/z_main_expander.cpp` now also asserts brake light pin `1` during the idle/deceleration phase of the blink cycle.
- `env:expander` was rebuilt and reflashed successfully after adding brake light pin `1` to the test pattern.
- `src/z_main_expander.cpp` now drives dedicated test phases for pin `0` (main light), pin `1` (brake light), pin `8`, and pin `9` so wiring and polarity can be checked directly.
- `env:expander` compiles successfully with the explicit four-phase light test.
- The truck-side light wiring is now known well enough to move from scripted GPIO tests to a real light-control task.
- Added `LightService` as a real expander-backed task for brake, indicator, and reverse lights.
- Updated the expander setup so reverse light pin `11` is configured as an output.
- Wired the light task into `env:hwtest` so it can observe steering, desired speed, and MPU6050 data together.
- `env:hwtest` compiles successfully with the new light-control task.
- `env:hwtest` was flashed successfully to the truck with the integrated light-control task.
- Existing MQTT-oriented telemetry files under `src/telemetry` and `include/telemetry` were intentionally left untouched for later Pi-server work.
- Added `include/basic_telemetry/basic_logger.h` and `src/basic_telemetry/basic_logger.cpp` for RAM-backed runtime logging.
- Added `include/basic_telemetry/basic_web_server.h` and `src/basic_telemetry/basic_web_server.cpp` for the simple Wi-Fi debug server.
- Added `src/z_main_basic_telemetry.cpp` as a safe stationary telemetry test entry point.
- Added `env:basictelemetry` to `platformio.ini`.
- `env:basictelemetry` compiles successfully and produces a working firmware image with a web page, `/logs`, and `/status`.
- The Wi-Fi path now always starts a dedicated debug AP and also attempts station connection using the existing secrets.
- `env:basictelemetry` was flashed successfully to the truck.
- Updated the expander-board resource note with the recent verified lighting and firmware findings.
- A simpler AP-only telemetry variant with fixed SSID `ToyTruckDebug` now compiles successfully and is ready to flash.

## Task Plan
1. Create `basic_telemetry` logger and web-server modules without reusing the existing MQTT telemetry filenames.
2. Build a safe test env that publishes recent log lines and selected `setget` values over Wi-Fi without moving the truck.
3. Verify the new env compiles and flashes successfully.
4. Leave the truck with a working telemetry test image that can be checked over Wi-Fi.

## Definition Of Done
- A `basic_telemetry` logger exists and can collect runtime log lines in RAM.
- A simple web page exposes recent log lines plus selected `setget` values over Wi-Fi.
- The chosen verification env compiles successfully.
- The updated firmware is flashed successfully to the truck.

## Immediate Next Actions
- Close anything holding the truck serial port, flash the fixed-SSID `ToyTruckDebug` telemetry build, and then verify the HTML page, `/logs`, and `/status`.
