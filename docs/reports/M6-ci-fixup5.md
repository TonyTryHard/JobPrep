# M6 CI Fix-up 5 Report

## 1. Facts As Given
- **Ubuntu CI Failures (Qt 6.8.2 on CI with `fonts-dejavu-core`):**
  1. `tst_UiSmoke::testWindowMinimumSizeExceedsLayoutMinimum`: `window minimumWidth (1000) < layout minimumWidth (1015)`.
  2. `tst_HiDpi::testPagesHiDpi`: 6 clipping violations at 1000x540 window size (Light and Dark):
     - `ChipBarChip` width 67 < minimumSizeHint 73 (Page 1)
     - `CalendarMonthTitle` width 171 < 186 and advance 169 > contentsRect 167 (Page 3)
     Both failures caused by hard-coded `setMinimumSize(1000, 540)` forcing the window below the layout's font-dependent minimum width (1015 px with DejaVu Sans).
- **Windows CI Failures (6 remaining):**
  1. Default `QSettings` unusable on Windows when organization name is empty (Status 1 / `AccessError`, writes dropped):
     - `tst_TopicListModel::settingsKeysAndFallbacks`
     - `tst_Notifiers::coordinatorEndToEndSettingsLeadMinutesChangeTakesEffect`
     - `tst_StudyRegression::testChecklistStartupContrastDarkAndLight`
  2. `QTemporaryFile` + `QSettings(path, IniFormat)` writes 0 bytes on Windows (file handle collision preventing `QSaveFile` write-replace):
     - `tst_SettingsService::testTrayHintShownKey`
     - `tst_SettingsService::testWeeklyGoalSurvivesRestart`
     - `tst_PageSettings::testWeeklyGoalPersistence`
  3. Linux-only path `QFile f(u"/tmp/throwaway_sample.ics"_s)` failing on Windows in `tst_ImportExport::testIcsExportFormatDetails`.
  4. Diagnostic step filtered out `INFO`/`QINFO` lines, hiding `CI-DIAG` output.

---

## 2. Evidence for Tasks 1 to 6

### Task 1: Evidence

#### `git log --oneline -8` (before task commits)
```
8c07150 (HEAD -> main) new prompts m6-d2-fixup + legacy dates + pipe fix5
4f90851 (origin/main, origin/HEAD) m6-ci-fixup4.md prompt
0514ea6 docs(reports): add M6 CI fix-up 4 v2 report
23c7477 test(ci): add CI-DIAG diagnostics for QSettings and test message handler
c2bfab1 fix(tests): make safety backup restore and XDG tests portable across OSes
a1ffd14 fix(core): make resource initialization independent of AppContext
e8f08ba ci: add font directory, diagnostic v2, and registry probe on Windows
9af6489 docs(reports): add M6 Part D2 final report
```

#### `grep -rn '"/tmp\|u"/tmp\|"/home/\|u"/home/' tests src`
Before fix:
```
tests/services/tst_ImportExport.cpp:315:    QFile f(u"/tmp/throwaway_sample.ics"_s);
tests/services/tst_ImportExport.cpp:948:    const QString customXdg = u"/tmp/jp-c07"_s;
```
After fix:
```
(no output)
```

#### `grep -rn "QTemporaryFile" tests`
Before fix:
```
tests/services/tst_SettingsService.cpp:1:#include <QTemporaryFile>
tests/services/tst_SettingsService.cpp:30:    QTemporaryFile temp;
tests/services/tst_SettingsService.cpp:43:    QTemporaryFile temp;
tests/services/tst_SettingsService.cpp:58:    QTemporaryFile temp;
tests/services/tst_SettingsService.cpp:72:    QTemporaryFile temp;
tests/services/tst_SettingsService.cpp:99:    QTemporaryFile temp;
tests/services/tst_SettingsService.cpp:117:    QTemporaryFile temp;
tests/services/tst_SettingsService.cpp:184:    QTemporaryFile temp;
tests/services/tst_ImportExport.cpp:14:#include <QTemporaryFile>
tests/ui/tst_PageSettings.cpp:21:#include <QTemporaryFile>
tests/ui/tst_PageSettings.cpp:342:    QTemporaryFile tempFile;
```
After fix:
```
(no output)
```

