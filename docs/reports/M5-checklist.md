# Milestone 5 Checklist Display Report

- **Date:** 2026-10-26
- **Host OS & Kernel:** Linux 7.0.0-38-generic (Ubuntu 26.04 LTS x86_64)
- **Compiler:** GCC 15.2.0 (`-std=c++20 -Wall -Wextra -Wpedantic -Werror`)
- **Qt Version:** Qt 6.10.2 (system packages, GCC 15.2.0 64-bit)
- **Execution Mode:** Headless / offscreen (`QT_QPA_PLATFORM=offscreen`)

---

## 1. Facts from the User's Real Run
1. Adding checklist items works: the log shows `checklist add: topic= 11 len= 8` and `TopicRepository::changed` for each add, and the database has the new rows (`subtasks` ids 102 and 103 for topic 11). The board card shows the new count (`0/6`, later `0/8`). The model is correct.
2. The items do not appear in the panel's checklist list: nothing is visible after Add. Selecting another card and coming back does not help (the list is repopulated, still nothing visible).
3. Switching the theme fixes it. Dark -> light shows all items; switching back to light -> dark keeps them visible; a quick dark -> light -> dark also fixes it. Items added later stay visible too. After a fresh start the problem is back.
4. The text input "New checklist item" and the Add button are visible and work.
5. The items exist in the widget but are not painted visibly at startup (stale colors, palette or style polish), and any theme change re-applies the style and repairs it.

---

## 2. Task 1: Find the Cause (Evidence & Root Cause)

### 1. Startup Order
```
$ grep -n "ThemeManager\|setMode\|apply\|SettingsService\|MainWindow" src/main.cpp
9:#include "services/SettingsService.h"
10:#include "ui/MainWindow.h"
58:    JobPrep::Ui::MainWindow mainWindow(ctx);
```
- In `main.cpp`, `AppContext ctx` is constructed before `MainWindow`.
- Inside `AppContext`, `m_themeManager` is initialized with the saved theme mode from `m_settings` and calls `applyTheme()`, setting the palette and application stylesheet on `qApp` and emitting `themeChanged()`.
- Because `MainWindow` and `TopicDetailPanel` do not exist yet when `ThemeManager` constructs, no UI widgets receive `themeChanged()` at startup.

### 2. Colors in the Panel
```
$ grep -n "setForeground\|setBackground\|ForegroundRole\|BackgroundRole\|tokens()\|palette\|QColor\|setStyleSheet\|themeChanged" src/ui/widgets/TopicDetailPanel.cpp
```
Command returned exit code 1 (no occurrences).
- `TopicDetailPanel` does not hard-code or bake foreground/background colors into items or widgets at construction time.

### 3. Stylesheet Rules for Checklist
```
$ grep -n -C 5 "StudyDetailCheckList" resources/styles/theme.qss
448-#StudyDetailCloseBtn:hover {
449-    background-color: @surfaceAlt;
450-    color: @text;
451-}
452-
453:#StudyDetailCheckList {
454-    background-color: @bg;
455-    border: 1px solid @border;
456-    border-radius: 8px;
457-}
458-
459:#StudyDetailCheckList::item {
460-    border-radius: 6px;
461-    padding: 3px 6px;
462-}
463-
464:#StudyDetailCheckList::item:hover {
465-    background-color: @surfaceAlt;
466-}
467-
468-#StudyDetailResources {
```
- In `theme.qss`, `#StudyDetailCheckList` and `#StudyDetailCheckList::item` defined `background-color`, `border`, and `padding`, but lacked explicit `color: @text;`.

### 4. Root Cause Named: (d) Something Else
In `src/ui/widgets/TopicDetailPanel.cpp`, `fillChecklist()` and `addChecklistItem()` wrapped item creation with `const QSignalBlocker blocker(m_checkList->model());`:
```cpp
const QSignalBlocker blocker(m_checkList->model());
m_checkList->clear();
for (const auto& subtask : subtasks) {
    auto* item = new QListWidgetItem(subtask.text, m_checkList);
    ...
}
```
**Why this caused the bug:**
1. In Qt, `QListWidget` uses an internal model (`QListModel`). When rows are added or removed, the model emits `rowsInserted` / `rowsAboutToBeInserted`. `QListView` (the base class of `QListWidget`) listens to these model signals to update its geometry and calculate item bounding rects.
2. By placing `QSignalBlocker blocker(m_checkList->model());`, `QListView` was completely silenced and never notified that rows were inserted. Consequently, `QListView` never laid out the items: `checkList->visualItemRect(item)` returned `QRect(-1, -1, 0, 0)`.
3. In `QListView::paintEvent()`, Qt only paints items whose visual rect intersects the viewport. Because all items had rect `(-1, -1, 0, 0)`, **none of the checklist items were ever painted** on screen!
4. **Why theme switching repaired it:** When the user switched themes (or light/dark mode), `ThemeManager::applyTheme()` set a new stylesheet on `qApp`. Applying a stylesheet triggers a full widget polish and invokes `doItemsLayout()` on all `QListView` widgets across the application. That forced `m_checkList` to finally lay out its rows. Once laid out, items remained visible until the app was restarted. On a fresh launch, `fillChecklist()` blocked the model signals again, putting the checklist back into the un-laid-out invisible state.

