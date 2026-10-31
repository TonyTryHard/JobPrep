# Milestone 6 CI Fix-up 3 Report: Windows Test Failure Visibility

**Environment:**
- OS: Linux tony-laptop 7.0.0-38-generic #38-Ubuntu SMP PREEMPT_DYNAMIC Fri Sep 4 09:10:14 UTC 2026 x86_64 GNU/Linux
- Compiler: g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0
- Build tool: cmake version 4.2.3, ninja 1.13.2
- Qt: Qt version 6.10.2 in /usr/lib/x86_64-linux-gnu
- Git HEAD commit: `418817b` (`ci: add Windows test diagnostic step and enable stderr logging`)
- Test suite total: **33 binaries, 454 functions**
- Local gate runtime: `Total Test time (real) = 45.30 sec`

---

## 1. Facts from the Second GitHub Actions Run

1. **Ubuntu job:** Configure, build, and test runs passed completely (`100% tests passed out of 33`), Qt 6.8.2. Linux is 100% green.
2. **Windows job:** Configure and build passed cleanly across all 222 targets using MSVC (`/W4 /permissive- /WX`, zero `warning C` compiler warnings). Then: `61% tests passed, 13 tests failed out of 33`, total test time 227 s.
3. **Test Breakdown on Windows:**
   - **Failed (13 tests):** `ui_smoke`, `tst_AgendaListModel`, `tst_TopicListModel`, `tst_ImportExport`, `tst_SettingsService`, `tst_Notifiers`, `tst_StudyRegression`, `tst_PageJobs`, `tst_PageCalendar`, `tst_PageStudy`, `tst_PageHome`, `tst_PageSettings`, `tst_Shortcuts`.
   - **Passed (20 tests):** all 9 `tests/data` tests (`tst_ApplicationRepository`, `tst_Database`, `tst_InterviewRepository`, `tst_ReminderLogRepository`, `tst_SeedService`, `tst_SessionRepository`, `tst_TopicRepository`, `tst_TrackRepository`, `tst_TransactionBehavior`), `tst_MonthView`, `tst_ApplicationTableModel`, `tst_ApplicationFilterProxy`, `tst_TopicBoardProxy`, `tst_TopicTableProxy`, `tst_AgendaService`, `tst_DayWatcher`, `tst_ExportService`, `tst_ReminderService`, `tst_StatsService`, `tst_EmptyStatesAndFocus`.
4. **Lost Output:** `scripts/check.ps1` runs `ctest --preset dev-windows -j N --output-on-failure`, yet after each `***Failed` line the output block is empty (one blank line). The output never reached ctest's pipe. The uploaded artifact `test-logs-windows-latest` was only 3103 bytes (2 files).
5. **Execution Duration:** Several UI tests take noticeably longer on Windows (`tst_PageStudy` 97 s, `tst_PageHome` 88 s, `tst_PageJobs` 59 s, `tst_PageCalendar` 33 s) vs. ~1–5 s on Linux, suggesting wait/timeout loops (e.g. `QTRY_*` or `qWaitForWindowActive`) expiring. The tests run and fail, but their output text is lost. (Hypothesis, not fact).
6. **Qt Windows Logging Behavior:** `qDebug/qWarning/qFatal` output routes to the Windows debugger via `OutputDebugString` unless stderr is detected as a console; `QT_FORCE_STDERR_LOGGING=1` and `QT_ASSUME_STDERR_HAS_CONSOLE=1` force console stderr emission. In addition, QtTest result streams can be written directly to a file via `-o <file>,txt`, bypassing pipe buffering.

---

## 2. Task 1: Evidence First

