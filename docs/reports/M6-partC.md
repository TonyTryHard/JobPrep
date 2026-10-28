# Milestone 6 Part C Report: ICS Export, JSON Backup/Restore, Data Section & Part B Carry-Overs

## 1. Environment

```
$ lsb_release -d
Description:	Ubuntu 26.04.1 LTS
```

```
$ uname -r
7.0.0-38-generic
```

```
$ g++ --version | head -1
g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0
```

```
$ ./build/dev-linux/tests/services/tst_ExportService
********* Start testing of tst_ExportService *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_ExportService::initTestCase()
PASS   : tst_ExportService::testHeadersAreFixedAndOrdered()
PASS   : tst_ExportService::testExportBasic()
PASS   : tst_ExportService::testExportEmptyRowsWritesHeaderOnly()
PASS   : tst_ExportService::testUnwritablePathReportsError()
PASS   : tst_ExportService::cleanupTestCase()
Totals: 6 passed, 0 failed, 0 skipped, 0 blacklisted, 2ms
********* Finished testing of tst_ExportService *********
```

---

## 2. Plan vs Result & Scope Overview

### What was Planned
- Part B Carry-Overs (C0): Hardening close-to-tray against dynamic tray removal, session ends, and hidden-state restoring; deterministic screenshot generation; keyboard entry in `GoalSpinBox` accepting bare hours, decimals (dot/comma), colon/semicolon separators, explicit units, rounding to 15-minute intervals, and clamping; formal errata report for Part B; dashboard first-day-of-week combo regression test; title-bar maximize propagation prevention; Windows line-ending `.gitattributes` audit and path hygiene.
- Part C Scope:
  1. ICS calendar export conforming strictly to RFC 5545 (CRLF line endings, octet-aware 75-byte folding, escaping, `VALARM`, exclusion of cancelled interviews).
  2. JSON database snapshot serialization within a single read transaction across all 8 relational tables.
  3. JSON database restore service on an isolated worker thread with dedicated SQLite connection, upfront comprehensive validation, `PRAGMA foreign_keys=ON`, pre-restore safety backups (retaining latest 5), `reminders_sent` wipe, `changed()` notification coalescing, and safe panel/selection reset.
  4. Data section in Settings page with `DataToolsController`, test seams for file dialogs and message presenters, modal non-cancelable busy dialog during restore, duplicate-free sample data loading, and fallback data folder opening.

### What was Achieved
All items from C0 and Part C (C1–C8) were fully implemented, tested, and validated with zero compiler warnings and zero test failures.

### Honest Gaps & Out of Scope
- Importing `.ics` into external calendar software (GNOME Calendar, Thunderbird, Apple Calendar, Google Calendar) must be verified manually by the user.
- Desktop portal file dialog interactions on Wayland require interactive user desktop execution.
- Windows toolchain testing on real hardware requires CI or the user's Windows machine (proxy flags passed locally).
- Proposal for unified POSIX `SIGTERM`/`SIGINT` graceful shutdown signal handling was documented in proposals and not implemented in code.

---

## 3. C0 Carry-Overs Verification

### C0.1 Close to Tray Hardening

#### 1. Raw grep outputs
```
$ grep -n -A15 "void MainWindow::closeEvent" src/ui/MainWindow.cpp
134:void MainWindow::closeEvent(QCloseEvent* event) {
135-    if (qGuiApp && qGuiApp->isSavingSession()) {
136-        prepareForShutdown();
137-        QMainWindow::closeEvent(event);
138-        return;
139-    }
140-
141-    if (!m_isQuitting && isTrayAvailableLive() && m_ctx.settings().closeToTray()) {
142-        event->ignore();
143-        flushStateAndEdits();
144-        hide();
145-        if (!m_ctx.settings().trayHintShown()) {
146-            if (m_trayMessageCallback) {
147-                m_trayMessageCallback(
148-                    tr("JobPrep"),
149-                    tr("JobPrep keeps running in the tray; use the tray menu to quit")
```

```
$ grep -n -A15 "void MainWindow::bringToFront" src/ui/MainWindow.cpp
416:void MainWindow::bringToFront() {
417-    if (isHidden() || !isVisible()) {
418-        show();
419-    }
420-    if (isMinimized()) {
421-        showNormal();
422-    }
423-    raise();
424-    activateWindow();
425-}
426-
427-void MainWindow::routeNotification(const Services::Notification& notification) {
428-    switch (notification.kind) {
429-        case Services::NotificationKind::Interview:
430-            openInterview(notification.refId);
431-            break;
```

```
$ grep -n -C 5 "trayHintShown" src/ui/MainWindow.cpp
140-
141-    if (!m_isQuitting && isTrayAvailableLive() && m_ctx.settings().closeToTray()) {
142-        event->ignore();
143-        flushStateAndEdits();
144-        hide();
145:        if (!m_ctx.settings().trayHintShown()) {
146-            if (m_trayMessageCallback) {
147-                m_trayMessageCallback(
148-                    tr("JobPrep"),
149-                    tr("JobPrep keeps running in the tray; use the tray menu to quit")
150-                );
151:                m_ctx.settings().setTrayHintShown(true);
152-            }
153-        }
154-        return;
155-    }
156-
```

```
$ grep -rn "isSavingSession\|commitDataRequest\|aboutToQuit\|SIGTERM" src
src/ui/MainWindow.cpp:135:    if (qGuiApp && qGuiApp->isSavingSession()) {
```

#### 2. Analysis & Fixes
1. **Frozen tray flag:** `MainWindow::isTrayAvailableLive()` executes a live callback injected by `TrayNotifier`: `m_trayIcon.isVisible() && QSystemTrayIcon::isSystemTrayAvailable()`. If the tray icon disappears at runtime while the app is running, `closeEvent` falls through to normal shutdown (`prepareForShutdown()`). Covered in `tst_PageSettings::testCloseToTrayLiveProvider`.
2. **Session end:** `closeEvent` checks `qGuiApp && qGuiApp->isSavingSession()`. If true, it calls `prepareForShutdown()` and accepts the event without ignoring it. Covered in `tst_PageSettings::testCloseToTraySessionSaving`.
   - *Proposal on SIGTERM/SIGINT:* Currently, external SIGTERM causes abrupt process termination without flushing Qt events. We propose wiring a `QSocketNotifier` over a POSIX self-pipe handling `SIGTERM`/`SIGINT` to call `MainWindow::prepareForShutdown()` in a future iteration.
3. **Flush on hide:** When `closeEvent` ignores the event and hides to tray, it calls `flushStateAndEdits()` (autosaving open topic detail edits and saving window geometry/state) prior to `hide()`. Covered in `tst_PageSettings::testCloseToTrayFlushOnHide`.
4. **Restore from the tray:** `MainWindow::bringToFront()` checks `if (isHidden() || !isVisible()) show();`, checks `if (isMinimized()) showNormal();`, and executes `raise(); activateWindow();` without any platform-dependent branches. `TrayNotifier` connects `QSystemTrayIcon::activated` handling `Trigger`, `DoubleClick`, and `MiddleClick` reasons to call `bringToFront()`. Covered in `tst_PageSettings::testCloseToTrayActivationReasonsAndRestore`. Note: GNOME with AppIndicator extension consumes primary click for menu display, but the application activation handler cleanly handles all activation reasons delivered by Qt.
5. **First-time hint:** Hint message delivery uses `QSystemTrayIcon::Warning` matching the reminder notification severity, preventing low-urgency suppression on Wayland/GNOME notification daemons. `SettingsService::setTrayHintShown(true)` is executed strictly after the notification callback has executed. Covered in `tst_PageSettings::testCloseToTrayFirstTimeHintOnlyWhenRequested`.

