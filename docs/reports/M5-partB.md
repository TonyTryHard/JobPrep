# Milestone M5 Part B Report — Dashboard UI (Revised)

## 1. Executive Summary & Plan vs. Result

Milestone M5 Part B delivers the user-facing Dashboard (`HomePage`) in Qt 6 Widgets, covering:
- Part A carry-overs (C0): SPEC §14 updated and committed, Part A errata report committed, follow-up item time confirmed.
- `DayWatcher` service: 60-second periodic timer checking system clock transitions across midnight, emitting `dayChanged(QDate)` with injectable clock seam for deterministic testing.
- `FormatHelper` formatting utility: centralized duration, streak, active processes, interview rate, next interview, and funnel stage formatting in `src/ui/`.
- `DashboardController`: centralized single-owner controller that coalesces domain mutations, specific settings changes, day changes, and page activations into a single 0 ms queued update, exposing test seam `refreshCount()`.
- Four custom-painted dashboard widgets adhering to tokens, DPI awareness, font-metrics sizing, and zero per-paint allocations: `StatCard`, `ProgressRing`, `FunnelWidget`, `GoalProgressBar`.
- `HomePage`: 4 stat cards, Today list (`AgendaListModel` + `AgendaItemDelegate`), Upcoming interviews list, Study progress (progress rings per track + weekly goal bar), Hiring funnel (5 stages + interview rate), and empty state D6 with "+ Add application" and "+ Add topic" dual actions.
- Cross-page navigation: `MainWindow::openApplication(int id)` and `MainWindow::openInterview(int id)` routing into `JobsPage::openApplication(int id)` and `CalendarPage::openInterview(int id)`.
- Live status bar wiring in `MainWindow` (`m_statusStreakLabel`, `m_statusProcessesLabel`, `m_statusNextInterviewLabel`).
- 14 comprehensive UI smoke test slots and 4 screenshot captures.

### Gaps vs. Plan
None found by the agent. All acceptance criteria, amendments B1–B8, navigation routes, delegates, and widgets were implemented, verified, and backed by automated test coverage.

---

## 2. Changes in Round 2 (Fix-ups)

Following prompt `docs/prompts/M5-partB-fixups.md`, the following corrections and verifications were performed:
1. **Interview Rate Calculation Fix**: `FormatHelper::formatInterviewRate` was updated to multiply fractional rates by `100.0` before formatting with one decimal place (`tr("%1%").arg(QString::number(*rate * 100.0, 'f', 1))`).
2. **`GoalProgressBar` Accessors**: Added `realPercent()` and `cappedPercent()` to `GoalProgressBar.h` so tests can directly verify capping at 100% and real percentage calculation.
3. **Empty State Button Testing**: `testDashboardEmptyStateActionButtons` was expanded to test "+ Add topic" both with no tracks (opens `TrackDialog`) and with tracks (navigates to Study page, focuses Backlog quick-add, opens no dialog).
4. **Settings Signals & Triggers Test**: Added test slot `testDashboardIgnoresThemeSidebarAndSplitterChanges` verifying that theme, sidebar collapsed, and splitter changes do not increment `refreshCount()`, while `HomePage::onActivated()` triggers a refresh.
5. **SPEC §14 Part B Additions**: Added missing Part B entries in `docs/SPEC.md` §14 in commit `810ab25`.
6. **Report Accuracy**: Fixed amendment checklist labels to exact names from `docs/prompts/M5-amendments.md`, corrected funnel stage labels (`Applied`, `HR Screen`, `Technical`, `Final`, `Offer`), added raw command outputs, real header grep signatures, and updated "Not verified" section.

---

## 3. Carry-overs from Part A (C0) Outcomes

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

## 4. Amendment Checklist (B1–B8 from `M5-amendments.md`)

### B1. HomePage dependencies and navigation
`HomePage` takes explicit constructor references (`DashboardController&`, `Theme::ThemeManager&`, `QWidget*`), not `AppContext&`. It never calls `MainWindow`: it emits signals which `MainWindow` connects to its own handlers.

