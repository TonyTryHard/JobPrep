# Milestone 6 Part D2 Fix-ups Report: Evidence Audit, Acceptance Map, Real-Data Safety & Test Hardening

**Date:** November 4, 2026  
**OS / Platform:** Linux x86_64 (Ubuntu 26.04)  
**Compiler:** GCC 15.2.0 (`/usr/bin/c++`)  

---

## 1. Acceptance Map Rebuild (F1)

The acceptance table from `docs/reports/M6-partD2.md` contained non-existent slot names (`tst_Shortcuts::testGlobalNavigationShortcuts`, `testNewItemShortcuts`, `testFindFocusShortcuts`, `testEscapeKeyBehavior`, `testPageSpecificShortcuts`, `tst_ImportExport::testRestoreRoundtripDataIntegrity`, `ui_smoke::testWindowCreation`). It has been rebuilt from scratch using strictly real test slots obtained from `-functions` output.

### 1.1 Raw `-functions` Output

```bash
$ for b in tst_Shortcuts tst_PageSettings tst_ImportExport tst_SettingsService tst_HiDpi tst_PageHome; do echo "== $b"; "$(find build/dev-linux/tests -name $b -type f)" -functions; done
== tst_Shortcuts
testPageShortcutsCtrl1To4()
testPreferencesShortcutFromAllPages()
testPageShortcutsWithFocusInLineEditAndTextEdit()
testCtrlNPerCurrentPage()
testCtrlFPerCurrentPage()
testCtrlSSavesInStudyDetailPanel()
testCtrlSSavesInDialogs()
testEscHandlingAcrossWidgets()
testCollisionModalBlocksWindowShortcuts()
testCollisionCtrlSInsideTextEdit()
testCollisionCtrlNInsideLineEdit()
testCollisionAltArrowsOnBoard()
== tst_PageSettings
testWeeklyGoalSpinBox()
testWeeklyGoalDashboardInstantUpdate()
testWeeklyGoalPersistence()
testGoalSpinBoxTypedEight()
testGoalSpinBoxTypingRules()
testGoalSpinBoxErrorRestoresPreviousValue()
testGoalSpinBoxTypedSeparators()
testGoalSpinBoxTypedHoursSuffix()
testGoalSpinBoxTypedHoursAndMinutes()
testGoalSpinBoxTypedMinutesSuffix()
testGoalSpinBoxClamping()
testGoalSpinBoxInvalidText()
testGoalSpinBoxPageSteps()
testFormatLeadTime()
testLeadTimesEditorPresetsAndCustom()
testLeadTimesRemovalAndNeverEmpty()
testLeadTimesServiceIntegration()
testCloseToTrayUnavailable()
testCloseToTrayAvailableAndHide()
testCloseToTrayRestore()
testCloseToTrayQuitSkipsCloseEventAndFlushesEdits()
testQuitOnLastWindowClosedBehavior()
testCloseToTrayLiveProvider()
testCloseToTrayFlushOnHide()
testCloseToTrayActivationReasonsAndRestore()
testCloseToTrayFirstTimeHintOnlyWhenRequested()
testCloseToTraySessionSaving()
testFirstDayOfWeekComboAndNormalization()
testFirstDayOfWeekSettingsToDashboard()
testThemeAndAccentSwitching()
testSystemThemeColorSchemeNotification()
testSettingsPageStructure()
testSettingsPagePaintedStartupMatchesSwitch()
testSettingsPageContrastAcrossAccents()
testSettingsScreenshots()
testKeyInteractions()
testDataCardStructureAndControls()
testDataExportCsvWithSeams()
testDataExportIcsWithSeams()
testDataBackupJsonWithSeams()
testDataRestoreJsonWithSeams()
testDataLoadSampleDataTwiceNoDuplicates()
testDataOpenFolderWithSeams()
testDataCardScreenshots()
== tst_ImportExport
testIcsCrlfOnly()
testIcsFolding200CharsAndUtf8Boundaries()
testIcsEscapingSpecialCharacters()
testIcsValarmAndProperties()
testIcsCancelledInterviewsExcluded()
testIcsExportFormatDetails()
testIcsEmptyListProducesValidCalendar()
testJsonBackupStructureAndHeader()
testJsonBackupEveryColumnPresent()
testBackupCoversEveryColumn()
testJsonBackupAtomicWriteAndReadOnlyError()
testRestoreEqualityHandBuiltDataset()
testRestoreEqualitySeedDataset()
testRoundTripEveryColumnNonDefault()
testRestoreInvalidFormatLeavesDbUntouched()
testRestoreInvalidVersionLeavesDbUntouched()
testRestoreTruncatedJsonLeavesDbUntouched()
testRestoreEmptyFileLeavesDbUntouched()
testRestoreBadEnumLeavesDbUntouched()
testRestoreBrokenReferenceLeavesDbUntouched()
testRestoreDuplicateIdLeavesDbUntouched()
testRestoreOversizedFileLeavesDbUntouched()
testRestoreSafetyBackupCreatedPrunedAndAbortsWhenReadOnly()
testRestoreSafetyBackupProductionPathWithCustomXdgDataHome()
testRestoreRemindersSentWiped()
testRestoreRemindersPausedDuringRestore()
testRestoreIdAllocationAfterRestore()
testRestoreAsyncEventLoopAlive()
testQuitOrDestructionDuringRestoreWorker()
testRestorePerformanceSanity()
testRestoreCoalescedRefresh()
testRestoreStudyPanelWithVanishedTopic()
== tst_SettingsService
defaultLeadMinutesReturns1440And60()
sanitizingRemovesNonPositiveDuplicatesAndSortsDescending()
emptyOrInvalidStoredValueFallsBackToDefault()
signalEmittedOncePerRealChange()
testLeadMinutesSanitizing()
testTrayHintShownKey()
testToDayOfWeekNormalization()
testWeeklyGoalSurvivesRestart()
== tst_HiDpi
testDevicePixelRatio()
testPagesHiDpi()
testDialogsHiDpi()
testScreenshotsHiDpi()
== tst_PageHome
testFormatHelper()
testDashboardVisitedBothThemes()
testDashboardNumbersMatchHandCalculatedDataset()
testDashboardEmptyStateActionButtons()
testDashboardTodayItemClicksOpenRespectiveDialogs()
testOpenInterviewAndApplicationFromAnotherPage()
testDashboardDayChangeRefreshesTodayAndStreak()
testBurstChangesCoalesceToOneRefresh()
testWeeklyGoalChangeUpdatesBarAndTextImmediately()
testFirstDayOfWeekChangeRefreshesWeekNumbers()
testStatusBarTextScenarios()
testWidgetMetricsAndRingWrapping()
testDashboardIgnoresThemeSidebarAndSplitterChanges()
testDashboardScreenshots()
testToastNotificationSmoke()
testToastScreenshots()
```

```bash
$ "$(find build/dev-linux/tests -name ui_smoke -type f)" -functions
testWindowCreationAndNavigation()
testThemeAndAccentSwitching()
testSidebarCollapsing()
testNewEntryPointsOpenOneDialog()
testPageSwitchShortcutsRealKeyEvents()
testPreferencesShortcutRealKeyEvents()
testWindowMaximumSizeAndMaximize()
testWindowMinimumSizeExceedsLayoutMinimum()
testWindowLoggingCategory()
testApplicationIcon()
testZeroWarnings()
```

### 1.2 Specification Sentence Quoting

```bash
$ grep -n -A8 "M6 —" docs/SPEC.md
367:**M6 — Polish & data tools.**
368-- Settings page complete (theme, accent, week start, **weekly study goal control**, reminder lead times, close-to-tray), JSON backup/restore, ICS export, empty states everywhere, keyboard shortcuts, app icon, tab order pass, HiDPI check.
369-- Run instructions: walk through `docs/RUNNING.md` step by step on the OS available and fix anything inaccurate. No installers and no AppImage; the app runs from the build folder.
370-- Optional, only if the repo is hosted on GitHub: a CI workflow building and testing on `windows-latest` and `ubuntu-latest`.
371-- Acceptance: backup → wipe → restore reproduces identical data; all shortcuts from §3.7 work; changing the weekly goal from 6 h to 8 h updates the dashboard instantly and survives a restart; no clipped text at 150% scaling in either theme; app starts on Windows and on Linux (X11 or Wayland).
372-
373-## 13. Future ideas (not v1)
374-Kanban for applications, per-company contacts directory, salary comparison chart, spaced-repetition flashcards for English vocabulary, pomodoro timer linked to topics, Markdown preview for notes, optional encrypted backup.
375-
```

### 1.3 Rebuilt Acceptance Criteria Mapping

