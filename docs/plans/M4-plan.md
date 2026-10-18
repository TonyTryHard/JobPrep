# M4 — Study page

Status: planned; awaiting approval to build. One local commit, `feat(study): topic board (M4)`, only after `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` passes. Local commits only, never push.

Environment: Linux, Qt 6.10.2 (system, `/usr/lib/x86_64-linux-gnu/cmake/Qt6`), Ninja / Debug preset `dev-linux`, `JOBPREP_WERROR=ON`, `QT_PREFIX_PATH` unset. Build with `CMAKE_BUILD_PARALLEL_LEVEL=2`.

Confirmed decisions:
- List view: flat `QTableView` with a Track column (grouping key), not a tree.
- Track recolor: preset swatch palette (design tokens + seeded colors), no free-form picker.
- Cross-page wiring: replace the M3 placeholders now — calendar study-target rows jump to the topic; header "+ New" gains "Topic" and enables "Study session".
- Header search stays Jobs-only (no M3 behavior change); Study gets its own local search box.
- Quick-add with track filter "All" uses the first track by position; disabled with a hint when no track exists.

---

## 0. Pre-flight findings (verified against the real code)

| # | Finding |
|---|---|
| 1 | `StudyPage` (src/ui/pages/StudyPage.cpp:9) is a dep-less stub; `MainWindow` builds it as `new Pages::StudyPage(m_pageStack)` (MainWindow.cpp:121). Constructor and wiring must change. |
| 2 | `TopicRepository` has full CRUD incl. `updatePositions`, `subtasks`, `add/update/setSubtaskDone/remove/reorderSubtasks`, `subtaskCount/doneSubtaskCount`, `byTargetDateRange`. No batched subtask-count query → N+1 risk. `position` is one global column, so per-column ordering needs a purpose-built atomic move. |
| 3 | `TrackRepository` (`all/byId/idByName/insert/update/remove/count`) and `SessionRepository` (`insert`, `totalMinutesInRange`) are complete. |
| 4 | `StatusStyle` already has `TopicStatus` and `Priority` colors+labels (StatusStyle.cpp:60–110) — reuse, no new theme class. |
| 5 | `StatusBadgeDelegate` is hard-wired to `ApplicationStatus` + `ApplicationTableModel::StatusRole`; it needs to support `TopicStatus` for the list view. |
| 6 | `ChipBar` is exclusive single-select (good for the track filter) but has no `clear()`; tracks are dynamic, so it needs `clearChips()`. |
| 7 | `SettingsService` has `weeklyGoalMinutes` but no study splitter/view settings → add them. |
| 8 | M3 left placeholders M4 supersedes: `MainWindow` "+ New → Study session" disabled "Arrives in M4" (MainWindow.cpp:191–195); Calendar study-target rows show "opens in M4" (CalendarPage.cpp:423–434); `ui_smoke` asserts the former (tst_UiSmoke.cpp:968–969). |
| 9 | `resources/icons/study.svg` exists; `plus/search/chevron-*` exist. A few monochrome icons may be added. |
| 10 | DnD auto-removal guard: `QAbstractItemView::startDrag` calls `clearOrRemove()` after a `MoveAction`; the source model's default `removeRows()` returns false, so we keep `TopicListModel` free of `removeRows` and mutate the repo ourselves. |

---

## Design decisions

| Q | Decision | Reason |
|---|---|---|
| D1 | `TopicListModel` is a `QAbstractTableModel` (Title, Track, Status, Priority, Target, Progress, Tags). `QListView` (board) shows column 0; `QTableView` (list) shows all columns. | One model, two views; no extra tree model; stays within the named classes. |
| D2 | Progress (SPEC §4) computed in the model cache: has subtasks → done/total; else backlog 0, in_progress 50, review 80, done 100. Exposed as `ProgressRole`. | Single source; M5 StatsService can reuse the rule. |
| D3 | `TopicBoardProxy` (one per column) filters by `TopicStatus` + optional track id and owns DnD with MIME `application/x-jobprep-topic-id`. Drop calls a new atomic `TopicRepository::moveTopic(id, status, orderedIds)`. | One transaction, one `changed()`; testable at model level. |
| D4 | Board columns are a `TopicBoardColumn` widget (header + `QListView` + "New topic…" quick-add). | Keeps column glue out of the page. |
| D5 | List view = flat sortable table, Track column sorted first by default. | Grouping by track, simplest implementation. |
| D6 | Detail panel = `TopicDetailPanel` (`QSplitter` right pane, collapsible), autosave on focus loss + Ctrl+S; checkable drag-reorderable checklist writing immediately; resources as a title/url table serialized to the `resources` JSON. | SPEC S3, S7. |
| D7 | `SessionDialog` (date default today, minutes, note, optional topic incl. "(No topic)"); `TrackDialog` manages tracks (add/rename/recolor via swatches/delete + confirm + cascade). | SPEC S1, S4. |
| D8 | New `SettingsService` keys: `study/splitterState`, `study/viewMode` ("board"/"list"), `study/trackFilter` (-1 = All). | Persist page state. |
| D9 | `StatusBadgeDelegate` gains a `Mode {Application, Topic}` + status-role ctor param (default Application); new `TopicCardDelegate` (track stripe, progress bar, due chip red when overdue, priority marker) and `ProgressBarDelegate`. | Reuse one badge painter; Jobs unchanged. |

