# M5 Part B: go (dashboard UI), with Part A carry-overs

Read `AGENTS.md`, `docs/SPEC.md` (§3.1, §6, §7.1, §7.2, §8, §14), `docs/plans/M5-plan.md`, `docs/prompts/M5-amendments.md` (Part B section B1-B8 and the report rules),
`docs/reports/M5-partA.md`, and the real code: `src/services/StatsService.h`, `src/services/AgendaService.h`, `src/ui/MainWindow.*`, `src/ui/pages/HomePage.*`, `CalendarPage.*`, `JobsPage.*`.
Part A is accepted **with the carry-overs in C0**. Do C0 first as its own local commit, then Part B in one or more commits. Local commits only, never push. Prompt and report files go in their own commits.
Do **not** start Part C (reminders) or Part D (notifiers, toast).

## Report rules (same as `M5-amendments.md`, plus one)

1. Every command output is raw, from a real run, with the exact command above it. No line numbers as evidence (`grep -n <symbol>` output and test slot names are fine).
2. Hashes only from `git log --oneline`; report written last and committed separately.
3. **Totals must equal the sum of the rows.** The Part A report said 182 + 25 = 207, but its own table rows add up to 221 before, 26 added, 247 after. Compute totals from the raw `-functions` output with a command (for example `... | awk '{s+=$NF} END {print s}'`), not by hand.
4. Scope comes only from SPEC and amendments. No invented criteria, no features from §13.

## C0. Carry-overs from Part A (one small commit)

1. `docs/reports/M5-partA-errata.md`: the correct totals (recomputed with a command), and the report commit hash (`git log --oneline`).
2. SPEC §14 (1.7): one line for the new repository queries that have no entry yet: `ApplicationRepository::appliedCountInRange`, `funnelStageCounts`, `byNextActionDateUpTo`, `SessionRepository::distinctSessionDates`, `TopicRepository::byTargetDateUpTo`. (SPEC change in its own commit.)
3. Answer in the Part B report, with raw `grep`: does an `AgendaItem` of kind follow-up carry a `time`? The SPEC D2 mockup shows "09:00 Follow up: Acme". Show a time in the Today list **only** if the item has one; do not invent 09:00.

## Part B scope (amendments B1-B8; the points below are binding)

**Services (no Qt Widgets):**
- `DayWatcher` (QObject in `src/services/`): injectable `std::function<QDate()>` clock, 60 s timer, `checkNow()` for tests, signal `dayChanged(QDate)`. Not part of `ReminderService`. New test `tst_DayWatcher` (no real waiting: inject a clock and call `checkNow()`).

