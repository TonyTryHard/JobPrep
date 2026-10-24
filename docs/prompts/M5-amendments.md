# M5 plan amendments (and Part A go), v1

Read `AGENTS.md`, `docs/SPEC.md`, `docs/plans/M5-plan.md` and this file.
The M5 plan is **approved with the amendments below**. Do not rewrite the plan: plan + this file are the contract. Where they differ, this file wins.
Anything that deviates from the plan, this file or the SPEC gets a SPEC §14 entry (version **1.7**). Local commits only, never push. Prompt files and report files go in their own commits.

## Parts: four, not three

- **Part A (do now):** data queries, `domain::stageRank`, `StatsService`, `AgendaService` additions, and **all their tests**. No UI.
- **Part B (later go file):** dashboard widgets, `HomePage`, navigation methods, status bar, refresh logic.
- **Part C (later go file):** `ReminderService`, `INotifier` contract, settings key, composition. No UI widgets.
- **Part D (later go file):** `TrayNotifier`, `InAppNotifier`, `Toast`, click-to-open, end-to-end tests, screenshots, final M5 report.

Each part: own commit(s), `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` green with zero warnings, own report `docs/reports/M5-partA.md` etc.
**Implement Part A only, write its report, and stop.** The B, C and D amendments are binding; read them now so Part A does not block them.

## Report rules (strict, same as M4)

1. Every command output is raw, copied from a real run in this session, with the exact command above it.
2. **No line numbers** as evidence. Use `grep -n <symbol>` output, test slot names or `git show --stat`.
3. Hashes only from `git log --oneline`; write the report after the code commits and commit it separately.
4. Check arithmetic yourself. The plan's example dataset had wrong numbers (see A8); a "hand-calculated" test must really be hand-calculated.
5. Scope comes only from SPEC and amendments. Extras go under "Proposals".

## Plan review: what is accepted and what changes (summary)

Accepted: pre-flight findings, notifier layering (`INotifier` in `services`, concrete notifiers in `ui`), smallest-lead coalescing, overdue-once-per-day, aggregate SQL for stats, open-item methods, coalesced refresh, hand-calculated tests, §14 plan.
Changed: items below. Most important: A1 (no presentation in services), A8 (arithmetic), C1/C2 (delivery contract, reschedule), C9 (close-to-tray out of M5), A3 (do not change `inRange`).

---

## Part A amendments (implement now)

**A1. Numbers only in `StatsService` results.** `DashboardStats` must not contain presentation: no `goalProgressText`, no `interviewRateText`, no `FunnelStageStat::label`, no `std::string`-like labels, no default goal value.
Fields are numbers/enums/hex: `appliedThisWeek`, `activeProcesses`, `interviewsNext7Days`, `studyStreakDays`, `weekMinutes`, `goalMinutes` (echo of the argument), `goalPercentReal` and `goalPercentCapped`, `funnel` (`status`, `reachedCount` for ranks 1..5), `std::optional<double> interviewRate` (empty when the denominator is 0), `trackProgress` (`trackId`, `trackName`, `colorHex`, `percent`).
Formatting ("4h 20m / 6h", "–", labels from `StatusStyle`) is done in `ui` (Part B). Define the struct in `src/services/StatsService.h` (derived values are not domain model); say so in the report. `domain` gets only `stageRank` (A2).

**A2. `domain::stageRank(ApplicationStatus)`.** Pure function returning `std::optional<int>`: wishlist 0, applied 1, hr_screen 2, technical 3, final 4, offer 5, accepted 5; rejected, withdrawn, ghosted: `std::nullopt` (SPEC §6). Unit test for every enum value.
`ApplicationRepository::funnelStageCounts()` returns reached counts for ranks 1..5 in one query: an application reached rank N if its current status or any `status_history.to_status` has rank >= N. The status lists per rank come from `stageRank` (no second rank table hidden in SQL text).

**A3. Repository additions, existing behavior untouched.**
- `ApplicationRepository::appliedCountInRange(QDate from, QDate to)` (inclusive, state it), `funnelStageCounts()`.
- `SessionRepository::distinctSessionDates(QDate upTo)`: descending, dates <= `upTo` only.
- `InterviewRepository`: **do not change `inRange` or its callers' behavior.** Add `countUpcoming(QDateTime from, QDateTime to)` and `upcoming(QDateTime from, int limit)`, both excluding `cancelled`. Run `grep -rn "nextUpcoming" src tests` and report every caller; change `nextUpcoming` only if every caller wants cancelled excluded, otherwise leave it.
- All existing repository and Calendar tests stay unchanged and green.
- Tests (named slots) for each new method: boundaries inclusive/exclusive as documented, cancelled excluded, empty DB, history-only reach for `funnelStageCounts`.

