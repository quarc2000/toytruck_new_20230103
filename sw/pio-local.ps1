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