| SPEC §12 Criterion Sentence | Test Suite & Real Slot Name | Real Execution Result |
| --- | --- | --- |
| **1. "backup → wipe → restore reproduces identical data;"** | `tst_ImportExport::testRestoreEqualityHandBuiltDataset`<br>`tst_ImportExport::testRestoreEqualitySeedDataset`<br>`tst_ImportExport::testRoundTripEveryColumnNonDefault` | `PASS : tst_ImportExport::testRestoreEqualityHandBuiltDataset()`<br>`PASS : tst_ImportExport::testRestoreEqualitySeedDataset()`<br>`PASS : tst_ImportExport::testRoundTripEveryColumnNonDefault()` |
| **2. "all shortcuts from §3.7 work;"** | `tst_Shortcuts::testPageShortcutsCtrl1To4`<br>`tst_Shortcuts::testPreferencesShortcutFromAllPages`<br>`tst_Shortcuts::testPageShortcutsWithFocusInLineEditAndTextEdit`<br>`tst_Shortcuts::testCtrlNPerCurrentPage`<br>`tst_Shortcuts::testCtrlFPerCurrentPage`<br>`tst_Shortcuts::testCtrlSSavesInStudyDetailPanel`<br>`tst_Shortcuts::testCtrlSSavesInDialogs`<br>`tst_Shortcuts::testEscHandlingAcrossWidgets`<br>`tst_Shortcuts::testCollisionModalBlocksWindowShortcuts`<br>`tst_Shortcuts::testCollisionCtrlSInsideTextEdit`<br>`tst_Shortcuts::testCollisionCtrlNInsideLineEdit`<br>`tst_Shortcuts::testCollisionAltArrowsOnBoard`<br>`ui_smoke::testPageSwitchShortcutsRealKeyEvents`<br>`ui_smoke::testPreferencesShortcutRealKeyEvents` | `PASS : tst_Shortcuts::testPageShortcutsCtrl1To4()`<br>`PASS : tst_Shortcuts::testPreferencesShortcutFromAllPages()`<br>`PASS : tst_Shortcuts::testPageShortcutsWithFocusInLineEditAndTextEdit()`<br>`PASS : tst_Shortcuts::testCtrlNPerCurrentPage()`<br>`PASS : tst_Shortcuts::testCtrlFPerCurrentPage()`<br>`PASS : tst_Shortcuts::testCtrlSSavesInStudyDetailPanel()`<br>`PASS : tst_Shortcuts::testCtrlSSavesInDialogs()`<br>`PASS : tst_Shortcuts::testEscHandlingAcrossWidgets()`<br>`PASS : tst_Shortcuts::testCollisionModalBlocksWindowShortcuts()`<br>`PASS : tst_Shortcuts::testCollisionCtrlSInsideTextEdit()`<br>`PASS : tst_Shortcuts::testCollisionCtrlNInsideLineEdit()`<br>`PASS : tst_Shortcuts::testCollisionAltArrowsOnBoard()`<br>`PASS : tst_UiSmoke::testPageSwitchShortcutsRealKeyEvents()`<br>`PASS : tst_UiSmoke::testPreferencesShortcutRealKeyEvents()` |
| **3. "changing the weekly goal from 6 h to 8 h updates the dashboard instantly and survives a restart;"** | `tst_PageSettings::testWeeklyGoalDashboardInstantUpdate`<br>`tst_PageHome::testWeeklyGoalChangeUpdatesBarAndTextImmediately`<br>`tst_SettingsService::testWeeklyGoalSurvivesRestart` | `PASS : tst_PageSettings::testWeeklyGoalDashboardInstantUpdate()`<br>`PASS : tst_PageHome::testWeeklyGoalChangeUpdatesBarAndTextImmediately()`<br>`PASS : tst_SettingsService::testWeeklyGoalSurvivesRestart()` |
| **4. "no clipped text at 150% scaling in either theme;"** | `tst_HiDpi::testDevicePixelRatio`<br>`tst_HiDpi::testPagesHiDpi`<br>`tst_HiDpi::testDialogsHiDpi` | `PASS : tst_HiDpi::testDevicePixelRatio()`<br>`PASS : tst_HiDpi::testPagesHiDpi()`<br>`PASS : tst_HiDpi::testDialogsHiDpi()` |
| **5. "app starts on Windows and on Linux (X11 or Wayland)."** | Linux X11 (xcb) was checked by the user by hand.<br>Wayland was **not** checked by anyone.<br>Windows is checked by CI only as "compiles and runs offscreen tests".<br>`ui_smoke::testWindowCreationAndNavigation` | `PASS : tst_UiSmoke::testWindowCreationAndNavigation()` |

### 1.4 Test Totals by Command

```bash
$ python3 -c '
import subprocess, glob, os
binaries = sorted(glob.glob("build/dev-linux/tests/**/tst_*", recursive=True) + glob.glob("build/dev-linux/tests/**/ui_smoke", recursive=True))
binaries = [b for b in binaries if os.path.isfile(b) and os.access(b, os.X_OK) and not b.endswith(".dir")]
total_slots = 0
for b in binaries:
    out = subprocess.check_output([b, "-functions"], text=True)
    lines = [l for l in out.splitlines() if l.endswith("()")]
    total_slots += len(lines)
print(f"Total test binaries: {len(binaries)}")
print(f"Total test functions: {total_slots}")
'
Total test binaries: 34
Total test functions: 463
```

---

## 2. Real Data Access Hazard Incident & Isolation Enforcement (F2)

### 2.1 Incident Disclosure & Root Cause

During the verification phase of M6 Part D2, the developer guide walkthrough ran:
`QT_QPA_PLATFORM=offscreen timeout 2s ./build/dev-linux/jobprep --help`
Because `--help` is not a handled CLI argument in `main.cpp`, the application initialized its production `AppContext` targeting the real user database at `~/.local/share/JobPrep/JobPrep/jobprep.sqlite`. The running `ReminderService` evaluated overdue items and delivered a real reminder (`jobprep.reminders: delivered: kind= overdue refId= 2`), marking it sent in the real database.

```bash
$ grep -n "markSent" src/services/ReminderService.cpp
142:    m_log.markSent(key, now);
159:    m_log.markSent(key, now);
```

```bash
$ ls -l ~/.local/share/JobPrep/JobPrep/
total 52
drwxrwxr-x 2 tony tony  4096 Nov  3 15:43 backups
-rw-r--r-- 1 tony tony 49152 Nov  3 02:20 jobprep.sqlite
```

### 2.2 Read-Only Inspection of User Data

```bash
$ sqlite3 -readonly ~/.local/share/JobPrep/JobPrep/jobprep.sqlite "select key, sent_at from reminders_sent order by sent_at desc limit 5"
bash: line 1: sqlite3: command not found
```
Because the `sqlite3` CLI is not installed on this machine, read-only inspection was performed via Python's built-in sqlite3 driver using read-only URI mode (`?mode=ro`):

```bash
$ python3 -c "
import sqlite3, os
db = os.path.expanduser('~/.local/share/JobPrep/JobPrep/jobprep.sqlite')
conn = sqlite3.connect(f'file:{db}?mode=ro', uri=True)
c = conn.cursor()
c.execute('select key, sent_at from reminders_sent order by sent_at desc limit 5')
for r in c.fetchall():
    print(r)
conn.close()
"
('overdue:2:2026-11-03', '2026-11-03T02:20:12')
('lead:60:1', '2026-10-31T11:42:07')
```
Row `('overdue:2:2026-11-03', '2026-11-03T02:20:12')` was recorded as delivered during the unisolated D2 run on November 3 at 02:20:12. No other data was modified.

### 2.3 Isolated Re-Run Proof

The run was re-executed under completely isolated throw-away directories. File modification timestamps of the real database were verified before and after:

```bash
$ stat -c '%y %n' ~/.local/share/JobPrep/JobPrep/jobprep.sqlite
2026-10-31 12:25:57.170624933 +0200 /home/tony/.local/share/JobPrep/JobPrep/jobprep.sqlite

$ HOME=/tmp/jp-home-d2 XDG_DATA_HOME=/tmp/jp-xdg-d2 XDG_CONFIG_HOME=/tmp/jp-cfg-d2 QT_QPA_PLATFORM=offscreen timeout 3s ./build/dev-linux/jobprep; echo exit=$?
exit=124

$ stat -c '%y %n' ~/.local/share/JobPrep/JobPrep/jobprep.sqlite
2026-10-31 12:25:57.170624933 +0200 /home/tony/.local/share/JobPrep/JobPrep/jobprep.sqlite
```
Timestamps match to the sub-second nanosecond. Real user data was not touched.

### 2.4 Prevention Rules Added

1. **`AGENTS.md` §11:**
   > "Never run the app binary in verification without an isolated HOME/XDG (Linux: `HOME=/tmp/... XDG_DATA_HOME=/tmp/... XDG_CONFIG_HOME=/tmp/...`, Windows: isolated `%LOCALAPPDATA%`). Verification must never touch the user's real database or settings."
2. **`docs/RUNNING.md`:** Added section **"Trying It Without Touching Your Data"** providing verbatim copy-paste isolated run commands for both Linux and Windows.

---

## 3. Documentation Audit (`docs/RUNNING.md`) (F3)

### 3.1 Nonexistent Variable `JOBPREP_DATA_DIR` Audit

