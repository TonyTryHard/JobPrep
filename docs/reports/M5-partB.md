# Milestone M5 Part B Report — Dashboard UI

## 1. Executive Summary & Plan vs. Result

Milestone M5 Part B delivers the user-facing Dashboard (Home page) in Qt 6 Widgets, complete with:
- Part A carry-overs (C0): SPEC §14 updated and committed, Part A errata report committed, follow-up item time confirmed.
- `DayWatcher` service: 60-second periodic timer checking system clock transitions across midnight, emitting `dayChanged(QDate)` with injectable clock seam for fast deterministic testing.
- `FormatHelper` formatting utility: centralized duration, streak, active processes, interview rate, next interview, and funnel stage formatting.
- `DashboardController`: centralized single-owner controller that coalesces domain mutations and date changes into a single 0 ms queued update, exposing test seam `refreshCount()`.
- Four custom-painted dashboard widgets adhering to tokens, DPI awareness, font-metrics sizing, and zero per-paint allocations: `StatCard`, `ProgressRing`, `FunnelWidget`, `GoalProgressBar`.
- `HomePage`: 4 stat cards, Today list (`AgendaListModel` + `AgendaItemDelegate`), Upcoming interviews list, Study progress (progress rings per track + weekly goal bar), Hiring funnel (5 stages + interview rate), and empty state D6 with "+ Add application" and "+ Add topic" dual actions.
- Cross-page navigation: `JobsPage::openApplication(int id)` and `CalendarPage::openInterview(int id)` for seamless deep-linking from dashboard items.
- Live status bar wiring in `MainWindow` (`m_statusStreakLabel`, `m_statusProcessesLabel`, `m_statusNextInterviewLabel`).
- 13 comprehensive UI smoke test slots and 4 screenshot captures.

### Gaps vs. Plan
None. All components, delegates, widgets, controller logic, navigation routes, and acceptance criteria were fully implemented and verified.

---

## 2. Carry-overs from Part A (C0) Outcomes

1. **`docs/SPEC.md` §14 Update**: Committed in `e8fec90` (Version 1.7). Documents all Part A repository query methods, signatures, parameters, and return types.
2. **`docs/reports/M5-partA-errata.md`**: Committed in `6bf3acb`. Re-tallied test counts using awk to rectify previous row-sum discrepancy (`221 + 26 = 247`).
3. **Follow-up Item Time Confirmation**: Verified via grep in `src/services/AgendaService.cpp`. Because job applications store only `next_action_date` (`QDate`), follow-up items explicitly set `item.time = std::nullopt;`. The Today list displays time only when `item.time.has_value()`; an artificial 09:00 is never created.

Raw grep evidence:
```
$ grep -n "item.time = " src/services/AgendaService.cpp
109:        item.time = std::nullopt;
202:        item.time = std::nullopt;
```

---

## 3. Amendments Checklist (B1–B8)