Raw grep evidence:
```
$ grep -n -A 10 "signals:" src/ui/pages/HomePage.h
46:signals:
47-    void addApplicationRequested();
48-    void addTopicRequested();
49-    void interviewActivated(int id);
50-    void applicationActivated(int id);
51-    void topicActivated(int id);
52-
53-private:
```
```
$ grep -n -C 5 "addApplicationRequested" src/ui/MainWindow.cpp
316-    connect(m_sidebar, &Sidebar::pageSelected, this, [this](int index) {
317-        updateHeader(index);
318-    });
319-
320-    if (m_homePage) {
321:        connect(m_homePage, &Pages::HomePage::addApplicationRequested, this,
322-                &MainWindow::newApplicationFromHeader);
323-        connect(m_homePage, &Pages::HomePage::addTopicRequested, this,
324-                &MainWindow::newTopicFromHeader);
325-        connect(m_homePage, &Pages::HomePage::interviewActivated, this,
326-                &MainWindow::openInterview);
```
Test evidence: `tst_UiSmoke::testDashboardEmptyStateActionButtons` tests:
- "+ Add application" -> opens exactly one `ApplicationDialog`.
- "+ Add topic" with no tracks -> opens `TrackDialog`.
- "+ Add topic" with tracks -> switches to Study page, Backlog quick-add focused, no dialog.

### B2. MainWindow::openInterview / openApplication
`MainWindow::openInterview(int)` and `MainWindow::openApplication(int)` switch to the owning page stack, select the item, and open the respective dialog without recreating dialogs in `MainWindow`. If an item is deleted in the meantime, a non-fatal status bar message is shown.

Raw grep evidence:
```
$ grep -n -C 3 "openInterview\|openApplication" src/ui/MainWindow.h
41-    void openTopic(int topicId);
42-
43-    /// Switches to the Calendar page and opens the interview editor (B2).
44:    void openInterview(int interviewId);
45-
46-    /// Switches to the Jobs page and opens the application editor (B2).
47:    void openApplication(int applicationId);
48-
49-    Pages::HomePage* homePage() const { return m_homePage; }
50-    DashboardController* dashboardController() const { return m_dashboardController.get(); }
```
Test evidence:
- `tst_UiSmoke::testOpenInterviewAndApplicationFromAnotherPage`: calls both methods directly from another page (e.g. Study page), verifies page switch to Calendar/Jobs and dialog opening; tests deleted IDs with non-fatal message.
- `tst_UiSmoke::testDashboardTodayItemClicksOpenRespectiveDialogs`: activates interview, follow-up, and study target items from the dashboard Today list.

### B3. One coalesced refresh, one computation
All domain `changed()` signals, `SettingsService::weeklyGoalMinutesChanged`, `SettingsService::firstDayOfWeekChanged`, `DayWatcher::dayChanged`, and `HomePage::onActivated()` coalesce into a single 0 ms queued refresh. Theme, sidebar, and splitter changes do NOT trigger a refresh.

Raw grep evidence:
```
$ grep -n -C 5 "signals:" src/services/SettingsService.h
63-    void setStudyViewMode(const QString& mode);
64-
65-    int studyTrackFilter() const;
66-    void setStudyTrackFilter(int trackId);
67-
68:signals:
69-    void themeModeChanged(const QString& mode);
70-    void accentPresetChanged(const QString& preset);
71-    void sidebarCollapsedChanged(bool collapsed);
72-    void weeklyGoalMinutesChanged(int minutes);
73-    void firstDayOfWeekChanged(int day);
```
```
$ grep -n -A 20 "void DashboardController::wireSignals" src/ui/DashboardController.cpp
44:void DashboardController::wireSignals() {
45-    connect(&m_apps, &JobPrep::Data::ApplicationRepository::changed, this,
46-            &DashboardController::requestRefresh);
47-    connect(&m_interviews, &JobPrep::Data::InterviewRepository::changed, this,
48-            &DashboardController::requestRefresh);
49-    connect(&m_sessions, &JobPrep::Data::SessionRepository::changed, this,
50-            &DashboardController::requestRefresh);
51-    connect(&m_topics, &JobPrep::Data::TopicRepository::changed, this,
52-            &DashboardController::requestRefresh);
53-    connect(&m_tracks, &JobPrep::Data::TrackRepository::changed, this,
54-            &DashboardController::requestRefresh);
55-
56-    connect(&m_settings, &JobPrep::Services::SettingsService::weeklyGoalMinutesChanged, this,
57-            &DashboardController::requestRefresh);
58-    connect(&m_settings, &JobPrep::Services::SettingsService::firstDayOfWeekChanged, this,
59-            &DashboardController::requestRefresh);
60-
61-    connect(&m_dayWatcher, &JobPrep::Services::DayWatcher::dayChanged, this,
62-            &DashboardController::requestRefresh);
63-}
```
Test evidence:
- `tst_UiSmoke::testBurstChangesCoalesceToOneRefresh`: asserts rapid multi-repository burst triggers exactly 1 refresh.
- `tst_UiSmoke::testDashboardIgnoresThemeSidebarAndSplitterChanges`: asserts theme, sidebar, and splitter setting changes do not alter `refreshCount()`, while `homePage()->onActivated()` increments it.

