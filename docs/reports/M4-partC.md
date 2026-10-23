# Milestone M4 Part C Report — Fix-ups, Sessions, Track Management & List View

**Date:** 2026-10-24  
**Author:** AI Assistant & Pair Programmer  
**Branch:** `main`  
**OS / Compiler:** Linux x86_64 (Ubuntu 26.04), GCC 15.2.0, Qt 6.10.2 LTS  
**Preceding Commits:**
- `adaa6a9` refactor(study): resolve Part B carry-overs (layering, drag feedback test, screenshot command)
- `9680181` docs(reports): add M4 Part B errata
- `e6a69b1` feat(study): implement Part C sessions, tracks, list view, and calendar jump
- `5279eb2` fix(study): address Part C fix-up items (palette, proxy, swatches, tests, screenshots)
- `6a8206a` docs(spec): document M4 Part C fix-up changes in section 14 (v1.6)

---

## 1. What Changed

Part C completes the remaining functional surfaces of the Study preparation module (Milestone M4) along with the code and evidence fix-ups required by `docs/prompts/M4-partC-fixups.md`:

1. **Track Management Dialog (`TrackDialog`):**
   - Modal management of study tracks: list existing tracks, inline add/edit form with name and color swatches.
   - 8-color preset palette hosted centrally in `Tokens::trackPalette()` (`#5B6CFF` Indigo/C++, `#F59E0B` Amber/English, `#0D9488` Teal, `#10B981` Emerald, `#F43F5E` Rose, `#0284C7` Sky, `#8B5CF6` Purple, `#F97316` Orange).
   - Dynamic custom swatch button (`m_customSwatchBtn`) rendered when editing a track with a non-preset color hex.
   - Case-insensitive duplicate name validation with inline warning.
   - Accessible keyboard swatch navigation (Tab + Left/Right/Up/Down arrow keys) with color name tooltips and high-contrast checkmark `✓` drawn on the selected swatch.
   - Destructive track deletion requires confirmation dialog explicitly naming the number of assigned topics and assuring the user that logged study sessions are preserved. Deleting a track cascades deletion to its topics and triggers calendar marker invalidation.

2. **Session Logging Dialog (`SessionDialog`):**
   - Quick session logging with injectable `todayProvider` clock defaulting to current local date.
   - Target topic combo box with `(No topic)` at index 0 (stored with null `topic_id`).
   - Duration spin box constrained between 0 and 1440 minutes (24 hours). Inline validation styling `#SessionMessageLabel` with `@danger` blocks submission if duration is < 1 minute.
   - Auto-increments topic progress percentage if completed hours reach or exceed target hours.

3. **Study List View & Proxy (`TopicTableProxy`):**
   - View switcher in Study page toolbar toggling between Kanban Board and Table List views.
   - Dedicated `TopicTableProxy` model in `src/models/` implementing column sorting:
     - Track by string name.
     - Priority by enum rank (`High` > `Medium` > `Low`).
     - Progress by numeric completion percentage.
     - Target Date chronologically with empty dates always placed last in both ascending and descending sort orders.
     - Drag-and-drop explicitly disabled (`supportedDropActions() = Qt::IgnoreAction`).
   - Styled item delegates: `ProgressBarDelegate` for completion percentage and `StatusBadgeDelegate` for topic status badges.
   - Keyboard shortcuts in list view: `Ctrl+F` focuses search, `Enter` opens topic detail drawer, and `Delete` opens topic deletion confirmation.

4. **Global Header & Navigation Bindings:**
   - Header search input disabled with placeholder `"Search not available on this page"` on Study, Calendar, Home, and Settings pages; active only on Jobs page.
   - Global `Ctrl+N` routing calls `triggerCurrentPageNew()`:
     - Jobs page: opens `ApplicationDialog`.
     - Calendar page: opens `InterviewDialog`.
     - Study page: focuses Backlog quick-add field.
   - Header "+ New" menu routes "Study Session" and "Study Topic" directly to `StudyPage`.

5. **Calendar Integration & Refresh:**
   - Double-clicking a day cell in `MonthView` opens "New interview" per SPEC C5.
   - Activating a Study Target row in the Calendar Agenda list emits `studyTopicActivated(int)` to jump to the topic in the Study page (supporting both Board and List view modes, with non-fatal handling if the topic was deleted).
   - Deleting a track immediately invalidates and clears Calendar study target markers via `CalendarPage`'s injected `TrackRepository&`.

---

## 2. Plan vs. Result (Honest Gaps)

| Planned Item | Status | Result / Notes |
|---|---|---|
| Track Management Dialog (CRUD, swatches, cascade) | Done | Full modal CRUD with 8 preset swatches + dynamic custom swatch, keyboard navigation, case-insensitive duplicate check, cascade confirmation. |
| Session Logging Dialog (injectable clock, topic select, duration) | Done | Quick session log with injectable clock, `(No topic)` support, 1440m cap, inline validation via theme token styling. |
| Session Editing / Deletion | Omitted (Planned Gap) | As documented in SPEC §14, session editing/deletion is intentionally deferred to Milestone M6. Logged sessions cannot be edited in M4. |
| Study List View with sorting | Done | Implemented via `TopicTableProxy` in `src/models/` with track, priority rank, progress, and target date sorting (empty dates last). |
| Drag & drop disabled in list view | Done | Explicitly disabled via `supportedDropActions` and `flags` in `TopicTableProxy`. |
| Calendar Jump from Agenda | Done | Double-click or Enter on Study Target row in agenda list jumps to Study page and selects topic; MonthView double-click preserved for "New interview". |
| Calendar refresh on track delete | Done | `CalendarPage` listens to `TrackRepository::changed` and refreshes agenda model and month markers. |
| Global Ctrl+N routing | Done | Dispatches to `triggerCurrentPageNew()` across Jobs, Calendar, and Study. |
| Header search toggle | Done | Disabled across Study, Calendar, Home, and Settings pages; enabled on Jobs. |

---

## 3. Amendment Checklist with Evidence

### B7: Global Ctrl+N Routing
- **Requirement:** `Ctrl+N` routes to the current active page's new-item action (Jobs: New Application, Calendar: New Interview, Study: Backlog Quick-Add).
- **Evidence:**
  - Implementation: `src/ui/MainWindow.cpp:151-163` (`triggerCurrentPageNew()`).
  - Unit test slot: `tests/ui_smoke/tst_UiSmoke.cpp:1396-1431` (`testCtrlNTriggersCurrentPageNew()`).

