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
$ git log --oneline -n 12
```
```
7bdc55f (HEAD -> main) m7-amendments
e2ebe2b docs: add M7 plan
9fe8def (origin/main, origin/HEAD) m7 plan request
0c11c50 (tag: m6) docs(reports): add M6 fix-up 8 report
2152753 fix(test): use relative path for offscreen config to prevent Windows path splitting, disable abort dialog, and clean up
a05a89a test(hidpi): add CI-DIAG progress tracking and watchdog to tst_HiDpi
495a191 ci: add test timeout to ctest and step timeouts to quality gate
6b2a6c7 m6-fixup7-upd prompt
05e719b m6-fixup 7 prompts
bbddfa7 docs(reports): add M6 fix-up 7 report
84e464c docs(spec): document small-screen dialog constraints and tab bar scrolling fix
1a4fcbc fix(ui): wrap ApplicationDialog tabs in scroll areas, disable tab scroll buttons, and enforce small screen limits
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

Raw commands and full outputs are catalogued in the [Appendix: Evidence](#appendix-evidence).

| Target / Item | Finding / Location | Evidence Command |
|---|---|---|
| Agenda row delegate | `JobPrep::Ui::Delegates::AgendaItemDelegate` in `src/ui/delegates/AgendaItemDelegate.h`, `.cpp` (used in `HomePage` and `CalendarPage`) | `grep -rn "AgendaItemDelegate" src/` |
| Where HomePage builds "Today" | `HomePage::setupUi()` lines 120–150; `HomePage::applySnapshot()` line 261 | `sed -n '120,150p' src/ui/pages/HomePage.cpp` |
| All Date Controls | 5 occurrences, all instantiated as `SingleTabDateEdit` (`TopicDetailPanel`, `InterviewDialog`, `ApplicationDialog` [x2], `SessionDialog`) | `grep -rn "setCalendarPopup" src/` |
| `invalidateFilter` usage | 4 occurrences (`ApplicationFilterProxy.cpp:21, 26`, `TopicBoardProxy.cpp:26, 33`) | `grep -rn "invalidateFilter" src/` |
| Deprecation warning suppression | Suppressed only for `__GNUC__`/`__clang__` at lines 1–4 of `ApplicationFilterProxy.cpp` & `TopicBoardProxy.cpp` | `head -n 5 src/models/ApplicationFilterProxy.cpp` |
| CI timeouts | `timeout-minutes: 15` (Linux), `timeout-minutes: 30` (Windows), `ctest --timeout 600` | `grep -n "timeout" .github/workflows/ci.yml scripts/check.sh` |
| `CI-DIAG` instrumentation | Watchdog thread (240s) + progress markers present in `tests/ui/tst_HiDpi.cpp`; diagnostic logs in `src/ui/dialogs/ApplicationDialog.cpp` | `grep -rn "CI-DIAG" src/ tests/` |
| String literal count | 482 `tr(` calls in 27 files; 50 `QCoreApplication::translate()` calls in 3 files; 0 `*_NOOP` | Python script over regex in `src/` |
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
- Reproduced offscreen on Linux using a scratch harness (`scratch/reproduce_today.cpp`) with an in-memory DB containing a topic with target date today, track "C++", and title "Advanced Metaprogramming with Concepts, Fold Expressions and SFINAE".
- Screenshots generated:
  - `scratch/today_view.png` (7.1 KB)
  - `scratch/today_dashboard.png` (72 KB)
- The harness source and execution output are documented in the [Evidence Appendix §A.2](#a2-item-1-overlap-reproduction-harness).

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
**Identified Defects (Backed by Code and QSS):**
1. In `resources/styles/theme.qss` (line 206), `QFrame[card="true"]` defines background and border, but there is no `#CardTitle` selector. `titleLabel` has no `objectName` and no stylesheet typography rules, leaving its font size, color, and padding unstyled. In contrast, `#CalendarAgendaTitle` explicitly sets `font-size: 11pt; font-weight: 600; color: @text;` (lines 309–313).
2. In `HomePage.cpp` lines 128–135, `m_todayView` lacks `setFrameShape(QFrame::NoFrame)` and has no `#TodayView` selector with `background: transparent; border: none;`, unlike `#CalendarAgendaList` in `CalendarPage.cpp` (lines 315–318), causing a double-border box effect.
3. Left-alignment progression:
   - `titleLabel` sits at `x = 16 px` (from `cardLayout->setContentsMargins(16, 14, 16, 14)`).
   - In `AgendaItemDelegate.cpp:48`, `content` is `option.rect.adjusted(kHorizontalMargin, ..., -kHorizontalMargin, ...)`, where `kHorizontalMargin = 10`. So the icon begins at `16 + 10 = 26 px`.
   - In `AgendaItemDelegate.cpp:73`, `textRect` is `content.adjusted(kIconSize + 8, ...)`, where `kIconSize = 16`. So row text begins at `26 + 16 + 8 = 50 px`.
   - Without styling, the title at 16 px misaligns with row items starting at 50 px.

### 3.4 Why `tst_HiDpi::checkClipping` Did Not Catch It
1. `checkClipping(QWidget* root)` inspects only `root->findChildren<QWidget*>()`. `QStyledItemDelegate` is not a child widget; its painting is internal to `QListView::paintEvent`.
2. `checkClipping` only tests whether a widget's allocated width/height is smaller than its `minimumSizeHint()` or `heightForWidth()`. It has **no logic to detect bounding-box collisions between sibling widgets or painted text rectangles within delegates**.

### 3.5 Proposed Solution & New Overlap Verification Check
1. **Delegate Single Source of Truth:**
   - In `AgendaItemDelegate`, expose a pure layout helper:
     ```cpp
     struct SubRects {
         QRect iconRect;
         QRect titleRect;
         QRect subtitleRect;
         QRect rightTextRect;
         int requiredHeight;
     };
     static SubRects computeLayout(const QStyleOptionViewItem& option, const QModelIndex& index);
     ```
   - Used identically by `paint()` and `sizeHint()`.
2. **Content-Derived Dynamic Row Height:**
   - Formula:
     ```cpp
     const QFontMetrics tm(titleFont);
     const QFontMetrics sm(subtitleFont);
     const int textHeight = hasSubtitle ? (tm.height() + sm.height() + 4) : tm.height();
     const int rowHeight = qMax(kIconSize + 16, textHeight + 14);
     ```
   - Values across font matrix at 100% and 150%:
     - DejaVu Sans 10pt (100%): `tm=18`, `sm=16` -> two-line `rowHeight = 18 + 16 + 4 + 14 = 52 px` (single-line `32 px`).
     - DejaVu Sans 10pt (150%): `tm=27`, `sm=24` -> two-line `rowHeight = 27 + 24 + 6 + 21 = 78 px`.
3. **Independent Painted-Pixel Ink Test:**
   - Render the item with only `title` into a transparent `QImage`, and with only `subtitle` into a second transparent `QImage`.
   - Compute ink bounding rects (pixels with alpha > 0). Assert that `titleInkRect.intersects(subtitleInkRect)` is strictly `false`.
   - Verify that this test **fails on the unpatched code** (12 px overlap) and passes on the patched code.
4. **Card Header Styling & Theme Switch Verification:**
   - Set `titleLabel->setObjectName(u"CardTitle"_s)`.
   - Add `#CardTitle` and `#TodayListView` to `theme.qss` (`font-size: 11pt; font-weight: 600; color: @text; background: transparent; border: none;`).
   - Call `m_todayView->setFrameShape(QFrame::NoFrame);`.
   - Verify layout and colors in both light and dark themes, and after runtime switching via `ThemeManager`.
5. **Layout-Managed Sibling Collision Check:**
   - Restrict the sibling overlap check strictly to siblings managed by the same layout (e.g. `QBoxLayout`, `QFormLayout`) to prevent false positives from stacked widgets, scroll areas, or intended overlays.

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
- **Proven by test in scratch harness on Linux (`scratch/test_calendar_popup.cpp`):**
  - Sending `Key_F4`, `Alt+Key_Down`, or `Key_Space` to a focused `QDateEdit` with `calendarPopup = true` does **not** open the calendar popup (`isVisible` remains 0).
  - Simulating a mouse press on the drop-down arrow subcontrol (`QStyle::SC_SpinBoxDown` / `QStyle::SC_ComboBoxArrow`) opens `QCalendarPopup` (`isVisible: 1`).
  - Pressing `Key_Escape` dismisses the open popup (`isVisible: 0`).
  - Navigating dates and pressing Enter selects the date and updates `QDateEdit::date()`.
- **Not Proven by Agent (User to Check on Windows):**
  - Behavior on Windows hardware across native Windows styles (`windowsvista`, `windows11`, `windows`).
  - Whether specific Windows screen readers or IME handlers capture `Alt+Down`.

### 4.3 Evaluation of Technical Options
1. **Option A: Synthesized Click on Arrow Subcontrol**
   - Query `style()->subControlRect(QStyle::CC_ComboBox, &cboOpt, QStyle::SC_ComboBoxArrow, this)`. If empty, fallback to `QStyle::CC_SpinBox` / `SC_SpinBoxDown`.
   - Post `QMouseEvent(MouseButtonPress)` and `MouseButtonRelease` to the center point.
   - *Pros:* Uses Qt's existing popup infrastructure and calendar synchronization; zero custom popup lifecycle to manage.
   - *Cons:* Style-dependent subcontrol rects; fragile if a custom style draws arrows outside the widget bounds.
2. **Option B: Dedicated Popup Wrapper (Recommended)**
   - Query Qt's `calendarWidget()`. If null, allow lazy creation.
   - If `calendarWidget()` is available, its top-level window `QWidget* popup = calendarWidget()->window()` is the popup container.
   - Position `popup` directly under the date edit (`mapToGlobal(QPoint(0, height()))`) and call `popup->show()`.
   - *Pros:* 100% style-agnostic; no reliance on subcontrol hit tests; works identically under `fusion`, `windows`, and `windows11`.
   - *Cons:* Must ensure proper popup window flags and event loop grab.

**Recommendation:** Hybrid implementation in `SingleTabDateEdit`: attempt `calendarWidget()->window()->show()` positioned under the field. If Qt private mechanics require mouse activation, trigger via combo arrow hit test with spinbox fallback.

### 4.4 Shortcut Override & Modifier Safety
- **Modifier check:** Use `(event->modifiers() & ~Qt::KeypadModifier) == Qt::AltModifier` to ensure `Alt+Down` from the numeric keypad functions correctly.
- **ShortcutOverride:** In `TopicDetailPanel` and `TopicBoardColumn`, `Alt+Up/Down` is used for reordering checklist items and cards. To prevent parent filters or `QShortcut`s from swallowing `Alt+Down`, `SingleTabDateEdit` must intercept `QEvent::ShortcutOverride`:
  ```cpp
  bool SingleTabDateEdit::event(QEvent* e) {
      if (e->type() == QEvent::ShortcutOverride) {
          auto* ke = static_cast<QKeyEvent*>(e);
          const auto mods = ke->modifiers() & ~Qt::KeypadModifier;
          if ((mods == Qt::AltModifier && ke->key() == Qt::Key_Down) ||
              (mods == Qt::NoModifier && ke->key() == Qt::Key_F4)) {
              ke->accept();
              return true;
          }
      }
      return QDateEdit::event(e);
  }
  ```

### 4.5 Tooltip Hint
- Set tooltip in `SingleTabDateEdit`:
  If existing tooltip is non-empty, append: `u"\n%1"_s.arg(tr("Press Alt+Down or F4 to open calendar"))`.
  Otherwise set directly to the translated string.

### 4.6 Parametrized Multi-Style Test
- A data-driven test covering all 5 controls by name:
  `TopicDetailPanelDateEdit`, `InterviewDateEdit`, `ApplicationAppliedDateEdit`, `ApplicationNextActionDateEdit`, `SessionDateEdit`.
- Iterates over each style returned by `QStyleFactory::keys()` (e.g. `fusion`, `windows`).
- Asserts:
  1. `Alt+Down` opens popup (`popup->isVisible() == true`).
  2. `Esc` closes only the popup (`popup->isVisible() == false`, dialog remains open).
  3. `F4` re-opens popup.
  4. Date selection with arrow keys + Enter updates date, closes popup, leaves dialog open.
  5. A subsequent `Esc` closes the parent dialog.

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
- In Qt 6.9+, `QSortFilterProxyModel` introduced `beginFilterChange()` and `endFilterChange(Directions directions = Direction::Both)`. `invalidateFilter()` is deprecated with `QT_DEPRECATED_VERSION_X_6_13("Use begin/endFilterChange() instead")`.
- `Direction` is declared as `enum class Direction { Rows = 0x01, Columns = 0x02, Both = Rows | Columns };` under `QSortFilterProxyModel`.
- Methods are `protected` in `QSortFilterProxyModel` (`qsortfilterproxymodel.h:124-125`).
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
- Confirmed via `grep -rn "pragma" src tests CMakeLists.txt`: no other warning suppressions exist in the project.

### 5.3 CI Matrix Implementation (Binding Decision 6)
- **Triggers:**
  - `push` and `pull_request` on `main`: Run Option B (Ubuntu 6.8.2, Ubuntu 6.10.3, Windows 6.10.3).
  - `workflow_dispatch` and weekly `schedule` (cron: `0 3 * * 0`): Also include Windows 6.8.2.
- **Estimated CI Runtime Impact:**
  - Ubuntu jobs run in ~3.5 minutes.
  - Windows jobs run in ~10–12 minutes.
  - Adding Ubuntu 6.10.3 and switching Windows to 6.10.3 adds an estimated ~3.5 minutes total to the pull-request gate, keeping Windows cost neutral.

### 5.4 `docs/RUNNING.md` Windows Section Updates
- **Documented Facts:**
  - Known failing compiler: MSVC `19.30.30706` (VS 2022 17.0) fails with `fatal error C1001: Internal compiler error` in `QtCore/qrangemodel_impl.h(903)` when compiling with Qt 6.10.
  - Known good compiler: MSVC `19.44.35229` (user's PC) and MSVC `19.42+` (GitHub Actions CI `windows-latest`).
  - Document `-DJOBPREP_WERROR=OFF` strictly as a temporary workaround that is superseded by the M7 fix.
- **Steps:**
  1. Open **x64 Native Tools Command Prompt for VS 2022**.
  2. `set QT_PREFIX_PATH=C:\Qt\6.10.3\msvc2022_64` (or `6.8.2`)
  3. `set PATH=%QT_PREFIX_PATH%\bin;%PATH%`
  4. `cmake --preset dev-windows`
  5. `cmake --build --preset dev-windows`
  6. `ctest --preset dev-windows --output-on-failure`
  7. Run: `.\build\dev-windows\jobprep.exe` (reported working by user on his PC).

---

## 6. Item 4: CI Hygiene & Windows Test Performance

### 6.1 Audit of Existing Safeguards & Fix-up 8 Conclusion
- `docs/reports/M6-fixup8.md` lines 280–286 & 564:
  - **Evidenced Fact:** In `QT_QPA_PLATFORM="offscreen:configfile=C:/..."`, Qt splits on `:` and took `"C"` as the config filename, causing `qFatal("Could not find platform config file C")` and exit code -6.
  - **Inferred Hypothesis:** On Windows MSVC Debug, `abort()` / `qFatal()` opened a Win32 CRT modal assertion message box that stalled indefinitely in CI.
  - **Verification Status:** Unproven on Windows hardware; only confirmed by a green Windows CI run containing commit `2152753`.
- `jobprep_offscreen_1080p.json`: If file creation fails, `tst_HiDpi` must not fail silently; add an explicit warning/skip statement.
- `_set_abort_behavior` under `#if defined(Q_OS_WIN)`: Note that this line was written for Windows CI and was never compiled locally on Linux.

### 6.2 Removal of Diagnostic Lines from Production Code
- In `src/ui/dialogs/ApplicationDialog.cpp` (lines 106–126) and `ApplicationDialog.h` (line 65):
  Remove `showEvent` and diagnostic `qInfo("// CI-DIAG ...")` statements.
- Retain watchdog and progress markers in `tests/ui/tst_HiDpi.cpp` (test-only, zero production impact).

### 6.3 Slow Tests Measurements
Local measured durations under Linux offscreen (`build/dev-linux`):
- `tst_PageStudy`: **17.914 s** (31 reset data calls)
  - `testChipBarJobsAndCalendarRegression`: 1.599 s
  - `testStudyPageApi`: 1.244 s
  - `testStudyListViewToggleSortAndDelegates`: 0.820 s
- `tst_PageHome`: **16.100 s**
  - `testToastNotificationSmoke`: 2.955 s
  - `testDashboardVisitedBothThemes`: 2.920 s
- `tst_PageJobs`: **12.050 s** (20 reset data calls)
  - `testJobsPageThemes`: 1.842 s
  - `testApplicationDialogInterviewsTabEditable`: 0.716 s
- `tst_EmptyStatesAndFocus`: **10.075 s**
  - `testFocusIndicatorDoesNotChangeGeometryOrSizeHint`: 1.272 s
  - `testVisibleFocusIndicatorChip`: 1.267 s

*Note:* Refactoring test fixtures remains a proposal for future milestones; no test logic changes in Part A.

---

## 7. Item 5: GUI Translation (Ukrainian `uk` & Russian `ru`)

### 7.1 Reconciled String Audit
- Total `tr(` calls: **482** across 27 files.
- Total `QCoreApplication::translate()` calls: **50** across 3 files (`StatusStyle.cpp: 28`, `NotificationText.cpp: 19`, `AgendaKindStyle.cpp: 3`).
- Total translatable occurrences: **532**.
- String concatenation around `tr()`: **0**.
- Untranslated UI literals audit:
  - `MainWindow.cpp:51`: `setWindowTitle(u"JobPrep"_s)` (Application name, intentional).
  - `MainWindow.cpp:323, 325`: `QLabel(u" | "_s)` (Visual separator).
  - `ApplicationDialog.cpp:264`: `m_urlEdit->setPlaceholderText(u"https://"_s)` (URL schema prefix).
  - `Sidebar.cpp:122`: `QLabel(u"JobPrep"_s)` (Application branding).
  - `FunnelWidget.cpp:138`: `drawText(arrowRect, u"→"_s)` (Directional arrow).
  - `FollowUpDialog.cpp:112`: `u"%1 – %2"_s.arg(...)` (Composite entity label).

### 7.2 Stored Values vs Display Text
- Enums are converted to/from lowercase snake_case strings (`src/domain/EnumStrings.cpp`).
- Display text is mapped exclusively in `ui/theme/StatusStyle.cpp` and `ui/theme/AgendaKindStyle.cpp`.
- **Export & Storage Policy:**
  - SQLite tables store snake_case strings and ISO-8601 timestamps.
  - JSON backups store language-neutral keys and values.
  - CSV exports keep the 22 standardized English column headers and snake_case values.
  - ICS exports keep standard VCALENDAR English property names.
  - Backup/restore across language changes is 100% binary- and data-identical.

### 7.3 Tooling & Build Integration
- Add `LinguistTools` to `find_package(Qt6 ...)` in `CMakeLists.txt`.
- Translations source files placed in `resources/translations/jobprep_uk.ts` and `resources/translations/jobprep_ru.ts`.
- Use `qt_add_translations(jobprep_core TS_FILES ... RESOURCE_PREFIX "/i18n")` to compile `.ts` into `.qm` embedded into the binary resource system.
- Ensure resources are initialized via `JobPrep::App::ensureResources()`.
- Add test `tst_Translations` asserting zero unfinished or empty strings, and a script step running `lupdate` into a temporary file to assert no untracked strings exist.

### 7.4 Standard Qt Strings
- `QMessageBox::StandardButton` (Save, Cancel, Yes, No, Discard), `QFileDialog`, and `QDialogButtonBox` texts come from `qtbase_<lang>.qm`.
- In `main.cpp`, load `qtbase_<lang>.qm` from `QLibraryInfo::path(QLibraryInfo::TranslationsPath)`. If missing, fallback to English without warning.

### 7.5 Plurals Inventory & Test Values
- Count formatting locations:
  1. `FormatHelper.cpp:34`: `tr("%n day(s)", "", days)`
  2. `FormatHelper.cpp:38`: `tr("%n hour(s)", "", hours)`
  3. `FormatHelper.cpp:40`: `tr("%n minute(s)", "", minutes)`
  4. `FormatHelper.cpp:51`: `tr("%n-day streak", "", days)`
  5. `FormatHelper.cpp:58`: `tr("%n active process(es)", "", count)`
  6. `ApplicationDialog.cpp:571`: `tr("%n min", "", interview.durationMin)`
  7. `NotificationText.cpp:15-17`: `translate("NotificationText", "in %n hour(s)", "", hours)`
  8. `NotificationText.cpp:20-22`: `translate("NotificationText", "in %n minute(s)", "", leadMinutes)`
  9. `NotificationText.cpp:127-129`: `translate("NotificationText", "%n follow-up(s) overdue", "", count)`
  10. `InAppNotifier.cpp:101`: `tr("+%n more", "", overflowCount)`
- Plural test values: `1, 2, 5, 11, 21, 22, 25, 101` for `uk` and `ru`.

### 7.6 Language Selection: Restart-Based
- Language setting stored in `SettingsService` under `ui/language` (`system`, `en`, `uk`, `ru`).
- Selecting a language in `SettingsPage` displays an inline message: *"Language will take effect after restarting JobPrep."*
- `main.cpp` installs `QTranslator` and calls `QLocale::setDefault()` before creating `MainWindow`.

### 7.7 Keyboard Shortcuts & Mnemonics
- Global shortcuts (`Ctrl+1..4`, `Ctrl+N`, `Ctrl+F`, `Ctrl+S`, `Esc`) match by physical scan code in Qt. Marked **not proven by agent on Windows hardware, user to check**.
- Form labels and buttons with `&` mnemonics will **omit mnemonics in Ukrainian and Russian** to avoid layout-dependent Alt-key collisions.

### 7.8 Early Layout Verification
- In Part C, load a test-only pseudo-translation (strings padded by 40% with Cyrillic glyphs) in `tst_HiDpi` to detect layout clipping before translating.

### 7.9 Draft Domain Glossary (For User Review)

| English UI Term | Ukrainian Draft (`uk`) | Russian Draft (`ru`) | Context / Notes |
|---|---|---|---|
| Application | Заявка | Заявка / Отклик | Job application tracking |
| Interview | Співбесіда | Собеседование | Scheduled interview stage |
| Follow-up | Подальший контакт / Фоллоу-ап | Контакт / Фоллоу-ап | Distinct from Reminder (`Нагадування`) |
| Study Target | Ціль вивчення | Цель изучения | Topic due date target in Agenda |
| Streak | Серія днів | Серия дней | Consecutive study days |
| Wishlist | Бажане | В планах | Application stage 0 |
| Applied | Подано | Отправлено | Application stage 1 |
| HR Screen | HR-скринінг | HR-скрининг | Interview type / application stage |
| Technical | Технічне | Техническое | Technical interview stage |
| Live coding | Лайв-кодинг | Лайв-кодинг | Interview type |
| System design | Системний дизайн | Системный дизайн | Interview type |
| Behavioral | Поведінкове | Поведенческое | Interview type |
| Final | Фінальне | Финальное | Final interview stage |
| Offer | Офер | Оффер | Job offer received |
| Accepted | Прийнято | Принято | Offer accepted |
| Rejected | Відхилено | Отказ | Application rejected |
| Withdrawn | Відкликано | Отозвано | Withdrawn by candidate |
| Ghosted | Без відповіді | Без ответа | Employer stopped responding |
| Backlog | Беклог | Бэклог | Study topic status |
| In Progress | В процесі | В процессе | Study topic status |
| Review | Повторення | Повторение | Study topic status |
| Done | Вивчено | Изучено | Study topic status |

---

## 8. Proposed SPEC §12 & §14 Text

### SPEC §12: Milestone M7 Entry
```markdown
**M7 — Localization, Compatibility & UI Quality.**
- GUI localization: Ukrainian (uk) and Russian (ru), with English remaining the source language. Language selection in Settings (System default / English / Українська / Русский), stored in SettingsService; language takes effect on application startup. Plural forms (%n) handled for all counters. MonthView day and month names follow the active locale. Stored database values and CSV/JSON/ICS exports remain language-neutral.
- Agenda item layout: Fixed text overlap between title and track category label in two-line agenda rows; polished card header styling and spacing.
- Date controls keyboard accessibility: Alt+Down and F4 open the calendar popup across all date fields via SingleTabDateEdit; Esc closes the popup; tooltip shortcut hint added.
- Qt 6.10 & MSVC compatibility: Replaced deprecated QSortFilterProxyModel::invalidateFilter() with beginFilterChange()/endFilterChange() guarded by QT_VERSION_CHECK; eliminated compiler warning suppressions; updated CI matrix to include Qt 6.10; updated RUNNING.md for MSVC toolset requirements.
- Acceptance: Switching language in Settings and restarting displays all UI pages, dialogs, badges, and notifications in Ukrainian or Russian; zero text clipping or delegate overlap at 100% and 150% scaling in both themes; date popups open via keyboard in all dialogs; the CI matrix passes on Qt 6.8.x and 6.10.x.
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
  - `src/ui/dialogs/ApplicationDialog.h`, `.cpp`
  - `.github/workflows/ci.yml`
  - `docs/RUNNING.md`
- **Actions:**
  1. Replace `invalidateFilter()` with `beginFilterChange()` / `endFilterChange(Direction::Rows)` guarded by `QT_VERSION_CHECK(6, 9, 0)`.
  2. Remove `#pragma GCC diagnostic ignored "-Wdeprecated-declarations"` blocks.
  3. Remove all `// CI-DIAG` lines from `src/ui/dialogs/ApplicationDialog.cpp` and `.h`.
  4. Update `.github/workflows/ci.yml` to include Qt 6.10.3 in the PR matrix, plus Windows 6.8.2 on workflow_dispatch/schedule.
  5. Add CI diagnostic step for `lrelease`/`lupdate` and `Qt6LinguistTools`.
  6. Update `docs/RUNNING.md` with Visual Studio 2022 toolset requirements and tested command lines.
- **Verification:** `./scripts/check.sh` passes with zero warnings.

### Part B: UI Bug Fixes (Today Overlap & Date Keyboard Access)
- **Scope:** Item 1 (Agenda text overlap & card headers) and Item 2 (Date picker keyboard navigation).
- **Files Touched:**
  - `src/ui/delegates/AgendaItemDelegate.cpp`, `.h`
  - `src/ui/pages/HomePage.cpp`
  - `src/ui/widgets/SingleTabDateTime.h`, `.cpp`
  - `resources/styles/theme.qss`
  - `tests/ui/tst_HiDpi.cpp`
  - `tests/ui/tst_Shortcuts.cpp`
- **Actions:**
  1. Implement `SubRects computeLayout(...)` in `AgendaItemDelegate`.
  2. Fix `paint()` and dynamic `sizeHint()`. Add independent painted-pixel ink test.
  3. Style card headers with `#CardTitle` and clean `HomePage` list view frame.
  4. Add `Alt+Down` and `F4` key event handling, `ShortcutOverride`, and tooltip hints in `SingleTabDateEdit`.
  5. Add keyboard popup activation tests across all 5 date controls in all available styles.
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
  7. Run early layout tests with pseudo-translation in `tst_HiDpi`.
- **Verification:** `./scripts/check.sh` green; English UI operates unchanged.

### Part D: Translation Delivery, Review & Verification
- **Scope:** Ukrainian and Russian translations, glossary, review documentation, HiDPI verification.
- **Files Touched:**
  - `resources/translations/jobprep_uk.ts`
  - `resources/translations/jobprep_ru.ts`
  - `docs/i18n-glossary.md`
  - `docs/reports/i18n-review.md`
  - `tests/ui/tst_HiDpi.cpp`
  - `tests/i18n/tst_Translations.cpp`
  - `docs/reports/M7.md`
- **Actions:**
  1. Create `docs/i18n-glossary.md`.
  2. Populate `jobprep_uk.ts` and `jobprep_ru.ts` with complete, polished translations.
  3. Generate `docs/reports/i18n-review.md` table for user review.
  4. Add `tst_Translations` checking complete translations and plural rules.
  5. Run `tst_HiDpi` under `uk` and `ru` locales; verify zero clipping at 100% and 150%.
  6. Compile final M7 report.
- **Verification:** All tests green; zero unfinished strings; zero clipping.

---

## 10. Test Inventory & Expected Baselines

### New & Extended Test Slots

| Test Binary | Target Slot | Type | Verification Scope |
|---|---|---|---|
| `tst_HiDpi` | `testDelegateAndWidgetTextOverlap` | Existing Binary | Renders agenda rows; asserts non-overlap of title and category ink rects at 100% & 150% scales |
| `tst_HiDpi` | `testTranslationsHiDpi` | Existing Binary | Validates all pages and dialogs in `uk` and `ru` at 100% & 150% scales for zero text clipping |
| `tst_Shortcuts` | `testDateEditKeyboardPopup` | Existing Binary | Sends `Alt+Down` and `F4` to `SingleTabDateEdit`; verifies popup opens, date selects, Esc closes |
| `tst_PageJobs` | `testApplicationDialogDatePopupKeys` | Existing Binary | Tests keyboard popup on Applied Date and Next Action Date in `ApplicationDialog` |
| `tst_PageCalendar` | `testInterviewDialogDatePopupKeys` | Existing Binary | Tests keyboard popup on Date in `InterviewDialog` |
| `tst_SettingsService` | `testLanguageSetting` | Existing Binary | Verifies `language()` getter/setter persistence and signal emission |
| `tst_Translations` | `testAllTranslationsFinished` | **New Binary** | Verifies `jobprep_uk.ts` and `jobprep_ru.ts` have 0 untranslated or obsolete entries |
| `tst_Translations` | `testPluralForms` | **New Binary** | Verifies 1, 2, 5, 11, 21, 22, 25, 101 plural outputs in Ukrainian and Russian |
| `tst_Translations` | `testResourcePresence` | **New Binary** | Asserts `:/i18n/jobprep_uk.qm` exists in executable |
| `ui_smoke` | `testLanguageSmoke` | Existing Binary | Loads `MainWindow` under Ukrainian and Russian locales without Qt warnings |

### Expected Baseline Projection
- **Current Baseline:** 34 test binaries, 469 test functions.
- **Projected New Baseline:** 35 test binaries (+1 for `tst_Translations`), ~480 test functions (+11 new functions).

---

## 11. Appendix: Evidence

### A.1 Pre-Flight Findings Commands & Outputs

#### 1. Proxy Models Diagnostic Pragmas
```bash
$ head -n 5 src/models/ApplicationFilterProxy.cpp && head -n 5 src/models/TopicBoardProxy.cpp
```
```
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#include "models/ApplicationFilterProxy.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
```

#### 2. QSortFilterProxyModel Filter Change Methods Declaration
```bash
$ grep -n -B3 -A3 "beginFilterChange" /usr/include/x86_64-linux-gnu/qt6/QtCore/qsortfilterproxymodel.h
```
```
121-    virtual bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const;
122-    virtual bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;
123-
124:    void beginFilterChange();
125-    void endFilterChange(Directions directions = Direction::Both);
126-#if QT_DEPRECATED_SINCE(6, 13)
127-    QT_DEPRECATED_VERSION_X_6_13("Use begin/endFilterChange() instead")
```

#### 3. invalidateFilter Deprecation Macro
```bash
$ grep -n -B3 -A3 "invalidateFilter" /usr/include/x86_64-linux-gnu/qt6/QtCore/qsortfilterproxymodel.h
```
```
125-    void endFilterChange(Directions directions = Direction::Both);
126-#if QT_DEPRECATED_SINCE(6, 13)
127-    QT_DEPRECATED_VERSION_X_6_13("Use begin/endFilterChange() instead")
128:    void invalidateFilter();
129-    QT_DEPRECATED_VERSION_X_6_13("Use begin/endFilterChange(QSortFilterProxyModel::Direction::Rows) instead")
130-    void invalidateRowsFilter();
131-    QT_DEPRECATED_VERSION_X_6_13("Use begin/endFilterChange(QSortFilterProxyModel::Direction::Columns) instead")
```

#### 4. Available Qt 6.10 Architectures in aqtinstall
```bash
$ python3 -m aqt list-qt linux desktop --arch 6.10.3 && python3 -m aqt list-qt windows desktop --arch 6.10.3
```
```
linux_gcc_64
win64_llvm_mingw win64_mingw win64_msvc2022_64 win64_msvc2022_arm64_cross_compiled
```

#### 5. SingleTabDateTime.h Implementation
```bash
$ cat src/ui/widgets/SingleTabDateTime.h
```
```cpp
#pragma once

#include <QDateEdit>
#include <QTimeEdit>

namespace JobPrep::Ui::Widgets {

/// A QDateEdit where Tab and Backtab advance to the next or previous widget
/// on a single key press instead of stepping through internal date sections.
class SingleTabDateEdit : public QDateEdit {
public:
    using QDateEdit::QDateEdit;

protected:
    bool focusNextPrevChild(bool next) override {
        return QWidget::focusNextPrevChild(next);
    }
};

/// A QTimeEdit where Tab and Backtab advance to the next or previous widget
/// on a single key press instead of stepping through internal time sections.
class SingleTabTimeEdit : public QTimeEdit {
public:
    using QTimeEdit::QTimeEdit;

protected:
    bool focusNextPrevChild(bool next) override {
        return QWidget::focusNextPrevChild(next);
    }
};

}  // namespace JobPrep::Ui::Widgets
```

#### 6. Diagnostic Pragmas Audit
```bash
$ grep -rn "pragma" src tests CMakeLists.txt
```
```
src/models/AgendaListModel.h:1:#pragma once
src/models/ApplicationFilterProxy.cpp:2:#pragma GCC diagnostic push
src/models/ApplicationFilterProxy.cpp:3:#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
src/models/ApplicationFilterProxy.cpp:67:#pragma GCC diagnostic pop
src/models/TopicTableProxy.h:1:#pragma once
src/models/ApplicationFilterProxy.h:1:#pragma once
src/models/TopicListModel.h:1:#pragma once
src/models/TopicBoardProxy.cpp:2:#pragma GCC diagnostic push
src/models/TopicBoardProxy.cpp:3:#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
src/models/TopicBoardProxy.cpp:173:#pragma GCC diagnostic pop
... (all remaining matches are `#pragma once` or SQLite `PRAGMA` statements)
```

#### 7. Last SPEC Changelog Entries
```bash
$ grep -n "^- 1\." docs/SPEC.md | tail -3
```
```
529:- 1.21 — M6 Part D2 Fix-ups (Evidence audit, real-data safety, font metrics, test hardening):
535:- 1.22 — Legacy timestamp UI presentation cutoff (2013-01-01):
537:- 1.23 — Small-screen dialog constraints and tab bar scrolling fix:
```

### A.2 Item 1 Overlap Reproduction Harness

Source of scratch harness executed outside git tracking:
```cpp
#include <QApplication>
#include <QDate>
#include <QDateTime>
#include <QImage>
#include <QFontMetrics>
#include <QListView>
#include <iostream>

#include "app/AppContext.h"
#include "data/TrackRepository.h"
#include "data/TopicRepository.h"
#include "domain/Structs.h"
#include "services/StatsService.h"
#include "services/DayWatcher.h"
#include "ui/MainWindow.h"
#include "ui/pages/HomePage.h"

using namespace JobPrep;
using namespace Qt::StringLiterals;

int main(int argc, char* argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    App::AppContext ctx(u":memory:"_s);
    const QDate today = QDate::currentDate();
    ctx.statsService().setClock([today] { return QDateTime(today, QTime(12, 0)); });
    ctx.dayWatcher().setClock([today] { return today; });

    Domain::Track track;
    track.name = u"C++"_s;
    track.color = u"#3b82f6"_s;
    ctx.tracks().insert(track);

    Domain::Topic topic;
    topic.trackId = track.id;
    topic.title = u"Advanced Metaprogramming with Concepts, Fold Expressions and SFINAE"_s;
    topic.targetDate = today;
    topic.status = Domain::TopicStatus::InProgress;
    ctx.topics().insert(topic);

    Ui::MainWindow win(ctx);
    win.resize(1100, 700);
    win.show();
    win.dashboardController()->refreshNow();
    QApplication::processEvents();

    auto* home = win.homePage();
    auto* todayView = home->findChild<QListView*>();

    const QFont font = todayView->font();
    QFont titleFont = font;
    QFont subtitleFont = font;
    subtitleFont.setPointSizeF(qMax(7.5, font.pointSizeF() - 1.0));
    const QFontMetrics titleMetrics(titleFont);
    const QFontMetrics subtitleMetrics(subtitleFont);

    const int kRowHeight = 44;
    const int kIconSize = 16;
    const int kHorizontalMargin = 10;
    QRect itemRect(0, 0, todayView->width(), kRowHeight);
    const QRect content = itemRect.adjusted(kHorizontalMargin, 0, -kHorizontalMargin, 0);
    const QRect textRect = content.adjusted(kIconSize + 8, 3, 0, -3);

    int titleDrawTop = textRect.top() + (textRect.height() - titleMetrics.height()) / 2;
    int titleDrawBottom = titleDrawTop + titleMetrics.height();

    QRect subtitleRect = textRect.adjusted(0, titleMetrics.height() / 2 + 2, 0, 0);
    int subtitleDrawTop = subtitleRect.top() + (subtitleRect.height() - subtitleMetrics.height()) / 2;
    int subtitleDrawBottom = subtitleDrawTop + subtitleMetrics.height();

    std::cout << "todayView geometry: " << todayView->geometry().x() << "," << todayView->geometry().y()
              << " " << todayView->geometry().width() << "x" << todayView->geometry().height() << "\n";
    std::cout << "itemRect: " << itemRect.x() << "," << itemRect.y() << " " << itemRect.width() << "x" << itemRect.height() << "\n";
    std::cout << "textRect: " << textRect.x() << "," << textRect.y() << " " << textRect.width() << "x" << textRect.height() << "\n";
    std::cout << "titleMetrics height: " << titleMetrics.height() << "\n";
    std::cout << "subtitleMetrics height: " << subtitleMetrics.height() << "\n";
    std::cout << "Title vertical bounds: [" << titleDrawTop << " .. " << titleDrawBottom << "]\n";
    std::cout << "Subtitle vertical bounds: [" << subtitleDrawTop << " .. " << subtitleDrawBottom << "]\n";
    int overlap = std::max(0, std::min(titleDrawBottom, subtitleDrawBottom) - std::max(titleDrawTop, subtitleDrawTop));
    std::cout << "OVERLAP: " << overlap << " px\n";
    return 0;
}
```
Execution command:
```bash
$ HOME=/tmp/jp-isolated XDG_DATA_HOME=/tmp/jp-isolated/.local/share XDG_CONFIG_HOME=/tmp/jp-isolated/.config scratch/reproduce_today
```
Output:
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