---

## 3. Task 2: Reproduce at Startup in Dark Theme (Failing Output on HEAD)

On commit `143b239` (before the fix), `testChecklistStartupContrastDarkAndLight` failed immediately on fresh start in dark mode because rows were never laid out:

```
$ cmake --build --preset dev-linux --target tst_StudyRegression && ./build/dev-linux/tests/ui/tst_StudyRegression testChecklistStartupContrastDarkAndLight
[1/4] Automatic MOC and UIC for target tst_StudyRegression
[2/3] Building CXX object tests/ui/CMakeFiles/tst_StudyRegression.dir/tst_StudyRegression.cpp.o
[3/3] Linking CXX executable tests/ui/tst_StudyRegression
********* Start testing of tst_StudyRegression *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_StudyRegression::initTestCase()
QDEBUG : tst_StudyRegression::testChecklistStartupContrastDarkAndLight() detailVisible: true clVisible: true clSize: QSize(272, 140) vpRect: QRect(0,0 270x138) rowRect: QRect(-1,-1 0x0)
FAIL!  : tst_StudyRegression::testChecklistStartupContrastDarkAndLight() 'rowRect.intersects(vpRect)' returned FALSE. (Row 0 not in viewport)
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_StudyRegression.cpp(915)]
QDEBUG : tst_StudyRegression::testChecklistStartupContrastDarkAndLight() detailVisible: true clVisible: true clSize: QSize(272, 140) vpRect: QRect(0,0 270x138) rowRect: QRect(-1,-1 0x0)
FAIL!  : tst_StudyRegression::testChecklistStartupContrastDarkAndLight() 'rowRect.intersects(vpRect)' returned FALSE. (Row 0 not in viewport)
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_StudyRegression.cpp(915)]
PASS   : tst_StudyRegression::cleanupTestCase()
Totals: 2 passed, 1 failed, 0 skipped, 0 blacklisted, 704ms
********* Finished testing of tst_StudyRegression *********
```

---

## 4. Task 3: Root-Cause Fix and Improvements

1. **Removed Signal Blocking on Model:**
   - Removed `const QSignalBlocker blocker(m_checkList->model())` in `fillChecklist()`, `addChecklistItem()`, `deleteChecklistItem()`, and `moveChecklistItem()`.
   - Introduced `bool m_syncingChecklist{false}` to guard `persistChecklistOrder()`, ensuring `persistChecklistOrder()` only runs on user drag-and-drop operations and not during internal model synchronization.
   - Connected `TopicDetailPanel` to `ThemeManager::themeChanged` to request viewport updates upon runtime theme switches.
2. **QSS Explicit Text Colors:**
   - Added `color: @text;` explicitly to `#StudyDetailCheckList`, `#StudyDetailCheckList::item`, and `#StudyDetailResourcesTable` in `resources/styles/theme.qss`.
3. **Audit of Token Colors:**
   - `grep -rn "setForeground\|setBackground\|QBrush\|tokens()\.\(text\|surface\|accent\)" src/ui | grep -v "^src/ui/theme/"` returned 0 occurrences of stored/baked token colors. All widgets dynamically resolve tokens during paint events or through stylesheet variables.
4. **Checklist Add Workflow, Scroll, and Focus:**
   - In `addChecklistItem()`, after persisting to `TopicRepository`:
     - Calls `fillChecklist(*m_topicId)` to refresh from the repository.
     - Automatically scrolls to the newly created row via `m_checkList->scrollToItem(...)`.
     - Clears `m_checkAddEdit` and keeps keyboard focus (`m_checkAddEdit->setFocus()`) so pressing Enter repeatedly adds subsequent checklist items.
     - Set `autoDefault(false)`, `default(false)`, and `focusPolicy(Qt::ClickFocus)` on `checkAddBtn` to prevent it from stealing focus on Enter.
5. **Dynamic Checklist Sizing & Visible Scrollbar:**
   - Removed the fixed `140` pixel height.
   - Configured minimum height dynamically via `(fontMetrics().lineSpacing() + 8) * 5 + 4` ensuring at least 5 checklist rows are always visible.
   - Configured `m_checkList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding)` with layout stretch 1 in `m_form` so it expands to absorb spare detail panel height.
   - Verified that when checklist items overflow (e.g. 20 items), the list's internal vertical scrollbar is visible and has high contrast in both themes.