| Item | Requirement | Implementation / Evidence |
|---|---|---|
| **B1** | Today item time display | Displayed only when `time.has_value()`. Grep: `AgendaItemDelegate.cpp:46: if (item.time.has_value()) { ... }`. Follow-up actions display no time; interviews display `HH:mm`. |
| **B2** | Empty state D6 | `EmptyState` supports optional secondary action button. Shown when `totalApplications == 0 && totalTopics == 0`. Secondary action "+ Add application" switches to Jobs & opens dialog; primary "+ Add topic" opens TopicDialog. Verified in `testDashboardEmptyStateActionButtons`. |
| **B3** | Today list styling | Overdue non-interview items render "Overdue" badge in `tokens.danger`. Today items show time if set. Past interviews are dimmed with `tokens.textMuted`. Upcoming interviews show short weekday if date != today. Verified in `AgendaItemDelegate.cpp` and `testDashboardTodayItemClicksOpenRespectiveDialogs`. |
| **B4** | Wrap progress rings | `ProgressRing::paintEvent` flows rings in rows of `columnsForWidth(width())`, wrapping when tracks exceed available width. Rings render track name elided under ring with tooltip for full text. Ring inner text is centered. Verified in `testWidgetMetricsAndRingWrapping`. |
| **B5** | Funnel widget styling | Horizontal 5-stage funnel (`Applied`, `Screen`, `Technical`, `Offer`, `Accepted`). Values centered with stage names below. Chevrons in `tokens.border` between stages. Offer-to-Interview conversion rate rendered below. Minimum width based on font metrics. Verified in `testDashboardNumbersMatchHandCalculatedDataset`. |
| **B6** | Refresh coalescing | `DashboardController` batches burst changes (`trackRepo`, `topicRepo`, `sessionRepo`, `appRepo`, `interviewRepo`, `settingsRepo`, `dayWatcher`) with single-shot 0 ms timer. Multiple rapid writes trigger exactly 1 refresh. Verified in `testBurstChangesCoalesceToOneRefresh`. |
| **B7** | Status bar live labels | `MainWindow` updates `m_statusStreakLabel`, `m_statusProcessesLabel`, and `m_statusNextInterviewLabel` on snapshot emission. Verified in `testStatusBarTextScenarios`. |
| **B8** | Weekly goal & First Day | Immediate recalculation on `settingsService.changed()`. Weekly goal bar clamped visually to 100% with real % displayed in text. Sunday/Monday week boundary switch re-evaluates week study minutes immediately. Verified in `testWeeklyGoalChangeUpdatesBarAndTextImmediately` and `testFirstDayOfWeekChangeRefreshesWeekNumbers`. |

---

## 4. Design of the Refresh Owner (`DashboardController`)

- **Location**: `src/ui/DashboardController.h` and `src/ui/DashboardController.cpp`.
- **Ownership**: Owned by `MainWindow` as a Qt child (`QObject`), instantiated in `MainWindow`'s constructor and passed to `HomePage`.
- **Monitored Signals**:
  - `TrackRepository::changed`
  - `TopicRepository::changed`
  - `SessionRepository::changed`
  - `ApplicationRepository::changed`
  - `InterviewRepository::changed`
  - `SettingsService::changed`
  - `DayWatcher::dayChanged`
- **Coalescing Mechanism**:
  A single-shot `QTimer` (`m_coalesceTimer`) configured with `setInterval(0)`. Any signal triggers `requestRefresh()`, which starts or keeps the timer running. When the Qt event loop enters the next idle iteration, `refreshNow()` executes once, reading all data from `StatsService` and `AgendaService`, assembling an immutable `DashboardSnapshot`, and emitting `snapshotReady(snapshot)`.
- **Testing Seam (`refreshCount()`)**:
  `refreshCount()` returns an `int` counter incremented every time `refreshNow()` completes. This allows deterministic unit and smoke tests to assert that multiple synchronous changes coalesce into exactly one rebuild.

---

## 5. New Public Methods

### `JobsPage`
- `bool openApplication(int id)`: Switches to the Jobs page, selects the application with matching ID, and opens `ApplicationDialog` for editing. Returns `true` if found.

### `CalendarPage`
- `bool openInterview(int id)`: Switches to the Calendar page, selects the interview's date in `MonthView`, and opens `InterviewDialog` for editing. Returns `true` if found.

### `DayWatcher`
- `explicit DayWatcher(QObject *parent = nullptr)`
- `QDate currentDate() const`
- `void checkNow()`
- `void setClock(Clock clock)` (test seam for time manipulation)
- Signal: `void dayChanged(QDate newDate)`

### `DashboardController`
- `explicit DashboardController(AppContext &ctx, QObject *parent = nullptr)`
- `void requestRefresh()`
- `void refreshNow()`
- `const DashboardSnapshot &snapshot() const`
- `int refreshCount() const` (test seam)
- Signal: `void snapshotReady(const DashboardSnapshot &snapshot)`

### `StatCard`, `ProgressRing`, `FunnelWidget`, `GoalProgressBar`
- `StatCard::setData(const QString &title, const QString &value, const QString &subtitle)`
- `ProgressRing::setTracks(const QList<Domain::TrackProgress> &tracks)`
- `GoalProgressBar::setProgress(int loggedMinutes, int targetMinutes)`
- `FunnelWidget::setStages(const Domain::Funnel &funnel, double interviewRate)`

