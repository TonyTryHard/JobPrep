# M5 Part B: fix-ups (report accuracy and a few code checks)

Read `AGENTS.md`, `docs/SPEC.md` (§3.1 D1-D6, §6, §7.2), `docs/prompts/M5-amendments.md` (Part B section B1-B8), `docs/prompts/M5-partB-go.md` and `docs/reports/M5-partB.md`.
Part B is **not accepted yet**. Hashes, test totals (264 = 247 + 4 + 13) and the DayWatcher/C0 work look right, but the report contains labels and descriptions that do not match the amendments, the SPEC or Part A, and several required items are missing.
Fix the report first, change code only where a check shows a real problem. Local commits only, never push. Code, SPEC and report in separate commits.

Rules: raw command output with the command above it; no line numbers as evidence (`grep -n <symbol>` output and test slot names are fine); signatures and class names come from `grep -n` of the real headers, **not from memory**; hashes from `git log --oneline`; no claim without evidence.

## R1. Amendment checklist uses wrong names
The table labels B1-B8 do not match `M5-amendments.md`. Redo it with the exact names and one `grep`/slot-name evidence per sub-point:
B1 HomePage dependencies and navigation (signals, no MainWindow calls, empty-state buttons); B2 `MainWindow::openInterview` / `openApplication`; B3 one coalesced refresh, one computation; B4 day change (`DayWatcher`, separate from reminders); B5 formatting lives in `ui`; B6 Today list; B7 widgets; B8 screenshots.

## R2. Funnel and interview rate
The report says the funnel is "Applied, Screen, Technical, Offer, Accepted" and shows an "Offer-to-Interview conversion rate". SPEC D4 and §6: stages Applied -> HR screen -> Technical -> Final -> Offer (ranks 1..5 from `DashboardStats::funnel`), interview rate = reached(rank >= 2) / reached(rank >= 1).
- Show `grep -rn "Accepted\|Final" src/ui` and the labels used. If the widget or formatter is wrong, fix it.
- Test on the real widget/formatter with the Part A dataset: five labels in order, counts 9, 6, 4, 2, 1, rate 6/9 shown as agreed text, empty rate shown as "–" (`interviewRate` is `std::optional<double>`; the report says `double`: show how "no data" reaches the widget).

## R3. Empty state buttons
The report says "+ Add topic" "opens TopicDialog". Run `grep -rn "TopicDialog" src`. There is no topic dialog in M4: `+ New -> Topic` focuses the Backlog quick-add (or opens `TrackDialog` when there are no tracks).
- `HomePage` emits `addApplicationRequested()` / `addTopicRequested()`; `MainWindow` connects them to the same methods the header uses. Show the `signals:` block of `HomePage.h` and the `connect` lines.
- Rewrite the test description and the test if needed: Add application -> one `ApplicationDialog`; Add topic with tracks -> Study page, Backlog quick-add focused, no dialog; Add topic with no tracks -> one `TrackDialog`.

## R4. Dependencies
`DashboardController(AppContext &ctx, ...)` takes `AppContext&`. The agreed rule is explicit references (`StatsService&`, `AgendaService&`, the repositories whose `changed()` it listens to, `SettingsService&`, `DayWatcher&`, an injectable clock). Change it, and show `grep -n "DashboardController(" src/ui/DashboardController.h` and the `HomePage` constructor before and after.

## R5. Settings signals and refresh triggers
The report says the controller listens to `SettingsService::changed` (and calls it "settingsRepo").
- Show `grep -n "changed" src/services/SettingsService.h`. B3 asks for `weeklyGoalMinutesChanged` and `firstDayOfWeekChanged` only. If a generic signal exists or was added and fires for sidebar, theme or splitter changes, the dashboard recomputes on every splitter move. Use the two specific signals.
- Add a test: changing the theme, the sidebar state and a splitter state does not change `refreshCount()`.
- `HomePage::onActivated()` is a required trigger and is missing from the report's list; show it.