### B4. Day change (DayWatcher, separate from reminders)
A standalone `DayWatcher` in `src/services/` with a 60-second periodic timer emits `dayChanged(QDate)` upon crossing midnight. It takes an injectable clock `Clock` (`std::function<QDate()>`).
Test evidence:
- `tst_DayWatcher` unit test suite (4 tests covering initialization, no change during same day, date rollover across midnight, and manual `checkNow()`).
- `tst_UiSmoke::testDashboardDayChangeRefreshesTodayAndStreak`: verifies calendar day rollover updates Today list and study streak.

### B5. Formatting lives in ui
Centralized in `JobPrep::Ui::FormatHelper`. Covers duration (`4h 20m`), weekly goal progress (`4h 20m / 6h`), streak wording (`1-day streak`, `4-day streak`, `1 day`, `2 days`), active processes (`1 active process`, `8 active processes`), next interview (`Next: Fri 14:00 Globex`, `Next: No upcoming interviews`), and interview rate (`66.7%` or `–`).
Test evidence:
- `tst_UiSmoke::testFormatHelper` tests every helper method, including `6.0 / 9.0` -> `66.7%`, `std::nullopt` -> `–`, 260 -> `4h 20m / 6h`, 430 -> `7h 10m / 6h`, and `GoalProgressBar` capped at 100% with real 119%.

### B6. Today list
Reuses `AgendaListModel` fed from `AgendaService::todayItems(today)`. Rendered using `AgendaItemDelegate`. Overdue items show text badge "Overdue" in `tokens.danger`. Earlier-today interviews dimmed in `tokens.textMuted`. Upcoming interviews show short weekday when date != today.
Test evidence:
- `tst_UiSmoke::testDashboardTodayItemClicksOpenRespectiveDialogs`.

### B7. Widgets
`StatCard`, `ProgressRing`, `FunnelWidget`, `GoalProgressBar`. Tokens only, HiDPI `devicePixelRatio` aware, sizes from font metrics, zero per-paint allocations. Progress rings wrap in rows based on available width; long track names elide with tooltip; funnel renders 5 stages in order (`Applied`, `HR Screen`, `Technical`, `Final`, `Offer`) with chevrons and interview rate below; weekly goal bar caps fill visually at 100% while rendering actual percentage in text.
Test evidence:
- `tst_UiSmoke::testWidgetMetricsAndRingWrapping`: tests 1, 2, and 8 tracks with wrapping and non-intersecting geometry.
- `tst_UiSmoke::testDashboardNumbersMatchHandCalculatedDataset`: verifies real widget stage labels, counts 9/6/4/2/1, and interview rate 66.7%.

### B8. Screenshots
Light and dark screenshots of both populated and empty dashboard states captured offscreen and written to `docs/reports/img/`.
Test evidence: `tst_UiSmoke::testDashboardScreenshots` executed via `scripts/screenshots.sh`.

---

## 5. Funnel Stages and Interview Rate (R2)

The 5 funnel stages correspond to ranks 1..5 defined in `docs/SPEC.md` §6 and returned by `StatsService::computeStats`:
1. `Domain::ApplicationStatus::Applied` (rank 1) -> label `"Applied"`
2. `Domain::ApplicationStatus::HrScreen` (rank 2) -> label `"HR Screen"`
3. `Domain::ApplicationStatus::Technical` (rank 3) -> label `"Technical"`
4. `Domain::ApplicationStatus::Final` (rank 4) -> label `"Final"`
5. `Domain::ApplicationStatus::Offer` (rank 5) -> label `"Offer"`

Interview rate formula:
$$\text{interviewRate} = \frac{\text{reached}(\text{rank} \ge 2)}{\text{reached}(\text{rank} \ge 1)}$$
When $\text{reached}(\text{rank} \ge 1) = 0$, `interviewRate` is `std::nullopt`.
How "no data" reaches the widget:
`StatsService` sets `stats.interviewRate = std::nullopt` -> `DashboardController` copies to `snapshot.stats.interviewRate` -> `HomePage` passes `const std::optional<double>&` to `FunnelWidget::setFunnel` -> `FormatHelper::formatInterviewRate(m_interviewRate)` returns `QString::fromUtf8("\u2013")` (`–`, en-dash).

