# M7 Plan: UI Bug Fixes, Qt/MSVC Compatibility, CI Hygiene, and GUI Localization (Ukrainian & Russian)

## 1. Environment & Baseline

Commands executed on the host system in this session:

### Environment Command
```bash
$ uname -a && lsb_release -a && g++ --version && qmake6 -v
```
```
Linux tony-laptop 7.0.0-42-generic #42-Ubuntu SMP PREEMPT_DYNAMIC Thu Oct 15 08:06:44 UTC 2026 x86_64 GNU/Linux
No LSB modules are available.
Distributor ID:	Ubuntu
Description:	Ubuntu 26.04.1 LTS
Release:	26.04
Codename:	resolute
g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0
Copyright (C) 2025 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

QMake version 3.1
Using Qt version 6.10.2 in /usr/lib/x86_64-linux-gnu
```

### Git Baseline
```bash
$ git log -n 5 --oneline
```
```
9fe8def (HEAD -> main, origin/main, origin/HEAD) m7 plan request
0c11c50 (tag: m6) docs(reports): add M6 fix-up 8 report
2152753 fix(test): use relative path for offscreen config to prevent Windows path splitting, disable abort dialog, and clean up
a05a89a test(hidpi): add CI-DIAG progress tracking and watchdog to tst_HiDpi
495a191 ci: add test timeout to ctest and step timeouts to quality gate
```

### Test Count Baseline Verification
```bash
$ python3 -c '
import os, subprocess
tests_dir = "build/dev-linux/tests"
binaries = []
for root, dirs, files in os.walk(tests_dir):
    for f in files:
        p = os.path.join(root, f)
        if os.access(p, os.X_OK) and not f.endswith(".sh") and not f.endswith(".so") and not "." in f:
            binaries.append(p)
binaries.sort()
total_funcs = 0
for b in binaries:
    name = os.path.basename(b)
    try:
        out = subprocess.check_output([b, "-functions"], stderr=subprocess.STDOUT, text=True, env=dict(os.environ, QT_QPA_PLATFORM="offscreen"))
        lines = [l.strip() for l in out.splitlines() if l.strip() and not l.startswith("*") and not l.startswith("Config:")]
        funcs = [l for l in lines if "(" in l and not l.startswith("initTestCase") and not l.startswith("cleanupTestCase") and not l.startswith("init()") and not l.startswith("cleanup()")]
        print(f"{name}: {len(funcs)}")
        total_funcs += len(funcs)
    except Exception as e:
        print(f"Error {name}: {e}")
print(f"Total test binaries: {len(binaries)}")
print(f"Total test functions: {total_funcs}")
'
```
```
tst_ApplicationRepository: 20
tst_Database: 16
tst_InterviewRepository: 9
tst_ReminderLogRepository: 7
tst_SeedService: 5
tst_SessionRepository: 10
tst_TopicRepository: 18
tst_TrackRepository: 8
tst_TransactionBehavior: 8
tst_AgendaListModel: 5
tst_ApplicationFilterProxy: 2
tst_ApplicationTableModel: 10
tst_TopicBoardProxy: 9
tst_TopicListModel: 7
tst_TopicTableProxy: 6
tst_AgendaService: 24
tst_DayWatcher: 4
tst_ExportService: 4
tst_ImportExport: 33
tst_ReminderService: 18
tst_SettingsService: 8
tst_StatsService: 10
tst_EmptyStatesAndFocus: 34
tst_HiDpi: 5
tst_Notifiers: 21
tst_PageCalendar: 12
tst_PageHome: 16
tst_PageJobs: 22
tst_PageSettings: 44
tst_PageStudy: 26
tst_Shortcuts: 12
tst_StudyRegression: 12
tst_MonthView: 13
ui_smoke: 11
Total test binaries: 34
Total test functions: 469
```

Baseline matches `docs/prompts/M7-plan-request.md` line 15: **34 test binaries, 469 test functions**.

---

## 2. Pre-Flight Findings

| Target / Item | Finding / Location | Evidence Command |
|---|---|---|
| Agenda row delegate | `JobPrep::Ui::Delegates::AgendaItemDelegate` in `src/ui/delegates/AgendaItemDelegate.h`, `.cpp` | `grep -n "m_todayDelegate" src/ui/pages/HomePage.cpp` |
| Where HomePage builds "Today" | `HomePage::setupUi()` lines 120–150; `HomePage::applySnapshot()` line 261 | `sed -n '120,150p' src/ui/pages/HomePage.cpp` |
| All Date Controls | 5 occurrences, all instantiated as `SingleTabDateEdit` (`TopicDetailPanel`, `InterviewDialog`, `ApplicationDialog` [x2], `SessionDialog`) | `grep -rn "setCalendarPopup" src/` |
| `invalidateFilter` usage | 4 occurrences (`ApplicationFilterProxy.cpp:21, 26`, `TopicBoardProxy.cpp:26, 33`) | `grep -rn "invalidateFilter" src/` |
| Deprecation warning suppression | Suppressed only for `__GNUC__`/`__clang__` at lines 1–4 of `ApplicationFilterProxy.cpp` & `TopicBoardProxy.cpp` | `head -n 5 src/models/ApplicationFilterProxy.cpp` |
| CI timeouts | `timeout-minutes: 15` (Linux), `timeout-minutes: 30` (Windows), `ctest --timeout 600` | `grep -n "timeout" .github/workflows/ci.yml scripts/check.sh` |
| `CI-DIAG` instrumentation | Watchdog thread (240s) + progress markers present in `tests/ui/tst_HiDpi.cpp` | `grep -n "CI-DIAG" tests/ui/tst_HiDpi.cpp` |
| `tr()` count | 482 calls across 27 files in `src/` | `grep -rn "tr(" src/ \| wc -l` |
| Stored enum persistence | Stable lowercase snake_case strings in `src/domain/EnumStrings.cpp` | `cat src/domain/EnumStrings.h` |
| Current `QLocale` / `QTranslator` usage | `MonthView.cpp:26` hard-codes `QLocale(QLocale::English)`; `QTranslator` count is 0 | `grep -rn "QLocale\|QTranslator" src/` |
| SettingsService keys | 21 keys; no language key exists | `grep -o 'u"[^"]*"_s' src/services/SettingsService.cpp \| sort \| uniq` |
| Resources layout | `resources.qrc` with `styles/theme.qss`, `icons/`, `seed/topics.json`, `fonts/` | `cat resources/resources.qrc` |
| Qt Linguist in CMakeLists.txt | `find_package(Qt6 ...)` does not request `LinguistTools` | `grep -n -C 5 "find_package" CMakeLists.txt` |
| Linguist tools on dev machine | `Qt6LinguistTools` cmake package and `/usr/lib/qt6/bin/lrelease`, `lupdate` present | `find /usr/lib/qt6/bin -name "lrelease*"` |
| Font Cyrillic support | `DejaVu Sans`, `Liberation Sans`, `Ubuntu` all support `:lang=uk:lang=ru` | `fc-list :lang=uk:lang=ru family \| sort -u` |

