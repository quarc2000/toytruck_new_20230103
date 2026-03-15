# State

## Current Task Memory
- Active task: build a minimal `basic_telemetry` Wi-Fi debug path with RAM-backed logs and a simple web page.
- Relevant current state:
  - the last acceptable `accsensor` build is back on the truck
  - MPU6050-only speed and distance are now deferred to the backlog
  - existing `src/telemetry/*` files are from another team and should not be reused for this first web-debug path
  - the new work should live under `include/basic_telemetry` and `src/basic_telemetry`
  - `src/secrets.cpp` already contains a Wi-Fi SSID and password that can be reused for a first station-mode attempt if needed
  - the first verification env should be safe and stationary, not one that drives the truck
  - `env:basictelemetry` now compiles successfully and serves a simple HTML page plus `/logs` and `/status`
  - the web server always starts a direct debug AP and also attempts STA connection with the existing secrets
  - `env:basictelemetry` is now flashed successfully to the truck
  - a newer AP-only telemetry variant with fixed SSID `ToyTruckDebug` now compiles successfully but is not flashed yet because `COM7` was busy during upload
  - the expander board note under `.context/resources/boards/IO_EXPANDER.md` now reflects the recent verified lighting findings
- Current next step:
  - close whatever holds `COM7`, flash the fixed-SSID `ToyTruckDebug` build, then verify the web page plus log and status endpoints
