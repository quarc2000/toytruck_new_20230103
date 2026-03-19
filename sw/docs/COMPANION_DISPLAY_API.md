# Companion Display API

## Purpose

This document is the handoff-level truck-facing API contract for the companion display client.

It describes the current outputs exposed by the truck runtime that the display should consume today.

## Scope

This contract is currently provided by `env:exploremap`.

It is not yet a project-wide telemetry standard across every runtime. A display agent should therefore treat this as the current `exploremap` contract, not as a promise that every truck firmware mode exposes the same endpoints.

## Network Baseline

Current truck-side Wi-Fi baseline in `env:exploremap`:

- AP SSID: `ToyTruckDebug`
- AP password: `truckdebug`
- truck IP: `192.168.4.1`

Current display bootstrap client baseline:

- join `ToyTruckDebug`
- use base URL `http://192.168.4.1`
- fetch `/status`
- fetch `/map`
- optionally fetch `/control`
- optionally fetch `/logs`

## Endpoint Summary

| Endpoint | Method | Content-Type | Purpose |
|---|---|---|---|
| `/status` | `GET` | `application/json` | Compact live explorer and driver state |
| `/map` | `GET` | `application/json` | Full observed-grid snapshot |
| `/control` | `GET` | `application/json` | Current control ownership and remote-command watchdog state |
| `/control/enable` | `POST` | `application/json` | Enable supervised remote control |
| `/control/drive` | `POST` | `application/json` | Send one remote speed and steering command |
| `/control/stop` | `POST` | `application/json` | Command zero speed and zero steering while staying in remote mode |
| `/control/disable` | `POST` | `application/json` | Return actuator ownership to the autonomous explorer |
| `/logs` | `GET` | `text/plain` | Recent in-memory log lines |

## `/status`

### Response shape

```json
{
  "finished": false,
  "state": 1,
  "frontierReachable": true,
  "frontierBias": 0,
  "posePacked": 1687562690,
  "heading": 0,
  "controlMode": "auto",
  "remoteTimeoutMs": 1500,
  "remoteAgeMs": 42,
  "driverSpeed": 100,
  "driverTurn": 0
}
```

### Field meanings

| Field | Type | Meaning |
|---|---|---|
| `finished` | boolean | `true` when the explorer has concluded that no reachable frontier remains and the runtime has stopped exploration |
| `state` | integer | Current internal explorer state enum |
| `frontierReachable` | boolean | Whether the planner currently sees a reachable frontier cell |
| `frontierBias` | integer | Turn preference toward the current frontier plan: `-1` left, `0` neutral, `1` right |
| `posePacked` | integer | Packed 32-bit observed pose |
| `heading` | integer | Relative heading in `deg * 10`, wrapped to `0..3599` |
| `controlMode` | string | One of `auto`, `remote`, or `remote_timeout` |
| `remoteTimeoutMs` | integer | Active remote-command watchdog timeout in milliseconds |
| `remoteAgeMs` | integer | Milliseconds since the latest remote-control command update |
| `driverSpeed` | integer | Current desired speed command on the normalized driver scale |
| `driverTurn` | integer | Current desired turn command on the normalized steering scale |

### `state` enum

Current `ObservedExplorerService` state values:

| Value | Meaning |
|---|---|
| `0` | `ExplorerIdle` |
| `1` | `ExplorerForward` |
| `2` | `ExplorerRecoverStop` |
| `3` | `ExplorerRecoverReverse` |
| `4` | `ExplorerRecoverPause` |
| `5` | `ExplorerFinished` |

### `heading`

`heading` is not absolute world north.

It is the runtime's heading relative to the exploration start heading:

- unit: tenths of degrees
- range: `0..3599`
- positive direction: clockwise
- exploration start reference: truck initially facing positive observed-map `Y`

### `posePacked`

`posePacked` stores four signed bytes inside one signed 32-bit integer:

| Byte | Meaning |
|---|---|
| bits `7..0` | `x` |
| bits `15..8` | `y` |
| bits `23..16` | `direction` |
| bits `31..24` | `speed` |

