# M5 Part C: go (reminder core, no UI), with Part B carry-overs

Read `AGENTS.md`, `docs/SPEC.md` (§3.5, §5 `reminders_sent`, §8, §14), `docs/plans/M5-plan.md`, `docs/prompts/M5-amendments.md` (Part C section C1-C7 and the report rules),
`docs/reports/M5-partB.md`, and the code: `src/data/ReminderLogRepository.*`, `InterviewRepository.*`, `ApplicationRepository.*`, `src/services/SettingsService.*`, `src/services/DayWatcher.*`, `src/app/AppContext.*`.
Part B is accepted **with the carry-overs in C0**. Do C0 first as its own local commit, then Part C in one or more commits. Local commits only, never push.
Do **not** start Part D. Part C has **no UI**: no widgets, no `TrayNotifier`/`InAppNotifier`/`Toast`, no `MainWindow` wiring, no timer started in the real app (there is no real notifier yet; wiring is Part D).

## Report rules (same as before)

1. Every command output is raw, from a real run, with the exact command above it. Do not shorten or trim output (the Part B report cut a `grep -A 10` short).
2. No line numbers as evidence; `grep -n <symbol>` output and test slot names are fine. Hashes only from `git log --oneline`; report written last and committed separately.
3. Test totals computed by command (baseline now: 21 binaries, `ui_smoke` 78, total 265). New binaries and slots must add up.
4. Scope comes only from SPEC and amendments. Say honestly what is not done or not tested: no "none found" without evidence.

## C0. Carry-overs from Part B (one small commit)

1. `docs/reports/M5-partB-errata.md`: the Part B round 1 report claimed a funnel with `Offer, Accepted`, a `TopicDialog`, `AppContext&` in `DashboardController`, "Inter typography" and 13 named slots when 8 were named; the interview rate was really displayed as a fraction (0.7%) until `cbefc94`. State what was wrong, what was fixed and in which commit hash (from `git log --oneline`).
2. Paste the raw output of: `grep -n "void " tests/services/tst_DayWatcher.cpp`. The go file for Part B required: no signal on the same day, one signal on change, no repeat, date moving backwards. Add the missing slots.
3. Raw `grep -n "firstDayOfWeek" src/ui/DashboardController.cpp src/services/StatsService.cpp src/services/SettingsService.cpp`: show how the stored `int` becomes `Qt::DayOfWeek`.
4. `FormatHelper` has two `formatNextInterview` overloads and two streak helpers (`formatStreak`, `formatStreakDays`). Run `grep -rn "formatNextInterview\|formatStreakDays\|formatStreak(" src/ui` and remove any overload that nothing calls (AGENTS: no dead code). Also show one assert that checks the **rendered** interview-rate text from the funnel widget ("66.7%"), not only `stats.interviewRate`.

## Part C scope (amendments C1-C7, refined below; this file wins on conflict)

**Services (`src/services/`, no Qt Widgets, no QtGui):**

- `INotifier.h`: `class INotifier { virtual bool notify(const Notification&) = 0; }`. **Contract (C1):** returns true only if the notification was really presented to the user; false means "not shown, retry on the next tick" and nothing is recorded.
- **`Notification` carries data, not text** (this replaces the `title`/`message` fields of amendment C1): `kind` (`Interview`, `FollowUp`, `OverdueFollowUps`), `refId` (interview id or application id, 0 for a summary), `QString company`, `QString position`, `std::optional<Domain::InterviewType> interviewType`, `QDateTime when`, `int leadMinutes`, `int count`. Services cannot use `StatusStyle`/`tr()` labels for enums; the notifiers in `ui` (Part D) build the visible text. Record this in §14.
- `ReminderLogRepository::sentAt(QStringView key) -> std::optional<QDateTime>` (C2 needs `sent_at`; today only `wasSent`/`sentKeys` exist). Tests for it. Do not add `removeForInterview`.
- `ReminderService : QObject`, explicit references (`InterviewRepository&`, `ApplicationRepository&`, `ReminderLogRepository&`, `INotifier&`), injectable `std::function<QDateTime()>` clock, `setLeadMinutes`, `start()` (startup pass once, then a 60 s timer), `checkNow(bool startupPass = false)` for tests (no real waiting in tests).
- **Pure computation** separated from side effects: a function (static or free) that takes `now`, `startupPass`, interviews, follow-up applications, overdue applications and a "key is valid-sent" predicate and returns the notifications **with the keys to record after delivery**. This is the main target of the unit tests.

**Rules (C2, C3, refined):**

