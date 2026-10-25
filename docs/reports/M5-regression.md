# M5 Regression Investigation and Resolution Report

**Date:** 2026-10-25  
**Platform:** Ubuntu 26.04.1 LTS x86_64, Linux 6.14.0-35-generic, GCC 15.2.0, Qt 6.10.2 (offscreen / xcb)  
**Status:** All 25 test suites passing, zero warnings, production composition verified with real input events.

---

## 1. Symptoms Reported by User

On a production desktop installation (Ubuntu 26.04 GNOME, real display, real database) following Milestone 5 Part D:
1. **Topic details panel is unusable:** Shows empty state (empty title, target date `2000-01-01`, status Backlog, priority Low) although topics exist in the database; form is disabled and fields cannot be edited.
2. **Checklist "Add" does nothing:** Clicking "Add" or pressing Enter in the checklist input fails to create subtasks.
3. **Target-date calendar popup does not work:** The date field is disabled or unresponsive.
4. **Multiple cards outlined simultaneously:** Clicking cards across columns resulted in one card outlined per column (Backlog, In Progress, Review) simultaneously, rather than a single board-wide selection.
5. **No reminder notification was ever seen:** Neither desktop tray bubbles nor in-app toasts appeared for scheduled reminders.

---

## 2. Why Did Existing Tests Miss It?

Existing test suites (`ui_smoke`, `tst_TopicBoardProxy`, `tst_TopicListModel`) tested Study board operations predominantly via programmatic method calls (`studyPage->selectTopic(id)`, `column->selectTopicId(id)`, `setCurrentIndex(idx)`). They did not simulate real user mouse press/release events on the `TopicListView` viewport rectangle within the full application composition (`MainWindow` with `ReminderCoordinator`, active notifiers, and focus tracking).

### Step 1 Grep Evidence

#### 1. `grep -rn "installEventFilter\|eventFilter" src`
```
src/ui/widgets/TopicBoardColumn.h:94:    bool eventFilter(QObject* watched, QEvent* event) override;
src/ui/widgets/TopicListView.h:31:    bool eventFilter(QObject* watched, QEvent* event) override;
src/ui/widgets/TopicBoardColumn.cpp:57:    m_quickAdd->installEventFilter(this);
src/ui/widgets/TopicBoardColumn.cpp:97:    m_list->installEventFilter(this);
src/ui/widgets/TopicBoardColumn.cpp:98:    m_list->viewport()->installEventFilter(this);
src/ui/widgets/TopicBoardColumn.cpp:213:bool TopicBoardColumn::eventFilter(QObject* watched, QEvent* event) {
src/ui/widgets/TopicBoardColumn.cpp:227:            return QWidget::eventFilter(watched, event);
src/ui/widgets/TopicBoardColumn.cpp:231:            return QWidget::eventFilter(watched, event);
src/ui/widgets/TopicBoardColumn.cpp:270:    return QWidget::eventFilter(watched, event);
src/ui/widgets/TopicDetailPanel.h:77:    bool eventFilter(QObject* watched, QEvent* event) override;
src/ui/widgets/TopicDetailPanel.cpp:172:    m_checkList->installEventFilter(this);
src/ui/widgets/TopicDetailPanel.cpp:249:        editor->installEventFilter(this);
src/ui/widgets/TopicDetailPanel.cpp:381:bool TopicDetailPanel::eventFilter(QObject* watched, QEvent* event) {
src/ui/widgets/TopicDetailPanel.cpp:421:    return QWidget::eventFilter(watched, event);
src/ui/widgets/TopicListView.cpp:25:    viewport()->installEventFilter(this);
src/ui/widgets/TopicListView.cpp:51:bool TopicListView::eventFilter(QObject* watched, QEvent* event) {
src/ui/widgets/TopicListView.cpp:70:    return QListView::eventFilter(watched, event);
src/ui/InAppNotifier.cpp:25:    m_window.installEventFilter(this);
src/ui/InAppNotifier.cpp:157:bool InAppNotifier::eventFilter(QObject* watched, QEvent* event) {
src/ui/InAppNotifier.cpp:170:    return QObject::eventFilter(watched, event);
src/ui/InAppNotifier.h:45:    bool eventFilter(QObject* watched, QEvent* event) override;
src/ui/dialogs/TrackDialog.cpp:137:        btn->installEventFilter(this);
src/ui/dialogs/TrackDialog.cpp:150:    m_customSwatchBtn->installEventFilter(this);
src/ui/dialogs/TrackDialog.cpp:232:bool TrackDialog::eventFilter(QObject* watched, QEvent* event) {
src/ui/dialogs/TrackDialog.cpp:254:    return QDialog::eventFilter(watched, event);
src/ui/dialogs/TrackDialog.h:40:    bool eventFilter(QObject* watched, QEvent* event) override;
src/ui/pages/JobsPage.h:80:    bool eventFilter(QObject* watched, QEvent* event) override;
src/ui/pages/StudyPage.cpp:319:    m_tableView->installEventFilter(this);
src/ui/pages/StudyPage.cpp:368:bool StudyPage::eventFilter(QObject* watched, QEvent* event) {
src/ui/pages/StudyPage.cpp:384:    return PageBase::eventFilter(watched, event);
src/ui/pages/StudyPage.h:104:    bool eventFilter(QObject* watched, QEvent* event) override;
src/ui/pages/JobsPage.cpp:180:    m_view->installEventFilter(this);
src/ui/pages/JobsPage.cpp:275:bool JobsPage::eventFilter(QObject* watched, QEvent* event) {
src/ui/pages/JobsPage.cpp:288:    return PageBase::eventFilter(watched, event);
```

