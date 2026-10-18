# M4 Part A — Topic models, progress rule, repository moves and settings

Covers Part A of Milestone M4 per `docs/prompts/M4-amendments.md` (cross-checked with SPEC 1.5/1.6 and `docs/plans/M4-plan.md`).

Commit: `8a83475` (`feat(study): topic models, progress rule and repository move (M4 Part A)`)

---

## Plan versus result

| Plan / Amendment item | Result |
|---|---|
| A1: `topicProgress` helper in `domain` | Added `src/domain/TopicProgress.{h,cpp}`: pure function `int topicProgress(TopicStatus status, int doneSubtasks, int totalSubtasks)`. |
| A2: Neutral model roles | `TopicListModel` exposes only neutral types: hex `QString` for track color (never `QColor`), ints, `QDate`, `QString`. No `QColor/QIcon/QFont/QPainter`, no `ui/` or `theme` headers in `src/models/`. |
| A3: `TopicRepository::moveTopic` semantics | Implemented in `TopicRepository.{h,cpp}`: atomic single-transaction move reordering visible target topics (`orderedIds`) while preserving relative order of hidden topics across track filters. No-op writes and emits nothing. Unknown ID returns false and rolls back. |
| A4: Drop semantics in `TopicBoardProxy` | Implemented in `TopicBoardProxy.{h,cpp}`: MIME `application/x-jobprep-topic-id`, drop on card = insert before, drop with `row == -1` / empty area = append, drop on same place = no-op. `removeRows` remains unimplemented. |
| A5: Stable reload | `TopicListModel::reload()` compares cached row identity (IDs in sequence); data-only changes (subtask ticks, inline field edits) emit `dataChanged` and never `modelReset`. Structural changes (insert/remove) emit `modelReset`. |
| A6: Batched subtask progress query | `TopicRepository::subtaskProgress()` returns done/total counts in one SQL query (`GROUP BY topic_id`). Fixed query count (topics, tracks, progress) with no N+1. |
| A7: Overdue model fact with clock | `TopicListModel` accepts injectable `TodayClock` (`std::function<QDate()>`); `OverdueRole` evaluates true only if target date < today and status is not Done. Done topics are never overdue. |
| A8: Track changes reach model | `TopicListModel` connects to both `TopicRepository::changed` and `TrackRepository::changed`. Track rename and recolor trigger reload and emit `dataChanged`. |
| A9: Study settings in `SettingsService` | Added `study/splitterState`, `study/viewMode` (defaults to "board", unknown values return "board"), and `study/trackFilter` (defaults to -1, stored id returned as is). |
| Tests | Added `tests/models/tst_TopicListModel.cpp`, `tests/models/tst_TopicBoardProxy.cpp`, and extended `tests/data/tst_TopicRepository.cpp`. |

---

## What changed

- **`src/domain/TopicProgress.{h,cpp}`**:
  Pure function `JobPrep::Domain::topicProgress(TopicStatus status, int doneSubtasks, int totalSubtasks)`. If `totalSubtasks > 0`, calculates `(std::clamp(doneSubtasks, 0, totalSubtasks) * 100) / totalSubtasks`. Otherwise uses status fallback: Backlog = 0, InProgress = 50, Review = 80, Done = 100. Safe against total 0, negative counts, or done > total.
- **`src/data/TopicRepository.{h,cpp}`**:
  Added `struct TopicSubtaskProgress { int done{0}; int total{0}; }`.
  Added `subtaskProgress()` returning `QHash<int, TopicSubtaskProgress>` via `SELECT topic_id, COUNT(*), COALESCE(SUM(done), 0) FROM subtasks GROUP BY topic_id`.
  Added `moveTopic(int id, Domain::TopicStatus targetStatus, const QList<int>& orderedIds)` managing global `position` ordering atomically in a single transaction with a single `changed()` notification.
- **`src/models/TopicListModel.{h,cpp}`**:
  Flat 7-column table model (`Title`, `Track`, `Status`, `Priority`, `TargetDate`, `Progress`, `Tags`). Neutral roles only: `IdRole`, `StatusRole`, `PriorityRole`, `ProgressRole`, `TrackIdRole`, `TrackColorRole` (hex QString), `TrackNameRole`, `TargetDateRole`, `DoneSubtasksRole`, `TotalSubtasksRole`, `OverdueRole`, `NotesRole`, `ResourcesRole`, `TagsRole`, `SortRole`. Takes injectable `TodayClock`. Reload compares row identity to preserve selection/focus on data changes.
