# Agents And PlatformIO

## Purpose

This document is intended to be self-contained. Another person or agent should be able to read only this file and understand:

- the Windows-specific PlatformIO problem seen in sandboxed agent runs
- the exact repo-local workaround used here
- the commands to run
- the configuration snippets that must exist
- how to diagnose whether a failure is a source-code problem or an agent-runtime problem

## Problem Summary

This repository builds ESP32 firmware with PlatformIO on Windows.

Normal user-shell builds can succeed, but sandboxed agent builds have shown intermittent archive finalization failures during the library and framework packaging steps. Typical failures looked like:

```text
xtensa-esp32-elf-ar: unable to rename '...libWire.a'; reason: Permission denied
xtensa-esp32-elf-ranlib: unable to rename '...libFrameworkArduinoVariant.a'; reason: Permission denied
```

Important interpretation:

- compilation of `.cpp` to `.o` files worked
- the failure happened later, during archive creation or indexing
- the same environment could succeed in a normal user shell
- therefore the failure was treated as a Windows sandbox runtime issue, not automatically as a firmware-source failure

The working theory is that on Windows, short-lived file or directory locking interfered with the rename or indexing step used by `ar` and `ranlib`. Simple reruns were not enough. Retrying the actual archive commands was enough.

## What The Workaround Is And Is Not

The retry workaround is a robustness measure for transient Windows file locking. It is not a permission hack and it is not a sandbox bypass.

What it does:

- run the same `ar` or `ranlib` command again after a short delay
- wait for the temporary file or directory lock window to pass
- let the normal allowed operation succeed once the filesystem is available again

What it does not do:

- change ACLs
- request elevated privileges
- disable sandboxing
- bypass monitoring
- access files outside the normal build workflow

This distinction matters. If the sandbox or OS had permanently denied the operation, retries would never succeed. The fact that retries succeed means the operation is permitted but sometimes temporarily blocked.

The most likely underlying mechanism is:

- a sandbox supervisor, watcher, or similar process briefly opens a file or directory in the build tree
- the handle does not allow the Windows delete-share mode required for rename or replace
- `ar` or `ranlib` hits a timing window and gets `ERROR_ACCESS_DENIED`
- a short retry later succeeds after that handle is released

This class of issue is common on Windows in monitored build environments.

## Required Build Workflow

For agent builds in this repository, use this exact pattern:

```powershell
.\pio-local.ps1
pio run -j1 -e <environment>
```

Use `-j1` unless there is a strong reason not to. Single-job builds reduce Windows file-operation races during archive generation.

For verbose command inspection:

```powershell
.\pio-local.ps1
pio run -v -j1 -e <environment>
```

## Repo-Local PlatformIO Shell Setup

The repo uses a PowerShell helper script so PlatformIO state stays inside the repository instead of relying on writable global state under the user's profile.

The exact script is:

```powershell
$env:PLATFORMIO_CORE_DIR = Join-Path $PSScriptRoot ".platformio-local"
$env:PLATFORMIO_PLATFORMS_DIR = Join-Path $env:PLATFORMIO_CORE_DIR "platforms"
$env:PLATFORMIO_PACKAGES_DIR = Join-Path $env:PLATFORMIO_CORE_DIR "packages"

New-Item -ItemType Directory -Force $env:PLATFORMIO_CORE_DIR | Out-Null
New-Item -ItemType Directory -Force $env:PLATFORMIO_PLATFORMS_DIR | Out-Null
New-Item -ItemType Directory -Force $env:PLATFORMIO_PACKAGES_DIR | Out-Null

Write-Host "PlatformIO local cache configured for this shell session:"
Write-Host "  PLATFORMIO_CORE_DIR=$env:PLATFORMIO_CORE_DIR"
Write-Host "  PLATFORMIO_PLATFORMS_DIR=$env:PLATFORMIO_PLATFORMS_DIR"
Write-Host "  PLATFORMIO_PACKAGES_DIR=$env:PLATFORMIO_PACKAGES_DIR"
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Seed .platformio-local from your global PlatformIO install if needed."
Write-Host "  2. Run: pio run -e <env>"
```

Why this exists:

- earlier agent runs failed on global PlatformIO lockfiles such as `C:\Users\...\ .platformio\platforms.lock`
- putting the PlatformIO core, platforms, and packages inside the repo made builds reproducible and agent-usable

## Required `platformio.ini` Behavior

The project configuration contains three important build-system adjustments for agent-friendly Windows builds.

### 1. Repo-local build directory

PlatformIO uses a repo-local build directory:

```ini
[platformio]
build_dir = .pio-build
```

Reason:

- keep generated output in a stable in-repo path
- avoid depending on the default `.pio/build` path
- this was also tested as a diagnostic to see whether the archive problem was tied specifically to `.pio`

Result:

- the failure reproduced under `.pio-build`
- so this setting helps organization, but it was not the root fix by itself

### 2. Extra script for archive-command interception

The common environment configuration contains:

```ini
[env]
platform = espressif32@3.5.0
board = esp32dev
framework = arduino
extra_scripts = pre:tools/pio_retry_archives.py
build_flags = -v
monitor_speed = 57600
```

The key line is:

```ini
extra_scripts = pre:tools/pio_retry_archives.py
```

That script overrides the actual SCons archive and ranlib command strings so they pass through retry wrappers.

### 3. Targeted diagnostic on `accsensor`

One environment currently contains:

```ini
[env:accsensor]
build_flags =
lib_archive = no
build_src_filter = -<*.cpp> +<config.cpp> +<sensors/accsensor.cpp> +<variables/setget.cpp> +<task_safe_wire.cpp> +<z_main_accsensor.cpp>
```

Reason:

- this was introduced during diagnosis to see whether disabling user-library archiving would bypass one part of the failure

Observed result:

- it moved the failure away from `libWire.a`
- but framework archive generation still failed until the retry interception was strengthened

So this setting may be diagnostic residue or part of the current workaround path, depending on future cleanup.

## Exact Archive Retry Implementation

This is the Python extra script used by PlatformIO:

```python
from os.path import join

Import("env")

project_dir = env.subst("$PROJECT_DIR")
retry_ar = join(project_dir, "tools", "retry-ar.cmd")
retry_ranlib = join(project_dir, "tools", "retry-ranlib.cmd")
orig_ar = env.subst("$AR")
orig_ranlib = env.subst("$RANLIB")

env.Replace(
    ARCOM='"{}" "{}" $ARFLAGS $TARGET $SOURCES'.format(retry_ar, orig_ar),
    RANLIBCOM='"{}" "{}" $TARGET'.format(retry_ranlib, orig_ranlib),
)
```

Important note:

- an earlier attempt replaced only `AR` and `RANLIB`
- that did not intercept the real commands SCons executed
- overriding `ARCOM` and `RANLIBCOM` directly was necessary

### `retry-ar.cmd`

```cmd
@echo off
setlocal

set TOOL=%~1
shift
set RETRIES=5
set DELAY=2
set COUNT=0

:retry
"%TOOL%" %*
if %ERRORLEVEL%==0 exit /b 0

set /a COUNT+=1
if %COUNT% GEQ %RETRIES% exit /b %ERRORLEVEL%

timeout /t %DELAY% /nobreak >nul
goto retry
```

### `retry-ranlib.cmd`

```cmd
@echo off
setlocal

set TOOL=%~1
shift
set RETRIES=5
set DELAY=2
set COUNT=0

:retry
"%TOOL%" %*
if %ERRORLEVEL%==0 exit /b 0

set /a COUNT+=1
if %COUNT% GEQ %RETRIES% exit /b %ERRORLEVEL%

timeout /t %DELAY% /nobreak >nul
goto retry
```

What these do:

- accept the real tool path or tool name as the first argument
- run the archive tool
- if the tool fails, wait two seconds
- retry up to five times

## What Was Observed During Diagnosis

The sequence below matters because it explains why the current workaround exists.

### Stage 1: normal compile worked, archive failed

In sandboxed runs:

- `.o` compilation worked
- archive creation failed at `xtensa-esp32-elf-ar` or `xtensa-esp32-elf-ranlib`

### Stage 2: moving build output did not solve it

Changing PlatformIO to use `.pio-build` instead of `.pio/build` did not remove the failure.

Conclusion:

