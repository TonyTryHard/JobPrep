# M6 Part B: go (Settings page behavior, close to tray), with Part A carry-overs

Read `AGENTS.md`, `docs/SPEC.md` (§3.6, §3.7, §7.6, §8, §14), `docs/plans/M6-plan.md`, `docs/prompts/M6-amendments.md` (Part B section B1-B6 and the report rules), `docs/reports/M6-partA.md`, and the code: `src/ui/pages/SettingsPage.*`, `src/ui/MainWindow.*`, `src/ui/ReminderCoordinator.*`, `src/ui/TrayNotifier.*`, `src/ui/theme/ThemeManager.*`, `src/services/SettingsService.*`, `src/ui/DashboardController.*`, `src/ui/pages/CalendarPage.*`, `src/main.cpp`, `tests/ui_smoke/`, the UI test helper library.
Part A is accepted **with the carry-overs in C0**. Do C0 first as its own local commit, then Part B in one or more commits. Local commits only, never push. Prompt and report files go in their own commits.
Do **not** start Part C (export/backup/restore, the Data card) or Part D. Do not add buttons to the Settings page that do nothing yet.

## Report rules (same as before)
1. Every command output is raw, from a real run, exact command above it, nothing trimmed, no `...` placeholders.
2. No line numbers as evidence (`grep -n <symbol>` output and test slot names are fine). Hashes only from `git log --oneline`; report written last and committed separately.
3. Totals by command with the raw loop output; baseline now **29 binaries, 326 functions** (`ui_smoke` 7, `tst_PageHome` 16, `tst_PageJobs` 21, `tst_PageCalendar` 12, `tst_PageStudy` 26).
4. Environment line from commands. Never claim Windows works; say what is verified by running and what is reasoning.
5. Every Part B report ends with the raw output of `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` (green).

## C0. Carry-overs from Part A (one small commit, plus a SPEC fix commit)

1. **Evidence the Part A report did not paste.** Paste raw: (a) the final `scripts/check.sh` output (29 test binaries green); (b) the proxy warning build: the exact configure/build commands in the separate build directory and `grep -c "warning:"` of its log, before and after your fixes; (c) `grep -rn "QIODevice::Text\|QFile::Text\|QIODevice::Text" src`, `grep -rnE "std::(min|max)\(" src | head -20`, `grep -rn "#include <windows.h>" src`; (d) how `/tmp/ui_smoke_baseline_functions.txt` and `/tmp/ui_smoke_split_functions.txt` were produced (the commands), and the `ctest --preset dev-linux -j4 --output-on-failure` command with its raw tail.
2. **Ctrl+1..4 mapping.** The user checked the real app by hand: Ctrl+2 opens Study, Ctrl+3 Jobs, Ctrl+4 Calendar. That is correct (sidebar order Dashboard, Study, Jobs, Calendar; SPEC §3.7, §7.1). But the Part A report says the test asserts Ctrl+2 -> Jobs, Ctrl+3 -> Calendar, Ctrl+4 -> Study. Find out which is wrong, the report text or the test: show the test code for `testPageSwitchShortcutsRealKeyEvents` and `grep -n "Key_1\|m_navItems\|pageIndex" src/ui/MainWindow.cpp src/ui/Sidebar.cpp`. The test must assert the **page identity** (`currentWidget()` is the Study page, by `objectName` or type), not raw stack indexes, and must match the user-verified order for all four keys. No production change is expected; correct the report text in the errata. If the code really does follow stack indexes by accident, say so and add a test that would break if the sidebar order and the stack order ever differ.
3. **SPEC §14 names.** The entry mentions `StudyPage::onSelectionChanged`; the real handler name in the code is probably `onColumnSelectionChanged`. Show `grep -n "onSelectionChanged\|onColumnSelectionChanged" src/ui/pages/StudyPage.h src/ui/pages/StudyPage.cpp` and fix the entry. Also say whether the `Ctrl+,` shortcut existed before Part A (`git log -S"Key_Comma" --oneline -- src/ui/MainWindow.cpp`), because the Part A report presents it as a fix.
4. **Gate time.** The sequential gate only went from about 111 s to 96 s: `check.sh` and `check.ps1` run `ctest` without parallelism. Run ctest in parallel with a default of 2 (`CTEST_PARALLEL_LEVEL` can override; keep the CMake build parallel level as it is), keep `--output-on-failure`, and show the raw gate output and the time before/after. If any test is not `-j` safe, fix the test, not the script.
5. The shared bootstrap recommendation (keep composition in `main.cpp`) is accepted for now. One sentence in the report: which parts of `main.cpp` the test fixtures do **not** exercise (the M5 regression came from exactly such a gap).

## Part B scope (amendments B1-B6; refined below, this file wins on conflict)

**Settings page structure.** Read the current `SettingsPage` first (Appearance card, partial Preferences card). Keep the layout scrollable, cards from tokens, no fixed pixel sizes, tooltips on every control, sensible tab order, every change applies **immediately** (no Apply button, no restart). No Data card in this part.

**B1. Weekly goal.** A spin box in **minutes** (range 60-2400, step 15) whose text reads like the dashboard ("6 h", "6 h 15 min"), keyboard editable, stored in `study/weeklyGoalMinutes`. Not a `QDoubleSpinBox`. Tests: 6 h -> 8 h updates the dashboard bar and text instantly through the existing signal; the value survives a restart (settings file in a temp dir, new `SettingsService` instance); steps of 15 minutes; out-of-range input is clamped.