- **`src/models/TopicBoardProxy.{h,cpp}`**:
  Per-column filter proxy (`columnStatus()`, optional `trackFilter()`). Implements DnD with MIME `application/x-jobprep-topic-id`, `canDropMimeData`, `dropMimeData` (card = insert before, row -1 = append, self-drop = no-op). `removeRows()` returns false.
- **`src/services/SettingsService.{h,cpp}`**:
  Added accessors and change signals for `studySplitterState`, `studyViewMode`, and `studyTrackFilter`.
- **`docs/SPEC.md`**:
  Updated §14 changelog with version 1.6 entries for `topicProgress()`, Done topics not overdue, `moveTopic`/`subtaskProgress`, and study settings keys.

---

## Layering and QtGui verification (A2)

Grep check for prohibited types and UI/theme includes across `src/models/*`:
```bash
$ grep -En 'QColor|QIcon|QFont|QPainter|ui/|theme' src/models/*
src/models/TopicListModel.h:20:/// are handled by delegates in ui/.
```
*(The single match is a comment explaining that delegates in `ui/` handle painting).*

Grep check for `QtGui` in `src/models/*`:
```bash
$ grep -En 'QtGui' src/models/*
(no output, exit code 1)
```

---

## Done topics and overdue status analysis (A7)

- In `TopicListModel`, `OverdueRole` evaluates `topic.targetDate.has_value() && (*topic.targetDate < today) && (topic.status != Domain::TopicStatus::Done)`. Done topics are never overdue.
- Current `AgendaService` behavior: `AgendaService` queries `TopicRepository::byTargetDateRange(from, to)`. In `TopicRepository.cpp`, this SQL query explicitly includes `AND status != 'done'`, so `AgendaService` already filters out Done topics from study targets.
- M5 "Today" list (SPEC §3.1 D2): per the same rule, "topics whose target date is today/overdue" will check the model's `OverdueRole` or open topics only; Done topics must never appear as overdue. No changes to `AgendaService` were needed in M4.

---

## How to run

```bash
cd /home/tony/projects/JobPrep
CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
```

---

## How verified

Environment: Linux (Ubuntu 26.04 x86_64), GCC 15.2.0, Ninja, Qt 6.10.2 (system), `dev-linux` preset, `JOBPREP_WERROR=ON`.

`CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` output:
```
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.2s)
-- Generating done (0.1s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
ninja: no work to do.
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start  1: tst_Database
 1/18 Test  #1: tst_Database .....................   Passed    0.04 sec
      Start  2: tst_TrackRepository
 2/18 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start  3: tst_TopicRepository
 3/18 Test  #3: tst_TopicRepository ..............   Passed    0.19 sec
      Start  4: tst_SessionRepository
 4/18 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  5: tst_ApplicationRepository
 5/18 Test  #5: tst_ApplicationRepository ........   Passed    0.14 sec
      Start  6: tst_InterviewRepository
 6/18 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start  7: tst_ReminderLogRepository
 7/18 Test  #7: tst_ReminderLogRepository ........   Passed    0.03 sec
      Start  8: tst_SeedService
 8/18 Test  #8: tst_SeedService ..................   Passed    0.85 sec
      Start  9: tst_TransactionBehavior
 9/18 Test  #9: tst_TransactionBehavior ..........   Passed    0.04 sec
      Start 10: ui_smoke
10/18 Test #10: ui_smoke .........................   Passed   10.45 sec
      Start 11: tst_MonthView
11/18 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 12: tst_AgendaListModel
12/18 Test #12: tst_AgendaListModel ..............   Passed    0.16 sec
      Start 13: tst_ApplicationTableModel
13/18 Test #13: tst_ApplicationTableModel ........   Passed    0.08 sec
      Start 14: tst_ApplicationFilterProxy
14/18 Test #14: tst_ApplicationFilterProxy .......   Passed    0.02 sec
      Start 15: tst_TopicListModel
15/18 Test #15: tst_TopicListModel ...............   Passed    0.23 sec
      Start 16: tst_TopicBoardProxy
16/18 Test #16: tst_TopicBoardProxy ..............   Passed    0.14 sec
      Start 17: tst_AgendaService
17/18 Test #17: tst_AgendaService ................   Passed    0.10 sec
      Start 18: tst_ExportService
18/18 Test #18: tst_ExportService ................   Passed    0.02 sec

100% tests passed, 0 tests failed out of 18

Total Test time (real) =  12.70 sec
==> Quality check PASSED with zero warnings and all tests green!
```

