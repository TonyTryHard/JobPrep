$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir

Set-Location $RepoRoot

$ReportImgDir = Join-Path $RepoRoot "docs/reports/img"
if (-not (Test-Path $ReportImgDir)) {
    New-Item -ItemType Directory -Path $ReportImgDir | Out-Null
}

Write-Host "==> Building ui_smoke..."
cmake --build --preset dev-windows --target ui_smoke

Write-Host "==> Running screenshot generation with JOBPREP_REPORT_IMG_DIR=$ReportImgDir..."
$env:JOBPREP_REPORT_IMG_DIR = $ReportImgDir
$env:QT_QPA_PLATFORM = "offscreen"
& .\build\dev-windows\tests\ui_smoke\ui_smoke.exe testCalendarScreenshots testStudyScreenshots

Write-Host "==> Screenshots successfully regenerated in $ReportImgDir!"