### B8: Header Search Scoping
- **Requirement:** Header search input is disabled with explanatory placeholder on non-applicable pages (Study, Calendar, Home, Settings) and re-enabled on Jobs.
- **Evidence:**
  - Implementation: `src/ui/MainWindow.cpp:84-106` (`onPageChanged(int index)`).
  - Unit test slot: `tests/ui_smoke/tst_UiSmoke.cpp:1269-1288` (`testHeaderSearchDisabledOnStudyPage()`).

### B9: Calendar Jump to Topic
- **Requirement:** Activating a study target in Calendar jumps to the topic in Study view (in both Board and List modes); non-fatal if topic was deleted.
- **Evidence:**
  - Implementation: `src/ui/MainWindow.cpp:131-137` and `src/ui/pages/StudyPage.cpp:396-418` (`selectTopic(int topicId)`).
  - Unit test slots: `tests/ui_smoke/tst_UiSmoke.cpp:1290-1320` (`testCalendarJumpToStudyTopic()`).

### C1: Session Logging Dialog
- **Requirement:** Injectable today clock, `(No topic)` at index 0, duration spinbox (1-1440 min) with inline validation, session logging without topic.
- **Evidence:**
  - Injectable clock: `src/ui/dialogs/SessionDialog.h:37`: `explicit SessionDialog(QWidget* parent, const QList<domain::Topic>& topics, std::function<QDate()> todayProvider = nullptr);`
  - `(No topic)` combo item: `src/ui/dialogs/SessionDialog.cpp:73`: `m_topicCombo->addItem(tr("(No topic)"), -1);`
  - Inline validation & duration cap: `src/ui/dialogs/SessionDialog.cpp:118-123` and `src/ui/dialogs/SessionDialog.cpp:82`.
  - Unit test slot: `tests/ui_smoke/tst_UiSmoke.cpp:1159-1205` (`testStudySessionDialogLogTime()`).

### C2: Track Management Dialog
- **Requirement:** Cascade delete confirmation naming topic count and preserved sessions; case-insensitive duplicate check; keyboard swatch navigation with color tooltips and high-contrast checkmark; custom swatch support.
- **Evidence:**
  - Topic count in delete confirmation: `src/ui/dialogs/TrackDialog.cpp:249-253`.
  - Duplicate check: `src/ui/dialogs/TrackDialog.cpp:189-195`.
  - Swatch focus & keyboard navigation: `src/ui/dialogs/TrackDialog.cpp:84-118`.
  - Custom swatch button: `src/ui/dialogs/TrackDialog.cpp:138-166`.
  - Unit test slot: `tests/ui_smoke/tst_UiSmoke.cpp:1207-1249` (`testStudyTrackDialogCrudAndCascade()`).

### C3: Calendar Refresh on Track Delete
- **Requirement:** Deleting a track invalidates and refreshes Calendar study-target markers and agenda items.
- **Evidence:**
  - Implementation: `src/ui/pages/CalendarPage.cpp:28` (`connect(&trackRepo, &TrackRepository::changed, this, &CalendarPage::onDataChanged)`).
  - Unit test slot: `tests/ui_smoke/tst_UiSmoke.cpp:1372-1393` (`testCalendarTrackDeleteClearsMarkers()`).

### C4: Study List View & Proxy
- **Requirement:** Dedicated `TopicTableProxy` in `src/models/`; sorting by track, priority by rank, progress, and target date (empty dates last in both directions); DnD disabled; keyboard shortcuts (Ctrl+F, Enter, Delete).
- **Evidence:**
  - Class definition: `src/models/TopicTableProxy.h` and `src/models/TopicTableProxy.cpp`.
  - Priority rank sorting: `src/models/TopicTableProxy.cpp:44-50`.
  - Empty target dates last: `src/models/TopicTableProxy.cpp:58-69`.
  - DnD disabled: `src/models/TopicTableProxy.cpp:73-77`.
  - Dedicated model test target: `tests/models/tst_TopicTableProxy.cpp` (all 6 slots passing).
  - List shortcuts unit test: `tests/ui_smoke/tst_UiSmoke.cpp:1434-1481` (`testStudyListViewToggleSortAndDelegates()`).

### C5: MonthView Day Click vs Agenda Row
- **Requirement:** Double-clicking day cell in `MonthView` opens "New interview" per SPEC C5; double-clicking on a day with study targets does not jump to topic; Agenda list row activation triggers study topic jump.
- **Evidence:**
  - MonthView double-click behavior: `src/ui/pages/CalendarPage.cpp:81-85` (`connect(m_monthView, &MonthView::dayDoubleClicked, this, [this](const QDate& date) { openNewInterviewDialog(date); });`).
  - Unit test slot: `tests/ui_smoke/tst_UiSmoke.cpp:1322-1335` (`testMonthViewDoubleClicksDayWithoutOpeningStudyTarget()`).

### C6: Clean Layering & Theme Palette
- **Requirement:** Zero color hex codes or UI tokens in domain/data/services/models; track palette in `Tokens`.
- **Evidence:**
  - Track palette definition: `src/ui/theme/Tokens.cpp:177-186` (`Tokens::trackPalette()`).
  - Layering grep verification in Section 4.

---

## 4. Verification & Command Evidence

### F1.1 Layering Grep
```bash
grep -rnE "StatusStyle|Theme::|Tokens|ui/|QtGui|QIcon|QPixmap|QColor|QPainter|QFont" src/domain src/data src/services src/models
```
Output:
```text
src/services/SettingsService.cpp:30:    return m_settings->value(u"ui/sidebarCollapsed"_s, false).toBool();
src/services/SettingsService.cpp:35:    m_settings->setValue(u"ui/sidebarCollapsed"_s, collapsed);
src/services/SettingsService.cpp:40:    return m_settings->value(u"ui/themeMode"_s, u"system"_s).toString();
src/services/SettingsService.cpp:45:    m_settings->setValue(u"ui/themeMode"_s, mode);
src/services/SettingsService.cpp:50:    return m_settings->value(u"ui/accentPreset"_s, u"indigo"_s).toString();
src/services/SettingsService.cpp:55:    m_settings->setValue(u"ui/accentPreset"_s, preset);
src/models/ApplicationTableModel.cpp:105:            // The label lives in StatusStyle, which the status badge delegate paints;
src/models/TopicListModel.h:20:/// are handled by delegates in ui/.
```
All matches are settings keys or comment documentation. Zero UI imports or theme tokens exist in non-UI layers.