#### 2. `grep -rn "qApp->install\|QApplication::instance()->install" src`
Exit code 1 (no global application event filters installed).

#### 3. `grep -n "show()\|setVisible\|setContextMenu" src/ui/TrayNotifier.cpp src/ui/InAppNotifier.cpp src/ui/Toast.cpp`
```
src/ui/TrayNotifier.cpp:35:    m_trayIcon.setContextMenu(m_trayMenu);
src/ui/TrayNotifier.cpp:50:    m_trayIcon.show();
src/ui/InAppNotifier.cpp:130:        t->show();
src/ui/InAppNotifier.cpp:141:        m_overflowLabel->show();
```

### Children of `MainWindow` Created by `InAppNotifier` and `Toast`

`InAppNotifier` creates two categories of child widgets parented directly to `MainWindow`:
1. `Toast* toast`: Created on demand in `InAppNotifier::notify`. Geometry is pinned to the bottom-right corner `(x = win.width() - 320 - 16, y = win.height() - 16 - total_height)`. When no toast is active, `m_toasts` is empty and no `Toast` widget exists in `MainWindow`.
2. `QLabel* m_overflowLabel`: Created on demand when more than 3 toasts are queued. When `m_toasts.isEmpty()`, `updateLayout()` explicitly calls `m_overflowLabel->hide()`.

There was no transparent overlay widget covering the window. However, `InAppNotifier::eventFilter` intercepted `QEvent::KeyPress` with `Key_Escape` on `MainWindow`, swallowing Escape globally when toasts were queued rather than only when focus was inside the toast.

---

## 3. Reproduction with Real Input in Production Composition

A new test binary `tests/ui/tst_StudyRegression.cpp` was authored to instantiate the production composition (`AppContext`, `MainWindow` shown, `ReminderCoordinator` active) and drive interactions exclusively through simulated real user events:
- `QTest::mouseClick` on topic card viewport rectangles in Backlog, then In Progress, then Review.
- `QTest::keyClicks` into `StudyDetailTitleEdit` and selection changes for autosave.
- `QTest::keyClicks` into checklist inputs and `QTest::mouseClick` on `StudyDetailCheckAddBtn`, followed by `Key_Space` toggling.
- `QTest::mouseClick` on `StudyDetailDateClearBtn` ("Set date" / "Clear") and date manipulation.
- Hit testing via `QApplication::widgetAt` and `win.childAt` confirming real widgets receive clicks without overlay interference.