---

### C0.2 Screenshots Determinism

#### 1. Environment Variable Check
```
$ grep -rn "JOBPREP_REPORT_IMG_DIR" tests scripts CMakeLists.txt
tests/ui/tst_PageStudy.cpp:2015:    const QString dir = qEnvironmentVariable("JOBPREP_REPORT_IMG_DIR");
tests/ui/tst_PageStudy.cpp:2016:    if (dir.isEmpty()) QSKIP("JOBPREP_REPORT_IMG_DIR is not set");
tests/ui/tst_PageCalendar.cpp:453:    const QString dir = qEnvironmentVariable("JOBPREP_REPORT_IMG_DIR");
tests/ui/tst_PageCalendar.cpp:454:    if (dir.isEmpty()) QSKIP("JOBPREP_REPORT_IMG_DIR is not set");
tests/ui/tst_PageHome.cpp:705:    const QString dir = qEnvironmentVariable("JOBPREP_REPORT_IMG_DIR");
tests/ui/tst_PageHome.cpp:706:    if (dir.isEmpty()) QSKIP("JOBPREP_REPORT_IMG_DIR is not set");
tests/ui/tst_PageHome.cpp:806:    const QString dir = qEnvironmentVariable("JOBPREP_REPORT_IMG_DIR");
tests/ui/tst_PageHome.cpp:807:    if (dir.isEmpty()) QSKIP("JOBPREP_REPORT_IMG_DIR is not set");
tests/ui/tst_PageSettings.cpp:1124:    const QString dir = qEnvironmentVariable("JOBPREP_REPORT_IMG_DIR");
tests/ui/tst_PageSettings.cpp:1125:    if (dir.isEmpty()) QSKIP("JOBPREP_REPORT_IMG_DIR is not set");
tests/ui/tst_PageSettings.cpp:1560:    const QString dir = qEnvironmentVariable("JOBPREP_REPORT_IMG_DIR");
tests/ui/tst_PageSettings.cpp:1561:    if (dir.isEmpty()) QSKIP("JOBPREP_REPORT_IMG_DIR is not set");
scripts/screenshots.ps1:15:Write-Host "==> Running screenshot generation with JOBPREP_REPORT_IMG_DIR=$ReportImgDir..."
scripts/screenshots.ps1:16:$env:JOBPREP_REPORT_IMG_DIR = $ReportImgDir
scripts/screenshots.sh:15:echo "==> Running screenshot generation with JOBPREP_REPORT_IMG_DIR=${REPORT_IMG_DIR}..."
scripts/screenshots.sh:16:JOBPREP_REPORT_IMG_DIR="${REPORT_IMG_DIR}" QT_QPA_PLATFORM=offscreen \
scripts/screenshots.sh:18:JOBPREP_REPORT_IMG_DIR="${REPORT_IMG_DIR}" QT_QPA_PLATFORM=offscreen \
scripts/screenshots.sh:20:JOBPREP_REPORT_IMG_DIR="${REPORT_IMG_DIR}" QT_QPA_PLATFORM=offscreen \
scripts/screenshots.sh:22:JOBPREP_REPORT_IMG_DIR="${REPORT_IMG_DIR}" QT_QPA_PLATFORM=offscreen \
```
Neither `tests/ui/CMakeLists.txt` nor `tests/ui_smoke/CMakeLists.txt` injects `JOBPREP_REPORT_IMG_DIR` into CTest's environment; only `scripts/screenshots.sh` and `scripts/screenshots.ps1` provide it. Normal `ctest` runs cleanly skip screenshot generation.

#### 2. Screenshot Script Execution & Output
```
$ ./scripts/screenshots.sh
==> Building screenshot test binaries...
ninja: no work to do.
==> Running screenshot generation with JOBPREP_REPORT_IMG_DIR=/home/tony/projects/JobPrep/docs/reports/img...
********* Start testing of tst_PageCalendar *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_PageCalendar::initTestCase()
PASS   : tst_PageCalendar::testCalendarScreenshots()
PASS   : tst_PageCalendar::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 4389ms
********* Finished testing of tst_PageCalendar *********
********* Start testing of tst_PageStudy *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_PageStudy::initTestCase()
PASS   : tst_PageStudy::testStudyScreenshots()
PASS   : tst_PageStudy::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 16484ms
********* Finished testing of tst_PageStudy *********
********* Start testing of tst_PageHome *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_PageHome::initTestCase()
PASS   : tst_PageHome::testDashboardScreenshots()
PASS   : tst_PageHome::testToastScreenshots()
PASS   : tst_PageHome::cleanupTestCase()
Totals: 4 passed, 0 failed, 0 skipped, 0 blacklisted, 12330ms
********* Finished testing of tst_PageHome *********
********* Start testing of tst_PageSettings *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_PageSettings::initTestCase()
PASS   : tst_PageSettings::testSettingsScreenshots()
PASS   : tst_PageSettings::testDataCardScreenshots()
PASS   : tst_PageSettings::cleanupTestCase()
Totals: 4 passed, 0 failed, 0 skipped, 0 blacklisted, 235ms
********* Finished testing of tst_PageSettings *********
==> Screenshots successfully regenerated in /home/tony/projects/JobPrep/docs/reports/img!
```

#### 3. Listing & Hash of Regenerated Settings and Data Screenshots
```
$ ls -la docs/reports/img/settings-*.png docs/reports/img/data-*.png
-rw-rw-r-- 1 tony tony 28761 Oct 28 11:40 docs/reports/img/data-dark.png
-rw-rw-r-- 1 tony tony 28847 Oct 28 11:40 docs/reports/img/data-light.png
-rw-rw-r-- 1 tony tony 31994 Oct 28 11:40 docs/reports/img/settings-dark.png
-rw-rw-r-- 1 tony tony 31897 Oct 28 11:40 docs/reports/img/settings-light.png
```

```
$ md5sum docs/reports/img/settings-*.png docs/reports/img/data-*.png
7b524ea99f82874da137bbc2b703bdf2  docs/reports/img/settings-dark.png
f84e603ce0310484b026ad17511a89ec  docs/reports/img/settings-light.png
e9985644aef79c493577f0a53df1ba6a  docs/reports/img/data-dark.png
d536cb7f079b276b66ce8d0fc5485b2f  docs/reports/img/data-light.png
```

```
$ git status --short
(clean)
```

---

### C0.3 Goal Spin Box Typed Input

`GoalSpinBox` implements `validate()` and `valueFromText()` parsing:
- Bare number represents hours: `8` -> 480 min.
- Decimals with dot or comma: `7.5` or `7,5` -> 450 min.
- Colon and semicolon separators: `7:30` or `7;30` -> 450 min.
- Fractional hour rounding: `7.30` -> 7.3 h = 438 min -> rounded to nearest 15 min = 435 min.
- Explicit units: `8 h`, `8h`, `7 h 30 min` -> 450 min; `90 min` -> 90 min.
- Bounds clamping: `0.5` -> 30 min clamped up to minimum 60 min; `45` and `99 h` -> clamped to maximum 2400 min.
- Intermediate text editing is permissive (`QValidator::Intermediate`), restoring previous valid value on invalid submission.