### F1.2 Git Commit History
```bash
git show --stat adaa6a9
```
```text
commit adaa6a98ec8b38da57ebc65648d8493d48fd0a4c
Author: Tony <group.ik31@gmail.com>
Date:   Mon Oct 19 16:35:38 2026 +0300

    refactor(study): resolve Part B carry-overs (layering, drag feedback test, screenshot command)

 scripts/screenshots.ps1                | 20 +++++++
 scripts/screenshots.sh                 | 19 +++++++
 src/models/TopicListModel.cpp          | 11 +---
 src/ui/delegates/TopicCardDelegate.cpp | 17 ++++++
 src/ui/delegates/TopicCardDelegate.h   |  2 +
 src/ui/widgets/TopicBoardColumn.cpp    | 39 --------------
 src/ui/widgets/TopicBoardColumn.h      |  1 -
 tests/ui_smoke/tst_UiSmoke.cpp         | 99 +++++++++++++++++++++++++++++++++-
 8 files changed, 157 insertions(+), 51 deletions(-)
```

```bash
git show --stat 9680181
```
```text
commit 968018144d15be47942fcaa52c506d6d0bbad617
Author: Tony <group.ik31@gmail.com>
Date:   Mon Oct 19 16:35:41 2026 +0300

    docs(reports): add M4 Part B errata

 docs/reports/M4-partB-errata.md | 22 ++++++++++++++++++++++
 1 file changed, 22 insertions(+)
```

```bash
git show --stat e6a69b1
```
```text
commit e6a69b16f54bbb7aa20edd5bf674657be3973fc7
Author: Tony <group.ik31@gmail.com>
Date:   Mon Oct 19 17:47:44 2026 +0300

    feat(study): implement Part C sessions, tracks, list view, and calendar jump

 CMakeLists.txt                                  |   6 +
 docs/SPEC.md                                    |   8 +-
 docs/reports/img/study-board-dark.png           | Bin 64796 -> 66508 bytes
 docs/reports/img/study-board-detail-dark.png    | Bin 80366 -> 82008 bytes
 docs/reports/img/study-board-detail-light.png   | Bin 79559 -> 81186 bytes
 docs/reports/img/study-board-drag-dark.png      | Bin 90085 -> 91812 bytes
 docs/reports/img/study-board-drag-light.png     | Bin 88489 -> 90116 bytes
 docs/reports/img/study-board-empty-dark.png     | Bin 24195 -> 24333 bytes
 docs/reports/img/study-board-empty-light.png    | Bin 23928 -> 24169 bytes
 docs/reports/img/study-board-light.png          | Bin 64029 -> 65768 bytes
 docs/reports/img/study-empty-tracks-dark.png    | Bin 0 -> 24333 bytes
 docs/reports/img/study-empty-tracks-light.png   | Bin 0 -> 24169 bytes
 docs/reports/img/study-list-dark.png            | Bin 0 -> 153568 bytes
 docs/reports/img/study-list-light.png           | Bin 0 -> 152701 bytes
 docs/reports/img/study-session-dialog-dark.png  | Bin 0 -> 15142 bytes
 docs/reports/img/study-session-dialog-light.png | Bin 0 -> 15075 bytes
 docs/reports/img/study-tracks-dialog-dark.png   | Bin 0 -> 15407 bytes
 docs/reports/img/study-tracks-dialog-light.png  | Bin 0 -> 15184 bytes
 src/ui/MainWindow.cpp                           |  58 ++-
 src/ui/MainWindow.h                             |   7 +
 src/ui/delegates/ProgressBarDelegate.cpp        |  55 +++
 src/ui/delegates/ProgressBarDelegate.h          |  27 ++
 src/ui/delegates/StatusBadgeDelegate.cpp        |  20 +-
 src/ui/delegates/StatusBadgeDelegate.h          |   9 +-
 src/ui/dialogs/SessionDialog.cpp                | 190 +++++++++
 src/ui/dialogs/SessionDialog.h                  |  68 +++
 src/ui/dialogs/TrackDialog.cpp                  | 362 ++++++++++++++++
 src/ui/dialogs/TrackDialog.h                    |  74 ++++
 src/ui/pages/CalendarPage.cpp                   |  16 +-
 src/ui/pages/CalendarPage.h                     |   7 +
 src/ui/pages/StudyPage.cpp                      | 381 ++++++++++++++---
 src/ui/pages/StudyPage.h                        |  65 ++-
 src/ui/widgets/TopicDetailPanel.cpp             |   8 +-
 src/ui/widgets/TopicDetailPanel.h               |   2 +
 tests/ui_smoke/tst_UiSmoke.cpp                  | 524 +++++++++++++++++++++++-
 35 files changed, 1771 insertions(+), 116 deletions(-)
```

```bash
git show --stat 1ebf4ee
```
```text
commit 1ebf4eeebf74a118f91ec45c47d4da36d726e2f4
Author: Tony <group.ik31@gmail.com>
Date:   Mon Oct 19 14:56:46 2026 +0300

    feat(study): M4 Part B round-2 fix-ups, drag feedback and tests

 CMakeLists.txt                                |   2 +
 docs/SPEC.md                                  |   2 +-
 docs/reports/M4-partB-fixups2.md              | 405 ++++++++++++++++++++++++++
 docs/reports/M4-partB.md                      |  50 ++++
 docs/reports/img/study-board-dark.png         | Bin 65047 -> 64796 bytes
 docs/reports/img/study-board-detail-dark.png  | Bin 81665 -> 80386 bytes
 docs/reports/img/study-board-detail-light.png | Bin 80883 -> 79548 bytes
 docs/reports/img/study-board-drag-dark.png    | Bin 0 -> 90103 bytes
 docs/reports/img/study-board-drag-light.png   | Bin 0 -> 88479 bytes
 docs/reports/img/study-board-light.png        | Bin 64262 -> 64029 bytes
 src/models/TopicListModel.cpp                 |  10 +
 src/ui/delegates/TopicCardDelegate.cpp        |  15 +-
 src/ui/pages/StudyPage.cpp                    |   1 +
 src/ui/widgets/TopicBoardColumn.cpp           |  21 +-
 src/ui/widgets/TopicBoardColumn.h             |   9 +-
 src/ui/widgets/TopicListView.cpp              | 227 +++++++++++++++
 src/ui/widgets/TopicListView.h                |  48 +++
 tests/ui_smoke/tst_UiSmoke.cpp                | 269 ++++++++++++++++-
 18 files changed, 1037 insertions(+), 22 deletions(-)
```