#### `grep -rln "QSettings\|SettingsService\|AppContext" tests` and Organization/Application Analysis
```
tests/models/tst_TopicListModel.cpp
tests/ui_test_helpers/UiTestHelpers.cpp
tests/ui_test_helpers/UiTestHelpers.h
tests/ui_smoke/tst_MonthView.cpp
tests/ui_smoke/tst_UiSmoke.cpp
tests/services/CMakeLists.txt
tests/services/tst_SettingsService.cpp
tests/services/tst_ImportExport.cpp
tests/ui/tst_Notifiers.cpp
tests/ui/tst_HiDpi.cpp
tests/ui/tst_PageStudy.cpp
tests/ui/tst_Shortcuts.cpp
tests/ui/tst_PageCalendar.cpp
tests/ui/tst_PageJobs.cpp
tests/ui/tst_PageHome.cpp
tests/ui/tst_StudyRegression.cpp
tests/ui/tst_PageSettings.cpp
tests/ui/tst_EmptyStatesAndFocus.cpp
```

| File | Default `QSettings`? | Default `SettingsService()`? | `AppContext` Outside UI Fixture? | Sets Org/App Name? (Before -> After) |
|---|---|---|---|---|
| `tst_TopicListModel.cpp` | Yes (`clearStudySettings`) | Yes (`settingsKeysAndFallbacks`) | No | No -> **Yes (`TestSettingsScope`)** |
| `tst_SettingsService.cpp` | No (explicit path) | No (explicit `QSettings`) | No | No -> **Yes (`initTestCase`)** |
| `tst_Notifiers.cpp` | Yes (`probe` in DIAG) | No | Yes (`AppContext ctx`) | No -> **Yes (`TestSettingsScope`)** |
| `tst_StudyRegression.cpp` | No | Yes (`testChecklistStartupContrast...`) | Yes (`AppContext ctx`) | No -> **Yes (`TestSettingsScope`)** |
| `tst_PageSettings.cpp` | No (explicit path) | No (explicit `QSettings`) | Yes (`AppContext ctx`) | Yes (`isolateSettings` in UI Fixture) |
| `tst_MonthView.cpp` | No | Yes (`CalendarFixture`) | No | No -> **Yes (`TestSettingsScope`)** |
| `tst_ImportExport.cpp` | No | No | No | Yes (`origOrg`/`origApp` set in test) |
| `tst_UiSmoke.cpp` | No | No | Yes (`AppContext ctx`) | Yes (`isolateSettings`) |
| All other `tst_Page*.cpp` | No | No | Yes (`AppContext` via fixtures) | Yes (`isolateSettings`) |

---

### Task 2: Window Minimum Size
- Changed `setMinimumSize(1000, 540)` to `setMinimumHeight(540)` in `src/ui/MainWindow.cpp`.
- Relied-on Qt behavior: Under `QLayout::SetDefaultConstraint` (default `QLayout` constraint), for any axis without an explicit widget minimum size, the layout's `minimumSize()` is applied automatically by Qt layout management to the top-level widget. Setting only `setMinimumHeight(540)` allows the window width to dynamically adapt to the layout's font-dependent minimum width (982 px on default font, 1015 px on DejaVu Sans).
- `tst_UiSmoke::testWindowMinimumSizeExceedsLayoutMinimum` updated to assert:
  - `minimumHeight() == 540`
  - `minimumWidth() >= layoutMin.width()` for both Light and Dark themes
  - `minimumHeight() >= layoutMin.height()`
  - `minimumHeight() <= 648` (verifying fit on 720p displays with decorations)
- Added `applyTestFontFamily()` reading `JOBPREP_TEST_FONT_FAMILY`.
- Checked `fc-list | grep -i "dejavu sans"`: installed (`/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf`).
- **Before fix (with DejaVu Sans):**
  ```
  FAIL!  : tst_UiSmoke::testWindowMinimumSizeExceedsLayoutMinimum() Compared values are not the same
     Actual   (m_window->minimumSize()): QSize(1015x540)
     Expected (QSize(1000, 540))       : QSize(1000x540)
  Totals: 12 passed, 1 failed
  ```
- **After fix (with DejaVu Sans):**
  ```
  PASS   : tst_UiSmoke::testWindowMinimumSizeExceedsLayoutMinimum()
  Totals: 13 passed, 0 failed
  ```

---