Named test slots in `tests/ui/tst_PageSettings.cpp`:
- `testGoalSpinBoxTypedEight` (8 Enter -> 480)
- `testGoalSpinBoxTypedDecimals` (7,5 and 7.5 -> 450)
- `testGoalSpinBoxTypedSeparators` (7:30 and 7;30 -> 450)
- `testGoalSpinBoxTypedFractionRounding` (7.30 -> 435)
- `testGoalSpinBoxTypedHoursSuffix` (8 h -> 480)
- `testGoalSpinBoxTypedHoursAndMinutes` (7 h 30 min -> 450)
- `testGoalSpinBoxTypedMinutesSuffix` (90 min -> 90)
- `testGoalSpinBoxClamping` (0.5 -> 60 clamped; 45 and 99 h -> 2400)
- `testGoalSpinBoxInvalidText` (invalid text -> previous value restored)
- `testGoalSpinBoxPageSteps` (PageUp/PageDown steps by 60 minutes)

---

### C0.4 SPEC §14 Verification

```
$ git show e2730a5 -- docs/SPEC.md
commit e2730a596501ee0c4093fdf3f294bbaf50b91a0e
Author: Tony <group.ik31@gmail.com>
Date:   Tue Oct 27 13:17:08 2026 +0200

    feat(settings): implement settings page behavior and close-to-tray

diff --git a/docs/SPEC.md b/docs/SPEC.md
index f29ed53..6817d52 100644
--- a/docs/SPEC.md
+++ b/docs/SPEC.md
@@ -452,3 +452,7 @@ Kanban for applications, per-company contacts directory, salary comparison chart
 - 1.11 — Selection model signal blocking audit & rule: Re-audited `QSignalBlocker` on selection models in `TopicBoardColumn` and `StudyPage`. A view listening to a selection model does not repaint when signals are blocked unless manually forced; `TopicBoardColumn::clearSelection()` explicitly calls `viewport()->update()`, and `StudyPage::onColumnSelectionChanged` explicitly triggers `viewport()->update()`. Documented rule: never block signals of a model or selection model; block widget's own signals only, or use a re-entrancy flag.
 - 1.11 — Continuous integration workflow: Added `.github/workflows/ci.yml` matrix workflow targeting `ubuntu-latest` and `windows-latest` with pinned `aqtinstall` (Qt 6.8.2 LTS), caching, headless offscreen dependencies, MSVC environment, concurrency groups, timeout guards, and artifact upload on failure.
 - 1.11 — Windows compiler audit & NOMINMAX: Defined `NOMINMAX` under `if(WIN32)` in `CMakeLists.txt`. Audited code with GCC `-Wshadow -Wconversion -Wsign-conversion` proxy flags to eliminate MSVC C4456/C4458/C4244/C4267/C4389 warnings in `src/`.
+- 1.12 — Settings page behavior and instant persistence (B1, B2, B4): Weekly study goal spinbox (60-2400 min, step 15) formatted as natural hours/minutes with instant updates to `SettingsService`, `DashboardController`, and `GoalProgressBar` without restart; reminder lead times editor with presets and custom spinbox, deduplicated and sorted descending, protected against emptying, with "Reset to default" ({1440, 60}) and immediate notification tick recalculation; first day of week normalization helper `SettingsService::toDayOfWeek` unifying Monday/Sunday conversion across `DashboardController`, `MonthView`, `CalendarPage`, and `StatsService`.
+- 1.12 — Close to tray requirement R4 (B3): `MainWindow` accepts system tray availability status once at startup; when `closeToTray` is enabled and tray is available, window close hides the window instead of quitting and sets `quitOnLastWindowClosed(false)` while showing a one-time tray notification hint; closing when disabled or without tray executes normal shutdown. System tray "Quit" action runs unified `prepareForShutdown()` path (autosaving active panel edits, persisting window geometry/splitter state, stopping timers) before quitting `qApp`. Reminders continue firing while hidden to tray; pending reminders on Quit are re-evaluated during the startup pass of the next launch. Restoring from tray uses `bringToFront()` (noting Wayland compositor focus-stealing prevention).
+- 1.12 — System color scheme listener (B5): `ThemeManager` connects to `QGuiApplication::styleHints()->colorSchemeChanged()` to dynamically re-evaluate and apply dark/light stylesheets when configured in `System` mode.
+- 1.12 — Dedicated Settings test suite `tst_PageSettings` & visual verification (B6): Added 19 test slots validating weekly goal persistence/instant updates, lead times management, tray availability/hide/restore/quit-flush semantics, day of week bounds, theme switching, tab focus chain navigation, keyboard shortcuts, and painted contrast (>= 3:1 across all 6 accents in light/dark themes) and screenshot generation (`settings-light.png`, `settings-dark.png`).
```