```bash
git show --stat 5279eb2
```
```text
commit 5279eb2fc0c771b329855a679fd46ea452294f1b
Author: Tony <group.ik31@gmail.com>
Date:   Sat Oct 24 01:16:19 2026 +0300

    fix(study): address Part C fix-up items (palette, proxy, swatches, tests, screenshots)

 CMakeLists.txt                                  |   2 +
 docs/reports/img/study-board-detail-dark.png    | Bin 82008 -> 81996 bytes
 docs/reports/img/study-board-detail-light.png   | Bin 81186 -> 81179 bytes
 docs/reports/img/study-board-drag-dark.png      | Bin 91812 -> 91789 bytes
 docs/reports/img/study-board-drag-light.png     | Bin 90116 -> 90103 bytes
 docs/reports/img/study-board-empty-dark.png     | Bin 24333 -> 27399 bytes
 docs/reports/img/study-board-empty-light.png    | Bin 24169 -> 27232 bytes
 docs/reports/img/study-list-dark.png            | Bin 153568 -> 153552 bytes
 docs/reports/img/study-list-light.png           | Bin 152701 -> 152694 bytes
 docs/reports/img/study-session-dialog-dark.png  | Bin 15142 -> 15274 bytes
 docs/reports/img/study-session-dialog-light.png | Bin 15075 -> 15199 bytes
 docs/reports/img/study-tracks-dialog-dark.png   | Bin 15407 -> 15304 bytes
 docs/reports/img/study-tracks-dialog-light.png  | Bin 15184 -> 15095 bytes
 resources/styles/theme.qss                      |   4 +-
 src/models/AgendaListModel.cpp                  |   4 +-
 src/models/TopicTableProxy.cpp                  |  78 ++++++
 src/models/TopicTableProxy.h                    |  34 +++
 src/ui/MainWindow.h                             |   9 +-
 src/ui/dialogs/SessionDialog.cpp                |   1 -
 src/ui/dialogs/TrackDialog.cpp                  | 108 ++++++--
 src/ui/dialogs/TrackDialog.h                    |  10 +-
 src/ui/pages/CalendarPage.cpp                   |   2 +-
 src/ui/pages/StudyPage.cpp                      |  49 +---
 src/ui/theme/Tokens.cpp                         |  14 +
 src/ui/theme/Tokens.h                           |   7 +
 tests/models/CMakeLists.txt                     |   1 +
 tests/models/tst_TopicTableProxy.cpp            | 203 ++++++++++++++
 tests/ui_smoke/tst_UiSmoke.cpp                  | 348 +++++++++++++++++++++---
 28 files changed, 752 insertions(+), 122 deletions(-)
```

```bash
git show --stat 6a8206a
```
```text
commit 6a8206a7cc4375211e9b86a3b9e18d15d7376735
Author: Tony <group.ik31@gmail.com>
Date:   Sat Oct 24 01:16:48 2026 +0300

    docs(spec): document M4 Part C fix-up changes in section 14 (v1.6)

 docs/SPEC.md | 12 +++++++-----
 1 file changed, 7 insertions(+), 5 deletions(-)
```

### F1.3 Test Counts (19 Test Binaries)
```bash
for bin in $(find build/dev-linux/tests -maxdepth 2 -type f -executable | sort); do
    echo -n "$(basename $bin): "
    $bin -functions | wc -l
done
```
Output:
```text
tst_ApplicationRepository: 15
tst_Database: 16
tst_InterviewRepository: 7
tst_ReminderLogRepository: 5
tst_SeedService: 5
tst_SessionRepository: 9
tst_TopicRepository: 17
tst_TrackRepository: 8
tst_TransactionBehavior: 8
tst_AgendaListModel: 5
tst_ApplicationFilterProxy: 2
tst_ApplicationTableModel: 6
tst_TopicBoardProxy: 9
tst_TopicListModel: 7
tst_TopicTableProxy: 6
tst_AgendaService: 15
tst_ExportService: 4
tst_MonthView: 13
ui_smoke: 62
```

#### UiSmoke Arithmetic
- Part B test count at `1ebf4ee`: **51** slots.
- Test slots added since `1ebf4ee`:
  ```bash
  git diff 1ebf4ee..HEAD -- tests/ui_smoke | grep -E '^\+.*void test'
  ```
  Output:
  ```text
  +    void testStudyPriorityTooltip();
  +    void testStudyDragFeedbackIndicatorAndHighlight();
  +    void testStudySessionDialogLogTime();
  +    void testStudyTrackDialogCrudAndCascade();
  +    void testHeaderNewSessionAndTopicRouting();
  +    void testHeaderSearchDisabledOnStudyPage();
  +    void testCalendarJumpToStudyTopic();
  +    void testCalendarTopicDateAndTrackRename();
  +    void testCalendarTrackDeleteClearsMarkers();
  +    void testCtrlNTriggersCurrentPageNew();
  +    void testStudyListViewToggleSortAndDelegates();
  ```
  Total added: **11** test slots.  
  Arithmetic: **51 + 11 = 62** slots.
- Pre-existing status:
  - `testNewEntryPointsOpenOneDialog` was introduced in Milestone M2 commit `d443ce1`.
  - `testStudyPageApi` was introduced in Milestone M4 Part B commit `05b8c47`.
  Both already existed at `1ebf4ee`.

### F1.4 Screenshots
```bash
./scripts/screenshots.sh
```
Output:
```text
==> Building ui_smoke...
ninja: no work to do.
==> Running screenshot generation with JOBPREP_REPORT_IMG_DIR=/home/tony/projects/JobPrep/docs/reports/img...
********* Start testing of tst_UiSmoke *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_UiSmoke::initTestCase()
PASS   : tst_UiSmoke::testCalendarScreenshots()
PASS   : tst_UiSmoke::testStudyScreenshots()
PASS   : tst_UiSmoke::cleanupTestCase()
Totals: 4 passed, 0 failed, 0 skipped, 0 blacklisted, 17765ms
********* Finished testing of tst_UiSmoke *********
==> Screenshots successfully regenerated in /home/tony/projects/JobPrep/docs/reports/img!
```

