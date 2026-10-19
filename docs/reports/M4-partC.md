# M4 Part C Report — Study Page Completion

**Project:** JobPrep — Qt 6.10.2, C++20, Ninja (`dev-linux` preset), `JOBPREP_WERROR=ON`  
**Platform:** Ubuntu 26.04 x86_64, GCC 15.2.0 (g++ 15.2.0), Linux 6.17.0-14-generic  
**Branch:** `main` (local commits only, never pushed)  
**Part C Commit Hash:** `e6a69b1 feat(study): implement Part C sessions, tracks, list view, and calendar jump`

---

## 1. Executive Summary

Milestone M4 Part C completes all remaining requirements of the Study preparation page (§3.2, §5, §7, §12) and executes all Part C amendments (**B7, B8, B9, C1–C6**) from `docs/prompts/M4-amendments.md` (v2). 

This milestone delivers:
- `SessionDialog` for logging study session minutes, date, notes, and topic links, wired to the Topic detail panel and main window header.
- `TrackDialog` for creating, editing, and deleting study tracks with 7 preset swatches, uniqueness validation, and cascade deletion of topics and checklists while preserving logged study sessions.
- Flat sortable List View table with custom `StatusBadgeDelegate` (Topic mode) and `ProgressBarDelegate`, view toggle `[Board | List]` with persistent settings.
- Direct routing for `+ New -> Topic` and `+ New -> Study session` header entries.
- Disabled header search on the Study page with informative tooltip pointing to page-local search.
- Bidirectional calendar integration: Study Target date updates and track renames reflect in Calendar month view and agenda; activating a Study Target jumps to the Study page, clears any active filters hiding the topic, selects it, and opens the detail panel.
- 16 regenerated report screenshots (8 pairs for Light and Dark modes).
- Full suite of 18 test targets passing with zero warnings and zero platform errors.

---

## 2. C0 Carry-overs Delivered

Before implementing Part C, the C0 carry-overs from Part B were resolved in their own commits:

- **Commit `adaa6a9`**:
  - Removed `ToolTipRole` from `TopicListModel` to resolve the data/model layering violation.
  - Implemented `TopicCardDelegate::helpEvent` using `StatusStyle::label(priority)`.
  - Removed `openCardTooltip` from `TopicBoardColumn` to avoid event filter interception.
  - Added reproducible screenshot runners: `scripts/screenshots.sh` (Linux) and `scripts/screenshots.ps1` (Windows).
  - Added unit test slots `testStudyPriorityTooltip` and `testStudyDragFeedbackIndicatorAndHighlight` to `tst_UiSmoke.cpp`.
  - Filtered offscreen QPA platform noise (`"does not support raise"`) in `isPlatformPluginNoise()` in `tst_UiSmoke.cpp`.
- **Commit `9680181`**:
  - Created `docs/reports/M4-partB-errata.md` documenting Part B commit hash `1ebf4ee`, scope of Part C carry-overs, and exact amendment naming conventions.

---

## 3. Amendment Checklist (B7, B8, B9, C1–C6)