### Full `.github/workflows/ci.yml` (Prior to Changes)
```yaml
name: CI

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]

permissions:
  contents: read

concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true

jobs:
  gate:
    name: Gate (${{ matrix.os }})
    runs-on: ${{ matrix.os }}
    timeout-minutes: 45

    strategy:
      fail-fast: false
      matrix:
        include:
          - os: ubuntu-latest
            qt-arch: linux_gcc_64
            qt-target: desktop
          - os: windows-latest
            qt-arch: win64_msvc2022_64
            qt-target: desktop

    steps:
      - name: Checkout repository
        uses: actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683 # v4.2.2

      - name: Set up Python for aqtinstall
        uses: actions/setup-python@42375524e23c412d93fb67b49958b491fce71c38 # v5.4.0
        with:
          python-version: '3.11'

      - name: Install Ninja and system libraries (Linux)
        if: runner.os == 'Linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y --no-install-recommends \
            ninja-build \
            libgl1-mesa-dev \
            libfontconfig1 \
            libdbus-1-3 \
            libxkbcommon0 \
            libxkbcommon-x11-0 \
            fonts-dejavu-core

      - name: Set up MSVC environment (Windows)
        if: runner.os == 'Windows'
        uses: ilammy/msvc-dev-cmd@0b201ec74fa43914dc39ae48a89fd1d8cb592756 # v1.13.0
        with:
          arch: x64

      - name: Cache Qt installation
        id: cache-qt
        uses: actions/cache@d4323d4df104b026a6aa633fdb11d772146be0bf # v4.2.2
        with:
          path: ${{ runner.workspace }}/Qt
          key: qt-6.8.2-${{ matrix.os }}-${{ matrix.qt-arch }}-aqt-3.2.0

      - name: Install Qt via aqtinstall
        if: steps.cache-qt.outputs.cache-hit != 'true'
        shell: bash
        run: |
          pip install aqtinstall==3.2.0
          python -m aqt install-qt ${{ runner.os == 'Windows' && 'windows' || 'linux' }} ${{ matrix.qt-target }} 6.8.2 ${{ matrix.qt-arch }} \
            --outputdir "${{ runner.workspace }}/Qt"

      - name: Locate Qt prefix (Linux)
        if: runner.os == 'Linux'
        shell: bash
        run: |
          CONFIG_FILE=$(find "${RUNNER_WORKSPACE}/Qt" -type f -name Qt6Config.cmake -path "*/lib/cmake/Qt6/Qt6Config.cmake" | head -n 1)
          if [ -z "${CONFIG_FILE}" ]; then
            CONFIG_FILE=$(find "${RUNNER_WORKSPACE}/Qt" -type f -name Qt6Config.cmake | head -n 1)
          fi
          if [ -z "${CONFIG_FILE}" ]; then
            echo "::error::Qt6Config.cmake not found under ${RUNNER_WORKSPACE}/Qt" >&2
            exit 1
          fi
          QT_PREFIX=$(cd "$(dirname "${CONFIG_FILE}")/../../.." && pwd)
          echo "Found QT_PREFIX_PATH: ${QT_PREFIX}"
          echo "QT_PREFIX_PATH=${QT_PREFIX}" >> "${GITHUB_ENV}"

      - name: Locate Qt prefix and set PATH (Windows)
        if: runner.os == 'Windows'
        shell: pwsh
        run: |
          $configFile = Get-ChildItem -Path "$env:RUNNER_WORKSPACE\Qt" -Filter Qt6Config.cmake -Recurse | Select-Object -First 1
          if (-not $configFile) {
            Write-Error "Qt6Config.cmake not found under $env:RUNNER_WORKSPACE\Qt"
            exit 1
          }
          $qtPrefix = $configFile.Directory.Parent.Parent.Parent.FullName
          Write-Host "Found QT_PREFIX_PATH: $qtPrefix"
          "QT_PREFIX_PATH=$qtPrefix" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
          "$qtPrefix\bin" | Out-File -FilePath $env:GITHUB_PATH -Append -Encoding utf8

      - name: Diagnostic information (Linux)
        if: runner.os == 'Linux'
        shell: bash
        run: |
          echo "=== Tools & Compiler ==="
          cmake --version
          ninja --version
          g++ --version | head -1
          echo "=== Qt Environment ==="
          echo "QT_PREFIX_PATH: ${QT_PREFIX_PATH}"
          echo "--- platforms ---"
          ls -la "${QT_PREFIX_PATH}/plugins/platforms" || true
          echo "--- sqldrivers ---"
          ls -la "${QT_PREFIX_PATH}/plugins/sqldrivers" || true

      - name: Diagnostic information (Windows)
        if: runner.os == 'Windows'
        shell: pwsh
        run: |
          Write-Host "=== Tools & Compiler ==="
          cmake --version
          ninja --version
          & cl.exe 2>&1 | Select-Object -First 1
          Write-Host "=== Qt Environment ==="
          Write-Host "QT_PREFIX_PATH: $env:QT_PREFIX_PATH"
          Write-Host "--- where Qt6Core.dll ---"
          where.exe Qt6Core.dll
          Write-Host "--- platforms ---"
          Get-ChildItem -Path "$env:QT_PREFIX_PATH\plugins\platforms" | Select-Object Name
          Write-Host "--- sqldrivers ---"
          Get-ChildItem -Path "$env:QT_PREFIX_PATH\plugins\sqldrivers" | Select-Object Name

      - name: Run quality gate (Linux)
        if: runner.os == 'Linux'
        env:
          CMAKE_BUILD_PARALLEL_LEVEL: 2
          QT_QPA_PLATFORM: offscreen
          JOBPREP_KEEP_GOING: 1
        run: ./scripts/check.sh

      - name: Run quality gate (Windows)
        if: runner.os == 'Windows'
        env:
          CMAKE_BUILD_PARALLEL_LEVEL: 2
          QT_QPA_PLATFORM: offscreen
          JOBPREP_KEEP_GOING: 1
        shell: pwsh
        run: ./scripts/check.ps1

      - name: Upload test logs on failure
        if: failure()
        uses: actions/upload-artifact@4cec3d8aa04e39d1a68397de0c4cd6fb9dce8ec1 # v4.6.1
        with:
          name: test-logs-${{ matrix.os }}
          if-no-files-found: ignore
          path: |
            build/*/Testing/Temporary/LastTest.log
            build/*/Testing/Temporary/*.log
```