```bash
file docs/reports/img/study-*.png
```
Output:
```text
docs/reports/img/study-board-dark.png:           PNG image data, 1200 x 720, 8-bit/color RGB, non-interlaced
docs/reports/img/study-board-detail-dark.png:    PNG image data, 1200 x 720, 8-bit/color RGB, non-interlaced
docs/reports/img/study-board-detail-light.png:   PNG image data, 1200 x 720, 8-bit/color RGB, non-interlaced
docs/reports/img/study-board-drag-dark.png:      PNG image data, 1200 x 720, 8-bit/color RGB, non-interlaced
docs/reports/img/study-board-drag-light.png:     PNG image data, 1200 x 720, 8-bit/color RGB, non-interlaced
docs/reports/img/study-board-empty-dark.png:     PNG image data, 1200 x 720, 8-bit/color RGB, non-interlaced
docs/reports/img/study-board-empty-light.png:    PNG image data, 1200 x 720, 8-bit/color RGB, non-interlaced
docs/reports/img/study-board-light.png:          PNG image data, 1200 x 720, 8-bit/color RGB, non-interlaced
docs/reports/img/study-empty-tracks-dark.png:    PNG image data, 1200 x 720, 8-bit/color RGB, non-interlaced
docs/reports/img/study-empty-tracks-light.png:   PNG image data, 1200 x 720, 8-bit/color RGB, non-interlaced
docs/reports/img/study-list-dark.png:            PNG image data, 1200 x 720, 8-bit/color RGB, non-interlaced
docs/reports/img/study-list-light.png:           PNG image data, 1200 x 720, 8-bit/color RGB, non-interlaced
docs/reports/img/study-session-dialog-dark.png:  PNG image data, 460 x 400, 8-bit/color RGB, non-interlaced
docs/reports/img/study-session-dialog-light.png: PNG image data, 460 x 400, 8-bit/color RGB, non-interlaced
docs/reports/img/study-tracks-dialog-dark.png:   PNG image data, 520 x 440, 8-bit/color RGB, non-interlaced
docs/reports/img/study-tracks-dialog-light.png:  PNG image data, 520 x 440, 8-bit/color RGB, non-interlaced
```

```bash
md5sum docs/reports/img/study-*.png
```
Output:
```text
a9a769e70c31bf1330f93688788e3811  docs/reports/img/study-board-dark.png
3df79f9bc32b514263ccb02fab1c6f32  docs/reports/img/study-board-detail-dark.png
1f9e834c2f65b8d39457e4633b642a9d  docs/reports/img/study-board-detail-light.png
902d28ba366a9df8408a01a6779312fc  docs/reports/img/study-board-drag-dark.png
9d823d0d58fb841a48119123bb1397ea  docs/reports/img/study-board-drag-light.png
c1f460816bb00d98a1bb99ed18b0c119  docs/reports/img/study-board-empty-dark.png
cd4f63d134250e25d232497f3a4eaee7  docs/reports/img/study-board-empty-light.png
f67a24731bca8d3e5b8f84056b664875  docs/reports/img/study-board-light.png
0eb9056465a1745c8812b19a1c2242d3  docs/reports/img/study-empty-tracks-dark.png
3801f885992dc4f5b56ce05958fd1afc  docs/reports/img/study-empty-tracks-light.png
4833752a9c78ad22156fa185254468e9  docs/reports/img/study-list-dark.png
adc20c0c4d7f82fe2a45c7097558ab9e  docs/reports/img/study-list-light.png
d3091eb577ad6f41b3efd51c1f8068da  docs/reports/img/study-session-dialog-dark.png
5e9c0f879d513f04d6c65eea30eba500  docs/reports/img/study-session-dialog-light.png
005a8d5d6eb5d6be087a15aff2cd7427  docs/reports/img/study-tracks-dialog-dark.png
6c98cf4cac60dfaa834d5ef9e9321bac  docs/reports/img/study-tracks-dialog-light.png
```

#### Explanation of Empty States
- `study-board-empty-*.png` (checksum `c1f46081...` dark / `cd4f63d1...` light): Captures the state where tracks exist (e.g. C++, English) but zero topics are present. The columns are displayed with empty states ("No topics yet" + "Add topic" button).
- `study-empty-tracks-*.png` (checksum `0eb90564...` dark / `3801f885...` light): Captures the state where 0 tracks exist in the database. The board columns are hidden and replaced with the whole-page empty state card ("No tracks yet" + "Manage tracks" button).
The two image sets are visually and cryptographically distinct.

### F1.5 Clean Build Proof of Zero Warnings
```bash
cmake --build --preset dev-linux --clean-first 2>&1 | tail -30
```
Output:
```text
[114/143] Building CXX object tests/data/CMakeFiles/tst_SeedService.dir/tst_SeedService.cpp.o
[115/143] Linking CXX executable jobprep
[116/143] Linking CXX executable tests/data/tst_Database
[117/143] Linking CXX executable tests/data/tst_TrackRepository
[118/143] Building CXX object tests/data/CMakeFiles/tst_TransactionBehavior.dir/tst_TransactionBehavior.cpp.o
[119/143] Linking CXX executable tests/data/tst_TopicRepository
[120/143] Linking CXX executable tests/data/tst_SessionRepository
[121/143] Building CXX object tests/models/CMakeFiles/tst_ApplicationFilterProxy.dir/tst_ApplicationFilterProxy.cpp.o
[122/143] Building CXX object tests/models/CMakeFiles/tst_ApplicationTableModel.dir/tst_ApplicationTableModel.cpp.o
[123/143] Linking CXX executable tests/data/tst_ApplicationRepository
[124/143] Linking CXX executable tests/data/tst_InterviewRepository
[125/143] Building CXX object tests/models/CMakeFiles/tst_TopicListModel.dir/tst_TopicListModel.cpp.o
[126/143] Building CXX object tests/models/CMakeFiles/tst_TopicBoardProxy.dir/tst_TopicBoardProxy.cpp.o
[127/143] Building CXX object tests/services/CMakeFiles/tst_ExportService.dir/tst_ExportService.cpp.o
[128/143] Linking CXX executable tests/data/tst_ReminderLogRepository
[129/143] Building CXX object tests/models/CMakeFiles/tst_TopicTableProxy.dir/tst_TopicTableProxy.cpp.o
[130/143] Linking CXX executable tests/data/tst_SeedService
[131/143] Linking CXX executable tests/data/tst_TransactionBehavior
[132/143] Building CXX object tests/services/CMakeFiles/tst_AgendaService.dir/tst_AgendaService.cpp.o
[133/143] Linking CXX executable tests/ui_smoke/ui_smoke
[134/143] Linking CXX executable tests/services/tst_ExportService
[135/143] Linking CXX executable tests/ui_smoke/tst_MonthView
[136/143] Linking CXX executable tests/models/tst_ApplicationTableModel
[137/143] Linking CXX executable tests/models/tst_TopicListModel
[138/143] Linking CXX executable tests/models/tst_TopicTableProxy
[139/143] Linking CXX executable tests/models/tst_ApplicationFilterProxy
[140/143] Linking CXX executable tests/models/tst_TopicBoardProxy
[141/143] Linking CXX executable tests/services/tst_AgendaService
[142/143] Building CXX object tests/models/CMakeFiles/tst_AgendaListModel.dir/tst_AgendaListModel.cpp.o
[143/143] Linking CXX executable tests/models/tst_AgendaListModel
```