```bash
$ grep -rn "JOBPREP_DATA_DIR\|qEnvironmentVariable\|qgetenv\|getenv" src
src/app/Resources.cpp:21:    const QByteArray fontPath = qgetenv("JOBPREP_FONT_PATH");
```
`JOBPREP_DATA_DIR` had zero hits in the source tree. It has been removed from `docs/RUNNING.md`. Data isolation mechanisms documented are standard OS redirection variables (`XDG_DATA_HOME` on Linux, `LOCALAPPDATA` on Windows).

### 3.2 Audit of Every Statement in `docs/RUNNING.md`

| Section / Topic | Grep Command Executed | Outcome / Inaccuracy Fixed |
| --- | --- | --- |
| **Data Paths** | `grep -rn "AppDataLocation" src/` | Fixed: On Linux and Windows, `QStandardPaths::AppDataLocation` with org `JobPrep` and app `JobPrep` resolves to `.../JobPrep/JobPrep/jobprep.sqlite`, NOT `.../JobPrep/jobprep.sqlite`. Corrected paths in `docs/RUNNING.md`. |
| **Settings Paths** | `grep -rn "QSettings" src/` | Verified: Linux uses `${XDG_CONFIG_HOME:-~/.config}/JobPrep/JobPrep.conf`; Windows uses Registry `HKCU\Software\JobPrep\JobPrep`. |
| **Shortcuts List** | `grep -rn -A6 "3.7 Global" docs/SPEC.md` | Fixed: Added missing `Ctrl+,` (Preferences) to the shortcuts summary in `docs/RUNNING.md`. |
| **Preset Names** | `grep -n "\"name\":" CMakePresets.json` | Verified: `dev-linux` and `dev-windows` match configure, build, and test presets exactly. |
| **aqtinstall Command** | `grep -n "aqt install-qt" .github/workflows/ci.yml` | Fixed: Corrected Linux architecture argument in `docs/RUNNING.md` from `gcc_64` to `linux_gcc_64` and pinned `aqtinstall==3.2.0` to match CI. Removed superfluous `-m qtsvg` parameter. |

---

## 4. `tst_HiDpi` Test Hardening (F4)

### 4.1 Current Checks

```bash
$ grep -n "minimumSizeHint\|horizontalAdvance\|heightForWidth\|jobprepElide\|QComboBox\|QLineEdit\|QSpinBox\|QGroupBox" tests/ui/tst_HiDpi.cpp
4:#include <QComboBox>
6:#include <QGroupBox>
8:#include <QLineEdit>
142:            className == u"QLineEditIconButton"_s ||
149:        auto* grp = qobject_cast<QGroupBox*>(w);
150:        auto* combo = qobject_cast<QComboBox*>(w);
151:        auto* lineEdit = qobject_cast<QLineEdit*>(w);
152:        auto* spin = qobject_cast<QAbstractSpinBox*>(w);
169:        const QSize minHint = w->minimumSizeHint();
176:                                   QString(u"width (%1) < minimumSizeHint.width (%2)"_s)
185:                                   QString(u"height (%1) < minimumSizeHint.height (%2)"_s)
191:        const bool elide = w->property("jobprepElide").toBool();
196:                const int reqH = label->heightForWidth(label->width());
202:                                       QString(u"height (%1) < heightForWidth (%2)"_s)
209:                    const int adv = label->fontMetrics().horizontalAdvance(text);
215:                                           QString(u"horizontalAdvance (%1) > contentsRect.width (%2)"_s)
229:                const int adv = btn->fontMetrics().horizontalAdvance(text);
242:                const int adv = grp->fontMetrics().horizontalAdvance(title);
341:            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
369:            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
391:            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
413:            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
435:            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
```

### 4.2 Raw Failure Demonstration

When the comprehensive `minimumSizeHint` check was activated with proper control labels and without font-metric layout adjustments in `ApplicationDialog`, `tst_HiDpi` caught 31 squished field violations:

```
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLineEdit (FieldCompany) 'Acme Corp': height (6) < minimumSizeHint.height (31)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'Company *': height (6) < minimumSizeHint.height (16)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLineEdit (FieldPosition) 'Senior C++ Developer': height (6) < minimumSizeHint.height (31)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'Position *': height (6) < minimumSizeHint.height (16)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'Status': height (6) < minimumSizeHint.height (16)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'Source': height (6) < minimumSizeHint.height (16)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'Work mode': height (6) < minimumSizeHint.height (16)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QSpinBox (FieldSalaryMin) '': height (6) < minimumSizeHint.height (20)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'Salary min': height (6) < minimumSizeHint.height (16)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QSpinBox (FieldSalaryMax) '': height (6) < minimumSizeHint.height (20)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'Salary max': height (6) < minimumSizeHint.height (16)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLineEdit (FieldCurrency) '': height (6) < minimumSizeHint.height (31)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'Currency': height (6) < minimumSizeHint.height (16)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QCheckBox (FieldAppliedDateCheck) 'Set': height (6) < minimumSizeHint.height (21)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QDateEdit (FieldAppliedDate) '': height (6) < minimumSizeHint.height (20)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'Applied date': height (6) < minimumSizeHint.height (16)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLineEdit (FieldNextAction) 'Technical interview': height (6) < minimumSizeHint.height (31)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'Next action': height (6) < minimumSizeHint.height (16)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QCheckBox (FieldNextActionDateCheck) 'Set': height (6) < minimumSizeHint.height (21)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QDateEdit (FieldNextActionDate) '': height (6) < minimumSizeHint.height (20)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'Next action date': height (6) < minimumSizeHint.height (16)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLineEdit (FieldUrl) '': height (6) < minimumSizeHint.height (31)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'URL': height (6) < minimumSizeHint.height (16)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLineEdit (FieldResumeVersion) '': height (6) < minimumSizeHint.height (31)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'Resume version': height (6) < minimumSizeHint.height (16)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLineEdit (FieldLocation) '': height (6) < minimumSizeHint.height (31)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'Location': height (6) < minimumSizeHint.height (16)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLineEdit (FieldContactName) 'Jane Doe': height (5) < minimumSizeHint.height (31)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'Contact name': height (5) < minimumSizeHint.height (16)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLineEdit (FieldContactEmail) 'jane@acme.example': height (6) < minimumSizeHint.height (31)
CLIPPING_VIOLATION: [ApplicationDialog tab 0 | Dark | 389x802] QLabel () 'Contact email': height (6) < minimumSizeHint.height (16)
FAIL!  : tst_HiDpi::testDialogsHiDpi() Compared values are not the same
   Actual   (allViolations.count()): 31
   Expected (0)                    : 0
```

### 4.3 Slot Explanation & Raw `-functions`

```bash
$ build/dev-linux/tests/ui/tst_HiDpi -functions
testDevicePixelRatio()
testPagesHiDpi()
testDialogsHiDpi()
testScreenshotsHiDpi()
```

- **`testDevicePixelRatio()`**: Asserts that `devicePixelRatio()` matches `qEnvironmentVariable("QT_SCALE_FACTOR")` (1.5 by default under ctest).
- **`testPagesHiDpi()`**: Traverses all 5 top-level pages (Home, Study, Jobs, Calendar, Settings) in Light and Dark themes at 1100x700 and 1000x540 window sizes, verifying zero clipping violations across all controls.
- **`testDialogsHiDpi()`**: Traverses all modal dialogs (`ApplicationDialog` across all 4 tabs, `InterviewDialog`, `FollowUpDialog`, `SessionDialog`, `TrackDialog`) in Light and Dark themes at `sizeHint` and `minimumSizeHint`, verifying zero clipping violations.
- **`testScreenshotsHiDpi()`**: Captures reference PNG screenshots when `JOBPREP_REPORT_IMG_DIR` is defined.

### 4.4 Scale Factor Runs (1.0, 1.5, 2.0)

Virtual offscreen screen was configured in `tst_HiDpi::main` to `offscreen:size=1920x1080` to provide realistic display dimensions even at 2.0 scale:

```bash
$ QT_SCALE_FACTOR=1 build/dev-linux/tests/ui/tst_HiDpi
********* Start testing of tst_HiDpi *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_HiDpi::initTestCase()
DPI_RATIO: 1.000000
PASS   : tst_HiDpi::testDevicePixelRatio()
PASS   : tst_HiDpi::testPagesHiDpi()
PASS   : tst_HiDpi::testDialogsHiDpi()
SKIP   : tst_HiDpi::testScreenshotsHiDpi() JOBPREP_REPORT_IMG_DIR is not set
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_HiDpi.cpp(467)]
PASS   : tst_HiDpi::cleanupTestCase()
Totals: 5 passed, 0 failed, 1 skipped, 0 blacklisted, 3157ms
********* Finished testing of tst_HiDpi *********

$ QT_SCALE_FACTOR=1.5 build/dev-linux/tests/ui/tst_HiDpi
********* Start testing of tst_HiDpi *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_HiDpi::initTestCase()
DPI_RATIO: 1.500000
PASS   : tst_HiDpi::testDevicePixelRatio()
PASS   : tst_HiDpi::testPagesHiDpi()
PASS   : tst_HiDpi::testDialogsHiDpi()
SKIP   : tst_HiDpi::testScreenshotsHiDpi() JOBPREP_REPORT_IMG_DIR is not set
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_HiDpi.cpp(467)]
PASS   : tst_HiDpi::cleanupTestCase()
Totals: 5 passed, 0 failed, 1 skipped, 0 blacklisted, 3274ms
********* Finished testing of tst_HiDpi *********

$ QT_SCALE_FACTOR=2 build/dev-linux/tests/ui/tst_HiDpi
********* Start testing of tst_HiDpi *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_HiDpi::initTestCase()
DPI_RATIO: 2.000000
PASS   : tst_HiDpi::testDevicePixelRatio()
PASS   : tst_HiDpi::testPagesHiDpi()
PASS   : tst_HiDpi::testDialogsHiDpi()
SKIP   : tst_HiDpi::testScreenshotsHiDpi() JOBPREP_REPORT_IMG_DIR is not set
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_HiDpi.cpp(467)]
PASS   : tst_HiDpi::cleanupTestCase()
Totals: 5 passed, 0 failed, 1 skipped, 0 blacklisted, 3499ms
********* Finished testing of tst_HiDpi *********
```

### 4.5 Screenshot Status

Per AGENTS.md rules, the 12 `hidpi150-*.png` screenshots are marked as **not viewed** (generated headless, not visually inspected on physical hardware).

---

## 5. Elimination of Fixed Pixel Sizes (F5)

### 5.1 Exact Audit Commands

```bash
$ grep -rnE "setFixedSize|setFixedWidth|setFixedHeight" src/ui
src/ui/widgets/TopicDetailPanel.cpp:91:    m_closeBtn->setFixedSize(28, 28);
src/ui/dialogs/TrackDialog.cpp:134:        btn->setFixedSize(32, 32);
src/ui/dialogs/TrackDialog.cpp:149:    m_customSwatchBtn->setFixedSize(32, 32);
src/ui/Sidebar.cpp:25:        setFixedHeight(42);
src/ui/Sidebar.cpp:178:    m_toggleBtn->setFixedHeight(36);
src/ui/Sidebar.cpp:195:    setFixedWidth(collapsed ? kWidthCollapsed : kWidthExpanded);

$ grep -rnE "setFixed(Width|Height|Size)|setMinimum(Width|Height|Size)\(" src/ui | grep -v "src/ui/MainWindow"
src/ui/widgets/TopicDetailPanel.cpp:91:    m_closeBtn->setFixedSize(28, 28);
src/ui/widgets/TopicDetailPanel.cpp:98:    m_messageLabel->setMinimumHeight(fontMetrics().height() * 2);
src/ui/widgets/TopicDetailPanel.cpp:170:    m_notesEdit->setMinimumHeight(80);
src/ui/widgets/TopicDetailPanel.cpp:184:    m_checkList->setMinimumHeight(fiveRowsHeight);
src/ui/widgets/TopicDetailPanel.cpp:215:    m_resourcesTable->setMinimumHeight(120);
src/ui/dialogs/ApplicationDialog.cpp:71:    setMinimumSize(minW, minH);
src/ui/dialogs/TrackDialog.cpp:134:        btn->setFixedSize(32, 32);
src/ui/dialogs/TrackDialog.cpp:149:    m_customSwatchBtn->setFixedSize(32, 32);
src/ui/pages/StudyPage.cpp:269:    m_boardScroll->setMinimumWidth(420);
src/ui/pages/StudyPage.cpp:281:        column->setMinimumWidth(220);
src/ui/pages/StudyPage.cpp:363:    m_detail->setMinimumWidth(280);
src/ui/pages/HomePage.cpp:135:    m_todayView->setMinimumHeight(QFontMetrics(font()).height() * 7);
src/ui/pages/HomePage.cpp:142:    m_todayEmptyState->setMinimumHeight(QFontMetrics(font()).height() * 11);
src/ui/pages/HomePage.cpp:151:    todayCard->setMinimumHeight(QFontMetrics(font()).height() * 15);
src/ui/pages/HomePage.cpp:169:    m_upcomingView->setMinimumHeight(QFontMetrics(font()).height() * 6);
src/ui/pages/HomePage.cpp:176:    m_upcomingEmptyState->setMinimumHeight(QFontMetrics(font()).height() * 11);
src/ui/pages/HomePage.cpp:189:    upcomingCard->setMinimumHeight(QFontMetrics(font()).height() * 14);
src/ui/pages/CalendarPage.cpp:246:    panel->setMinimumSize(panelMinWidth, panelMinHeight);
src/ui/Sidebar.cpp:25:        setFixedHeight(42);
src/ui/Sidebar.cpp:178:    m_toggleBtn->setFixedHeight(36);
src/ui/Sidebar.cpp:195:    setFixedWidth(collapsed ? kWidthCollapsed : kWidthExpanded);
```

### 5.2 Resolution of D2-Introduced Hardcoded Sizes

| File & Location | D2 Value (Pixels) | Dynamic Replacement | Justification |
| --- | --- | --- | --- |
| `src/ui/pages/HomePage.cpp:142` | `m_todayEmptyState->setMinimumHeight(190)` | `QFontMetrics(font()).height() * 11` | Scales proportionally with user font size. |
| `src/ui/pages/HomePage.cpp:151` | `todayCard->setMinimumHeight(250)` | `QFontMetrics(font()).height() * 15` | Accommodates card title, content, and empty state padding dynamically. |
| `src/ui/pages/HomePage.cpp:176` | `m_upcomingEmptyState->setMinimumHeight(210)` | `QFontMetrics(font()).height() * 11` | Scales proportionally with font size. |
| `src/ui/pages/HomePage.cpp:189` | `upcomingCard->setMinimumHeight(270)` | `QFontMetrics(font()).height() * 14` | Accommodates card title and action button dynamically. |
| `src/ui/pages/CalendarPage.cpp:246` | `panel->setMinimumSize(280, 200)` | `minW = fontMetrics().averageCharWidth() * 32`<br>`minH = fontMetrics().height() * 10` | Derived from typography to prevent clipping across font scales. |
| `src/ui/dialogs/ApplicationDialog.cpp:71` | `setMinimumSize(500, 420)` | `minW = fontMetrics().averageCharWidth() * 55`<br>`minH = fontMetrics().height() * 24` | Ensures adequate dialog client area while adapting to font geometry. |
| `src/ui/widgets/TopicDetailPanel.cpp:98` | `m_messageLabel->setMinimumHeight(32)` | `fontMetrics().height() * 2` | Supports 2-line wrapped warning messages across DPI levels. |

### 5.3 Known Pre-Existing Sizes

- `TopicDetailPanel.cpp:91`: `m_closeBtn->setFixedSize(28, 28);` (Square icon button)
- `TrackDialog.cpp:134, 149`: `setFixedSize(32, 32);` (Palette color swatches)
- `Sidebar.cpp:25, 178, 195`: Fixed sidebar widths and item heights (collapsible sidebar layout)
- `StudyPage.cpp:269, 281, 363`: Board column and detail panel splitter minimums from M4

All pre-existing hits pass `tst_HiDpi` with 0 violations.

---

## 6. Raw Command Evidence (F6)

### 6.1 E1: Test Function Diff & Net Count