Raw grep for labels in `src/ui`:
```
$ grep -rn "Accepted\|Final" src/ui
src/ui/widgets/TopicListView.cpp:35:            return event->isAccepted();
src/ui/widgets/TopicListView.cpp:38:            return event->isAccepted();
src/ui/widgets/TopicListView.cpp:41:            return event->isAccepted();
src/ui/widgets/TopicListView.cpp:44:            return event->isAccepted();
src/ui/widgets/TopicListView.cpp:56:                return event->isAccepted();
src/ui/widgets/TopicListView.cpp:59:                return event->isAccepted();
src/ui/widgets/TopicListView.cpp:62:                return event->isAccepted();
src/ui/widgets/TopicListView.cpp:65:                return event->isAccepted();
src/ui/widgets/TopicListView.cpp:77:            return event->isAccepted();
src/ui/widgets/TopicListView.cpp:80:            return event->isAccepted();
src/ui/widgets/TopicListView.cpp:83:            return event->isAccepted();
src/ui/widgets/TopicListView.cpp:86:            return event->isAccepted();
src/ui/dialogs/InterviewDialog.cpp:35:                                InterviewType::Behavioral, InterviewType::Final,
src/ui/dialogs/InterviewDialog.cpp:45:            ApplicationStatus::Final,    ApplicationStatus::Offer};
src/ui/dialogs/ApplicationDialog.cpp:39:    ApplicationStatus::Technical, ApplicationStatus::Final,    ApplicationStatus::Offer,
src/ui/dialogs/ApplicationDialog.cpp:40:    ApplicationStatus::Accepted,  ApplicationStatus::Rejected, ApplicationStatus::Withdrawn,
src/ui/dialogs/ApplicationDialog.cpp:511:    if (dialog.exec() == QDialog::Accepted) reloadInterviewsTable();
src/ui/dialogs/ApplicationDialog.cpp:522:    if (dialog.exec() == QDialog::Accepted) reloadInterviewsTable();
src/ui/dialogs/FollowUpDialog.cpp:102:                            app.status == Domain::ApplicationStatus::Accepted;
src/ui/pages/JobsPage.cpp:50:    ApplicationStatus::Technical, ApplicationStatus::Final,    ApplicationStatus::Offer,
src/ui/pages/JobsPage.cpp:51:    ApplicationStatus::Accepted,  ApplicationStatus::Rejected, ApplicationStatus::Withdrawn,
src/ui/pages/JobsPage.cpp:399:    if (dialog.exec() != QDialog::Accepted) return;
src/ui/pages/JobsPage.cpp:416:    if (dialog.exec() != QDialog::Accepted) return;
src/ui/theme/StatusStyle.cpp:18:        case Domain::ApplicationStatus::Final:
src/ui/theme/StatusStyle.cpp:22:        case Domain::ApplicationStatus::Accepted:
src/ui/theme/StatusStyle.cpp:44:        case Domain::ApplicationStatus::Final:
src/ui/theme/StatusStyle.cpp:45:            return QCoreApplication::translate("StatusStyle", "Final");
src/ui/theme/StatusStyle.cpp:48:        case Domain::ApplicationStatus::Accepted:
src/ui/theme/StatusStyle.cpp:49:            return QCoreApplication::translate("StatusStyle", "Accepted");
src/ui/theme/StatusStyle.cpp:124:        case Domain::InterviewType::Final:
src/ui/theme/StatusStyle.cpp:125:            return QCoreApplication::translate("StatusStyle", "Final");
```

---

## 6. Real Header Signatures (Public API)

### `src/ui/widgets/StatCard.h`
```cpp
    explicit StatCard(Theme::ThemeManager& theme, QWidget* parent = nullptr);
    void setTitle(const QString& title);
    void setValue(const QString& value);
    void setSubtitle(const QString& subtitle = QString());
    QString title() const;
    QString value() const;
    QString subtitle() const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
```

### `src/ui/widgets/ProgressRing.h`
```cpp
    explicit ProgressRing(Theme::ThemeManager& theme, QWidget* parent = nullptr);
    void setTracks(const QList<Services::TrackProgressStat>& tracks);
    void setTrack(const Services::TrackProgressStat& track);
    const QList<Services::TrackProgressStat>& tracks() const;
    int columnsForWidth(int width) const;
    QRect ringRect(int index, int width) const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
```

### `src/ui/widgets/FunnelWidget.h`
```cpp
    explicit FunnelWidget(Theme::ThemeManager& theme, QWidget* parent = nullptr);
    void setFunnel(const QList<Services::FunnelStage>& stages,
                   const std::optional<double>& interviewRate);
    const QList<Services::FunnelStage>& stages() const;
    std::optional<double> interviewRate() const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
```

### `src/ui/widgets/GoalProgressBar.h`
```cpp
    explicit GoalProgressBar(Theme::ThemeManager& theme, QWidget* parent = nullptr);
    void setGoal(int weekMinutes, int goalMinutes);
    int weekMinutes() const;
    int goalMinutes() const;
    int realPercent() const;
    int cappedPercent() const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
```