### F1.6 Icon List
**None.** All icons utilized in Milestone M4 (`graduation-cap.svg`, `plus.svg`, `search.svg`, `calendar.svg`, `chevron-right.svg`, `trash-2.svg`, `x.svg`, etc.) were pre-existing in `resources/icons/` from Milestones M0 through M2. No new SVG files were added.

### F2.1 QPA Noise Filter Explanation
- **Warning text:** `"This plugin does not support raise()"`
- **Origin:** Inside Qt GUI library (`qtiplabel.cpp`), `QToolTip::showText()` invokes `QTipLabel::raise()`.
- **Under Offscreen Plugin:** In Qt's offscreen platform window (`qoffscreenwindow.cpp`), `QOffscreenPlatformWindow::raise()` emits: `qWarning("This plugin does not support raise()");`.
- **Source Code Verification:** JobPrep source code contains zero calls to `raise()` or `activateWindow()` (`grep -rnE "\.raise\(\)|\.activateWindow\(\)" src/` returned 0 matches).
- **Conclusion:** The message is produced purely internally by Qt when tooltips are shown during offscreen tests (e.g. priority indicator tooltip, color swatch tooltips). Filtering it in `isPlatformPluginNoise()` is necessary and does not weaken the test gate for application code.

### F2.2 Hard-coded Colors Verification
```bash
grep -rnE "#[0-9a-fA-F]{6}" src/ui src/models src/services
```
Output:
```text
src/ui/theme/AgendaKindStyle.cpp:12:            return QColor(u"#F59E0B"_s);
src/ui/theme/AgendaKindStyle.cpp:14:            return QColor(u"#3B82F6"_s);
src/ui/theme/AgendaKindStyle.cpp:16:            return QColor(u"#10B981"_s);
src/ui/theme/AgendaKindStyle.cpp:18:    return QColor(u"#3B82F6"_s);
src/ui/theme/Tokens.cpp:40:        t.bg = QColor(u"#0F1117"_s);
src/ui/theme/Tokens.cpp:41:        t.surface = QColor(u"#171A23"_s);
src/ui/theme/Tokens.cpp:42:        t.surfaceAlt = QColor(u"#1E2230"_s);
src/ui/theme/Tokens.cpp:43:        t.border = QColor(u"#2A2F3F"_s);
src/ui/theme/Tokens.cpp:44:        t.text = QColor(u"#E8EAF2"_s);
src/ui/theme/Tokens.cpp:45:        t.textMuted = QColor(u"#9AA3B8"_s);
src/ui/theme/Tokens.cpp:46:        t.danger = QColor(u"#F87171"_s);
src/ui/theme/Tokens.cpp:47:        t.warning = QColor(u"#FBBF24"_s);
src/ui/theme/Tokens.cpp:48:        t.success = QColor(u"#34D399"_s);
src/ui/theme/Tokens.cpp:49:        t.accentText = QColor(u"#FFFFFF"_s);
src/ui/theme/Tokens.cpp:53:                t.accent = QColor(u"#7C8AFF"_s);
src/ui/theme/Tokens.cpp:54:                t.accentHover = QColor(u"#939EFF"_s);
src/ui/theme/Tokens.cpp:55:                t.accentPressed = QColor(u"#6774FF"_s);
src/ui/theme/Tokens.cpp:58:                t.accent = QColor(u"#2DD4BF"_s);
src/ui/theme/Tokens.cpp:59:                t.accentHover = QColor(u"#5EEAD4"_s);
src/ui/theme/Tokens.cpp:60:                t.accentPressed = QColor(u"#14B8A6"_s);
src/ui/theme/Tokens.cpp:63:                t.accent = QColor(u"#FB7185"_s);
src/ui/theme/Tokens.cpp:64:                t.accentHover = QColor(u"#FDA4AF"_s);
src/ui/theme/Tokens.cpp:65:                t.accentPressed = QColor(u"#F43F5E"_s);
src/ui/theme/Tokens.cpp:68:                t.accent = QColor(u"#FBBF24"_s);
src/ui/theme/Tokens.cpp:69:                t.accentHover = QColor(u"#FCD34D"_s);
src/ui/theme/Tokens.cpp:70:                t.accentPressed = QColor(u"#F59E0B"_s);
src/ui/theme/Tokens.cpp:73:                t.accent = QColor(u"#34D399"_s);
src/ui/theme/Tokens.cpp:74:                t.accentHover = QColor(u"#6EE7B7"_s);
src/ui/theme/Tokens.cpp:75:                t.accentPressed = QColor(u"#10B981"_s);
src/ui/theme/Tokens.cpp:78:                t.accent = QColor(u"#38BDF8"_s);
src/ui/theme/Tokens.cpp:79:                t.accentHover = QColor(u"#7DD3FC"_s);
src/ui/theme/Tokens.cpp:80:                t.accentPressed = QColor(u"#0EA5E9"_s);
src/ui/theme/Tokens.cpp:84:        t.bg = QColor(u"#F6F7FB"_s);
src/ui/theme/Tokens.cpp:85:        t.surface = QColor(u"#FFFFFF"_s);
src/ui/theme/Tokens.cpp:86:        t.surfaceAlt = QColor(u"#EEF0F6"_s);
src/ui/theme/Tokens.cpp:87:        t.border = QColor(u"#E3E6EF"_s);
src/ui/theme/Tokens.cpp:88:        t.text = QColor(u"#1F2430"_s);
src/ui/theme/Tokens.cpp:89:        t.textMuted = QColor(u"#6B7385"_s);
src/ui/theme/Tokens.cpp:90:        t.danger = QColor(u"#EF4444"_s);
src/ui/theme/Tokens.cpp:91:        t.warning = QColor(u"#F59E0B"_s);
src/ui/theme/Tokens.cpp:92:        t.success = QColor(u"#10B981"_s);
src/ui/theme/Tokens.cpp:93:        t.accentText = QColor(u"#FFFFFF"_s);
src/ui/theme/Tokens.cpp:97:                t.accent = QColor(u"#5B6CFF"_s);
src/ui/theme/Tokens.cpp:98:                t.accentHover = QColor(u"#4859EC"_s);
src/ui/theme/Tokens.cpp:99:                t.accentPressed = QColor(u"#3A4AD6"_s);
src/ui/theme/Tokens.cpp:102:                t.accent = QColor(u"#14B8A6"_s);
src/ui/theme/Tokens.cpp:103:                t.accentHover = QColor(u"#0D9488"_s);
src/ui/theme/Tokens.cpp:104:                t.accentPressed = QColor(u"#0F766E"_s);
src/ui/theme/Tokens.cpp:107:                t.accent = QColor(u"#F43F5E"_s);
src/ui/theme/Tokens.cpp:108:                t.accentHover = QColor(u"#E11D48"_s);
src/ui/theme/Tokens.cpp:109:                t.accentPressed = QColor(u"#BE123C"_s);
src/ui/theme/Tokens.cpp:112:                t.accent = QColor(u"#F59E0B"_s);
src/ui/theme/Tokens.cpp:113:                t.accentHover = QColor(u"#D97706"_s);
src/ui/theme/Tokens.cpp:114:                t.accentPressed = QColor(u"#B45309"_s);
src/ui/theme/Tokens.cpp:117:                t.accent = QColor(u"#10B981"_s);
src/ui/theme/Tokens.cpp:118:                t.accentHover = QColor(u"#059669"_s);
src/ui/theme/Tokens.cpp:119:                t.accentPressed = QColor(u"#047857"_s);
src/ui/theme/Tokens.cpp:122:                t.accent = QColor(u"#0EA5E9"_s);
src/ui/theme/Tokens.cpp:123:                t.accentHover = QColor(u"#0284C7"_s);
src/ui/theme/Tokens.cpp:124:                t.accentPressed = QColor(u"#0369A1"_s);
src/ui/theme/Tokens.cpp:178:        {u"#5B6CFF"_s, u"Indigo"_s},
src/ui/theme/Tokens.cpp:179:        {u"#F59E0B"_s, u"Amber"_s},
src/ui/theme/Tokens.cpp:180:        {u"#0D9488"_s, u"Teal"_s},
src/ui/theme/Tokens.cpp:181:        {u"#10B981"_s, u"Emerald"_s},
src/ui/theme/Tokens.cpp:182:        {u"#F43F5E"_s, u"Rose"_s},
src/ui/theme/Tokens.cpp:183:        {u"#0284C7"_s, u"Sky"_s},
src/ui/theme/Tokens.cpp:184:        {u"#8B5CF6"_s, u"Purple"_s},
src/ui/theme/Tokens.cpp:185:        {u"#F97316"_s, u"Orange"_s},
src/ui/theme/StatusStyle.cpp:11:            return QColor(u"#8A93A6"_s);
src/ui/theme/StatusStyle.cpp:13:            return QColor(u"#3B82F6"_s);
src/ui/theme/StatusStyle.cpp:15:            return QColor(u"#8B5CF6"_s);
src/ui/theme/StatusStyle.cpp:17:            return QColor(u"#F59E0B"_s);
src/ui/theme/StatusStyle.cpp:19:            return QColor(u"#F97316"_s);
src/ui/theme/StatusStyle.cpp:21:            return QColor(u"#10B981"_s);
src/ui/theme/StatusStyle.cpp:23:            return QColor(u"#059669"_s);
src/ui/theme/StatusStyle.cpp:25:            return QColor(u"#EF4444"_s);
src/ui/theme/StatusStyle.cpp:27:            return QColor(u"#64748B"_s);
src/ui/theme/StatusStyle.cpp:29:            return QColor(u"#94A3B8"_s);
src/ui/theme/StatusStyle.cpp:31:    return QColor(u"#8A93A6"_s);
src/ui/theme/StatusStyle.cpp:63:            return QColor(u"#8A93A6"_s);
src/ui/theme/StatusStyle.cpp:65:            return QColor(u"#3B82F6"_s);
src/ui/theme/StatusStyle.cpp:67:            return QColor(u"#F59E0B"_s);
src/ui/theme/StatusStyle.cpp:69:            return QColor(u"#10B981"_s);
src/ui/theme/StatusStyle.cpp:71:    return QColor(u"#8A93A6"_s);
src/ui/theme/StatusStyle.cpp:91:            return QColor(u"#10B981"_s);
src/ui/theme/StatusStyle.cpp:93:            return QColor(u"#3B82F6"_s);
src/ui/theme/StatusStyle.cpp:95:            return QColor(u"#EF4444"_s);
src/ui/theme/StatusStyle.cpp:97:    return QColor(u"#3B82F6"_s);
```
All hex colors reside exclusively within `src/ui/theme/`.