---

## 3. Item 1: Dashboard "Today" Section Text Overlap & Header Fix

### 3.1 Investigation & Image Verification
- Checked for `docs/reports/images/m7-today-overlap.png`: file does not exist (`ls: cannot access 'docs/reports/images/m7-today-overlap.png': No such file or directory`).
- As instructed, reproduced offscreen using a scratch harness (`scratch/reproduce_today.cpp`) with an in-memory DB containing a topic with target date today, track "C++", and title "Advanced Metaprogramming with Concepts, Fold Expressions and SFINAE".
- Screenshots generated:
  - `scratch/today_view.png` (7.1 KB)
  - `scratch/today_dashboard.png` (72 KB)

### 3.2 Root Cause Analysis (Delegate Paint Math)
In `src/ui/delegates/AgendaItemDelegate.cpp` lines 73–95:
```cpp
const QRect textRect = content.adjusted(kIconSize + 8, 3, 0, -3);
...
painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::AlignAbsolute,
                  titleMetrics.elidedText(title, Qt::ElideRight, titleWidth));
...
if (!subtitle.isEmpty()) {
    painter->setFont(subtitleFont);
    painter->setPen(mutedColor);
    const QFontMetrics subtitleMetrics(subtitleFont);
    painter->drawText(textRect.adjusted(0, titleMetrics.height() / 2 + 2, 0, 0),
                      Qt::AlignLeft | Qt::AlignVCenter | Qt::AlignAbsolute,
                      subtitleMetrics.elidedText(subtitle, Qt::ElideRight, textRect.width()));
}
```
**Exact Geometric Computation from Scratch Execution Output:**
```
todayView geometry: 0,0 376x188
itemRect: 0,0 376x44
textRect: 34,3 332x38
titleMetrics height: 18
subtitleMetrics height: 17
Title vertical bounds: [13 .. 31]
Subtitle vertical bounds: [19 .. 36]
OVERLAP: 12 px
```
- When `subtitle` is non-empty (e.g. "C++"), `title` is vertically centered in `textRect` (y = 13 to 31).
- `subtitle` is drawn in `textRect.adjusted(0, titleMetrics.height() / 2 + 2, 0, 0)` with `Qt::AlignVCenter`. Its vertical center lands at y = 19 to 36.
- Both texts begin at the identical horizontal position (`x = 34`).
- **Mathematical overlap is 12 px** out of an 18 px font height. The category text is rendered directly on top of the title text.
- When `subtitle` is empty, single-line centering works, but the two-line path failed to partition `textRect` into upper and lower bands.

### 3.3 Section Header Issues
In `src/ui/pages/HomePage.cpp` lines 48–66 (`HomePage::createCard`):
```cpp
QFrame* HomePage::createCard(const QString& title, QWidget* content) {
    auto* card = new QFrame(this);
    card->setProperty("card", true);
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 14, 16, 14);
    cardLayout->setSpacing(12);
    auto* titleLabel = new QLabel(title, card);
    QFont titleFont = font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    ...
```
**Identified Defects:**
1. `titleLabel` has no `objectName` (e.g. `CardTitle`) and no stylesheet typography rules in `resources/styles/theme.qss`. Unlike `#CalendarAgendaTitle` which explicitly sets `font-size: 11pt; font-weight: 600; color: @text;`, `createCard` leaves title styling unmanaged.
2. `m_todayView` in `HomePage.cpp` lacks `setFrameShape(QFrame::NoFrame)` and `background: transparent; border: none;`, unlike `#CalendarAgendaList` in `CalendarPage.cpp`, creating visual box-in-box artifacts.
3. Left-alignment mismatch: `titleLabel` sits at `x = 16px` (card margin), while `AgendaItemDelegate` item content starts at `x = 26px` (16 card + 10 item margin) with text starting at `x = 50px`.

### 3.4 Why `tst_HiDpi::checkClipping` Did Not Catch It
1. `checkClipping(QWidget* root)` inspects only `root->findChildren<QWidget*>()`. `QStyledItemDelegate` is not a child widget; its painting is internal to `QListView::paintEvent`.
2. `checkClipping` only tests whether a widget's allocated width/height is smaller than its `minimumSizeHint()` or `heightForWidth()`. It has **no logic to detect bounding-box collisions between sibling widgets or painted text rectangles within delegates**.

