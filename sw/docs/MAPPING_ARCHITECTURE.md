# Mapping Architecture

## Problem

The truck needs a map representation that is small enough to run on ESP32 firmware, but rich enough to support:

- a programmed map with fixed obstacles, a start point, and a destination point
- a live observed map that changes while the truck drives
- later alignment between planned and observed space using estimated pose, speed, direction, and fused sensor data
- future projection from richer sensors such as VL53L5 into the same 2D navigation surface

The map foundation now exists and the first exploration runtime is using it. The current work is still a first operational baseline rather than a finished navigation stack.

## Design Summary

The new code adds a `navigation` subsystem built around four core concepts:

1. `GridMap`
   A reusable 2D occupancy grid with configurable width, height, cell size in millimeters, and world origin.

2. `ProgrammedMapDefinition`
   A compact format for a static map definition that includes geometry, start cell, goal cell, and an immutable cell array.

3. `MapBundle`
   A pair of maps:
   - one programmed map
   - one observed map
   plus the alignment metadata that will later tie them together.

4. `CellFlags`
   A bitmapped cell encoding rather than a single-value enum.

5. `ObservedExplorerService`
   The first runtime that:
   - keeps a live observed map
   - projects current sensor readings into cells
   - marks visited space
   - searches for reachable frontier cells
   - stops when no reachable frontier remains

Position exchange is now based on one packed 32-bit integer:

- byte 0: `x`
- byte 1: `y`
- byte 2: `direction`
- byte 3: `speed`

Each field is a signed byte. `-128` is reserved as `unknown`.

Field meanings are:

- `x`: grid X coordinate in cells, positive east
- `y`: grid Y coordinate in cells, positive north
- `direction`: 5 degree clockwise steps from north
- `speed`: signed cm/s for compact map-state exchange

This packed pose is intentionally compact and shared-state friendly. Finer-grained dead reckoning can continue to use separate variables such as `calcSpeed`.

## Why Bit Flags

A single enum like `unknown`, `free`, `blocked` would not age well. The same cell may later need to carry multiple meanings at once, for example:

- known free space
- blocked space
- programmed-space marker
- observed-space marker
- start or goal markers
- temporary block from a moving or uncertain obstacle
- path-planning annotation

The current bit layout is:

- `CellKnown`
- `CellBlocked`
- `CellVisited`
- `CellPlannedPath`
- `CellStart`
- `CellGoal`
- `CellProgrammed`
- `CellObserved`
- `CellTemporaryBlock`
- `CellLowConfidence`

Unknown space is represented by the absence of `CellKnown`.

## Chosen Data Shape

Each cell is currently:

```cpp
struct MapCell {
    uint16_t flags;
    uint8_t confidence;
    uint8_t reserved;
};
```

That gives a 4-byte cell.

This is a practical compromise:

- small enough for ESP32-scale grids
- explicit enough for occupancy and confidence
- simple enough to serialize and inspect

Example memory cost:

- `50 x 50` cells: about `10 KB`
- `100 x 100` cells: about `40 KB`

Because the code stores `cell_size_mm` in the geometry, the same subsystem can support either `100 mm` cells or `50 mm` cells without redesign.

## Current Operational Grid

The first operational exploration runtime now uses:

- `100 x 100` cells
- `100 mm` cell size
- start in the center cell
- initial heading aligned so the start direction is positive `Y`

Reason:

- `100 mm` is coarse but robust for the current sensing and dead-reckoning uncertainty
- it keeps RAM use acceptable on ESP32
- it is enough to start frontier-based exploration before finer map alignment exists

Finer grids such as `50 mm` remain a later option rather than the current baseline.

## Programmed Versus Observed Map

The architecture deliberately separates:

- `programmed`
  The expected world or mission map loaded ahead of time.

- `observed`
  The runtime map built from sensors.

This matters because the truck should not assume the real world exactly matches the programmed world.

The `MapBundle` object keeps both maps and stores:

- start cell
- goal cell
- estimated pose in programmed-map coordinates
- observed-to-programmed alignment offset and heading

That is the hook for the next phase where motion estimates and sensor fusion improve map registration.