### Raw Failing Test Output on HEAD Before Fix
```
********* Start testing of tst_StudyRegression *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_StudyRegression::initTestCase()
FAIL!  : tst_StudyRegression::testRealMouseClickSelectionAcrossColumns() Compared values are not the same
   Actual   (studyPage->selectedTopicId())       : nullopt
   Expected (std::optional<int>(expectedTopicId)): std::optional(1)
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_StudyRegression.cpp(117)]
FAIL!  : tst_StudyRegression::testRealMouseClickSelectionAcrossColumns() Compared values are not the same
   Actual   (studyPage->selectedTopicId())       : nullopt
   Expected (std::optional<int>(expectedTopicId)): std::optional(2)
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_StudyRegression.cpp(117)]
FAIL!  : tst_StudyRegression::testRealMouseClickSelectionAcrossColumns() Compared values are not the same
   Actual   (studyPage->selectedTopicId())       : nullopt
   Expected (std::optional<int>(expectedTopicId)): std::optional(3)
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_StudyRegression.cpp(117)]
FAIL!  : tst_StudyRegression::testWidgetAtReturnsRealControlsNotOverlay() 'widgetAtTitle == titleEdit || titleEdit->isAncestorOf(widgetAtTitle)' returned FALSE. ()
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_StudyRegression.cpp(221)]
FAIL!  : tst_StudyRegression::testRealKeyClicksTitleAutosave() Compared values are not the same
   Actual   (studyPage->selectedTopicId()): nullopt
   Expected (std::optional<int>(idB))     : std::optional(2)
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_StudyRegression.cpp(291)]
FAIL!  : tst_StudyRegression::testRealMouseClickChecklistAddAndSpaceToggle() Compared values are not the same
   Actual   (checkList->count()): 0
   Expected (1)                 : 1
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_StudyRegression.cpp(350)]
FAIL!  : tst_StudyRegression::testRealMouseClickCalendarDateEdit() Compared values are not the same
   Actual   (dateBtn->text()): ""
   Expected (tr("Set date")) : "Set date"
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_StudyRegression.cpp(405)]
FAIL!  : tst_StudyRegression::testRegressionSequenceWithToastVisible() Compared values are not the same
   Actual   (studyPage->selectedTopicId()): nullopt
   Expected (std::optional<int>(idA))     : std::optional(1)
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_StudyRegression.cpp(496)]
PASS   : tst_StudyRegression::cleanupTestCase()
Totals: 2 passed, 6 failed, 0 skipped, 0 blacklisted, 868ms
********* Finished testing of tst_StudyRegression *********
```

---

## 4. Git Bisect and First Bad Commit

Running `git bisect` from tag `m4` (`2fb1f05`) to `HEAD` demonstrated that the bug was present at `m4`.

### `git bisect log`
```
# bad: [33ca62bc4d6268bb7afe2dcd052ea810f260020d] docs(prompts): add M5 study regression prompt
# good: [2fb1f0546897ad1dc7b826154c880f3eadcd2ca8] m4 prompts
git bisect start 'HEAD' 'm4'
# bad: [2fb1f0546897ad1dc7b826154c880f3eadcd2ca8] m4 prompts
git bisect bad 2fb1f0546897ad1dc7b826154c880f3eadcd2ca8
```

### Analysis
Examining history prior to `m4` via `git log -S "isSelected(current)" --oneline`:
```
b12e6e1 feat(study): coordinate single selection across board and add acceptance tests
```
Commit `b12e6e1b6cb963ed37f427de1b5d491d20903f9e` introduced the premature check:
```cpp
if (!current.isValid() || (activeColumn->list()->selectionModel() && !activeColumn->list()->selectionModel()->isSelected(current))) {
```
Because the M4 test suite only tested programmatic selection (`selectTopicId`), the failure on real user mouse clicks went undetected at `m4`.

---

## 5. Root Causes

### Root Cause 1: Premature Selection Check in `onColumnCurrentChanged`
In Qt's `QAbstractItemView` / `QListView` mouse press processing, `QItemSelectionModel::currentChanged` is emitted **before** `select(idx, ...)`. When the user clicked on an unselected card:
1. `currentChanged` fired with `isSelected(current) == false`.
2. `StudyPage::onColumnCurrentChanged` evaluated `!activeColumn->list()->selectionModel()->isSelected(current)` as `true`.
3. Finding no existing selection in other columns, it flushed and cleared the detail panel, set `m_selectedTopicId = std::nullopt`, and exited early.
4. `select()` was then executed by Qt, emitting `selectionChanged`.
5. Because `StudyPage` did not listen to `selectionChanged`, the selection was never acknowledged: `m_selectedTopicId` remained unset, `TopicDetailPanel::loadFrom` was never invoked, and other columns were never deselected.

### Root Cause 2: Inactive Column Outline Repaint Suppression
In `TopicBoardColumn::clearSelection()`, `m_list->selectionModel()` was blocked with `QSignalBlocker` to avoid recursive change notifications. However, `m_list->viewport()->update()` was never called. As a result, when cards were selected in different columns, the old columns retained their visual selection borders, resulting in multiple cards outlined across the board.

### Root Cause 3: Default Date and Empty State Hint in `TopicDetailPanel`
`TopicDetailPanel`'s `QDateEdit` defaulted to Qt's minimum date `2000-01-01`. Without `specialValueText`, an empty target date rendered as `2000-01-01`. Furthermore, when no topic was selected, the panel remained disabled without an informative empty-state hint.

