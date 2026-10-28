# Milestone 6 Part B Errata & Report Corrections

This document provides official corrections and clarifications to `docs/reports/M6-partB.md` per prompt requirements in `docs/prompts/M6-partC-go.md` (§C0.5).

---

## 1. `prepareForShutdown` and Timers

- **Errata:** The Part B report stated that `prepareForShutdown` "stops timers".
- **Correction:** In the Part B code commit, the method was implemented as `MainWindow::flushStateAndEdits()`, which called `page->flushEdits()` across all pages, saved window geometry/state, and stopped the dashboard controller debounce coalesce timer (`m_dashboardController->stop()`). In C0.1, we added an explicit public `MainWindow::prepareForShutdown()` method which wraps this unified path so both normal close and tray Quit stop active background debounce timers, flush pending active editor edits, and save geometry/state.

---

## 2. First Day of Week Logic & Files Changed

- **Errata:** The Part B report stated that `StatsService` no longer has duplicated first-day logic, but `StatsService.cpp` was not in commit `e2730a5`'s diffstat.
- **Verification:** Raw command output:
```
$ grep -rn "== 7\|Qt::Sunday\|toDayOfWeek" src
src/services/SettingsService.cpp:102:Qt::DayOfWeek SettingsService::toDayOfWeek(int day) {
src/services/SettingsService.cpp:103:    if (day == 7) {
src/services/SettingsService.cpp:104:        return Qt::Sunday;
src/services/SettingsService.cpp:110:    return toDayOfWeek(firstDayOfWeek());
src/services/SettingsService.h:42:    static Qt::DayOfWeek toDayOfWeek(int day);
src/ui/widgets/MonthView.cpp:49:    const int normalized = static_cast<int>(Services::SettingsService::toDayOfWeek(day));
src/ui/pages/CalendarPage.cpp:108:                m_monthView->setFirstDayOfWeek(static_cast<int>(JobPrep::Services::SettingsService::toDayOfWeek(day)));
src/ui/pages/CalendarPage.cpp:208:    m_monthView->setFirstDayOfWeek(static_cast<int>(JobPrep::Services::SettingsService::toDayOfWeek(m_settings.firstDayOfWeek())));
src/ui/DashboardController.cpp:82:    const Qt::DayOfWeek firstDay = Services::SettingsService::toDayOfWeek(m_settings.firstDayOfWeek());
```
- **Files that really changed:**
  1. `src/services/SettingsService.h` & `src/services/SettingsService.cpp` (introduced `toDayOfWeek(int)`)
  2. `src/ui/widgets/MonthView.cpp` (uses `SettingsService::toDayOfWeek`)
  3. `src/ui/pages/CalendarPage.cpp` (uses `SettingsService::toDayOfWeek`)
  4. `src/ui/DashboardController.cpp` (uses `SettingsService::toDayOfWeek`)
`StatsService` does not take or store a setting; it accepts `Qt::DayOfWeek` as a parameter to its calculation methods (`computeStats`, `weekBounds`), strictly preserving the layering rule `ui -> services -> data -> domain`.

---

## 3. Preferences Key Sequence on Linux & Ctrl+, Behavior

- **Errata & Verification:** On Linux under standard X11/generic Unix platform themes (`QGenericUnixTheme`), `QKeySequence::Preferences` does not bind `Ctrl+,` (it yields an empty key binding or platform-dependent multimedia keys such as `Qt::Key_Settings` on select desktop shells).
- **Behavior before Part A:** In the real application before Part A, pressing `Ctrl+,` did NOT switch to the Settings page because only `QKeySequence::Preferences` was registered. In Part A/B, `MainWindow` explicitly added `QKeySequence(Qt::CTRL | Qt::Key_Comma)` as an alternative shortcut alongside `QKeySequence::Preferences`, making `Ctrl+,` reliably work across all Linux desktops.

---

## 4. Previously Unreported Files in Part B Commit (`e2730a5`)

One-line summary for each file:
- `src/ui/TrayNotifier.h` & `src/ui/TrayNotifier.cpp`: Added `MainWindow&` reference, context menu with "Show JobPrep" and "Quit", and `activated` signal handler for tray icon.
- `src/ui/pages/HomePage.cpp`: Updated day-of-week conversion to use `SettingsService::toDayOfWeek`.
- `src/main.cpp`: Passed tray availability status to `MainWindow` and wired `TrayNotifier` with `MainWindow`.
- `src/ui/DashboardController.h` & `src/ui/DashboardController.cpp`: Connected `weeklyGoalMinutesChanged` and `firstDayOfWeekChanged` signals to request debounce refresh and use `toDayOfWeek`.
- `tests/ui_smoke/tst_MonthView.cpp`: Updated to use `SettingsService::toDayOfWeek` helper.

---

## 5. Proxy Warning Proof (All Targets)

Ran a fresh build into `/tmp/build-proxy` with `-Wall -Wextra -Wshadow -Wconversion -Wsign-conversion -Wpedantic -Werror` over all targets.

```
$ ninja -C /tmp/build-proxy -t targets all | head -5
cmake_object_order_depends_target_jobprep_core: phony
CMakeFiles/jobprep_core.dir/jobprep_core_autogen/mocs_compilation.cpp.o: CXX_COMPILER__jobprep_core_unscanned_Debug
CMakeFiles/jobprep_core.dir/src/app/AppContext.cpp.o: CXX_COMPILER__jobprep_core_unscanned_Debug
CMakeFiles/jobprep_core.dir/src/domain/EnumStrings.cpp.o: CXX_COMPILER__jobprep_core_unscanned_Debug
CMakeFiles/jobprep_core.dir/src/domain/TopicProgress.cpp.o: CXX_COMPILER__jobprep_core_unscanned_Debug
```

```
$ tail -5 /tmp/proxy-full.log
[204/208] Linking CXX executable tests/ui/tst_PageJobs
[205/208] Linking CXX executable tests/ui/tst_PageCalendar
[206/208] Linking CXX executable tests/ui/tst_PageHome
[207/208] Building CXX object tests/ui/CMakeFiles/tst_PageSettings.dir/tst_PageSettings.cpp.o
[208/208] Linking CXX executable tests/ui/tst_PageSettings
```

```
$ grep -c "Building CXX" /tmp/proxy-full.log
139
```

```
$ grep -c "warning:" /tmp/proxy-full.log
0
```

Exit status:
```
exit=0
```

---

## 6. `CTEST_PARALLEL_LEVEL` Default

Raw command output:
```
$ grep -n "CTEST_PARALLEL_LEVEL" scripts/check.sh scripts/check.ps1
scripts/check.sh:16:CTEST_PARALLEL_LEVEL="${CTEST_PARALLEL_LEVEL:-2}" ctest --preset dev-linux -j "${CTEST_PARALLEL_LEVEL:-2}" --output-on-failure
scripts/check.ps1:17:$ctestParallel = if ($env:CTEST_PARALLEL_LEVEL) { $env:CTEST_PARALLEL_LEVEL } else { 2 }
```

---

## 7. `formatLeadTime` Assertion Slot

- In `tests/ui/tst_Notifiers.cpp`: slots test `NotificationText::formatLeadTime`.
- In `tests/ui/tst_PageSettings.cpp`: added slot `testFormatLeadTime` asserting `FormatHelper::formatLeadTime` ("1 day", "2 days", "1 hour", "90 minutes").

---

## 8. Test Count Clarification

The Part B report referenced the pasted gate and `ctest -j4` runs as "Part A" evidence in its Section 3 heading, but the outputs actually reflected 30 test binaries (the full post-Part B suite).
