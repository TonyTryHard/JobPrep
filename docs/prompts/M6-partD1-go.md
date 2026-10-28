# M6 Part D1: go (Part C carry-overs, keyboard shortcuts, empty states, tab order)

Read `AGENTS.md`, `docs/SPEC.md` (§3.1 D6, §3.7, §7, §8 widget list, §11, §12 M6, §14), `docs/prompts/M6-amendments.md` (D1, D2, D3 and the report rules), `docs/reports/M6-partC.md`, `docs/reports/M6-ci-fixup.md`, and the code: `src/ui/MainWindow.*`, `src/ui/pages/*`, `src/ui/dialogs/*`, `src/ui/widgets/EmptyState.*` (and every place that draws its own empty text), `tests/ui` helpers (`sendKeyClick`, `sendKeySequence`, the "exactly one dialog" helper), `src/data/Migrations.cpp`, `src/services/ExportService.*`, `src/services/RestoreService.*`.
Part C is accepted for structure, tests and the green gate, **with the carry-overs in C0**. Do C0 first, as its own local commits. Local commits only, never push. Code, SPEC and report are separate commits; the report is written last.
Part D1 is the first half of Part D. Part D2 (app icon, desktop entry script, HiDPI, `docs/RUNNING.md`, Windows subsystem decision, final M6 report) comes in a later go file; do not start it.

## Report rules (same as before, plus two)

1. Every command output is raw, from a real run, exact command above it, nothing trimmed. **An empty output is written as `(no output)`; never write "clean" or any other word in its place.**
2. No line numbers as evidence (no `File.cpp:605` references). Hashes only from `git log --oneline`. Totals by command (baseline after Part C: **31 binaries, 401 functions**).
3. The environment line comes from commands run in this session.
4. No claim without a command or a named test slot. Say honestly what is not tested.
5. **Qt floor.** SPEC §11 promises Qt 6.8+, and the first GitHub run (Qt 6.8.2) already failed on `QString + QStringView`, which compiles on the 6.10.2 of this machine. You cannot compile against 6.8.2 here, so in all new code: never concatenate `QString` with `QStringView` (use `QString::arg`, `.toString()`, or `QStringBuilder` with `QString`/`u"..."_s`), use only Qt APIs you already see used in the repo, and list in the report every new Qt class or function you introduced.
6. New tests must not add more than a few seconds to the gate. Put them in new offscreen binaries; add at most one slot to `ui_smoke`.

## C0. Part C carry-overs (do first)

**C0.1 Backup completeness (highest priority, possible data loss).** The Part C report §5 lists the `applications` columns as `target_date`, `salary_currency`, `contact_phone` and has no `work_mode`, `next_action`, `currency`; SPEC §5 says the opposite. The SPEC §14 1.14 entry names tables `resources` and `application_sta…` which do not exist in SPEC §5 (`resources` is a column of `topics`; the table is `status_history`). Either the report was written from memory or the schema differs from SPEC: find out which.
- Show raw: `grep -n "CREATE TABLE" src/data/Migrations.cpp`, the full definition of every table, every migration that alters a table (`grep -n "ALTER TABLE\|user_version" src/data/Migrations.cpp`), then the **real** column list of every table from a freshly migrated database (`PRAGMA table_info(<t>)` from a tiny throw-away test or the `sqlite3` CLI if installed).
- Show the JSON keys written by `exportDatabaseToJson` and the column list of every `INSERT` in the restore code, and compare them column by column with the real schema in one table (in the report).
- Show the body of `compareDatabaseContents()`: does it compare **every column through `SELECT *`**, or a hand-written column list? If it is a hand-written list, replace it with a schema-driven one (columns from `PRAGMA table_info`).
- New test `testBackupCoversEveryColumn` (in `tst_ImportExport`): for each table except `reminders_sent`, read `PRAGMA table_info` and assert that every column is present in the JSON record of a fully populated row. A column added to a migration without backup support must make it fail.
- New test `testRoundTripEveryColumnNonDefault`: a dataset where **every column of every table is non-default** (salary range and currency, work mode, next action and date, contact fields, notes with newlines, quotes and non-ASCII text, interview place/interviewer/notes/outcome, a session without topic, a subtask, a topic with resources JSON and tags), backup -> wipe -> restore -> equality through the schema-driven helper.
- If a column is missing in the backup or the restore: fix the code (separate commit, before the tests), say so in the report, and bump nothing else.
- Correct the Part C report §5 in a **new errata file** `docs/reports/M6-partC-errata.md` (never rewrite history) and add a new SPEC §14 entry that fixes the table names and states "7 tables exported, `reminders_sent` excluded".