### Full `scripts/check.ps1`
```powershell
# JobPrep Windows Quality Check Script
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
Set-Location $repoRoot

Write-Host "==> Configuring JobPrep with dev-windows preset..." -ForegroundColor Cyan
cmake --preset dev-windows
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Building JobPrep with dev-windows preset..." -ForegroundColor Cyan
if ($env:JOBPREP_KEEP_GOING -eq "1") {
    cmake --build --preset dev-windows -- -k 0
} else {
    cmake --build --preset dev-windows
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Running tests with dev-windows preset..." -ForegroundColor Cyan
$ctestParallel = if ($env:CTEST_PARALLEL_LEVEL) { $env:CTEST_PARALLEL_LEVEL } else { 2 }
ctest --preset dev-windows -j $ctestParallel --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Quality check PASSED with zero warnings and all tests green!" -ForegroundColor Green
```

### `add_test` & `set_tests_properties` Blocks in Test CMakeLists
```bash
$ grep -rn -C3 "add_test\|set_tests_properties" tests/CMakeLists.txt tests/models/CMakeLists.txt tests/services/CMakeLists.txt tests/ui/CMakeLists.txt tests/ui_smoke/CMakeLists.txt
tests/models/CMakeLists.txt-14-    add_executable(${test_name} ${test_name}.cpp)
tests/models/CMakeLists.txt-15-    target_link_libraries(${test_name} PRIVATE jobprep_core jobprep_test_helpers Qt6::Test)
tests/models/CMakeLists.txt-16-    jobprep_apply_warnings(${test_name})
tests/models/CMakeLists.txt:17:    add_test(NAME ${test_name} COMMAND ${test_name})
tests/models/CMakeLists.txt:18:    set_tests_properties(${test_name} PROPERTIES
tests/models/CMakeLists.txt-19-        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
tests/models/CMakeLists.txt-20-        LABELS "model"
tests/models/CMakeLists.txt-21-    )
--
tests/services/CMakeLists.txt-12-    add_executable(${test_name} ${test_name}.cpp)
tests/services/CMakeLists.txt-13-    target_link_libraries(${test_name} PRIVATE jobprep_core jobprep_test_helpers Qt6::Test)
tests/services/CMakeLists.txt-14-    jobprep_apply_warnings(${test_name})
tests/services/CMakeLists.txt:15:    add_test(NAME ${test_name} COMMAND ${test_name})
tests/services/CMakeLists.txt:16:    set_tests_properties(${test_name} PROPERTIES
tests/services/CMakeLists.txt-17-        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
tests/services/CMakeLists.txt-18-        LABELS "service"
tests/services/CMakeLists.txt-19-    )
--
tests/ui/CMakeLists.txt-3-target_include_directories(tst_Notifiers PRIVATE ${CMAKE_SOURCE_DIR}/tests/data)
tests/ui/CMakeLists.txt-4-jobprep_apply_warnings(tst_Notifiers)
tests/ui/CMakeLists.txt-5-
tests/ui/CMakeLists.txt:6:add_test(NAME tst_Notifiers COMMAND tst_Notifiers)
tests/ui/CMakeLists.txt:7:set_tests_properties(tst_Notifiers PROPERTIES
tests/ui/CMakeLists.txt-8-    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
tests/ui/CMakeLists.txt-9-    LABELS "ui"
tests/ui/CMakeLists.txt-10-)
--
tests/ui/CMakeLists.txt-14-target_include_directories(tst_StudyRegression PRIVATE ${CMAKE_SOURCE_DIR}/tests/data)
tests/ui/CMakeLists.txt-15-jobprep_apply_warnings(tst_StudyRegression)
tests/ui/CMakeLists.txt-16-
tests/ui/CMakeLists.txt:17:add_test(NAME tst_StudyRegression COMMAND tst_StudyRegression)
tests/ui/CMakeLists.txt:18:set_tests_properties(tst_StudyRegression PROPERTIES
tests/ui/CMakeLists.txt-19-    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
tests/ui/CMakeLists.txt-20-    LABELS "ui"
tests/ui/CMakeLists.txt-21-)
--
tests/ui/CMakeLists.txt-34-    add_executable(${test_name} ${test_name}.cpp)
tests/ui/CMakeLists.txt-35-    target_link_libraries(${test_name} PRIVATE jobprep_core jobprep_ui_test_helpers Qt6::Test Qt6::Widgets)
tests/ui/CMakeLists.txt-36-    jobprep_apply_warnings(${test_name})
tests/ui/CMakeLists.txt:37:    add_test(NAME ${test_name} COMMAND ${test_name})
tests/ui/CMakeLists.txt:38:    set_tests_properties(${test_name} PROPERTIES
tests/ui/CMakeLists.txt-39-        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
tests/ui/CMakeLists.txt-40-        LABELS "ui"
tests/ui/CMakeLists.txt-41-    )
--
tests/ui_smoke/CMakeLists.txt-10-
tests/ui_smoke/CMakeLists.txt-11-jobprep_apply_warnings(ui_smoke)
tests/ui_smoke/CMakeLists.txt-12-
tests/ui_smoke/CMakeLists.txt:13:add_test(NAME ui_smoke COMMAND ui_smoke)
tests/ui_smoke/CMakeLists.txt:14:set_tests_properties(ui_smoke PROPERTIES
tests/ui_smoke/CMakeLists.txt-15-    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
tests/ui_smoke/CMakeLists.txt-16-    LABELS "ui"
tests/ui_smoke/CMakeLists.txt-17-)
--
tests/ui_smoke/CMakeLists.txt-22-    Qt6::Test
tests/ui_smoke/CMakeLists.txt-23-)
tests/ui_smoke/CMakeLists.txt-24-jobprep_apply_warnings(tst_MonthView)
tests/ui_smoke/CMakeLists.txt:25:add_test(NAME tst_MonthView COMMAND tst_MonthView)
tests/ui_smoke/CMakeLists.txt:26:set_tests_properties(tst_MonthView PROPERTIES
tests/ui_smoke/CMakeLists.txt-27-    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
tests/ui_smoke/CMakeLists.txt-28-    LABELS "ui"
tests/ui_smoke/CMakeLists.txt-29-)
```

