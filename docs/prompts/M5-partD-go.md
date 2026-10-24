# M5 Part D: go (notifiers, toast, wiring, end-to-end, final M5 report), with Part C carry-overs

Read `AGENTS.md`, `docs/SPEC.md` (§3.5 R1-R4, §7, §8, §12 M5, §14), `docs/plans/M5-plan.md`, `docs/prompts/M5-amendments.md` (Part D section D1-D5, report rules),
`docs/prompts/M5-partC-go.md`, `docs/reports/M5-partC.md`, and the code: `src/services/INotifier.h`, `ReminderService.*`, `src/ui/MainWindow.*`, `src/main.cpp`, `src/app/AppContext.*`, `src/ui/theme/*`, `src/ui/FormatHelper.*`.
Part C is accepted **with the carry-overs in C0**. Do C0 first as its own local commit, then Part D in one or more commits. Local commits only, never push.
This is the last part of M5: the final report must also close the milestone.

## Report rules (same as before)

1. Every command output is raw, from a real run, exact command above it, nothing trimmed.
2. No line numbers as evidence. Hashes only from `git log --oneline`; report written last, committed separately.
3. Test totals by command (baseline now: 23 binaries, `ui_smoke` 78, total 281).
4. Environment line in the report (OS, compiler, Qt) must come from commands run in this session (`lsb_release -d`, `g++ --version | head -1`, the `Config:` line of a QtTest run). The Part C report said Ubuntu 24.04, GCC 13.3.0, Qt 6.8.3, but every earlier run on this machine shows Ubuntu 26.04, GCC 15.2.0, Qt 6.10.2.
5. No claim without a command or a test slot. Say honestly what is not tested, especially the real tray path.

## C0. Carry-overs from Part C (one small commit, docs and tests only)

1. **Errata for the errata.** `docs/reports/M5-partB-errata.md` now says "Inter font bundling is handled at application startup". That is false: Inter is not bundled until M6. Show `ls resources/fonts` and `grep -rn "addApplicationFont" src`, then fix the errata in a **new commit** (never rewrite history) and state which font the screenshots really use.
2. Correct the environment line of `docs/reports/M5-partC.md` (rule 4) in an errata note or a report fix commit.
3. Paste the raw output of `-functions` for `tst_SettingsService`, `tst_ReminderLogRepository` and `tst_ReminderService`. Part C only gave counts for the first two. Confirm that `tst_SettingsService` covers: default, sanitizing, empty/invalid fallback, signal once per real change.
4. Several `tst_ReminderService` slots bundle many scenarios (reschedule: four cases in one slot; overdue: five). Split them into one slot or one `_data` row per scenario so a failure names the case. Rename the slot with "Dst" in its name: it tests a day border, not DST. Recompute totals by command.
5. `ReminderService` reschedule path does `remove(key)` then `markSent(key, now)` as two writes. Show the two calls (`grep -n "remove(\|markSent(" src/services/ReminderService.cpp`) and add a test: if `markSent` fails after `remove`, the next check delivers again (no crash, no silent loss).

## Part D scope (amendments D1-D5; refined here, this file wins on conflict)

**Ownership and composition (C5 follow-up).** The Part C report sketched `std::make_unique<ReminderService>(..., &mainWindow)` in `main.cpp`: a `unique_ptr` **and** a Qt parent is double ownership, and the notifier would be destroyed before the service that references it. Do this instead:
- A small class `ReminderCoordinator` in `src/ui/` (not in `AppContext`, which stays headless) owns the notifier and the `ReminderService` as `std::unique_ptr` members declared in this order: notifier, then service (members are destroyed in reverse, so the service dies first). No Qt parent on them.
- `main.cpp` creates it **after** `MainWindow` as a local variable, calls `show()` first and `start()` after, so it is destroyed before the window.
- It connects `SettingsService::interviewLeadMinutesChanged` to `ReminderService::setLeadMinutes`, applies the stored lead minutes at start, and takes an injectable `std::function<QDateTime()>` clock and a `bool trayAvailable` (default `QSystemTrayIcon::isSystemTrayAvailable()` decided once at startup).
- **Tests never auto-start reminders:** `MainWindow` and `ui_smoke` must not start a real timer or write `reminders_sent`. Only the new tests construct a coordinator with a fixed clock.

**Notifier selection (testable).** A function `makeNotifier(bool trayAvailable, MainWindow&, ...)` returning `std::unique_ptr<INotifier>`; tests call it with both values.

**Text lives in `ui` (decision from Part C: `Notification` carries data).** A small helper (for example `NotificationText`) builds title and message from a `Notification` using `StatusStyle::label(InterviewType)`, `FormatHelper`, `QLocale` and `tr()`. Lead minutes read naturally ("in 24 hours", "in 1 hour", "in 90 minutes"), singular/plural handled. Overdue: one application -> its company and position; summary -> "N follow-ups are overdue". Unit-tested.