### 3.5 Proposed Solution & New Overlap Verification Check
1. **Delegate Layout Fix:**
   - In `AgendaItemDelegate::paint`:
     - If `subtitle.isEmpty()`: draw `title` centered vertically in `textRect`.
     - If `subtitle` is present: calculate distinct non-overlapping rects:
       `titleRect = QRect(textRect.left(), textRect.top(), titleWidth, titleMetrics.height());`
       `subtitleRect = QRect(textRect.left(), titleRect.bottom() + 2, textRect.width() - rightWidth, subtitleMetrics.height());`
   - Adjust `sizeHint()` dynamically to ensure `kRowHeight` accommodates `titleMetrics.height() + subtitleMetrics.height() + 8` across 100% and 150% scaling.
2. **Card Header Styling:**
   - Assign `titleLabel->setObjectName(u"CardTitle"_s)`.
   - Add `#CardTitle` and `#TodayView` rules to `theme.qss` (`font-size: 11pt; font-weight: 600; color: @text; border: none; background: transparent;`).
   - Call `m_todayView->setFrameShape(QFrame::NoFrame);`.
3. **Automated Overlap Test in `tst_HiDpi`:**
   - Add a test function `testDelegateAndWidgetTextOverlap()`:
     - Populates `HomePage` and `CalendarPage` with multi-line items (seeded track + topic target date).
     - Renders delegate cells at 100% and 150% display scaling across Light and Dark themes under DejaVu Sans / Segoe UI.
     - Calculates bounding rects of title, subtitle, and badge/time text; asserts that `QRect::intersects()` between title rect and subtitle rect is strictly `false`.
     - Traverses sibling child widgets in card layouts and dialog form rows, asserting that no two visible sibling labels/buttons intersect in parent coordinates.

---

## 4. Item 2: Date Fields Keyboard Navigation (`Alt+Down` / `F4`)

### 4.1 Full Inventory of Date Controls in `src/`

Command: `grep -rn "setCalendarPopup" src/`
```
src/ui/widgets/TopicDetailPanel.cpp:145:    m_dateEdit->setCalendarPopup(true);
src/ui/dialogs/InterviewDialog.cpp:100:    m_dateEdit->setCalendarPopup(true);
src/ui/dialogs/ApplicationDialog.cpp:231:    m_appliedDateEdit->setCalendarPopup(true);
src/ui/dialogs/ApplicationDialog.cpp:251:    m_nextActionDateEdit->setCalendarPopup(true);
src/ui/dialogs/SessionDialog.cpp:51:    m_dateEdit->setCalendarPopup(true);
```
All 5 controls are instances of `JobPrep::Ui::Widgets::SingleTabDateEdit` (defined in `src/ui/widgets/SingleTabDateTime.h`).

### 4.2 Qt Keyboard Behavior (Proven vs Not Proven)
- **Proven by test in scratch harness (`scratch/test_calendar_popup.cpp`):**
  - Sending `Key_F4`, `Alt+Key_Down`, or `Key_Space` to a focused `QDateEdit` with `calendarPopup = true` does **not** open the calendar popup on Linux or Windows offscreen (`isVisible` remains 0).
  - Simulating a mouse press on the drop-down arrow subcontrol (`QStyle::SC_SpinBoxDown`) opens `QCalendarPopup` (`isVisible: 1`).
  - Pressing `Key_Escape` dismisses the open popup (`isVisible: 0`).
  - Navigating dates and pressing Enter selects the date and updates `QDateEdit::date()`.
- **Not Proven on Windows Hardware by Agent:**
  - Whether native Windows Narrator or specific IME configurations hook `Alt+Down` before Qt receives the key event.

### 4.3 Unified Solution: `SingleTabDateEdit`
Because every date field in JobPrep uses `SingleTabDateEdit`, we implement the fix in one central class:
1. In `src/ui/widgets/SingleTabDateTime.h` (or extracted `.cpp` if logic exceeds inline):
   - Override `keyPressEvent(QKeyEvent* event)`:
     ```cpp
     if ((event->modifiers() == Qt::AltModifier && event->key() == Qt::Key_Down) ||
         (event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_F4)) {
         showCalendarPopup();
         event->accept();
         return;
     }
     QDateEdit::keyPressEvent(event);
     ```
   - Implement `showCalendarPopup()`:
     Calculates `subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxDown, this)` and posts the synthesized mouse press event to open `QCalendarPopup` safely through Qt's internal machinery.
2. In the constructor of `SingleTabDateEdit`:
   - Set or append tooltip: `tr("Press Alt+Down or F4 to open calendar")`.
3. Tab Order:
   - `SingleTabDateEdit` already handles single-tab focus transition via `focusNextPrevChild()`. Tab lands on the date field, `Alt+Down` opens the popup, calendar keyboard navigation selects a date, Enter commits and closes, Esc cancels.

### 4.4 Real Key-Event Tests
- Create test slots in `tests/ui/` covering all 4 dialogs/panels (`InterviewDialog`, `ApplicationDialog`, `SessionDialog`, `TopicDetailPanel`):
  1. Show dialog/panel, set focus to the date field.
  2. Send `Alt+Down` key event -> assert `calendarWidget()->window()->isVisible()` is `true`.
  3. Send `Esc` -> assert `calendarWidget()->window()->isVisible()` is `false`.
  4. Send `F4` -> assert popup is visible again.
  5. Send arrow keys + Enter -> assert date updates to selected date and popup closes.