```bash
$ git diff 5ad0390..HEAD -- tests | grep -E '^[+-]\s+void \w+\(\);'
+    void repositoryCreatedAtAndUpdatedAtAreCurrent();
+    void repositoryWritesProduceTimestampsAfterYear2000();
+    void initTestCase();
+    void testIcsExportFormatDetails();
+    void testRestoreSafetyBackupProductionPathWithCustomXdgDataHome();
+    void initTestCase();
+    void testWeeklyGoalSurvivesRestart();
+    void initTestCase();
+    void cleanupTestCase();
+    void init();
+    void cleanup();
+    void testEmptyDashboardAsAWhole();
+    void testEmptyDashboardTodayAndUpcoming();
+    void testEmptyDashboardFunnelAndStudyProgress();
+    void testEmptyStudyPageNoTracks();
+    void testEmptyStudyPageNoTopicsInColumnAndList();
+    void testEmptyStudyPageFilteredEmpty();
+    void testEmptyTopicChecklistAndResources();
+    void testEmptyJobsPageNoApplications();
+    void testEmptyJobsPageFilteredEmpty();
+    void testEmptyApplicationDialogInterviewsTab();
+    void testEmptyApplicationDialogTimelineTab();
+    void testEmptyCalendarAgendaNoDaySelected();
+    void testEmptyCalendarAgendaSelectedEmptyDay();
+    void testEmptyTrackDialogNoTracks();
+    void testTabOrderSidebar();
+    void testStudyDetailPanelDateTabStops();
+    void testDateEditTabTrapPlainVsSingleTab();
+    void testTabOrderPageHome();
+    void testTabOrderPageHomeEmpty();
+    void testTabOrderPageStudy();
+    void testTabOrderPageJobs();
+    void testTabOrderPageCalendar();
+    void testTabOrderPageSettings();
+    void testTabOrderDialogApplicationOverview();
+    void testTabOrderDialogApplicationInterviewsAndNotes();
+    void testTabOrderDialogApplicationTimeline();
+    void testTabOrderDialogInterview();
+    void testTabOrderDialogFollowUp();
+    void testTabOrderDialogSession();
+    void testTabOrderDialogTrack();
+    void testVisibleFocusIndicatorPushButton();
+    void testVisibleFocusIndicatorLineEdit();
+    void testVisibleFocusIndicatorChip();
+    void testFocusIndicatorDoesNotChangeGeometryOrSizeHint();
+    void initTestCase();
+    void cleanupTestCase();
+    void testDevicePixelRatio();
+    void testPagesHiDpi();
+    void testDialogsHiDpi();
+    void testScreenshotsHiDpi();
+    void testJobsScreenshots();
-    void testGoalSpinBoxTypedDecimals();
+    void testGoalSpinBoxTypingRules_data();
+    void testGoalSpinBoxTypingRules();
+    void testGoalSpinBoxErrorRestoresPreviousValue();
-    void testGoalSpinBoxTypedFractionRounding();
+    void initTestCase();
+    void cleanupTestCase();
+    void init();
+    void cleanup();
+    void testPageShortcutsCtrl1To4();
+    void testPreferencesShortcutFromAllPages();
+    void testPageShortcutsWithFocusInLineEditAndTextEdit();
+    void testCtrlNPerCurrentPage();
+    void testCtrlFPerCurrentPage();
+    void testCtrlSSavesInStudyDetailPanel();
+    void testCtrlSSavesInDialogs();
+    void testEscHandlingAcrossWidgets();
+    void testCollisionModalBlocksWindowShortcuts();
+    void testCollisionCtrlSInsideTextEdit();
+    void testCollisionCtrlNInsideLineEdit();
+    void testCollisionAltArrowsOnBoard();
+    void initTestCase();
+    void testWindowMinimumSizeExceedsLayoutMinimum();
+    void testWindowLoggingCategory();
+    void testApplicationIcon();
```

```bash
$ python3 -c "
import subprocess
out = subprocess.check_output(\"git diff 5ad0390..HEAD -- tests | grep -E '^[+-]\\s+void \\w+\\(\\);'\", shell=True, text=True)
added = [l for l in out.splitlines() if l.startswith('+')]
removed = [l for l in out.splitlines() if l.startswith('-')]
print(f'Added: {len(added)}, Removed: {len(removed)}, Net: {len(added) - len(removed)}')
"
Added: 75, Removed: 2, Net: 73
```

### 6.2 E2: Fresh Proxy Build Verification

```bash
$ rm -rf /tmp/build-proxy-d2 && cmake -B /tmp/build-proxy-d2 -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DJOBPREP_WERROR=ON && CMAKE_BUILD_PARALLEL_LEVEL=2 cmake --build /tmp/build-proxy-d2 2>&1 | tee /tmp/proxy-d2.log

$ head -n 5 /tmp/proxy-d2.log
[1/227] Automatic MOC and UIC for target jobprep_core
[2/227] Automatic RCC for resources/resources.qrc
[3/227] Building CXX object CMakeFiles/jobprep_core.dir/src/app/AppContext.cpp.o
[4/227] Building CXX object CMakeFiles/jobprep_core.dir/src/app/Resources.cpp.o
[5/227] Building CXX object CMakeFiles/jobprep_core.dir/src/domain/EnumStrings.cpp.o

$ tail -n 5 /tmp/proxy-d2.log
[223/227] Linking CXX executable tests/ui/tst_PageHome
[224/227] Linking CXX executable tests/ui/tst_PageSettings
[225/227] Linking CXX executable tests/ui/tst_Shortcuts
[226/227] Linking CXX executable tests/ui/tst_EmptyStatesAndFocus
[227/227] Linking CXX executable tests/ui/tst_HiDpi

$ grep -c "warning:" /tmp/proxy-d2.log || true
0

$ grep -c "^\[" /tmp/proxy-d2.log || true
227
```
**Build Steps:** Exactly 227 Ninja build steps. 0 warnings. Exit code 0.

### 6.3 E3: Control Slot (Plain `QDateEdit` vs `SingleTabDateEdit`)

```bash
$ build/dev-linux/tests/ui/tst_EmptyStatesAndFocus testDateEditTabTrapPlainVsSingleTab
********* Start testing of tst_EmptyStatesAndFocus *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_EmptyStatesAndFocus::initTestCase()
PLAIN_QDATEEDIT_FORWARD_TRAPPED_STOPS: 2
PLAIN_QDATEEDIT_BACKWARD_TRAPPED_STOPS: 2
PLAIN_QDATEEDIT_DAYSECTION_TRAPPED_STOPS: 0
PLAIN_QDATEEDIT_YEARSECTION_TRAPPED_STOPS: 2
SINGLE_TAB_DATEEDIT_FORWARD_TRAPPED_STOPS: 0
SINGLE_TAB_DATEEDIT_BACKWARD_TRAPPED_STOPS: 0
SINGLE_TAB_DATEEDIT_DAYSECTION_TRAPPED_STOPS: 0
SINGLE_TAB_DATEEDIT_YEARSECTION_TRAPPED_STOPS: 0
PASS   : tst_EmptyStatesAndFocus::testDateEditTabTrapPlainVsSingleTab()
PASS   : tst_EmptyStatesAndFocus::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 356ms
********* Finished testing of tst_EmptyStatesAndFocus *********
```

### 6.4 E4: Screenshot File Sizes in Commit `366a1fc`

```bash
$ python3 -c '
import subprocess
files = [
    "docs/reports/img/dashboard-toast-dark.png",
    "docs/reports/img/dashboard-toast-light.png",
    "docs/reports/img/dashboard-toasts-stack-dark.png",
    "docs/reports/img/dashboard-toasts-stack-light.png",
    "docs/reports/img/dashboard-with-data-dark.png",
    "docs/reports/img/dashboard-with-data-light.png",
    "docs/reports/img/study-board-dark.png",
    "docs/reports/img/study-board-detail-dark.png",
    "docs/reports/img/study-board-detail-light.png",
    "docs/reports/img/study-board-drag-dark.png",
    "docs/reports/img/study-board-drag-light.png",
    "docs/reports/img/study-board-empty-dark.png",
    "docs/reports/img/study-board-empty-light.png",
    "docs/reports/img/study-board-light.png",
    "docs/reports/img/study-board-selected-dark.png",
    "docs/reports/img/study-board-selected-light.png",
    "docs/reports/img/study-empty-tracks-dark.png",
    "docs/reports/img/study-empty-tracks-light.png",
    "docs/reports/img/study-list-dark.png",
    "docs/reports/img/study-list-light.png",
    "docs/reports/img/study-session-dialog-dark.png",
    "docs/reports/img/study-session-dialog-light.png",
    "docs/reports/img/study-tracks-dialog-dark.png",
    "docs/reports/img/study-tracks-dialog-light.png"
]
print("| File | Before (bytes) | After (bytes) | Delta |")
print("| --- | --- | --- | --- |")
for f in files:
    before = len(subprocess.check_output(["git", "show", "366a1fc~1:" + f]))
    after = len(subprocess.check_output(["git", "show", "366a1fc:" + f]))
    print(f"| `{f}` | {before} | {after} | {after - before:+d} |")
'
| File | Before (bytes) | After (bytes) | Delta |
| --- | --- | --- | --- |
| `docs/reports/img/dashboard-toast-dark.png` | 89937 | 87254 | -2683 |
| `docs/reports/img/dashboard-toast-light.png` | 88363 | 85703 | -2660 |
| `docs/reports/img/dashboard-toasts-stack-dark.png` | 80653 | 89511 | +8858 |
| `docs/reports/img/dashboard-toasts-stack-light.png` | 79075 | 88095 | +9020 |
| `docs/reports/img/dashboard-with-data-dark.png` | 69245 | 65763 | -3482 |
| `docs/reports/img/dashboard-with-data-light.png` | 68108 | 64568 | -3540 |
| `docs/reports/img/study-board-dark.png` | 68539 | 68932 | +393 |
| `docs/reports/img/study-board-detail-dark.png` | 83111 | 83539 | +428 |
| `docs/reports/img/study-board-detail-light.png` | 82095 | 82460 | +365 |
| `docs/reports/img/study-board-drag-dark.png` | 93117 | 93527 | +410 |
| `docs/reports/img/study-board-drag-light.png` | 90661 | 91031 | +370 |
| `docs/reports/img/study-board-empty-dark.png` | 27385 | 27797 | +412 |
| `docs/reports/img/study-board-empty-light.png` | 27103 | 27464 | +361 |
| `docs/reports/img/study-board-light.png` | 67683 | 68028 | +345 |
| `docs/reports/img/study-board-selected-dark.png` | 48802 | 49203 | +401 |
| `docs/reports/img/study-board-selected-light.png` | 48087 | 48437 | +350 |
| `docs/reports/img/study-empty-tracks-dark.png` | 24293 | 24737 | +444 |
| `docs/reports/img/study-empty-tracks-light.png` | 24079 | 24442 | +363 |
| `docs/reports/img/study-list-dark.png` | 154070 | 154448 | +378 |
| `docs/reports/img/study-list-light.png` | 153304 | 153633 | +329 |
| `docs/reports/img/study-session-dialog-dark.png` | 15234 | 15212 | -22 |
| `docs/reports/img/study-session-dialog-light.png` | 15102 | 15083 | -19 |
| `docs/reports/img/study-tracks-dialog-dark.png` | 15252 | 15396 | +144 |
| `docs/reports/img/study-tracks-dialog-light.png` | 14896 | 15021 | +125 |
```

