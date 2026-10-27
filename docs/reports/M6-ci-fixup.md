# Milestone 6 CI Fix-up Report

- **Date:** 2026-10-27
- **Status:** Complete (CI workflow fix-up in `.github/workflows/ci.yml`)
- **Document Version:** 1.0

---

## 1. Facts Used (Verified by User)

1. `aqtinstall==3.2.0` exists and installs cleanly under Python 3.11/3.14.
2. `aqt list-qt linux desktop --arch 6.8.2` prints `linux_gcc_64` (not `gcc_64`).
3. `aqt list-qt windows desktop --arch 6.8.2` offers `win64_msvc2022_64`.
4. Qt 6.8.2 base package includes `qtsvg`, `qtbase`, and `qtsql`; `-m qtsvg` was invalid and removed on both OS.
5. Action pins verified via git commits:
   - `actions/checkout` v4.2.2 = `11bd71901bbe5b1630ceea73d27597364c9af683`
   - `actions/setup-python` v5.4.0 = `42375524e23c412d93fb67b49958b491fce71c38`
   - `actions/cache` v4.2.2 = `d4323d4df104b026a6aa633fdb11d772146be0bf`
   - `actions/upload-artifact` v4.6.1 = `4cec3d8aa04e39d1a68397de0c4cd6fb9dce8ec1`
   - `ilammy/msvc-dev-cmd` v1.13.0 annotated tag points to commit `0b201ec74fa43914dc39ae48a89fd1d8cb592756` (replaces incorrect `cbc9c03b...`).

---

## 2. Action Pins (`uses:`) Before & After

### Before
```
$ grep -n "uses:" .github/workflows/ci.yml
37:        uses: actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683 # v4.2.2
40:        uses: actions/setup-python@42375524e23c412d93fb67b49958b491fce71c38 # v5.4.0
58:        uses: ilammy/msvc-dev-cmd@cbc9c03b73288a1da9438908613a776c21e6495e # v1.13.0
62:        uses: actions/cache@d4323d4df104b026a6aa633fdb11d772146be0bf # v4.2.2
93:        uses: actions/upload-artifact@4cec3d8aa04e39d1a68397de0c4cd6fb9dce8ec1 # v4.6.1
```

### After
```
$ grep -n "uses:" .github/workflows/ci.yml
35:        uses: actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683 # v4.2.2
38:        uses: actions/setup-python@42375524e23c412d93fb67b49958b491fce71c38 # v5.4.0
57:        uses: ilammy/msvc-dev-cmd@0b201ec74fa43914dc39ae48a89fd1d8cb592756 # v1.13.0
63:        uses: actions/cache@d4323d4df104b026a6aa633fdb11d772146be0bf # v4.2.2
155:        uses: actions/upload-artifact@4cec3d8aa04e39d1a68397de0c4cd6fb9dce8ec1 # v4.6.1
```

---

## 3. Task 4 Output: Platform Plugin Selection

```
$ grep -rn "QT_QPA_PLATFORM\|offscreen" CMakeLists.txt tests scripts 2>/dev/null
tests/models/CMakeLists.txt:19:        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
tests/ui_test_helpers/UiTestHelpers.h:17:/// offscreen platform plugin noise.
tests/ui_smoke/CMakeLists.txt:15:    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;JOBPREP_REPORT_IMG_DIR=${CMAKE_SOURCE_DIR}/docs/reports/img"
tests/ui_smoke/CMakeLists.txt:27:    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
tests/services/CMakeLists.txt:16:        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
tests/data/CMakeLists.txt:39:        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
tests/ui/CMakeLists.txt:8:    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
tests/ui/CMakeLists.txt:19:    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
tests/ui/CMakeLists.txt:37:        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;JOBPREP_REPORT_IMG_DIR=${CMAKE_SOURCE_DIR}/docs/reports/img"
tests/ui/tst_Notifiers.cpp:58:    // 4. TrayNotifier offscreen behavior
tests/ui/tst_Notifiers.cpp:406:        // Under offscreen platform without a system tray, notify() returns false per F5
tests/ui/tst_PageJobs.cpp:873:    // The context menu cannot be popped up offscreen, so trigger its action directly.
scripts/screenshots.ps1:17:$env:QT_QPA_PLATFORM = "offscreen"
scripts/screenshots.sh:16:JOBPREP_REPORT_IMG_DIR="${REPORT_IMG_DIR}" QT_QPA_PLATFORM=offscreen \
scripts/screenshots.sh:18:JOBPREP_REPORT_IMG_DIR="${REPORT_IMG_DIR}" QT_QPA_PLATFORM=offscreen \
scripts/screenshots.sh:20:JOBPREP_REPORT_IMG_DIR="${REPORT_IMG_DIR}" QT_QPA_PLATFORM=offscreen \
scripts/screenshots.sh:22:JOBPREP_REPORT_IMG_DIR="${REPORT_IMG_DIR}" QT_QPA_PLATFORM=offscreen \
```

