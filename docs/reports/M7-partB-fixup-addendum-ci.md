# M7 Part B Fix-Up Addendum: First CI Run of Part B (Windows Compile Error, Ubuntu Apt Outage)

## 1. Facts (GitHub Actions Run, Three Jobs, Commits of Part B; Times UTC, 2026-11-11)
- **Windows (Qt 6.10.3, MSVC 14.51.36231):** build failed at `FAILED: [code=2] tests/ui/CMakeFiles/tst_Shortcuts.dir/tst_Shortcuts.cpp.obj`:
  ```
  D:\a\JobPrep\JobPrep\tests\ui\tst_Shortcuts.cpp(823): error C2220: the following warning is treated as an error
  D:\a\JobPrep\JobPrep\tests\ui\tst_Shortcuts.cpp(823): warning C4456: declaration of 'btn' hides previous local declaration
  D:\a\JobPrep\JobPrep\tests\ui\tst_Shortcuts.cpp(821): note: see declaration of 'btn'
  ```
  Only one `FAILED:` line existed in the log (Ninja stopped at the first failure; later files were not compiled to the end).
- **Ubuntu (Qt 6.8.2 and Qt 6.10.3):** both jobs failed in the `apt-get update` / `apt-get install` step before any build could start: `Temporary failure resolving 'security.ubuntu.com'`, many `Ign:` and `Err:` lines for `archive.ubuntu.com`/`azure.archive.ubuntu.com`/`security.ubuntu.com`, then `E: Unable to fetch some archives` and `##[error]Process completed with exit code 100`. The step ran about 19 minutes (15:50 to 16:09). This is a network outage of the runner, not JobPrep code. Nothing of Part B was compiled or tested on Linux CI in that run.

---

## 2. Task 1: Fix the Shadowing in `tests/ui/tst_Shortcuts.cpp`

### Command: `sed -n '810,830p' tests/ui/tst_Shortcuts.cpp`
```
        const QString tip = dateEdit->toolTip();
        QVERIFY2(tip.contains(JobPrep::Ui::Widgets::SingleTabDateEdit::tr("Press Alt+Down or F4 to open calendar")),
                 qPrintable(QString(u"Tooltip missing shortcut hint: '%1'"_s).arg(tip)));
        if (controlName == u"InterviewDateEdit"_s) {
            QVERIFY2(tip.contains(u"Interview date"_s),
                     qPrintable(QString(u"Original tooltip was overwritten: '%1'"_s).arg(tip)));
        }

        // 6. Second Esc closes dialog (if it is a QDialog)
        if (dialogPtr) {
            answerNextMessageBox([](QMessageBox* box) {
                if (auto* discardBtn = box->button(QMessageBox::Discard)) {
                    discardBtn->click();
                } else if (auto* yesBtn = box->button(QMessageBox::Yes)) {
                    yesBtn->click();
                }
            });
            QTest::keyClick(dateEdit, Qt::Key_Escape);
            QCoreApplication::processEvents();
            QVERIFY2(!dialogPtr->isVisible(),
                     "Second Esc failed to reject/close the parent dialog");
```

### Explanation of Change
Renamed the inner variables in `tests/ui/tst_Shortcuts.cpp` from `btn` to `discardBtn` and `yesBtn`. The outer variable is not hidden, eliminating MSVC `C4456`.

### Command: `ctest --preset dev-linux -R tst_Shortcuts --output-on-failure`
```
Test project /home/tony/projects/JobPrep/build/dev-linux
    Start 32: tst_Shortcuts
1/1 Test #32: tst_Shortcuts ....................   Passed    5.28 sec

100% tests passed, 0 tests failed out of 1

Label Time Summary:
ui    =   5.28 sec*proc (1 test)

Total Test time (real) =   5.29 sec
```

---

## 3. Task 2: Find the Rest Before CI Does