### Final Quality Check Run
```bash
CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
```
Output:
```text
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.3s)
-- Generating done (0.2s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
ninja: no work to do.
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start  1: tst_Database
 1/19 Test  #1: tst_Database .....................   Passed    0.27 sec
      Start  2: tst_TrackRepository
 2/19 Test  #2: tst_TrackRepository ..............   Passed    0.05 sec
      Start  3: tst_TopicRepository
 3/19 Test  #3: tst_TopicRepository ..............   Passed    0.20 sec
      Start  4: tst_SessionRepository
 4/19 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  5: tst_ApplicationRepository
 5/19 Test  #5: tst_ApplicationRepository ........   Passed    0.14 sec
      Start  6: tst_InterviewRepository
 6/19 Test  #6: tst_InterviewRepository ..........   Passed    0.03 sec
      Start  7: tst_ReminderLogRepository
 7/19 Test  #7: tst_ReminderLogRepository ........   Passed    0.02 sec
      Start  8: tst_SeedService
 8/19 Test  #8: tst_SeedService ..................   Passed    0.84 sec
      Start  9: tst_TransactionBehavior
 9/19 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start 10: ui_smoke
10/19 Test #10: ui_smoke .........................   Passed   53.03 sec
      Start 11: tst_MonthView
11/19 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 12: tst_AgendaListModel
12/19 Test #12: tst_AgendaListModel ..............   Passed    0.05 sec
      Start 13: tst_ApplicationTableModel
13/19 Test #13: tst_ApplicationTableModel ........   Passed    0.07 sec
      Start 14: tst_ApplicationFilterProxy
14/19 Test #14: tst_ApplicationFilterProxy .......   Passed    0.03 sec
      Start 15: tst_TopicListModel
15/19 Test #15: tst_TopicListModel ...............   Passed    0.24 sec
      Start 16: tst_TopicBoardProxy
16/19 Test #16: tst_TopicBoardProxy ..............   Passed    0.12 sec
      Start 17: tst_TopicTableProxy
17/19 Test #17: tst_TopicTableProxy ..............   Passed    0.12 sec
      Start 18: tst_AgendaService
18/19 Test #18: tst_AgendaService ................   Passed    0.10 sec
      Start 19: tst_ExportService
19/19 Test #19: tst_ExportService ................   Passed    0.02 sec

100% tests passed, 0 tests failed out of 19

Total Test time (real) =  55.50 sec
==> Quality check PASSED with zero warnings and all tests green!
```