- **Offscreen Limitation Notice:** The offscreen platform verifies Qt event dispatch, window visibility flags, and date synchronization, but cannot verify OS window manager grab or visual compositing on Windows.

---

## 5. Item 3: Qt 6.10 Compatibility & CI Matrix

### 5.1 Root Cause of Linux vs MSVC Discrepancy
- In `src/models/ApplicationFilterProxy.cpp` (lines 1–4) and `src/models/TopicBoardProxy.cpp` (lines 1–4):
  ```cpp
  #if defined(__GNUC__) || defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  #endif
  ```
- **Proof:** Linux GCC/Clang built with `-Werror` because warnings were explicitly suppressed via GCC-specific pragmas. MSVC on Windows does not recognize `#pragma GCC`, was not suppressed, issued warning C4996 for `invalidateFilter()`, and `/WX` turned it into a fatal build error.
- Suppressing warnings via pragmas violates `AGENTS.md` §3 ("Fix warnings, never disable them").

### 5.2 Replacement with Modern Qt 6.9+ Filter APIs
- In Qt 6.9+, `QSortFilterProxyModel` introduced `beginFilterChange()` and `endFilterChange(Directions directions = Direction::Both)`. `invalidateFilter()` is deprecated starting in Qt 6.10 / 6.13.
- Both methods are `protected` in `QSortFilterProxyModel` (verified in `/usr/include/x86_64-linux-gnu/qt6/QtCore/qsortfilterproxymodel.h:124-125`).
- Fix:
  Remove the diagnostic pragmas entirely. Replace calls in `ApplicationFilterProxy` and `TopicBoardProxy`:
  ```cpp
  #if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
      beginFilterChange();
      m_chip = chip;
      endFilterChange(QSortFilterProxyModel::Direction::Rows);
  #else
      m_chip = chip;
      QSortFilterProxyModel::invalidateFilter();
  #endif
  ```
- No other deprecated Qt 6.9/6.10 APIs exist in `src/` (verified via `grep -rn "invalidateFilter" src/`).

### 5.3 CI Matrix Proposals

Command: `python3 -m aqt list-qt linux desktop`
Newest Qt 6.10.x version available: **Qt 6.10.3**.
Architectures: `linux_gcc_64` (Linux) and `win64_msvc2022_64` (Windows).

**Comparison of CI Matrix Options:**
1. **Option A (Full 2x2 Matrix, 4 jobs):**
   - Ubuntu: Qt 6.8.2 & Qt 6.10.3
   - Windows: Qt 6.8.2 & Qt 6.10.3
   - *Cost:* Adds ~12–14 minutes of Windows runner time and ~3.5 minutes of Linux runner time per CI push. Windows build + debug test is relatively slow (~10–12 min).
2. **Option B (Recommended 3-job Hybrid, 3 jobs):**
   - Ubuntu: Qt 6.8.2 (LTS baseline)
   - Ubuntu: Qt 6.10.3 (Modern GCC + newest Qt)
   - Windows: Qt 6.10.3 (Ensures MSVC compatibility with newest Qt and catches ICEs/C4996 warnings)
   - *Cost:* Adds only ~3.5 minutes of fast Linux runner time. Maintains Windows coverage on the latest compiler toolset while protecting Qt 6.8 LTS compatibility on Linux.
3. **Option C (2-job Divergent):**
   - Ubuntu: Qt 6.8.2
   - Windows: Qt 6.10.3
   - *Cost:* 0 added CI minutes, but drops Windows verification of Qt 6.8.2.

**Recommendation:** **Option B** (or Option A if the user accepts the additional 12 min Windows run time).

### 5.4 `docs/RUNNING.md` Windows Section Updates
- Add explicit warning: **Visual Studio 2022 must be updated (version 17.4+ / MSVC 19.34+ required)**; initial VS 2022 release (MSVC 19.30) causes an Internal Compiler Error (C1001) in `QtCore/qrangemodel_impl.h(903)` when compiling with Qt 6.10+.
- Document tested command steps:
  1. Open **x64 Native Tools Command Prompt for VS 2022**.
  2. `set QT_PREFIX_PATH=C:\Qt\6.10.3\msvc2022_64` (or `6.8.2`)
  3. `set PATH=%QT_PREFIX_PATH%\bin;%PATH%`
  4. `cmake --preset dev-windows`
  5. `cmake --build --preset dev-windows`
  6. `ctest --preset dev-windows --output-on-failure`
  7. Run: `.\build\dev-windows\jobprep.exe` (reported working by user on his PC).

---

## 6. Item 4: CI Hygiene & Windows Test Performance

### 6.1 Audit of Existing Safeguards
- `git log --oneline`: Confirmed commits `495a191`, `a05a89a`, `2152753`, and `0c11c50` implemented the hang addendum.
- `.github/workflows/ci.yml`: Contains `timeout-minutes: 15` (Linux) and `timeout-minutes: 30` (Windows).
- `scripts/check.sh` and `scripts/check.ps1`: Both invoke ctest with `--timeout 600 --output-on-failure`.
- `tests/ui/tst_HiDpi.cpp`: Contains `s_watchdogThread` (240s timeout) and `updateProgress()` instrumentation marked `// CI-DIAG`.

### 6.2 Proposals for Slow Windows UI Tests
Windows MSVC Debug test times:
- `tst_PageStudy`: 138 s
- `tst_PageHome`: 124 s
- `tst_PageJobs`: 86 s
- `tst_EmptyStatesAndFocus`: 66 s