### Command: `git diff --name-only 0f4b9b2..HEAD -- '*.cpp' '*.h'`
```
src/ui/delegates/AgendaItemDelegate.cpp
src/ui/delegates/AgendaItemDelegate.h
src/ui/pages/CalendarPage.cpp
src/ui/pages/HomePage.cpp
src/ui/widgets/SingleTabDateTime.h
src/ui/widgets/TopicDetailPanel.cpp
tests/ui/tst_HiDpi.cpp
tests/ui/tst_PageHome.cpp
tests/ui/tst_PageStudy.cpp
tests/ui/tst_Shortcuts.cpp
tests/ui_test_helpers/UiTestHelpers.cpp
```

### Compiling Part B C++ Files with GCC and `-Wshadow`
Command:
```bash
python3 -c "
import json, subprocess
with open('build/dev-linux/compile_commands.json') as f:
    cmds = json.load(f)
part_b_files = subprocess.check_output(['git', 'diff', '--name-only', '0f4b9b2..HEAD', '--', '*.cpp']).decode().splitlines()
for f in part_b_files:
    match = [c for c in cmds if c['file'].endswith(f)]
    if not match: continue
    cmd = match[0]['command']
    if '-Wshadow' not in cmd: cmd = cmd + ' -Wshadow'
    print(f'=== Compiling {f} with -Wshadow ===')
    res = subprocess.run(cmd, shell=True, cwd=match[0]['directory'], capture_output=True, text=True)
    out = (res.stdout + res.stderr).strip()
    print(out if out else '(no output)')
"
```
Raw output:
```
=== Compiling src/ui/delegates/AgendaItemDelegate.cpp with -Wshadow ===
(no output)
=== Compiling src/ui/pages/CalendarPage.cpp with -Wshadow ===
(no output)
=== Compiling src/ui/pages/HomePage.cpp with -Wshadow ===
(no output)
=== Compiling src/ui/widgets/TopicDetailPanel.cpp with -Wshadow ===
(no output)
=== Compiling tests/ui/tst_HiDpi.cpp with -Wshadow ===
(no output)
=== Compiling tests/ui/tst_PageHome.cpp with -Wshadow ===
(no output)
=== Compiling tests/ui/tst_PageStudy.cpp with -Wshadow ===
(no output)
=== Compiling tests/ui/tst_Shortcuts.cpp with -Wshadow ===
(no output)
=== Compiling tests/ui_test_helpers/UiTestHelpers.cpp with -Wshadow ===
(no output)
```

### Whole Project Compilation with `-Wshadow` in `/tmp`
Command:
```bash
cmake -B /tmp/jobprep-shadow -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DJOBPREP_WERROR=OFF -DCMAKE_CXX_FLAGS="-Wshadow" && ninja -C /tmp/jobprep-shadow -j 2
```
Command output: all 227 build steps compiled and linked with exit code 0.

Count of `-Wshadow` warnings across the whole project:
```bash
grep -c "Wshadow" /home/tony/.gemini/antigravity-cli/brain/543c608e-af3d-42f5-9ecc-e3e4ad4ec9f0/.system_generated/steps/174/output.txt
```
Raw output:
```
0
```

### Warning Flag Decision
The count of `-Wshadow` warnings outside Part B files is 0. With `tst_Shortcuts.cpp` resolved, `-Wshadow` was permanently enabled for GCC and Clang in `jobprep_apply_warnings()` in `CMakeLists.txt` alongside `-Wall -Wextra -Wpedantic`.

### MSVC-only `/W4` Warning Classes Analysis
Other MSVC `/W4` warning classes that can potentially trigger under `/WX` include:
- `C4189`: local variable initialized but not referenced
- `C4100`: unreferenced formal parameter
- `C4267` / `C4244`: conversion from size_t to smaller integer type (narrowing)
- `C4702`: unreachable code