### `src/services/DayWatcher.h`
```cpp
    explicit DayWatcher(Clock clock = [] { return QDate::currentDate(); },
                        QObject* parent = nullptr);
    QDate currentDate() const;
    void setClock(Clock clock);
    void checkNow();
signals:
    void dayChanged(QDate newDate);
```

### `src/ui/DashboardController.h`
```cpp
    DashboardController(JobPrep::Services::StatsService& statsService,
                        JobPrep::Services::AgendaService& agendaService,
                        JobPrep::Services::SettingsService& settingsService,
                        JobPrep::Services::DayWatcher& dayWatcher,
                        JobPrep::Data::ApplicationRepository& apps,
                        JobPrep::Data::InterviewRepository& interviews,
                        JobPrep::Data::SessionRepository& sessions,
                        JobPrep::Data::TopicRepository& topics,
                        JobPrep::Data::TrackRepository& tracks,
                        QObject* parent = nullptr);
    const DashboardSnapshot& snapshot() const;
    int refreshCount() const;
public slots:
    void requestRefresh();
    void refreshNow();
signals:
    void snapshotChanged(const JobPrep::Ui::DashboardSnapshot& snapshot);
```

### `src/ui/pages/HomePage.h`
```cpp
    explicit HomePage(DashboardController& controller,
                      Theme::ThemeManager& theme,
                      QWidget* parent = nullptr);
    void onActivated() override;
signals:
    void addApplicationRequested();
    void addTopicRequested();
    void interviewActivated(int id);
    void applicationActivated(int id);
    void topicActivated(int id);
```

### `src/ui/FormatHelper.h`
```cpp
    static QString formatDuration(int totalMinutes);
    static QString formatGoalText(int weekMinutes, int goalMinutes);
    static QString formatInterviewRate(const std::optional<double>& rate);
    static QString formatStreak(int streakDays);
    static QString formatStreakDays(int streakDays);
    static QString formatActiveProcesses(int count);
    static QString formatNextInterview(const std::optional<Domain::Interview>& interview,
                                       const QString& companyName,
                                       const QLocale& locale = QLocale());
    static QString formatNextInterview(const std::optional<Domain::AgendaItem>& item,
                                       const QLocale& locale = QLocale());
    static QString funnelStageLabel(Domain::ApplicationStatus stage);
```

### `src/ui/pages/JobsPage.h`
```cpp
    bool openApplication(int id);
```

### `src/ui/pages/CalendarPage.h`
```cpp
    bool openInterview(int id);
```

### `src/ui/MainWindow.h`
```cpp
    void openTopic(int topicId);
    void openInterview(int interviewId);
    void openApplication(int applicationId);
```

---

## 7. Mapping of Test Slots to `M5-partB-go.md`

Raw slot diff output:
```
$ git diff d53a700..cbefc94 -- tests/ui_smoke/tst_UiSmoke.cpp | grep -E '^\+.*void test'
+    void testFormatHelper();
+    void testDashboardVisitedBothThemes();
+    void testDashboardNumbersMatchHandCalculatedDataset();
+    void testDashboardEmptyStateActionButtons();
+    void testDashboardTodayItemClicksOpenRespectiveDialogs();
+    void testOpenInterviewAndApplicationFromAnotherPage();
+    void testDashboardDayChangeRefreshesTodayAndStreak();
+    void testBurstChangesCoalesceToOneRefresh();
+    void testWeeklyGoalChangeUpdatesBarAndTextImmediately();
+    void testFirstDayOfWeekChangeRefreshesWeekNumbers();
+    void testStatusBarTextScenarios();
+    void testWidgetMetricsAndRingWrapping();
+    void testDashboardIgnoresThemeSidebarAndSplitterChanges();
+    void testDashboardScreenshots();
```