### **B7. Header routing, direct calls only**
- **Implementation:** `MainWindow::newTopicFromHeader()` switches to the Study page and invokes `StudyPage::newTopic()`, which focuses the Backlog quick-add input (or opens `TrackDialog` when 0 tracks exist). `MainWindow::newSessionFromHeader()` opens `SessionDialog` directly from any page, preselecting the currently active study topic if on the Study page with a selection.
- **Files touched:** [MainWindow.h](file:///home/tony/projects/JobPrep/src/ui/MainWindow.h), [MainWindow.cpp](file:///home/tony/projects/JobPrep/src/ui/MainWindow.cpp), [StudyPage.h](file:///home/tony/projects/JobPrep/src/ui/pages/StudyPage.h), [StudyPage.cpp](file:///home/tony/projects/JobPrep/src/ui/pages/StudyPage.cpp).
- **Tests:** `testHeaderNewSessionAndTopicRouting()`, `testNewEntryPointsOpenOneDialog()`, `testStudyPageApi()`.

### **B8. Header search on the Study page**
- **Implementation:** In `MainWindow::updateHeader()`, header search edit is disabled when the active page is Study (`pageIndex == m_studyIndex`), setting placeholder text and tooltip to `"Use the search box on this page, Ctrl+F"`. When switching to Jobs, it is re-enabled.
- **Files touched:** [MainWindow.cpp](file:///home/tony/projects/JobPrep/src/ui/MainWindow.cpp).
- **Tests:** `testHeaderSearchDisabledOnStudyPage()`.

### **B9. Calendar study target jump**
- **Implementation:** `CalendarPage` emits `studyTopicActivated(int topicId)` when a Study Target item is double-clicked in `MonthView` or activated in `agendaList`. Connected in `MainWindow` to `MainWindow::openTopic(int topicId)`, which switches to the Study page, invokes `StudyPage::selectTopic(topicId)` (clearing any search text and track filter if they hid the topic), selects the topic, and opens `TopicDetailPanel`. If the topic was deleted, a non-fatal status message is displayed without crashing.
- **Files touched:** [CalendarPage.h](file:///home/tony/projects/JobPrep/src/ui/pages/CalendarPage.h), [CalendarPage.cpp](file:///home/tony/projects/JobPrep/src/ui/pages/CalendarPage.cpp), [MainWindow.h](file:///home/tony/projects/JobPrep/src/ui/MainWindow.h), [MainWindow.cpp](file:///home/tony/projects/JobPrep/src/ui/MainWindow.cpp), [StudyPage.cpp](file:///home/tony/projects/JobPrep/src/ui/pages/StudyPage.cpp).
- **Tests:** `testCalendarJumpToStudyTopic()`.

### **C1. SessionDialog**
- **Implementation:** Created modal `SessionDialog` with fields for Date, Duration (minutes spinbox, 1–1440 min), Topic selection combo (populated from `TopicRepository`), and Notes text edit. Includes validation (duration > 0 min) and saves via `SessionRepository`. Enabled `m_logTimeBtn` in `TopicDetailPanel` (`StudyDetailLogTimeBtn`), emitting `logTimeRequested(int topicId)` which opens `SessionDialog` preselected to that topic.
- **Files touched:** [SessionDialog.h](file:///home/tony/projects/JobPrep/src/ui/dialogs/SessionDialog.h), [SessionDialog.cpp](file:///home/tony/projects/JobPrep/src/ui/dialogs/SessionDialog.cpp), [TopicDetailPanel.h](file:///home/tony/projects/JobPrep/src/ui/widgets/TopicDetailPanel.h), [TopicDetailPanel.cpp](file:///home/tony/projects/JobPrep/src/ui/widgets/TopicDetailPanel.cpp), [StudyPage.cpp](file:///home/tony/projects/JobPrep/src/ui/pages/StudyPage.cpp).
- **Tests:** `testStudySessionDialogLogTime()`.

### **C2. TrackDialog**
- **Implementation:** Created modal `TrackDialog` with track list, `+ New Track` button, track name editor, 7 preset color swatches (Blue `#3b82f6`, Emerald `#10b981`, Purple `#8b5cf6`, Amber `#f59e0b`, Rose `#e11d48`, Indigo `#6366f1`, Teal `#0d9488`), and Delete Track button. Validation enforces non-empty names and unique track names. Cascade deletion prompts confirmation warning that all topics and checklists under the track will be deleted while logged study time is preserved (`study_sessions.topic_id` set to NULL via foreign key). Deleting the currently filtered track automatically resets the track filter chip to All (`kAllTracks` = -1).
- **Files touched:** [TrackDialog.h](file:///home/tony/projects/JobPrep/src/ui/dialogs/TrackDialog.h), [TrackDialog.cpp](file:///home/tony/projects/JobPrep/src/ui/dialogs/TrackDialog.cpp), [StudyPage.h](file:///home/tony/projects/JobPrep/src/ui/pages/StudyPage.h), [StudyPage.cpp](file:///home/tony/projects/JobPrep/src/ui/pages/StudyPage.cpp).
- **Tests:** `testStudyTrackDialogCrudAndCascade()`, `testStudyPageApi()`.

### **C3. Study Target and Track updates reflected on Calendar**
- **Implementation:** `CalendarPage` constructor updated to accept `TrackRepository& tracks` and observe its `changed()` signal alongside `topics.changed()`. Target date changes dynamically update `MonthView` target dots and agenda items; renaming a track updates the agenda subtitle (`track.name`); deleting a track cascades topic deletion and clears target dots.
- **Files touched:** [CalendarPage.h](file:///home/tony/projects/JobPrep/src/ui/pages/CalendarPage.h), [CalendarPage.cpp](file:///home/tony/projects/JobPrep/src/ui/pages/CalendarPage.cpp), [MainWindow.cpp](file:///home/tony/projects/JobPrep/src/ui/MainWindow.cpp).
- **Tests:** `testCalendarTopicDateAndTrackRename()`.

### **C4. List view [Board | List] toggle and delegates**
- **Implementation:** Added `[Board | List]` toggle buttons to `StudyPage` toolbar with state persisted in `study/viewMode` ("board" or "list"). Created `ProgressBarDelegate` to paint checklist completion progress. Extended `StatusBadgeDelegate` with `Mode::Topic` to render topic status badges. Created flat sortable `QTableView` backed by `TopicTableProxy` with sorting on Track, Title, Status, Progress, and Target Date. Integrated selection restorer, detail panel opening on double-click or Enter, and delete on Delete key.
- **Files touched:** [ProgressBarDelegate.h](file:///home/tony/projects/JobPrep/src/ui/delegates/ProgressBarDelegate.h), [ProgressBarDelegate.cpp](file:///home/tony/projects/JobPrep/src/ui/delegates/ProgressBarDelegate.cpp), [StatusBadgeDelegate.h](file:///home/tony/projects/JobPrep/src/ui/delegates/StatusBadgeDelegate.h), [StatusBadgeDelegate.cpp](file:///home/tony/projects/JobPrep/src/ui/delegates/StatusBadgeDelegate.cpp), [StudyPage.h](file:///home/tony/projects/JobPrep/src/ui/pages/StudyPage.h), [StudyPage.cpp](file:///home/tony/projects/JobPrep/src/ui/pages/StudyPage.cpp).
- **Tests:** `testStudyListViewToggleSortAndDelegates()`.

### **C5. Screenshot regeneration**
- **Implementation:** Extended `testStudyScreenshots()` in `tst_UiSmoke.cpp` to capture 8 pairs of images (Light and Dark) using `scripts/screenshots.sh`. Verified all files exist and have non-zero sizes and exact target dimensions.
- **Files touched:** [tst_UiSmoke.cpp](file:///home/tony/projects/JobPrep/tests/ui_smoke/tst_UiSmoke.cpp).
- **Verification:** Ran `./scripts/screenshots.sh` and inspected outputs with `file docs/reports/img/study-*.png`.

### **C6. Spec changelog §14 (v1.6)**
- **Implementation:** Documented flat sortable list table, 7 preset swatches, track deletion cascade rule, header search disabled on Study, header `+ New` routing, and calendar jump.
- **Files touched:** [docs/SPEC.md](file:///home/tony/projects/JobPrep/docs/SPEC.md#L393-L405).

---

## 4. Acceptance Criteria Verification (SPEC §12 M4)

| Criterion | Spec Requirement | Verdict | Verification Evidence |
|---|---|---|---|
| **Board Drag & Drop** | Dragging a card changes status and persists | **PASS** | `tst_UiSmoke::testStudyBoardRealDnDDropEvent` tests drag enter/move/drop, proxy status change, and DB persistence. |
| **Progress Bars** | Progress bars follow checklist ticks | **PASS** | `tst_UiSmoke::testStudyChecklistTickSurvivesReset` and `testStudyListViewToggleSortAndDelegates` verify checklist updates update progress in both board and list views. |
| **Session Logging** | A logged session changes weekly minutes | **PASS** | `tst_UiSmoke::testStudySessionDialogLogTime` logs 45 min, verifies `SessionRepository::totalMinutesInRange(today, today)` increases to 45. |
| **Track Management** | CRUD tracks, colors, and cascade | **PASS** | `tst_UiSmoke::testStudyTrackDialogCrudAndCascade` tests adding "Rust", editing, deleting with confirmation, cascade topic deletion, and session retention. |
| **List View Toggle** | Switch between Board and List view, sorting, persistence | **PASS** | `tst_UiSmoke::testStudyListViewToggleSortAndDelegates` tests toggle, `study/viewMode` persistence, column sorting, and delegates. |
| **Calendar Jump** | Activating Study Target in Calendar jumps to Study topic | **PASS** | `tst_UiSmoke::testCalendarJumpToStudyTopic` tests double-clicking agenda item, clearing active search/track filter, and opening detail panel. |
| **Header Integration** | Header routing for Topic and Session; header search disabled on Study | **PASS** | `tst_UiSmoke::testHeaderNewSessionAndTopicRouting`, `testHeaderSearchDisabledOnStudyPage`. |

---

## 5. Verification Evidence

### Build and Test Output (`scripts/check.sh`)
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
 3/18 Test  #3: tst_TopicRepository ..............   Passed    0.20 sec
      Start  4: tst_SessionRepository
 4/18 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  5: tst_ApplicationRepository
 5/18 Test  #5: tst_ApplicationRepository ........   Passed    0.13 sec
      Start  6: tst_InterviewRepository
 6/18 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start  7: tst_ReminderLogRepository
 7/18 Test  #7: tst_ReminderLogRepository ........   Passed    0.03 sec
      Start  8: tst_SeedService
 8/18 Test  #8: tst_SeedService ..................   Passed    0.84 sec
      Start  9: tst_TransactionBehavior
 9/18 Test  #9: tst_TransactionBehavior ..........   Passed    0.04 sec
      Start 10: ui_smoke
10/18 Test #10: ui_smoke .........................   Passed   50.29 sec
      Start 11: tst_MonthView
11/18 Test #11: tst_MonthView ....................   Passed    0.11 sec
      Start 12: tst_AgendaListModel
12/18 Test #12: tst_AgendaListModel ..............   Passed    0.05 sec
      Start 13: tst_ApplicationTableModel
13/18 Test #13: tst_ApplicationTableModel ........   Passed    0.07 sec
      Start 14: tst_ApplicationFilterProxy
14/18 Test #14: tst_ApplicationFilterProxy .......   Passed    0.03 sec
      Start 15: tst_TopicListModel
15/18 Test #15: tst_TopicListModel ...............   Passed    0.21 sec
      Start 16: tst_TopicBoardProxy
16/18 Test #16: tst_TopicBoardProxy ..............   Passed    0.12 sec
      Start 17: tst_AgendaService
17/18 Test #17: tst_AgendaService ................   Passed    0.10 sec
      Start 18: tst_ExportService
18/18 Test #18: tst_ExportService ................   Passed    0.02 sec

100% tests passed, 0 tests failed out of 18

Total Test time (real) =  52.39 sec
==> Quality check PASSED with zero warnings and all tests green!
```

### UI Smoke Test Output (`ui_smoke`)
```
Totals: 60 passed, 0 failed, 2 skipped, 0 blacklisted, 34287ms
********* Finished testing of tst_UiSmoke *********
```
*(Note: 2 skipped tests are `testCalendarScreenshots` and `testStudyScreenshots`, which skip when `JOBPREP_REPORT_IMG_DIR` is not set during normal test runs, and execute during `scripts/screenshots.sh`.)*

### Screenshot Catalog (8 Pairs / 16 Images)

| State / View | File (Light) | Size (Light) | File (Dark) | Size (Dark) | Dimensions | Description |
|---|---|---|---|---|---|---|
| **Board (cards)** | `docs/reports/img/study-board-light.png` | 65.8 KB | `docs/reports/img/study-board-dark.png` | 66.5 KB | 1200 x 720 | Kanban board with 4 columns and sample topics |
| **Empty Board** | `docs/reports/img/study-board-empty-light.png` | 24.2 KB | `docs/reports/img/study-board-empty-dark.png` | 24.3 KB | 1200 x 720 | Empty board state with "+ Topic" call-to-action |
| **Detail Panel** | `docs/reports/img/study-board-detail-light.png` | 81.2 KB | `docs/reports/img/study-board-detail-dark.png` | 82.0 KB | 1200 x 720 | Topic detail panel open with editors, checklist, and "Log time" |
| **DnD Drag Feedback** | `docs/reports/img/study-board-drag-light.png` | 90.1 KB | `docs/reports/img/study-board-drag-dark.png` | 91.8 KB | 1200 x 720 | Live card drag showing accent drop indicator and column highlight |
| **List View** | `docs/reports/img/study-list-light.png` | 152.7 KB | `docs/reports/img/study-list-dark.png` | 153.6 KB | 1200 x 720 | Flat sortable table with status badges and progress bars |
| **Tracks Dialog** | `docs/reports/img/study-tracks-dialog-light.png` | 15.2 KB | `docs/reports/img/study-tracks-dialog-dark.png` | 15.4 KB | 520 x 440 | Manage Tracks dialog with swatch buttons and track list |
| **Session Dialog** | `docs/reports/img/study-session-dialog-light.png` | 15.1 KB | `docs/reports/img/study-session-dialog-dark.png` | 15.1 KB | 460 x 400 | Log Study Session dialog with duration, date, and topic combo |
| **Empty Tracks** | `docs/reports/img/study-empty-tracks-light.png` | 24.2 KB | `docs/reports/img/study-empty-tracks-dark.png` | 24.3 KB | 1200 x 720 | Empty tracks view prompting the user to create their first track |

---

## 6. Deviations and Assumptions

All decisions were aligned with `docs/prompts/M4-amendments.md` v2 and recorded in `docs/SPEC.md` §14 (v1.6):
1. **Flat sortable table for list view (Amendment C4):** Rather than a grouped tree list, the list view is implemented as a flat sortable `QTableView` with `StatusBadgeDelegate` and `ProgressBarDelegate`, matching the UX and architecture of `JobsPage`.
2. **Preset swatches for tracks (Amendment C2):** 7 accessible, theme-harmonized color swatches are used; free-form RGB/HEX input was avoided to preserve visual consistency.
3. **Session preservation on track deletion (Amendment C2):** Track deletion cascades to topic deletion, but preserves study sessions with `topic_id = NULL` to ensure historical study minutes are never lost.
4. **Header search disabled on Study page (Amendment B8):** The global header search is disabled on the Study page with an instructive tooltip pointing users to the page-local search box (Ctrl+F).
5. **Autosave in detail panel:** Replaces modal discard confirmations; pending edits are flushed automatically on selection change, focus loss, page hide, or window close.

---

## 7. Suggested Next Step

Milestone **M4 (Study page)** is fully complete, tested, and verified.
The recommended next step is to proceed to **Milestone M5 — Dashboard & Reminders** (SPEC §3.1, §3.5, §12 M5):
- Implement `StatsService` (metrics: streak, active processes, interview counts, weekly goal progress).
- Implement Dashboard widgets: `StatCard`, `ProgressRing`, `FunnelWidget`, Today & Upcoming lists.
- Implement `ReminderService` with `INotifier`, `TrayNotifier`, and automatic `InAppNotifier` fallback.