To check these on GCC across Part B translation units, `-Wunused -Wconversion` was run over all Part B files:
Command:
```bash
python3 -c "
import json, subprocess
with open('build/dev-linux/compile_commands.json') as f:
    cmds = json.load(f)
part_b_files = subprocess.check_output(['git', 'diff', '--name-only', '0f4b9b2..HEAD', '--', '*.cpp']).decode().splitlines()
for f in part_b_files:
    match = [c for c in cmds if c['file'].endswith(f)]
    if not match: continue
    cmd = match[0]['command']
    cmd = cmd + ' -Wunused -Wconversion'
    print(f'=== Compiling {f} with -Wunused -Wconversion ===')
    res = subprocess.run(cmd, shell=True, cwd=match[0]['directory'], capture_output=True, text=True)
    out = (res.stdout + res.stderr).strip()
    print(out if out else '(no output)')
"
```
Raw output:
```
=== Compiling src/ui/delegates/AgendaItemDelegate.cpp with -Wunused -Wconversion ===
(no output)
=== Compiling src/ui/pages/CalendarPage.cpp with -Wunused -Wconversion ===
(no output)
=== Compiling src/ui/pages/HomePage.cpp with -Wunused -Wconversion ===
(no output)
=== Compiling src/ui/widgets/TopicDetailPanel.cpp with -Wunused -Wconversion ===
(no output)
=== Compiling tests/ui/tst_HiDpi.cpp with -Wunused -Wconversion ===
(no output)
=== Compiling tests/ui/tst_PageHome.cpp with -Wunused -Wconversion ===
(no output)
=== Compiling tests/ui/tst_PageStudy.cpp with -Wunused -Wconversion ===
(no output)
=== Compiling tests/ui/tst_Shortcuts.cpp with -Wunused -Wconversion ===
(no output)
=== Compiling tests/ui_test_helpers/UiTestHelpers.cpp with -Wunused -Wconversion ===
(no output)
```

---

## 4. Task 3: Make the Apt Step Robust

### Command: `grep -n "apt-get" .github/workflows/ci.yml`
```
55:          sudo apt-get -o Acquire::Retries=5 -o Acquire::http::Timeout=20 -o Acquire::https::Timeout=20 update
56:          sudo apt-get -o Acquire::Retries=5 -o Acquire::http::Timeout=20 -o Acquire::https::Timeout=20 install -y --no-install-recommends \
```

### Command: `git diff .github/workflows/ci.yml`
```
diff --git a/.github/workflows/ci.yml b/.github/workflows/ci.yml
index 248dc7c..5acb615 100644
--- a/.github/workflows/ci.yml
+++ b/.github/workflows/ci.yml
@@ -50,9 +50,10 @@ jobs:
 
       - name: Install Ninja and system libraries (Linux)
         if: runner.os == 'Linux'
+        timeout-minutes: 10
         run: |
-          sudo apt-get update
-          sudo apt-get install -y --no-install-recommends \
+          sudo apt-get -o Acquire::Retries=5 -o Acquire::http::Timeout=20 -o Acquire::https::Timeout=20 update
+          sudo apt-get -o Acquire::Retries=5 -o Acquire::http::Timeout=20 -o Acquire::https::Timeout=20 install -y --no-install-recommends \
             ninja-build \
             libgl1-mesa-dev \
             libfontconfig1 \
```

### Command: `python3 -c "import yaml; yaml.safe_load(open('.github/workflows/ci.yml')) and print('YAML parse: OK')"`
```
YAML parse: OK
```

*Note:* Retries and timeouts cannot fix an external DNS outage; they only guarantee the step fails within 10 minutes instead of hanging for 19 minutes, allowing the user to re-run the job once network connectivity is restored.

---

