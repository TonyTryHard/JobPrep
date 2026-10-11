# M3 Part B — InterviewDialog, editable Interviews tab, Jobs & header wiring

Covers Part B of Milestone M3 (plan `docs/plans/M3-plan.md`, amendments
`docs/prompts/M3-amendments.md`), delivered in two local commits:
`4486b2b` (Part A confirmations + `InterviewDialog` + its tests) and the B2 commit that
follows (editable Interviews tab, Jobs menu, header routing, remaining tests).

## Part A confirmations

1. **Boundary tests.** `tst_AgendaService::dayBoundaryInterviewCheck` already covers an
   interview at `00:00:00` on `from`, at `23:59:59` on `to` (both included) and at
   `00:00:00` on `to+1` (excluded). No test added. Note: Part A kept
   `InterviewRepository::inRange` inclusive and calls it with `23:59:59` rather than the
   amendment's half-open text bound; behaviour is identical and covered by this test.
2. **Fixed-clock tests.** Interviews were covered by `pastInterviewFlagged` (before and
   after "now"). Follow-ups and study targets were incomplete, so `pastFollowUpFlagged`
   gained a future row and a new `pastStudyTargetFlagged` covers target before, on and
   after "today" with a fixed clock.
3. **First-day-of-week signal.** `SettingsService` already has
   `firstDayOfWeekChanged(int)` alongside `firstDayOfWeek()` / `setFirstDayOfWeek(int)`,
   so Part C can rely on it. No change; there is no `SettingsService` test suite to extend.

## Decisions

- **`TrackRepository` is now a required `TrackRepository&`.** It was an optional raw
  pointer used only to look up the study-target subtitle (`track.name`). The null branch
  is gone; the constructor, `AppContext` and `tst_AgendaService` were updated.
- **Meaning of `past`.** It is a neutral fact only. `Structs.h` now carries a one-line
  comment: UI dims only past interviews; overdue follow-ups and study targets still need
  attention and must not look "done". Part C will honour this.

## What changed

### B1 (`4486b2b`)
- `AgendaService` takes a required `Data::TrackRepository&`; `AppContext` passes
  `*m_tracks`; `tst_AgendaService` updated.
- `Domain::AgendaItem::past` documented in `src/domain/Structs.h`.
- New `src/ui/dialogs/InterviewDialog.{h,cpp}`: create/edit one interview (application
  combo, date + time, duration, type, outcome, place, interviewer, notes), inline
  validation (application required, duration >= 1), dirty tracking with discard
  confirmation on Cancel/close, and a Delete button with confirmation in Edit mode.
  Application combo lists non-closed applications; the edited interview's application is
  always offered even when closed.
- `ui_smoke`: `testInterviewDialogCreateAndEdit`, `testInterviewDialogValidation`,
  `testInterviewDialogDeleteWithConfirmation`.
- Interview type/outcome labels centralized as `StatusStyle::label(InterviewType)` /
  `label(InterviewOutcome)`, so the dialog and tables share one label source.

### B2 (this commit)
- `ApplicationDialog` Interviews tab is now editable: hint label, selectable table with
  the interview id in `Qt::UserRole`, `+ Interview` / `Edit...` / `Delete...` buttons and
  double-click-to-edit. Changes save immediately and are explicitly outside the Overview
  "Discard changes?" logic.
- `JobsPage`: the row context menu "Add interview..." is enabled
  (`JobsAddInterviewAction`) and opens `InterviewDialog` with the row's application
  preselected.
- `MainWindow`: `+ New -> Application` switches to Jobs and calls
  `JobsPage::newApplication()`; `+ New -> Interview` switches to Calendar and calls
  `CalendarPage::newInterview(selectedDay)`; Ctrl+N still runs the current page's
  `triggerNew()`.
- `CalendarPage`: final constructor (explicit repository/service refs, Amendment B2),
  `newInterview(QDate)`, `selectedDay()`, `triggerNew()` and `hasItemsOn(QDate)` (a seam
  over `AgendaService` that Part C reuses for the month grid).
