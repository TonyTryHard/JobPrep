# M6: plan request (Polish and data tools). PLAN ONLY, no code

Read `AGENTS.md` (all of it, especially §5 threads, §8, §9, §11, §12), `docs/SPEC.md` (§3.6, §3.7, §7.1, §10, §11, §12 M6, §14 up to the last entry), `docs/reports/M5-checklist.md`, `docs/reports/M5-partD.md`, and the real code under `src/` and `tests/`.
If `docs/prompts/M6.md` exists, read it too; **this file adds constraints and wins on any conflict**.

Deliverable: `docs/plans/M6-plan.md`, committed locally on its own (`docs: add M6 plan`). Do **not** write production code or tests in this step. Never push. Then stop and wait for approval. The plan comes back with amendments, as for M3-M5.

## Evidence rules (earlier reports contained invented or staged output; these are strict)

1. Every statement about the existing code is backed by a command you ran in this session, with its raw output: `grep -n <symbol>`, `ls`, `sed -n` of a few lines. **No line numbers from memory, no trimmed output, no hand-edited output** (no `...` inside a grep result).
2. Do not claim a file, class, method, setting key or test slot exists unless a command showed it. If it does not exist, say so.
3. Environment line (OS, kernel, compiler, Qt) from commands run now (`lsb_release -d`, `uname -r`, `g++ --version | head -1`, `Config:` line of a QtTest run).
4. Scope comes only from the SPEC and this file. Extras go under "Proposals", separated.
5. Never claim that something works on Windows. Windows has never been built. Say what is expected to break and how the plan finds out.

## What M6 contains (SPEC §12 M6, binding)
Settings page (theme, accent, week start, **weekly study goal control**, reminder lead times, close-to-tray; plus the "Data" section of §3.6: Export CSV, Export ICS, Backup JSON, Restore JSON (replace, with confirmation), Load sample data, Open data folder), JSON backup/restore, ICS export, empty states everywhere, keyboard shortcuts of §3.7, app icon, tab-order pass, HiDPI check, `docs/RUNNING.md` verified step by step, optional CI.
Acceptance (quote it from the SPEC with `grep`, do not paraphrase): backup -> wipe -> restore reproduces identical data; all §3.7 shortcuts work; weekly goal 6 h -> 8 h updates the dashboard instantly and survives a restart; no clipped text at 150% scaling in either theme; the app starts on Windows and on Linux (X11 or Wayland).

## Carry-overs from M3-M5 that M6 must absorb (verify each with a command, then plan it)
- **Real key-event tests** for every shortcut in §3.7 (Ctrl+1..4, Ctrl+N, Ctrl+F, Ctrl+, , Ctrl+S, Esc). Today Ctrl+N is tested by calling `triggerCurrentPageNew()`; a wrong binding would pass. Tests send real `QKeyEvent`s to the focused widget of a shown, active window (see how `tst_StudyRegression` does real input).
- **`ui_smoke` is about 100 s and 3700+ lines** (the whole gate is about 110 s). Propose splitting it into a few binaries (for example per page plus a small real "smoke"), keeping the AGENTS rule that `ui_smoke` visits every page, both themes, every dialog and fails on unexpected warnings. Say how `ctest -j` and the screenshot slots are affected.
- **`QSignalBlocker` on a model hid the checklist items** (M5 checklist bug: no `rowsInserted`, so the view never laid out its rows). Run `grep -rn "QSignalBlocker\|blockSignals" src` and list every use on a model or view with the verdict "safe / risky" and why; plan tests for the risky ones.
- **Painted-output tests:** the startup contrast test exists for several pages. Plan the same for the new Settings page and dialogs in both themes.
- **Inter font:** not bundled (`resources/fonts` contains only a README). Never invent font files. Ask the user (see questions) to provide the Inter files and the OFL license text, or plan the system-font fallback and say so.
- **Close to tray (R4)** was deferred from M5. `TrayNotifier` already has a tray menu ("Show JobPrep", "Quit") and a left-click handler. Plan `closeEvent` behavior (hide to tray only when the setting is on and the tray is available), the disabled-with-tooltip state when there is no tray, and what "Quit" does with pending reminders.
- **Reminder lead times UI:** the key `reminders/interviewLeadMinutes` and its change signal exist; plan the editor (validation, defaults, "Reset").
- **Wayland app identity:** notifications and window icons on GNOME are tied to the desktop-file name. Say whether `QGuiApplication::setDesktopFileName` and a sample `.desktop` file for development belong in M6 (no installers; AGENTS §12), as a proposal.
- **Open the data folder and links** only through `QDesktopServices::openUrl`.

