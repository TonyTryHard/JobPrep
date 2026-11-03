# M6 Part D2 fix-ups (evidence, acceptance map, real-data hazard, HiDpi test strength)

Read `docs/reports/M6-partD2.md`, `docs/reports/M6-ci-fixup4-v2.md`, `docs/prompts/M6-partD2-go.md` (the task you were given; its rules apply again), `AGENTS.md`, `tests/ui/tst_HiDpi.cpp`, `docs/RUNNING.md`.
Local commits only, never push. Rules as always: raw output of real commands with the exact command above it, nothing trimmed, empty output as `(no output)`, hashes only from `git log --oneline`, totals by command, no claim without a command. Commits: code fixes (one commit per topic), `docs/RUNNING.md`, SPEC if needed, report last. Do **not** touch `.github/workflows/` (a CI result is pending; do not change what it tests).

The D2 report has good work in it (icon, desktop entry, Qt 6.8 alignment, resource fix) but large parts are claims without commands, some are wrong, and one step touched the user's real data. Fix them as below. Write the results into a **new report** `docs/reports/M6-partD2-fixups.md`; do not rewrite `M6-partD2.md` except to add one line at its top: "Superseded in part by M6-partD2-fixups.md".

## F1. The acceptance table contains slot names that do not exist

I checked the names against the repository. These do **not** exist: `tst_Shortcuts::testGlobalNavigationShortcuts`, `testNewItemShortcuts`, `testFindFocusShortcuts`, `testEscapeKeyBehavior`, `testPageSpecificShortcuts` (and there is no Ctrl+5; Preferences is Ctrl+,), `tst_ImportExport::testRestoreRoundtripDataIntegrity`, `ui_smoke::testWindowCreation` (the real one is `testWindowCreationAndNavigation`). The statement "Enter, Space, arrows, context menus" for shortcuts is invented. Do this:
1. Run and paste, raw, `for b in tst_Shortcuts tst_PageSettings tst_ImportExport tst_SettingsService tst_HiDpi tst_PageHome; do echo "== $b"; "$(find build/dev-linux/tests -name $b -type f)" -functions; done` and the same for `ui_smoke`.
2. Rebuild the acceptance table (SPEC §12 M6, the five sentences quoted with `grep -n -A8 "M6 —" docs/SPEC.md`) **only from names in that output**. For "6 h → 8 h updates the dashboard instantly" the real slots are `tst_PageSettings::testWeeklyGoalDashboardInstantUpdate` and `tst_PageHome::testWeeklyGoalChangeUpdatesBarAndTextImmediately`; for "survives a restart" name the slot that really reopens the settings (if `tst_SettingsService::testWeeklyGoalSurvivesRestart` exists, show it in the `-functions` output; if not, say there is none and add it). For each slot paste its raw PASS line from a real run (`<binary> <slot>`).
3. Criterion 5: delete "Verified on Linux (X11/Wayland/offscreen)". The truth: Linux X11 (xcb) was checked by the user by hand, Wayland was **not** checked by anyone, Windows is checked by CI only as "compiles and runs offscreen tests". Write exactly that.
4. Give the test totals **by command** (binaries and `-functions` lines per binary, summed by the command, not by hand). The D2 report's slot table has no command above it.

## F2. A step of the D2 verification used the user's REAL data

In the fresh-clone check of `RUNNING.md` the report ran `QT_QPA_PLATFORM=offscreen timeout 2s ./build/dev-linux/jobprep --help` without an isolated data folder. The output `jobprep.reminders: delivered: kind= overdue refId= 2` shows that the app opened the user's real database and delivered (and therefore recorded as sent) a real overdue follow-up reminder. (`--help` is not an option of this program; it ignores arguments.) Do this:
1. Say plainly in the report that it happened, and which data it can have changed: `grep -n "markSent" src/services/ReminderService.cpp` raw, and `ls -l` of the data folder (the real path from the code).
2. Read-only, for the user: `sqlite3 -readonly <real db path> "select key, sent_at from reminders_sent order by sent_at desc limit 5"` (if `sqlite3` is missing say so) so the user can see which row was added at the time of the run. Do not modify anything.
3. Redo the "run the app" step of the fresh-clone verification **isolated**: `HOME=/tmp/jp-home-d2 XDG_DATA_HOME=/tmp/jp-xdg-d2 XDG_CONFIG_HOME=/tmp/jp-cfg-d2 QT_QPA_PLATFORM=offscreen timeout 3s ./build/dev-linux/jobprep; echo exit=$?`, show that the real data folder timestamps did not change (`stat -c '%y %n' <real db>` before and after).
4. `RUNNING.md` must have a short "Trying it without touching your data" section with exactly such a command, and `AGENTS.md` / the report must state the rule: **never run the app binary in verification without an isolated HOME/XDG**.

