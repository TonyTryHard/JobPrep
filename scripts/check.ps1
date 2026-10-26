# JobPrep Windows Quality Check Script
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
Set-Location $repoRoot

Write-Host "==> Configuring JobPrep with dev-windows preset..." -ForegroundColor Cyan
cmake --preset dev-windows
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Building JobPrep with dev-windows preset..." -ForegroundColor Cyan
cmake --build --preset dev-windows
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Running tests with dev-windows preset..." -ForegroundColor Cyan
$ctestParallel = if ($env:CTEST_PARALLEL_LEVEL) { $env:CTEST_PARALLEL_LEVEL } else { 2 }
ctest --preset dev-windows -j $ctestParallel --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Quality check PASSED with zero warnings and all tests green!" -ForegroundColor Green
