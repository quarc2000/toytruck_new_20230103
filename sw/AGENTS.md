# Project Agent Context

## Project Goal
This project is using small modular toy trucks with added control board (ESP32) and with sensors. The scale of the truck is about 20:1. In the basic shape the truck has engines for both boith left and right rear wheels separately. And a servo for controlling the steering. There are aother configurations. It also had utrasound sensors in all 4 directions as well as a 6050 accelerometer and gyro. The truck also has an expansion board via i2c that has additional IO, used for the lights, and also an I2C switch which allows for plugging in more I2c devices like LV0x and Lv5x TOF laser sensors. The idea is to navigate autonomously in a laburinth.

## Instruction Precedence
- The MCP-served `AGENTS.md` and MCP context-engineering files are the starting point and must be followed first.
- All files present in the CCP shared-resources are to be seen as the base for any local copy of the same file
- Local project copies of any context file might only restrict further, if contradicting central MCP based rules that MUST be documented as a deviation.  
- After that, follow this project's local `.context/GUARDRAILS.md` and other local context files.
- If MCP guidance and local guidance conflict, stop and ask the user which to prioritise.
- Conflicts and approved deviations should be documented when that workflow is implemented in this project.
- If a conflict is detected, the agent must append it to the root `CONFLICTS.md`, stop execution, and wait for user or owner resolution before continuing.

##File access
- The agent CAN read any of the files in the repository without asking for permission.
- The agent MUST NOT ask for permission before reading files in the repository.
- Reading repository files is always pre-approved and should be done without notifying the user unless the read itself is important context for the work.
- The agent CAN and MUST keep the files PLAN.md, STATE.md, PROJECT_HISTORY.md updated without asking for permission.

## Context Files (same structure in the MCP and locally)
- Guardrails in `.context/GUARDRAILS.md` are mandatory.
- Follow policies in `.context/resources/policies`
- Follow `.context/GUARDRAILS.md` strictly, including the mandatory preflight gate.
| File | Purpose |
|------|---------|
| [.context/GUARDRAILS.md](.context/GUARDRAILS.md) | Rules that always apply. If instructions conflict, ask the user which to prioritize. |
| [.context/ARCHITECTURE.md](.context/ARCHITECTURE.md) | Canonical architecture and technical decisions. |
| [.context/STATE.md](.context/STATE.md) | Current project state and blockers. MUST be updated at every step to act as memory for the agent.|
| [.context/PROJECT_HISTORY.md]| Chronological activity log. The agent should treat this file as append-only as much as possible and avoid rereading it in normal operation, to save time and context. Read it only when necessary. |
| [.context/PLAN.md](.context/PLAN.md) | Planning the current TASK in stages. MUST be updated at every step so agent always knows where to continue.|
| [.context/TASK.md](.context/TASK.md) | Current active task. |
| [.context/TOOLS.md](.context/TOOLS.md) | Provides information on which tools are available and which are not, to avoid guessing |
| [.context/resources/README.md](.context/resources/README.md) | Index for hardware/resource reference files. |
| [CONFLICTS.md](CONFLICTS.md) | A file containing all identified conflicts between instructions, rules and policies. |
| [BACKLOG.md](BACKLOG.md) | Where we put anything we figure out we need to do, but it is not part of the current TASK. |
| [DECISIONS.md](DECISIONS.md) | Significant project decisions must be logged here with date and username. Context-architecture housekeeping and simple backlog deferrals do not belong here. |

## Optional Context Indexes (Enable As Needed)
<!-- - [.context/skills/README.md](.context/skills/README.md) | Index for project skills/guides. | -->
<!-- - [.context/tools/README.md](.context/tools/README.md) | Index for tooling notes/runbooks. | -->
<!-- - [.context/decisions/README.md](.context/decisions/README.md) | Index for ADRs/decision records. | -->
<!-- - [.context/apis/README.md](.context/apis/README.md) | Index for external API contracts/examples. | -->

## Context Management
- Always log major changes/understandings in `.context/PROJECT_HISTORY.md`.
- Keep `.context/STATE.md` current as working memory for the current task only.
- Update `.context/ARCHITECTURE.md` whenever architecture/buffer-model/state-machine decisions change.
- If anything in resources appears incorrect, ask the user before changing it.

## Standing permissions 
- The agent may update `.context/STATE.md` and append to `.context/PROJECT_HISTORY.md` after meaningful work without asking each time.

## Key Files
| File | Purpose |
|------|---------|
| `src/main.cpp` | Main application entry point. |
| `platformio.ini` | Build/upload/monitor configuration. |

## Maintenance Instructions
- Update `.context/STATE.md` as current-task memory only. Do not use it for history, and do not put next steps there if they belong in `.context/PLAN.md` or `.context/TASK.md`.
- Append to `.context/PROJECT_HISTORY.md` after significant work.
- Keep `.context/TASK.md` current with active task and next steps.
- Keep resource files updated via `.context/resources/README.md` index.
- Use the root `CONFLICTS.md` as the formal conflict register and keep it in table format.
- Use the root `DECISIONS.md` to log significant project decisions, especially around conflicts, deviations, and architectural direction. Do not use it for context-maintenance housekeeping or backlog-only deferrals.
- Whenever any file is added to or deleted from the repository, the agent MUST document the change in `.context/PROJECT_HISTORY.md`, including why the file was added or removed.
- After meaningful progress (especially after debugging struggles), remind the user that making a git commit checkpoint is a good idea.