- `ui_smoke`: `testApplicationDialogInterviewsTabEditable`,
  `testHeaderNewApplicationOnCalendarPage`, `testJobsContextMenuAddInterviewEndToEnd`
  (added before `testZeroWarnings`).

## Amendment checklist

- **B1 — header menu routing.** Done: Application switches to Jobs and opens one
  `ApplicationDialog` from any page; Interview always routes to
  `CalendarPage::newInterview(QDate)`; Ctrl+N uses `triggerNew()`.
  `testHeaderNewApplicationOnCalendarPage` asserts exactly one dialog from the Calendar page.
- **B2 — injection style.** Done: `CalendarPage` takes explicit references (no
  `AppContext&`), so Part C keeps the constructor.
- **B3 — dialog behaviour.** Done: combo lists non-closed applications and always
  includes the edited interview's application; the Interviews tab hint states interview
  changes save immediately and are not part of the Overview "Discard changes?" prompt.
- **B4 — end-to-end test.** Done: `testJobsContextMenuAddInterviewEndToEnd` drives Jobs
  row context menu -> "Add interview..." -> save with a date -> asserts the Calendar page
  (`hasItemsOn`) reflects the marker.

## How to run

```bash
cd /home/tony/projects/JobPrep
CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
```

## How verified

Environment: Linux 26.04, GCC 15.2.0, Ninja, `dev-linux` preset, `CMAKE_BUILD_TYPE=Debug`,
`JOBPREP_WERROR=ON`, Qt 6.10.2.

- `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` — configure OK, build OK with zero
  warnings, `100% tests passed, 0 tests failed out of 14`. `ui_smoke` runs 25 slots.
- Live display (no GUI automation tools available): launched the real `jobprep` binary
  with a fresh `XDG_DATA_HOME` on native Wayland and on `QT_QPA_PLATFORM=xcb`; both stayed
  up for the timeout with an empty stderr log and created
  `$XDG_DATA_HOME/JobPrep/JobPrep/jobprep.sqlite`.
- Drove the required click-through on the live display with `ui_smoke` (same code paths):
  `QT_QPA_PLATFORM=xcb ./tests/ui_smoke/ui_smoke` — 25/25 pass, including add application,
  context-menu add interview, the editable Interviews tab, and edit/delete with
  confirmation. On native Wayland all 25 slots pass functionally; only `testZeroWarnings`
  trips on Qt's own `QWaylandTextInputv3` compositor messages (not our code).

## Deviations and assumptions

- Part A left `InterviewRepository::inRange` inclusive and the service passes `23:59:59`
  instead of the amendment's half-open text bound. Behaviour is covered; left as accepted
  in Part A.
- `CalendarPage` still shows only an empty state in Part B; the month grid, agenda panel
  and layer chips arrive in Part C. `selectedDay()` returns today until Part C adds
  selection. `hasItemsOn()` is the Part B end-to-end seam over `AgendaService`.
- `m_topics`, `m_settings` and `m_theme` are stored in `CalendarPage` but unused until
  Part C; the constructor is frozen per B2.
- The native-Wayland `QWaylandTextInputv3` messages are Qt platform-plugin noise. The
  zero-warning gate (correctly) runs offscreen; the filter was not widened to hide
  compositor messages.

## Not verified by agent, user to check

- Native-Wayland manual click-through of the app (no automation tool; only the test
  binary and a clean launch were exercised).
- Visual appearance of the new `InterviewDialog` and the editable Interviews tab in light
  and dark themes.
- Whether `+ New -> Interview` should default `selectedDay()` to today until Part C; Part
  C will revisit when day selection exists.

## Next step

Part C: `MonthView`, `AgendaKindStyle`, `AgendaItemDelegate`, `AgendaListModel` and the
full `CalendarPage` (splitter, real nav widgets, layer chips, agenda panel), plus the
Calendar unit/smoke tests and light/dark screenshots.
