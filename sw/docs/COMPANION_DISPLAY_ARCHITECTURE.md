# Companion Display Architecture

## Purpose

This document defines the first architecture baseline for a companion display device that connects to the toy truck over Wi-Fi and shows live runtime information on an ESP32 LCD board.

The display is not part of the truck controller firmware itself. It is a separate client device.

For the current truck-facing endpoint contract, use `docs/COMPANION_DISPLAY_API.md` as the implementation-level source of truth.

## Hardware Family

Current intended baseline:

- AZDelivery ESP32-LCD-4.3

Related board family note:

- a 7-inch compatible board also exists
- current understanding is that it is largely compatible with the 4.3-inch board
- the main currently known hardware delta is an additional UART on the 7-inch board

The first implementation should target the 4.3-inch board and keep the 7-inch board as a closely related future variant rather than trying to generalize everything immediately.

## Reused Proven Board Facts

These facts come from the sibling project and should be treated as the current board baseline:

- display resolution: `800 x 480`
- touch controller: `GT911`
- touch I2C pins:
  - SDA `8`
  - SCL `9`
- control expander: `CH422G`
- known CH422G roles include:
  - touch reset
  - LCD backlight
  - LCD reset
  - SD chip-select
- known-good SD path:
  - SDSPI via ESP-IDF
  - MOSI `11`
  - CLK `12`
  - MISO `13`

## Relationship To The Truck

The truck remains the publisher.

The companion display acts as:

- Wi-Fi client
- status viewer
- map viewer
- later SD logger

The truck should not need to know about the display beyond continuing to expose its normal debug or telemetry interface.

## First Data Contract

The first client implementation should consume the truck outputs that already exist rather than inventing a new protocol immediately.

Current useful endpoints from `env:exploremap`:

- `/status`
  - JSON status summary
- `/map`
  - JSON observed-map payload
- `/control`
  - JSON control ownership and watchdog state
- `/logs`
  - plain-text log stream or snapshot

This is enough for a first companion client.

## UI Direction

Preferred first UI framework:

- `LVGL`

Reason:

- multiple screens are needed
- swipe navigation is desirable
- map rendering is easier with a retained UI framework than with ad hoc drawing only
- the board family already has a vendor LVGL porting reference

This does not mean the project should adopt a large or decorative UI.
The first UI should stay minimal and operational.

## First Screen Model

The first companion display should have a small number of screens:

1. Status
- truck connection state
- active runtime
- main numeric variables
- heading, speed command, turn command, frontier status

2. Map
- observed grid
- robot position
- simple state summary

3. Diagnostics
- selected logs
- Wi-Fi status
- SD status

4. Control
- manual remote-control page
- later mission dispatch or mission monitoring page

Swipe navigation between these screens is preferred.

## Rendering Strategy

The first map rendering should be intentionally simple:

- one cell maps to one small rectangle
- unknown, free, visited, blocked, and robot cells get distinct colors
- no anti-aliased shapes or expensive effects

The main goal is readability and low implementation risk.

## SD Logging Direction

The SD-card path should reuse the sibling project's proven setup directly where practical:

- SDSPI FAT mount
- CH422G-controlled SD chip-select handling
- simple smoke-test operations:
  - create
  - append
  - delete

The first logging scope should stay small:

- connection events
- status snapshots
- optional `/status` samples

The map itself does not need a rich binary storage format in the first increment.

The current hardware note is favorable:

- a `128 MB` SD card is already present in the board

So the SD path should be treated as immediately usable test hardware, not a hypothetical future option.

## Networking Direction

The first client connection model should be simple:

- the truck exposes its AP
- the display board joins as a station
- the display fetches `/status`, `/map`, and later `/control` as needed with HTTP

The first refresh rates can stay conservative:

- status about `2 Hz`
- map about `1 Hz`

This is enough for observability without overloading the display task or the truck web server.

The first remote-control layer now exists on the truck side, but the display client should still earn that complexity after the passive observe/render path is working reliably.

## Separation Rule

Truck-side runtimes such as `frontavoid` and `exploremap` should remain the source of runtime truth.

The companion display must not become a hidden control dependency for the truck.

So the display client should:

- observe
- render
- log

and only later, if explicitly requested, grow into an input or command device.

## First Implementation Recommendation

The first practical code increment should be:

- a separate board-specific display client scaffold
- with Wi-Fi client startup
- HTTP fetch from `/status`
- HTTP fetch from `/map`
- later HTTP fetch from `/control`
- a minimal LVGL screen shell
- SD mount and smoke-test path

That is enough to validate the architecture before more ambitious UI work.
