# M3 Part A — Pre-flight fixes & AgendaService Report

Covers the Part A work of Milestone M3 (plan `docs/plans/M3-plan.md`, amendments `docs/prompts/M3-amendments.md`), delivered in commits `848d7f7` (plan update) and `84785fe` (Part A implementation).

## What changed

### Domain & SPEC
- `Domain::AgendaItem` in `src/domain/Structs.h`: removed `QColor color` and the `#include <QColor>` dependency so the domain layer stays free of `QtGui` types. Added `bool past{false}` to indicate past events for dimming in UI.
- `docs/SPEC.md`: updated the changelog with version 1.4 recording the `AgendaItem` adjustments.

### Data Layer
- `Database`: added regression test `twoLiveDatabasesAreIndependent` in `tests/data/tst_Database.cpp` to verify that destroying one `Database` does not affect other active instances using distinct connection names.
- `ApplicationRepository`: added `byNextActionDateRange(QDate from, QDate to)` returning applications with `next_action_date` in `[from, to]` inclusive, excluding closed statuses (`rejected`, `withdrawn`, `ghosted`, `accepted`), and including active and wishlist applications. Added tests in `tests/data/tst_ApplicationRepository.cpp`.
- `TopicRepository`: added `byTargetDateRange(QDate from, QDate to)` returning non-done topics with `target_date` in `[from, to]` inclusive. Added tests in `tests/data/tst_TopicRepository.cpp`.

### Settings Service
- Added getters, setters, and change signals for layer toggles in `SettingsService`:
  - `calendarLayerInterviews` / `setCalendarLayerInterviews` (default `true`)
  - `calendarLayerFollowUps` / `setCalendarLayerFollowUps` (default `true`)
  - `calendarLayerTargets` / `setCalendarLayerTargets` (default `true`)

### Agenda Service
- `AgendaService` created in `src/services/AgendaService.h` and `src/services/AgendaService.cpp`.
- Takes references to `InterviewRepository`, `ApplicationRepository`, `TopicRepository`, optional `TrackRepository*`, and an injectable `Clock` (`std::function<QDateTime()>`).
- Computes `items(QDate from, QDate to)` by querying:
  - `InterviewRepository::inRange` with day bounds `[from 00:00:00, to 23:59:59]`, excluding cancelled interviews.
  - `ApplicationRepository::byNextActionDateRange(from, to)`.
  - `TopicRepository::byTargetDateRange(from, to)`.
- Flags items as `past` based on the injected clock (interviews compare exact start time to now; follow-ups and study targets compare date to today).
- Enforces deterministic ordering: `date` asc, then `kind` (`Interview` < `FollowUp` < `StudyTarget`), then `time` (items with time sort before items without time; earlier time first), then `title` asc.
- Wired into composition root `AppContext`.

## How to run

```bash
cd /home/tony/projects/JobPrep
CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
```

## How verified

Environment: Linux 26.04, GCC 15.2.0, Ninja, `dev-linux` preset, `CMAKE_BUILD_TYPE=Debug`, `JOBPREP_WERROR=ON`, Qt 6.10.2.

Commands and results:
- `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` — configure OK, build OK with zero warnings, `100% tests passed, 0 tests failed out of 14`.
- All 14 test suites passed:
  1. `tst_Database` (including `twoLiveDatabasesAreIndependent`)
  2. `tst_TrackRepository`
  3. `tst_TopicRepository` (including `byTargetDateRangeQueries`)
  4. `tst_SessionRepository`
  5. `tst_ApplicationRepository` (including `byNextActionDateRangeQueries`)
  6. `tst_InterviewRepository`
  7. `tst_ReminderLogRepository`
  8. `tst_SeedService`
  9. `tst_TransactionBehavior`
  10. `ui_smoke`
  11. `tst_ApplicationTableModel`
  12. `tst_ApplicationFilterProxy`
  13. `tst_AgendaService` (14 unit test slots)
  14. `tst_ExportService`

## Deviations and assumptions
- In `AgendaService::items()`, `InterviewRepository::inRange` is called with `[from 00:00:00, to 23:59:59]` because `InterviewRepository::inRange` uses inclusive SQL comparison (`start_at >= ? AND start_at <= ?`), ensuring events on day `to+1` at `00:00:00` are excluded.
- The `ApplicationTableModel` constructor was verified to already accept `InterviewRepository&` and connect to `changed()`, so no modifications were needed there.

## Next step
Proceed to **Part B** as outlined in `docs/plans/M3-plan.md`:
- `InterviewDialog`
- Editable Interviews tab in `ApplicationDialog`
- Jobs context menu "Add interview…" integration
- Header `+ New → Interview` and `+ New → Application` menu routing