---

## 6. Screenshots

| Screenshot | Path | MD5 Checksum |
|---|---|---|
| Dashboard Empty (Dark) | `docs/reports/img/dashboard-empty-dark.png` | `a00e1981a3101593bc93a9f73efcad47` |
| Dashboard Empty (Light) | `docs/reports/img/dashboard-empty-light.png` | `68d60724adfe1113f4f794577605ca0c` |
| Dashboard Populated (Dark) | `docs/reports/img/dashboard-with-data-dark.png` | `8886c905dffcaae8be1422292d08601b` |
| Dashboard Populated (Light) | `docs/reports/img/dashboard-with-data-light.png` | `5a4a678f6feeeeb67cd24de2ffa0ac10` |

All four screenshots verify high-contrast readability, polished token-based palette, consistent Inter typography, correct layout alignments, and proper empty/populated transitions.

---

## 7. Verification and Test Results

### A. Full Quality Check (`./scripts/check.sh`)
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
 1/21 Test  #1: tst_Database .....................   Passed    0.04 sec
      Start  2: tst_TrackRepository
 2/21 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start  3: tst_TopicRepository
 3/21 Test  #3: tst_TopicRepository ..............   Passed    0.20 sec
      Start  4: tst_SessionRepository
 4/21 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  5: tst_ApplicationRepository
 5/21 Test  #5: tst_ApplicationRepository ........   Passed    0.15 sec
      Start  6: tst_InterviewRepository
 6/21 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start  7: tst_ReminderLogRepository
 7/21 Test  #7: tst_ReminderLogRepository ........   Passed    0.03 sec
      Start  8: tst_SeedService
 8/21 Test  #8: tst_SeedService ..................   Passed    0.84 sec
      Start  9: tst_TransactionBehavior
 9/21 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start 10: ui_smoke
10/21 Test #10: ui_smoke .........................   Passed   74.21 sec
      Start 11: tst_MonthView
11/21 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 12: tst_AgendaListModel
12/21 Test #12: tst_AgendaListModel ..............   Passed    0.04 sec
      Start 13: tst_ApplicationTableModel
13/21 Test #13: tst_ApplicationTableModel ........   Passed    0.07 sec
      Start 14: tst_ApplicationFilterProxy
14/21 Test #14: tst_ApplicationFilterProxy .......   Passed    0.03 sec
      Start 15: tst_TopicListModel
15/21 Test #15: tst_TopicListModel ...............   Passed    0.20 sec
      Start 16: tst_TopicBoardProxy
16/21 Test #16: tst_TopicBoardProxy ..............   Passed    0.11 sec
      Start 17: tst_TopicTableProxy
17/21 Test #17: tst_TopicTableProxy ..............   Passed    0.13 sec
      Start 18: tst_AgendaService
18/21 Test #18: tst_AgendaService ................   Passed    0.15 sec
      Start 19: tst_DayWatcher
19/21 Test #19: tst_DayWatcher ...................   Passed    0.02 sec
      Start 20: tst_ExportService
20/21 Test #20: tst_ExportService ................   Passed    0.01 sec
      Start 21: tst_StatsService
21/21 Test #21: tst_StatsService .................   Passed    2.13 sec

100% tests passed, 0 tests failed out of 21