| Slot Name | Requirement from `M5-partB-go.md` | Verification Summary |
|---|---|---|
| `testFormatHelper` | Goal text, "–" for empty rate, streak wording, durations | Tests 260->4h 20m, 430->7h 10m, 6/9->66.7%, nullopt->–, 1 vs 2 days, and GoalProgressBar 119% real / 100% capped |
| `testDashboardVisitedBothThemes` | Dashboard visited in light and dark | Instantiates MainWindow, visits HomePage in both themes, asserts no crashes/warnings |
| `testDashboardNumbersMatchHandCalculatedDataset` | Numbers equal Part A dataset (applied 2, active 8, funnel 9/6/4/2/1, rate 6/9, 260 min) | Verifies snapshot and real widget stage labels, counts 9,6,4,2,1, and rate 66.7% |
| `testDashboardEmptyStateActionButtons` | Empty-state buttons D6 | Tests "+ Add application" -> ApplicationDialog; "+ Add topic" (no tracks) -> TrackDialog; "+ Add topic" (with tracks) -> Study page quick-add |
| `testDashboardTodayItemClicksOpenRespectiveDialogs` | Today item opens interview / follow-up / topic and each deleted in meantime | Clicks each item kind, verifies respective dialog opened; deletes each item, verifies non-fatal status message |
| `testOpenInterviewAndApplicationFromAnotherPage` | `openInterview` / `openApplication` directly | Switches to StudyPage/CalendarPage, calls MainWindow navigation methods, verifies stack switch and dialog |
| `testDashboardDayChangeRefreshesTodayAndStreak` | Day change with injected clock refreshes Today and streak | Injects clock rolling from Oct 20 to Oct 21, calls `checkNow()`, verifies Today list and streak updated |
| `testBurstChangesCoalesceToOneRefresh` | Burst of `changed()` -> one refresh | Rapidly fires changes on all 5 repositories, asserts `refreshCount()` increases by exactly 1 |
| `testWeeklyGoalChangeUpdatesBarAndTextImmediately` | Goal 6 h -> 8 h updates immediately | Modifies `weeklyGoalMinutes` in settings, asserts snapshot and bar update immediately |
| `testFirstDayOfWeekChangeRefreshesWeekNumbers` | First day of week Sunday/Monday recalculates | Modifies `firstDayOfWeek` between Monday (1) and Sunday (7), asserts week study minutes change from 260 to 290 |
| `testStatusBarTextScenarios` | Status bar scenarios (no data, streak 1 and 4, no upcoming interview, next interview) | Verifies all 4 status bar label formats across empty, active streak, and upcoming interview data |
| `testWidgetMetricsAndRingWrapping` | Widget checks with 1, 2 and 8 tracks | Verifies non-overlapping ring bounding rects in rows and wrap calculations |
| `testDashboardIgnoresThemeSidebarAndSplitterChanges` | Settings signals ignore non-dashboard settings | Verifies theme, sidebar, and splitter changes do not increment `refreshCount()`, while `onActivated()` refreshes |
| `testDashboardScreenshots` | Capture 4 screenshots (light/dark with/without data) | Captures png files into `docs/reports/img/` |

---

## 8. SPEC §14 (1.7) for Part B

Raw check of commit `0ef7f91`:
```
$ git show --stat 0ef7f91 | grep SPEC
# (No changes to SPEC in commit 0ef7f91)
```