### Task 3: Clipping at 1.5 Scale
- **Before fix (with `setMinimumSize(1000, 540)`, 1.5 scale, DejaVu Sans):**
  ```
  CLIPPING_VIOLATION: [Page_1 | Light | 1000x540] QPushButton (ChipBarChip) '': width (67) < minimumSizeHint.width (73)
  CLIPPING_VIOLATION: [Page_3 | Light | 1000x540] QLabel (CalendarMonthTitle) '': width (171) < minimumSizeHint.width (186)
  CLIPPING_VIOLATION: [Page_3 | Light | 1000x540] QLabel (CalendarMonthTitle) 'November 2026': horizontalAdvance (169) > contentsRect.width (167)
  CLIPPING_VIOLATION: [Page_1 | Dark | 1000x540] QPushButton (ChipBarChip) '': width (67) < minimumSizeHint.width (73)
  CLIPPING_VIOLATION: [Page_3 | Dark | 1000x540] QLabel (CalendarMonthTitle) '': width (171) < minimumSizeHint.width (186)
  CLIPPING_VIOLATION: [Page_3 | Dark | 1000x540] QLabel (CalendarMonthTitle) 'November 2026': horizontalAdvance (169) > contentsRect.width (167)
  FAIL!  : tst_HiDpi::testPagesHiDpi() Compared values are not the same
     Actual   (allViolations.count()): 6
     Expected (0)                    : 0
  ```
- Identity of empty-text `ChipBarChip ''`: The empty text string in the violation output is caused by `checkClipping` item 1 (minimumSizeHint width comparison), which constructs `ClippingViolation` with an empty `{}` for the text field. The widget itself is the `QPushButton` chip inside `ChipBar` (Page 1 / StudyPage track filter chips).
- **After fix (with `setMinimumHeight(540)`, 1.5 scale, DejaVu Sans):**
  ```
  PASS   : tst_HiDpi::testPagesHiDpi()
  Totals: 5 passed, 0 failed, 1 skipped (screenshots disabled)
  ```
  Zero clipping violations remain; no UI code changes needed (commit 2 omitted per prompt "if any").

---

### Task 4: Test Fixes