**`TrayNotifier` (D1).** Uses an existing themed SVG icon (list any new icon; none expected). `showMessage`; stores the last `Notification` because `messageClicked` has no payload; emits a signal with it; returns true after `showMessage`. Add a minimal tray menu "Show JobPrep" / "Quit" and make a left click call `bringToFront()` (an icon with no way to open the app is poor UX; this is not close-to-tray, which stays in M6). Record in §14.

**`InAppNotifier` and `Toast` (D2).**
- `notify` returns **false** when the main window is hidden or minimized (nothing shown, nothing recorded, retried next tick); true when the toast is shown in a visible window.
- Toasts are sticky (no auto-dismiss timer), at most 3 visible plus a "+N more" line, repositioned on window resize, border and surface contrast instead of `QGraphicsDropShadowEffect`, they never take keyboard focus, Esc or the close button dismisses the focused toast, Enter or click opens the item, accessible name/role, tokens only, both themes, text through `tr()`.

**Click to open (D3).** `MainWindow::openItemFromNotification(const Notification&)` = `bringToFront()` then a separate routing function that tests call directly: `Interview` -> `openInterview(refId)`, `FollowUp` and a single `OverdueFollowUps` -> `openApplication(refId)`, a summary (`refId == 0`) -> switch to the Jobs page. `bringToFront()` is `showNormal()` if minimized, `raise()`, `activateWindow()`, with no `platformName()` branch; tests never call it. **Do not widen the `ui_smoke` QPA noise filter.** Say in the report that on native Wayland the compositor may refuse to raise a window.

## Tests (named slots; zero warnings; `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` green)

`ui_smoke` is already about 80 s of the gate. Put the new notifier tests in a **new offscreen binary** (for example `tst_Notifiers`) and keep `ui_smoke` additions to what the AGENTS rule needs (one slot that shows a toast and checks no warnings). Report the runtime change.
- `makeNotifier(false)` gives `InAppNotifier`, `makeNotifier(true)` gives `TrayNotifier` (`dynamic_cast`).
- `NotificationText`: interview 60 and 1440 and 90 minutes, follow-up, overdue single and summary, singular/plural.
- `InAppNotifier`/`Toast`: hidden or minimized window -> false and nothing shown; visible -> true, toast shown, still shown after a long simulated wait (no auto-dismiss); 4 notifications -> 3 visible plus "+1 more"; Esc dismisses; click opens the item; focus widget unchanged after a toast appears; position follows a resize; accessible name set.
- Coordinator end-to-end (fixed clock, in-memory data, fake or real in-app notifier): an interview 59 minutes ahead gives exactly one toast and one `reminders_sent` pair of keys; clicking it opens the `InterviewDialog` for that interview (use the same "exactly one dialog" helper as the other tests); overdue summary click switches to Jobs; window minimized -> nothing recorded, `showNormal()` then next `checkNow()` delivers once; no tray -> toast; lead minutes changed in settings take effect.
- `TrayNotifier`: only what can run offscreen (constructs, stores last notification, click handler maps to the right item through the routing function). List honestly what is not covered.
- Screenshots (offscreen `QWidget::grab()`, light and dark, 100%): dashboard with a toast visible, and a stack of 4 toasts. Extend `scripts/screenshots.sh`; paste its raw output, `md5sum` of the new images (all different), `git status --short`.

## Checks to show (raw output)

- Layering: `grep -rnE "StatusStyle|Theme::|Tokens|ui/|QtGui|QIcon|QPixmap|QColor|QPainter|QFont|QtWidgets|QSystemTrayIcon" src/domain src/data src/services src/models`
- No hard-coded colors: `grep -rnE "#[0-9a-fA-F]{6}" src/ui | grep -v "^src/ui/theme/"`
- No fixed sizes: `grep -rnE "setFixedSize|setFixedWidth|setFixedHeight" src/ui`
- No double ownership: `grep -rn "make_unique<.*ReminderService\|new ReminderService" src`
- `-functions | wc -l` per binary, totals by command, `git log --oneline`, `git show --stat` of each code commit.

## Final M5 report `docs/reports/M5-partD.md` (also the M5 summary)

- Plan vs result across all four parts with honest gaps; C0 outcomes; the amendment checklist with exact names (D1-D5 from `M5-amendments.md`).
- **SPEC §12 M5 acceptance quoted with `grep -n -A8 "M5 — Dashboard" docs/SPEC.md`** (do not paraphrase) and checked one by one with test slot names: numbers equal the hand-calculated data (Part A `testHandCalculatedDataset`, Part B `testDashboardNumbersMatchHandCalculatedDataset`), goal progress uses 360 by default and another goal in tests, a reminder for an interview in 59 minutes fires once and clicking opens the interview, no system tray -> in-app toast.
- All SPEC §14 (1.7) entries really committed (`grep -n "^- 1\.7" docs/SPEC.md | cut -c1-160`), including the tray menu, `ReminderCoordinator`, and the notification text helper.
- "Not verified by agent, user to check": the real tray path (Ubuntu ships the AppIndicator extension, so the tray path may be the one you get; the in-app path needs the extension disabled), notification click on GNOME/KDE/Windows, raise/focus on Wayland, reminders over hours and after sleep/resume, 100-200% scaling of toasts in both themes, toast stack at minimum window size, midnight rollover.