Every test executable is configured with CTest property `ENVIRONMENT "QT_QPA_PLATFORM=offscreen"` across all test directories. To provide full defense-in-depth on CI runners where tests might be invoked directly, `QT_QPA_PLATFORM: offscreen` was also explicitly set in the workflow environment for both the Linux and Windows quality gate steps.

---

## 4. Task 6 Output: Windows Job, Presets & Scripts

### MSVC Environment & Ninja/MSVC Preset Integration
- MSVC environment is initialized via `ilammy/msvc-dev-cmd` with `arch: x64`.
- `scripts/check.ps1` invokes `cmake --preset dev-windows`, `cmake --build --preset dev-windows`, and `ctest --preset dev-windows -j $ctestParallel --output-on-failure`.
- `CMakePresets.json` defines preset `dev-windows` inheriting from `base`, which specifies:
  `"generator": "Ninja"` and `"CMAKE_PREFIX_PATH": "$penv{QT_PREFIX_PATH}"`.
- The workflow locates `Qt6Config.cmake` dynamically, computes `<prefix>` (three directory levels up from `lib/cmake/Qt6`), and writes `QT_PREFIX_PATH` to `$GITHUB_ENV` and `<prefix>\bin` to `$GITHUB_PATH`.

### `CMakePresets.json`
```
$ cat CMakePresets.json
{
  "version": 3,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 21,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "base",
      "hidden": true,
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
        "JOBPREP_WERROR": "ON",
        "CMAKE_PREFIX_PATH": "$penv{QT_PREFIX_PATH}"
      }
    },
    {
      "name": "dev-linux",
      "displayName": "Linux Development",
      "description": "Configure for Linux with Ninja and Qt 6",
      "inherits": "base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    },
    {
      "name": "dev-windows",
      "displayName": "Windows Development",
      "description": "Configure for Windows with Ninja/MSVC and Qt 6",
      "inherits": "base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "dev-linux",
      "displayName": "Build Linux Development",
      "configurePreset": "dev-linux"
    },
    {
      "name": "dev-windows",
      "displayName": "Build Windows Development",
      "configurePreset": "dev-windows"
    }
  ],
  "testPresets": [
    {
      "name": "dev-linux",
      "displayName": "Test Linux Development",
      "configurePreset": "dev-linux",
      "output": {
        "outputOnFailure": true
      },
      "execution": {
        "noTestsAction": "error"
      }
    },
    {
      "name": "dev-windows",
      "displayName": "Test Windows Development",
      "configurePreset": "dev-windows",
      "output": {
        "outputOnFailure": true
      },
      "execution": {
        "noTestsAction": "error"
      }
    }
  ]
}
```

### `scripts/check.ps1`
```
$ cat scripts/check.ps1
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
```

---

## 5. Task 8 Output: Static Verification & Diffs

### YAML Parser Check
```
$ python3 -c "import yaml,sys; yaml.safe_load(open('.github/workflows/ci.yml')); print('yaml ok')"
yaml ok
```

### Actionlint Check
```
$ which actionlint || echo "actionlint not found"
actionlint not found
```
*(Status: not run, actionlint not installed in environment).*

### Diffstat
```
$ git show --stat e4b8697
commit e4b8697efecec9f038bbd2dd949970992bd6a696 (HEAD -> main)
Author: Tony <group.ik31@gmail.com>
Date:   Tue Oct 27 14:20:27 2026 +0200

    ci: fix aqtinstall arch, remove -m qtsvg, pin msvc-dev-cmd and discover Qt prefix

 .github/workflows/ci.yml | 84 +++++++++++++++++++++++++++++++++++++++++-------
 1 file changed, 73 insertions(+), 11 deletions(-)
```

