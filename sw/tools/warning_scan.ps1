$ErrorActionPreference = "Stop"

. .\pio-local.ps1

$reportDir = "build_reports"
New-Item -ItemType Directory -Force -Path $reportDir | Out-Null

$envs = @(
    "accsensor",
    "accsensorkalman",
    "hwtest",
    "accsensorcalibration",
    "gy271",
    "usensor",
    "driver",
    "steer",
    "expander"
)

foreach ($env in $envs) {
    $stdout = Join-Path $reportDir ("2026-03-14-current-" + $env + ".stdout.log")
    $stderr = Join-Path $reportDir ("2026-03-14-current-" + $env + ".stderr.log")

    if (Test-Path $stdout) { Remove-Item $stdout -Force }
    if (Test-Path $stderr) { Remove-Item $stderr -Force }

    Start-Process -FilePath "pio" `
        -ArgumentList @("run", "-e", $env) `
        -WorkingDirectory (Get-Location).Path `
        -NoNewWindow `
        -PassThru `
        -Wait `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr | Out-Null
}

$matches = Select-String -Path `
    (Join-Path $reportDir "2026-03-14-current-*.stdout.log"), `
    (Join-Path $reportDir "2026-03-14-current-*.stderr.log") `
    -Pattern "warning:"

$matches | ForEach-Object {
    [PSCustomObject]@{
        File = Split-Path $_.Path -Leaf
        Line = $_.Line.Trim()
    }
} | Format-List