## Packed Position Model

The current position model uses only grid-space values, not millimeter coordinates.

The packing API is:

- `pos2int(...)`
- `int2pos(...)`

and packs:

- `x`
- `y`
- `direction`
- `speed`

into one signed 32-bit integer using four signed bytes.

Unknown is represented by `-128` for any individual field.

Coordinate conventions:

- map X grows east
- map Y grows north
- `0,0` is the map-frame origin, not automatically the start point
- the observed map can use a local origin
- the programmed map can place the route anywhere relative to its own origin

Direction conventions:

- `0` = north
- positive direction is clockwise
- `18` = east
- `36` = south
- `54` = west

This matches the project's requirement that the full position state should fit in one `int` for task-safe sharing through `setget`.

The map still stores `cell_size_mm` per map, so tasks can convert grid deltas into real distance:

- observed map cell size
- programmed map cell size

That is the intended boundary:

- positions are exchanged in grid units
- physical distance is derived from the map's cell size

## Files Added

- `include/navigation/grid_map.h`
- `src/navigation/grid_map.cpp`
- `include/navigation/sample_maps.h`
- `src/navigation/sample_maps.cpp`
- `src/z_main_map.cpp`
- `include/navigation/observed_explorer.h`
- `src/navigation/observed_explorer.cpp`
- `include/basic_telemetry/explore_web_server.h`
- `src/basic_telemetry/explore_web_server.cpp`
- `src/z_main_explore_map.cpp`

## Code Walkthrough

### `grid_map.h` / `grid_map.cpp`

This is the core subsystem.

`GridMap` handles:

- allocation of the cell buffer
- geometry ownership
- cell reads and writes
- cell-size access for distance calculations
- loading a static programmed map definition

`MapBundle` is the bridge between path planning and later sensor fusion.

### `sample_maps.h` / `sample_maps.cpp`

This shows the intended static map format.

The sample map is an `8 x 8` labyrinth-style grid with:

- blocked border
- internal corridor structure
- start at `(1, 1)`
- goal at `(6, 6)`

This file is not meant to be the final map source format for production tooling. It is the first concrete and compilable firmware-side representation.

### `z_main_map.cpp`

This is a focused integration entry point that:

- loads the programmed sample map
- creates the observed map
- injects one observed obstacle
- packs an observed pose into one integer
- prints a summary over serial

It exists to make the new subsystem easier to bring up in isolation.

## First Operational Exploration Runtime

`env:exploremap` is now the first runtime that uses the mapping subsystem directly.

Current behavior:

- starts from the center of a `100 x 100` observed grid
- treats the current heading at boot as "up" or positive `Y`
- projects:
  - front ultrasonic forward
  - left ultrasonic to the left
  - right ultrasonic to the right
  - rear ultrasonic backward
  - front-left and front-right `VL53L0X` forward from lateral offsets that match the front sensor spacing conceptually
- marks free cells along each ray
- marks blocked endpoints when a sensor sees an obstacle inside trusted range
- marks the current robot cell as visited
- searches for the nearest reachable frontier cell
- keeps exploring while reachable frontier exists
- stops when no reachable frontier remains

It also exposes:

- `/status` JSON
- `/map` JSON
- a simple map web page

through the dedicated exploration web server.

## What This Still Does Not Yet Do

The current exploration runtime still does not:

- load or follow a real programmed map
- align the observed map back to a programmed map
- run full path planning through a programmed route
- do robust SLAM-style pose correction
- project VL53L5 data
- use wheel odometry

The current pose correction is only a simple first pass:

- integrate movement from commanded motion
- compare repeated front or rear obstacle readings
- shrink the traveled-distance estimate when the observed obstacle delta proves the commanded travel estimate was too optimistic

## Next Steps

1. Hardware-validate `env:exploremap` on the truck and check map orientation from the center start.
2. Verify that the frontier search actually consumes reachable unexplored space instead of looping locally.
3. Improve pose correction beyond the current repeated-obstacle distance adjustment.
4. Add a real programmed-map loading and alignment path.
5. Add richer projection rules later for denser range sensors such as VL53L5.