## Pre-flight findings the plan must contain (table with the command used)
`SettingsPage` and the sidebar today (is there a Settings entry at all?); `SettingsService` keys and signals (full list); `ExportService` current API (CSV exists since M2; ICS and JSON?) and `ImportService`; `SeedService` and the "sample data prompted" flow; app icon and `resources/icons/app.svg`; `src/platform/`; CMake handling of `MSVC`/`WIN32`; `.github/` and `docs/RUNNING.md` (do they exist?); existing `QAction`/`QShortcut`/`QKeySequence` bindings; `setTabOrder` use; `QStandardPaths` data folder and DB open path; how long-running work is done today (AGENTS §5: more than about 100 ms must not run on the GUI thread); `reminders_sent` and what a restore should do with it; test targets and CMake layout; `scripts/check.sh`/`check.ps1` and `scripts/screenshots.*`.

## SPEC and AGENTS cross-check table (conflict -> resolution), like `M5-amendments.md`
Points you must address, plus any you find:
- **Backup format (§10):** `{ "format": "jobprep-backup", "version": 1, "exportedAt", tracks, topics, subtasks, sessions, applications, statusHistory, interviews }`. It has no settings and no `reminders_sent`. Decide and justify what restore does with `reminders_sent` (keys for ids that no longer match must not suppress or duplicate reminders) and say it is a §14 entry. Are ids preserved? They must be for referential integrity and for "identical data".
- **Restore safety:** validate before touching the DB (format, version, types, enum strings, date formats, referential integrity, duplicates, sizes); one transaction; rollback on any error; confirmation before replace; a backup made automatically before a restore? (proposal only). The work runs off the GUI thread with a busy state and no half-updated UI; after restore all models and the dashboard refresh once (bursts of `changed()` are coalesced today).
- **Backup file writes** are atomic (`QSaveFile`); UTF-8; the CSV BOM stays; the **ICS uses CRLF written explicitly**, floating local times, `VALARM` 60 minutes (§10).
- **"Backup -> wipe -> restore reproduces identical data":** define "identical" precisely (every table, every column, including ids and timestamps) and design the test (a comparison helper over all tables). The SPEC has no "wipe" button: say how the test wipes and whether a UI action is needed (proposal only).
- **First-day-of-week:** the stored value is an `int` and the dashboard maps only 7 to Sunday, everything else to Monday. Settings must offer exactly Monday and Sunday and keep the mapping consistent in Calendar, Dashboard and stats.
- **Weekly goal:** hours with 15-minute steps, range 1-40 h (SPEC §3.6), stored in minutes under `study/weeklyGoalMinutes`; changes reach the dashboard through the existing signal; survives restart (test with a temp settings file).
- **Accent presets and theme switching** at runtime without restart: the Settings page uses `ThemeManager`; no widget may depend on a theme switch to look right (M5 lesson). Plan a test: startup in each saved theme/accent and after a switch gives the same painted result.
- **Ctrl+, (Preferences) and Esc** behaviors: which pages/dialogs; keep Ctrl+S inside detail panels only. Shortcut collisions with existing keys (Alt+arrows on the board, Ctrl+F on Study) must be listed.
- **Windows:** the plan cannot build Windows locally. Propose how Windows problems are found anyway (a GitHub Actions workflow on `windows-latest` and `ubuntu-latest` running configure, build, ctest offscreen; the SPEC allows it because the repo is on GitHub; the agent cannot run it, the user pushes and reports). List the likely MSVC issues to check by reading code (warnings as errors, `/W4`, `Q_OS_*` outside `src/platform`, paths, `NOMINMAX`, narrowing, `QT_PREFIX_PATH`, SQLite plugin, DLLs next to the exe, `windeployqt` in RUNNING.md as a run step not an installer) and say how the CI Qt version is chosen (must be 6.8 LTS or newer).
- **HiDPI/scaling:** how 150% is tested headlessly (for example `QT_SCALE_FACTOR` or font DPI in a dedicated test binary), what stays a manual check, and how "no clipped text" is asserted (size hints against text metrics, painted checks).
- **Empty states everywhere (AGENTS §7):** an inventory table of every list/table/board/panel with its empty state (icon + one-line hint + primary action) and which already exist.
- **Tab order and tooltips:** inventory of dialogs and pages; how a test proves the order.

