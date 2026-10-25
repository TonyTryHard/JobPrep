# M5 regression: fix-ups, round 2 (checklist Add, date field, drag warning, painted selection, notifications, report)

Read `AGENTS.md`, `docs/SPEC.md` (§3.2 S2, S3, S7, §3.5), `docs/prompts/M5-study-regression.md`, `docs/reports/M5-regression.md`, `src/ui/widgets/TopicDetailPanel.*`, `TopicBoardColumn.*`, `TopicListView.*`, `SelectionRestorer.*`, `src/ui/pages/StudyPage.*`, `src/ui/TrayNotifier.*`, `NotificationText.*`, `src/services/ReminderService.cpp`.
Local commits only, never push. Code, SPEC and report in separate commits. Rules: raw command output only, nothing trimmed, no line numbers as evidence, no claim without a command or a test slot, totals by command with the raw output pasted.

## What the user saw on the real desktop (Ubuntu 26.04 GNOME, **Wayland**, HEAD `dca572d`, all tests green)
- Board selection is now fixed (one outline, panel loads).
- **Checklist: typing an item and pressing "Add" still adds nothing.**
- **Date field:** the calendar popup works only after "Set date"; after "Clear date" it stops working again.
- The terminal printed `QDragManager::drag in possibly invalid state` once during use.
- Reminders: the log shows ticks, `Notify result: true`, recorded keys `overdue:2:2026-10-25` and `interview:9:1440` / `interview:9:60`, and `dbus-monitor` shows the app's `Notify` call (app "JobPrep", icon `dialog-information`, summary "argadfg", body "Technical (asdf) in 29 minutes", **urgency 0 = low**, timeout 10000) reaching gnome-shell. So the reminder logic and the tray path work; the user still saw nothing on screen.

## F1. Checklist "Add" in the real app (the test passes, the real app fails)
`testRealMouseClickChecklistAddAndSpaceToggle` passes offscreen, so the test does not reproduce what a real, **active** window does. First suspect (verify, do not assume): in a real window, clicking the Add button first moves focus away from the "New checklist item" line edit; the panel autosaves on focus loss (M4 B5); the save writes to `TopicRepository` (even if nothing changed) and emits `changed()`; the model resets; `SelectionRestorer` / `onColumnSelectionChanged` rebinds the panel (`loadFrom`), which **clears the typed checklist text**, and the click then arrives with an empty input. The same mechanism would explain the date field (clicking it moves focus, autosave, rebind, widget state lost).
Do this:
1. Show `grep -n "focusOut\|FocusOut\|save()\|loadFrom\|clearTopic" src/ui/widgets/TopicDetailPanel.cpp` and `grep -n "changed" src/data/TopicRepository.cpp | head -30`. Does `save()` write when no field changed? It must not (no write, no `changed()`), as for `moveTopic` no-ops.
2. Add a debug logging category `jobprep.study` (default threshold **info**, so debug lines appear only with `QT_LOGGING_RULES="jobprep.study.debug=true"`): panel `loadFrom(topicId)`, `clearTopic`, `save` (and whether it wrote), checklist Add handler entry (topic id, input length), `TopicRepository::changed`, `selectionChanged` handler (topic id or none). Keep lines short. The user will run the real app with this and paste the log.
3. Reproduce offscreen with **real focus movement**: show the window, `QTest::qWaitForWindowActive`, click into the checklist input, type, then click Add with `QTest::mouseClick` **after** moving focus the way a real click does (`button->setFocus(Qt::MouseFocusReason)` before the click if the platform does not move it). Assert the item is added and persisted, the input text was not cleared before the click, and `loadFrom` was not called for the same topic id. The test must fail on `dca572d`; paste that output.
4. Fix at the root: rebinding the panel for the **same topic id** after a model reset must not reset input widgets (checklist input, popups, cursor, focus); only refresh values that changed and only when the user is not editing that field. Autosave on focus loss must be a no-op when nothing changed. A reload must never destroy UI state (M4 amendment A5). Add the same real-focus test for: tick a checklist item, edit the title, change priority.