## R6. `MainWindow::openInterview` / `openApplication`
The report lists only page methods (`JobsPage::openApplication`, `CalendarPage::openInterview`) and says pages "switch to the page". Pages do not switch the stack. Run `grep -n "openInterview\|openApplication" src/ui/MainWindow.h`.
- B2 requires `MainWindow::openInterview(int)` and `openApplication(int)`: switch to the owning page, then call the page method. Add them if missing.
- Tests: both called directly from another page; deleted item gives the non-fatal message; from a Today item of each kind.

## R7. Tests: name them and map them to the go file
Run `git diff d53a700..0ef7f91 -- tests/ui_smoke/tst_UiSmoke.cpp | grep -E '^\+.*void test'` and paste it. Map each slot to the required list in `M5-partB-go.md`:
dashboard visited in light and dark; numbers equal the Part A dataset (applied 2, active 8, funnel 9/6/4/2/1, rate 6/9, 260 minutes); empty-state buttons; Today item opens interview / follow-up / topic and each deleted in the meantime; `openInterview` / `openApplication` directly; **day change with an injected clock refreshes Today and the streak**; burst of `changed()` -> one refresh; goal 6 h -> 8 h; first day of week; status bar scenarios (no data, streak 1 and 4, no upcoming interview, next interview); widget checks with 1, 2 and 8 tracks.
Add what is missing. Also show where the formatting unit tests are (260 -> "4h 20m / 6h"; 430 -> "7h 10m / 6h" with 100% bar and real 119%; empty rate "–"; 1 vs 2 days). If there is none, add `tst_FormatHelper` (and update the count by command).

## R8. Raw evidence that is missing or summarized
- `grep -rnE "#[0-9a-fA-F]{6}" src/ui | grep -v "^src/ui/theme/"` (expected: empty). The report has a comment line instead of output.
- Raw `./scripts/screenshots.sh` output, raw `md5sum docs/reports/img/dashboard-*.png`, and `git status --short` afterwards.
- `git show --stat 0ef7f91` (what the code commit really contains: SPEC? SettingsService? MainWindow?).
- Layering grep again at the end.

## R9. Public API from the headers
Replace the "New Public Methods" section with `grep -n` output from the real headers. The report shows `Domain::TrackProgress`, `Domain::Funnel` and `double interviewRate`, which do not match Part A (`DashboardStats` lives in `src/services/StatsService.h`). Paste the real signatures of `StatCard`, `ProgressRing`, `FunnelWidget`, `GoalProgressBar`, `DayWatcher`, `DashboardController`, the format helper and the new page methods.

## R10. SPEC §14 (1.7) for Part B
Show `git show --stat 0ef7f91 | grep SPEC` and `grep -n "^- 1\.7" docs/SPEC.md | cut -c1-140`. Part B entries are expected for: `DayWatcher`; `DashboardController` and its snapshot; the format helper; `GoalProgressBar` (a widget not in the plan or SPEC §8); `EmptyState` secondary action; `MainWindow::openInterview` / `openApplication` and the page methods. Add what is missing (SPEC change in its own commit).

## R11. Statements to remove or correct in the report
- "consistent Inter typography": Inter is not bundled until M6. Remove.
- "Not verified" item 1 mentions `INotifier` and the tray; neither exists until Part C. Rewrite the list: dashboard look at 100-200% scaling in both themes, real click-through of Today / Upcoming items, status bar at narrow window width, midnight rollover by hand, Sunday-first week on the dashboard, empty dashboard on a fresh data folder.
- "Gaps vs. Plan: None": list real gaps or write "none found by the agent" with the evidence above.
- "13 UI smoke test slots": give the names (R7).

## R12. Finish
Update `docs/reports/M5-partB.md` in place, add a "Changes in round 2" section with new hashes from `git log --oneline`, and finish with `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` (raw output) and the per-binary `-functions` totals computed by command.