### Local Tool Availability
```bash
$ command -v pwsh actionlint
(no output)
$ echo "exit=$?"
exit=1
```
Neither `pwsh` nor `actionlint` is installed on this local Linux development machine.

---

## 3. Tasks 2 & 3: Workflow Updates

### Task 2: Windows Gate Environment
Added to the step `Run quality gate (Windows)` in `.github/workflows/ci.yml`:
```yaml
          QT_FORCE_STDERR_LOGGING: 1
          QT_ASSUME_STDERR_HAS_CONSOLE: 1
```
Linux environment was kept unchanged to maintain a minimal diff.

### Task 3: Diagnostic Step for Failed Windows Tests
Placed immediately following the Windows gate step and before the artifact upload:
- Triggered exclusively on `if: failure() && runner.os == 'Windows'`.
- Configured with `continue-on-error: true` and no `exit 1` to ensure the artifact upload step always executes.
- Reads failed test names from `build/dev-windows/Testing/Temporary/LastTestsFailed.log` (falling back to finding `tst_*.exe` / `ui_smoke.exe` if the file is absent).
- Executes each failing test serially with `$env:QT_QPA_PLATFORM = "offscreen"`, `-v2`, and `-o "<logdir>/<name>.txt,txt"`.
- Enforces a 150 s per-test timeout via `Start-Process -PassThru` and `WaitForExit(150000)`, killing on timeout and printing `TIMEOUT`.
- Emits test header, decimal and hex exit code (`'{0} (0x{0:X8})' -f $code`), and file content (displaying first 120 and last 60 lines with line-cut count if exceeding 200 lines).
- Extended artifact paths to capture `build/*/Testing/Temporary/LastTestsFailed.log` and `build/*/Testing/Temporary/diag/*.txt`.