**C0.2 Evidence missing from Part C.** Paste raw:
- `./build/dev-linux/tests/services/tst_ImportExport -functions` (27 names) and `./build/dev-linux/tests/ui/tst_PageSettings -functions | grep -E "Data|GoalSpin|CloseToTray|Maximize|FirstDay"`. Replace the counts "6 slots", "8 slots" in the C1-C8 table by the names.
- The output of the performance slot (the "126 ms" figure) from a real run.
- A **real ICS file** produced by the export code from a dataset with one interview whose notes contain a comma, a semicolon, a backslash, a newline and a Ukrainian line longer than 75 octets, one cancelled interview, and one interview without a place: `cat -A file.ics` (so `^M$` line ends and folded lines starting with a space are visible). Say which line is the `SUMMARY` and compare it with SPEC §10 (`Interview: <company> — <type>`).
- The first 15 lines of a real JSON backup.
- The content of `.gitattributes` (`cat .gitattributes`); the report only showed `ls -la`. It must contain a rule that gives LF in the working tree on Windows (`* text=auto eol=lf` or equivalent); if it does not, add it in its own commit and say so.
- The content of `docs/reports/M6-partB-errata.md`, and for each of the nine items of Part C C0.5 one line: corrected there, or not. Anything not corrected goes into `docs/reports/M6-partC-errata.md`.

**C0.3 Proxy build of the Part C code.** The report says the Windows audit was done with proxy flags but shows no output. Configure a **separate** build directory with `-Wshadow -Wconversion -Wsign-conversion` added to the compiler flags, build **all** targets from scratch, and paste the tail of the build log and `echo "exit=$?"`. Do not use `grep -c` to hide a failed build.

**C0.4 Quit during restore.** Show how `RestoreService`, `DataToolsController` and `MainWindow::prepareForShutdown` behave when the application quits while the restore worker is running (tray "Quit", session end, `qApp->quit()`). Required: no "QThread: Destroyed while thread is still running", the database is consistent (one transaction), the reminder service is not left paused after a failed restore. If this is not guaranteed today, fix it (the service waits for the worker with a bounded timeout, or the Quit action is refused while a restore is running) and add a test with a test-only hook that holds the worker. Name the slots.

**C0.5 Maximize fix is unproven.** The user still has to confirm it. As I read Qt's layout code, `Qt::AlignVCenter` removes only the **vertical** bound of that layout item, so `setMaximumWidth(320)` is untouched, and the top-level `maximumSize()` is normally not derived from the layout at all unless a size constraint is set. So `testWindowMaximumSizeAndMaximize` may pass without the fix. Show facts:
- The body of `testWindowMaximumSizeAndMaximize` and `testWindowMaximizeAndRestoreGeometry`.
- A **before/after demonstration**: with the fix reverted locally (`git stash`), does the test fail? Paste both runs. If it passes without the fix, say so plainly: the test proves nothing and the real cause is not yet found.
- Printed numbers (QDebug in a throw-away test, not committed) of `window->maximumSize()`, `windowHandle()->maximumSize()`, the central widget's and each page's `maximumSize()` and `layout()->totalMaximumSize()`, and the horizontal and vertical maximum of the toolbar layouts of Study and Jobs.
- Remove the sentence "SPEC §7 requires clean resizing without fixed limits"; SPEC §7 contains no such requirement.
- Do **not** invent another cause. If the numbers show no bound at all, report that and list what remains to check on the user's machine (the user will try `QT_QPA_PLATFORM=xcb ./build/dev-linux/jobprep` to see whether the problem is Wayland-specific).

**C0.6 Typed goal `7.30`.** Today `7.30` means 7.3 hours and is rounded to 7 h 15 min. Someone who types `7.30` usually means 7:30. Do **not** change the behavior. Add to the report one paragraph with the two options and their cost (keep decimals; or treat a two-digit fraction as minutes, which makes `7.50` ambiguous), and show what the spin box displays after `7.30` Enter, so the user can choose.

## D1. Keyboard shortcuts (SPEC §3.7, amendment D1)

First output of the task, **before any code**: a table of every shortcut in SPEC §3.7 and every shortcut already in the code (`grep -rn "QShortcut\|QKeySequence\|setShortcut\|keyPressEvent\|Key_" src/ui` raw). Columns: key, scope (window, page, dialog, panel), what it does, test slot. Then implement and test:

- Ctrl+1..4 pages (Home, Study, Jobs, Calendar) and Ctrl+, Settings: exist since Part A; keep and extend the test to run from every page and with focus inside a line edit and a text edit.
- **Ctrl+N** creates the new item of the **current page**: Home -> application, Study -> topic (existing `newTopic()`), Jobs -> application, Calendar -> interview, Settings -> nothing (not registered, not a silent no-op that eats the key).
- **Ctrl+F** focuses and selects the search box of Study and Jobs; on other pages it does nothing.
- **Ctrl+S** saves in the Study topic detail panel (same path as focus loss). Decide and document for dialogs with a Save/OK button (`ApplicationDialog`, `InterviewDialog`, `FollowUpDialog`, `SessionDialog`): recommended Ctrl+S = the same as the default button; record the decision in SPEC §14.
- **Esc**: dialogs (reject; the existing "discard changes?" rules stay), Study detail panel (saves and collapses, existing), quick-add (clears the line, existing), search edits (clear the text, keep focus rules as they are today), toast (only with focus inside the toast, see M5 Part D F1). An open combo popup closes first.
- Use `QKeySequence::StandardKey` where one exists (`New`, `Find`, `Save`); Ctrl+, stays explicit because `Preferences` has no standard key on Windows and Linux. Show the shortcut in tooltips or menu texts (`+ New  Ctrl+N`) through `tr()`.
- **Collisions to list and resolve** (state the rule in the report and in SPEC §14): Alt+arrows on the board vs the window; Ctrl+F on Study (page-local search, not the header search); Ctrl+, pressed inside a modal dialog (expected: nothing happens, no crash); page shortcuts must not fire while a modal dialog is open; Ctrl+S inside a `QTextEdit`; Ctrl+N inside a line edit.
- Tests use **real key events** through the existing helper and the focused widget, never `QAction::trigger()`. One named slot per shortcut and per collision. Keyboard layout: tests run with the default layout; say in the report that Ctrl+N/F/S under the Ukrainian layout is **not** verified by the agent (the user already confirmed Ctrl+, in both layouts).

