$ErrorActionPreference = "Stop"

. .\pio-local.ps1

$reportDir = "build_reports"
New-Item -ItemType Directory -Force -Path $reportDir | Out-Null

$timestamp = "2026-03-14-current"
$excludeEnvs = @("driver")
$envs = Select-String -Path "platformio.ini" -Pattern '^\[env:([^\]]+)\]' | ForEach-Object {
    $_.Matches[0].Groups[1].Value
} | Where-Object { $_ -notin $excludeEnvs }

foreach ($env in $envs) {
    $stdout = Join-Path $reportDir ($timestamp + "-" + $env + ".stdout.log")
    $stderr = Join-Path $reportDir ($timestamp + "-" + $env + ".stderr.log")

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

$warningPattern = '^[^:]+:\d+:\d+:\s+warning:'
$matches = Select-String -Path `
    (Join-Path $reportDir ($timestamp + "-*.stdout.log")), `
    (Join-Path $reportDir ($timestamp + "-*.stderr.log")) `
    -Pattern $warningPattern | Where-Object {
        $_.Line -notmatch "COLLECT_GCC_OPTIONS=" -and
        $_.Line -notmatch "Input redirection is not supported"
    }

$projectMatches = $matches | Where-Object { $_.Line -notmatch "\\.platformio-local\\" }
$frameworkMatches = $matches | Where-Object { $_.Line -match "\\.platformio-local\\" }

$reportPath = Join-Path $reportDir ($timestamp + "-warnings-summary.md")
$lines = @(
    "# Warning Scan Summary",
    "",
    "## Project Warnings",
    ""
)

if ($projectMatches.Count -eq 0) {
    $lines += "- None"
} else {
    $lines += $projectMatches | ForEach-Object {
        "- " + (Split-Path $_.Path -Leaf) + ": " + $_.Line.Trim()
    }
}

$lines += @(
    "",
    "## Framework Warnings",
    ""
)

if ($frameworkMatches.Count -eq 0) {
    $lines += "- None"
} else {
    $lines += $frameworkMatches | ForEach-Object {
        "- " + (Split-Path $_.Path -Leaf) + ": " + $_.Line.Trim()
    }
}

$lines | Out-File -FilePath $reportPath -Encoding utf8

Write-Host "Project warnings:"
$projectMatches | ForEach-Object {
    [PSCustomObject]@{
        File = Split-Path $_.Path -Leaf
        Line = $_.Line.Trim()
    }
} | Format-List

Write-Host ""
Write-Host "Framework warnings:"
$frameworkMatches | ForEach-Object {
    [PSCustomObject]@{
        File = Split-Path $_.Path -Leaf
        Line = $_.Line.Trim()
    }
} | Format-List

Write-Host ""
Write-Host "Summary report written to $reportPath"