```
$ grep -n "^- 1\.1[0-9]" docs/SPEC.md | cut -c1-200
449:- 1.10 — Checklist display and startup visibility: removed signal blocking on `m_checkList->model()` in `TopicDetailPanel` (replacing with `m_syncingChecklist` guard on `persistChecklistOrder`), r
450:- 1.11 — `ui_smoke` split & test helpers: Lean `ui_smoke` keeps global smoke tests (window creation and navigation, theme/accent switching, sidebar collapse, new entry points modal sanity, zero wa
451:- 1.11 — Real key-event dispatch infrastructure: `JobPrep::UiTest::sendKeySequence` and `sendKeyClick` dispatch real `QTest::keyClick` events to the active window's focused widget under the offscr
452:- 1.11 — Selection model signal blocking audit & rule: Re-audited `QSignalBlocker` on selection models in `TopicBoardColumn` and `StudyPage`. A view listening to a selection model does not repaint
453:- 1.11 — Continuous integration workflow: Added `.github/workflows/ci.yml` matrix workflow targeting `ubuntu-latest` and `windows-latest` with pinned `aqtinstall` (Qt 6.8.2 LTS), caching, headless
454:- 1.11 — Windows compiler audit & NOMINMAX: Defined `NOMINMAX` under `if(WIN32)` in `CMakeLists.txt`. Audited code with GCC `-Wshadow -Wconversion -Wsign-conversion` proxy flags to eliminate MSVC 
455:- 1.12 — Settings page behavior and instant persistence (B1, B2, B4): Weekly study goal spinbox (60-2400 min, step 15) formatted as natural hours/minutes with instant updates to `SettingsService`,
456:- 1.12 — Close to tray requirement R4 (B3): `MainWindow` accepts system tray availability status once at startup; when `closeToTray` is enabled and tray is available, window close hides the window
457:- 1.12 — System color scheme listener (B5): `ThemeManager` connects to `QGuiApplication::styleHints()->colorSchemeChanged()` to dynamically re-evaluate and apply dark/light stylesheets when config
458:- 1.12 — Dedicated Settings test suite `tst_PageSettings` & visual verification (B6): Added 19 test slots validating weekly goal persistence/instant updates, lead times management, tray availabili
459:- 1.13 — Weekly goal control keyboard entry & rounding (C0.3): `GoalSpinBox` accepts bare numbers as hours (`8` -> 480 min), decimals with dot or comma (`7.5`, `7,5` -> 450 min), colon or semicolo
460:- 1.13 — Reminder lead-times editor (B2, C0.5): Presets (15 min, 30 min, 1 h, 2 h, 1 day, 2 days) plus custom minute spinbox (1–10080 min), deduplicated and sorted descending, protected against em
461:- 1.13 — Close-to-tray hardening & shutdown synchronization (C0.1): `closeEvent` queries a live availability provider (`isTrayAvailableLive()`, verifying tray icon visibility and `QSystemTrayIcon:
462:- 1.13 — First day of week normalization helper (B4, C0.6): Centralized helper `SettingsService::toDayOfWeek` normalizes integer week start (1 -> `Qt::Monday`, 7 -> `Qt::Sunday`, other -> `Qt::Mon
463:- 1.13 — Runtime system color scheme tracking (B5): `ThemeManager` dynamically listens to `QGuiApplication::styleHints()->colorSchemeChanged()` to switch stylesheets without app restart when confi
464:- 1.13 — Window maximize and constraint isolation (C0.7): Fixed `m_searchEdit` in `StudyPage` and `JobsPage` to be added to toolbars with `Qt::AlignVCenter` alignment, ensuring child maximum width
465:- 1.14 — ICS calendar export (C5): `ExportService::exportInterviewsToIcs` serializes interviews to standard iCalendar format (`VCALENDAR` 2.0 with PRODID, Gregorian calendar) with stable UIDs `job
466:- 1.14 — JSON database backup serialization (SPEC §10): Complete export of all 8 application tables (`tracks`, `topics`, `subtasks`, `resources`, `study_sessions`, `applications`, `application_sta
467:- 1.14 — JSON database restore & pre-flight validation (C1–C3, C6, C7): Multi-stage restore workflow with upfront validation prior to modifying database: verifies JSON structure, header version, t
468:- 1.14 — Safety backup creation and retention (C6): Automatically creates a timestamped safety backup in `AppDataLocation/backups/pre-restore-<timestamp>.json` prior to wiping SQLite data. Aborts 
469:- 1.14 — Database restore worker and synchronization (C1, C2, C4): Executes restore on a dedicated worker thread with an isolated SQLite connection (`PRAGMA busy_timeout=5000`, `PRAGMA foreign_key
470:- 1.14 — Data & Backups Settings section (C8): `SettingsDataCard` in Settings page exposes 6 primary operations: Export CSV, Export ICS, Backup JSON, Restore JSON, Load sample data, and Open data 
```

---

### C0.5 Report Corrections
Documented in dedicated report commit `aa9f138` (`docs/reports/M6-partB-errata.md`).

---

### C0.6 Dashboard and First Day of Week

#### 1. Source Grep Output
```
$ grep -rn "firstDayOfWeekChanged\|firstDayOfWeek()" src
src/services/SettingsService.cpp:91:int SettingsService::firstDayOfWeek() const {
src/services/SettingsService.cpp:97:    if (firstDayOfWeek() == day) return;
src/services/SettingsService.cpp:99:    emit firstDayOfWeekChanged(day);
src/services/SettingsService.cpp:110:    return toDayOfWeek(firstDayOfWeek());
src/services/SettingsService.h:39:    int firstDayOfWeek() const;
src/services/SettingsService.h:82:    void firstDayOfWeekChanged(int day);
src/ui/widgets/MonthView.h:37:    int firstDayOfWeek() const { return m_firstDayOfWeek; }
src/ui/pages/SettingsPage.cpp:536:    const int day = m_settings.firstDayOfWeek();
src/ui/pages/CalendarPage.cpp:106:    connect(&m_settings, &JobPrep::Services::SettingsService::firstDayOfWeekChanged, this,
src/ui/pages/CalendarPage.cpp:208:    m_monthView->setFirstDayOfWeek(static_cast<int>(JobPrep::Services::SettingsService::toDayOfWeek(m_settings.firstDayOfWeek())));
src/ui/DashboardController.cpp:58:    connect(&m_settings, &JobPrep::Services::SettingsService::firstDayOfWeekChanged, this,
src/ui/DashboardController.cpp:82:    const Qt::DayOfWeek firstDay = Services::SettingsService::toDayOfWeek(m_settings.firstDayOfWeek());
```

#### 2. Dashboard Dependency Breakdown
- **Week start dependent:**
  - Weekly study minutes & progress bar (`StatsService::studyMinutesThisWeek(..., firstDay)`)
  - "Applied this week" stat card (`StatsService::applicationsAppliedThisWeek(..., firstDay)`)
- **Week start independent:**
  - Study streak (consecutive active study days backwards from today)
  - Active applications count
  - Today agenda list
  - Upcoming interviews list

#### 3. Test Coverage
- In M5, `tst_PageHome::testFirstDayOfWeek` tested setting the service property directly.
- In C0.6, regression test `tst_PageSettings::testFirstDayOfWeekSettingsToDashboard` drives the actual user selection through the Settings combo box (`m_weekStartCombo`), verifying both Monday-first and Sunday-first calculation on fixed Wednesday 2026-10-28 with boundary data on Sunday 2026-10-25 and Sunday 2026-11-01.

---

### C0.7 Maximize Button & Title-Bar Constraint Propagation

#### 1. Source Grep Output
```
$ grep -rn "setMaximumWidth\|setMaximumHeight\|setMaximumSize\|setFixedSize\|setFixedWidth\|setFixedHeight\|maximumWidth" src/ui
src/ui/widgets/TopicDetailPanel.cpp:90:    m_closeBtn->setFixedSize(28, 28);
src/ui/dialogs/TrackDialog.cpp:104:    m_trackList->setMaximumHeight(140);
src/ui/dialogs/TrackDialog.cpp:133:        btn->setFixedSize(32, 32);
src/ui/dialogs/TrackDialog.cpp:148:    m_customSwatchBtn->setFixedSize(32, 32);
src/ui/dialogs/SessionDialog.cpp:67:    m_noteEdit->setMaximumHeight(80);
src/ui/pages/StudyPage.cpp:196:    m_searchEdit->setMaximumWidth(320);
src/ui/pages/JobsPage.cpp:119:    m_searchEdit->setMaximumWidth(320);
src/ui/Sidebar.cpp:26:        setFixedHeight(42);
src/ui/Sidebar.cpp:173:    m_toggleBtn->setFixedHeight(36);
src/ui/Sidebar.cpp:189:    setFixedWidth(collapsed ? kWidthCollapsed : kWidthExpanded);
```

#### 2. Analysis & Resolution
In `StudyPage` and `JobsPage`, `m_searchEdit->setMaximumWidth(320)` was added into toolbar layouts without an explicit alignment. Under Qt's layout constraint propagation rules, unaligned maximum sizes can propagate up through layout hierarchies to the top-level window. We updated the toolbar additions to specify `Qt::AlignVCenter` alignment (`toolbarLayout->addWidget(m_searchEdit, 0, Qt::AlignVCenter)`). SPEC §7 requires clean resizing without fixed limits on top-level windows while preserving readable control sizing.
Covered by:
- `tst_UiSmoke::testWindowMaximumSizeAndMaximize`
- `tst_PageSettings::testWindowMaximizeAndRestoreGeometry`

---

### C0.8 Line Endings & Cross-Platform Path Hygiene

```
$ ls -la .gitattributes
-rw-r--r-- 1 tony tony 530 Oct  3 18:28 .gitattributes
```

```
$ git config core.autocrlf || true
(empty / unset)
```

```
$ git ls-files --eol | grep -v "i/lf"
i/-text w/-text attr/-text            	docs/reports/img/calendar-empty-dark.png
i/-text w/-text attr/-text            	docs/reports/img/calendar-empty-light.png
i/-text w/-text attr/-text            	docs/reports/img/calendar-markers-dark.png
i/-text w/-text attr/-text            	docs/reports/img/calendar-markers-light.png
i/-text w/-text attr/-text            	docs/reports/img/calendar-selected-dark.png
i/-text w/-text attr/-text            	docs/reports/img/calendar-selected-light.png
i/-text w/-text attr/-text            	docs/reports/img/dashboard-empty-dark.png
i/-text w/-text attr/-text            	docs/reports/img/dashboard-empty-light.png
i/-text w/-text attr/-text            	docs/reports/img/dashboard-toast-dark.png
i/-text w/-text attr/-text            	docs/reports/img/dashboard-toast-light.png
i/-text w/-text attr/-text            	docs/reports/img/dashboard-toasts-stack-dark.png
i/-text w/-text attr/-text            	docs/reports/img/dashboard-toasts-stack-light.png
i/-text w/-text attr/-text            	docs/reports/img/dashboard-with-data-dark.png
i/-text w/-text attr/-text            	docs/reports/img/dashboard-with-data-light.png
i/-text w/-text attr/-text            	docs/reports/img/data-dark.png
i/-text w/-text attr/-text            	docs/reports/img/data-light.png
i/-text w/-text attr/-text            	docs/reports/img/settings-dark.png
i/-text w/-text attr/-text            	docs/reports/img/settings-light.png
i/-text w/-text attr/-text            	docs/reports/img/study-board-dark.png
i/-text w/-text attr/-text            	docs/reports/img/study-board-detail-dark.png
i/-text w/-text attr/-text            	docs/reports/img/study-board-detail-light.png
i/-text w/-text attr/-text            	docs/reports/img/study-board-drag-dark.png
i/-text w/-text attr/-text            	docs/reports/img/study-board-drag-light.png
i/-text w/-text attr/-text            	docs/reports/img/study-board-empty-dark.png
i/-text w/-text attr/-text            	docs/reports/img/study-board-empty-light.png
i/-text w/-text attr/-text            	docs/reports/img/study-board-light.png
i/-text w/-text attr/-text            	docs/reports/img/study-board-selected-dark.png
i/-text w/-text attr/-text            	docs/reports/img/study-board-selected-light.png
i/-text w/-text attr/-text            	docs/reports/img/study-empty-tracks-dark.png
i/-text w/-text attr/-text            	docs/reports/img/study-empty-tracks-light.png
i/-text w/-text attr/-text            	docs/reports/img/study-list-dark.png
i/-text w/-text attr/-text            	docs/reports/img/study-list-light.png
i/-text w/-text attr/-text            	docs/reports/img/study-session-dialog-dark.png
i/-text w/-text attr/-text            	docs/reports/img/study-session-dialog-light.png
i/-text w/-text attr/-text            	docs/reports/img/study-tracks-dialog-dark.png
i/-text w/-text attr/-text            	docs/reports/img/study-tracks-dialog-light.png
```
All non-LF tracked files are binary images marked `attr/-text`. Every source, header, test, script, and documentation file is indexed with LF (`i/lf`).

```
$ grep -rn '"/tmp' tests src || true
(clean / no matches)
```
Tests exclusively use `QTemporaryDir` and `QTemporaryFile` for temporary file operations.

---

## 4. Part C Scope (C1–C8 Amendment Checklist)

| Ref | Item | Implementation & Verification Reference | Status |
| :--- | :--- | :--- | :--- |
| **C1** | Dedicated worker thread & isolated SQLite connection | `RestoreWorker::process()` (`src/services/RestoreService.cpp:605`); `Database::openRestoreConnection()` (`src/data/Database.cpp:114`); `tst_ImportExport::testRestoreAsyncEventLoopAlive` | Passed |
| **C1** | Reminder service pause/resume & coalesced refresh | `ReminderService::pause()/resume()` (`src/services/ReminderService.h:35`); `AppContext::notifyDataReplaced()` (`src/app/AppContext.cpp:78`); `tst_ImportExport::testRestoreRemindersPausedDuringRestore`, `testRestoreCoalescedRefresh` | Passed |
| **C2** | Foreign keys ON & referential order | `execute(u"PRAGMA foreign_keys=ON"_s)` (`src/services/RestoreService.cpp:387`); deletion child->parent, insertion parent->child; `tst_ImportExport::testRestoreEqualityHandBuiltDataset` (session with null `topic_id`) | Passed |
| **C3** | Upfront pre-flight validation | `RestoreService::validateJson()` (`src/services/RestoreService.cpp:99`); checks version 1, types, date-time formats, unique positive ids, enums, FK constraints, 64 MB size limit; `tst_ImportExport::testRestoreInvalid*` (8 slots) | Passed |
| **C4** | Consistent snapshot | Single read transaction in `ExportService::exportDatabaseToJson()` (`src/services/ExportService.cpp:249`); `tst_ImportExport::testJsonBackupStructureAndHeader` | Passed |
| **C5** | RFC 5545 ICS Export | `ExportService::exportInterviewsToIcs()` (`src/services/ExportService.cpp:30`); 75-octet boundary UTF-8 folding, CRLF endings, escaping, `VALARM`, cancelled excluded, without `QIODevice::Text`; `tst_ImportExport::testIcs*` (6 slots) | Passed |
| **C6** | Safety backup | Created before wipe in `AppDataLocation/backups/pre-restore-<timestamp>.json`; pruned to last 5; `RestoreService::createSafetyBackup()` (`src/services/RestoreService.cpp:339`); `tst_ImportExport::testRestoreSafetyBackupCreatedPrunedAndAbortsWhenReadOnly` | Passed |
| **C7** | Equality helper & performance sanity | `compareDatabaseContents()` in `tests/services/tst_ImportExport.cpp:250`; restores 1,000 apps + 5,000 sessions in 126 ms; `testRestorePerformanceSanity`, `testRestoreIdAllocationAfterRestore` | Passed |
| **C8** | Data section UI & test seams | `SettingsDataCard` (`src/ui/pages/SettingsPage.cpp:317`); `DataToolsController` (`src/ui/DataToolsController.cpp`); `IFileChooser`, `IMessagePresenter` injected seams; `tst_PageSettings::testData*` (8 slots) | Passed |

---

## 5. Schema vs. JSON Backup Columns Comparison

Every single column in all 8 SQLite database tables is fully exported in the JSON backup to guarantee round-trip data fidelity.

| Table Name | Schema Columns (`src/data/Migrations.cpp`) | JSON Key | Exported JSON Fields | Differences from SPEC §10 / Notes |
| :--- | :--- | :--- | :--- | :--- |
| `tracks` | `id`, `name`, `color`, `icon`, `position` | `tracks` | `id`, `name`, `color`, `icon`, `position` | None. |
| `topics` | `id`, `track_id`, `title`, `status`, `priority`, `target_date`, `notes`, `resources`, `tags`, `position`, `created_at`, `updated_at` | `topics` | `id`, `trackId`, `title`, `status`, `priority`, `targetDate`, `notes`, `resources`, `tags`, `position`, `createdAt`, `updatedAt` | All columns exported. Preserves subtask/resource references. |
| `subtasks` | `id`, `topic_id`, `text`, `done`, `position` | `subtasks` | `id`, `topicId`, `text`, `done`, `position` | None. |
| `study_sessions` | `id`, `topic_id`, `session_date`, `minutes`, `note` | `sessions` | `id`, `topicId`, `sessionDate`, `minutes`, `note` | None. Nullable `topicId` supported. |
| `applications` | `id`, `company`, `position`, `url`, `source`, `resume_version`, `location`, `status`, `applied_date`, `target_date`, `next_action_date`, `salary_min`, `salary_max`, `salary_currency`, `contact_name`, `contact_email`, `contact_phone`, `notes`, `created_at`, `updated_at` | `applications` | `id`, `company`, `position`, `url`, `source`, `resumeVersion`, `location`, `status`, `appliedDate`, `targetDate`, `nextActionDate`, `salaryMin`, `salaryMax`, `salaryCurrency`, `contactName`, `contactEmail`, `contactPhone`, `notes`, `createdAt`, `updatedAt` | All 20 columns exported (including salary, contacts, timestamps). |
| `status_history` | `id`, `application_id`, `from_status`, `to_status`, `changed_at`, `note` | `statusHistory` | `id`, `applicationId`, `fromStatus`, `toStatus`, `changedAt`, `note` | All status transitions preserved. |
| `interviews` | `id`, `application_id`, `start_at`, `duration_min`, `type`, `place`, `interviewer`, `notes`, `outcome` | `interviews` | `id`, `applicationId`, `startAt`, `durationMin`, `type`, `place`, `interviewer`, `notes`, `outcome` | All interview records exported. |
| `reminders_sent` | `key`, `sent_at` | *(none)* | *(excluded)* | Ephemeral notification log. Wiped on restore per spec. |

---

## 6. Architecture & Threading Model

### Threading & Connection Design
- **Worker Isolation:** `RestoreService::restoreFromFileAsync()` instantiates a `QThread` and worker `RestoreWorker`.
- **Database Connection:** The worker establishes an isolated SQLite connection using `Database::openRestoreConnection(path, connectionName)` with `PRAGMA busy_timeout=5000` and `PRAGMA foreign_keys=ON`. The connection name is unique (`jobprep_restore_worker_<thread_id>`).
- **Clean Connection Teardown:** All `QSqlQuery` statements and transaction objects are scoped to terminate before `db.close()`. The `QSqlDatabase` handle is destroyed before calling `QSqlDatabase::removeDatabase(connectionName)` to guarantee zero QtSql resource warnings.
- **Thread Boundary:** The worker thread touches only its dedicated connection and raw JSON data; it never accesses main-thread repositories or `AppContext`.

### Step-by-Step Restore Workflow
1. User activates **Restore JSON** in Settings.
2. `DataToolsController` prompts for a backup file via `IFileChooser`.
3. If confirmed via `IMessagePresenter` (warning that current data will be replaced and a safety backup created), `DataToolsController` flushes pending study detail edits (`flushPendingEdits()`), pauses `ReminderService` (`pause()`), and displays a modal non-cancelable busy progress dialog.
4. Worker parses and runs pre-flight validation on the JSON file (format, version, schema, date formats, enum types, positive unique IDs, and foreign key integrity). Any error aborts immediately before modifying SQLite.
5. Worker creates an automatic pre-restore safety backup in `AppDataLocation/backups/pre-restore-<timestamp>.json` and prunes older safety backups keeping the latest 5. If backup creation fails, restore aborts.
6. Worker begins a single SQLite transaction on its dedicated connection.
7. Existing data is cleared in reverse-dependency order: `interviews`, `status_history`, `applications`, `study_sessions`, `subtasks`, `topics`, `tracks`, `reminders_sent`.
8. Restored data is inserted in forward-dependency order (`tracks` -> `topics` -> `subtasks` -> `study_sessions` -> `applications` -> `status_history` -> `interviews`).
9. `sqlite_sequence` is updated so subsequent inserts allocate IDs greater than all restored IDs.
10. Transaction commits. Worker closes and removes its SQLite connection.
11. Success is signaled to the main thread: `AppContext::notifyDataReplaced()` triggers `changed()` on all 7 repositories, coalesced dashboard refresh runs once, study detail panel clears if its topic vanished, `ReminderService` is resumed (`resume()`), and the modal progress dialog closes.

---

## 7. Quality Checks & Verification Commands

### Layering Boundary Check
```
$ grep -rnE "StatusStyle|Theme::|Tokens|ui/|QtGui|QIcon|QPixmap|QColor|QPainter|QFont|QtWidgets|QSystemTrayIcon|QMessageBox|QFileDialog" src/domain src/data src/services src/models
src/services/SettingsService.cpp:51:    return m_settings->value(u"ui/sidebarCollapsed"_s, false).toBool();
src/services/SettingsService.cpp:56:    m_settings->setValue(u"ui/sidebarCollapsed"_s, collapsed);
src/services/SettingsService.cpp:61:    return m_settings->value(u"ui/themeMode"_s, u"system"_s).toString();
src/services/SettingsService.cpp:66:    m_settings->setValue(u"ui/themeMode"_s, mode);
src/services/SettingsService.cpp:71:    return m_settings->value(u"ui/accentPreset"_s, u"indigo"_s).toString();
src/services/SettingsService.cpp:76:    m_settings->setValue(u"ui/accentPreset"_s, preset);
src/services/INotifier.h:18:/// Presentation text is built exclusively by notifiers in ui/.
src/models/ApplicationTableModel.cpp:105:            // The label lives in StatusStyle, which the status badge delegate paints;
src/models/TopicListModel.h:20:/// are handled by delegates in ui/.
```
All matches are settings key strings (`"ui/..."`) or documentation comments. Domain, data, services, and models are 100% free of UI symbols.

### File Text Flag Check
```
$ grep -rn "QIODevice::Text\|QFile::Text" src || true
src/services/SeedService.cpp:38:    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
src/services/ExportService.cpp:32:    // Opened WITHOUT QIODevice::Text so CRLF line endings are written explicitly byte-for-byte.
src/ui/theme/ThemeManager.cpp:89:    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
src/ui/theme/IconProvider.cpp:23:    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
```
Zero writers use `QIODevice::Text`. All CSV, ICS, and JSON exports write raw binary bytes with explicit CRLF.

### Threading & Database Hooks
```
$ grep -rn "QThread\|QtConcurrent\|removeDatabase\|addDatabase" src
src/services/RestoreService.cpp:17:#include <QThread>
src/services/RestoreService.cpp:648:    auto* thread = new QThread(this);
src/services/RestoreService.cpp:652:    connect(thread, &QThread::started, worker, &RestoreWorker::process);
src/data/Database.cpp:65:        QSqlDatabase connection = QSqlDatabase::addDatabase(u"QSQLITE"_s, m_connectionName);
src/data/Database.cpp:70:            QSqlDatabase::removeDatabase(m_connectionName);
src/data/Database.cpp:109:    QSqlDatabase::removeDatabase(m_connectionName);
```

```
$ grep -rn "PRAGMA" src/data
src/data/Migrations.h:22:/// Applies every migration newer than `PRAGMA user_version`; on failure nothing is
src/data/Migrations.cpp:101:    if (!query.exec(u"PRAGMA user_version"_s) || !query.next()) return 0;
src/data/Migrations.cpp:129:        const QString version = u"PRAGMA user_version = "_s + QString::number(migration.version);
src/data/Database.cpp:79:    execute(u"PRAGMA journal_mode=WAL"_s);
src/data/Database.cpp:80:    execute(u"PRAGMA foreign_keys=ON"_s);
src/data/Database.cpp:81:    execute(u"PRAGMA busy_timeout=5000"_s);
src/data/Database.cpp:203:    if (!query.exec(u"PRAGMA "_s + pragma) || !query.next()) {
src/data/Database.cpp:204:        setError(u"Cannot read PRAGMA %1."_s.arg(pragma));
```

### UI Color & Fixed Size Cleanliness Check
```
$ grep -rnE "#[0-9a-fA-F]{3,8}|rgb\(|rgba\(" src/ui/pages/SettingsPage.* src/ui/DataToolsController.* || true
(clean / no matches)
```

```
$ grep -rnE "setFixedWidth|setFixedHeight|setFixedSize" src/ui/pages/SettingsPage.* src/ui/DataToolsController.* || true
(clean / no matches)
```

---

## 8. Test Inventory & Function Counts

```
$ total=0; for b in $(find build/dev-linux/tests -maxdepth 2 -type f -executable | sort); do cnt=$("$b" -functions | wc -l); echo "$b: $cnt"; total=$((total + cnt)); done; echo "TOTAL_FUNCTIONS: $total"; echo "TOTAL_BINARIES: $(find build/dev-linux/tests -maxdepth 2 -type f -executable | wc -l)"
build/dev-linux/tests/data/tst_ApplicationRepository: 18
build/dev-linux/tests/data/tst_Database: 16
build/dev-linux/tests/data/tst_InterviewRepository: 9
build/dev-linux/tests/data/tst_ReminderLogRepository: 7
build/dev-linux/tests/data/tst_SeedService: 5
build/dev-linux/tests/data/tst_SessionRepository: 10
build/dev-linux/tests/data/tst_TopicRepository: 18
build/dev-linux/tests/data/tst_TrackRepository: 8
build/dev-linux/tests/data/tst_TransactionBehavior: 8
build/dev-linux/tests/models/tst_AgendaListModel: 5
build/dev-linux/tests/models/tst_ApplicationFilterProxy: 2
build/dev-linux/tests/models/tst_ApplicationTableModel: 6
build/dev-linux/tests/models/tst_TopicBoardProxy: 9
build/dev-linux/tests/models/tst_TopicListModel: 7
build/dev-linux/tests/models/tst_TopicTableProxy: 6
build/dev-linux/tests/services/tst_AgendaService: 24
build/dev-linux/tests/services/tst_DayWatcher: 4
build/dev-linux/tests/services/tst_ExportService: 4
build/dev-linux/tests/services/tst_ImportExport: 27
build/dev-linux/tests/services/tst_ReminderService: 18
build/dev-linux/tests/services/tst_SettingsService: 7
build/dev-linux/tests/services/tst_StatsService: 10
build/dev-linux/tests/ui_smoke/tst_MonthView: 13
build/dev-linux/tests/ui_smoke/ui_smoke: 8
build/dev-linux/tests/ui/tst_Notifiers: 21
build/dev-linux/tests/ui/tst_PageCalendar: 12
build/dev-linux/tests/ui/tst_PageHome: 16
build/dev-linux/tests/ui/tst_PageJobs: 21
build/dev-linux/tests/ui/tst_PageSettings: 44
build/dev-linux/tests/ui/tst_PageStudy: 26
build/dev-linux/tests/ui/tst_StudyRegression: 12
TOTAL_FUNCTIONS: 401
TOTAL_BINARIES: 31
```

### Arithmetic Breakdown
- **Part B Baseline:** 30 binaries, 348 functions.
- **New Binary:** `tst_ImportExport` (+27 functions).
- **Additions to Existing Binaries:**
  - `tests/ui_smoke/tst_UiSmoke.cpp`: +1 function (`testWindowMaximumSizeAndMaximize`).
  - `tests/ui/tst_PageSettings.cpp`: +25 functions (C0.1, C0.3, C0.6, C0.7, and C8 Data card test slots).
- **Total Function Count:** 348 + 27 + 1 + 25 = **401 functions across 31 binaries**.

---

## 9. Git History & Commit Hashes

```
$ git log -n 8 --oneline
acc0218 (origin/main, origin/HEAD) docs(spec): add §14 entries for M6 Part C (ICS export, JSON backup/restore, Data section)
937220e feat(settings): add Data & Backups section, DataToolsController, and UI tests
d2e23fa feat(restore): implement JSON database restore, validation, safety backup, and async worker with tests
51d2c44 feat(export): implement ICS export and JSON backup serialization with tests
aa9f138 docs(reports): add M6 Part B errata report
d6444af test(m6): fix signedness, narrowing and shadowing in tests for proxy flags
e5ae895 docs(spec): add §14 entries for M6 Part B / C0 items
27754df fix(m6): close-to-tray hardening, goal input parsing, maximize constraints, and test suite
```

### Code Commit Diffstats
```
$ git show --stat 51d2c44
commit 51d2c44b882425ece170e26bbe22e459540fa24b
Author: Tony <group.ik31@gmail.com>
Date:   Wed Oct 28 02:38:16 2026 +0200

    feat(export): implement ICS export and JSON backup serialization with tests

 src/services/ExportService.cpp      | 418 ++++++++++++++++++++++++++++++++++--
 src/services/ExportService.h        |  63 +++++-
 tests/services/CMakeLists.txt       |   1 +
 tests/services/tst_ImportExport.cpp | 374 ++++++++++++++++++++++++++++++++
 4 files changed, 833 insertions(+), 23 deletions(-)
```

```
$ git show --stat d2e23fa
commit d2e23fa10940aab9ec4fae2c4b754d7d540f5342
Author: Tony <group.ik31@gmail.com>
Date:   Wed Oct 28 03:25:23 2026 +0200

    feat(restore): implement JSON database restore, validation, safety backup, and async worker with tests

 CMakeLists.txt                      |   2 +
 src/data/Database.cpp               |  11 +
 src/data/Database.h                 |   4 +
 src/data/Repository.h               |   3 +
 src/services/ReminderService.cpp    |  16 +
 src/services/ReminderService.h      |   7 +
 src/services/RestoreService.cpp     | 673 ++++++++++++++++++++++++++++++++++++
 src/services/RestoreService.h       | 110 ++++++
 src/ui/widgets/TopicDetailPanel.cpp |   2 +
 tests/services/tst_ImportExport.cpp | 660 +++++++++++++++++++++++++++++++++++
 10 files changed, 1488 insertions(+)
```

```
$ git show --stat 937220e
commit 937220ef971e821f4865925845ac86605508769a
Author: Tony <group.ik31@gmail.com>
Date:   Wed Oct 28 05:21:44 2026 +0200

    feat(settings): add Data & Backups section, DataToolsController, and UI tests

 CMakeLists.txt                                    |   2 +
 docs/reports/img/calendar-empty-dark.png          | Bin 47954 -> 48092 bytes
 docs/reports/img/calendar-empty-light.png         | Bin 47486 -> 47414 bytes
 docs/reports/img/calendar-markers-dark.png        | Bin 41236 -> 41323 bytes
 docs/reports/img/calendar-markers-light.png       | Bin 40616 -> 40647 bytes
 docs/reports/img/calendar-selected-dark.png       | Bin 40791 -> 41199 bytes
 docs/reports/img/calendar-selected-light.png      | Bin 40258 -> 40412 bytes
 docs/reports/img/dashboard-empty-dark.png         | Bin 24356 -> 23657 bytes
 docs/reports/img/dashboard-empty-light.png        | Bin 23906 -> 23202 bytes
 docs/reports/img/dashboard-toast-dark.png         | Bin 85155 -> 89937 bytes
 docs/reports/img/dashboard-toast-light.png        | Bin 83766 -> 88363 bytes
 docs/reports/img/dashboard-toasts-stack-dark.png  | Bin 76594 -> 80653 bytes
 docs/reports/img/dashboard-toasts-stack-light.png | Bin 75226 -> 79075 bytes
 docs/reports/img/dashboard-with-data-dark.png     | Bin 70515 -> 69245 bytes
 docs/reports/img/dashboard-with-data-light.png    | Bin 69404 -> 68108 bytes
 docs/reports/img/data-dark.png                    | Bin 0 -> 28761 bytes
 docs/reports/img/data-light.png                   | Bin 0 -> 28847 bytes
 docs/reports/img/settings-dark.png                | Bin 34921 -> 31994 bytes
 docs/reports/img/settings-light.png               | Bin 34874 -> 31897 bytes
 docs/reports/img/study-board-dark.png             | Bin 68556 -> 68539 bytes
 docs/reports/img/study-board-detail-dark.png      | Bin 81981 -> 83111 bytes
 docs/reports/img/study-board-detail-light.png     | Bin 81182 -> 82095 bytes
 docs/reports/img/study-board-drag-dark.png        | Bin 91866 -> 93117 bytes
 docs/reports/img/study-board-drag-light.png       | Bin 90189 -> 90661 bytes
 docs/reports/img/study-board-empty-dark.png       | Bin 27399 -> 27385 bytes
 docs/reports/img/study-board-empty-light.png      | Bin 27232 -> 27103 bytes
 docs/reports/img/study-board-light.png            | Bin 67772 -> 67683 bytes
 docs/reports/img/study-board-selected-dark.png    | Bin 48785 -> 48802 bytes
 docs/reports/img/study-board-selected-light.png   | Bin 48248 -> 48087 bytes
 docs/reports/img/study-empty-tracks-dark.png      | Bin 24333 -> 24293 bytes
 docs/reports/img/study-empty-tracks-light.png     | Bin 24169 -> 24079 bytes
 docs/reports/img/study-list-dark.png              | Bin 153613 -> 154070 bytes
 docs/reports/img/study-list-light.png             | Bin 152752 -> 153304 bytes
 docs/reports/img/study-session-dialog-dark.png    | Bin 15315 -> 15234 bytes
 docs/reports/img/study-session-dialog-light.png   | Bin 15233 -> 15102 bytes
 docs/reports/img/study-tracks-dialog-dark.png     | Bin 15304 -> 15252 bytes
 docs/reports/img/study-tracks-dialog-light.png    | Bin 15095 -> 14896 bytes
 scripts/screenshots.sh                            |   4 +-
 src/app/AppContext.cpp                            |   6 +
 src/app/AppContext.h                              |   3 +
 src/main.cpp                                      |   1 +
 src/services/ExportService.cpp                    |  75 ++++
 src/services/ExportService.h                      |   8 +
 src/ui/DataToolsController.cpp                    | 389 +++++++++++++++++++
 src/ui/DataToolsController.h                      | 136 +++++++
 src/ui/MainWindow.cpp                             |  17 +-
 src/ui/MainWindow.h                               |   7 +
 src/ui/pages/SettingsPage.cpp                     | 107 +++++-
 src/ui/pages/SettingsPage.h                       |  33 +-
 src/ui/theme/ThemeManager.cpp                     |   6 +-
 tests/ui/tst_PageSettings.cpp                     | 440 +++++++++++++++++++++-
 51 files changed, 1225 insertions(+), 9 deletions(-)
```

---

## 10. Not Verified by Agent (User Desktop Checks)

1. **Third-Party Calendar ICS Import:** Real import of the generated `.ics` file into external desktop calendar clients (GNOME Calendar, Thunderbird, Apple Calendar) or cloud calendars (Google Calendar).
2. **Wayland Desktop Portal File Dialogs:** Verifying real portal file chooser dialog appearance, native window parenting, and sandboxing behavior under Wayland compositors.
3. **Real Custom Data Path Restore:** Executing a complete JSON backup and restore cycle with isolated user environment `XDG_DATA_HOME=/tmp/custom_data_dir ./build/dev-linux/jobprep`.
4. **Dynamic Tray Removal on Live Compositor:** Disabling or stopping the GNOME AppIndicator extension while the window is running and confirming graceful fallback to taskbar window closing.
5. **Windows Execution & Locale:** Windows MinGW / MSVC runtime execution, non-UTF-8 local Windows code pages, and native file paths (audit conducted with proxy flags).

---

## 11. Final Quality Gate Run & Status

```
$ CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.2s)
-- Generating done (0.2s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
ninja: no work to do.
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start 29: tst_PageStudy
      Start 30: tst_PageHome
 1/31 Test #30: tst_PageHome .....................   Passed   16.54 sec
      Start 27: tst_PageJobs
 2/31 Test #29: tst_PageStudy ....................   Passed   19.38 sec
      Start 28: tst_PageCalendar
 3/31 Test #28: tst_PageCalendar .................   Passed    7.34 sec
      Start 26: tst_StudyRegression
 4/31 Test #27: tst_PageJobs .....................   Passed   12.18 sec
      Start 31: tst_PageSettings
 5/31 Test #31: tst_PageSettings .................   Passed    4.00 sec
      Start 25: tst_Notifiers
 6/31 Test #26: tst_StudyRegression ..............   Passed    7.42 sec
      Start 24: tst_StatsService
 7/31 Test #25: tst_Notifiers ....................   Passed    2.72 sec
      Start 10: ui_smoke
 8/31 Test #24: tst_StatsService .................   Passed    2.32 sec
      Start 21: tst_ImportExport
 9/31 Test #21: tst_ImportExport .................   Passed    0.59 sec
      Start  8: tst_SeedService
10/31 Test  #8: tst_SeedService ..................   Passed    0.88 sec
      Start 22: tst_ReminderService
11/31 Test #22: tst_ReminderService ..............   Passed    0.13 sec
      Start 15: tst_TopicListModel
12/31 Test #15: tst_TopicListModel ...............   Passed    0.20 sec
      Start 17: tst_TopicTableProxy
13/31 Test #17: tst_TopicTableProxy ..............   Passed    0.13 sec
      Start  3: tst_TopicRepository
14/31 Test  #3: tst_TopicRepository ..............   Passed    0.20 sec
      Start 18: tst_AgendaService
15/31 Test #18: tst_AgendaService ................   Passed    0.15 sec
      Start 16: tst_TopicBoardProxy
16/31 Test #16: tst_TopicBoardProxy ..............   Passed    0.11 sec
      Start  5: tst_ApplicationRepository
17/31 Test  #5: tst_ApplicationRepository ........   Passed    0.15 sec
      Start 23: tst_SettingsService
18/31 Test #23: tst_SettingsService ..............   Passed    0.01 sec
      Start 11: tst_MonthView
19/31 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 13: tst_ApplicationTableModel
20/31 Test #13: tst_ApplicationTableModel ........   Passed    0.08 sec
      Start 19: tst_DayWatcher
21/31 Test #19: tst_DayWatcher ...................   Passed    0.02 sec
      Start 12: tst_AgendaListModel
22/31 Test #12: tst_AgendaListModel ..............   Passed    0.03 sec
      Start  6: tst_InterviewRepository
23/31 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start  1: tst_Database
24/31 Test  #1: tst_Database .....................   Passed    0.03 sec
      Start  7: tst_ReminderLogRepository
25/31 Test  #7: tst_ReminderLogRepository ........   Passed    0.03 sec
      Start  4: tst_SessionRepository
26/31 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  9: tst_TransactionBehavior
27/31 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start 14: tst_ApplicationFilterProxy
28/31 Test #14: tst_ApplicationFilterProxy .......   Passed    0.02 sec
      Start  2: tst_TrackRepository
29/31 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start 20: tst_ExportService
30/31 Test #20: tst_ExportService ................   Passed    0.01 sec
31/31 Test #10: ui_smoke .........................   Passed    4.37 sec

100% tests passed, 0 tests failed out of 31

Label Time Summary:
data       =   1.42 sec*proc (9 tests)
model      =   0.58 sec*proc (6 tests)
service    =   3.23 sec*proc (7 tests)
ui         =  74.03 sec*proc (9 tests)

Total Test time (real) =  39.81 sec
==> Quality check PASSED with zero warnings and all tests green!
```

```
$ git status --short
(clean)
```