## D2. Empty states, complete inventory (SPEC §3.1 D6, amendment D2)

First output, **before any code**: the inventory table of every list, table, panel and tab, with the current behavior (empty text yes/no, primary action yes/no). It must include at least: Dashboard as a whole (no data, with "Add application" / "Add topic" buttons), Dashboard Today list, Upcoming list, funnel with no applications, study progress with no tracks; Study with no tracks (`TrackDialog` opens or an empty state with a button), a column with no topics, list view with no topics, **filtered-empty** (search or track filter hides everything: a different text with "Clear filter"), topic checklist, topic resources table, session list if any; Jobs with no applications, **filtered-empty** (search or chip hides everything: "No matches" with "Clear filters"); `ApplicationDialog` Interviews tab and Timeline tab; Calendar agenda with no selected day and with a selected empty day; `TrackDialog` with no tracks.
- Use the shared `EmptyState` widget (SPEC §8) or a documented small helper; no copy-pasted label with its own colors. Tokens only, both themes, text through `tr()`, accessible name set.
- Distinguish **empty** from **filtered-empty**: never tell a user with 50 applications "No applications yet".
- One test slot per row of the inventory: set up the state with real data operations, assert the empty widget is visible and the content widget is not, click the primary action, and assert the expected dialog or focus change ("exactly one dialog" helper). Add the opposite check where cheap (one row of data hides the empty state).
- Screenshots for two representative empty states in both themes (`scripts/screenshots.sh`; raw output, `md5sum`, all images different, `git status --short`).

## D3. Tab order (amendment D3)

- A documented focus order per page and per dialog (a table in the report and in SPEC §14): `ApplicationDialog` (every tab), `InterviewDialog`, `FollowUpDialog`, `SessionDialog`, `TrackDialog`, and the pages Home, Study (toolbar, board columns, detail panel), Jobs, Calendar, Settings (every card).
- Use explicit `setTabOrder` only where the default creation order is wrong. Icon-only buttons (sidebar toggle, close "x", "+") get an accessible name through `tr()`.
- A test per page or dialog walks the chain with `focusNextChild()` from the first control, records `accessibleName` or `objectName`, and asserts: the sequence equals the documented list, the chain returns to the first control, every actionable control is reachable, no actionable control is `Qt::NoFocus`.
- A visible focus indicator: one painted test (both themes) that a focused push button, line edit and chip differ from the unfocused state; no hard-coded colors, tokens only.

## Tests, checks, report

- New binaries (offscreen), for example `tst_Shortcuts` and `tst_EmptyStatesAndFocus`; the totals by command show the new counts. Report the runtime change of the gate (`Total Test time (real)` before and after).
- Raw output of: the layering grep (`grep -rnE "StatusStyle|Theme::|Tokens|ui/|QtGui|QIcon|QPixmap|QColor|QPainter|QFont|QtWidgets|QSystemTrayIcon|QMessageBox|QFileDialog" src/domain src/data src/services src/models`), no hard-coded colors in new UI code (`grep -rnE "#[0-9a-fA-F]{3,8}|rgb\(|rgba\(" src/ui | grep -v "^src/ui/theme/"`), no fixed sizes (`grep -rnE "setFixedSize|setFixedWidth|setFixedHeight" src/ui`), `git status --short` after the gate, `git log --oneline`, `git show --stat` of each code commit, and a final `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh`.
- SPEC §14: new entries only (never rewrite old ones): the shortcut table and the Ctrl+S, Esc and collision decisions, the empty-state inventory and the empty versus filtered-empty rule, the documented focus order, the backup-completeness correction.
- Report `docs/reports/M6-partD1.md`: plan vs result with honest gaps; C0 outcomes one by one; the three tables (shortcuts, empty states, tab order) with slot names taken from raw `-functions` output; the Qt APIs introduced; **"Not verified by agent, user to check"**: Ctrl+N/F/S under the Ukrainian layout, real maximize on Wayland (and under `QT_QPA_PLATFORM=xcb`), ICS import into a real calendar, restore on real data, focus ring look on the real desktop, screen reader behavior, everything on Windows.