**Root Cause:**
Each test function in these suites repeatedly executes `resetStudyData()`, which creates a fresh in-memory database, runs all 4 SQLite DDL migrations, initializes `AppContext`, parses full QSS stylesheets, and renders widgets with MSVC `/MDd` unoptimized iterator debugging.

**Proposals:**
1. **Windows CI Test Build Preset (`RelWithDebInfo`):**
   - In MSVC, running tests under `/O2` / `/Ox` eliminates iterator debug overhead, speeding up QSS parsing and SQLite queries by ~4x to 8x. Expected Windows test suite reduction: from ~8 minutes down to ~2 minutes.
2. **Database Schema Pre-population:**
   - In test fixtures, initialize the SQLite schema once per test case into a template database and clone via in-memory backup API (`sqlite3_backup`) or rollback transaction rather than executing DDL migrations 26 times.
3. **Audit and Minimize `qWait(50)` Delays:**
   - Replace arbitrary `qWait(50)` sleep loops in `tst_PageStudy` with deterministic `QTRY_VERIFY` condition polling.

---

## 7. Item 5: GUI Translation (Ukrainian `uk` & Russian `ru`)

### 7.1 Inventory of User-Visible Strings
Total `tr()` calls in `src/`: **482** across 27 files.

| File / Component | `tr()` Count | Untranslated Literals Found | Description |
|---|---|---|---|
| `src/ui/dialogs/ApplicationDialog.cpp` | 62 | `u"https://"_s` (URL placeholder) | Form labels, tooltips, tab titles |
| `src/ui/pages/SettingsPage.cpp` | 59 | 0 | Theme, goal, export/import options |
| `src/ui/DataToolsController.cpp` | 54 | 0 | Data messages, confirmation dialogs |
| `src/ui/widgets/TopicDetailPanel.cpp` | 35 | 0 | Checklist actions, date picker text |
| `src/ui/dialogs/InterviewDialog.cpp` | 34 | 0 | Field labels, validation tooltips |
| `src/ui/pages/JobsPage.cpp` | 31 | 0 | Columns, filter chips, action buttons |
| `src/ui/pages/CalendarPage.cpp` | 28 | `u"ddd "_s`, `u"HH:mm"_s` (date format) | Month nav, layer toggles, agenda titles |
| `src/ui/pages/StudyPage.cpp` | 27 | 0 | Board columns, track selector |
| `src/ui/dialogs/TrackDialog.cpp` | 22 | 0 | Color swatches, track deletion warning |
| `src/ui/pages/HomePage.cpp` | 20 | 0 | Stat cards, card headers, empty states |
| `src/ui/MainWindow.cpp` | 17 | `u"JobPrep"_s` (app title) | Sidebar items, header action menu |
| `src/ui/dialogs/FollowUpDialog.cpp` | 15 | 0 | Follow-up prompts, dates |
| `src/ui/FormatHelper.cpp` | 15 | 0 | Relative dates, duration formatting |
| `src/ui/dialogs/SessionDialog.cpp` | 14 | 0 | Duration spinbox, date picker |
| `src/ui/Sidebar.cpp` | 8 | 0 | Navigation tooltips & button text |
| `src/models/ApplicationTableModel.cpp` | 8 | 0 | Table header names |
| `src/ui/widgets/TopicBoardColumn.cpp` | 7 | 0 | Column headers, quick-add |
| `src/models/TopicListModel.cpp` | 7 | 0 | Topic list table headers |
| `src/main.cpp` | 6 | 0 | CLI errors and fatal dialogs |
| `src/ui/widgets/GoalProgressBar.cpp` | 3 | 0 | Goal status summary |
| `src/ui/theme/StatusStyle.cpp` | 0 (uses `translate()`) | 0 | 28 status/outcome/priority labels |
| `src/ui/theme/AgendaKindStyle.cpp` | 0 (uses `translate()`) | 0 | 3 agenda layer labels |
| `src/ui/NotificationText.cpp` | 0 (uses `translate()`) | 0 | Lead time and notification messages |
| `src/ui/widgets/MonthView.cpp` | 0 | `englishLocale()` hard-coded | Day names, month header |
| Total | 482 | ~5 non-translatable tokens | |

### 7.2 Stored Values vs Display Text
- Enums are converted to/from lowercase snake_case strings (`src/domain/EnumStrings.cpp`).
- Display text is mapped exclusively in `ui/theme/StatusStyle.cpp` and `ui/theme/AgendaKindStyle.cpp`.
- **Export & Storage Policy:**
  - SQLite tables store snake_case strings and ISO-8601 timestamps (`yyyy-MM-dd` / `yyyy-MM-ddTHH:mm:ss`).
  - JSON backups store language-neutral keys and values.
  - CSV exports keep the 22 standardized English column headers and snake_case values (per SPEC §3.3 A1).
  - ICS exports keep standard VCALENDAR English property names.
  - Backup/restore across language changes is 100% binary- and data-identical.

### 7.3 Tooling & Build Integration
- Add `LinguistTools` to `find_package(Qt6 ...)` in `CMakeLists.txt`.
- Translations source files placed in `resources/translations/jobprep_uk.ts` and `resources/translations/jobprep_ru.ts`.
- Use `qt_add_translations(jobprep_core TS_FILES ... RESOURCE_PREFIX "/i18n")` to automatically compile `.ts` into `.qm` and embed them into the binary resource system.
- Loading at runtime via `":/i18n/jobprep_uk.qm"` avoids all pathing/installer issues on Windows and Linux.
- Add CI check: `lupdate -no-obsolete -check-translation` ensuring zero unfinished or missing translations.