Per-field meaning:

- `x`: observed-grid X cell
- `y`: observed-grid Y cell
- `direction`: heading in `5 degree` clockwise steps
- `speed`: signed commanded speed byte, not yet a true physical speed measurement

Unknown sentinel:

- `-128` in any packed byte means unknown for that field

Current runtime-specific caveat:

- the generic packed-pose helper was designed around north-referenced direction and signed `cm/s`
- the current `env:exploremap` producer actually publishes:
  - `direction = relative heading / 5 degrees`
  - `speed = normalized commanded speed`, not measured `cm/s`

So the transport shape is stable, but the last two semantics are still partly provisional.

### Driver command ranges

Current active meanings:

- `driverSpeed`
  - normalized signed command
  - positive = forward
  - negative = reverse
  - current runtime commonly uses values near `100`, `90`, `0`, and `-100`
- `driverTurn`
  - normalized signed steering command
  - negative = left
  - positive = right
  - nominal range `-100..100`

## `/map`

### Response shape

```json
{
  "apiVersion": 1,
  "width": 100,
  "height": 100,
  "cellSizeMm": 100,
  "finished": false,
  "state": 1,
  "frontierReachable": true,
  "frontierBias": 0,
  "posePacked": 1687562690,
  "controlMode": "auto",
  "robot": {
    "x": 50,
    "y": 50,
    "heading": 0,
    "speed": 100
  },
  "rows": [
    "??????????",
    "????..R???"
  ]
}
```

### Field meanings

| Field | Type | Meaning |
|---|---|---|
| `apiVersion` | integer | Current live-map payload version. The first explicit versioned payload is `1` |
| `width` | integer | Number of columns in the observed grid |
| `height` | integer | Number of rows in the observed grid |
| `cellSizeMm` | integer | Cell size in millimeters |
| `finished` | boolean | Same completion flag as `/status` |
| `state` | integer | Same explorer-state enum as `/status` |
| `frontierReachable` | boolean | Same frontier-reachability flag as `/status` |
| `frontierBias` | integer | Same frontier turn preference as `/status` |
| `posePacked` | integer | Same packed observed pose as `/status` |
| `controlMode` | string | Same control-owner state as `/status.controlMode` |
| `robot` | object | Explicit live robot pose and command snapshot for map rendering |
| `rows` | array of strings | Full row-major map snapshot |

### `robot` object

| Field | Type | Meaning |
|---|---|---|
| `x` | integer | Current observed-grid X cell |
| `y` | integer | Current observed-grid Y cell |
| `heading` | integer | Relative heading in `deg * 10`, same convention as `/status.heading` |
| `speed` | integer | Current commanded speed on the normalized driver scale |

### Current fixed geometry

Current `env:exploremap` baseline:

- `apiVersion = 1`
- `width = 100`
- `height = 100`
- `cellSizeMm = 100`

### `rows` encoding

- `rows.length == height`
- each row string length is `width`
- row `0` is the top of the rendered map
- increasing `x` moves right
- increasing row index moves downward

Current glyph meanings:

| Glyph | Meaning |
|---|---|
| `R` | current robot cell |
| `#` | known blocked cell |
| `v` | known visited cell |
| `.` | known free cell |
| `?` | unknown cell |

Rendering note:

- `R` overrides the underlying cell glyph for the current robot location
- clients should prefer `robot.x` and `robot.y` as the explicit robot position and treat `R` as a display convenience

Coordinate note:

- the observed map starts with the truck near the center of the grid
- positive runtime `Y` is rendered upward on the map, which means row index decreases as runtime `Y` increases

## `/control`

### Response shape

```json
{
  "controlMode": "remote",
  "remoteTimeoutMs": 1500,
  "remoteAgeMs": 85,
  "speed": 50,
  "steer": -20,
  "autonomyAvailable": true
}
```

### Field meanings