### Full Commit Diff
```
$ git diff 56a31d6 e4b8697
diff --git a/.github/workflows/ci.yml b/.github/workflows/ci.yml
index 145bb5e..eb269ed 100644
--- a/.github/workflows/ci.yml
+++ b/.github/workflows/ci.yml
@@ -17,20 +17,18 @@ jobs:
   gate:
     name: Gate (${{ matrix.os }})
     runs-on: ${{ matrix.os }}
-    timeout-minutes: 30
+    timeout-minutes: 45
 
     strategy:
       fail-fast: false
       matrix:
         include:
           - os: ubuntu-latest
-            qt-arch: gcc_64
+            qt-arch: linux_gcc_64
             qt-target: desktop
-            qt-dir: gcc_64
           - os: windows-latest
             qt-arch: win64_msvc2022_64
             qt-target: desktop
-            qt-dir: msvc2022_64
 
     steps:
       - name: Checkout repository
@@ -51,18 +49,21 @@ jobs:
             libfontconfig1 \
             libdbus-1-3 \
             libxkbcommon0 \
-            libxkbcommon-x11-0
+            libxkbcommon-x11-0 \
+            fonts-dejavu-core
 
       - name: Set up MSVC environment (Windows)
         if: runner.os == 'Windows'
-        uses: ilammy/msvc-dev-cmd@cbc9c03b73288a1da9438908613a776c21e6495e # v1.13.0
+        uses: ilammy/msvc-dev-cmd@0b201ec74fa43914dc39ae48a89fd1d8cb592756 # v1.13.0
+        with:
+          arch: x64
 
       - name: Cache Qt installation
         id: cache-qt
         uses: actions/cache@d4323d4df104b026a6aa633fdb11d772146be0bf # v4.2.2
         with:
           path: ${{ runner.workspace }}/Qt
-          key: qt-6.8.2-${{ matrix.os }}-${{ matrix.qt-arch }}
+          key: qt-6.8.2-${{ matrix.os }}-${{ matrix.qt-arch }}-aqt-3.2.0
 
       - name: Install Qt via aqtinstall
         if: steps.cache-qt.outputs.cache-hit != 'true'
@@ -70,21 +71,82 @@ jobs:
         run: |
           pip install aqtinstall==3.2.0
           python -m aqt install-qt ${{ runner.os == 'Windows' && 'windows' || 'linux' }} ${{ matrix.qt-target }} 6.8.2 ${{ matrix.qt-arch }} \
-            --outputdir "${{ runner.workspace }}/Qt" \
-            -m qtsvg
+            --outputdir "${{ runner.workspace }}/Qt"
+
+      - name: Locate Qt prefix (Linux)
+        if: runner.os == 'Linux'
+        shell: bash
+        run: |
+          CONFIG_FILE=$(find "${RUNNER_WORKSPACE}/Qt" -type f -name Qt6Config.cmake -path "*/lib/cmake/Qt6/Qt6Config.cmake" | head -n 1)
+          if [ -z "${CONFIG_FILE}" ]; then
+            CONFIG_FILE=$(find "${RUNNER_WORKSPACE}/Qt" -type f -name Qt6Config.cmake | head -n 1)
+          fi
+          if [ -z "${CONFIG_FILE}" ]; then
+            echo "::error::Qt6Config.cmake not found under ${RUNNER_WORKSPACE}/Qt" >&2
+            exit 1
+          fi
+          QT_PREFIX=$(cd "$(dirname "${CONFIG_FILE}")/../../.." && pwd)
+          echo "Found QT_PREFIX_PATH: ${QT_PREFIX}"
+          echo "QT_PREFIX_PATH=${QT_PREFIX}" >> "${GITHUB_ENV}"
+
+      - name: Locate Qt prefix and set PATH (Windows)
+        if: runner.os == 'Windows'
+        shell: pwsh
+        run: |
+          $configFile = Get-ChildItem -Path "$env:RUNNER_WORKSPACE\Qt" -Filter Qt6Config.cmake -Recurse | Select-Object -First 1
+          if (-not $configFile) {
+            Write-Error "Qt6Config.cmake not found under $env:RUNNER_WORKSPACE\Qt"
+            exit 1
+          }
+          $qtPrefix = $configFile.Directory.Parent.Parent.Parent.FullName
+          Write-Host "Found QT_PREFIX_PATH: $qtPrefix"
+          "QT_PREFIX_PATH=$qtPrefix" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
+          "$qtPrefix\bin" | Out-File -FilePath $env:GITHUB_PATH -Append -Encoding utf8
+
+      - name: Diagnostic information (Linux)
+        if: runner.os == 'Linux'
+        shell: bash
+        run: |
+          echo "=== Tools & Compiler ==="
+          cmake --version
+          ninja --version
+          g++ --version | head -1
+          echo "=== Qt Environment ==="
+          echo "QT_PREFIX_PATH: ${QT_PREFIX_PATH}"
+          echo "--- platforms ---"
+          ls -la "${QT_PREFIX_PATH}/plugins/platforms" || true
+          echo "--- sqldrivers ---"
+          ls -la "${QT_PREFIX_PATH}/plugins/sqldrivers" || true
+
+      - name: Diagnostic information (Windows)
+        if: runner.os == 'Windows'
+        shell: pwsh
+        run: |
+          Write-Host "=== Tools & Compiler ==="
+          cmake --version
+          ninja --version
+          & cl.exe 2>&1 | Select-Object -First 1
+          Write-Host "=== Qt Environment ==="
+          Write-Host "QT_PREFIX_PATH: $env:QT_PREFIX_PATH"
+          Write-Host "--- where Qt6Core.dll ---"
+          where.exe Qt6Core.dll
+          Write-Host "--- platforms ---"
+          Get-ChildItem -Path "$env:QT_PREFIX_PATH\plugins\platforms" | Select-Object Name
+          Write-Host "--- sqldrivers ---"
+          Get-ChildItem -Path "$env:QT_PREFIX_PATH\plugins\sqldrivers" | Select-Object Name
 
       - name: Run quality gate (Linux)
         if: runner.os == 'Linux'
         env:
-          QT_PREFIX_PATH: ${{ runner.workspace }}/Qt/6.8.2/${{ matrix.qt-dir }}
           CMAKE_BUILD_PARALLEL_LEVEL: 2
+          QT_QPA_PLATFORM: offscreen
         run: ./scripts/check.sh
 
       - name: Run quality gate (Windows)
         if: runner.os == 'Windows'
         env:
-          QT_PREFIX_PATH: ${{ runner.workspace }}\Qt\6.8.2\${{ matrix.qt-dir }}
           CMAKE_BUILD_PARALLEL_LEVEL: 2
+          QT_QPA_PLATFORM: offscreen
         shell: pwsh
         run: ./scripts/check.ps1
```

