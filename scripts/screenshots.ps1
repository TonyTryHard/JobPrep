$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir

Set-Location $RepoRoot

$ReportImgDir = Join-Path $RepoRoot "docs/reports/img"
if (-not (Test-Path $ReportImgDir)) {
    New-Item -ItemType Directory -Path $ReportImgDir | Out-Null
}

Write-Host "==> Building screenshot test binaries..."
cmake --build --preset dev-windows --target tst_PageCalendar tst_PageStudy tst_PageHome tst_PageSettings

Write-Host "==> Running screenshot generation with JOBPREP_REPORT_IMG_DIR=$ReportImgDir..."
$env:JOBPREP_REPORT_IMG_DIR = $ReportImgDir
$env:QT_QPA_PLATFORM = "offscreen"
& .\build\dev-windows\tests\ui\tst_PageCalendar.exe testCalendarScreenshots
& .\build\dev-windows\tests\ui\tst_PageStudy.exe testStudyScreenshots
& .\build\dev-windows\tests\ui\tst_PageHome.exe testDashboardScreenshots testToastScreenshots
& .\build\dev-windows\tests\ui\tst_PageSettings.exe testSettingsScreenshots

Write-Host "==> Screenshots successfully regenerated in $ReportImgDir!"
