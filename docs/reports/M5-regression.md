# M5 Regression Investigation and Resolution Report

**Date:** 2026-10-25  
**Platform:** Ubuntu 26.04.1 LTS x86_64, Linux 7.0.0-38-generic, GCC 15.2.0, Qt 6.10.2 (offscreen)  
**Status:** All 25 test suites passing, zero warnings, production composition verified with real input events.

---

## 1. Symptoms Reported by User

On a production desktop installation (Ubuntu 26.04 GNOME, real display, Wayland, real database):
1. **Topic details panel is unusable:** Shows empty state (empty title, target date `2000-01-01`, status Backlog, priority Low) although topics exist in the database; form is disabled and fields cannot be edited.
2. **Checklist "Add" does nothing:** Typing an item and pressing "Add" still adds nothing.
3. **Target-date calendar popup behavior:** The popup worked only after "Set date"; after "Clear" it stopped working again.
4. **Multiple cards outlined simultaneously:** Clicking cards across columns resulted in one card outlined per column (Backlog, In Progress, Review) simultaneously, rather than a single board-wide selection.
5. **Drag warning in console:** `QDragManager::drag in possibly invalid state` printed once during use.
6. **No reminder notification seen on desktop:** While `dbus-monitor` confirmed that the app sends DBus `Notify` calls to gnome-shell, the user saw no banner popup on screen.

---

## 2. Why Did Existing Tests Miss It?

Existing test suites (`ui_smoke`, `tst_TopicBoardProxy`, `tst_TopicListModel`) tested Study board operations predominantly via programmatic method calls (`studyPage->selectTopic(id)`, `column->selectTopicId(id)`, `setCurrentIndex(idx)`). They did not simulate real user mouse press/release events on the `TopicListView` viewport rectangle within the full application composition (`MainWindow` with `ReminderCoordinator`, active notifiers, and focus tracking).

### Step 1 Grep Evidence