## 5. Labelled Prediction for Next CI Run
- **Windows (Qt 6.10.3, MSVC 14.51):** The C4456 variable shadowing in `tst_Shortcuts.cpp` is resolved. However, MSVC `/W4` with `/WX` could potentially flag other MSVC-specific diagnostic classes in files not reached in the first run after Ninja aborted at `tst_Shortcuts.cpp`.
- **Ubuntu (Qt 6.8.2 and Qt 6.10.3):** Job success depends on GitHub Actions runner network resolution for `security.ubuntu.com` and `archive.ubuntu.com`. If network resolution succeeds, all tests and compilation should pass with zero warnings (as verified locally on Linux GCC 15 with `-Wshadow -Werror`).

---

## 6. Not Verified by Agent, User to Check
- The next GitHub Actions CI run on all three jobs (Windows Qt 6.10.3, Ubuntu Qt 6.8.2, Ubuntu Qt 6.10.3).
- In particular, the first CI execution of the new date-popup keyboard shortcut test under native Windows styles (`windowsvista`, `windows11`).

---

## 7. Verification: `scripts/check.sh`

### Command: `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh`
```
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.2s)
-- Generating done (0.6s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
[1/46] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/delegates/AgendaItemDelegate.cpp.o
[2/46] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/widgets/TopicDetailPanel.cpp.o
[3/46] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/pages/HomePage.cpp.o
[4/46] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/pages/CalendarPage.cpp.o
[5/46] Linking CXX static library libjobprep_core.a
[6/46] Linking CXX executable jobprep
[7/46] Linking CXX executable tests/data/tst_Database
[8/46] Linking CXX executable tests/data/tst_TrackRepository
[9/46] Linking CXX executable tests/data/tst_TopicRepository
[10/46] Linking CXX executable tests/data/tst_SessionRepository
[11/46] Linking CXX executable tests/data/tst_ApplicationRepository
[12/46] Building CXX object tests/ui_test_helpers/CMakeFiles/jobprep_ui_test_helpers.dir/UiTestHelpers.cpp.o
[13/46] Linking CXX static library tests/ui_test_helpers/libjobprep_ui_test_helpers.a
[14/46] Linking CXX executable tests/data/tst_InterviewRepository
[15/46] Building CXX object tests/ui/CMakeFiles/tst_PageStudy.dir/tst_PageStudy.cpp.o
[16/46] Building CXX object tests/ui/CMakeFiles/tst_PageHome.dir/tst_PageHome.cpp.o
[17/46] Building CXX object tests/ui/CMakeFiles/tst_Shortcuts.dir/tst_Shortcuts.cpp.o
[18/46] Building CXX object tests/ui/CMakeFiles/tst_HiDpi.dir/tst_HiDpi.cpp.o
[19/46] Linking CXX executable tests/data/tst_ReminderLogRepository
[20/46] Linking CXX executable tests/data/tst_SeedService
[21/46] Linking CXX executable tests/data/tst_TransactionBehavior
[22/46] Linking CXX executable tests/ui_smoke/ui_smoke
[23/46] Linking CXX executable tests/ui_smoke/tst_MonthView
[24/46] Linking CXX executable tests/models/tst_AgendaListModel
[25/46] Linking CXX executable tests/models/tst_ApplicationTableModel
[26/46] Linking CXX executable tests/models/tst_ApplicationFilterProxy
[27/46] Linking CXX executable tests/models/tst_TopicListModel
[28/46] Linking CXX executable tests/models/tst_TopicBoardProxy
[29/46] Linking CXX executable tests/models/tst_TopicTableProxy
[30/46] Linking CXX executable tests/services/tst_AgendaService
[31/46] Linking CXX executable tests/services/tst_ExportService
[32/46] Linking CXX executable tests/services/tst_DayWatcher
[33/46] Linking CXX executable tests/services/tst_ImportExport
[34/46] Linking CXX executable tests/services/tst_ReminderService
[35/46] Linking CXX executable tests/services/tst_SettingsService
[36/46] Linking CXX executable tests/services/tst_StatsService
[37/46] Linking CXX executable tests/ui/tst_Notifiers
[38/46] Linking CXX executable tests/ui/tst_StudyRegression
[39/46] Linking CXX executable tests/ui/tst_PageJobs
[40/46] Linking CXX executable tests/ui/tst_PageCalendar
[41/46] Linking CXX executable tests/ui/tst_PageStudy
[42/46] Linking CXX executable tests/ui/tst_PageHome
[43/46] Linking CXX executable tests/ui/tst_PageSettings
[44/46] Linking CXX executable tests/ui/tst_Shortcuts
[45/46] Linking CXX executable tests/ui/tst_EmptyStatesAndFocus
[46/46] Linking CXX executable tests/ui/tst_HiDpi
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start 34: tst_HiDpi
      Start 29: tst_PageStudy
 1/34 Test #34: tst_HiDpi ........................   Passed    9.55 sec
      Start 30: tst_PageHome
 2/34 Test #29: tst_PageStudy ....................   Passed   20.07 sec
      Start 33: tst_EmptyStatesAndFocus
 3/34 Test #30: tst_PageHome .....................   Passed   17.97 sec
      Start 27: tst_PageJobs
 4/34 Test #33: tst_EmptyStatesAndFocus ..........   Passed   11.30 sec
      Start 28: tst_PageCalendar
 5/34 Test #28: tst_PageCalendar .................   Passed    7.54 sec
      Start 26: tst_StudyRegression
 6/34 Test #27: tst_PageJobs .....................   Passed   13.44 sec
      Start 31: tst_PageSettings
 7/34 Test #31: tst_PageSettings .................   Passed    4.82 sec
      Start 10: ui_smoke
 8/34 Test #26: tst_StudyRegression ..............   Passed    7.80 sec
      Start 25: tst_Notifiers
 9/34 Test #25: tst_Notifiers ....................   Passed    2.88 sec
      Start 32: tst_Shortcuts
10/34 Test #10: ui_smoke .........................   Passed    6.76 sec
      Start 21: tst_ImportExport
11/34 Test #21: tst_ImportExport .................   Passed    0.87 sec
      Start 24: tst_StatsService
12/34 Test #24: tst_StatsService .................   Passed    1.12 sec
      Start  8: tst_SeedService
13/34 Test  #8: tst_SeedService ..................   Passed    0.34 sec
      Start 22: tst_ReminderService
14/34 Test #22: tst_ReminderService ..............   Passed    0.09 sec
      Start 17: tst_TopicTableProxy
15/34 Test #17: tst_TopicTableProxy ..............   Passed    0.08 sec
      Start 11: tst_MonthView
16/34 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 18: tst_AgendaService
17/34 Test #18: tst_AgendaService ................   Passed    0.10 sec
      Start  5: tst_ApplicationRepository
18/34 Test  #5: tst_ApplicationRepository ........   Passed    0.09 sec
      Start 20: tst_ExportService
19/34 Test #20: tst_ExportService ................   Passed    0.01 sec
      Start  3: tst_TopicRepository
20/34 Test  #3: tst_TopicRepository ..............   Passed    0.09 sec
      Start 16: tst_TopicBoardProxy
21/34 Test #16: tst_TopicBoardProxy ..............   Passed    0.06 sec
      Start 13: tst_ApplicationTableModel
22/34 Test #13: tst_ApplicationTableModel ........   Passed    0.07 sec
      Start 15: tst_TopicListModel
23/34 Test #15: tst_TopicListModel ...............   Passed    0.04 sec
      Start 12: tst_AgendaListModel
24/34 Test #12: tst_AgendaListModel ..............   Passed    0.04 sec
      Start  6: tst_InterviewRepository
25/34 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start  1: tst_Database
26/34 Test  #1: tst_Database .....................   Passed    0.04 sec
      Start 23: tst_SettingsService
27/34 Test #23: tst_SettingsService ..............   Passed    0.02 sec
      Start  9: tst_TransactionBehavior
28/34 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start  4: tst_SessionRepository
29/34 Test #32: tst_Shortcuts ....................   Passed    6.20 sec
      Start 19: tst_DayWatcher
30/34 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  2: tst_TrackRepository
31/34 Test #19: tst_DayWatcher ...................   Passed    0.02 sec
      Start  7: tst_ReminderLogRepository
32/34 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start 14: tst_ApplicationFilterProxy
33/34 Test  #7: tst_ReminderLogRepository ........   Passed    0.03 sec
34/34 Test #14: tst_ApplicationFilterProxy .......   Passed    0.02 sec

100% tests passed, 0 tests failed out of 34

Label Time Summary:
data       =   0.72 sec*proc (9 tests)
model      =   0.32 sec*proc (6 tests)
service    =   2.22 sec*proc (7 tests)
ui         = 108.42 sec*proc (12 tests)

Total Test time (real) =  55.87 sec
==> Quality check PASSED with zero warnings and all tests green!
```