## F2. Target-date field (still behaves as "broken")
The flow "disabled until Set date, then Clear disables it again" is a design problem. Required (SPEC S2: optional, clearable):
- The field is **always enabled** while a topic is selected; with no date it shows "No target date" (special value text).
- Clicking the field or its arrow opens the calendar popup; choosing a day sets the date and autosaves.
- A small "Clear" control (or Delete key in the field) resets to "No target date" and keeps the field enabled; the popup works again afterwards. Remove the "Set date" step. Keep keyboard use and tooltips.
Tests with real events and the F1 focus movement: no date -> open popup -> pick a day (`dateEdit->calendarWidget()`) -> saved; clear -> text "No target date", enabled, date empty in the repository; open popup again -> pick another day -> saved. State what differs on Wayland, if anything.

## F3. `QDragManager::drag in possibly invalid state`
Qt prints this when a drag starts while another drag object is still active. Search for double handling: `TopicListView` installs an event filter on its viewport, `TopicBoardColumn` installs one on the same viewport and the list, and `QListView` has its own mouse handling, so `startDrag` may be called twice. Show `grep -n "startDrag\|QDrag\|mouseMoveEvent\|mousePressEvent\|startDragDistance" src/ui/widgets/TopicListView.cpp src/ui/widgets/TopicBoardColumn.cpp`. Find who can start two drags (count calls in a test that sends press and moves with real mouse events), fix it, and add a test that a drag start calls `startDrag` exactly once and that a normal click does not start a drag. Make sure single click selection still works after the fix.

## F4. Multi-outline test must check what is painted
Add a test that clicks cards in Backlog, In Progress and Review with real events and, after each click, grabs the board (`QWidget::grab()` offscreen) and checks that exactly one card border shows the selection color from the theme token (sample pixels on each visible card's border rectangle, both themes). Repeat after a model reset (tick a checklist item).

## F5. Notifications on the real desktop
- **Urgency:** `showMessage` with `QSystemTrayIcon::Information` sent urgency 0 (low). GNOME may show low-urgency notifications only in the list. Use `QSystemTrayIcon::Warning` (normal urgency) for interview reminders and follow-ups; keep the message type per `Notification::Kind` in one place. Show the new `Notify` hints if you can reproduce them with `dbus-monitor` (otherwise say you could not).
- **Text:** the summary is the company name only. Make the summary say what it is and when ("Interview in 29 minutes", "Interview today at 17:00") and put "Company · Type (position)" in the body. Update `NotificationText` and its tests.
- `TrayNotifier::notify` returns false (nothing recorded, retried next tick) when the tray icon is not visible or `isSystemTrayAvailable()` is false; if the tray is available but `supportsMessages()` is false, fall back to the in-app toast for that notification. Show the code (`grep -n "isVisible\|isSystemTrayAvailable\|supportsMessages" src/ui/TrayNotifier.cpp src/ui/NotifierFactory.cpp src/ui/ReminderCoordinator.cpp`) and add a log line with the reason.
- **Log level:** `jobprep.reminders` lines appeared without any `QT_LOGGING_RULES`, one per minute. Create the category with a default threshold of **info** (`Q_LOGGING_CATEGORY(name, "jobprep.reminders", QtInfoMsg)`) so tick lines show only with `jobprep.reminders.debug=true`; keep one info line per delivered notification.
- Do **not** change the delivery design (tray when available, toast when not). In the report list the open question for the user: should an active, visible window also show an in-app toast, because GNOME banners can be hidden by Do Not Disturb? Do not implement it.

## F6. Report accuracy
- Header: "Linux 6.14.0-35-generic" and "(offscreen / xcb)". Run `uname -r`; xcb was never used. Correct both.
- **Bisect:** the pasted log marks the *good* `m4` commit as bad; that is not a bisect. Either perform a real one or state that none was done; the user reproduced the bugs on a clean `m4` build. Say what `b12e6e1` changed compared with the commit before it.
- **Totals:** "328 slots (+18: +6, +2, +10 in supporting suites)": paste the per-binary `-functions | wc -l` loop with its sum and `git diff 7519117..HEAD -- tests | grep -E '^\+.*void '`; name every new slot and explain the unexplained +10.
- The missing notification is **not** explained by root cause 5 (static "in 24 hours" text). Rewrite the report section with the evidence above (the app delivers the notification to gnome-shell).
- Paste `git log --oneline -8`.

## Report
Update `docs/reports/M5-regression.md` in place with a "Changes in round 2" section (hashes from `git log --oneline`). Finish with `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` raw output and totals by command. List under "Not verified by agent": real Wayland clicks and focus (the agent cannot drive them), GNOME banner display.