1. Interview lead L is due when `now >= start - L` and `now < start`. Cancelled interviews and interviews that already started are skipped. Only `cancelled` excludes an interview (rule A5; closed applications do not matter).
2. **Reschedule (C2):** a key `interview:<id>:<lead>` counts as sent only if its `sent_at >= start_at - lead`; otherwise it is stale and the lead is due again. No schema change, no write into reminders from `InterviewRepository`.
3. If several leads are due together, send **one** notification for the smallest due lead. If a smaller lead was already delivered for this start time, mark the larger due leads as sent **without** notifying (covers a lead time added later in settings). All keys are recorded only after `notify` returned true, with `markSent(key, now)` using the injected clock (never the default argument).
4. Follow-up: due when `next_action_date == today`, `now` is at or after 09:00 and the application is not closed; key `followup:<appId>:<yyyy-MM-dd>`.
5. **Overdue follow-ups (refines C3, SPEC R1 "mentioned once on startup"):** only in the startup pass, for non-closed applications with `next_action_date < today` and no key `overdue:<appId>:<today>`. Send **one** notification: for a single application kind `OverdueFollowUps` with its `refId`, for several one summary with `count` and `refId = 0`. Record every overdue key after delivery, so a restart on the same day does not repeat it. Record this in §14.
6. If `notify` returns false, record nothing; the next 60 s tick tries again (no burst, no duplicate).
7. No pruning of old keys.

**Settings (C4):** `reminders/interviewLeadMinutes` in `SettingsService`: list of ints, default 1440 and 60, sanitized (positive, unique, sorted descending; an empty or invalid stored value falls back to the default), accessor, setter and a change signal. `ReminderService::setLeadMinutes` is connected to it where the service is composed (Part D). No settings UI (M6). There is no `tst_SettingsService` yet: add one (or put the slots in an existing target with a clear name) with a temporary settings file; test default, sanitizing, signal emitted once per real change.

**Composition (C5):** write down (report, no code beyond what Part C needs) where `ReminderService` and the notifier will be created in Part D: the notifier needs the window, so `ReminderService` is created after `MainWindow` and owned with a Qt parent; `AppContext` stays free of UI. Do not add it to `AppContext` or `main.cpp` in this part.

**Close to tray (C7):** not part of M5 (deferred to M6, §14).

## Tests (named slots; zero warnings; `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` green)

`tst_ReminderService` (fake `INotifier` that records calls and can be told to return false; fixed clock; in-memory DB):
- interview 59 minutes ahead: one notification, both keys recorded, **no repeat** on the next `checkNow()` and after constructing a new service on the same DB (restart);
- 24 h and 1 h leads fire separately when the app was running (1440 first, 60 later);
- smaller lead already delivered, larger due lead added later: marked, not notified;
- cancelled, started and past interviews: nothing; an interview of a rejected application **is** reminded (rule A5);
- reschedule: moved later -> fires again; moved earlier into the window -> fires; deleted -> nothing; not rescheduled -> no repeat;
- notifier returns false: nothing recorded, next tick delivers once;
- follow-up before 09:00 -> nothing, at 09:00 -> once, closed application -> nothing;
- overdue: startup pass only, one application -> that application, three applications -> one summary with `count 3`, same-day restart -> nothing, next day -> again;
- lead minutes change takes effect on the next check;
- DST/day border: an interview at 00:30 with the 1440 lead is due the previous day.

Also: `tst_ReminderLogRepository` slots for `sentAt` (present, absent, updated by a second `markSent`? state what `markSent` does on an existing key), `tst_SettingsService` as above. Existing tests stay unchanged and green.

## Checks to show in the report (raw output)

- Layering: `grep -rnE "StatusStyle|Theme::|Tokens|ui/|QtGui|QIcon|QPixmap|QColor|QPainter|QFont|QtWidgets|QSystemTrayIcon" src/domain src/data src/services src/models`
- `grep -rn "removeForInterview\|markSent(" src` (no default-clock `markSent` calls in `ReminderService`).
- `-functions | wc -l` per binary, totals by command, `git log --oneline`, `git show --stat` of the code commit.

## Report `docs/reports/M5-partC.md`

Plan vs result with honest gaps; C0 outcomes; the amendment checklist with exact names (C1-C7 from `M5-amendments.md`, noting where this file refines them); the rules above as a table with the test slot that proves each row; §14 entries (1.7) really committed (notifier layering, `INotifier` contract, data-carrying `Notification`, reschedule by `sent_at`, smallest-lead coalescing, overdue summary, lead-minutes key, close-to-tray deferred); hashes; "Not verified by agent, user to check" (at least: nothing here is visible in the app yet; real timer behavior over hours; behavior across sleep/resume).
