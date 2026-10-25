# M5 urgent: Study page broken in the real app, and no reminder was ever seen

Read `AGENTS.md`, `docs/SPEC.md` (§3.2 S3, S5, S7, §3.5), `docs/reports/M4-partC.md`, `docs/reports/M5-partD.md`, `src/main.cpp`, `src/ui/ReminderCoordinator.*`, `InAppNotifier.*`, `Toast.*`, `TrayNotifier.*`, `NotifierFactory.*`, `MainWindow.*`, `src/ui/pages/StudyPage.*`, `src/ui/widgets/TopicDetailPanel.*`, `TopicBoardColumn.*`, `TopicListView.*`, `SelectionRestorer.*`.
The user tested the real desktop app (Ubuntu 26.04 GNOME, real display, real data) after M5 Part D. All `check.sh` tests are green, yet:

1. **Topic details panel is unusable.** It shows an empty state (empty title, date `2000-01-01`, status Backlog, priority Low) although cards exist; nothing can be edited.
2. **Checklist "Add" does nothing.**
3. **The target-date calendar popup does not work.**
4. **Several cards are outlined at the same time** (one per column: Backlog, In Progress, Review) although M4 fixed it to exactly one selected card. (Screenshot from the user: three cards with the accent outline, panel empty.)
5. **No reminder notification was ever seen** (not a toast, not a desktop bubble).
Do not guess. Reproduce, find the root cause with evidence, fix, and add tests that would have caught it. Local commits only, never push. Do not tag or change milestones.

Rules: raw command output only, nothing trimmed, no line numbers as evidence, no claim without a command or a test slot. Code, SPEC and report in separate commits.

## Step 1. Why did the tests not see it?
Existing tests build `MainWindow` **without** the production composition (`ReminderCoordinator`, notifier, tray) and drive Study mostly through programmatic calls (`selectTopic`, `setCurrentIndex`). The real app has the coordinator, an `InAppNotifier`/`TrayNotifier`, event filters, and real mouse/keyboard focus.
Run and paste: `grep -rn "installEventFilter\|eventFilter" src`, `grep -rn "qApp->install\|QApplication::instance()->install" src`, and `grep -n "show()\|setVisible\|setContextMenu" src/ui/TrayNotifier.cpp src/ui/InAppNotifier.cpp src/ui/Toast.cpp`.
List every widget that `InAppNotifier`/`Toast` creates as a child of `MainWindow` and its geometry when **no** toast is visible. A transparent container or overlay that is full width/height, or an event filter that returns true too often, would block clicks, typing and popups on part of the window.

## Step 2. Reproduce with real input, in the production composition
Write tests (new offscreen binary or `tst_Notifiers`) that construct exactly what `main.cpp` constructs (`MainWindow` shown, `ReminderCoordinator` with `trayAvailable` both `false` and `true`, started with an injected clock), then use **real events**, not method calls:
- `QTest::mouseClick` on a card's viewport rectangle in Backlog, then in In Progress, then in Review: after each click exactly one topic is selected board-wide **and** exactly one card is painted as selected/current (check `selectedIndexes()` and `currentIndex()` of every column), and the detail panel shows that topic's title and is enabled.
- `QTest::keyClicks` into the detail title edit, then change selection: the new title is saved.
- Type text into "New checklist item" and click "Add" with `QTest::mouseClick` on the button: the item appears and is persisted; Space ticks it; progress on the card changes.
- Open the target-date `QDateEdit` popup (click the arrow or press the key), pick a day with a mouse click in the calendar popup, and verify the date is set and saved; verify clearing the date works.
- `QApplication::widgetAt` / `childAt` at the centre of the title edit, the Add button and a card returns the expected widget, not an overlay.
- Same sequence with a toast visible.
If xvfb and xdotool are installed (`which xvfb-run xdotool`), also run the real app under `xvfb-run` with the xcb platform and report what you could and could not drive; if not installed, say so and do not install anything.
At least one of these tests must **fail on the current HEAD**. Paste the failing output.

## Step 3. Find the first bad commit
Use the failing test and `git bisect` (with `git bisect run` and a script that builds and runs only that test) between tag `m4` and `HEAD`. Paste `git bisect log` and the first bad commit hash from `git log --oneline`. If the test also fails at `m4`, say so; then the bug is older and the M4 tests were not enough. Clean up with `git bisect reset`; do not change history.

## Step 4. Fix
Fix the root cause only. Constraints:
- Esc must dismiss a toast **only when keyboard focus is inside that toast**; no global event filter may swallow key or mouse events (see `docs/prompts/M5-partD-fixups.md` F1).
- Exactly one selected topic board-wide **and** exactly one outlined card. If the outline is painted for the list view's *current index*, clear the current index in the other columns too and paint the outline from the real selection state.
- When no topic is selected the detail panel must be visibly disabled with a one-line hint ("Select a topic"); the target-date field must show "No target date" when empty, not `2000-01-01`.
- Add the regression tests from step 2 to the gate.

## Step 5. Reminders in the real app
- Add a logging category `jobprep.reminders` (debug level, enabled with `QT_LOGGING_RULES="jobprep.reminders.debug=true"`): start, each tick with the number of due reminders, each `notify` result, each recorded key. No other output.
- Check `TrayNotifier`: is the tray icon actually `show()`n? Without that `showMessage` shows nothing. `notify` currently returns true after `showMessage`; say what happens when the desktop suppresses the bubble.
- Check `NotificationText`: for an interview 3 hours away the only due lead is 1440, so the text must **not** say "in 24 hours". Use the interview time (`when`) and the real remaining time ("today at 17:00", "in 3 hours"). Add tests.
- Show with a test, using the real `main.cpp` composition and an injected clock, that an interview 50 minutes ahead gives one visible notification through the in-app path and one `notify` call through a fake tray path.

## Report `docs/reports/M5-regression.md`
Symptoms; why tests missed it; the failing test output; bisect result (hash); root cause with the `grep`/code evidence; fix commits; new tests with slot names; final `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` raw output and totals by command (baseline 24 binaries, 310); what is still not verified (real tray bubble, Wayland raise); `ui_smoke` runtime before and after.