### Root Cause 4: Global Escape Swallowing in `InAppNotifier`
`InAppNotifier::eventFilter` intercepted `Key_Escape` globally on `MainWindow` whenever any toasts were queued, preventing child controls and popups from processing Escape.

### Root Cause 5: Static Lead Display in `NotificationText`
`NotificationText::formatMessage` for interviews formatted only static `notification.leadMinutes` ("in 24 hours"). For an interview 3 hours away with lead 1440, it displayed "in 24 hours" instead of natural remaining time ("today at 17:00").

---

## 6. Fix Implementation

### 1. Board Selection & Viewport Repaint Synchronization
- In `src/ui/pages/StudyPage.h` and `src/ui/pages/StudyPage.cpp`:
  - Connected `selectionModel()->selectionChanged` to `onColumnSelectionChanged(activeColumn)`.
  - When `activeColumn` gains selection: ensures `activeColumn->list()->currentIndex()` matches the selected index, clears selection and current index across all other columns, assigns `m_selectedTopicId`, loads the topic into `m_detail`, and opens the panel.
  - When selection is cleared board-wide: calls `m_detail->clearTopic()` and resets `m_selectedTopicId`.
  - In `onColumnCurrentChanged`: if `current.isValid()` but not yet in selection model, calls `select(current, ClearAndSelect)`.
- In `src/ui/widgets/TopicBoardColumn.cpp`:
  - In `clearSelection()`, added `m_list->viewport()->update()` to immediately erase stale card outlines on inactive columns.

### 2. Topic Details Panel Empty State and Target Date
- In `src/ui/widgets/TopicDetailPanel.cpp`:
  - Configured `m_dateEdit->setSpecialValueText(tr("No target date"))` and initialized it to `minimumDate()`.
  - When `!m_hasTargetDate`, the field displays `"No target date"` and is disabled.
  - In `setFormEnabled(false)` and `clearTopic()`, updated heading to `"Select a topic"` and displayed inline info hint `"Select a topic"`.

### 3. Escape Key Dismissal Scoped to Toast Focus
- In `src/ui/InAppNotifier.cpp`:
  - Removed `QEvent::KeyPress` filter on `MainWindow`. Esc dismissal is handled solely within `Toast::keyPressEvent` when the toast has keyboard focus.

### 4. Natural Remaining Time Formatting and Reminders Logging
- In `src/ui/NotificationText.h` and `src/ui/NotificationText.cpp`:
  - Implemented `formatRemainingTime(const Services::Notification& notification, const QDateTime& now)`.
  - Computes natural remaining time relative to current time: `"in %1 minutes"`, `"today at %1"` (e.g. `"today at 17:00"`), `"tomorrow at %1"`.
- In `src/services/ReminderService.cpp`:
  - Added logging category `Q_LOGGING_CATEGORY(lcReminders, "jobprep.reminders")`.
  - Logs service start, tick due counts, `notify` delivery results, and recorded keys.

---

## 7. Commits

1. `9bfea82` — `fix(study): fix board single selection, detail panel empty state and natural reminder text`
2. `641e012` — `docs(spec): document v1.8 regression fixes in section 14`
3. Current commit — `docs(reports): add M5 regression report`

---

## 8. New Tests

### `tests/ui/tst_StudyRegression.cpp`
- `testRealMouseClickSelectionAcrossColumns`: Verifies real mouse clicks across Backlog, In Progress, Review result in single board-wide selection, single card outline, and loaded detail panel.
- `testWidgetAtReturnsRealControlsNotOverlay`: Verifies hit testing at center of title edit, add button, and card returns expected controls without overlay blockage.
- `testRealKeyClicksTitleAutosave`: Types new title into detail panel and changes selection via mouse click; verifies database persistence.
- `testRealMouseClickChecklistAddAndSpaceToggle`: Types checklist item, clicks "Add" button, verifies persistence; presses Space to toggle check state.
- `testRealMouseClickCalendarDateEdit`: Tests "Set date" and "Clear" button clicks, date setting, and verifies `"No target date"` text when cleared.
- `testRegressionSequenceWithToastVisible`: Repeats card selection sequence with active in-app toast to confirm no event blockage.

### `tests/ui/tst_Notifiers.cpp`
- `notificationTextInterviewRemainingTime`: Verifies interview 3 hours away today formats as `"today at 17:00"`, 50 minutes away as `"in 50 minutes"`, tomorrow at 10:00 as `"tomorrow at 10:00"`.
- `coordinatorEndToEndInterview50MinutesAheadInAppAndTrayPaths`: Verifies interview 50 minutes ahead produces 1 visible toast via in-app path and 1 `notify` delivery via tray path.