---

## 4. Task 4: Static Verification & Diffs

### YAML Validation
```bash
$ python3 -c "import yaml; yaml.safe_load(open('.github/workflows/ci.yml')); print('yaml ok')"
yaml ok
```

### Actionlint & PowerShell Parse Checks
- `actionlint`: not run (not installed locally).
- `pwsh` parse-check: not run (not installed locally).

### Git Diff
```bash
$ git diff 007a550..418817b -- .github/workflows/ci.yml
diff --git a/.github/workflows/ci.yml b/.github/workflows/ci.yml
index 0112422..a6e96ca 100644
--- a/.github/workflows/ci.yml
+++ b/.github/workflows/ci.yml
@@ -149,9 +149,67 @@ jobs:
           CMAKE_BUILD_PARALLEL_LEVEL: 2
           QT_QPA_PLATFORM: offscreen
           JOBPREP_KEEP_GOING: 1
+          QT_FORCE_STDERR_LOGGING: 1
+          QT_ASSUME_STDERR_HAS_CONSOLE: 1
         shell: pwsh
         run: ./scripts/check.ps1
 
+      - name: Diagnose failed tests (Windows)
+        if: failure() && runner.os == 'Windows'
+        continue-on-error: true
+        shell: pwsh
+        run: |
+          $logdir = "build/dev-windows/Testing/Temporary/diag"
+          New-Item -ItemType Directory -Force -Path $logdir | Out-Null
+          $logFile = "build/dev-windows/Testing/Temporary/LastTestsFailed.log"
+          $testNames = @()
+          if (Test-Path $logFile) {
+            Get-Content $logFile | ForEach-Object {
+              if ($_ -match '^\d+:(.+)$') {
+                $testNames += $Matches[1].Trim()
+              }
+            }
+          } else {
+            Write-Host "LastTestsFailed.log missing"
+            Get-ChildItem -Path "build/dev-windows/tests" -Recurse -Filter "*.exe" | ForEach-Object {
+              if ($_.BaseName -like "tst_*" -or $_.BaseName -eq "ui_smoke") {
+                $testNames += $_.BaseName
+              }
+            }
+          }
+          $env:QT_QPA_PLATFORM = "offscreen"
+          foreach ($name in $testNames) {
+            $exe = Get-ChildItem -Path "build/dev-windows" -Recurse -Filter "$name.exe" | Select-Object -First 1
+            if (-not $exe) {
+              Write-Host "Executable for $name not found"
+              continue
+            }
+            $outFile = "$logdir/$name.txt"
+            $proc = Start-Process -FilePath $exe.FullName -ArgumentList @("-v2", "-o", "$outFile,txt") -PassThru -NoNewWindow
+            $finished = $proc.WaitForExit(150000)
+            if (-not $finished) {
+              $proc.Kill()
+              $proc.WaitForExit()
+              Write-Host "TIMEOUT"
+            }
+            $code = $proc.ExitCode
+            Write-Host "=== TEST: $name ==="
+            Write-Host ('{0} (0x{0:X8})' -f $code)
+            if (Test-Path $outFile) {
+              $lines = @(Get-Content $outFile)
+              if ($lines.Count -le 200) {
+                $lines | ForEach-Object { Write-Host $_ }
+              } else {
+                $lines[0..119] | ForEach-Object { Write-Host $_ }
+                $cut = $lines.Count - 180
+                Write-Host "... [$cut lines cut] ..."
+                $lines[($lines.Count - 60)..($lines.Count - 1)] | ForEach-Object { Write-Host $_ }
+              }
+            } else {
+              Write-Host "(no output file produced)"
+            }
+          }
+
       - name: Upload test logs on failure
         if: failure()
         uses: actions/upload-artifact@4cec3d8aa04e39d1a68397de0c4cd6fb9dce8ec1 # v4.6.1
@@ -161,3 +219,5 @@ jobs:
           path: |
             build/*/Testing/Temporary/LastTest.log
             build/*/Testing/Temporary/*.log
+            build/*/Testing/Temporary/LastTestsFailed.log
+            build/*/Testing/Temporary/diag/*.txt
```