6. **Logging:**
   - Added `qCDebug(lcStudy) << "checklist refreshed: count=" << m_checkList->count();` in `fillChecklist`. Preserved all existing log lines and autosave no-op rules.

---

## 5. Fix Commits

```
$ git log --oneline -5
f207615 (HEAD -> main) docs(spec): document v1.10 checklist display, model signaling, and sizing
b6257e4 fix(study): fix checklist item display, model signaling, focus retention, and sizing
143b239 docs(prompts): add M5 checklist display prompt
72be6ac docs(reports): record user resolution on tray-only notification delivery
9c2f4c2 docs(reports): update M5 regression report with round 2 fix-ups
```

- **Commit `b6257e4`:** Code fix for `TopicDetailPanel`, `theme.qss`, `tst_StudyRegression`, and updated smoke screenshots.
- **Commit `f207615`:** Updated `docs/SPEC.md` Section 14 (Changelog v1.10).

---

## 6. New Test Slots

Three new test slots were added to `tests/ui/tst_StudyRegression.cpp`:
1. `testChecklistStartupContrastDarkAndLight`:
   - Builds window in same order as `main.cpp` with saved dark theme and no subsequent theme switch (and repeats for light theme).
   - Real click selection on topic card, real key clicks and button click to add a 5th item.
   - Grabs viewport image and verifies WCAG contrast ratio $\ge 3:1$ on text pixels for all 5 rows.
   - Performs theme switch (dark -> light -> dark) and asserts the painted pixel image matches the fresh start image identically (`imgFresh == imgAfterSwitch`).
2. `testChecklistAddFocusScrollAndOverflowSizing`:
   - Verifies minimum height shows at least 5 rows dynamically without hard-coded pixel heights.
   - Tests Enter key adds checklist item, clears input, and retains focus.
   - Tests second Enter key adds another item immediately.
   - Verifies newly added row is scrolled into view.
   - Verifies checklist expanding policy and stretch factor 1 absorb spare panel height.
   - Verifies 20 items trigger visible internal vertical scrollbar.
3. `testGeneralStartupContrastAllPagesBothThemes`:
   - Tests saved dark theme and light theme with no theme switch on a fresh start across all application pages.
   - Verifies WCAG text contrast $\ge 3:1$ on Dashboard (stat cards / rings), Study (board, checklist, resources table), Jobs (table view), and Calendar (month / agenda).

---

## 7. Raw Test Suite Output (`./scripts/check.sh`)