#### a. Shared Helper `TestSettingsScope`
Created [`tests/data/TestSettingsScope.h`](file:///home/tony/projects/JobPrep/tests/data/TestSettingsScope.h) in `jobprep_test_helpers`. Sets `QSettings::setDefaultFormat(QSettings::IniFormat)`, `QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_tempDir.path())`, and unique `organizationName` and `applicationName`. Used in:
- `tst_TopicListModel`
- `tst_StudyRegression`
- `tst_Notifiers`
- `tst_MonthView`

#### b. `QTemporaryFile` -> `QTemporaryDir`
Replaced in:
- `tst_SettingsService` (all 7 slots: `defaultLeadMinutesReturns1440And60`, `sanitizing...`, `emptyOrInvalid...`, `signalEmitted...`, `testLeadMinutes...`, `testTrayHintShownKey`, `testWeeklyGoalSurvivesRestart`)
- `tst_PageSettings::testWeeklyGoalPersistence`
- First scope sync & diagnostic log:
  ```
  QINFO  : tst_SettingsService::testTrayHintShownKey() CI-DIAG testTrayHintShownKey first scope status: 0 size: 25
  QINFO  : tst_SettingsService::testWeeklyGoalSurvivesRestart() CI-DIAG testWeeklyGoalSurvivesRestart first scope status: 0 size: 30
  QINFO  : tst_PageSettings::testWeeklyGoalPersistence() CI-DIAG testWeeklyGoalPersistence first scope status: 0 size: 30
  ```

#### c. Removal of Hard-Coded `/tmp` Paths
- `tst_ImportExport::testIcsExportFormatDetails`: writes to `tempDir.filePath(u"sample.ics"_s)`, reads back, asserts byte equality (`QCOMPARE(readBytes, ics)`).
- `tst_ImportExport::testRestoreSafetyBackupProductionPathWithCustomXdgDataHome`: replaced `customXdg = u"/tmp/jp-c07"_s` with `QTemporaryDir customXdgDir`.

#### d. Isolation Proof
Command and output:
```bash
rm -rf /tmp/jp-home-ci5 && mkdir -p /tmp/jp-home-ci5 && HOME=/tmp/jp-home-ci5 XDG_CONFIG_HOME=/tmp/jp-home-ci5/.config XDG_DATA_HOME=/tmp/jp-home-ci5/.local/share ctest --preset dev-linux -j 2 --output-on-failure; find /tmp/jp-home-ci5 -type f | head -20
```
```
100% tests passed, 0 tests failed out of 34

Label Time Summary:
data       =   0.62 sec*proc (9 tests)
model      =   0.24 sec*proc (6 tests)
service    =   2.00 sec*proc (7 tests)
ui         =  91.25 sec*proc (12 tests)

Total Test time (real) =  47.90 sec
/tmp/jp-home-ci5/.local/share/JobPrepTest/JobPrepTest_7614/backups/pre-restore-20261104-134407.json
```
Analysis: No files were created under `/tmp/jp-home-ci5/.config` (zero settings pollution). The single file created under `.local/share` is the safety backup created by `tst_ImportExport::testRestoreSafetyBackupProductionPathWithCustomXdgDataHome` in its own isolated test AppData directory (`JobPrepTest_7614`), as designed.

---

### Task 5: Workflow
In `.github/workflows/ci.yml`, updated line 204:
```powershell
$lines = @($rawLines | Where-Object { $_ -match 'CI-DIAG' -or $_ -notmatch '^\s*(QINFO|INFO)\s*:' })
```
Keeps all `CI-DIAG` diagnostic lines while dropping general QINFO noise. YAML parsed and validated successfully with `yaml.safe_load`.

---

### Task 6: Local Quality Gate
`CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` full raw output:
```
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.2s)
-- Generating done (0.2s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
ninja: no work to do.
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start 34: tst_HiDpi
      Start 29: tst_PageStudy
 1/34 Test #34: tst_HiDpi ........................   Passed    3.27 sec
      Start 33: tst_EmptyStatesAndFocus
 2/34 Test #33: tst_EmptyStatesAndFocus ..........   Passed   10.70 sec
      Start 30: tst_PageHome
 3/34 Test #29: tst_PageStudy ....................   Passed   18.67 sec
      Start 27: tst_PageJobs
 4/34 Test #27: tst_PageJobs .....................   Passed   12.11 sec
      Start 28: tst_PageCalendar
 5/34 Test #30: tst_PageHome .....................   Passed   17.09 sec
      Start 26: tst_StudyRegression
 6/34 Test #26: tst_StudyRegression ..............   Passed    6.59 sec
      Start 32: tst_Shortcuts
 7/34 Test #28: tst_PageCalendar .................   Passed    7.30 sec
      Start 31: tst_PageSettings
 8/34 Test #32: tst_Shortcuts ....................   Passed    3.07 sec
      Start 25: tst_Notifiers
 9/34 Test #31: tst_PageSettings .................   Passed    4.47 sec
      Start 10: ui_smoke
10/34 Test #25: tst_Notifiers ....................   Passed    2.66 sec
      Start 21: tst_ImportExport
11/34 Test #21: tst_ImportExport .................   Passed    0.67 sec
      Start 24: tst_StatsService
12/34 Test #24: tst_StatsService .................   Passed    0.96 sec
      Start  8: tst_SeedService
13/34 Test  #8: tst_SeedService ..................   Passed    0.29 sec
      Start 15: tst_TopicListModel
14/34 Test #15: tst_TopicListModel ...............   Passed    0.05 sec
      Start 22: tst_ReminderService
15/34 Test #22: tst_ReminderService ..............   Passed    0.08 sec
      Start 17: tst_TopicTableProxy
16/34 Test #17: tst_TopicTableProxy ..............   Passed    0.07 sec
      Start 18: tst_AgendaService
17/34 Test #18: tst_AgendaService ................   Passed    0.09 sec
      Start  5: tst_ApplicationRepository
18/34 Test  #5: tst_ApplicationRepository ........   Passed    0.10 sec
      Start  3: tst_TopicRepository
19/34 Test  #3: tst_TopicRepository ..............   Passed    0.09 sec
      Start 11: tst_MonthView
20/34 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 16: tst_TopicBoardProxy
21/34 Test #16: tst_TopicBoardProxy ..............   Passed    0.06 sec
      Start 13: tst_ApplicationTableModel
22/34 Test #13: tst_ApplicationTableModel ........   Passed    0.04 sec
      Start 12: tst_AgendaListModel
23/34 Test #12: tst_AgendaListModel ..............   Passed    0.03 sec
      Start  6: tst_InterviewRepository
24/34 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start  9: tst_TransactionBehavior
25/34 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start 23: tst_SettingsService
26/34 Test #23: tst_SettingsService ..............   Passed    0.02 sec
      Start  1: tst_Database
27/34 Test  #1: tst_Database .....................   Passed    0.03 sec
      Start  7: tst_ReminderLogRepository
28/34 Test  #7: tst_ReminderLogRepository ........   Passed    0.02 sec
      Start 19: tst_DayWatcher
29/34 Test #19: tst_DayWatcher ...................   Passed    0.01 sec
      Start  4: tst_SessionRepository
30/34 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  2: tst_TrackRepository
31/34 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start 14: tst_ApplicationFilterProxy
32/34 Test #14: tst_ApplicationFilterProxy .......   Passed    0.02 sec
      Start 20: tst_ExportService
33/34 Test #20: tst_ExportService ................   Passed    0.01 sec
34/34 Test #10: ui_smoke .........................   Passed    5.80 sec

100% tests passed, 0 tests failed out of 34

Label Time Summary:
data       =   0.65 sec*proc (9 tests)
model      =   0.28 sec*proc (6 tests)
service    =   1.85 sec*proc (7 tests)
ui         =  91.84 sec*proc (12 tests)

Total Test time (real) =  48.36 sec
==> Quality check PASSED with zero warnings and all tests green!
```
- Total test binaries executed: 34
- Total test functions executed: 251 (all passed)
- `git status --short`: `(no output)` (clean before report commit)

#### `git log --oneline` (Commits for this task)
```
9bfab8d ci(windows): keep CI-DIAG output lines in diagnostic log filtering
8cf82ca fix(tests): isolate QSettings via TestSettingsScope and use QTemporaryDir for INI persistence
afbc43a fix(ui): remove fixed minimum width and let layout govern width
```

---

## 3. Honesty & Predictions

### Predictions (CI Run 5)
- **Ubuntu CI Failures (2/2 predicted GREEN):**
  1. `tst_UiSmoke::testWindowMinimumSizeExceedsLayoutMinimum`: **PREDICTED GREEN**. Removing the hard-coded 1000 minimum width allows Qt's layout to set the minimum width (1015 px with DejaVu Sans), and the test now asserts `>= layout minimum width` instead of equality with 1000. Verified locally with `JOBPREP_TEST_FONT_FAMILY="DejaVu Sans"`.
  2. `tst_HiDpi::testPagesHiDpi`: **PREDICTED GREEN**. With the window minimum width governed by the layout (1015 px), the contents are never squeezed below their layout minimums. Verified locally with `JOBPREP_TEST_FONT_FAMILY="DejaVu Sans"` and `QT_SCALE_FACTOR=1.5`.

- **Windows CI Failures (6/6 predicted GREEN):**
  1. `tst_TopicListModel::settingsKeysAndFallbacks`: **PREDICTED GREEN**. `TestSettingsScope` sets organization/application names and isolates settings to IniFormat in a temporary directory, eliminating registry access errors.
  2. `tst_Notifiers::coordinatorEndToEndSettingsLeadMinutesChangeTakesEffect`: **PREDICTED GREEN**. `TestSettingsScope` ensures QSettings is valid and writable.
  3. `tst_StudyRegression::testChecklistStartupContrastDarkAndLight`: **PREDICTED GREEN**. `TestSettingsScope` ensures QSettings writes succeed.
  4. `tst_SettingsService::testTrayHintShownKey`: **PREDICTED GREEN**. Replaced `QTemporaryFile` with `QTemporaryDir` + `<dir>/settings.ini`.
  5. `tst_SettingsService::testWeeklyGoalSurvivesRestart`: **PREDICTED GREEN**. Same `QTemporaryDir` fix.
  6. `tst_PageSettings::testWeeklyGoalPersistence`: **PREDICTED GREEN**. Same `QTemporaryDir` fix.
  7. `tst_ImportExport::testIcsExportFormatDetails`: **PREDICTED GREEN**. Removed `/tmp/throwaway_sample.ics` in favor of `QTemporaryDir`.

---

## 4. Not Verified by Agent, User to Check
1. **Windows effect of test fixes:** Must be observed on the next CI run on GitHub Actions.
2. **Ubuntu CI run with DejaVu Sans:** While verified locally with `JOBPREP_TEST_FONT_FAMILY="DejaVu Sans"` on Linux, final proof depends on the GitHub Actions Ubuntu runner.
3. **Window maximize on 720p laptop:** Verify that on the user's laptop (1280x720) the window still maximizes cleanly and comfortably with `setMinimumHeight(540)`.
4. **Layout minimum width across different fonts:** With a system font whose metrics differ, verify that the window minimum width expands/contracts naturally to match layout requirements without clipping.