### Test counts by real slot names

- **`tst_TopicRepository`** (17 test slots):
  `insertFillsIdAndTimestamps`, `resourcesRoundTripUnchanged`, `updateStoresNewValuesAndBumpsTimestamp`, `setStatusWritesStatusOnly`, `updatePositionsRewritesOrder`, `removeKeepsSessionsButClearsTopic`, `subtaskLifecycle`, `reorderSubtasksRewritesOrder`, `unknownIdsFailWithoutChangedSignal`, `queriesByTrackAndCount`, `byTargetDateRangeQueries`, `moveTopicAcrossStatus`, `moveTopicReorderInsideColumn`, `moveTopicTrackFilteredPreservesHiddenOrder`, `moveTopicAppendAtEnd`, `moveTopicUnknownIdAndNoOp`, `subtaskProgressBatchedQuery`.
- **`tst_TopicListModel`** (7 test slots):
  `topicProgressRule`, `modelStructureAndRoles`, `overdueRoleWithInjectableClock`, `reloadEmitsDataChangedForDataOnlyChanges`, `reloadEmitsModelResetForStructuralChanges`, `trackChangesUpdateModel`, `settingsKeysAndFallbacks`.
- **`tst_TopicBoardProxy`** (9 test slots):
  `statusFilterAcceptsMatchingColumnOnly`, `trackFilterFiltersByTrack`, `mimeDataRoundTrip`, `canDropMimeDataValidation`, `dropOnCardInsertsBeforeIt`, `dropOnEmptyAreaAppends`, `dropOnSamePlaceIsNoOp`, `dropAcrossStatusMovesTopic`, `removeRowsIsUnimplemented`.
- **Pre-existing test suites**:
  `ui_smoke` (34 slots), `tst_MonthView` (11 slots), `tst_AgendaService` (15 slots), `tst_ApplicationTableModel` (6 slots), `tst_ApplicationFilterProxy` (3 slots), `tst_AgendaListModel` (5 slots), `tst_ExportService` (4 slots), `tst_Database` (6 slots), `tst_TrackRepository` (7 slots), `tst_SessionRepository` (8 slots), `tst_ApplicationRepository` (12 slots), `tst_InterviewRepository` (8 slots), `tst_ReminderLogRepository` (6 slots), `tst_SeedService` (4 slots), `tst_TransactionBehavior` (4 slots).

---

## SPEC §14 entries (1.6)

1. `topicProgress()` helper lives in `domain`: pure derived-rule helper function so models and services can both calculate topic progress without coupling.
2. Done topics are never overdue (SPEC §3.2 S5): `TopicListModel::OverdueRole` evaluates to false when topic status is Done, matching `AgendaService` which already excludes Done topics from study targets.
3. `TopicRepository::moveTopic` and `subtaskProgress()` added to the data layer: `moveTopic` provides atomic status and position updates across columns and track filters with relative order preservation for hidden topics; `subtaskProgress()` provides batched subtask counts to avoid N+1 queries.
4. Settings keys `study/splitterState`, `study/viewMode` (default "board", unknown values fall back to "board"), and `study/trackFilter` (default -1) added to `SettingsService`.

---

## Not verified by agent, user to check

- Real graphical DnD interaction on the board (part of Part B).
- Behavior with user's native X11 or Wayland window manager under system theme changes (part of Part B/C).

---

## Suggested next step

Proceed to **Part B** (steps 2–3 of M4: `TopicBoardColumn`, `TopicCardDelegate`, DnD visual integration, keyboard navigation, `TopicDetailPanel`, and `StudyPage` board shell).