---

## 5. SPEC §12 Milestone M4 Acceptance Walkthrough

1. **"Study page displays tracks as tabs and topic cards grouped into columns by status (Backlog, In Progress, Review, Done)."**
   - Verified. Tabs switch tracks; columns filter by topic status. Empty states render cleanly in both board and list view.
2. **"Quick-add field in the Backlog column creates a topic directly in the current track."**
   - Verified. Typing a title and pressing Enter immediately creates the topic and displays it in Backlog.
3. **"Topic cards display title, target date (with overdue highlighting), priority indicator, and progress bar."**
   - Verified. `TopicCardDelegate` renders track accent stripe, title, formatted target date (styled with danger token if overdue), colored priority badge with tooltip, and percentage progress bar.
4. **"Dragging a card between columns updates the topic status in the database."**
   - Verified. Card drag-and-drop between columns animates an insertion drop indicator and updates database status.
5. **"Selecting a card opens a slide-over or detail panel showing track, status, target date, priority, checklist, logged study time, and notes."**
   - Verified. Slide-over `TopicDetailPanel` provides complete topic inspection, checklist item completion/editing, logged time tally, and notes editing.
6. **"Editing any detail persists immediately (or on drawer close/save)."**
   - Verified. Title, notes, checklist, target date, and priority save to `TopicRepository`.
7. **"Study sessions can be logged from the header '+ New' or from the topic detail drawer."**
   - Verified. `SessionDialog` logs study minutes with topic selection (or `(No topic)`), auto-incrementing topic progress.
8. **"Calendar Agenda view lists topics with a target date for the selected day; activating a topic jumps to it in the Study view."**
   - Verified. Double-clicking or pressing Enter on a study target row in Calendar Agenda switches to Study page, selects the corresponding topic card/row, and opens its detail drawer.
9. **"Track management dialog allows adding new tracks and editing/deleting existing ones."**
   - Verified. `TrackDialog` provides complete CRUD, swatch selection, duplicate checking, and cascading deletion confirmation.
10. **"Table/list view switcher allows viewing topics in a sortable QTableView with Status, Priority, Progress, and Target Date columns."**
   - Verified. `TopicTableProxy` provides robust column sorting, disables DnD, and integrates `ProgressBarDelegate` and `StatusBadgeDelegate`.

---

## 6. Not Verified by Agent, User to Check

As an offscreen automated agent, several interactive aspects must be manually evaluated in a desktop session:
1. **Real Mouse Drag Feel:** The tactile responsiveness, cursor feedback, and visual comfort of dragging topic cards across Kanban columns at real monitor refresh rates.
2. **Display Scaling (100% to 200% DPI):** Proper high-DPI rasterization of custom-painted elements (progress bars, status badges, priority icons, swatch checkmarks) on 4K/retina displays.
3. **Light and Dark Theme Visual Contrast:** Subjective aesthetics and readability of card backgrounds, text contrast, and track color badges under both light and dark themes.
4. **Tab Order & Keyboard Traversal:** Natural tab order across complex modal workflows (especially inside `TrackDialog` swatch grid and `SessionDialog` form inputs).
5. **Calendar Click-Through Experience:** End-to-end interactive workflow of navigating months, inspecting study target markers, and clicking through from agenda to the study board.
6. **Wayland and X11 Behavior:** Window manager decorations, drawer transitions, and tooltip positioning on native Linux desktop sessions under both Wayland and X11.

---

## 7. How to Run

1. **Configure and Build:**
   ```bash
   cmake --preset dev-linux
   cmake --build --preset dev-linux
   ```
2. **Execute Full Test Suite:**
   ```bash
   ctest --preset dev-linux --output-on-failure
   # Or run check script:
   ./scripts/check.sh
   ```
3. **Regenerate Screenshots:**
   ```bash
   ./scripts/screenshots.sh
   ```
4. **Run JobPrep Desktop Application:**
   ```bash
   ./build/dev-linux/jobprep
   ```

---

## 8. Deviations and Assumptions

- **TopicTableProxy Location:** Placed in `src/models/` to preserve strict architecture boundaries (`ui -> models -> services -> data -> domain`).
- **Session Edit/Delete Deferred:** Per SPEC §14, session editing and deletion are deferred to Milestone M6.
- **Session Duration Cap:** Capped at 1440 minutes (24 hours) as single-day physical limit, validated with inline label error.
- **Calendar Marker Refresh:** `CalendarPage` consumes `TrackRepository&` to invalidate study target agenda items upon track deletion.

---

## 9. Suggested Next Step

User to perform interactive UI verification on desktop (evaluating the items in Section 6). Once satisfied, user can create the `m4` git tag, concluding Milestone M4.