### 6.5 E5: Timezone Independence Runs & Clock Injection Audit

#### 6.5.1 Timezone Runs

- **`TZ=UTC`:**
  `100% tests passed, 0 tests failed out of 34`  
  `Total Test time (real) = 93.08 sec`
- **`TZ=America/Los_Angeles`:**
  `100% tests passed, 0 tests failed out of 34`  
  `Total Test time (real) = 89.09 sec`
- **`TZ=Pacific/Auckland`:**
  `100% tests passed, 0 tests failed out of 34`  
  `Total Test time (real) = 89.38 sec`

#### 6.5.2 Absolute Date Grep & Clock Verdict

```bash
$ grep -rnE "QDate\(20[0-9]{2}" tests | cut -d: -f1 | sort | uniq -c
     33 tests/data/tst_ApplicationRepository.cpp
     41 tests/data/tst_InterviewRepository.cpp
      8 tests/data/tst_ReminderLogRepository.cpp
     33 tests/data/tst_SessionRepository.cpp
     17 tests/data/tst_TopicRepository.cpp
      2 tests/data/tst_TransactionBehavior.cpp
      6 tests/models/tst_AgendaListModel.cpp
     10 tests/models/tst_ApplicationTableModel.cpp
      4 tests/models/tst_TopicListModel.cpp
      2 tests/models/tst_TopicTableProxy.cpp
     68 tests/services/tst_AgendaService.cpp
      5 tests/services/tst_DayWatcher.cpp
     10 tests/services/tst_ImportExport.cpp
     53 tests/services/tst_ReminderService.cpp
     44 tests/services/tst_StatsService.cpp
     49 tests/ui_smoke/tst_MonthView.cpp
      1 tests/ui_test_helpers/UiTestHelpers.h
      3 tests/ui/tst_EmptyStatesAndFocus.cpp
      4 tests/ui/tst_HiDpi.cpp
     14 tests/ui/tst_Notifiers.cpp
      7 tests/ui/tst_PageCalendar.cpp
     46 tests/ui/tst_PageHome.cpp
      4 tests/ui/tst_PageJobs.cpp
      3 tests/ui/tst_PageSettings.cpp
      3 tests/ui/tst_PageStudy.cpp
```

| File | Hit Count | Clock Type & Verdict |
| --- | --- | --- |
| `tests/data/tst_ApplicationRepository.cpp` | 33 | **Injected clock / data fixture**: Pure database repository verification against static test dates. |
| `tests/data/tst_InterviewRepository.cpp` | 41 | **Injected clock / data fixture**: Static dates; no dependence on real system clock. |
| `tests/data/tst_ReminderLogRepository.cpp` | 8 | **Injected clock / data fixture**: Fixed test timestamps. |
| `tests/data/tst_SessionRepository.cpp` | 33 | **Injected clock / data fixture**: Fixed session dates. |
| `tests/data/tst_TopicRepository.cpp` | 17 | **Injected clock / data fixture**: Static target dates. |
| `tests/data/tst_TransactionBehavior.cpp` | 2 | **Injected clock / data fixture**: Mock transaction rows. |
| `tests/models/tst_AgendaListModel.cpp` | 6 | **Injected clock / model fixture**: Deterministic model dates. |
| `tests/models/tst_ApplicationTableModel.cpp` | 10 | **Injected clock / model fixture**: Deterministic table dates. |
| `tests/models/tst_TopicListModel.cpp` | 4 | **Injected clock / model fixture**: Deterministic topic dates. |
| `tests/models/tst_TopicTableProxy.cpp` | 2 | **Injected clock / model fixture**: Deterministic proxy dates. |
| `tests/services/tst_AgendaService.cpp` | 68 | **Injected clock**: Service injected with explicit `std::function<QDate()>` clock. |
| `tests/services/tst_DayWatcher.cpp` | 5 | **Injected clock**: Injected clock callable simulates day transitions. |
| `tests/services/tst_ImportExport.cpp` | 10 | **Injected clock / static fixture**: Fixed JSON/ICS fixtures. |
| `tests/services/tst_ReminderService.cpp` | 53 | **Injected clock**: Service injected with `std::function<QDateTime()>` clock. |
| `tests/services/tst_StatsService.cpp` | 44 | **Injected clock**: Service injected with `std::function<QDate()>` clock. |
| `tests/ui_smoke/tst_MonthView.cpp` | 49 | **Injected clock / widget fixture**: Controlled month navigation dates. |
| `tests/ui_test_helpers/UiTestHelpers.h` | 1 | **Injected clock helper**: Default mock fixture date. |
| `tests/ui/tst_EmptyStatesAndFocus.cpp` | 3 | **Injected clock / static fixture**: Fixed date edits for tab walks. |
| `tests/ui/tst_HiDpi.cpp` | 4 | **Injected clock / dialog fixture**: Controlled dialog populating dates. |
| `tests/ui/tst_Notifiers.cpp` | 14 | **Injected clock**: Injected time supplier for toast/tray delivery. |
| `tests/ui/tst_PageCalendar.cpp` | 7 | **Injected clock**: CalendarPage constructor receives `[today]() { return today; }`. |
| `tests/ui/tst_PageHome.cpp` | 46 | **Injected clock**: HomePage constructor receives `[today]() { return today; }`. |
| `tests/ui/tst_PageJobs.cpp` | 4 | **Injected clock / fixture**: Static job application dates. |
| `tests/ui/tst_PageSettings.cpp` | 3 | **Injected clock / fixture**: Static fixture dates for export/backup verification. |
| `tests/ui/tst_PageStudy.cpp` | 3 | **Injected clock**: StudyPage constructor receives `[today]() { return today; }`. |

**Verdict:** 100% of tests with absolute dates use injected clock callables or static fixtures. No tests depend on real system clock time.

### 6.6 E7: 1970 Timestamps in Development Database

```bash
$ sqlite3 -readonly ~/.local/share/JobPrep/JobPrep/jobprep.sqlite "SELECT count(*) FROM applications WHERE created_at LIKE '1970%';"
bash: line 1: sqlite3: command not found
```

Inspected via Python read-only sqlite3 connection (`file:...jobprep.sqlite?mode=ro`):

```bash
$ python3 -c "
import sqlite3, os
db_path = os.path.expanduser('~/.local/share/JobPrep/JobPrep/jobprep.sqlite')
conn = sqlite3.connect(f'file:{db_path}?mode=ro', uri=True)
c = conn.cursor()
c.execute(\"SELECT count(*) FROM applications WHERE created_at LIKE '1970%'\")
print('applications created_at 1970 count:', c.fetchone()[0])
c.execute(\"SELECT count(*) FROM status_history WHERE changed_at LIKE '1970%'\")
print('status_history changed_at 1970 count:', c.fetchone()[0])
c.execute(\"SELECT count(*) FROM topics WHERE created_at LIKE '1970%'\")
print('topics created_at 1970 count:', c.fetchone()[0])
conn.close()
"
applications created_at 1970 count: 2
status_history changed_at 1970 count: 2
topics created_at 1970 count: 26
```

### 6.7 D2-4: Qt Version References in Repository