---

## 9. Final Verification

### Final `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` Raw Output
```
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.2s)
-- Generating done (0.2s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
ninja: no work to do.
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start  1: tst_Database
 1/25 Test  #1: tst_Database .....................   Passed    0.04 sec
      Start  2: tst_TrackRepository
 2/25 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start  3: tst_TopicRepository
 3/25 Test  #3: tst_TopicRepository ..............   Passed    0.19 sec
      Start  4: tst_SessionRepository
 4/25 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  5: tst_ApplicationRepository
 5/25 Test  #5: tst_ApplicationRepository ........   Passed    0.14 sec
      Start  6: tst_InterviewRepository
 6/25 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start  7: tst_ReminderLogRepository
 7/25 Test  #7: tst_ReminderLogRepository ........   Passed    0.03 sec
      Start  8: tst_SeedService
 8/25 Test  #8: tst_SeedService ..................   Passed    0.85 sec
      Start  9: tst_TransactionBehavior
 9/25 Test  #9: tst_TransactionBehavior ..........   Passed    0.04 sec
      Start 10: ui_smoke
10/25 Test #10: ui_smoke .........................   Passed   87.86 sec
      Start 11: tst_MonthView
11/25 Test #11: tst_MonthView ....................   Passed    0.10 sec
      Start 12: tst_AgendaListModel
12/25 Test #12: tst_AgendaListModel ..............   Passed    0.04 sec
      Start 13: tst_ApplicationTableModel
13/25 Test #13: tst_ApplicationTableModel ........   Passed    0.07 sec
      Start 14: tst_ApplicationFilterProxy
14/25 Test #14: tst_ApplicationFilterProxy .......   Passed    0.03 sec
      Start 15: tst_TopicListModel
15/25 Test #15: tst_TopicListModel ...............   Passed    0.20 sec
      Start 16: tst_TopicBoardProxy
16/25 Test #16: tst_TopicBoardProxy ..............   Passed    0.11 sec
      Start 17: tst_TopicTableProxy
17/25 Test #17: tst_TopicTableProxy ..............   Passed    0.12 sec
      Start 18: tst_AgendaService
18/25 Test #18: tst_AgendaService ................   Passed    0.15 sec
      Start 19: tst_DayWatcher
19/25 Test #19: tst_DayWatcher ...................   Passed    0.01 sec
      Start 20: tst_ExportService
20/25 Test #20: tst_ExportService ................   Passed    0.01 sec
      Start 21: tst_ReminderService
21/25 Test #21: tst_ReminderService ..............   Passed    0.12 sec
      Start 22: tst_SettingsService
22/25 Test #22: tst_SettingsService ..............   Passed    0.02 sec
      Start 23: tst_StatsService
23/25 Test #23: tst_StatsService .................   Passed    2.11 sec
      Start 24: tst_Notifiers
24/25 Test #24: tst_Notifiers ....................   Passed    2.32 sec
      Start 25: tst_StudyRegression
25/25 Test #25: tst_StudyRegression ..............   Passed    0.99 sec

100% tests passed, 0 tests failed out of 25

Total Test time (real) =  95.68 sec
==> Quality check PASSED with zero warnings and all tests green!
```

### Totals by Command
- **Test Binaries:** 25 binaries (baseline 24; +1: `tst_StudyRegression`).
- **Total Test Slots:** 328 slots (baseline 310; +18: +6 in `tst_StudyRegression`, +2 in `tst_Notifiers`, +10 in supporting suites).

### `ui_smoke` Runtime
- **Before fix (M5 Part D):** 99.77 sec
- **After fix:** 87.86 sec

---

## 10. Items Still Not Verified

1. **Real System Tray Balloon Popups:** `QSystemTrayIcon::showMessage` issues notifications over DBus (`org.freedesktop.Notifications`). If the desktop environment suppresses notifications (e.g. GNOME Do-Not-Disturb or absence of an AppIndicator shell extension), the OS drops visual popups silently while Qt reports delivery success.
2. **Wayland Window Raising:** In Wayland compositors (GNOME Wayland), programmatic window activation (`raise()`, `activateWindow()`) from notification click handlers is subject to compositor focus-stealing prevention.
3. **External Display Automation (`xvfb-run`, `xdotool`):** Verified absent on test host (`which xvfb-run xdotool` returned exit code 1); all integration tests executed through headless offscreen QPA.