**A4. `AgendaService` additions move into Part A, with tests.**
`todayItems(QDate today)` and `upcomingInterviews(QDateTime from, int limit)`.
- `todayItems`: interviews on `today` (including earlier today, `past` set), follow-ups with `next_action_date <= today` for non-closed applications, topics with `target_date <= today` and status not Done. Cancelled interviews excluded.
- Define and document the order: overdue follow-ups and overdue topics first (oldest date first), then today's items by time (items without time after timed ones). Overdue is "date < today"; do not add a field to `AgendaItem` unless needed (then §14).
- New repository queries for "up to date" (for example `byNextActionDateUpTo`, `byTargetDateUpTo`) instead of passing a fake very old `from`.
- `items(from, to)` behavior unchanged; its tests stay green.
- `tst_AgendaService` new slots: overdue follow-up included, overdue topic included, Done topic excluded, closed application excluded, cancelled interview excluded, interview earlier today has `past`, order as defined, limit and ordering of `upcomingInterviews`, month-border and year-border cases.

**A5. One rule for interviews, used everywhere.** Only `outcome == cancelled` removes an interview from "next 7 days", Upcoming, Today, status bar "Next:" and (Part C) reminders. Interviews of rejected/withdrawn/ghosted applications are still shown and reminded until the user marks them cancelled; this matches Calendar today. Record it in §14. (The user may change this rule; do not invent another one.)