## F3. `RUNNING.md` documents a variable that may not exist

The D2 report lists `JOBPREP_DATA_DIR` among "supported environment variables". `grep -rn "JOBPREP_DATA_DIR\|qEnvironmentVariable\|qgetenv\|getenv" src` finds no such code in the pushed tree. Paste that grep (raw) for the current HEAD. If the variable does not exist, remove it from `RUNNING.md` (do not add the feature in this task) and say what the real way to isolate data is (XDG_DATA_HOME on Linux, `LOCALAPPDATA` on Windows; verify `QStandardPaths` behaviour from the code you have). Re-read every other statement in `RUNNING.md` against the code with one grep each (paths of the database/settings/backups, shortcut list vs SPEC §3.7, preset names vs `CMakePresets.json`, the `aqt` command vs the CI workflow). List every inaccuracy you find and fix; if you find none, say "no inaccuracies found" and show the greps.

## F4. `tst_HiDpi` is weaker than specified and the evidence is missing

The D2 task asked for: for every visible `QLabel`, `QAbstractButton`, `QGroupBox`, `QComboBox`, `QLineEdit`, `QSpinBox`/`QDateEdit`/`QTimeEdit`: `width() >= minimumSizeHint().width()` and the same for height; for non-wrapping labels and buttons the text advance fits `contentsRect().width()`; word-wrapped labels: `heightForWidth(width()) <= height()`; a dynamic property `jobprepElide=true` for intentional elision, each use explained. The report describes a weaker test (positive size, text advance, wrapped height) and does not show the failing run on the old code. Do this:
1. Show the current checks (`grep -n "minimumSizeHint\|horizontalAdvance\|heightForWidth\|jobprepElide\|QComboBox\|QLineEdit\|QSpinBox\|QGroupBox" tests/ui/tst_HiDpi.cpp`, raw).
2. Add the missing checks (`minimumSizeHint` for all the listed widget types, combo/line-edit/spin/group-box coverage). Run it and paste the raw failures (they are the point). Fix real clipping in the UI code or justify each exclusion with a one-line reason; no blanket whitelist.
3. `-functions` of `tst_HiDpi` raw. The slot names `testMainPagesAt150Scaling`, `testMainPagesAt200Scaling`, `testMainPagesAtDefaultScale` cannot mean three different scale factors inside one process (the factor is read at start-up and set by the ctest environment). Explain what each slot really does; rename them if the names mislead. Show how the 1.0 and 2.0 runs were done (`QT_SCALE_FACTOR=1 build/.../tst_HiDpi`, raw totals, and the same for 2).
4. Show that the screen scale really is 1.5 in the ctest run (the assertion on `devicePixelRatio()`), raw.
5. Same rule as before: the 12 `hidpi150-*.png` are "not viewed" unless you really opened them.

## F5. Fixed pixel sizes were added

The D2 fixes set explicit pixel minimums: `todayCard` 250, `upcomingCard` 270, `TodayEmptyState` 190, `UpcomingEmptyState` 210, agenda panel width 280, and others. `AGENTS.md` says no fixed pixel sizes in UI code, and pixel minimums break with larger fonts (the very thing a scaling pass should not do). The D1 report had a "No Fixed Sizes in UI" command; the D2 report dropped it.
1. Run it again (take the exact command from `docs/reports/M6-partD1.md`, section "No Fixed Sizes in UI") and paste the raw output; add `grep -rnE "setFixed(Width|Height|Size)|setMinimum(Width|Height|Size)\(" src/ui | grep -v "src/ui/MainWindow"` raw.
2. For each new hit introduced by D2 (`git diff 7f3ff74..HEAD -- src/ui | grep -E '^\+.*(setFixed|setMinimum)'` raw) replace the pixel value by something derived from the font or layout (`fontMetrics().height() * n`, `QStyle` metrics, layout stretch/size policy) or explain why it cannot be. Re-run `tst_HiDpi` at 1.0, 1.5 and 2.0 after the change.
3. Pre-existing hits (Sidebar 42/36, TrackDialog swatch 32, close button 28) stay unless they fail `tst_HiDpi`; list them as known.

## F6. Evidence that is only prose in the D2 report

