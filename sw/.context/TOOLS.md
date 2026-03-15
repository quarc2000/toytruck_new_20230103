# Tools

## Available Project Tooling
- PlatformIO project configuration is defined in `platformio.ini`.
- The repository is organized for Arduino-on-ESP32 development.
- Multiple PlatformIO environments are available for focused subsystem builds.
- PlatformIO CLI is installed locally at `C:\Users\patbwm\.platformio\penv\Scripts\platformio.exe`.
- `pio.exe` is also available at `C:\Users\patbwm\.platformio\penv\Scripts\pio.exe`.

## Available Agent Capabilities In This Session
- Read and edit repository files.
- Read MCP shared resources, including the shared `AGENTS.md` and resource index files.
- Run shell commands for inspection and verification, subject to environment permissions.

## Constraints
- Do not guess unavailable project tools or scripts; verify them from the repository first.
- Build and runtime validation should be performed against the specific PlatformIO environment relevant to the task.
- Treat shared MCP resources as the base reference when local copies overlap.
- `pio` is on the shell `PATH` in this environment and resolves to `C:\Users\patbwm\.platformio\penv\Scripts\pio.exe`.
- Direct `pio run -e <env>` and `pio run -e <env> -t <target>` invocations work when they are run in a shell session that has first sourced `.\pio-local.ps1`.
- Running `pio run -e <env>` without first sourcing `.\pio-local.ps1` still risks the old `C:\Users\patbwm\.platformio\platforms.lock` permission failure.
- Device-dependent targets such as upload cannot be validated unless the hardware is connected.

## PlatformIO Workarounds
- If the default PlatformIO home under `C:\Users\patbwm\.platformio` is read-only from the current agent sandbox, build commands may fail before compilation when PlatformIO tries to create or open `platforms.lock`.
- In that case, use a workspace-local PlatformIO home rooted at `.platformio-local` and point PlatformIO at it with these environment variables before invoking the CLI:
  - `PLATFORMIO_CORE_DIR=<repo>\\.platformio-local`
  - `PLATFORMIO_PLATFORMS_DIR=<repo>\\.platformio-local\\platforms`
  - `PLATFORMIO_PACKAGES_DIR=<repo>\\.platformio-local\\packages`
- Seed `.platformio-local` by copying the already-installed platform and package directories from `C:\Users\patbwm\.platformio` into the matching local `platforms` and `packages` subdirectories. At minimum, previous successful setup used `platforms\\espressif32` plus the required packages such as `framework-arduinoespressif32`, `tool-esptoolpy`, `tool-mkspiffs`, `tool-mklittlefs`, `tool-scons`, and `toolchain-xtensa32`.
- Example PowerShell pattern for future runs:
```powershell
$env:PLATFORMIO_CORE_DIR = (Resolve-Path '.platformio-local').Path
$env:PLATFORMIO_PLATFORMS_DIR = (Resolve-Path '.platformio-local\platforms').Path
$env:PLATFORMIO_PACKAGES_DIR = (Resolve-Path '.platformio-local\packages').Path
pio run -e usensor
```
- Verified in this shell on 2026-03-14:
  - `pio --version` succeeds.
  - `pio project config --json-output` succeeds.
  - `.\pio-local.ps1` correctly redirects PlatformIO home, platforms, and packages into `.platformio-local` for the current shell session.
  - `pio run -e maptest` succeeds when run in the same shell session after sourcing `.\pio-local.ps1`.
  - `pio run -e maptest -t clean` succeeds directly.
- The local-cache workflow is sufficient for focused builds and uploads in this sandbox when the shell first sources `.\pio-local.ps1`.
- The repo now carries the working Windows archive workaround:
  - `platformio.ini` uses `build_dir = .pio-build`
  - `platformio.ini` loads `extra_scripts = pre:tools/pio_retry_archives.py`
  - `tools/pio_retry_archives.py` overrides SCons `ARCOM` and `RANLIBCOM`
  - `tools/retry-ar.cmd` and `tools/retry-ranlib.cmd` retry transient archive-tool failures
- The detailed source of truth for this workflow is `.context/resources/AGENTS_AND_PLATFORMIO.md`. Future agents should use that file instead of trying to reconstruct the workaround from shell history.
- Treat `.platformio-local` as a disposable workspace cache. Recreate or refresh it from the installed PlatformIO packages if it becomes stale or incomplete.