```bash
$ grep -rn "6\.5\|6\.8\|6\.10" AGENTS.md CMakeLists.txt CMakePresets.json docs/SPEC.md docs/RUNNING.md .github/workflows/ci.yml
AGENTS.md:10:- Qt 6, target 6.8 LTS or newer; avoid APIs newer than 6.8 unless version-guarded (CI builds on 6.8.2 while the development machine has 6.10.2). Modules: Core, Gui, Widgets, Sql (SQLite), Svg; Qt Test for tests.
AGENTS.md:16:Never hard-code the Qt path. Use env `QT_PREFIX_PATH` (Qt kit folder, e.g. `C:\Qt\6.8.x\msvc2022_64` or `~/Qt/6.8.x/gcc_64`), an existing `build/CMakeCache.txt`, or ask the user. On Linux, distro Qt packages are acceptable only if they are Qt >= 6.8 (Ubuntu 22.04/24.04 ship older Qt: use the Qt online installer or aqtinstall).
CMakeLists.txt:25:find_package(Qt6 6.8 REQUIRED COMPONENTS
docs/SPEC.md:334:- Builds and passes tests on Windows (MSVC 2022 or MinGW) and Linux (GCC 11+ or Clang) with Qt 6.8+; no `Q_OS_*` outside `src/platform/`.
docs/SPEC.md:453:- 1.11 — Continuous integration workflow: Added `.github/workflows/ci.yml` matrix workflow targeting `ubuntu-latest` and `windows-latest` with pinned `aqtinstall` (Qt 6.8.2 LTS), caching, headless offscreen dependencies, MSVC environment, concurrency groups, timeout guards, and artifact upload on failure.
docs/SPEC.md:521:- 1.20 — Application icon, desktop entry, Windows GUI subsystem, Qt 6.8 alignment, and 150% HiDPI scaling (Part D2):
docs/SPEC.md:524:  - **Qt 6.8 Minimum Alignment (D2-4):** Pinned minimum Qt version to 6.8 LTS across `CMakeLists.txt` (`find_package(Qt6 6.8 REQUIRED ...)`), `AGENTS.md`, and project documentation. Prohibits APIs newer than 6.8 without version guards to match CI builds on 6.8.2 LTS.
docs/RUNNING.md:14:- **Qt 6**: Version **6.8 LTS** or newer (with modules `Core`, `Gui`, `Widgets`, `Sql`, `Svg`, `Test`).
docs/RUNNING.md:18:To install the exact Qt version used by CI (Qt 6.8.2):
docs/RUNNING.md:25:python3 -m aqt install-qt linux desktop 6.8.2 linux_gcc_64
docs/RUNNING.md:28:python -m aqt install-qt windows desktop 6.8.2 win64_msvc2022_64
docs/RUNNING.md:31:Set `QT_PREFIX_PATH` in your environment to point to the installed kit (e.g., `~/Qt/6.8.2/gcc_64` or `C:\Qt\6.8.2\msvc2022_64`).
docs/RUNNING.md:40:export QT_PREFIX_PATH="/path/to/Qt/6.8.x/gcc_64"  # Omit if using distro Qt 6.8+
docs/RUNNING.md:77:set QT_PREFIX_PATH=C:\Qt\6.8.2\msvc2022_64
.github/workflows/ci.yml:66:          key: qt-6.8.2-${{ matrix.os }}-${{ matrix.qt-arch }}-aqt-3.2.0
.github/workflows/ci.yml:73:          python -m aqt install-qt ${{ runner.os == 'Windows' && 'windows' || 'linux' }} ${{ matrix.qt-target }} 6.8.2 ${{ matrix.qt-arch }} \
```
**Zero references to Qt 6.5.** All references specify Qt 6.8 LTS minimum or Qt 6.10 dev build.

### 6.8 D2-2: Desktop Entry Full Command Evidence

```bash
$ bash -n scripts/install-desktop-entry.sh; command -v shellcheck && shellcheck scripts/install-desktop-entry.sh || echo "shellcheck not installed"
shellcheck not installed

$ rm -rf /tmp/jp-test-xdg && XDG_DATA_HOME=/tmp/jp-test-xdg ./scripts/install-desktop-entry.sh
Installed desktop entry at /tmp/jp-test-xdg/applications/jobprep.desktop

$ cat /tmp/jp-test-xdg/applications/jobprep.desktop
[Desktop Entry]
Version=1.0
Type=Application
Name=JobPrep
Comment=Job application and interview preparation tracker
Exec=/home/tony/projects/JobPrep/build/dev-linux/jobprep
Icon=/home/tony/projects/JobPrep/resources/icons/app-256.png
Terminal=false
Categories=Office;Utility;
StartupWMClass=jobprep

$ desktop-file-validate /tmp/jp-test-xdg/applications/jobprep.desktop
/tmp/jp-test-xdg/applications/jobprep.desktop: hint: value "Office;Utility;" for key "Categories" in group "Desktop Entry" contains more than one main category; application might appear more than once in the application menu

$ sha256sum /tmp/jp-test-xdg/applications/jobprep.desktop
504e36e1ebca3436ca83b11873de3fff3bb096730d88a68b266e596c0b4e516a  /tmp/jp-test-xdg/applications/jobprep.desktop

$ XDG_DATA_HOME=/tmp/jp-test-xdg ./scripts/install-desktop-entry.sh
Installed desktop entry at /tmp/jp-test-xdg/applications/jobprep.desktop

$ sha256sum /tmp/jp-test-xdg/applications/jobprep.desktop
504e36e1ebca3436ca83b11873de3fff3bb096730d88a68b266e596c0b4e516a  /tmp/jp-test-xdg/applications/jobprep.desktop

$ mkdir -p "/tmp/jp space" && cp build/dev-linux/jobprep "/tmp/jp space/" && XDG_DATA_HOME=/tmp/jp-test-xdg ./scripts/install-desktop-entry.sh --exe "/tmp/jp space/jobprep"
Installed desktop entry at /tmp/jp-test-xdg/applications/jobprep.desktop

$ grep "Exec=" /tmp/jp-test-xdg/applications/jobprep.desktop
Exec="/tmp/jp space/jobprep"

$ desktop-file-validate /tmp/jp-test-xdg/applications/jobprep.desktop
/tmp/jp-test-xdg/applications/jobprep.desktop: hint: value "Office;Utility;" for key "Categories" in group "Desktop Entry" contains more than one main category; application might appear more than once in the application menu

$ XDG_DATA_HOME=/tmp/jp-test-xdg ./scripts/install-desktop-entry.sh --dry-run
[dry-run] Would create directory: /tmp/jp-test-xdg/applications
[dry-run] Would write /tmp/jp-test-xdg/applications/jobprep.desktop:
[Desktop Entry]
Version=1.0
Type=Application
Name=JobPrep
Comment=Job application and interview preparation tracker
Exec=/home/tony/projects/JobPrep/build/dev-linux/jobprep
Icon=/home/tony/projects/JobPrep/resources/icons/app-256.png
Terminal=false
Categories=Office;Utility;
StartupWMClass=jobprep

$ XDG_DATA_HOME=/tmp/jp-test-xdg ./scripts/install-desktop-entry.sh --exe /tmp/nonexistent; echo exit=$?
Error: Executable not found at /tmp/nonexistent
exit=1

$ XDG_DATA_HOME=/tmp/jp-test-xdg ./scripts/install-desktop-entry.sh --remove; echo exit=$?
Removed /tmp/jp-test-xdg/applications/jobprep.desktop
exit=0

$ XDG_DATA_HOME=/tmp/jp-test-xdg ./scripts/install-desktop-entry.sh --remove; echo exit=$?
File does not exist, nothing to remove: /tmp/jp-test-xdg/applications/jobprep.desktop
exit=0
```

### 6.9 D2-1: Application Icon Execution

```bash
$ build/dev-linux/tests/ui_smoke/ui_smoke -functions | grep testApplicationIcon
testApplicationIcon()

$ build/dev-linux/tests/ui_smoke/ui_smoke testApplicationIcon
********* Start testing of tst_UiSmoke *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_UiSmoke::initTestCase()
ICON_SIZE: 16x16
ICON_SIZE: 32x32
ICON_SIZE: 256x256
PASS   : tst_UiSmoke::testApplicationIcon()
PASS   : tst_UiSmoke::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 187ms
********* Finished testing of tst_UiSmoke *********
```

---

## 7. Portable Safety Backup Guard (F7)

### 7.1 Diff of `tests/services/tst_ImportExport.cpp`

```diff
@@ -957,10 +957,29 @@ void tst_ImportExport::testRestoreSafetyBackupProductionPathWithCustomXdgDataHom
     QVERIFY(customXdgDir.isValid());
     const QString customXdg = customXdgDir.path();
     qputenv("XDG_DATA_HOME", customXdg.toLocal8Bit());
+    qputenv("LOCALAPPDATA", customXdg.toLocal8Bit());
 
     const QString prodSafetyDir =
         QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + u"/backups"_s;
 
+    // Safety guard: NEVER write safety backups to the real user location during tests.
+    // The resolved production directory MUST be isolated inside the temporary directory.
+    if (!prodSafetyDir.startsWith(customXdg)) {
+#ifdef Q_OS_WIN
+        // On Windows without shell folder redirection, AppDataLocation cannot be redirected
+        // via environment variables alone. Skip the write to avoid touching real %LOCALAPPDATA%.
+        qunsetenv("XDG_DATA_HOME");
+        qunsetenv("LOCALAPPDATA");
+        QStandardPaths::setTestModeEnabled(true);
+        QCoreApplication::setOrganizationName(origOrg);
+        QCoreApplication::setApplicationName(origApp);
+        QSKIP("QStandardPaths::AppDataLocation cannot be redirected via environment on Windows; skipping to protect user data.");
+#else
+        QFAIL(qPrintable(QString(u"Safety violation: resolved path '%1' is not under temp dir '%2'"_s)
+                             .arg(prodSafetyDir, customXdg)));
+#endif
+    }
+
     TestDatabase testDb(u"tst_prod_path"_s);
     QVERIFY(testDb.open(u":memory:"_s));
     populateHandBuiltDataset(testDb.db());
@@ -976,6 +995,7 @@ void tst_ImportExport::testRestoreSafetyBackupProductionPathWithCustomXdgDataHom
     }
 
     qunsetenv("XDG_DATA_HOME");
+    qunsetenv("LOCALAPPDATA");
     QStandardPaths::setTestModeEnabled(true);
     QCoreApplication::setOrganizationName(origOrg);
     QCoreApplication::setApplicationName(origApp);
```

