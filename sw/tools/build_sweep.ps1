param(
    [string[]]$ConfigFiles = @("platformio.ini", "platformio_optional.ini"),
    [string]$Timestamp = "2026-03-14-resweep2"
)

$ErrorActionPreference = "Stop"

. .\pio-local.ps1

$reportDir = "build_reports"
New-Item -ItemType Directory -Force -Path $reportDir | Out-Null

$files = $ConfigFiles | Where-Object { Test-Path $_ }
$envs = foreach ($file in $files) {
    Select-String -Path $file -Pattern '^\[env:([^\]]+)\]' | ForEach-Object {
        $_.Matches[0].Groups[1].Value
    }
}
$envs = $envs | Select-Object -Unique

$summary = @()
foreach ($envName in $envs) {
    cmd /c "rmdir /s /q .pio\build\$envName" | Out-Null

    $stdout = Join-Path $reportDir ($Timestamp + "-" + $envName + ".stdout.log")
    $stderr = Join-Path $reportDir ($Timestamp + "-" + $envName + ".stderr.log")

    if (Test-Path $stdout) { Remove-Item $stdout -Force }
    if (Test-Path $stderr) { Remove-Item $stderr -Force }

    $argList = @()
    foreach ($file in $files) {
        $argList += @("-c", $file)
    }
    $argList += @("run", "-e", $envName)

    $proc = Start-Process -FilePath "pio" `
        -ArgumentList $argList `
        -WorkingDirectory (Get-Location).Path `
        -NoNewWindow `
        -PassThru `
        -Wait `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr

    $matches = Select-String -Path $stdout, $stderr `
        -Pattern "fatal error:|error:|Permission denied|undefined reference|No such file or directory|cannot find|ranlib: unable to rename|ar: unable to rename" `
        -CaseSensitive:$false | Select-Object -First 6

    $diag = if ($matches) {
        ($matches | ForEach-Object { $_.Line.Trim() }) -join " || "
    } else {
        ""
    }

    $summary += [PSCustomObject]@{
        env         = $envName
        exit_code   = $proc.ExitCode
        status      = $(if ($proc.ExitCode -eq 0) { "SUCCESS" } else { "FAIL" })
        diagnostics = $diag
        stdout_log  = $stdout
        stderr_log  = $stderr
    }
}

$summaryPath = Join-Path $reportDir ($Timestamp + "-summary.md")
$lines = @(
    "# Build Sweep Summary",
    "",
    "| env | status | exit | diagnostics |",
    "|---|---|---:|---|"
)
$lines += $summary | ForEach-Object {
    "| $($_.env) | $($_.status) | $($_.exit_code) | $($_.diagnostics.Replace('|', '\|')) |"
}
$lines | Out-File -FilePath $summaryPath -Encoding utf8

Get-Content $summaryPath