---

## 8. Totals by Command

### Command:
```bash
python3 -c '
import subprocess, glob, os
binaries = []
for root, dirs, files in os.walk("build/dev-linux/tests"):
    for f in files:
        p = os.path.join(root, f)
        if os.access(p, os.X_OK) and not f.endswith(".sh") and not f.endswith(".py") and not f.endswith(".a") and not f.endswith(".so") and not "." in f:
            binaries.append(p)
binaries.sort()
total_funcs = 0
for b in binaries:
    name = os.path.basename(b)
    try:
        out = subprocess.check_output([b, "-functions"], stderr=subprocess.STDOUT, text=True)
        lines = [l.strip() for l in out.splitlines() if l.strip() and not l.startswith("*") and not l.startswith("Config:")]
        funcs = [l for l in lines if "(" in l and not l.startswith("initTestCase") and not l.startswith("cleanupTestCase") and not l.startswith("init()") and not l.startswith("cleanup()")]
        print(f"{name}: {len(funcs)}")
        total_funcs += len(funcs)
    except Exception as e:
        pass
print(f"Total test binaries: {len(binaries)}")
print(f"Total test functions: {total_funcs}")
'
```
Raw output:
```
tst_ApplicationRepository: 20
tst_Database: 16
tst_InterviewRepository: 9
tst_ReminderLogRepository: 7
tst_SeedService: 5
tst_SessionRepository: 10
tst_TopicRepository: 18
tst_TrackRepository: 8
tst_TransactionBehavior: 8
tst_AgendaListModel: 5
tst_ApplicationFilterProxy: 2
tst_ApplicationTableModel: 10
tst_TopicBoardProxy: 9
tst_TopicListModel: 7
tst_TopicTableProxy: 6
tst_AgendaService: 24
tst_DayWatcher: 4
tst_ExportService: 4
tst_ImportExport: 33
tst_ReminderService: 18
tst_SettingsService: 8
tst_StatsService: 10
tst_EmptyStatesAndFocus: 34
tst_HiDpi: 6
tst_Notifiers: 21
tst_PageCalendar: 12
tst_PageHome: 16
tst_PageJobs: 22
tst_PageSettings: 44
tst_PageStudy: 27
tst_Shortcuts: 14
tst_StudyRegression: 12
tst_MonthView: 13
ui_smoke: 11
Total test binaries: 34
Total test functions: 473
```

---

## 9. Git State

### Command: `git status --short`
```
?? docs/prompts/M7-partB-fixup-addendum-ci.md
?? docs/reports/M7-partB-fixup-addendum-ci.md
```

### Command: `git log --oneline -n 5`
```
1b39195 (HEAD -> main) docs(reports): append CI run of Part B section to M7-partB report
333ef36 ci: add timeouts and retries to apt step on Linux
9692058 build(cmake): enable -Wshadow for GCC and Clang
5dab0de test(ui): fix shadowed local variable in tst_Shortcuts date popup test
dc71d4f (origin/main, origin/HEAD) m7-partB prompt
```