---

## 6. Complete Final Workflow File (`cat .github/workflows/ci.yml`)

```
$ cat .github/workflows/ci.yml
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
        run: ./scripts/check.sh

      - name: Run quality gate (Windows)
        if: runner.os == 'Windows'
        env:
          CMAKE_BUILD_PARALLEL_LEVEL: 2
          QT_QPA_PLATFORM: offscreen
        shell: pwsh
        run: ./scripts/check.ps1

      - name: Upload test logs on failure
        if: failure()
        uses: actions/upload-artifact@4cec3d8aa04e39d1a68397de0c4cd6fb9dce8ec1 # v4.6.1
        with:
          name: test-logs-${{ matrix.os }}
          path: |
            build/*/Testing/Temporary/LastTest.log
            build/*/Testing/Temporary/*.log
```

---

## 7. Git Commit History

```
$ git log -n 5 --oneline
e4b8697 (HEAD -> main) ci: fix aqtinstall arch, remove -m qtsvg, pin msvc-dev-cmd and discover Qt prefix
56a31d6 ci fixup prompt
133c302 docs(reports): add M6 Part B report
e2730a5 feat(settings): implement settings page behavior and close-to-tray
03be7f9 test(smoke): assert page identity for navigation shortcuts and default ctest to parallel
```

---

## 8. Honesty & Risk Assessment

### Verification Limitations
As an agent operating in an offline/local container without GitHub credentials or external network access to GitHub Actions infrastructure, **nothing in this workflow has been executed on live GitHub runners**. All fixes are strictly based on the user's verified facts and static verification (YAML parsing, presets analysis, and script inspections).

### Remaining Failure Points on First GitHub Actions Run
1. **Linux Runner Shared Library Dependencies:** While `libgl1-mesa-dev`, `libfontconfig1`, `libdbus-1-3`, `libxkbcommon0`, `libxkbcommon-x11-0`, and `fonts-dejavu-core` are installed, official Qt 6.8.2 binaries built for `linux_gcc_64` dynamically link against system libraries (e.g. `libEGL`, `libglib-2.0`, or X11/xcb libraries). If a missing `.so` causes `libqoffscreen.so` or `libqsqlite.so` to fail to load, the diagnostic step will show the plugin listing and the test log will report the missing dependency for addition in a subsequent fix-up.
2. **Windows Runner Ninja Availability:** While CMake and MSVC are standard on `windows-latest`, `ninja` is expected to be pre-installed on the runner image. If `ninja.exe` is missing from `PATH`, `cmake --version` will pass but `ninja --version` will fail.
3. **Download Bandwidth / aqt Mirror Throttling:** Downloading Qt 6.8.2 on first run may encounter download throttling from Qt mirrors; `timeout-minutes: 45` was provided to mitigate timeout risks on cold cache.

---

## 9. Not Verified by Agent, User to Check

1. **First GitHub Actions Run:** Push local commits to trigger workflow execution and observe step execution on both `ubuntu-latest` and `windows-latest`.
2. **Qt Cache Hit on Second Run:** Re-trigger or run a second commit to verify that `actions/cache` restores `${{ runner.workspace }}/Qt` without re-downloading via `aqtinstall`.
3. **Windows DLL Discovery:** Confirm via the diagnostic step (`where.exe Qt6Core.dll`) and test runs that tests locate Qt runtime libraries on Windows without missing DLL errors.
4. **Font Availability for Painted Tests:** Verify that `fonts-dejavu-core` satisfies offscreen font requirements during visual test execution (`tst_PageSettings`, `tst_PageStudy`, etc.).