**A6. "Interviews in next 7 days" window.** `[now, QDateTime(today.addDays(6), 23:59:59)]`: today plus the six following calendar days, 7 days as the card label says. (The plan's `today + 7` covers up to 8 calendar days.) Boundary test: an interview at `today + 6, 23:00` counts, `today + 7, 09:00` does not, an interview earlier today (before `now`) does not. §14 entry.

**A7. `StatsService` details.**
- Signature `computeStats(int weeklyGoalMinutes, Qt::DayOfWeek firstDayOfWeek)` (not `int` for the day). Goal <= 0 must not divide by zero.
- Pure, tested week-bounds helper for Monday-first and Sunday-first.
- Track progress with a fixed number of queries: one `TrackRepository::all()`, one `TopicRepository::all()`, one `subtaskProgress()`; no per-track query. Average via `domain::topicProgress`; a track without topics is 0.
- Streak: consecutive days with >= 1 session ending today; if today has none, start from yesterday; sessions dated after today neither count nor break it.
- Weekly minutes: all sessions with date in the current week (SPEC §6), including a future-dated one inside the week; test this explicitly.
- `StatsService` uses one injected `std::function<QDate()>` clock; it never reads `SettingsService`.

**A8. Hand-calculated dataset (the plan's numbers were wrong).** The plan listed "10 applications" that add up to 11, counted 7 active (it is 8), and gave funnel 8/5/3/2/1 and rate 62.5% (the rejected application with history to Technical also counts for ranks 1-3). Also, a Monday clock with Tuesday/Thursday sessions makes those sessions future-dated. Use this dataset (or an equivalent one with the arithmetic checked in test comments):

Clock: Thursday 2026-10-29 10:00. Week Monday-first = Oct 26 - Nov 1.
Applications (11):
- 2 wishlist (no applied date)
- 3 applied: applied dates Oct 27, Oct 29 and Oct 20
- 2 hr_screen (applied dates before Oct 26)
- 1 technical, 1 final, 1 offer (applied dates before Oct 26)
- 1 rejected with history applied -> hr_screen -> technical -> rejected (applied date before Oct 26)
Expected: appliedThisWeek = 2; activeProcesses = 3+2+1+1+1 = 8 (wishlist and rejected excluded);
funnel reached counts rank1..5 = 9, 6, 4, 2, 1 (current statuses plus the rejected one for ranks 1-3);
interview rate = 6/9 = 66.7%.
Sessions: Sun Oct 25 = 30, Mon Oct 26 = 60, Tue Oct 27 = 120, Wed Oct 28 = 80, nothing Thu.
Expected Monday-first: weekMinutes = 60+120+80 = 260, streak = 4 (today empty, so Wed, Tue, Mon, Sun), percent for goal 360 = 72, for goal 480 = 54.
Expected Sunday-first (week Oct 25 - Oct 31): weekMinutes = 290, streak = 4.
Add a session Fri Oct 30 = 45: weekMinutes (Monday-first) = 305, streak still 4.
Goal text checks belong to Part B: 260 -> "4h 20m / 6h", 430 -> "7h 10m / 6h" with bar capped at 100% and real value 119%.
Interviews: Oct 30 14:00 and Nov 4 11:00 (count), Oct 31 cancelled (no), Oct 27 past (no), Nov 4 23:00 (counts), Nov 5 09:00 (no), Oct 29 08:00 earlier today (no).

**A9. Performance sanity.** 1,000 applications with history and 5,000 sessions in an in-memory DB; `computeStats` asserts a generous bound of < 1000 ms (the SPEC §11 cold-start budget) and prints the measured time with `qInfo`. Not 50 ms: Debug build, shared CI.

**A10. Not in Part A.** No `ReminderLogRepository` change (the reschedule solution is C2), no UI, no settings keys.

**A11. Part A report.** Plan vs result, raw layering grep (`grep -rnE "StatusStyle|Theme::|Tokens|ui/|QtGui|QIcon|QPixmap|QColor|QPainter|QFont" src/domain src/data src/services src/models`), `-functions | wc -l` for every test binary with old + new = total arithmetic, `git log --oneline`, §14 entries (1.7), "Not verified by agent, user to check".

---

## Part B amendments (binding, implemented later)

**B1. `HomePage` dependencies and navigation.** Explicit constructor references (no `AppContext&`), like `CalendarPage`. The page never calls `MainWindow`: it emits `addApplicationRequested()`, `addTopicRequested()`, `interviewActivated(int)`, `applicationActivated(int)`, `topicActivated(int)`; `MainWindow` connects them. Empty-state buttons open exactly one dialog each (test).

**B2. `MainWindow::openInterview(int)` / `openApplication(int)`.** They switch to the owning page (Calendar with that interview's day selected; Jobs with the row selected) and use that page's existing edit path. Do not construct dialogs inside `MainWindow`. First look for existing public methods on `CalendarPage` / `JobsPage`; add small public ones if missing. An item deleted in the meantime gives the same non-fatal message as `openTopic`. Tests: both methods, deleted case, from a Today item.

**B3. One coalesced refresh, one computation.** All `changed()` signals (applications, interviews, sessions, topics, tracks), `SettingsService::weeklyGoalMinutesChanged`, **`firstDayOfWeekChanged`**, `onActivated()` and the day change lead to one zero-interval single-shot refresh per event-loop turn. The stats are computed once and shared by `HomePage` and the status bar (not twice). State the design in the report and give a test seam (for example a refresh counter) showing that a burst of changes causes one refresh.

**B4. Day change.** A small service-level watcher with an injectable clock (not `ReminderService`) emits `dayChanged(QDate)`; dashboard, Today list, streak and status bar refresh on it. Test with an injected clock. One clock type for the dashboard side (`std::function<QDate()>`), and say how it is derived from the `QDateTime` clock used by `AgendaService`.

**B5. Formatting lives in `ui`.** Goal text, "–" for no interview rate, streak wording (singular/plural), funnel labels, status bar "Next: Fri 14:00 Globex" (`QLocale`). Tested. All text through `tr()`.

**B6. Today list.** Say which widget and model it uses (reuse `AgendaListModel` if it fits, fed from `AgendaService::todayItems`; no ad-hoc queries). Overdue items are labelled "Overdue" in text (not only by color), earlier-today interviews dimmed, Enter/double-click activates, tooltips, an empty line ("Nothing planned for today").

**B7. Widgets.** `StatCard`, `ProgressRing`, `FunnelWidget`: tokens only, `devicePixelRatio`, sizes from font metrics (no fixed pixel sizes), no per-paint allocations, accessible names/descriptions, funnel counts as readable text, rings wrap for any number of tracks, long track names elide with a tooltip, zero values render cleanly. 100-200% scaling. The weekly goal bar shows the real value in text and is capped at 100%.

**B8. Screenshots** (offscreen `QWidget::grab()`, light and dark, 100% only): dashboard with data, empty dashboard. Extend `scripts/screenshots.sh`.

---

## Part C amendments (binding, implemented later)

**C1. `INotifier` delivery contract.** `bool notify(const Notification&)` returns **true only if the notification was really presented to the user**. `InAppNotifier` returns false when the window is hidden or minimized; the key is then not written and the reminder is retried on the next 60 s tick. `reminders_sent` is written only after `true`, with `markSent(key, now)` using the **injected** clock (never the default argument). `Notification` carries `kind`, `refId`, `title`, `message`.

**C2. Reschedule without cross-repository writes.** Do not call `ReminderLogRepository::removeForInterview` from `InterviewRepository::update` (a repository writing into another table, and restore/import paths would bypass it). Use the existing `sent_at` column: a key `interview:<id>:<lead>` counts as sent only if `sent_at >= start_at - lead`; otherwise it is stale and the reminder is due again. This covers moving later, moving earlier into the due window, and import. Keys stay in the SPEC format; no schema change. Tests: not rescheduled -> no repeat; moved later -> fires again; moved earlier into the window -> fires; deleted interview -> nothing. §14 entry. (If you prefer another approach, argue it in the part report; default is this one.)

**C3. Which reminders.** Lead L is due when `now >= start - L` and `now < start`. If several leads are due at once, one notification for the smallest lead and all due leads are marked sent (once delivered). Excluded: cancelled and past interviews, follow-ups of closed applications (rule A5 applies to interviews). Follow-up: first pass at or after 09:00 on `next_action_date`. Overdue follow-ups: startup pass only, once per application per day (`overdue:<appId>:<yyyy-MM-dd>`). Do not add pruning of old keys.

**C4. Settings key.** `reminders/interviewLeadMinutes` in `SettingsService` (list of ints, default 1440 and 60; sanitized: positive, unique, sorted descending) with a change signal wired to `ReminderService::setLeadMinutes`. No settings UI (M6). The plan's file list missed `SettingsService`, `AppContext` and `main.cpp`: list every composition change.

**C5. Composition.** Say where `ReminderService` and the notifier are constructed (the notifier needs the window or `QApplication`). `AppContext` gets `StatsService`; `ReminderService` is created after the window exists, owned by Qt parent. A `makeNotifier(bool trayAvailable, ...)` function in `ui` with injectable availability, so both branches are testable. Move the plan's slots "fallback to in-app when no tray" and "click dispatches open item" out of `tst_ReminderService` into `ui_smoke`.

**C6. `tst_ReminderService`** covers only service logic with a fake `INotifier` and fixed clock: interview 59 minutes ahead fires once and only one notification, no repeat after restart, cancelled/past/closed, follow-up 09:00, overdue once per day, reschedule cases (C2), notifier returning false is retried, lead-time changes.

**C7. Close to tray is not part of M5.** No `closeEvent` behavior, no tray menu, no Settings UI (the plan's item 7 would be untested dead code, and a hidden window needs a tray menu with Quit). Move it to M6 and record R4 as deferred in §14.

---

## Part D amendments (binding, implemented later)

**D1. `TrayNotifier`.** Uses the existing themed SVG icon (list any new icon). `messageClicked` has no payload: store the last notification and open it. Returns true after `showMessage`.

**D2. `InAppNotifier` and `Toast`.** Sticky until clicked or dismissed; at most 3 visible, "+N more"; repositioned on window resize; border and surface contrast instead of `QGraphicsDropShadowEffect`; does not steal focus; Esc or the close button dismisses, Enter/click opens the item; accessible name and role; both themes.

**D3. Click to open.** `MainWindow::openItemFromNotification` = `bringToFront()` then the open method. `bringToFront()` has no `platformName()` branch; tests call only the open part (or inject a raise hook). The `ui_smoke` QPA noise filter must not be widened.

**D4. End-to-end.** Interview in 59 minutes fires once and clicking opens it; no tray -> in-app toast; reminder delayed while the window is minimized and delivered when it is shown.

**D5. Final M5 report.** SPEC §12 M5 acceptance criteria quoted from `docs/SPEC.md` (not paraphrased) and checked one by one with test slot names; screenshots dashboard data/empty/toast, light and dark; "Not verified by agent, user to check" including: tray notifications on GNOME/KDE/Windows (the user's GNOME probably has no tray, so only the in-app path is exercised on his machine), 100-200% scaling, midnight rollover by hand.

---

## Expected SPEC §14 entries (1.7), collected over the four parts

1. `INotifier`/`Notification` in `services`; `TrayNotifier`, `InAppNotifier`, `Toast` in `ui` (SPEC §8 listed them under services).
2. `INotifier::notify` returns true only when presented; reminder written after delivery.
3. Reschedule handled by `sent_at >= start_at - lead` validity (key format unchanged).
4. Smallest-lead coalescing; overdue follow-ups once per application per day.
5. "Interviews in next 7 days" = now to end of `today + 6`.
6. Only `cancelled` excludes an interview everywhere.
7. Streak and weekly minutes rules with future-dated sessions.
8. `DashboardStats` holds numbers only; `domain::stageRank`; new repository queries; `InterviewRepository::inRange` unchanged.
9. `AgendaService::todayItems` and `upcomingInterviews`.
10. `MainWindow::openInterview(int)`, `openApplication(int)`, `bringToFront()`.
11. `reminders/interviewLeadMinutes` setting (UI in M6).
12. R4 "Close to tray" deferred to M6.