```
$ CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.2s)
-- Generating done (0.2s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
[1/25] Linking CXX executable tests/data/tst_Database
[2/25] Linking CXX executable jobprep
[3/25] Linking CXX executable tests/data/tst_TrackRepository
[4/25] Linking CXX executable tests/data/tst_TopicRepository
[5/25] Linking CXX executable tests/data/tst_SessionRepository
[6/25] Linking CXX executable tests/data/tst_ApplicationRepository
[7/25] Linking CXX executable tests/data/tst_InterviewRepository
[8/25] Linking CXX executable tests/data/tst_ReminderLogRepository
[9/25] Linking CXX executable tests/data/tst_SeedService
[10/25] Linking CXX executable tests/data/tst_TransactionBehavior
[11/25] Linking CXX executable tests/ui_smoke/ui_smoke
[12/25] Linking CXX executable tests/ui_smoke/tst_MonthView
[13/25] Linking CXX executable tests/models/tst_AgendaListModel
[14/25] Linking CXX executable tests/models/tst_ApplicationTableModel
[15/25] Linking CXX executable tests/models/tst_ApplicationFilterProxy
[16/25] Linking CXX executable tests/models/tst_TopicListModel
[17/25] Linking CXX executable tests/models/tst_TopicBoardProxy
[18/25] Linking CXX executable tests/models/tst_TopicTableProxy
[19/25] Linking CXX executable tests/services/tst_AgendaService
[20/25] Linking CXX executable tests/services/tst_ExportService
[21/25] Linking CXX executable tests/services/tst_DayWatcher
[22/25] Linking CXX executable tests/services/tst_ReminderService
[23/25] Linking CXX executable tests/services/tst_SettingsService
[24/25] Linking CXX executable tests/services/tst_StatsService
[25/25] Linking CXX executable tests/ui/tst_Notifiers
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start  1: tst_Database
 1/25 Test  #1: tst_Database .....................   Passed    0.04 sec
      Start  2: tst_TrackRepository
 2/25 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start  3: tst_TopicRepository
 3/25 Test  #3: tst_TopicRepository ..............   Passed    0.20 sec
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
10/25 Test #10: ui_smoke .........................   Passed   98.98 sec
      Start 11: tst_MonthView
11/25 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 12: tst_AgendaListModel
12/25 Test #12: tst_AgendaListModel ..............   Passed    0.04 sec
      Start 13: tst_ApplicationTableModel
13/25 Test #13: tst_ApplicationTableModel ........   Passed    0.07 sec
      Start 14: tst_ApplicationFilterProxy
14/25 Test #14: tst_ApplicationFilterProxy .......   Passed    0.03 sec
      Start 15: tst_TopicListModel
15/25 Test #15: tst_TopicListModel ...............   Passed    0.20 sec
      Start 16: tst_TopicBoardProxy
16/25 Test #16: tst_TopicBoardProxy ..............   Passed    0.12 sec
      Start 17: tst_TopicTableProxy
17/25 Test #17: tst_TopicTableProxy ..............   Passed    0.13 sec
      Start 18: tst_AgendaService
18/25 Test #18: tst_AgendaService ................   Passed    0.15 sec
      Start 19: tst_DayWatcher
19/25 Test #19: tst_DayWatcher ...................   Passed    0.02 sec
      Start 20: tst_ExportService
20/25 Test #20: tst_ExportService ................   Passed    0.02 sec
      Start 21: tst_ReminderService
21/25 Test #21: tst_ReminderService ..............   Passed    0.12 sec
      Start 22: tst_SettingsService
22/25 Test #22: tst_SettingsService ..............   Passed    0.01 sec
      Start 23: tst_StatsService
23/25 Test #23: tst_StatsService .................   Passed    2.12 sec
      Start 24: tst_Notifiers
24/25 Test #24: tst_Notifiers ....................   Passed    2.40 sec
      Start 25: tst_StudyRegression
25/25 Test #25: tst_StudyRegression ..............   Passed    6.83 sec

100% tests passed, 0 tests failed out of 25

Total Test time (real) = 112.72 sec
==> Quality check PASSED with zero warnings and all tests green!
```

---

## 8. Totals by Command with Raw Output

```
$ total=0; for t in build/dev-linux/tests/*/*; do if [ -x "$t" ] && [ ! -d "$t" ]; then count=$("$t" -functions | grep -v '^[A-Z]' | wc -l); total=$((total + count)); echo "$t: $count"; fi; done; echo "TOTAL FUNCTIONS: $total"
build/dev-linux/tests/data/tst_ApplicationRepository: 18
build/dev-linux/tests/data/tst_Database: 16
build/dev-linux/tests/data/tst_InterviewRepository: 9
build/dev-linux/tests/data/tst_ReminderLogRepository: 7
build/dev-linux/tests/data/tst_SeedService: 5
build/dev-linux/tests/data/tst_SessionRepository: 10
build/dev-linux/tests/data/tst_TopicRepository: 18
build/dev-linux/tests/data/tst_TrackRepository: 8
build/dev-linux/tests/data/tst_TransactionBehavior: 8
build/dev-linux/tests/models/tst_AgendaListModel: 5
build/dev-linux/tests/models/tst_ApplicationFilterProxy: 2
build/dev-linux/tests/models/tst_ApplicationTableModel: 6
build/dev-linux/tests/models/tst_TopicBoardProxy: 9
build/dev-linux/tests/models/tst_TopicListModel: 7
build/dev-linux/tests/models/tst_TopicTableProxy: 6
build/dev-linux/tests/services/tst_AgendaService: 24
build/dev-linux/tests/services/tst_DayWatcher: 4
build/dev-linux/tests/services/tst_ExportService: 4
build/dev-linux/tests/services/tst_ReminderService: 18
build/dev-linux/tests/services/tst_SettingsService: 4
build/dev-linux/tests/services/tst_StatsService: 10
build/dev-linux/tests/ui_smoke/tst_MonthView: 13
build/dev-linux/tests/ui_smoke/ui_smoke: 80
build/dev-linux/tests/ui/tst_Notifiers: 21
build/dev-linux/tests/ui/tst_StudyRegression: 12
TOTAL FUNCTIONS: 324
```
- **Total Test Functions:** 324 across 25 binaries (+3 functions added to `tst_StudyRegression` from baseline 321).

---

## 9. Not Verified by Agent
- Real native Wayland desktop compositor rendering (tests execute via Qt headless `offscreen` platform plugin).
- The visual first-start appearance under 100% and 200% fractional display scaling in both themes on a real desktop monitor.
- System dark preference signal auto-detection when the OS theme flips dynamically via DBus portal outside the Qt test process.