### 7.4 Plurals and Formatting
- Ukrainian and Russian have three plural forms (e.g. 1 день, 2 дні, 5 днів / 1 день, 2 дня, 5 дней).
- Convert all formatted counts to use Qt's `%n` plural support:
  - `tr("%n minute(s)", "", count)`
  - `tr("%n interview(s)", "", count)`
  - `tr("%n follow-up(s) overdue", "", count)`
- Replace `englishLocale()` in `src/ui/widgets/MonthView.cpp` with active `QLocale()`.

### 7.5 Language Selection: Runtime vs Restart
- **Comparison:**
  - *Runtime Switching:* Requires `changeEvent(LanguageChange)` and `retranslateUi()` in ~40 classes, dynamic header refreshes, combobox re-population, and layout invalidation. High risk of visual artifacts.
  - *Restart-based (Recommended):* Setting stored in `SettingsService` under `ui/language` (`system`, `en`, `uk`, `ru`). Selecting a language displays an inline message: *"Language will take effect after restarting JobPrep."* On application start, `main.cpp` installs the chosen `QTranslator` into `QCoreApplication` before creating `MainWindow`. Clean, standard desktop behavior, touching ~5 files.

### 7.6 Layout & Font Cyrillic Verification
- Cyrillic strings are typically 20% to 40% longer than English.
- Fonts verified on host: `DejaVu Sans`, `Liberation Sans`, and `Ubuntu` contain full Cyrillic glyphs. Windows fonts (`Segoe UI`, `Arial`) support Cyrillic.
- Test harness: `tst_HiDpi` will run with `uk` and `ru` active, verifying zero clipping violations across 100% and 150% scales.

### 7.7 Keyboard Shortcuts & Mnemonics
- `QKeySequence` shortcuts (`Ctrl+1..4`, `Ctrl+N`, `Ctrl+F`, `Ctrl+S`, `Esc`) match by physical key scan code / primary Latin key in Qt on Windows and Linux, continuing to work under Cyrillic layouts.
- Menu and button mnemonics (`&`) in Ukrainian/Russian will use Latin accelerator hints (e.g. `Новий (&N)`) to avoid layout-dependent Alt-key collisions.

### 7.8 Sample Data & Notifications
- Sample data in `SeedService` remains in English (technical interview topics such as "C++ Smart Pointers", "Move Semantics" are universal programming terms).
- Notification formatting in `src/ui/NotificationText.cpp` is localized with `%n` plurals.

### 7.9 Translation Workflow & Glossary
- Maintain glossary in `docs/i18n-glossary.md` defining core domain terminology:
  - Application: uk `Заявка` / ru `Отклик`
  - Interview: uk `Співбесіда` / ru `Собеседование`
  - Follow-up: uk `Нагадування` / ru `Напоминание`
  - Study Target: uk `Ціль навчання` / ru `Цель обучения`
  - Streak: uk `Серія днів` / ru `Серия дней`
- Translator review file: `docs/reports/i18n-review.md` presenting English source alongside Ukrainian and Russian translations in a clean table.

---

## 8. Proposed SPEC §12 & §14 Text

### SPEC §12: Milestone M7 Entry
```markdown
**M7 — Localization, Compatibility & UI Quality.**
- GUI localization: Ukrainian (uk) and Russian (ru), with English remaining the source language. Language selection in Settings (System default / English / Українська / Русский), stored in SettingsService; language takes effect on application startup. Plural forms (%n) handled for all counters. MonthView day and month names follow the active locale. Stored database values and CSV/JSON/ICS exports remain language-neutral.
- Agenda item layout: Fixed text overlap between title and track category label in two-line agenda rows; polished card header styling and spacing.
- Date controls keyboard accessibility: Alt+Down and F4 open the calendar popup across all date fields via SingleTabDateEdit; Esc closes the popup; tooltip shortcut hint added.
- Qt 6.10 & MSVC compatibility: Replaced deprecated QSortFilterProxyModel::invalidateFilter() with beginFilterChange()/endFilterChange() guarded by QT_VERSION_CHECK; eliminated compiler warning suppressions; updated CI matrix to include Qt 6.10; updated RUNNING.md for MSVC toolset requirements.
- Acceptance: Switching language in Settings and restarting displays all UI pages, dialogs, badges, and notifications in Ukrainian or Russian; zero text clipping or delegate overlap at 100% and 150% scaling in both themes; date popups open via keyboard in all dialogs; check.sh passes on both Qt 6.8 and Qt 6.10.
```

### SPEC §14: Changelog Additions
```markdown
- 1.24 — Agenda row two-line delegate layout and card header styling (M7):
  Separated title and subtitle drawing rectangles in AgendaItemDelegate to prevent vertical text collisions when category subtitle is present; dynamic delegate row height based on font metrics; assigned CardTitle object name and unified transparent background styling for dashboard agenda lists.
- 1.25 — Date control keyboard popup shortcut (M7):
  SingleTabDateEdit handles Alt+Down and F4 to invoke the calendar popup without requiring mouse clicks; Esc cancels; tooltip displays keyboard shortcut hint.
- 1.26 — Qt 6.9+ QSortFilterProxyModel filter change modernization (M7):
  ApplicationFilterProxy and TopicBoardProxy use beginFilterChange() / endFilterChange(Rows) on Qt >= 6.9 and invalidateFilter() on Qt 6.8; eliminated local #pragma GCC diagnostic ignored blocks; updated RUNNING.md requiring Visual Studio 2022 version 17.4+ to prevent Qt 6.10 compiler ICE.
- 1.27 — Multi-language GUI localization (Ukrainian and Russian) (M7):
  Integrated Qt Linguist (.ts/.qm) via Qt resource system (:/i18n/); ui/language setting added to SettingsService; language applied at startup with QTranslator; plural rules (%n) implemented for hours, minutes, and counters; exports and database storage retain language-neutral keys.
```

