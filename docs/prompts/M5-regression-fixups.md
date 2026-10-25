# M5 regression: fix-ups (date field behavior, painted-selection test, report accuracy, tray check)

Read `AGENTS.md`, `docs/SPEC.md` (§3.2 S2, S7), `docs/prompts/M5-study-regression.md`, `docs/reports/M5-regression.md`, `src/ui/widgets/TopicDetailPanel.*`, `src/ui/widgets/TopicBoardColumn.*`, `src/ui/TrayNotifier.*`.
The regression fix is accepted in principle (selection coordinated through `selectionChanged`, viewport repaint, Esc scoped to the toast, notification text using the real time). The user's manual tests on the real desktop (Ubuntu 26.04, **Wayland**) show that the **bug was already present in the `m4` build** (multi-selection, checklist Add, date field). The points below are still open. Local commits only, never push. Code, SPEC and report in separate commits.
Rules: raw command output only, nothing trimmed, no line numbers as evidence, no claim without a command or a test slot, totals by command with the raw output pasted.

## F1. Target-date field: still behaves as "broken" for the user
User report on the real app: the calendar popup does not open until the user clicks "Set date"; then it works; after "Clear date" it again does not work. The regression fix keeps exactly this flow (the field is **disabled** when there is no target date). That is a design problem, not a rendering problem.
Required behavior (SPEC S2: "target date (optional)", clearable):
- The date field is **always enabled** while a topic is selected. With no date it shows "No target date" (special value text).
- Clicking the field or its drop-down arrow opens the calendar popup; choosing a day sets the date and autosaves.
- A small "Clear" control (button or Delete key in the field) removes the date, shows "No target date" again and **keeps the field enabled**. Opening the popup afterwards works.
- Remove the "Set date" step. Keep keyboard access (type a date, arrow keys, Delete to clear) and tooltips.
Tests with real events (named slots): select a topic with no date -> open the popup with a mouse click on the drop-down subcontrol -> click a day in `dateEdit->calendarWidget()` -> date saved in `TopicRepository`; clear -> text "No target date", field enabled, repository date empty; open the popup again and pick another day -> saved. Show what you did for the popup on Wayland if anything differs (a `QDateEdit` popup needs a visible parent window).

## F2. The multi-outline test must check what is painted
The regression test checks `selectedIndexes()` and `currentIndex()`. The bug the user saw is **painted** outlines. Add a test that clicks cards in Backlog, In Progress and Review with real mouse events and, after each click, grabs the board (`QWidget::grab()` offscreen) and checks that exactly one card border shows the selection color from the theme token (sample the pixels on each visible card's border rectangle; both themes). A second check: after a model reset (tick a checklist item) the outline is still on the one selected card only.

## F3. Report accuracy
- Header: "Linux 6.14.0-35-generic" and "(offscreen / xcb)". Run `uname -r`; xcb was never used (the report itself says `xvfb-run` is absent). Correct both.
- **Bisect:** the pasted log starts with `m4` as *good* and then marks `m4` as *bad*; that is not a bisect. Either perform a real one (check out the new test file into a worktree at candidate commits, build, run, `git bisect run`) or state that no bisect was done. The user's own manual run of the `m4` build reproduces the bug. Also say what `b12e6e1` changed compared with the commit before it (did it cause the detail panel to stop loading, or only fail to fix it?).
- **Totals:** "328 slots (+18: +6, +2, +10 in supporting suites)" has no command and the +10 is unexplained. Paste the per-binary `-functions | wc -l` loop output and its sum, and `git diff 7519117..HEAD -- tests | grep -E '^\+.*void '`. Name every new slot.
- Paste `git log --oneline -8`; the report lists hashes that I could not compare (`9bfea82`, `641e012`, "current commit").
- Root cause 5 (static "in 24 hours" text) is not the reason no notification appeared. Say that the cause of the missing notification is **not yet identified** (see F4), do not list it as resolved.

## F4. Tray path: say when the reminder is not really delivered
- `TrayNotifier::notify` must return **false** (nothing recorded, retried next tick) if the tray icon is not visible at that moment or `QSystemTrayIcon::isSystemTrayAvailable()` is false. Show the code and add the log line (`jobprep.reminders`) with the reason.
- Show with `grep -n "isVisible\|isSystemTrayAvailable\|supportsMessages" src/ui/TrayNotifier.cpp src/ui/NotifierFactory.cpp src/ui/ReminderCoordinator.cpp` how availability and `supportsMessages()` are used. On a desktop where `isSystemTrayAvailable()` is true but `QSystemTrayIcon::supportsMessages()` is false, fall back to the in-app toast for that notification.
- The user is checking the real app with `QT_LOGGING_RULES="jobprep.reminders.debug=true"` and `dbus-monitor`; keep the log lines stable and short: one line per tick (`due=N`), one per `notify` call (`kind`, `refId`, `result`), one per recorded key.

## Report
Update `docs/reports/M5-regression.md` in place (sections header, 4, 5, 9 and a new "Changes in round 2" with hashes from `git log --oneline`). Finish with `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` raw output and totals by command.