#### 1. `grep -rn "installEventFilter\|eventFilter" src`
```
src/ui/widgets/TopicBoardColumn.h:94:    bool eventFilter(QObject* watched, QEvent* event) override;
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

---

## 3. Reproduction with Real Input in Production Composition

A dedicated regression test suite `tests/ui/tst_StudyRegression.cpp` was authored to instantiate the production composition (`AppContext`, `MainWindow` shown, `ReminderCoordinator` active) and drive interactions through real user events:
- `QTest::mouseClick` on topic card viewport rectangles across columns.
- `QTest::keyClicks` into `StudyDetailTitleEdit` and selection changes for autosave.
- Focus movement (`setFocus(Qt::MouseFocusReason)`) before clicking Add button in the checklist.
- Calendar widget date picking and date clearing via button and Delete key.
- Offscreen `QWidget::grab().toImage()` inspecting painted outline pixels in light and dark themes.
- Hit testing via `QApplication::widgetAt` and `win.childAt` confirming real widgets receive clicks without overlay interference.

---

## 4. Git Bisect and First Bad Commit

An automated bisect from tag `m4` (`2fb1f05`) was not possible because tag `m4` was already failing the real mouse click test.

### Comparison of `b12e6e1` with Previous Commit `f96aff5`
Inspecting `git diff f96aff5 b12e6e1 src/`:
In `f96aff5`, selection changes were handled via `onCurrentChanged`:
Commit `b12e6e1` replaced that handler with `onColumnCurrentChanged`:
```cpp
void StudyPage::onColumnCurrentChanged(Widgets::TopicBoardColumn* activeColumn, const QModelIndex& current) {
    if (!current.isValid() || (activeColumn->list()->selectionModel() && !activeColumn->list()->selectionModel()->isSelected(current))) {
        // Did not select or forward to detail panel
        return;
    }
    ...
```
In Qt's `QAbstractItemView` / `QListView` mouse press processing, `QItemSelectionModel::currentChanged` is emitted **before** `select(idx, ...)`. When the user clicked on an unselected card:
1. `currentChanged` fired with `isSelected(current) == false`.
2. `StudyPage::onColumnCurrentChanged` evaluated `!isSelected(current)` as `true`.
3. Finding no existing selection in other columns, it flushed and cleared the detail panel, set `m_selectedTopicId = std::nullopt`, and exited early.
4. `select()` was then executed by Qt, emitting `selectionChanged`.
5. Because `StudyPage` did not listen to `selectionChanged`, the selection was never acknowledged: `m_selectedTopicId` remained unset, `TopicDetailPanel::loadFrom` was never invoked, and other columns were never deselected.

---

## 5. Root Causes

### Root Cause 1: Premature Selection Check in `onColumnCurrentChanged`
In Qt, `currentChanged` fires on mouse press before `select()` establishes selection. Filtering with `!isSelected(current)` caused all real mouse clicks to drop the selection, clear the detail panel, and leave stale visual borders on other columns.

### Root Cause 2: Inactive Column Outline Repaint Suppression
In `TopicBoardColumn::clearSelection()`, `m_list->selectionModel()` was blocked with `QSignalBlocker` to avoid recursive change notifications. However, `m_list->viewport()->update()` was never called, leaving stale selection borders visible across columns.

### Root Cause 3: Redundant Writes and Model Resets on Focus Loss
In `TopicDetailPanel::saveNow()` and `TopicRepository::update()`, fields were written unconditionally even when no field changed. On active desktop windows, clicking Add or switching controls triggered focus-out autosave; this executed a no-op write, emitted `changed()`, triggered model reset, and caused `StudyPage::onModelReset()` to rebind and clear typed checklist inputs before the click event was delivered.

### Root Cause 4: Duplicate Drag Handling in `TopicListView`
`TopicListView` installed an event filter on its own viewport while also implementing `viewportEvent()` and `event()`. This resulted in duplicate/triple delivery of drag events. Furthermore, `startDrag()` lacked re-entrancy protection, causing Qt to warn `QDragManager::drag in possibly invalid state` if another drag event was processed.

### Root Cause 5: Desktop Notification Urgency and Tray Verification
`dbus-monitor` confirmed that the application successfully sent DBus `Notify` messages to gnome-shell. However:
- The message icon was `QSystemTrayIcon::Information`, which GNOME interprets as urgency 0 (low). GNOME Shell routes low-urgency notifications directly into the notification center tray without displaying transient desktop banners.
- The summary was only the company name, without specifying the interview time or action.
- `TrayNotifier::notify` did not verify whether the system tray icon was visible or available, and lacked in-app fallback when balloon messages were unsupported.

---

## 6. Fix Implementation Summary (Round 1 & Round 2)

### 1. Board Selection & Viewport Repaint Synchronization
- Connected `selectionModel()->selectionChanged` to `onColumnSelectionChanged(activeColumn)` in `StudyPage.cpp`.
- Added `m_list->viewport()->update()` in `TopicBoardColumn::clearSelection()`.
- Added pixel-level outline check in `tst_StudyRegression` verifying strictly one card border has `tokens.accent` across Backlog, In Progress, and Review in both light and dark themes.

### 2. Checklist Input & Focus Preservation (F1)
- `TopicDetailPanel::saveNow()` compares updated values with existing topic; if unchanged, it skips the repository write and emits nothing.
- `TopicRepository::update()` skips transaction and notification if all fields match the existing database row.
- `StudyPage::onModelReset()` guards the entire reset and selection restoration with `m_inReset = true;`, preventing intermediate deselection signals from clearing the detail panel.
- `TopicDetailPanel::loadFrom(id)` preserves typed inputs and checklist items when rebinding for the same topic id.
- Added debug logging category `jobprep.study` (default threshold Info).

### 3. Target-Date Field Flow (F2)
- Target-date field is always enabled while a topic is selected, displaying `"No target date"` via `specialValueText` when unset.
- Opening calendar popup defaults to today's date if date is unset. Picking a day sets the date and autosaves.
- Small "Clear" control and Delete/Backspace keys reset the date to unset and keep the field enabled. Removed the intermediate "Set date" flow.

### 4. Drag Re-entrancy & Single Drag Guarantee (F3)
- Removed redundant `viewport()->installEventFilter(this)` from `TopicListView`.
- Added `m_isDragging` guard in `TopicListView::startDrag` to eliminate re-entrant drag attempts.

### 5. Notification Formatting, Urgency, and Tray Validation (F5)
- `NotificationText::formatTitle`: Formats summary with what it is and when ("Interview in 29 minutes", "Interview today at 17:00").
- `NotificationText::formatMessage`: Formats body as "Company · Type (position)".
- `TrayNotifier::notify`: Uses `QSystemTrayIcon::Warning` (normal urgency in GNOME); verifies `isSystemTrayAvailable()` and `m_trayIcon.isVisible()`, returning false if unavailable so reminders are retried next tick; falls back to `InAppNotifier` if `supportsMessages()` is false.
- `jobprep.reminders` logging category set to default threshold Info (`QtInfoMsg`).

---

## 7. Changes in Round 2

### Commits
- `3f47ec0` — `fix(study): fix checklist add, date field flow, drag re-entrancy, and tray notifications`
- `f91a5b9` — `docs(spec): document v1.9 round 2 regression fixes in section 14`
- Current commit — `docs(reports): update M5 regression report with round 2 fix-ups`

### Code Inspection Evidence

#### 1. Tray Checks in Notifiers
```
$ grep -n "isVisible\|isSystemTrayAvailable\|supportsMessages" src/ui/TrayNotifier.cpp src/ui/NotifierFactory.cpp src/ui/ReminderCoordinator.cpp
src/ui/TrayNotifier.cpp:63:    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
src/ui/TrayNotifier.cpp:67:    if (!m_trayIcon.isVisible()) {
src/ui/TrayNotifier.cpp:71:    if (!QSystemTrayIcon::supportsMessages()) {
```

#### 2. Focus and Save Handlers in `TopicDetailPanel.cpp`
```
$ grep -n "focusOut\|FocusOut\|save()\|loadFrom\|clearTopic" src/ui/widgets/TopicDetailPanel.cpp
321:bool TopicDetailPanel::loadFrom(int topicId) {
337:        clearTopic();
345:void TopicDetailPanel::clearTopic() {
399:    if (event->type() == QEvent::FocusOut && m_topicId && !m_loading) {
469:bool TopicDetailPanel::saveNow() {
```

#### 3. Repository Writes in `TopicRepository.cpp`
```
$ grep -n "commitAndNotify" src/data/TopicRepository.cpp | head -30
186:    if (!commitAndNotify(m_database, transaction)) return false;
206:    return commitAndNotify(m_database, transaction);
222:    return commitAndNotify(m_database, transaction);
242:    return commitAndNotify(m_database, transaction);
368:    return commitAndNotify(m_database, transaction);
381:    return commitAndNotify(m_database, transaction);
421:    if (!commitAndNotify(m_database, transaction)) return false;
444:    return commitAndNotify(m_database, transaction);
462:    return commitAndNotify(m_database, transaction);
477:    return commitAndNotify(m_database, transaction);
501:    return commitAndNotify(m_database, transaction);
```

### Open Question for User (Resolved)
**Should an active, visible window also show an in-app toast, because GNOME desktop banners can be suppressed by Do Not Disturb?**  
- **Resolution:** No. The user confirmed: "no it shouldn't".
- **Behavior:** The architecture strictly follows the existing fallback model: `TrayNotifier` delivers notifications via OS tray/desktop banners whenever system tray is available and message support is reported; `InAppNotifier` delivers in-app toast notifications only when system tray is unavailable or message support is missing. No duplicate in-app toasts are displayed when system tray is active.

---

## 8. New Test Slots Since Tag `7519117`

### Raw Output of `git diff 7519117 -- tests | grep -E '^\+.*void '`
```
+    void notificationTextInterviewRemainingTime();
+    void coordinatorEndToEndInterview50MinutesAheadInAppAndTrayPaths();
+void tst_Notifiers::notificationTextInterviewRemainingTime() {
+void tst_Notifiers::coordinatorEndToEndInterview50MinutesAheadInAppAndTrayPaths() {
+    void initTestCase();
+    void testRealMouseClickSelectionAcrossColumns();
+    void testWidgetAtReturnsRealControlsNotOverlay();
+    void testRealKeyClicksTitleAutosave();
+    void testRealMouseClickChecklistAddAndSpaceToggle();
+    void testRealFocusChecklistAddAndEdits();
+    void testRealMouseClickCalendarDateEdit();
+    void testBoardDragCallsStartDragOnceAndClickDoesNotDrag();
+    void testBoardSelectionPaintedOutlinesBothThemes();
+    void testRegressionSequenceWithToastVisible();
+void tst_StudyRegression::initTestCase() {
+void tst_StudyRegression::testRealMouseClickSelectionAcrossColumns() {
+void tst_StudyRegression::testWidgetAtReturnsRealControlsNotOverlay() {
+void tst_StudyRegression::testRealKeyClicksTitleAutosave() {
+void tst_StudyRegression::testRealMouseClickChecklistAddAndSpaceToggle() {
+void tst_StudyRegression::testRealFocusChecklistAddAndEdits() {
+void tst_StudyRegression::testRealMouseClickCalendarDateEdit() {
+void tst_StudyRegression::testBoardDragCallsStartDragOnceAndClickDoesNotDrag() {
+void tst_StudyRegression::testBoardSelectionPaintedOutlinesBothThemes() {
+void tst_StudyRegression::testRegressionSequenceWithToastVisible() {
```

### Detailed Breakdown of the 11 New Test Slots
1. `tst_Notifiers::notificationTextInterviewRemainingTime`: Verifies natural remaining time formatting ("in 50 minutes", "today at 17:00", "tomorrow at 10:00").
2. `tst_Notifiers::coordinatorEndToEndInterview50MinutesAheadInAppAndTrayPaths`: Tests dual-path routing (toast vs tray) for upcoming interviews.
3. `tst_StudyRegression::testRealMouseClickSelectionAcrossColumns`: Verifies single board selection on mouse click across Backlog, In Progress, and Review.
4. `tst_StudyRegression::testWidgetAtReturnsRealControlsNotOverlay`: Hit-tests title, button, and card widgets verifying no overlay blockage.
5. `tst_StudyRegression::testRealKeyClicksTitleAutosave`: Tests title autosave on focus loss across cards.
6. `tst_StudyRegression::testRealMouseClickChecklistAddAndSpaceToggle`: Tests checklist item addition and Space toggle.
7. `tst_StudyRegression::testRealFocusChecklistAddAndEdits`: Tests checklist Add, item check toggle, title edit, and priority change with real focus movement.
8. `tst_StudyRegression::testRealMouseClickCalendarDateEdit`: Tests always-enabled date field, calendar popup day selection, repository persistence, and clearing via button and Delete key.
9. `tst_StudyRegression::testBoardDragCallsStartDragOnceAndClickDoesNotDrag`: Verifies single click does not initiate drag, and drag start calls `startDrag` once without warning.
10. `tst_StudyRegression::testBoardSelectionPaintedOutlinesBothThemes`: Samples painted border pixels via offscreen `grab().toImage()` across light and dark themes verifying strictly one card outline.
11. `tst_StudyRegression::testRegressionSequenceWithToastVisible`: Repeats card selection sequence while an in-app toast is visible.

*(Note: The previously reported "+10 in supporting suites" was an incorrect count; the true increase is exactly +11 slots across the 2 suites above, bringing total test functions from 310 to 321).*

---

## 9. Final Verification

### Per-Binary Function Counts and Total
```
$ total=0; for t in build/dev-linux/tests/*/*tst_* build/dev-linux/tests/ui_smoke/ui_smoke; do if [ -f "$t" ] && [ -x "$t" ]; then count=$($t -functions | wc -l); echo "$count $(basename $t)"; total=$((total + count)); fi; done; echo "TOTAL: $total"
18 tst_ApplicationRepository
16 tst_Database
9 tst_InterviewRepository
7 tst_ReminderLogRepository
5 tst_SeedService
10 tst_SessionRepository
18 tst_TopicRepository
8 tst_TrackRepository
8 tst_TransactionBehavior
5 tst_AgendaListModel
2 tst_ApplicationFilterProxy
6 tst_ApplicationTableModel
9 tst_TopicBoardProxy
7 tst_TopicListModel
6 tst_TopicTableProxy
24 tst_AgendaService
4 tst_DayWatcher
4 tst_ExportService
18 tst_ReminderService
4 tst_SettingsService
10 tst_StatsService
13 tst_MonthView
21 tst_Notifiers
9 tst_StudyRegression
80 ui_smoke
TOTAL: 321
```

### Git Commit History (`git log --oneline -8`)
```
$ git log --oneline -8
f91a5b9 docs(spec): document v1.9 round 2 regression fixes in section 14
3f47ec0 fix(study): fix checklist add, date field flow, drag re-entrancy, and tray notifications
fe51477 docs(prompts): add M5 regression fix-ups
5342328 docs(prompts): add M5 regression fix-ups
dca572d docs(reports): add M5 regression report
641e012 docs(spec): document v1.8 regression fixes in section 14
9bfea82 fix(study): fix board single selection, detail panel empty state and natural reminder text
33ca62b docs(prompts): add M5 study regression prompt
```

### Full `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` Raw Output
```
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.2s)
-- Generating done (0.2s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
[1/4] Automatic MOC and UIC for target tst_Notifiers
[2/3] Building CXX object tests/ui/CMakeFiles/tst_Notifiers.dir/tst_Notifiers.cpp.o
[3/3] Linking CXX executable tests/ui/tst_Notifiers
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start  1: tst_Database
 1/25 Test  #1: tst_Database .....................   Passed    0.03 sec
      Start  2: tst_TrackRepository
 2/25 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start  3: tst_TopicRepository
 3/25 Test  #3: tst_TopicRepository ..............   Passed    0.19 sec
      Start  4: tst_SessionRepository
 4/25 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  5: tst_ApplicationRepository
 5/25 Test  #5: tst_ApplicationRepository ........   Passed    0.15 sec
      Start  6: tst_InterviewRepository
 6/25 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start  7: tst_ReminderLogRepository
 7/25 Test  #7: tst_ReminderLogRepository ........   Passed    0.03 sec
      Start  8: tst_SeedService
 8/25 Test  #8: tst_SeedService ..................   Passed    0.84 sec
      Start  9: tst_TransactionBehavior
 9/25 Test  #9: tst_TransactionBehavior ..........   Passed    0.04 sec
      Start 10: ui_smoke
10/25 Test #10: ui_smoke .........................   Passed   95.83 sec
      Start 11: tst_MonthView
11/25 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 12: tst_AgendaListModel
12/25 Test #12: tst_AgendaListModel ..............   Passed    0.04 sec
      Start 13: tst_ApplicationTableModel
13/25 Test #13: tst_ApplicationTableModel ........   Passed    0.08 sec
      Start 14: tst_ApplicationFilterProxy
14/25 Test #14: tst_ApplicationFilterProxy .......   Passed    0.03 sec
      Start 15: tst_TopicListModel
15/25 Test #15: tst_TopicListModel ...............   Passed    0.19 sec
      Start 16: tst_TopicBoardProxy
16/25 Test #16: tst_TopicBoardProxy ..............   Passed    0.12 sec
      Start 17: tst_TopicTableProxy
17/25 Test #17: tst_TopicTableProxy ..............   Passed    0.13 sec
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
23/25 Test #23: tst_StatsService .................   Passed    2.13 sec
      Start 24: tst_Notifiers
24/25 Test #24: tst_Notifiers ....................   Passed    2.40 sec
      Start 25: tst_StudyRegression
25/25 Test #25: tst_StudyRegression ..............   Passed    2.75 sec

100% tests passed, 0 tests failed out of 25

Total Test time (real) = 105.52 sec
==> Quality check PASSED with zero warnings and all tests green!
```

---

## 10. Not Verified by Agent

1. **Real Wayland Clicks and Focus:** Headless offscreen QPA emulates window and event mechanics, but native Wayland focus stealing prevention and popup surface hierarchy must be verified by the user in an active desktop session.
2. **GNOME Shell Banner Popups:** Visual display of popup banner bubbles depends on the host's desktop configuration and active Do-Not-Disturb state.