---

## 9. Project Plan & Parts

### Part A: CI Hygiene, Compiler Compatibility & Docs
- **Scope:** Item 3 (Qt 6.10 compatibility), Item 4 (CI hygiene verification), and `docs/RUNNING.md`.
- **Files Touched:**
  - `src/models/ApplicationFilterProxy.cpp`
  - `src/models/TopicBoardProxy.cpp`
  - `.github/workflows/ci.yml`
  - `docs/RUNNING.md`
- **Actions:**
  1. Replace `invalidateFilter()` with `beginFilterChange()` / `endFilterChange(Rows)` guarded by `QT_VERSION_CHECK(6, 9, 0)`.
  2. Remove `#pragma GCC diagnostic ignored "-Wdeprecated-declarations"` blocks.
  3. Update `.github/workflows/ci.yml` to include Qt 6.10.3 in the CI test matrix.
  4. Update `docs/RUNNING.md` with Visual Studio 2022 toolset requirements and tested command lines.
- **Verification:** `./scripts/check.sh` passes with zero warnings.

### Part B: UI Bug Fixes (Today Overlap & Date Keyboard Access)
- **Scope:** Item 1 (Agenda text overlap & card headers) and Item 2 (Date picker keyboard navigation).
- **Files Touched:**
  - `src/ui/delegates/AgendaItemDelegate.cpp`
  - `src/ui/delegates/AgendaItemDelegate.h`
  - `src/ui/pages/HomePage.cpp`
  - `src/ui/widgets/SingleTabDateTime.h`
  - `resources/styles/theme.qss`
  - `tests/ui/tst_HiDpi.cpp`
  - `tests/ui/tst_Shortcuts.cpp` (or dialog tests)
- **Actions:**
  1. Fix `AgendaItemDelegate::paint` rectangle division and dynamic `sizeHint()`.
  2. Style card headers with `#CardTitle` and clean `HomePage` list view frame.
  3. Add `Alt+Down` and `F4` key event handling and tooltip hints in `SingleTabDateEdit`.
  4. Add automated delegate text rectangle overlap test in `tst_HiDpi`.
  5. Add keyboard popup activation tests across all date dialogs.
- **Verification:** `./scripts/check.sh` passes; all new tests green; zero overlap detected.

### Part C: Localization Infrastructure & String Audit
- **Scope:** Item 5 (Infrastructure, strings, pluralization, locale).
- **Files Touched:**
  - `CMakeLists.txt`
  - `resources/resources.qrc`
  - `src/app/AppContext.h`, `.cpp`
  - `src/services/SettingsService.h`, `.cpp`
  - `src/ui/pages/SettingsPage.h`, `.cpp`
  - `src/ui/widgets/MonthView.cpp`
  - `src/ui/NotificationText.cpp`
  - `src/ui/FormatHelper.cpp`
  - `src/main.cpp`
  - `tests/services/tst_SettingsService.cpp`
- **Actions:**
  1. Configure `find_package(Qt6 ... LinguistTools)` and `qt_add_translations`.
  2. Add `ui/language` key (`system`, `en`, `uk`, `ru`) in `SettingsService`.
  3. Add language selection combobox and restart notice in `SettingsPage`.
  4. Audit untranslated strings; add `tr()` and convert plural formats to `%n`.
  5. Update `MonthView` to use active `QLocale`.
  6. Install `QTranslator` in `main.cpp` based on settings.
- **Verification:** `./scripts/check.sh` green; English UI operates unchanged.

### Part D: Translation Delivery, Review & Verification
- **Scope:** Ukrainian and Russian translations, glossary, review documentation, HiDPI verification.
- **Files Touched:**
  - `resources/translations/jobprep_uk.ts`
  - `resources/translations/jobprep_ru.ts`
  - `docs/i18n-glossary.md`
  - `docs/reports/i18n-review.md`
  - `tests/ui/tst_HiDpi.cpp`
  - `docs/reports/M7.md`
- **Actions:**
  1. Create `docs/i18n-glossary.md`.
  2. Populate `jobprep_uk.ts` and `jobprep_ru.ts` with complete, polished translations.
  3. Generate `docs/reports/i18n-review.md` table for user review.
  4. Verify complete translation status in CI / automated test.
  5. Run `tst_HiDpi` under `uk` and `ru` locales; verify zero clipping at 100% and 150%.
  6. Compile final M7 report.
- **Verification:** All tests green; zero unfinished strings; zero clipping.

---

## 10. Test Inventory & Expected Baselines

### New & Extended Test Slots