### Git Diff Stat
```bash
$ git diff --stat 007a550..418817b
 .github/workflows/ci.yml | 60 ++++++++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 60 insertions(+)
```

---

## 5. Task 5: Local Regression Verification

```bash
$ CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.2s)
-- Generating done (0.2s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
ninja: no work to do.
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start 33: tst_EmptyStatesAndFocus
      Start 29: tst_PageStudy
 1/33 Test #33: tst_EmptyStatesAndFocus ..........   Passed   10.56 sec
      Start 30: tst_PageHome
 2/33 Test #29: tst_PageStudy ....................   Passed   18.90 sec
      Start 27: tst_PageJobs
 3/33 Test #30: tst_PageHome .....................   Passed   16.83 sec
      Start 28: tst_PageCalendar
 4/33 Test #27: tst_PageJobs .....................   Passed   12.04 sec
      Start 32: tst_Shortcuts
 5/33 Test #32: tst_Shortcuts ....................   Passed    3.00 sec
      Start 26: tst_StudyRegression
 6/33 Test #28: tst_PageCalendar .................   Passed    7.24 sec
      Start 31: tst_PageSettings
 7/33 Test #31: tst_PageSettings .................   Passed    4.24 sec
      Start 25: tst_Notifiers
 8/33 Test #26: tst_StudyRegression ..............   Passed    6.90 sec
      Start 10: ui_smoke
 9/33 Test #25: tst_Notifiers ....................   Passed    2.64 sec
      Start 21: tst_ImportExport
10/33 Test #21: tst_ImportExport .................   Passed    0.67 sec
      Start 24: tst_StatsService
11/33 Test #24: tst_StatsService .................   Passed    0.94 sec
      Start  8: tst_SeedService
12/33 Test  #8: tst_SeedService ..................   Passed    0.28 sec
      Start 15: tst_TopicListModel
13/33 Test #15: tst_TopicListModel ...............   Passed    0.18 sec
      Start 22: tst_ReminderService
14/33 Test #22: tst_ReminderService ..............   Passed    0.08 sec
      Start 17: tst_TopicTableProxy
15/33 Test #17: tst_TopicTableProxy ..............   Passed    0.07 sec
      Start 23: tst_SettingsService
16/33 Test #23: tst_SettingsService ..............   Passed    0.02 sec
      Start 16: tst_TopicBoardProxy
17/33 Test #16: tst_TopicBoardProxy ..............   Passed    0.05 sec
      Start 18: tst_AgendaService
18/33 Test #18: tst_AgendaService ................   Passed    0.09 sec
      Start  5: tst_ApplicationRepository
19/33 Test  #5: tst_ApplicationRepository ........   Passed    0.08 sec
      Start 11: tst_MonthView
20/33 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start  3: tst_TopicRepository
21/33 Test  #3: tst_TopicRepository ..............   Passed    0.09 sec
      Start 19: tst_DayWatcher
22/33 Test #19: tst_DayWatcher ...................   Passed    0.01 sec
      Start  1: tst_Database
23/33 Test  #1: tst_Database .....................   Passed    0.03 sec
      Start 12: tst_AgendaListModel
24/33 Test #12: tst_AgendaListModel ..............   Passed    0.03 sec
      Start  6: tst_InterviewRepository
25/33 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start  9: tst_TransactionBehavior
26/33 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start  4: tst_SessionRepository
27/33 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  2: tst_TrackRepository
28/33 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start 13: tst_ApplicationTableModel
29/33 Test #13: tst_ApplicationTableModel ........   Passed    0.04 sec
      Start  7: tst_ReminderLogRepository
30/33 Test  #7: tst_ReminderLogRepository ........   Passed    0.02 sec
      Start 14: tst_ApplicationFilterProxy
31/33 Test #14: tst_ApplicationFilterProxy .......   Passed    0.02 sec
      Start 20: tst_ExportService
32/33 Test #20: tst_ExportService ................   Passed    0.01 sec
33/33 Test #10: ui_smoke .........................   Passed    4.45 sec

100% tests passed, 0 tests failed out of 33

Label Time Summary:
data       =   0.61 sec*proc (9 tests)
model      =   0.40 sec*proc (6 tests)
service    =   1.83 sec*proc (7 tests)
ui         =  86.90 sec*proc (11 tests)

Total Test time (real) =  45.30 sec
==> Quality check PASSED with zero warnings and all tests green!
```

