# M3 plan — amendments

Your plan `docs/plans/M3-plan.md` is approved **with the amendments below**.

What to do now:
1. Update `docs/plans/M3-plan.md` with every amendment (keep its structure, mark the changed places, extend the file and test tables accordingly). Commit the plan update locally.
2. Implement **Part A only**, then run `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh`, commit locally, and stop with a short report. Do not start Part B or C until I say go.

Local commits only, never push.

## Accepted as written
- Pre-flight results (wishlist `applied_date` is correct; the `ui_smoke` message filter stays).
- Q1–Q9 defaults and the three-part split. (Q8 cites "Spec C8"; there is no C8, the references are C4 and C5.)

## Part A amendments (apply now)

**A1. Range queries and day bounds.** `AgendaService` must use `byNextActionDateRange`, `byTargetDateRange` and `InterviewRepository::inRange`, not `all()` plus in-memory filtering. Remove the contradicting notes in findings 4f and 4g. Build the interview bounds as half-open day bounds `[from 00:00:00, to+1 day 00:00:00)` using the repository's explicit text format. Add tests for an interview at 00:00:00 on `from`, one at 23:59:59 on `to`, and one at 00:00:00 on `to+1` (excluded). Add repository tests for the two new queries (inclusive date bounds, NULL dates, done topics). State once where the closed-status filter lives (repository or service).

**A2. Injectable clock.** `AgendaService` takes a `Clock` (`std::function<QDateTime()>`, default `currentDateTime`), like `ApplicationTableModel` did in M2. Specify how "past" is decided (Q1 says past interviews are dimmed). Suggested: `AgendaItem` gets `bool past`, set by the service (interviews: start before now; follow-ups and targets: date before today). Add tests with a fixed clock.

**A3. SPEC changelog.** Record in `docs/SPEC.md` §14 that `AgendaItem` loses `color` (and gains `past` if you add it), in the Part A commit.

**A4. Inconsistencies to resolve.**
- Finding 4h says the model does not observe `InterviewRepository::changed()`, but the Part A file table adds a new `InterviewRepository&` constructor parameter. Check how `ApplicationTableModel` gets interview data today; add only the missing `connect`, and change the constructor only if it really lacks the reference.
- Verify that `SettingsService` has a change signal for the first day of week. The reload list in Part C relies on `firstDayOfWeekChanged()`. Add it if missing.
- The `CMakeLists.txt` row for Part A says to "update `ApplicationTableModel` to include `InterviewRepository`"; that is a source change, not a CMake change. Fix the row.

**A5. Tests.** Rename the slot `deterministic ordering` to a valid name. Place the new test files next to the existing ones (the folders where `tst_ApplicationTableModel` and `tst_AgendaService` live), not in `tests/ui_smoke/`. Only `ui_smoke` additions go into `tests/ui_smoke/`.

## Part B amendments (apply to the plan now, implement after my go)

**B1. Header menu routing.** `+ New → Application` must open a new application on every page: switch to Jobs and call `JobsPage::newApplication()`. `+ New → Interview` always calls a `CalendarPage::newInterview(QDate)` (selected day if the page was visited, otherwise today). Only the `QKeySequence::New` shortcut uses the current page's `triggerNew()`. In the current design "Application" calls the current page's `triggerNew()`, which would open an interview dialog on the Calendar page and do nothing on Home or Study. Add a test: on the Calendar page the header "Application" entry opens exactly one `ApplicationDialog`.

**B2. Injection style.** `CalendarPage` receives explicit references (as `JobsPage` does), not `AppContext&`. Decide the final constructor now so Part C does not change it again.

**B3. Dialog behavior.** The application combo in `InterviewDialog` lists non-closed applications by default; in edit mode the interview's current application is always present. State in the Interviews tab hint that interview changes are saved immediately and are not part of the Overview "Discard changes?" logic.

**B4. End-to-end acceptance test.** Add a `ui_smoke` slot: Jobs row context menu → "Add interview…" → set a date → save → the Calendar page shows the marker on that day. This is the literal M3 acceptance criterion; the current tests insert through the repository.

## Part C amendments (apply to the plan now, implement after my go)

**C1. `MonthView` is the grid only.** Prev, next, Today and the month title are real widgets in `CalendarPage` (`QToolButton`s with icons, tooltips, tab order), not painted rectangles. AGENTS.md §7 requires keyboard access and tooltips.

**C2. Selection rule.** Any selection that lands outside the displayed month (click on a dimmed day, arrow keys at a month edge, PageUp/PageDown) makes the displayed month follow and emits `monthChanged`. Replace `outOfMonthDaysDoNotChangeMonth` with tests for this rule.

**C3. Marker range.** Load markers and items for the whole visible grid (first to last of the 42 cells), exposed as `MonthView::visibleRange()`, not for the calendar month only. Add a test with a marker on a dimmed trailing day.

**C4. Marker API.** The markers must carry the total count so "+N" can be painted, e.g. `struct DayMarkers { QList<Domain::AgendaKind> kinds; /* first 3 */ int total; }`. Pass kinds, not `QColor`.

**C5. Layering.** `AgendaListModel` exposes `KindRole` only; remove `ColorRole` and any `Tokens` use from `models/`. Add `ui/theme/AgendaKindStyle` (color, label, icon per kind) in the style of `StatusStyle`, used by `MonthView`, `AgendaItemDelegate` and the layer chips. Replace the `colorRoleMapsByKind` test with a test of `AgendaKindStyle`. Add to the Part C file table: `AgendaKindStyle`, `AgendaItemDelegate`, the new SVG icons and the `resources.qrc` change.

**C6. Missing scope.** Double-click on a day (`dayActivated`) opens a new-interview dialog prefilled with that date and asks for the application through the combo (SPEC C5). "+ Interview" and Ctrl+N on the Calendar page also use the selected day, not today. Add tests for all three.

**C7. Layout.** Use a `QSplitter` (month on the left, agenda on the right) with persisted state instead of a fixed ~300 px panel (AGENTS.md §7, SPEC §3.7). Check it at the 1100×700 minimum window size.

**C8. Layer chips.** Style them like the `ChipBar` chips (shared QSS) with a small dot in the kind color. Keep them non-exclusive.

**C9. Injectable today.** `MonthView` and `CalendarPage` take a `std::function<QDate()>` for "today" (default `QDate::currentDate`). Tests for Today, Home and the today highlight use a fixed date. Replace `monthNeedingSixRows` (the grid always has six rows) with grid start and end tests: October 2023 starts on 2023-09-25 (Monday-first) and 2023-10-01 (Sunday-first); also February 2028 and a month that begins on the first weekday.

**C10. Report.** In `docs/reports/M3.md` include screenshots of the Calendar page in light and dark (`QWidget::grab()`, saved under `docs/reports/img/`) and a "Not verified by agent, user to check" list (appearance at 150% scaling, dot contrast in the dark theme).