| Test Binary | Target Slot | Type | Verification Scope |
|---|---|---|---|
| `tst_HiDpi` | `testDelegateAndWidgetTextOverlap` | Existing Binary | Renders agenda rows; asserts non-overlap of title and category rects at 100% & 150% scales |
| `tst_HiDpi` | `testTranslationsHiDpi` | Existing Binary | Validates all pages and dialogs in `uk` and `ru` at 100% & 150% scales for zero text clipping |
| `tst_Shortcuts` | `testDateEditKeyboardPopup` | Existing Binary | Sends `Alt+Down` and `F4` to `SingleTabDateEdit`; verifies popup opens, date selects, Esc closes |
| `tst_PageJobs` | `testApplicationDialogDatePopupKeys` | Existing Binary | Tests keyboard popup on Applied Date and Next Action Date in `ApplicationDialog` |
| `tst_PageCalendar` | `testInterviewDialogDatePopupKeys` | Existing Binary | Tests keyboard popup on Date in `InterviewDialog` |
| `tst_SettingsService` | `testLanguageSetting` | Existing Binary | Verifies `language()` getter/setter persistence and signal emission |
| `tst_Translations` | `testAllTranslationsFinished` | **New Binary** | Verifies `jobprep_uk.ts` and `jobprep_ru.ts` have 0 untranslated or obsolete entries |
| `tst_Translations` | `testPluralForms` | **New Binary** | Verifies 1, 2, 5 plural outputs in Ukrainian and Russian |
| `ui_smoke` | `testLanguageSmoke` | Existing Binary | Loads `MainWindow` under Ukrainian and Russian locales without Qt warnings |

### Expected Baseline Projection
- **Current Baseline:** 34 test binaries, 469 test functions.
- **Projected New Baseline:** 35 test binaries (+1 for `tst_Translations`), ~478 test functions (+9 new functions).

---

## 11. Risks & Files Touched

### Files Touched (Complete List)
- **CMake & Build:** `CMakeLists.txt`, `.github/workflows/ci.yml`
- **Resources & Styles:** `resources/resources.qrc`, `resources/styles/theme.qss`, `resources/translations/jobprep_uk.ts`, `resources/translations/jobprep_ru.ts`
- **Models & Proxies:** `src/models/ApplicationFilterProxy.cpp`, `src/models/TopicBoardProxy.cpp`, `src/models/ApplicationTableModel.cpp`, `src/models/TopicListModel.cpp`
- **Services:** `src/services/SettingsService.h`, `src/services/SettingsService.cpp`
- **UI Pages & Shell:** `src/main.cpp`, `src/app/AppContext.h`, `src/app/AppContext.cpp`, `src/ui/MainWindow.cpp`, `src/ui/Sidebar.cpp`, `src/ui/pages/HomePage.cpp`, `src/ui/pages/SettingsPage.h`, `src/ui/pages/SettingsPage.cpp`, `src/ui/pages/CalendarPage.cpp`, `src/ui/pages/JobsPage.cpp`, `src/ui/pages/StudyPage.cpp`
- **UI Widgets & Dialogs:** `src/ui/widgets/SingleTabDateTime.h`, `src/ui/widgets/MonthView.cpp`, `src/ui/widgets/TopicDetailPanel.cpp`, `src/ui/dialogs/ApplicationDialog.cpp`, `src/ui/dialogs/InterviewDialog.cpp`, `src/ui/dialogs/SessionDialog.cpp`, `src/ui/NotificationText.cpp`, `src/ui/FormatHelper.cpp`, `src/ui/delegates/AgendaItemDelegate.h`, `src/ui/delegates/AgendaItemDelegate.cpp`
- **Tests:** `tests/CMakeLists.txt`, `tests/ui/tst_HiDpi.cpp`, `tests/ui/tst_Shortcuts.cpp`, `tests/services/tst_SettingsService.cpp`, `tests/ui_smoke/ui_smoke.cpp`, `tests/i18n/CMakeLists.txt`, `tests/i18n/tst_Translations.cpp`
- **Documentation:** `docs/RUNNING.md`, `docs/i18n-glossary.md`, `docs/reports/i18n-review.md`

### Identified Risks & Mitigations
1. **Longer Cyrillic Strings:**
   - *Risk:* Ukrainian and Russian words can overflow fixed-width buttons, column headers, or card widths.
   - *Mitigation:* Verified against dynamic font metrics; `tst_HiDpi` extended to run full clipping assertions on both translations.
2. **Missing Strings in `.ts`:**
   - *Risk:* Hardcoded string literals in new UI widgets skipping `tr()`.
   - *Mitigation:* New `tst_Translations` quality test checking translation completeness in automated test suite.
3. **Multi-configuration Windows CI Paths:**
   - *Risk:* Translation `.qm` files failing to resolve if placed on disk next to executable.
   - *Mitigation:* Embedded into Qt resource system (`:/i18n/`) at compile time.

### Not Verified by Agent (User to Check)
1. Physical behavior of `Alt+Down` and `F4` on a live Windows desktop with MSVC.
2. Keyboard navigation using a physical Ukrainian hardware keyboard layout.
3. Linguistic naturalness and stylistic nuances of the Ukrainian and Russian translations.
4. Visual look of the dashboard at 150% scaling on high-density Windows monitors.

---

## 12. Open Questions for the User

1. **Language default:** Should JobPrep automatically default to the system UI language if it is Ukrainian or Russian, and otherwise default to English?
   - *(Recommended: Yes)*
2. **Windows CI Hang:** Was the green Windows run (following fix-up 7) a re-run of commit `05e719b` or a later commit?
   - *(The plan retains all watchdog and timeout safeguards regardless)*
3. **Language Switching Mode:** Should language switching take effect after application restart with a clear user notice, or dynamically at run time?
   - *(Recommended: After restart, ensuring complete visual stability and clean initialization)*
4. **Sample Data Localization:** Should the sample interview preparation topics in `SeedService` remain in English, or be translated into Ukrainian and Russian?
   - *(Recommended: Keep English, as technical C++ concepts are universal in technical interviews)*
5. **Data Export Neutrality:** Should CSV, ICS, and JSON export files remain language-neutral (English headers, snake_case enums, ISO dates)?
   - *(Recommended: Yes, ensuring complete data portability across devices and locales)*
