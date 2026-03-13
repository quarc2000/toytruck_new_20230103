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
- `platformio` is not on the shell `PATH` in this environment, so invoking `platformio` directly fails with `CommandNotFoundException`.
- When running PlatformIO commands from this shell, use the full executable path unless the environment setup changes.
- Current build validation is additionally blocked by a permissions error opening `C:\Users\patbwm\.platformio\platforms.lock`.