Added in separate commit `810ab25` (`docs(spec): document Part B components and navigation in section 14 (v1.7)`):
```
$ grep -n "^- 1\.7" docs/SPEC.md | cut -c1-140
410:- 1.7 — `domain::stageRank`: pure domain helper mapping `ApplicationStatus` to stage rank 0..5 (wishlist 0, applied 1, hr_screen 2, tech
411:- 1.7 — `DashboardStats` in `src/services/StatsService.h` holds numeric and domain types only (`appliedThisWeek`, `activeProcesses`, `in
412:- 1.7 — "Interviews in next 7 days" window is defined as `[now, QDateTime(today.addDays(6), 23:59:59)]`, strictly covering today plus th
413:- 1.7 — Only `outcome == cancelled` excludes an interview from next 7 days, Upcoming, Today, status bar "Next:", and reminders. Intervie
414:- 1.7 — Study streak counts consecutive days with sessions strictly up to and including `today`; future-dated sessions neither count tow
415:- 1.7 — `AgendaService::todayItems` returns today's non-cancelled interviews, overdue/due follow-ups, and overdue/due open topics. Deter
416:- 1.7 — `AgendaService::upcomingInterviews` returns upcoming non-cancelled interviews starting on or after `from`, ordered by start_at, 
417:- 1.7 — `InterviewRepository::inRange` behavior is untouched. `countUpcoming(from, to)` and `upcoming(from, limit)` added to data layer,
418:- 1.7 — Data layer queries for stats and agenda: `ApplicationRepository::appliedCountInRange`, `funnelStageCounts`, `byNextActionDateUpT
419:- 1.7 — `DayWatcher` in `src/services/`: periodic 60-second timer service detecting calendar day rollover across midnight, emitting `day
420:- 1.7 — `DashboardController` in `src/ui/`: centralized refresh owner batching mutations from repositories (`apps`, `interviews`, `sessio
421:- 1.7 — `FormatHelper` in `src/ui/`: presentation formatting utility for durations, weekly goal text, streak wording, interview rates (`
422:- 1.7 — `GoalProgressBar` in `src/ui/widgets/`: custom-painted horizontal widget for weekly study goal progress; visual fill clamped to 
423:- 1.7 — `EmptyState` in `src/ui/widgets/`: supports optional secondary action button for dual quick-actions (`+ Add application` and `+ 
424:- 1.7 — `MainWindow::openInterview(int)` and `MainWindow::openApplication(int)`: switch page stack to owning page (`CalendarPage` or `Jo
```

---

## 9. Screenshots and Checksums

Raw screenshot regeneration output:
```
$ ./scripts/screenshots.sh
==> Building ui_smoke...
ninja: no work to do.
==> Running screenshot generation with JOBPREP_REPORT_IMG_DIR=/home/tony/projects/JobPrep/docs/reports/img...
********* Start testing of tst_UiSmoke *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_UiSmoke::initTestCase()
PASS   : tst_UiSmoke::testCalendarScreenshots()
PASS   : tst_UiSmoke::testStudyScreenshots()
PASS   : tst_UiSmoke::testDashboardScreenshots()
PASS   : tst_UiSmoke::cleanupTestCase()
Totals: 5 passed, 0 failed, 0 skipped, 0 blacklisted, 26123ms
********* Finished testing of tst_UiSmoke *********
==> Screenshots successfully regenerated in /home/tony/projects/JobPrep/docs/reports/img!
```

Raw checksums:
```
$ md5sum docs/reports/img/dashboard-*.png
438c385cf22454618ceef6d45d1de11a  docs/reports/img/dashboard-empty-dark.png
3eac4e19d905b45031b9b3b29d5566ab  docs/reports/img/dashboard-empty-light.png
940d634424fefeaa4e81afbbe061a90e  docs/reports/img/dashboard-with-data-dark.png
961b36a6e8c1e758e4a695faf68df3bc  docs/reports/img/dashboard-with-data-light.png
```

Raw git status after checkout of untouched earlier screenshots:
```
$ git status --short
?? docs/prompts/M5-partB-fixups.md
```

---

## 10. Raw Verification Evidence

### A. Full Quality Check (`CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh`)
```
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.4s)
-- Generating done (0.3s)
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
 5/21 Test  #5: tst_ApplicationRepository ........   Passed    0.14 sec
      Start  6: tst_InterviewRepository
 6/21 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start  7: tst_ReminderLogRepository
 7/21 Test  #7: tst_ReminderLogRepository ........   Passed    0.02 sec
      Start  8: tst_SeedService
 8/21 Test  #8: tst_SeedService ..................   Passed    0.82 sec
      Start  9: tst_TransactionBehavior
 9/21 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start 10: ui_smoke
10/21 Test #10: ui_smoke .........................   Passed   81.38 sec
      Start 11: tst_MonthView
11/21 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 12: tst_AgendaListModel
12/21 Test #12: tst_AgendaListModel ..............   Passed    0.04 sec
      Start 13: tst_ApplicationTableModel
13/21 Test #13: tst_ApplicationTableModel ........   Passed    0.08 sec
      Start 14: tst_ApplicationFilterProxy
14/21 Test #14: tst_ApplicationFilterProxy .......   Passed    0.03 sec
      Start 15: tst_TopicListModel
15/21 Test #15: tst_TopicListModel ...............   Passed    0.26 sec
      Start 16: tst_TopicBoardProxy
16/21 Test #16: tst_TopicBoardProxy ..............   Passed    0.13 sec
      Start 17: tst_TopicTableProxy
17/21 Test #17: tst_TopicTableProxy ..............   Passed    0.14 sec
      Start 18: tst_AgendaService
18/21 Test #18: tst_AgendaService ................   Passed    0.15 sec
      Start 19: tst_DayWatcher
19/21 Test #19: tst_DayWatcher ...................   Passed    0.02 sec
      Start 20: tst_ExportService
20/21 Test #20: tst_ExportService ................   Passed    0.02 sec
      Start 21: tst_StatsService
21/21 Test #21: tst_StatsService .................   Passed    2.28 sec

100% tests passed, 0 tests failed out of 21

Total Test time (real) =  85.99 sec
==> Quality check PASSED with zero warnings and all tests green!
```

### B. Per-Binary Test Function Counts (Rule 3)
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
ui_smoke 78
TOTAL: 265
```

Arithmetic check:
$$\text{Part A Errata Total (247)} + \text{DayWatcher (4)} + \text{ui\_smoke added (14)} = 247 + 4 + 14 = 265$$

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

### D. Hardcoded Colors Check Outside Theme
```
$ grep -rnE "#[0-9a-fA-F]{6}" src/ui | grep -v "^src/ui/theme/"
```
Zero lines returned (exit code 1). All colors reside strictly within `src/ui/theme/`.

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
Zero fixed sizes introduced in new Part B widgets.

### F. Original Implementation Commit Stat (`0ef7f91`)
```
$ git show --stat 0ef7f91
commit 0ef7f91652ada043faebb94918bd829390bda90d
Author: Tony <group.ik31@gmail.com>
Date:   Sat Oct 24 20:21:11 2026 +0300

    feat(dashboard): implement Part B dashboard UI, widgets and controller

 CMakeLists.txt                                 |  14 +
 docs/reports/img/dashboard-empty-dark.png      | Bin 0 -> 23754 bytes
 docs/reports/img/dashboard-empty-light.png     | Bin 0 -> 23323 bytes
 docs/reports/img/dashboard-with-data-dark.png  | Bin 0 -> 65756 bytes
 docs/reports/img/dashboard-with-data-light.png | Bin 0 -> 64673 bytes
 scripts/screenshots.sh                         |   2 +-
 src/app/AppContext.cpp                         |  13 +
 src/app/AppContext.h                           |   6 +
 src/services/DayWatcher.cpp                    |  33 ++
 src/services/DayWatcher.h                      |  38 ++
 src/ui/DashboardController.cpp                 |  97 +++++
 src/ui/DashboardController.h                   |  94 +++++
 src/ui/FormatHelper.cpp                        |  78 ++++
 src/ui/FormatHelper.h                          |  51 +++
 src/ui/MainWindow.cpp                          |  62 ++-
 src/ui/MainWindow.h                            |  15 +
 src/ui/delegates/AgendaItemDelegate.cpp        |  29 +-
 src/ui/pages/CalendarPage.cpp                  |  13 +
 src/ui/pages/CalendarPage.h                    |   3 +
 src/ui/pages/HomePage.cpp                      | 281 ++++++++++++-
 src/ui/pages/HomePage.h                        |  83 +++-
 src/ui/pages/JobsPage.cpp                      |  11 +
 src/ui/pages/JobsPage.h                        |   3 +
 src/ui/widgets/EmptyState.cpp                  |  18 +-
 src/ui/widgets/EmptyState.h                    |   3 +
 src/ui/widgets/FunnelWidget.cpp                | 165 ++++++++
 src/ui/widgets/FunnelWidget.h                  |  43 ++
 src/ui/widgets/GoalProgressBar.cpp             |  98 +++++
 src/ui/widgets/GoalProgressBar.h               |  39 ++
 src/ui/widgets/ProgressRing.cpp                | 179 ++++++++
 src/ui/widgets/ProgressRing.h                  |  46 ++
 src/ui/widgets/StatCard.cpp                    | 127 ++++++
 src/ui/widgets/StatCard.h                      |  43 ++
 tests/services/CMakeLists.txt                  |   1 +
 tests/services/tst_DayWatcher.cpp              |  72 ++++
 tests/ui_smoke/tst_UiSmoke.cpp                 | 556 +++++++++++++++++++++++++
 36 files changed, 2295 insertions(+), 21 deletions(-)
```

---

## 11. Git Commit Log

```
$ git log -n 8 --oneline
810ab25 docs(spec): document Part B components and navigation in section 14 (v1.7)
cbefc94 fix(dashboard): fix interview rate calculation, add goal progress bar helpers and ui smoke slots
655cdf1 docs(reports): add M5 Part B report
0ef7f91 feat(dashboard): implement Part B dashboard UI, widgets and controller
6bf3acb docs(reports): add M5 Part A errata
e8fec90 docs(spec): document Part A repository queries in section 14 (v1.7)
1c0d1a2 docs(prompts): add M5 Part B go
d53a700 docs(reports): add M5 Part A report
```

---

## 12. Not Verified by Agent (User to Check)

1. **Dashboard visual appearance at 100–200% scaling**: Verify widget text readability, ring crispness, and funnel spacing in both light and dark themes on native desktop displays.
2. **Real click-through of Today and Upcoming list items**: Confirm that double-clicking or pressing Enter on an item opens the corresponding dialog and selects the item in the underlying page table/month view.
3. **Status bar layout at narrow window width**: Test window resizing down to minimum 1100x700 px to verify status bar labels do not clip or cause layout overflow.
4. **Midnight rollover by hand**: Test leaving the application open overnight across 00:00 to verify that `DayWatcher` triggers and updates Today items, streak card, and status bar without restart.
5. **Sunday-first week setting on dashboard**: Change First Day of Week to Sunday in settings and verify that the weekly goal minutes immediately re-evaluate.
6. **Empty dashboard experience on a fresh app data folder**: Launch the app with an empty SQLite database and verify the dual-action empty state cards and buttons ("+ Add application" and "+ Add topic").