---

## Files touched / added

| File | Status | Purpose |
|---|---|---|
| `src/models/TopicListModel.{h,cpp}` | new | Flat table model; roles incl. `StatusRole`, `PriorityRole`, `ProgressRole`, `TrackIdRole`, `TrackColorRole`, `TrackNameRole`, `TargetDateRole`, `Subtask*Role`, `SortRole`. |
| `src/models/TopicBoardProxy.{h,cpp}` | new | Per-column filter (status + track); MIME `application/x-jobprep-topic-id`; `mimeData`/`canDropMimeData`/`dropMimeData`/`flags`. |
| `src/ui/delegates/TopicCardDelegate.{h,cpp}` | new | Card painting for the board. |
| `src/ui/delegates/ProgressBarDelegate.{h,cpp}` | new | Progress column in the list view. |
| `src/ui/widgets/TopicBoardColumn.{h,cpp}` | new | Column header + list view + quick-add line. |
| `src/ui/widgets/TopicDetailPanel.{h,cpp}` | new | Inline editor with checklist and resources. |
| `src/ui/dialogs/SessionDialog.{h,cpp}` | new | Log a study session. |
| `src/ui/dialogs/TrackDialog.{h,cpp}` | new | Manage tracks. |
| `src/ui/pages/StudyPage.{h,cpp}` | rewrite | Toolbar (chips, toggle, search), board/list stack, splitter, detail panel. |
| `src/ui/MainWindow.{h,cpp}` | modify | Study ctor wiring; `+ New → Topic/Session`; calendar study-target navigation. |
| `src/ui/pages/CalendarPage.{h,cpp}` | modify | `studyTopicActivated(int)`; open instead of "opens in M4". |
| `src/models/AgendaListModel.cpp` | modify | Drop the "later milestone" study-target tooltip. |
| `src/ui/delegates/StatusBadgeDelegate.{h,cpp}` | modify | Mode + status-role parameters. |
| `src/ui/widgets/ChipBar.{h,cpp}` | modify | `clearChips()`. |
| `src/data/TopicRepository.{h,cpp}` | modify | `moveTopic`, batched `subtaskProgress()`. |
| `src/services/SettingsService.{h,cpp}` | modify | Study splitter/view/track-filter keys. |
| `resources/resources.qrc` + new SVGs | modify | Register icons actually used. |
| `resources/styles/theme.qss` | modify | Card, column, detail-panel, chip styles. |
| `CMakeLists.txt` | modify | Add new sources to `jobprep_core`. |
| `tests/models/CMakeLists.txt` | modify | Add `tst_TopicListModel`, `tst_TopicBoardProxy`. |
| `tests/data/tst_TopicRepository.cpp` | modify | `moveTopic`, `subtaskProgress`. |
| `tests/ui_smoke/tst_UiSmoke.cpp` | modify | Study slots; update the "+ New" assertion. |
| `docs/SPEC.md` §14 | modify | Changelog entries for M4 deviations. |
| `docs/reports/M4.md` | new | Report + light/dark screenshots. |

---

## Implementation order

1. **Model & data** — `TopicRepository::moveTopic` + `subtaskProgress`; `TopicListModel`; `TopicBoardProxy`; model/repo tests.
2. **Board** — `TopicBoardColumn`, `TopicCardDelegate`, DnD, quick-add, drag-reorder.
3. **Detail panel** — inline autosave, checklist (tick/reorder/add/edit/delete), resources JSON.
4. **Session logging** — `SessionDialog` + header/menu entry points.
5. **Track management** — `TrackDialog` + chips refresh.
6. **List view + polish** — view toggle, sortable table, `StatusBadgeDelegate` mode, empty states, qss, `ui_smoke`, screenshots, report.

---

## Test plan

| Test | Slots |
|---|---|
| `tst_TopicListModel` | row/column shape; roles; progress with and without subtasks; track color/name; reload on repository `changed()`. |
| `tst_TopicBoardProxy` | status filter; track filter; `mimeData` round-trip of `application/x-jobprep-topic-id`; `dropMimeData` changes status and persists; same-column reorder via `moveTopic`. |
| `tst_TopicRepository` (added) | `moveTopic` sets status + positions in one transaction; `subtaskProgress` counts total/done per topic. |
| `ui_smoke` (added/updated) | board↔list toggle; checklist tick updates card progress; log session changes `totalMinutesInRange`; track add/rename/delete (with confirm); quick-add creates in the right status/track; delete topic confirm keeps sessions; calendar study-target row opens the topic; header `+New → Topic/Session` opens exactly one dialog (updates the old "Arrives in M4" assertion); both themes; zero warnings. |

## Verification and report

`CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` — configure + build zero warnings + all tests green. Report `docs/reports/M4.md` (AGENTS §10 format) with `docs/reports/img/study-board-{light,dark}.png` and `study-list-{light,dark}.png`; note anything not verified by the agent.
