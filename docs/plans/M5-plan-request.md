# M5: plan request (Dashboard and reminders). PLAN ONLY, no code

Read `AGENTS.md`, `docs/SPEC.md` (at least §3.1, §3.5, §3.7, §4, §6, §7.1, §7.2, §8, §10, §11, §12 M5, §14),
`docs/reports/M4-partC.md`, `docs/reports/M3*.md` (Agenda, Calendar) and the real code under `src/`.
If `docs/prompts/M5.md` exists, read it too; **this file adds constraints and wins on any conflict**.

Deliverable: `docs/plans/M5-plan.md`, committed locally on its own (`docs: add M5 plan`). Do **not** write production code or tests in this step. Never push.
Then stop and wait for approval. The plan will come back with amendments, like M3 and M4.

## Evidence rules (earlier reports contained invented output, so these are strict)

1. Every statement about the existing code is backed by a command you ran in this session, with its raw output: `grep -n`, `ls`, `sed -n` of a few lines. **No line numbers from memory.**
2. Do not claim a file, class, method, setting key or test slot exists unless a command showed it. If something does not exist, say so.
3. Scope comes only from the SPEC and this file. Do not add features from §13 (Future ideas) or invent criteria. If you want something extra, put it under "Proposals", clearly separated.
4. Commit hashes only from `git log --oneline`.

## What the plan must contain

### 1. Pre-flight findings (verified against the code, table with the command used)
At least these, each answered with raw output:
- `AgendaService`: public API; how it treats **overdue** follow-ups and study targets (before `from`), Done topics, closed applications, cancelled interviews; what `AgendaItem` carries (`refId`, `kind`, `past`).
- `ApplicationRepository`, `InterviewRepository`, `SessionRepository`, `TopicRepository`, `ReminderLogRepository`: existing query methods and what is missing for SPEC §6 (counts by status, "reached stage" via `status_history`, applied in week, distinct session dates for the streak, minutes per week).
- `SettingsService`: existing keys for weekly goal, first day of week, reminder lead times, close-to-tray; which exist, which are missing.
- `MainWindow`: current status bar content, existing `openTopic(int)`, existing ways to open an application / an interview from another page, the existing `Toast`/message mechanism, how the window is raised today.
- `HomePage` (or whatever the dashboard stub is called): constructor and wiring today; `PageBase::onActivated()` use.
- Existing clocks: which classes already take `std::function<QDate()>`.
- Existing test targets and CMake layout for adding new tests.

### 2. SPEC and AGENTS cross-check table (conflict -> resolution), like `M4-amendments.md`
Known points you must address (add any others you find):
- SPEC §8 lists `TrayNotifier` and `InAppNotifier` under `services`, but `QSystemTrayIcon` is QtWidgets and a toast is UI. AGENTS §5 layering is strict. Proposal to evaluate: `INotifier` and a plain `Notification` struct in `services`; `TrayNotifier` and `InAppNotifier` in `ui/`. Needs a §14 entry (1.7).
- R2 key format `interview:<id>:<leadMinutes>` does not contain the start time. An interview that is **rescheduled** after its reminder fired would never remind again. Propose a solution (for example forgetting keys when `start_at` changes) and its layering; any deviation gets a §14 entry.
- When the app starts 59 minutes before an interview, both the 1440 and the 60 minute reminders are "due". Propose one notification (the smallest due lead) and mark the others as sent; test it.
- No reminders for cancelled interviews (`outcome`), past interviews, or follow-ups of closed applications.
- Overdue follow-ups: "mentioned once on startup" (R1) versus key `overdue:<appId>:<yyyy-MM-dd>`: state exactly when it fires and that a restart on the same day does not repeat it.
- Clicking a notification must raise the window and open the item. `raise()` under the offscreen test platform prints "This plugin does not support raise()", and the `ui_smoke` noise filter must **not** be widened. Propose how this stays testable (for example a small `MainWindow::bringToFront()` that tests do not call, with the open-item part tested separately).
- R4 "Close to tray" and the tray icon (the app icon arrives in M6): propose what M5 does and what waits for M6. Default recommendation: no settings UI in M5, behavior only if cheap, §14 entry.
- Midnight rollover while the app is open: dashboard, "Today" list and streak must refresh when the date changes.
- Many `changed()` signals at once (restore, bulk edits): coalesce dashboard recomputation (one refresh per event-loop turn).
- "Interviews in next 7 days" (D1) is not exactly defined: choose a definition, state it as an assumption, exclude cancelled interviews.
- Study streak and weekly minutes with sessions dated in the future (the session dialog allows any date): state the rule.
- Performance (§11): stats must come from aggregate queries in the **data** layer (only `data` touches SQL). Estimate with 1,000 applications and 5,000 sessions; propose a non-flaky sanity test.