Total Test time (real) =  78.61 sec
==> Quality check PASSED with zero warnings and all tests green!
```

### B. Test Function Counts (Rule 3)
```
$ for bin in build/dev-linux/tests/*/*; do
    if [ -x "$bin" ] && [ ! -d "$bin" ]; then
        name=$(basename "$bin")
        count=$("$bin" -functions | wc -l)
        echo "$name $count"
    fi
done | sort -u | awk '{print $1, $2; s+=$2} END {print "TOTAL:", s}'

tst_AgendaListModel 5
tst_AgendaService 24
tst_ApplicationFilterProxy 2
tst_ApplicationRepository 18
tst_ApplicationTableModel 6
tst_Database 16
tst_DayWatcher 4
tst_ExportService 4
tst_InterviewRepository 9
tst_MonthView 13
tst_ReminderLogRepository 5
tst_SeedService 5
tst_SessionRepository 10
tst_StatsService 10
tst_TopicBoardProxy 9
tst_TopicListModel 7
tst_TopicRepository 18
tst_TopicTableProxy 6
tst_TrackRepository 8
tst_TransactionBehavior 8
ui_smoke 77
TOTAL: 264
```

Verification arithmetic:
$$\text{Part A Errata Total} + \text{DayWatcher (4)} + \text{ui\_smoke added (13)} = 247 + 4 + 13 = 264$$

### C. Layering Check
```
$ grep -rnE "StatusStyle|Theme::|Tokens|ui/|QtGui|QIcon|QPixmap|QColor|QPainter|QFont" src/domain src/data src/services src/models
src/services/SettingsService.cpp:30:    return m_settings->value(u"ui/sidebarCollapsed"_s, false).toBool();
src/services/SettingsService.cpp:35:    m_settings->setValue(u"ui/sidebarCollapsed"_s, collapsed);
src/services/SettingsService.cpp:40:    return m_settings->value(u"ui/themeMode"_s, u"system"_s).toString();
src/services/SettingsService.cpp:45:    m_settings->setValue(u"ui/themeMode"_s, mode);
src/services/SettingsService.cpp:50:    return m_settings->value(u"ui/accentPreset"_s, u"indigo"_s).toString();
src/services/SettingsService.cpp:55:    m_settings->setValue(u"ui/accentPreset"_s, preset);
src/models/ApplicationTableModel.cpp:105:            // The label lives in StatusStyle, which the status badge delegate paints;
src/models/TopicListModel.h:20:/// are handled by delegates in ui/.
```
Zero layering violations detected.

### D. Hardcoded Colors Check
```
$ grep -rnE "#[0-9a-fA-F]{6}" src/ui
# Matches occur exclusively within src/ui/theme/Tokens.cpp, StatusStyle.cpp, and AgendaKindStyle.cpp.
```
Zero hardcoded colors in widgets. All painting consumes palette colors via `Tokens`.

### E. Fixed Sizes Check
```
$ grep -rnE "setFixedSize|setFixedWidth|setFixedHeight" src/ui
src/ui/widgets/TopicDetailPanel.cpp:88:    m_closeBtn->setFixedSize(28, 28);
src/ui/dialogs/TrackDialog.cpp:133:        btn->setFixedSize(32, 32);
src/ui/dialogs/TrackDialog.cpp:148:    m_customSwatchBtn->setFixedSize(32, 32);
src/ui/Sidebar.cpp:26:        setFixedHeight(42);
src/ui/Sidebar.cpp:173:    m_toggleBtn->setFixedHeight(36);
src/ui/Sidebar.cpp:189:    setFixedWidth(collapsed ? kWidthCollapsed : kWidthExpanded);
```
Zero fixed sizes introduced in new widgets (`StatCard`, `ProgressRing`, `FunnelWidget`, `GoalProgressBar`, `EmptyState`, `HomePage`).

---

## 8. Commit Log

```
0ef7f91 feat(dashboard): implement Part B dashboard UI, widgets and controller
6bf3acb docs(reports): add M5 Part A errata
e8fec90 docs(spec): document Part A repository queries in section 14 (v1.7)
1c0d1a2 docs(prompts): add M5 Part B go
d53a700 docs(reports): add M5 Part A report
07d9bb4 feat(stats): implement Part A domain, data queries, stats and agenda services
b63f774 docs(prompts): add M5 amendments
144361c docs: add M5 plan
1b8e377 right path for m5-plan-request
```

---

## 9. Not Verified by Agent (User to Check)

1. **System Tray Fallback on Native Desktop**: Tests run offscreen with headless display. While `INotifier` and status bar labels update cleanly, testing with an active X11 / Wayland notification daemon or GNOME shell environment requires manual inspection by the user.
2. **Physical HiDPI / Retina Display Scaling**: Tested via `devicePixelRatio` geometry checks in `testWidgetMetricsAndRingWrapping`, but visual fidelity on physical 4K / fractional scale displays (e.g. 125%, 150%) should be inspected by the user.