- the problem was not tied only to the literal `.pio` path

### Stage 3: simple archive wrapper did not intercept the real command

Replacing just `AR` and `RANLIB` was not enough.

Verbose output still showed direct archive commands like:

```text
xtensa-esp32-elf-ar rc .pio-build\accsensor\libbac\libWire.a ...
```

Conclusion:

- the interception had to target `ARCOM` and `RANLIBCOM`

### Stage 4: disabling user-library archiving was not sufficient

With:

```ini
lib_archive = no
```

on `accsensor`, the failure moved away from `libWire.a`, but still occurred on framework archive finalization such as:

```text
xtensa-esp32-elf-ranlib: unable to rename '.pio-build\accsensor\libFrameworkArduinoVariant.a'; reason: Permission denied
```

Conclusion:

- the issue was broader than one user library

### Stage 5: direct `ARCOM` and `RANLIBCOM` retries worked

After overriding the real SCons command strings, a sandboxed run of:

```powershell
.\pio-local.ps1
pio run -j1 -e accsensor
```

completed successfully.

Conclusion:

- the problem behaved like a transient Windows rename or indexing failure
- short retries were enough to survive it

## Why Windows Rename Matters Here

This problem is much more common on Windows than on Linux or macOS because Windows rename or replace semantics are stricter when other processes hold handles open.

In practice:

- creating a new `.o` file can succeed
- later renaming a temporary archive into place can fail

That is why the build looked inconsistent at first:

- compilation worked
- archive finalization failed

The archive tools were not failing to run. They were failing during the final rename or indexing stage.

## How To Decide Whether A Failure Is Real

### Treat as likely runtime-only if:

- the error is during `ar` or `ranlib`
- the error says `unable to rename`
- the error says `Permission denied`
- the same environment succeeds in the user's normal shell

### Treat as likely source or configuration problem if:

- the compiler reports missing headers
- the compiler reports syntax or type errors
- the linker reports undefined symbols unrelated to the archive workaround
- the same error reproduces in the user's normal shell

## Recommended Commands

### Normal agent build

```powershell
.\pio-local.ps1
pio run -j1 -e accsensor
```

### Verbose command inspection

```powershell
.\pio-local.ps1
pio run -v -j1 -e accsensor
```

### Example of proving the interception is active

In verbose output, you should see commands shaped like:

```text
"...\tools\retry-ar.cmd" "ar" rc ...
"...\tools\retry-ranlib.cmd" "ranlib" ...
```

If verbose output instead shows only direct `xtensa-esp32-elf-ar` and `xtensa-esp32-elf-ranlib` commands, then the interception is not wired correctly.

## Sweep Tooling

This repository also contains PowerShell helpers for repeatable multi-environment diagnostics.

### `tools/build_sweep.ps1`

Purpose:

- run a sequential full-environment sweep
- clean `.pio-build/<env>` before each run
- write per-environment stdout and stderr logs into `build_reports`
- write a compact markdown summary

Current behavior:

- reads environments from `platformio.ini`
- excludes deferred environments such as `driver` by default
- filters out known wrapper noise such as `Input redirection is not supported`

Typical use:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\build_sweep.ps1
```

### `tools/warning_scan.ps1`

Purpose:

- rerun the active environments
- collect warnings from the fresh logs
- separate project-source warnings from framework warnings

Typical use:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\warning_scan.ps1
```

Interpretation:

- project warnings are the cleanup targets
- framework warnings from `.platformio-local` are third-party noise unless they directly block something

## Current Known Good Example

The following command sequence is confirmed to complete in the sandbox with the current repo state:

```powershell
.\pio-local.ps1
pio run -j1 -e accsensor
```

## Current Known Cautions

- This document describes a Windows-specific workaround path.
- It was derived from observed behavior in a sandboxed agent runtime.
- It does not prove the underlying Windows lock source, only that retrying the archive tools is an effective repo-local mitigation.
- The current best explanation is a transient file or directory lock conflict during rename, not a permanent permission denial.
- If you remove or replace the retry wrappers, revalidate sandboxed agent builds before assuming the build path is still autonomous.
- Do not assume a sandbox-only archive rename failure means the firmware source is broken.