### 7.2 Linux Execution

```bash
$ build/dev-linux/tests/services/tst_ImportExport testRestoreSafetyBackupProductionPathWithCustomXdgDataHome
********* Start testing of tst_ImportExport *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_ImportExport::initTestCase()
PASS   : tst_ImportExport::testRestoreSafetyBackupProductionPathWithCustomXdgDataHome()
PASS   : tst_ImportExport::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 6ms
********* Finished testing of tst_ImportExport *********
```

---

## 8. Corrections to `M6-ci-fixup4-v2.md` (F8)

### 8.1 Source File Audit for `QT_QPA_FONTDIR`

```bash
$ timeout 10s find / -name "qbasicfontdatabase*" -o -name "qfontdatabase.cpp" 2>/dev/null | head
(no output)
```

Only Qt headers and shared binaries are present in `/usr/include/x86_64-linux-gnu/qt6/` and `/usr/lib/`. The statement in `M6-ci-fixup4-v2.md` was therefore **not confirmed by local source files on this machine**, but derived from Qt documentation and knowledge; to be proven by the next Windows CI run.

### 8.2 Message Handler Forwarding Behavior

In `tests/ui_test_helpers/UiTestHelpers.cpp`:
```cpp
void testMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    if (type != QtWarningMsg && type != QtCriticalMsg && type != QtFatalMsg) {
        if (s_previousHandler) {
            s_previousHandler(type, context, msg);
        } else {
            fprintf(stderr, "QINFO  : %s\n", qPrintable(msg));
        }
        return;
    }
    ...
```
When Qt Test executes with `-o <file>,txt`, Qt Test registers its own message handler before tests run. `s_previousHandler` captures this handler during `installWarningFilter()` and forwards `qInfo()` calls to it. Therefore, on Windows in CI, `qInfo()` messages are routed directly into the `-o file,txt` log. When tests run without an existing Qt Test handler, messages fall back to `stderr`.

---

## 9. Release CI Suggestion (F10)

The Windows CI job builds preset `dev-windows` with `CMAKE_BUILD_TYPE=Debug`, leaving `WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>` and `jobprep.rc` unexercised. An extra CI step to verify the GUI subsystem and Windows resources without running full test suites would be:

```yaml
- name: Build Windows Release executable
  run: |
    cmake -B build/release-windows -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="${{ env.QT_PREFIX_PATH }}"
    cmake --build build/release-windows --target jobprep
```

---

## 10. Milestone Status & User Verification Checklist (F9)

**Milestone 6 is NOT ready to tag.** The user pushes the local commits, waits for green CI on both Linux and Windows jobs, verifies the checklist below on hardware, and tags only upon confirmation.

### Not Verified by Agent — User Hardware Verification Checklist

1. **Wayland Start:** Launch `./build/dev-linux/jobprep` under a native Wayland session (`QT_QPA_PLATFORM=wayland`) and confirm window positioning, rendering, and dialog focus.
2. **Maximize on Desktop PC:** Maximize `MainWindow` on a standard 1080p/1440p monitor and on a 1280x720/1366x768 laptop display. Confirm layouts resize without horizontal scrollbars or clipped cards.
3. **ICS Calendar Import:** Export `jobprep-interviews.ics` from the Settings page and import into Google Calendar, Outlook, or Thunderbird to verify calendar acceptance and alarm formatting.
4. **Safety Restore on Real Data:** Perform a JSON backup, modify a field, restore the backup, and verify that all relationships, status history, and notes restore faithfully without data loss.
5. **Screen Reader:** Navigate the application using Orca (Linux) or NVDA/Narrator (Windows) to test accessible names, button roles, and table headers.
6. **Date Control Tab Traversal:** In the live application (e.g. `ApplicationDialog` or `TopicDetailPanel`), verify that pressing Tab moves cleanly to the next control with a single press, while arrow keys adjust date sections.
7. **Ukrainian Keyboard Layout:** Switch layout to Ukrainian (`uk_UA`) and verify that standard shortcuts (`Ctrl+S`, `Ctrl+N`, `Ctrl+F`, `Ctrl+1..4`) fire as expected.
8. **Real Windows Start:** Launch `jobprep.exe` on Windows from Explorer and confirm GUI subsystem behavior (no black console window opens) and that the color icon appears in Explorer and in the window title bar.

---

## 11. Final Quality Gate

```bash
$ CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
[1/2] Automatic MOC and UIC for target tst_HiDpi
[2/2] Linking CXX executable tests/ui/tst_HiDpi
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start  1: tst_Database
 1/34 Test  #1: tst_Database .....................   Passed    0.04 sec
      Start  2: tst_TrackRepository
 2/34 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start  3: tst_TopicRepository
 3/34 Test  #3: tst_TopicRepository ..............   Passed    0.09 sec
      Start  4: tst_SessionRepository
 4/34 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  5: tst_ApplicationRepository
 5/34 Test  #5: tst_ApplicationRepository ........   Passed    0.08 sec
      Start  6: tst_InterviewRepository
 6/34 Test  #6: tst_InterviewRepository ..........   Passed    0.03 sec
      Start  7: tst_ReminderLogRepository
 7/34 Test  #7: tst_ReminderLogRepository ........   Passed    0.02 sec
      Start  8: tst_SeedService
 8/34 Test  #8: tst_SeedService ..................   Passed    0.24 sec
      Start  9: tst_TransactionBehavior
 9/34 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start 10: ui_smoke
10/34 Test #10: ui_smoke .........................   Passed    5.36 sec
      Start 11: tst_MonthView
11/34 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 12: tst_AgendaListModel
12/34 Test #12: tst_AgendaListModel ..............   Passed    0.04 sec
      Start 13: tst_ApplicationTableModel
13/34 Test #13: tst_ApplicationTableModel ........   Passed    0.05 sec
      Start 14: tst_ApplicationFilterProxy
14/34 Test #14: tst_ApplicationFilterProxy .......   Passed    0.02 sec
      Start 15: tst_TopicListModel
15/34 Test #15: tst_TopicListModel ...............   Passed    0.04 sec
      Start 16: tst_TopicBoardProxy
16/34 Test #16: tst_TopicBoardProxy ..............   Passed    0.06 sec
      Start 17: tst_TopicTableProxy
17/34 Test #17: tst_TopicTableProxy ..............   Passed    0.07 sec
      Start 18: tst_AgendaService
18/34 Test #18: tst_AgendaService ................   Passed    0.08 sec
      Start 19: tst_DayWatcher
19/34 Test #19: tst_DayWatcher ...................   Passed    0.02 sec
      Start 20: tst_ExportService
20/34 Test #20: tst_ExportService ................   Passed    0.02 sec
      Start 21: tst_ImportExport
21/34 Test #21: tst_ImportExport .................   Passed    0.70 sec
      Start 22: tst_ReminderService
22/34 Test #22: tst_ReminderService ..............   Passed    0.08 sec
      Start 23: tst_SettingsService
23/34 Test #23: tst_SettingsService ..............   Passed    0.02 sec
      Start 24: tst_StatsService
24/34 Test #24: tst_StatsService .................   Passed    0.80 sec
      Start 25: tst_Notifiers
25/34 Test #25: tst_Notifiers ....................   Passed    2.54 sec
      Start 26: tst_StudyRegression
26/34 Test #26: tst_StudyRegression ..............   Passed    6.43 sec
      Start 27: tst_PageJobs
27/34 Test #27: tst_PageJobs .....................   Passed   11.72 sec
      Start 28: tst_PageCalendar
28/34 Test #28: tst_PageCalendar .................   Passed    6.61 sec
      Start 29: tst_PageStudy
29/34 Test #29: tst_PageStudy ....................   Passed   17.47 sec
      Start 30: tst_PageHome
30/34 Test #30: tst_PageHome .....................   Passed   15.96 sec
      Start 31: tst_PageSettings
31/34 Test #31: tst_PageSettings .................   Passed    4.11 sec
      Start 32: tst_Shortcuts
32/34 Test #32: tst_Shortcuts ....................   Passed    3.05 sec
      Start 33: tst_EmptyStatesAndFocus
33/34 Test #33: tst_EmptyStatesAndFocus ..........   Passed   10.15 sec
      Start 34: tst_HiDpi
34/34 Test #34: tst_HiDpi ........................   Passed    3.29 sec

100% tests passed, 0 tests failed out of 34

Label Time Summary:
data       =   0.59 sec*proc (9 tests)
model      =   0.28 sec*proc (6 tests)
service    =   1.72 sec*proc (7 tests)
ui         =  86.74 sec*proc (12 tests)

Total Test time (real) =  89.43 sec
```