**UI (`src/ui/`):**
- `StatCard`, `ProgressRing`, `FunnelWidget` (B7): tokens only, `devicePixelRatio`, sizes from font metrics (no fixed pixel sizes), no per-paint allocations, accessible name and description, funnel counts as readable text, rings wrap for any number of tracks, long track names elide with a tooltip, zero values render cleanly.
- Weekly goal bar: capped at 100% for the bar, real value in text ("7h 10m / 6h").
- A formatting helper in `ui/` (B5): goal text, "–" for an empty interview rate, streak wording (singular/plural), funnel labels from `StatusStyle`, status bar "Next: Fri 14:00 Globex" with `QLocale`. All through `tr()`. Unit-tested (260 -> "4h 20m / 6h", 430 -> "7h 10m / 6h" with 100% bar and real 119%, rate empty -> "–", 1 vs 2 days).
- **Single computation per refresh (B3).** Propose and implement one small owner (suggestion: `DashboardController` in `src/ui/`, QObject, explicit references) that holds the latest snapshot (`DashboardStats`, `AgendaService::todayItems`, `AgendaService::upcomingInterviews(now, 5)`, today's date) and emits `snapshotChanged`. `HomePage` and the status bar both read this snapshot; `computeStats` runs once per refresh, not once per consumer. You may choose another design if you explain why in the report.
- Refresh triggers (B3): `changed()` of applications, interviews, sessions, topics and tracks; `SettingsService::weeklyGoalMinutesChanged`; **`firstDayOfWeekChanged`**; `HomePage::onActivated()`; `DayWatcher::dayChanged`. All coalesced into one zero-interval single-shot per event-loop turn. Give a test seam (for example a refresh counter) that shows 10 `changed()` signals in a row cause one refresh.
- `SettingsService::firstDayOfWeek()` is an `int`; show how it maps to `Qt::DayOfWeek` (raw `grep`) and test Sunday-first through the UI path.
- `HomePage` (B1): explicit constructor references, no `AppContext&`; never calls `MainWindow`. Signals: `addApplicationRequested()`, `addTopicRequested()`, `interviewActivated(int)`, `applicationActivated(int)`, `topicActivated(int)`; `MainWindow` connects them.
- Layout per SPEC §7.2: four `StatCard`s, Today, Upcoming, Study progress (rings + goal bar), Hiring funnel with interview rate. Empty state (D6) when there are no applications and no topics, with "Add application" and "Add topic"; each opens exactly one dialog. Resizes cleanly at 1100x700 and larger; no fixed sizes.
- Today list (B6): say which widget/model you use (reuse `AgendaListModel` only if it fits and stays free of UI includes). Overdue items are labelled "Overdue" in text; earlier-today interviews dimmed; Enter/double-click activates; tooltips; empty line "Nothing planned for today". Upcoming: next 5 interviews, same activation.
- `MainWindow::openInterview(int)` and `openApplication(int)` (B2): switch to the owning page (Calendar with that interview's day selected; Jobs with the row selected) and reuse that page's existing edit path. First `grep` for existing public methods on `CalendarPage` / `JobsPage`; add small public methods there if missing; do not construct dialogs in `MainWindow`. A deleted item gives the same non-fatal message as `openTopic`. `openTopic(int)` is reused as is.
- Status bar: the three existing labels (`m_statusStreakLabel`, `m_statusProcessesLabel`, `m_statusNextInterviewLabel`) are filled from the snapshot.
- Keyboard access and tooltips on every control; lists reachable with Tab; Today/Upcoming activate with Enter.

## Tests (named slots; zero warnings; `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` green)

- `tst_DayWatcher`: no signal on the same day; one signal when the injected date changes; no repeat; date moving backwards.
- Formatting tests as listed above.
- `ui_smoke` additions (update the old static Home assertion if there is one):
  - dashboard visited in light and dark;
  - dashboard numbers equal the Part A hand-calculated dataset (build it with the same helper if possible: appliedThisWeek 2, active 8, funnel 9/6/4/2/1, rate 6/9, 260 minutes);
  - empty-state buttons: exactly one dialog each;
  - Today item opens: interview, follow-up, topic; each also with the item deleted in the meantime;
  - `openInterview` / `openApplication` called directly from another page;
  - day change with an injected clock refreshes Today and the streak;
  - 10 `changed()` signals cause one refresh;
  - weekly goal 6 h -> 8 h updates the bar and text immediately;
  - first day of week change refreshes the week numbers;
  - status bar text for: no data, streak 1 and 4, no upcoming interview, next interview.
- Widget checks without pixel constants: `minimumSizeHint` fits the text, rings wrap with 1, 2 and 8 tracks, no overlap.
- Screenshots (offscreen `QWidget::grab()`, light and dark, 100% only): dashboard with data, empty dashboard. Extend `scripts/screenshots.sh`, run it, paste the raw output and `md5sum` of the new images (all different), and `git status --short` afterwards.

## Checks to show in the report (raw output)

- Layering: `grep -rnE "StatusStyle|Theme::|Tokens|ui/|QtGui|QIcon|QPixmap|QColor|QPainter|QFont" src/domain src/data src/services src/models`
- No hard-coded colors: `grep -rnE "#[0-9a-fA-F]{6}" src/ui` (only `src/ui/theme/` allowed).
- No fixed sizes in the new widgets: `grep -rnE "setFixedSize|setFixedWidth|setFixedHeight" src/ui`.
- `-functions | wc -l` per test binary with totals computed by command (rule 3).

## Report `docs/reports/M5-partB.md`

Plan vs result with honest gaps; C0 outcomes with raw output; amendment checklist with the exact names (B1-B8) and one grep/test-slot evidence per sub-point;
design of the refresh owner; list of new public methods; §14 entries (1.7) really committed; hashes; "Not verified by agent, user to check" (at least: dashboard look at 100-200% scaling in both themes, real Today click-through, status bar text at narrow width, midnight rollover by hand).