### Clean Git Status
```bash
$ git status --short
(no output)
```

---

## 6. Task 6: Candidate Hypotheses (Unverified, No Fixes Applied)

Auditing the codebase and tests revealed the following concrete candidates for Windows-specific differences:

1. **`QTemporaryFile` & File Sharing in `tst_SettingsService`:**
   ```bash
   $ grep -n -A5 "QTemporaryFile temp" tests/services/tst_SettingsService.cpp
   24:    QTemporaryFile temp;
   25-    QVERIFY(temp.open());
   26-    const QString path = temp.fileName();
   27-    temp.close();
   28-
   29-    auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
   ```
   On Windows, `QTemporaryFile` keeps file access flags that can conflict with another handle or reopen attempt until destruction, potentially causing sharing violations or failed writes in `QSettings`.

2. **Linux-Specific `XDG_DATA_HOME` in `tst_ImportExport`:**
   ```bash
   $ grep -n -A5 "XDG_DATA_HOME" tests/services/tst_ImportExport.cpp
   950:    qputenv("XDG_DATA_HOME", customXdg.toLocal8Bit());
   951-
   952-    const QString prodSafetyDir =
   953-        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + u"/backups"_s;
   ...
   964-    QVERIFY(res.safetyBackupPath.startsWith(customXdg));
   ```
   On Windows, `QStandardPaths::AppDataLocation` does not inspect `XDG_DATA_HOME` (it inspects `%LOCALAPPDATA%`), so `safetyBackupPath` will resolve under `C:\Users\runneradmin\AppData\Local` rather than `customXdg`, causing assertion failure at line 964.

3. **Window Activation Timeouts in UI Helpers (`tst_Page*`, `ui_smoke`):**
   ```bash
   $ grep -n -A10 "activateWindow(QWidget" tests/ui_test_helpers/UiTestHelpers.cpp
   108:bool activateWindow(QWidget* window) {
   109-    if (!window) return false;
   110-    if (!window->isVisible()) {
   111-        window->show();
   112-    }
   113-    window->activateWindow();
   114-    window->raise();
   115-    QCoreApplication::processEvents();
   116-    if (!window->isActiveWindow()) {
   117-        const bool active = QTest::qWaitForWindowActive(window, 1000);
   118-        Q_UNUSED(active);
   119-    }
   120-    return true;
   121-}
   ```
   In headless offscreen mode under Windows, `isActiveWindow()` may not transition to true, causing every invocation of `activateWindow()` (and every `sendKeyClick`) to incur a full 1000 ms wait. This aligns with the long UI runtimes observed on Windows (up to 97 s).

4. **Line Ending Splitting in `tst_ImportExport`:**
   ```bash
   $ grep -n "folded.split" tests/services/tst_ImportExport.cpp
   175:    const auto lines = folded.split('\n');
   ```
   Splitting an RFC 5545 CRLF stream by `\n` alone retains trailing `\r` characters on each line, which may cause line-length checks to miscount by 1 octet.

---

## 7. Commits for this Task

```bash
$ git log -n 4 --oneline
949b6c7 docs(reports): add M6 CI fix-up 3 report
418817b ci: add Windows test diagnostic step and enable stderr logging
007a550 docs(reports): mark window maximize verified by user in M6 Part D1 report
bdae926 docs(reports): update F5 with maximize log analysis and 720p resolution
```

---

## 8. Not Verified by Agent, User to Check

1. **Diagnostic Step Execution:** That the new diagnostic step runs on the GitHub Actions Windows runner when tests fail and prints individual QtTest failure logs and exit codes. (Can only be confirmed by pushing and observing the next CI run).
2. **PowerShell Script Parsing:** That the inline PowerShell script parses and executes without syntax errors on the Windows runner (since `pwsh` was not installed locally).
3. **Stderr Logging Environment Variables:** Whether `QT_FORCE_STDERR_LOGGING=1` and `QT_ASSUME_STDERR_HAS_CONSOLE=1` successfully route ctest output into GitHub Actions console streams.
4. **Cause of the 13 Windows Failures:** The true cause of each failure remains **unknown until the next CI run produces the detailed QtTest logs**.