### 3. Design
- **Domain / services:** result structs (plain types, no QtGui), `StatsService` API (goal and week start passed in, injectable `today`, never reads settings), track progress via `domain::topicProgress()` and `TopicRepository::subtaskProgress()` (no N+1), `AgendaService` changes only if needed (each needs a §14 entry; M4 amendment A7: Done topics are never due or overdue).
- **Reminders:** `ReminderService` API, pure due-reminder computation (`now`, interviews, applications, already-sent keys -> list of notifications), injectable `std::function<QDateTime()>` clock, 60 s timer, startup pass, `INotifier` contract (what "delivered" means; `reminders_sent` is written only after delivery), automatic choice `isSystemTrayAvailable()` at startup, `InAppNotifier` must never lose a reminder (sticky toast or queue until the window is visible).
- **UI:** `StatCard`, `ProgressRing`, `FunnelWidget` (custom painted: tokens only, `devicePixelRatio`, no per-paint allocations, sizes from font metrics, no fixed pixel sizes), Today and Upcoming lists fed by `AgendaService` (not by a model), empty state with "Add application" and "Add topic" calling their targets directly, status bar summary, accessible names/descriptions for the custom widgets, funnel counts readable as text (not color alone), same status colors from `StatusStyle`, layouts for any number of tracks (rings wrap).
- **Navigation:** clicking a Today / Upcoming item or a notification calls `MainWindow::openTopic(int)` (exists) and new `openInterview(int)` / `openApplication(int)`-style methods; reuse the same ones for both. List the new public methods.
- **Refresh:** which signals each piece listens to (`changed()` of which repositories, settings change signal for the goal), plus `onActivated()`.

### 4. Parts (approval gate between parts, as in M4)
Propose a split into 2-4 parts, each its own commit(s), `check.sh` green and its own report. Suggested starting point to challenge: A) data queries + `StatsService` + tests; B) dashboard widgets, page, navigation methods, status bar; C) `ReminderService`, notifiers, toast, end-to-end tests, screenshots, final report. Say if you prefer another order and why.

### 5. Tests
List every planned test target and slot name (new `tst_StatsService`, `tst_ReminderService`, repository additions, `ui_smoke` additions) and what each asserts. Required cases:
- Stats on a **hand-calculated** dataset (write the arithmetic in comments), fixed clock, both week-start settings, goal 360 and another goal, goal text "7h 10m / 6h" style, bar capped at 100% with real value in the text.
- Funnel: current status or any history row counts; accepted = rank 5; rejected/withdrawn/ghosted use history; interview rate "-" when the denominator is 0.
- Streak: today empty -> starts from yesterday; gap breaks it; future-dated sessions per your rule.
- Reminders: interview in 59 minutes fires once (and only one notification), no repeat after a restart, cancelled/past/closed cases, follow-up at 09:00, overdue once, reschedule behavior, fake `INotifier`, no tray -> in-app notifier chosen, clicking opens the item.
- `ui_smoke`: Dashboard visited in both themes, empty state buttons open exactly one dialog each, Today item opens topic / interview / application, midnight rollover with an injected clock, no unexpected warnings.
- Screenshots (offscreen `QWidget::grab()`, light and dark, 100% only): dashboard with data, empty dashboard, in-app reminder toast. Say how `scripts/screenshots.sh` is extended.

### 6. Risks, files touched, SPEC §14 entries expected (version 1.7), and "Not verified by agent, user to check".

## Final message of this step
List the plan path, the commit hash from `git log --oneline`, the open questions for the user (if any), and nothing else. Do not start Part A.