Repeat these with commands and paste raw output (new sections of the new report):
1. **E1** `git diff 5ad0390..HEAD -- tests | grep -E '^[+-]\s+void \w+\(\);'` and the net count by command.
2. **E2** the fresh proxy build: `rm -rf /tmp/build-proxy-d2`, configure + build with `2>&1 | tee /tmp/proxy-d2.log`, then the first 5 and last 5 lines, `grep -c "warning:" /tmp/proxy-d2.log`, `grep -c "^\[" /tmp/proxy-d2.log`, exit code. The report says "222 targets"; a fresh clone now has **226** steps (the report's own fresh-clone log), so that number is stale or invented. Say how many steps this build really has.
3. **E3** the raw run of the control slot (plain `QDateEdit` versus `SingleTabDateEdit`), both directions. "0 versus 2 trapped steps" needs the lines that show it.
4. **E4** the `wc -c` before/after for each screenshot changed in `366a1fc` (the report gives only totals).
5. **E5** the TZ runs: raw, complete, for `UTC`, `America/Los_Angeles`, `Pacific/Auckland` as asked (the report used other zones) with `Total Test time` lines; and the `grep -rnE "QDate\(20[0-9]{2}" tests | wc -l` per file with the one-line verdict "injected clock / real clock" for each file that has absolute dates.
6. **E7** the `sqlite3 -readonly` counts of 1970 timestamps in the dev database (applications, status_history, topics), or "sqlite3 not installed". Do not repair anything.
7. **D2-4** `grep -rn "6\.5\|6\.8\|6\.10" AGENTS.md CMakeLists.txt CMakePresets.json docs/SPEC.md docs/RUNNING.md .github/workflows/ci.yml` raw (the report says "zero references to 6.5" with no output).
8. **D2-2** the desktop-entry evidence the task asked for, raw: install, `cat` of the file, second install with both `sha256sum`, a path with a space, `--remove` twice with exit codes, `--dry-run`, missing exe, `desktop-file-validate`, `shellcheck`/`bash -n`. All with `XDG_DATA_HOME=/tmp/...`.
9. **D2-1** `testApplicationIcon` raw run and `-functions` showing it.

## F7. The XDG test writes into the real data folder on Windows

In `tst_ImportExport::testRestoreSafetyBackupProductionPathWithCustomXdgDataHome` the portable change keeps `QStandardPaths` test mode **off** while the restore runs. On Linux `XDG_DATA_HOME` redirects the files into a temp folder; on Windows nothing redirects, so a developer running `ctest` there would write `pre-restore-*.json` into the real `%LOCALAPPDATA%\JobPrep\JobPrep\backups` **and prune the user's real safety backups to 5**. Fix it without weakening the assertion: run the production-path code under an isolated location on every OS (`QStandardPaths::setTestModeEnabled(true)` gives a throw-away location on all platforms; or point `HOME`/`LOCALAPPDATA`/`XDG_DATA_HOME` to the temp dir and skip the part that cannot be redirected), and make the test fail loudly if the resolved path is not under the temp dir. Show the diff and the Linux run. Windows effect: "next CI run".

## F8. Corrections to `M6-ci-fixup4-v2.md`

1. The report says the use of `QT_QPA_FONTDIR` was "confirmed in Qt source code (`qfontdatabase.cpp` and offscreen platform font loaders)". Which source files do you actually have on this machine? `find / -name "qbasicfontdatabase*" -o -name "qfontdatabase.cpp" 2>/dev/null | head` raw. If you only have headers, the statement is not a confirmation; replace it with "unconfirmed, comes from the Qt documentation/knowledge, to be proven by the next Windows run".
2. The three `CI-DIAG` helper changes in `tests/ui_test_helpers/UiTestHelpers.cpp` make `qInfo()` go to `stderr` in UI tests when there is no previous handler. Say whether on Windows the lines will end up in the `-o file,txt` log (via the previous handler) or only in the console; if only the console, say so in the report (the diagnostic step prints `QINFO`-less output).

## F9. The report must not say "100% complete, tag and push"

`M6-partD2.md` §11 says milestone 6 is 100% complete, tag `m6`, push. That is not true while the Windows CI is red. In the new report: "m6 is **not** ready to tag. The user pushes, waits for green CI on both jobs, then checks the 'Not verified by agent' list, then tags." Also complete the "Not verified by agent, user to check" list with the items from the D2 task item D2-7.4 that the report dropped: Wayland start, maximize on the desktop PC, ICS import into a real calendar, restore on real data, screen reader, Tab at the date control in the real app (unless F6.3 proved it), Ukrainian keyboard layout shortcuts, real Windows start (GUI subsystem, no console, icon in the title bar and in Explorer).

## F10. Suggestion only (do not implement)

The CI Windows job builds the `dev-windows` preset, which the CI logs show is a **Debug** build. The `WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>` setting and `jobprep.rc` are therefore **not exercised by CI at all**. Look at `CMakePresets.json` and say, in two or three lines, what an extra CI step would be (for example a Release configure + build of the `jobprep` target only, no tests). Do not edit the workflow; I will decide after the current CI run.

## Finish

`CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` full raw output, totals by command (binaries, functions), `git status --short` empty, `git log --oneline` of this task. The new report ends with the honest "Not verified by agent, user to check" list.