## Design
Services/data: `ExportService` (CSV existing, ICS, JSON), `ImportService` (validate, restore), their result types (no presentation text in services), settings keys to add. UI: `SettingsPage` sections and widgets, dialogs (restore confirmation, progress/busy), file dialogs (QFileDialog; note Wayland portals), menu/shortcut wiring in `MainWindow` (list new public methods), sidebar entry, app icon and window icon, `docs/RUNNING.md` outline. Threads: which operations run in a worker and how results return. Refresh: how restore triggers one coalesced refresh.

## Parts (approval gate between parts)
Propose 3-5 parts, each its own commit(s), `check.sh` green and its own report. Starting point to challenge: A) CI workflow + housekeeping (split `ui_smoke`, `QSignalBlocker` audit, shortcut test infrastructure, Windows code review list); B) Settings page + settings keys + close-to-tray + lead times + goal/week-start/theme/accent; C) ICS + JSON backup/restore (services, tests, off-thread, data section UI); D) shortcuts (real key tests), empty-state pass, tab order, app icon, HiDPI tests, `RUNNING.md` walkthrough, final M6 report. Say if you prefer another order and why.

## Tests
List every planned test target and slot name and what each asserts (new binaries for export/import, settings page, shortcuts; repository/service additions; split `ui_smoke`). Required cases: backup -> wipe -> restore equality over all tables (hand-built dataset with arithmetic in comments, plus the seeded data); invalid backups (wrong format/version, broken reference, bad enum, truncated JSON, empty file) leave the DB untouched; ICS output bytes (CRLF, folding of long lines, special characters escaped, `VALARM`); goal 6 h -> 8 h with restart; every shortcut with a real key event; theme/accent startup equals after-switch; empty-state test per page; close-to-tray with and without a tray; screenshots (offscreen `QWidget::grab()`, light and dark, 100%) of Settings and the new dialogs.
Totals are computed by command; baseline now: 25 test binaries, 324 test functions.

## Risks, files touched, SPEC §14 entries expected (continue the numbering after the last entry; check with `grep -n "^- 1\." docs/SPEC.md | tail -3`), "Not verified by agent, user to check".

## Questions for the user (put them in the plan under "Open questions" with a recommended default)
1. Inter font: the user can download Inter (OFL) and put the files and the license text in `resources/fonts`. Recommended: yes, user provides; otherwise system font and say so.
2. CI on GitHub (the repo is on GitHub): recommended yes.
3. Should Backup JSON also include the app settings? Recommended no (SPEC §10 lists tables only).
4. Should a restore make an automatic safety backup first? Recommended yes, in the data folder, keep the last few.

## Final message of this step
List the plan path, the commit hash from `git log --oneline`, the open questions, and nothing else. Do not start Part A.