**B2. Reminder lead times.** Editor with presets (15 min, 30 min, 1 h, 2 h, 1 day, 2 days) plus a custom value in minutes (1-10080); values unique, shown sorted descending, **never empty** (the UI blocks removing the last one), "Reset to default" (1440 and 60). Text for a lead value ("1 day", "2 hours", "90 minutes") comes from one formatter in `ui/` with singular/plural and `tr()`. Applies immediately through `SettingsService::setInterviewLeadMinutes` and the existing signal that reaches `ReminderService::setLeadMinutes` (prove it with a test that changes the lead in the page and checks the next reminder check). Tests for each rule.

**B3. Close to tray (R4).**
- `MainWindow` learns tray availability **once**, from the same decision as `ReminderCoordinator` (the value used by `makeNotifier`); do not create a second `QSystemTrayIcon` and do not call `isSystemTrayAvailable()` in several places. Name the new public method(s) in the report.
- `closeEvent` hides the window only when the setting is on, the tray is available and the app is not quitting; otherwise it closes normally (including the existing autosave flush and geometry saving).
- **Tray "Quit" (currently `qApp->quit`) skips `closeEvent`.** Quit must run the same shutdown path as closing the window: flush pending panel edits (autosave on window close, M4 S7), save geometry and splitter state, stop timers. Test it: edit a topic title without leaving the field, trigger Quit, and check the repository and the saved geometry.
- While hidden to the tray, reminders keep working through the tray notifier. Pending reminders on Quit: the startup pass of the next launch covers them (state this in §14).
- `setQuitOnLastWindowClosed` is handled so that hiding the window does not end the app and closing it with the setting off does.
- A one-time hint when the window is first hidden ("JobPrep keeps running in the tray; use the tray menu to quit"), shown through the tray, remembered in a settings key.
- The setting is disabled with the tooltip "System tray is not available on this system" when there is no tray; if the setting was stored as on and the tray is missing, the window still closes normally.
- Restoring from the tray uses `bringToFront()`; say that Wayland may refuse to raise the window.
- Tests with a fake tray flag, both branches, no real tray needed. List what stays manual: real tray hide/restore on GNOME/Wayland (the user's Ubuntu has the AppIndicator extension).

**B4. First day of week.** One helper converts the stored int to `Qt::DayOfWeek` (Monday = 1, Sunday = 7, anything else -> Monday). Find every place with `grep -rn "firstDayOfWeek" src` and replace the duplicated `== 7 ? Sunday : Monday` logic in the dashboard and calendar paths with the helper. The Settings combo offers exactly Monday and Sunday; a legacy or invalid stored value is shown and treated as Monday. Tests through `StatsService` week bounds, `MonthView` and the dashboard (Sunday-first gives the same week numbers as the Part A dataset: 290 minutes).

**B5. Theme "System".** Check whether `ThemeManager` follows OS color scheme changes at runtime in System mode (`QStyleHints::colorSchemeChanged`, Qt 6.5+). Show the grep. If it does not, implement it. A test may use `QStyleHints::setColorScheme` only behind a Qt version guard (6.8+) and skips otherwise with a clear message.

**B6. Painted tests for the Settings page.** (a) Startup in each saved theme and accent equals the state after switching theme and accent (same painted pixels of the whole page); (b) text/background contrast >= 3:1 on sampled text for all six accents in both themes; (c) screenshots of Settings, light and dark, 100% (extend `scripts/screenshots.sh` and `.ps1`, paste the raw output, `md5sum` of the new images, all different, and `git status --short`). Lesson from the M5 checklist bug: never block signals of a model or selection model, and do not rely on a theme switch to make a widget look right.

## Tests (named slots; zero warnings; gate green)
A new binary `tst_PageSettings` using the UI test helper library (with the CTest labels used by the other page binaries), plus additions to `tst_SettingsService` (lead sanitizing, close-to-tray hint key, first-day normalization) and the dashboard/calendar binaries for B4. List every new slot by name in the report with its binary. Required cases: everything named above, plus: changing theme and accent from the page updates `ThemeManager` and persists; every control reachable by Tab in a documented order; every control has a tooltip; real key events (Space on the checkbox, arrows in the spin box, Enter on "Add lead") through the helper from Part A.

## Checks to show (raw output)
- Layering: `grep -rnE "StatusStyle|Theme::|Tokens|ui/|QtGui|QIcon|QPixmap|QColor|QPainter|QFont|QtWidgets|QSystemTrayIcon" src/domain src/data src/services src/models`
- No hard-coded colors: `grep -rnE "#[0-9a-fA-F]{6}" src/ui | grep -v "^src/ui/theme/"`
- No fixed sizes: `grep -rnE "setFixedSize|setFixedWidth|setFixedHeight" src/ui`
- Signal blockers: `grep -rn "QSignalBlocker\|blockSignals" src/ui/pages/SettingsPage.cpp src/ui/MainWindow.cpp` (none on models or selection models)
- Per-binary function counts and totals by command; `git log --oneline`; `git show --stat` of each code commit.

## Report `docs/reports/M6-partB.md`
Plan vs result with honest gaps; C0 outcomes with raw output; the amendment checklist with exact names (B1-B6) and one `grep -n <symbol>` or test slot per sub-point; the close-to-tray state table (setting on/off x tray yes/no x Quit); new public methods; §14 entries really committed (continue the numbering after the last entry); hashes; the final gate output; "Not verified by agent, user to check" (at least: real tray hide/restore and Quit on GNOME/Wayland, the first-time hint, Settings look at 100-200% scaling in both themes, system theme switch by the desktop, CI result on GitHub).