| Field | Type | Meaning |
|---|---|---|
| `controlMode` | string | One of `auto`, `remote`, or `remote_timeout` |
| `remoteTimeoutMs` | integer | Active watchdog timeout for remote-command freshness |
| `remoteAgeMs` | integer | Time since the last remote command update |
| `speed` | integer | Last requested remote speed command |
| `steer` | integer | Last requested remote steering command |
| `autonomyAvailable` | boolean | Whether autonomous exploration can be resumed from this runtime |

### Control ownership behavior

- `auto`
  - the autonomous explorer owns motion
- `remote`
  - supervised remote control owns motion
  - the explorer keeps updating the map, but yields actuator ownership
- `remote_timeout`
  - the remote watchdog expired
  - the truck is forced stopped
  - autonomous motion does not resume until a new explicit control action is taken

## Remote control commands

The first remote-control command layer is intentionally small and form-style rather than JSON-body based.

Current request shape:

- HTTP method: `POST`
- parameters:
  - query string, for example `POST /control/drive?speed=40&steer=-15`
  - or URL-encoded form parameters with the same field names

### `POST /control/enable`

Optional parameters:

- `timeoutMs`

Behavior:

- enables remote control
- immediately stops the truck
- pauses autonomous actuator ownership

### `POST /control/drive`

Required parameters:

- `speed`
- `steer`

Optional parameters:

- `timeoutMs`

Behavior:

- accepted only while remote control is active
- updates the remote speed and steering command
- refreshes the watchdog timer

Current command ranges:

- `speed`
  - integer
  - clamped to `-100..100`
- `steer`
  - integer
  - clamped to `-100..100`

### `POST /control/stop`

Optional parameters:

- `timeoutMs`

Behavior:

- accepted only while remote control is active
- sends zero speed and zero steering
- keeps remote mode active
- refreshes the watchdog timer

### `POST /control/disable`

Behavior:

- clears remote control ownership
- returns actuator ownership to the autonomous explorer
- restarts the autonomous state machine from its idle entry point

### Watchdog behavior

- default timeout: `1500 ms`
- accepted timeout range: `250..10000 ms`
- if remote updates expire, the truck stops and enters `controlMode = \"remote_timeout\"`
- remote timeout does not silently resume autonomous driving

## `/logs`

### Response type

- content type: `text/plain`
- method: `GET`

### Current format

Each line is currently rendered as:

```text
<millis>ms <LEVEL> <message>
```

Examples:

```text
531ms INFO Explore web server started at http://192.168.4.1/
842ms INFO Fetched /status ok
```

Current log characteristics:

- RAM-backed ring buffer
- current capacity: `64` entries
- oldest entries roll off first
- current levels written by the logger: `INFO`, `WARN`, `ERROR`
- if no logs exist yet, the response body is exactly:

```text
No logs yet.
```

## Stability Notes

The following parts are stable enough for the first display client:

- endpoint names
- HTTP `GET`
- `/status` top-level field names
- `/map` top-level field names
- `/map.apiVersion = 1`
- `/map.robot`
- `/control` top-level field names
- map glyph alphabet
- `/logs` as plain text
- Wi-Fi baseline of `ToyTruckDebug` on `192.168.4.1`

The following parts should still be treated as provisional:

- `posePacked.speed` semantics
- whether `heading` and packed `direction` should later become absolute north-referenced rather than exploration-start-referenced
- the numeric `state` enum as a long-term external API
- the exact remote-command POST encoding if a JSON-body control API is adopted later
- the future mission dispatch API beyond the current manual control layer

## Display-Agent Recommendation

For the first display implementation:

- treat `/status` and `/map` as the primary live data sources
- treat `/control` as the source of truth for current actuator ownership if manual driving is added to the display
- treat `/logs` as optional diagnostics only
- decode `posePacked`, but do not over-trust the packed `speed` byte as a physical velocity
- use `/map.rows` as the source of truth for cell rendering
- use `/map.robot` as the source of truth for the live robot marker and heading overlay
- use `/status.state`, `finished`, `frontierReachable`, `driverSpeed`, and `driverTurn` for compact status widgets
