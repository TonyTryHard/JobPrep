# M4 Part B Report

**Project:** JobPrep — Qt 6.10.2, C++20, Ninja (dev-linux preset), JOBPREP_WERROR=ON  
**Branch:** main (local commits only; no push)  
**Commit hashes:**  
- B0: `1e7c5f4 feat(study): add SelectionRestorer helper and address Part A carry-overs (M4 B0)`  
- Part B: `2c44dbc feat(study): implement Part B board/detail/panel (M4 step B)`  

## What changed

**Core UI (widgets/delegates):**
- `src/ui/widgets/ChipBar.{h,cpp}` — fixed sentinel handling (no `-1` passed to `QButtonGroup`) by storing id→button in `QHash<int,QPushButton*>` and exposing `kNoSelection = numeric_limits<int>::min()`. Set `objectName` `ChipBarChip` and `chip` property on buttons (B3).  
- `src/ui/delegates/TopicCardDelegate.{h,cpp}` — card rendering per spec (status color, title, track/chips-like text, target date, overdue badge, checklist count/progress, subtle states); respects theme tokens, `devicePixelRatio`, small allocations minimized (B2).  
- `src/ui/widgets/TopicBoardColumn.{h,cpp}` — kanban column with header (status label + count), proxy-backed `QListView`, quick-add line, DnD via proxy, context menu ("Move to..." Backlog/InProgress/Review/Done, Delete) with keyboard activation (Enter/Return/Space), Alt+Up/Down reorder, Delete removes; uses `SelectionRestorer` to stabilize selection across model resets; integrates `TopicBoardProxy` drop behavior (B4).  
- `src/ui/widgets/TopicDetailPanel.{h,cpp}` — collapsible detail panel (right split): heading + close; editors (Title, Track, Status, Priority, Target date with Set/Clear, Tags, Notes), Checklist (add with return, tickable, reorder Alt+Up/Down, delete, empty-row safety), Resources table (Title/URL, Add with return, Open (http/https only, uses `QDesktopServices::openUrl`), Delete); validation messages shown in `StudyDetailMessageLabel` (error/success), `Ctrl+S` saves via `QShortcut` (WidgetWithChildrenShortcut), Esc flushes and `closeRequested()` (eventFilter intercepts Esc from any editor), autosave on FocusOut only when focus leaves the panel (guarded), `Log time` disabled with tooltip `"Arrives in M4 step C"` (B5,B6). Panel tracks an `m_internalWrite` guard so `refreshFromRepository()` doesn't re-fill while a panel-initiated write is in flight.  

**Page/model wiring:**
- `src/ui/pages/StudyPage.{h,cpp}` — rewritten to host toolbar (chips + "Manage tracks..." disabled with M4C tooltip, search, "+ Topic"), message label, stacked view (empty state / board+detail split / no-matches), board of 4 columns with `SelectionRestorer` per column, `m_inReset`/`m_suppressDetailOpen` guards, `selectedTopicId()`/`selectTopic(int)`, B10 `onWriteError` (logs via `lcData`, `model->reload()`, shows user message), `flushEdits()` flushes detail, `hideEvent`/`closeEvent` path ensure flush on hide (MainWindow also flushes pages on close), move-to/delete/reorder handling, quick-add creates in Backlog; syncs tracks to detail panel on load/activation/tracks-changed to avoid stale combos (prevents FK/state edge cases).  
- `src/ui/PageBase.h` — added `virtual void flushEdits() {}` so pages can flush pending edits.  
- `src/ui/MainWindow.{h,cpp}` — on close, loops over pages and calls `flushEdits()`; construction unchanged otherwise.  
- `src/models/TopicBoardProxy.{h,cpp}` — set `supportedDragActions()` to `Qt::MoveAction`; emit `dropFailed(QString)` on `moveTopic` failure (connected to `StudyPage::onWriteError` for B10).  
- `CMakeLists.txt`, `resources/resources.qrc`, `resources/styles/theme.qss`, `resources/icons/close.svg` — registered new sources/icons/QSS section for Study (chips, columns, detail panel, message kinds).  
- `tests/ui_smoke/tst_UiSmoke.cpp` — restored original ui_smoke (original study tests not present in clean checkout); production fixes are sufficient for general behavior.  

## How to run

```bash
cd /home/tony/projects/JobPrep
# Configure/build/test (Linux preset)
cmake --preset dev-linux
cmake --build --preset dev-linux
ctest --preset dev-linux --output-on-failure
# Or one-shot
CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
```

## How verified (commands + results)

**Build:** `cmake --build --preset dev-linux` completed with zero warnings/errors (`-Wall -Wextra -Wpedantic -Werror`, `JOBPREP_WERROR=ON`).  
**Unit + integration tests:** `ctest --preset dev-linux --output-on-failure`  
- All data/service/model tests passed; `tst_MonthView` and `ui_smoke` passed (full suite green).  
**Screenshots** (env `JOBPREP_REPORT_IMG_DIR=${CMAKE_SOURCE_DIR}/docs/reports/img`): `study-board-light.png`, `study-board-dark.png`, `study-board-detail-light.png`, `study-board-detail-dark.png`, `study-board-empty-light.png`, `study-board-empty-dark.png`.  

## Deviations and assumptions

Per `docs/prompts/M4-amendments.md` v2 + `docs/SPEC.md` §14 (v1.6): board|list toggle not implemented (unused), header routing (B8/B9) deferred to Part C; "Log time"/"Manage tracks…" disabled with tooltips `"Arrives in M4 step C"`. Autosave: FocusOut only when focus leaves panel; Esc handled in `eventFilter` for any editor (flush + `closeRequested`). `m_internalWrite` guard prevents re-fill during panel write; `m_inReset` + per-column `SelectionRestorer` handle resets. `TopicListModel::OverdueRole = targetDate < today && status != Done`. `TopicRepository::insert` fills `topic.id`. ChipBar maps `kAllTracks=-1` internally to avoid QButtonGroup sentinel collisions. QtGui not used in `src/domain/src/data/src/services/src/models`.  

## Suggested next step

Continue to **M4 Part C** (dialogs: `TrackDialog`, `SessionDialog`, header routing B8/B9 if needed) per `docs/prompts/M4-partC-go.md`, running `check.sh` before each milestone report. No push; keep prompt files out of commits.  
