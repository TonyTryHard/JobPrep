# M5 Plan — Dashboard & Reminders

**Status:** Planned; awaiting approval to execute.  
**Deliverable:** `docs/plans/M5-plan.md`  
**Target Milestone:** Milestone M5 (SPEC §12, §3.1, §3.5, §3.7, §4, §6, §7.1, §7.2, §8, §10, §11, §14)  
**Stack & Environment:** Linux x86_64, Qt 6.10.2 (system, `/usr/lib/x86_64-linux-gnu/cmake/Qt6`), GCC 15.2.0, CMake 3.21+, Ninja, `dev-linux` preset, `JOBPREP_WERROR=ON`.

---

## 1. Pre-Flight Findings (Verified Against Existing Code)

Every statement in this section is backed by commands executed during this session.

| # | Topic | Existing Code Finding | Command & Verification |
|---|---|---|---|
| 1 | `AgendaService` Public API | Takes `InterviewRepository&`, `ApplicationRepository&`, `TopicRepository&`, `TrackRepository&`, and an optional `Clock` (`std::function<QDateTime()>`). Public methods: `setClock(Clock)` and `QList<Domain::AgendaItem> items(QDate from, QDate to) const`. | `cat src/services/AgendaService.h`<br>```cpp
class AgendaService {
public:
    using Clock = std::function<QDateTime()>;
    explicit AgendaService(Data::InterviewRepository& interviews,
                           Data::ApplicationRepository& apps,
                           Data::TopicRepository& topics,
                           Data::TrackRepository& tracks,
                           Clock clock = [] { return QDateTime::currentDateTime(); });
    void setClock(Clock clock);
    QList<Domain::AgendaItem> items(QDate from, QDate to) const;
...
};
``` |
| 2 | `AgendaService` Overdue & Range Behavior | In `items(from, to)`, interviews use `[from 00:00:00, to 23:59:59]`, follow-ups use `m_apps.byNextActionDateRange(from, to)`, and study targets use `m_topics.byTargetDateRange(from, to)`. **Overdue follow-ups and study targets before `from` are excluded by `items(from, to)`** because SQL filters `next_action_date >= from` and `target_date >= from`. | `sed -n '75,115p' src/services/AgendaService.cpp`<br>```cpp
    const QDateTime ivFrom(from, QTime(0, 0, 0));
    const QDateTime ivTo(to, QTime(23, 59, 59));
    const auto interviews = m_interviews.inRange(ivFrom, ivTo);
    for (const auto& iv : interviews) {
        if (iv.outcome == Domain::InterviewOutcome::Cancelled) continue;
        ...
    }
    const auto followUpApps = m_apps.byNextActionDateRange(from, to);
    ...
    const auto targetTopics = m_topics.byTargetDateRange(from, to);
``` |
| 3 | `AgendaService` Done / Closed / Cancelled | - **Done topics**: excluded in SQL by `TopicRepository::byTargetDateRange` (`status != 'done'`).<br>- **Closed applications**: excluded in SQL by `ApplicationRepository::byNextActionDateRange` (`status NOT IN ('rejected', 'withdrawn', 'ghosted', 'accepted')`).<br>- **Cancelled interviews**: filtered in memory in `AgendaService::items` (`if (iv.outcome == Domain::InterviewOutcome::Cancelled) continue;`). | `sed -n '169,172p' src/data/ApplicationRepository.cpp`<br>```cpp
selectApplications() +
    u" WHERE next_action_date >= ? AND next_action_date <= ? "
    u"AND status NOT IN ('rejected', 'withdrawn', 'ghosted', 'accepted') "
    u"ORDER BY next_action_date, id"_s
```<br>`sed -n '121,125p' src/data/TopicRepository.cpp`<br>```cpp
selectTopics() +
    u" WHERE target_date >= ? AND target_date <= ? "
    u"AND status != 'done' "
    u"ORDER BY target_date, position, id"_s
``` |
| 4 | `AgendaItem` Fields | Contains `AgendaKind kind`, `QDate date`, `std::optional<QTime> time`, `QString title`, `QString subtitle`, `int refId`, and `bool past`. | `sed -n '115,125p' src/domain/Structs.h`<br>```cpp
struct AgendaItem {
    AgendaKind            kind{AgendaKind::Interview};
    QDate                 date;
    std::optional<QTime>  time;
    QString               title;
    QString               subtitle;
    int                   refId{0};
    bool                  past{false};
};
``` |
| 5 | `ApplicationRepository` Existing Queries & Missing for §6 | **Existing:** `all()`, `byId(int)`, `byStatuses(...)`, `byNextActionDateRange(...)`, `count()`, `isEmpty()`, `countsByStatus()` (returns `QHash<ApplicationStatus, int>`), `statusHistory(int)`, `allStatusHistory()`.<br>**Missing for §6:** SQL aggregate query for `appliedCountInRange(QDate from, QDate to)` and aggregate query for hiring funnel stage counts (applications reaching rank 1..5 via current status or `status_history`). Loading `all()` or `allStatusHistory()` into memory violates §11 scalability. | `grep -n "countsByStatus\|statusHistory\|byNextActionDateRange" src/data/ApplicationRepository.h`<br>```
43:    QList<Domain::JobApplication> byNextActionDateRange(QDate from, QDate to) const;
63:    QHash<Domain::ApplicationStatus, int> countsByStatus() const;
65:    QList<Domain::StatusChange> statusHistory(int applicationId) const;
66:    QList<Domain::StatusChange> allStatusHistory() const;
``` |
| 6 | `InterviewRepository` Queries & Missing for §6 | **Existing:** `all()`, `byApplication(int)`, `inRange(QDateTime, QDateTime)`, `byId(int)`, `nextUpcoming(QDateTime)`.<br>**Note / Missing:** `inRange` and `nextUpcoming` in `InterviewRepository` do NOT filter out cancelled interviews in SQL (`outcome != 'cancelled'`). For "Interviews in next 7 days" (D1) and status bar next interview, non-cancelled filtering is required. | `sed -n '105,111p' src/data/InterviewRepository.cpp`<br>```cpp
std::optional<Domain::Interview> InterviewRepository::nextUpcoming(const QDateTime& now) const {
    SqlStatement statement(m_database, selectInterviews() +
                                          u" WHERE start_at >= ? ORDER BY start_at, id LIMIT 1"_s);
    statement.bind(1, now);
    if (!statement.exec() || !statement.query().next()) return std::nullopt;
    return readInterview(m_database, statement.query());
}
``` |
| 7 | `SessionRepository` Queries & Missing for §6 | **Existing:** `all()`, `byTopic(int)`, `inRange(QDate, QDate)`, `byId(int)`, `count()`, `totalMinutesInRange(QDate from, QDate to)` (uses SQL `SUM(minutes)`).<br>**Missing for §6:** Distinct session dates query for study streak (`distinctDates(QDate upTo)`). Currently, finding streak requires fetching all sessions into memory. | `sed -n '141,149p' src/data/SessionRepository.cpp`<br>```cpp
int SessionRepository::totalMinutesInRange(QDate from, QDate to) const {
    SqlStatement statement(m_database,
                           u"SELECT COALESCE(SUM(minutes), 0) FROM study_sessions "
                           u"WHERE session_date >= ? AND session_date <= ?"_s);
    statement.bind(1, from);
    statement.bind(2, to);
    if (!statement.exec() || !statement.query().next()) return 0;
    return statement.query().value(0).toInt();
}
``` |
| 8 | `TopicRepository` Queries for Track Progress | **Existing:** `all()`, `byTrack(int)`, `byTargetDateRange(QDate, QDate)`, `byId(int)`, `subtaskProgress()` (returns `QHash<int, TopicSubtaskProgress>` in a single batched query), `domain::topicProgress(...)` in `src/domain/TopicProgress.h`.<br>**Sufficient:** Batched query already exists, avoiding N+1 queries for track progress. | `grep -n "subtaskProgress" src/data/TopicRepository.h`<br>```
47:    QHash<int, TopicSubtaskProgress> subtaskProgress() const;
```<br>`cat src/domain/TopicProgress.h`<br>```cpp
int topicProgress(TopicStatus status, int doneSubtasks, int totalSubtasks);
``` |
| 9 | `ReminderLogRepository` Existing & Reschedule Gap | **Existing:** `wasSent(QStringView key)`, `markSent(const QString& key, const QDateTime& sentAt)`, `remove(const QString& key)`, `sentKeys()`.<br>**Gap:** Only supports single key operations. If an interview is rescheduled, invalidating sent keys requires either prefix removal (`removeByPrefix`) or `removeForInterview(int id)`. | `cat src/data/ReminderLogRepository.h`<br>```cpp
    bool wasSent(QStringView key) const;
    bool markSent(const QString& key, const QDateTime& sentAt = QDateTime());
    bool remove(const QString& key);
    QList<QString> sentKeys() const;
``` |
| 10 | `SettingsService` Keys Status | **Existing:**<br>- `study/weeklyGoalMinutes` (default 360, signal `weeklyGoalMinutesChanged`)<br>- `calendar/firstDayOfWeek` (default 1, signal `firstDayOfWeekChanged`)<br>- `app/closeToTray` (default false, signal `closeToTrayChanged`)<br>**Missing:** Reminder lead times setting (e.g. `reminders/interviewLeadMinutes`). Currently non-existent. | `grep -n "weeklyGoalMinutes\|firstDayOfWeek\|closeToTray" src/services/SettingsService.h`<br>```
35:    int weeklyGoalMinutes() const;
38:    int firstDayOfWeek() const;
41:    bool closeToTray() const;
```<br>`grep -i "reminder" src/services/SettingsService.h || echo "NOT_FOUND"`<br>```
NOT_FOUND
``` |
| 11 | `MainWindow` Status Bar, Navigation & Toast | - **Status bar:** Labels `m_statusStreakLabel`, `m_statusProcessesLabel`, `m_statusNextInterviewLabel` created in `setupUi` (lines 153–161) but **never updated**.<br>- **`openTopic(int)`:** exists at lines 257–266; switches to Study page and calls `m_studyPage->selectTopic(topicId)`.<br>- **Cross-page item open:** No public `openInterview(int)` or `openApplication(int)` on `MainWindow`.<br>- **Toast mechanism:** No `Toast` class exists in `src/`. Only `statusBar()->showMessage(...)`.<br>- **Window raise:** No `raise()` call exists in `src/`. `tst_UiSmoke.cpp:99` filters `does not support raise` under offscreen. | `sed -n '153,161p' src/ui/MainWindow.cpp`<br>```cpp
    m_statusStreakLabel = new QLabel(tr("0-day streak"), this);
    m_statusProcessesLabel = new QLabel(tr("0 active processes"), this);
    m_statusNextInterviewLabel = new QLabel(tr("Next: No upcoming interviews"), this);
    status->addWidget(m_statusStreakLabel);
    status->addWidget(new QLabel(u" | "_s, this));
    status->addWidget(m_statusProcessesLabel);
    status->addWidget(new QLabel(u" | "_s, this));
    status->addWidget(m_statusNextInterviewLabel);
```<br>`grep -rn "Toast" src/ || echo "NO_TOAST"`<br>```
NO_TOAST
```<br>`grep -rn "raise" src/ || echo "NO_RAISE"`<br>```
NO_RAISE
``` |
| 12 | `HomePage` Construction & Wiring | `HomePage::HomePage(QWidget* parent)` creates a static `Widgets::EmptyState` in a `QVBoxLayout`. `MainWindow.cpp:126` appends `new Pages::HomePage(m_pageStack)` with no dependencies passed. `HomePage` does NOT override `PageBase::onActivated()`. `MainWindow::updateHeader` invokes `activePage->onActivated()` whenever a page is switched to. | `cat src/ui/pages/HomePage.cpp`<br>```cpp
HomePage::HomePage(QWidget* parent)
    : PageBase(tr("Dashboard"), parent) {
    auto* layout = new QVBoxLayout(this);
    ...
}
```<br>`sed -n '298,302p' src/ui/MainWindow.cpp`<br>```cpp
    m_pageStack->setCurrentIndex(pageIndex);
    auto* activePage = m_pages[pageIndex];
    m_pageTitleLabel->setText(activePage->pageTitle());
    activePage->onActivated();
``` |
| 13 | Existing Clocks (`std::function<QDate()>` / `QDateTime()`) | - `std::function<QDate()>`: `TopicListModel`, `MonthView`, `SessionDialog`, `CalendarPage`, `StudyPage`.<br>- `std::function<QDateTime()>`: `ApplicationTableModel`, `AgendaService`. | `grep -rn "std::function<QDate" src/`<br>```
src/models/TopicListModel.h:25:    using TodayClock = std::function<QDate()>;
src/ui/widgets/MonthView.h:33:     std::function<QDate()> todayProvider = {}, ...
src/ui/dialogs/SessionDialog.h:32: std::function<QDate()> todayProvider = {} ...
src/ui/pages/CalendarPage.h:59:    std::function<QDate()> todayProvider = {} ...
src/ui/pages/StudyPage.h:65:       std::function<QDate()> todayProvider = {} ...
``` |
| 14 | Existing Test Targets & CMake Layout | - `tests/services/CMakeLists.txt`: defines `SERVICES_TESTS` (`tst_AgendaService`, `tst_ExportService`), looping with `add_executable`, linking `jobprep_core jobprep_test_helpers Qt6::Test`, running offscreen.<br>- `tests/data/CMakeLists.txt`: data tests linking `jobprep_test_helpers`.<br>- `tests/ui_smoke/CMakeLists.txt`: `ui_smoke` and `tst_MonthView`. | `cat tests/services/CMakeLists.txt`<br>```cmake
set(SERVICES_TESTS
    tst_AgendaService
    tst_ExportService
)
foreach(test_name IN LISTS SERVICES_TESTS)
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE jobprep_core jobprep_test_helpers Qt6::Test)
    ...
``` |

---

## 2. SPEC and AGENTS Cross-Check Table (Conflict -> Resolution)

| Point | SPEC / Requirement | AGENTS / Architecture Constraint | Conflict / Issue | Resolution & Proposal | §14 Entry? |
|---|---|---|---|---|---|
| **1. Notifier Layering** | SPEC §8 table lists `TrayNotifier` and `InAppNotifier` under `services`. | AGENTS §5: Strict downward dependencies (`ui -> models -> services -> data -> domain`). `services`: business logic only, no widgets. | `QSystemTrayIcon` is QtWidgets; in-app toast is a `QWidget`. Placing them in `services` violates AGENTS §5. | `INotifier` interface and plain `Notification` struct live in `src/services/INotifier.h`. Concrete `TrayNotifier`, `InAppNotifier`, and `Toast` live in `src/ui/notifiers/` and `src/ui/widgets/`. Injected into `ReminderService`. | Yes (v1.7) |
| **2. Rescheduled Interviews** | SPEC §5 key format: `interview:<id>:<leadMinutes>` does not include `start_at`. | Reminders must fire for rescheduled appointments; user data must not be missed. | If an interview is rescheduled after a reminder fired, the old key remains in `reminders_sent`, permanently suppressing reminders for the new time. | In `InterviewRepository::update`, when `start_at` changes, invoke `ReminderLogRepository::removeForInterview(interview.id)` to delete matching keys `interview:<id>:%`. Key format remains unchanged. | Yes (v1.7) |
| **3. Multiple Due Leads at Startup** | SPEC §3.5 R1: Interview reminders at lead times (default 1440 and 60m). | App start 59m before interview makes both 1440m and 60m due simultaneously. | Showing two stacked notifications for the same interview at launch is confusing spam. | `ReminderService::dueReminders` evaluates due leads; for any interview with multiple pending due leads, emits only ONE notification for the smallest lead (60m) and marks all due leads (1440 and 60) as sent. | Yes (v1.7) |
| **4. Inactive Items Reminders** | Reminders must not fire for cancelled interviews, past interviews, or closed applications. | Reminders must be strictly relevant and actionable. | `InterviewRepository::inRange` currently returns cancelled interviews; applications might be closed. | `ReminderService` explicitly discards interviews with `outcome == Cancelled` or `startAt < now`, and follow-ups of closed applications (`rejected`, `withdrawn`, `ghosted`, `accepted`). | Yes (v1.7) |
| **5. Overdue Follow-ups Timing** | SPEC §3.5 R1: "Overdue follow-ups are mentioned once on startup" vs key `overdue:<appId>:<yyyy-MM-dd>`. | Must not repeat on every 60s timer tick or on every restart on the same day. | Ambiguity between "startup" trigger and persistent key. | On app launch (first check pass), overdue follow-ups (`next_action_date < today`, non-closed) check key `overdue:<appId>:<today>`. If absent, fires notification and marks key sent. Subsequent 60s ticks and same-day restarts do not repeat it. | Yes (v1.7) |
| **6. Notification Click & Offscreen `raise()`** | SPEC §3.5 R3: Clicking a notification raises the window and opens the item. | AGENTS §3: Tests run offscreen (`QT_QPA_PLATFORM=offscreen`). Offscreen logs "This plugin does not support raise()", and noise filter must not be widened. | Calling `raise()` in test harness generates offscreen warning spam. | Encapsulate activation in `MainWindow::bringToFront()`: check `QGuiApplication::platformName() != u"offscreen"_s` before `raise()` / `activateWindow()`. Item opening (`openInterview`, `openApplication`, `openTopic`) is testable directly. | Yes (v1.7) |
| **7. Close to Tray & App Icon** | SPEC §3.5 R4: Setting "Close to tray" (default off); disabled when no tray available. | AGENTS §12 / M0 carry-over: App icon arrives in M6; Inter font arrives in M6. | Full Settings page controls for close-to-tray are M6 deliverable. | In M5, `MainWindow::closeEvent` respects `SettingsService::closeToTray()` if `QSystemTrayIcon::isSystemTrayAvailable()` (using themed SVG icon). Settings page UI deferred to M6. | Yes (v1.7) |
| **8. Midnight Rollover** | SPEC §3.1 D1, D2: Dashboard, Today list, and streak depend on current date. | App open overnight across midnight must reflect new day without restart. | If the app remains open past 00:00, date-based calculations become stale. | `ReminderService` 60s timer tracks `lastKnownDate`. When `currentDate != lastKnownDate`, emit `dateChanged(QDate)`, triggering refresh of `HomePage` and `MainWindow` status bar. | Yes (v1.7) |
| **9. Rapid Signal Coalescing** | AGENTS §5: Repositories emit `changed()` on writes. | SPEC §11: Cold start < 1s, UI never freezes > 100ms. | Restoring JSON, bulk edits, or seeding emits many `changed()` signals in quick succession, triggering redundant recalculations. | Use zero-interval single-shot `QTimer` (`m_refreshTimer`) in `HomePage` and `MainWindow` status bar to coalesce multiple change events into a single refresh per event loop turn. | Yes (v1.7) |
| **10. "Interviews in next 7 days" Definition** | SPEC §3.1 D1: Stat card "Interviews in next 7 days". | Unambiguous boundary definition required. | Is "next 7 days" `[now, now + 7*24h]` or calendar days `[today, today + 7]`? | **Explicit Assumption:** Defined as `[now, QDateTime(today.addDays(7), QTime(23, 59, 59))]`, excluding cancelled interviews (`outcome == Cancelled`). | Yes (v1.7) |
| **11. Future-Dated Sessions & Streak / Minutes** | SPEC §3.2 S4: Session logging dialog allows any date. | Streak counts consecutive days with sessions. | Do future sessions count for streak or break streak? | **Explicit Rule:** Streak counts consecutive days with sessions strictly up to and including `today` (future sessions neither count nor break). Weekly minutes sum sessions within `[weekStart, weekEnd]`. | Yes (v1.7) |
| **12. Scalable Stats Queries** | SPEC §11: Cold start < 1s with 1,000 applications and 5,000 sessions. | AGENTS §5: Only `data` layer touches SQL. | Computing streak or funnel in memory by loading 5,000 sessions or all status history causes memory/CPU spikes. | Add dedicated SQL aggregate queries: `SessionRepository::distinctSessionDates`, `ApplicationRepository::funnelStageCounts`, `ApplicationRepository::appliedCountInRange`. | Yes (v1.7) |

---

## 3. Architecture & Detailed Design

```
+---------------------------------------------------------------------------------------+
|                                    UI Layer (src/ui/)                                 |
|  MainWindow (status bar, openApplication, openInterview, openTopic, bringToFront)     |
|  HomePage (4 StatCards, Today list, Upcoming list, ProgressRings, FunnelWidget)       |
|  Widgets: StatCard, ProgressRing, FunnelWidget, Toast                                 |
|  Notifiers: TrayNotifier (QSystemTrayIcon), InAppNotifier (Toast manager)             |
+-------------------------------------------+-------------------------------------------+
                                            | uses
+-------------------------------------------v-------------------------------------------+
|                                  Services Layer (src/services/)                       |
|  StatsService (pure dashboard computation: streak, funnel, weekly minutes, rings)     |
|  AgendaService (items, todayItems [overdue + today], upcomingInterviews)              |
|  ReminderService (60s timer, due calculation, smallest lead, reschedule invalidation)  |
|  INotifier (interface + Notification struct)                                          |
|  SettingsService (study/weeklyGoalMinutes, calendar/firstDayOfWeek, app/closeToTray)  |
+-------------------------------------------+-------------------------------------------+
                                            | uses
+-------------------------------------------v-------------------------------------------+
|                                    Data Layer (src/data/)                             |
|  ApplicationRepository (countsByStatus, appliedCountInRange, funnelStageCounts)       |
|  InterviewRepository (inRange, nextUpcoming with non-cancelled filter)                |
|  SessionRepository (totalMinutesInRange, distinctSessionDates)                        |
|  TopicRepository (subtaskProgress, byTargetDateRange)                                 |
|  ReminderLogRepository (wasSent, markSent, removeForInterview)                        |
+-------------------------------------------+-------------------------------------------+
                                            | uses
+-------------------------------------------v-------------------------------------------+
|                                   Domain Layer (src/domain/)                          |
|  Structs: DashboardStats, AgendaItem, Interview, JobApplication, Topic                |
|  Enums: ApplicationStatus, TopicStatus, AgendaKind, InterviewOutcome                  |
|  Functions: topicProgress(...)                                                        |
+---------------------------------------------------------------------------------------+
```

### 3.1 Domain & Services

#### Plain Domain Structs (`src/domain/Structs.h`)
```cpp
struct TrackProgressStat {
    int trackId{0};
    QString trackName;
    QString colorHex;
    int percent{0};
};

struct FunnelStageStat {
    ApplicationStatus status{ApplicationStatus::Wishlist};
    QString label;
    int count{0};
};

struct DashboardStats {
    int appliedThisWeek{0};
    int activeProcesses{0};
    int interviewsNext7Days{0};
    int studyStreakDays{0};
    int weekMinutes{0};
    int weeklyGoalMinutes{360};
    int goalProgressPercent{0};    // Clamped to 100 for the progress bar
    QString goalProgressText;       // e.g. "4h 20m / 6h" or "7h 10m / 6h"
    QList<TrackProgressStat> trackProgresses;
    QList<FunnelStageStat> funnelStages; // 5 stages: Applied, Screen, Tech, Final, Offer
    QString interviewRateText;      // e.g. "45%" or "-" when denominator is 0
};
```

#### `StatsService` API (`src/services/StatsService.h`)
- Injected references: `ApplicationRepository&`, `InterviewRepository&`, `SessionRepository&`, `TopicRepository&`, `TrackRepository&`.
- Injectable clock: `Clock = std::function<QDate()>`.
- Pure compute method:
  ```cpp
  class StatsService {
  public:
      using Clock = std::function<QDate()>;
      explicit StatsService(Data::ApplicationRepository& apps,
                            Data::InterviewRepository& interviews,
                            Data::SessionRepository& sessions,
                            Data::TopicRepository& topics,
                            Data::TrackRepository& tracks,
                            Clock clock = [] { return QDate::currentDate(); });

      void setClock(Clock clock);
      Domain::DashboardStats computeStats(int weeklyGoalMinutes, int firstDayOfWeek) const;
  };
  ```
- **Rules:**
  - `StatsService` never accesses `SettingsService`; settings values are passed in as arguments.
  - Track progress is computed by combining `TopicRepository::byTrack` (or cached topics), `TopicRepository::subtaskProgress()` (single batched query), and `Domain::topicProgress()`.
  - Goal text formatting: formats minutes into `Xh Ym` (e.g., `0h 0m`, `4h 20m`, `7h 10m`). Progress bar percentage is `std::min(100, (weekMinutes * 100) / weeklyGoalMinutes)`.

#### `AgendaService` Additions (`src/services/AgendaService.h`)
To support SPEC §3.1 D2 ("Today" list including overdue follow-ups and overdue study targets) and D5 (Upcoming 5 interviews):
```cpp
/// Returns items for the dashboard Today list:
/// 1. Interviews scheduled on `today` (excluding cancelled).
/// 2. Follow-ups due on `today` OR overdue (`next_action_date <= today`, non-closed).
/// 3. Topics with target date on `today` OR overdue (`target_date <= today`, status != Done).
QList<Domain::AgendaItem> todayItems(QDate today) const;

/// Returns the next `limit` non-cancelled interviews starting on or after `from`.
QList<Domain::AgendaItem> upcomingInterviews(const QDateTime& from, int limit = 5) const;
```

### 3.2 Reminders & Notification System

#### `INotifier` & `Notification` (`src/services/INotifier.h`)
```cpp
namespace JobPrep::Services {

struct Notification {
    enum class Kind { Interview, FollowUp, OverdueFollowUp };
    Kind kind{Kind::Interview};
    int refId{0}; // interviewId or applicationId
    QString title;
    QString message;
};

class INotifier {
public:
    virtual ~INotifier() = default;
    virtual bool notify(const Notification& notification) = 0;
};

} // namespace JobPrep::Services
```

#### `ReminderService` API (`src/services/ReminderService.h`)
```cpp
class ReminderService : public QObject {
    Q_OBJECT
public:
    using Clock = std::function<QDateTime()>;
    explicit ReminderService(Data::InterviewRepository& interviews,
                            Data::ApplicationRepository& apps,
                            Data::ReminderLogRepository& reminderLog,
                            INotifier& notifier,
                            QObject* parent = nullptr,
                            Clock clock = [] { return QDateTime::currentDateTime(); });

    void setClock(Clock clock);
    void checkReminders(); // Runs checks; called on 60s timer and at startup
    void setLeadMinutes(const QList<int>& leads); // Defaults to {1440, 60}

signals:
    void dateChanged(QDate newDate);

private:
    // Pure computation separating calculation from side effects:
    QList<Notification> computeDueReminders(const QDateTime& now, bool isStartupPass) const;
};
```
- **Delivery Guarantee:** A key is recorded in `ReminderLogRepository::markSent` ONLY if `notifier.notify()` was dispatched successfully.
- **Reschedule Invalidation:** When an interview's `start_at` changes, `ReminderLogRepository::removeForInterview(int id)` removes `interview:<id>:%` records from `reminders_sent`.

#### Concrete Notifiers (`src/ui/notifiers/`)
- `TrayNotifier` (`src/ui/notifiers/TrayNotifier.h`):
  Uses `QSystemTrayIcon`. Connects to `messageClicked` signal to invoke `MainWindow::openItemFromNotification`.
- `InAppNotifier` (`src/ui/notifiers/InAppNotifier.h`):
  Fallback when `!QSystemTrayIcon::isSystemTrayAvailable()`. Instantiates and animates `JobPrep::Ui::Widgets::Toast` inside `MainWindow`.
  **Zero-loss guarantee:** If the window is minimized or inactive, queued toasts remain persistent until the user views the window.
- `Toast` Widget (`src/ui/widgets/Toast.h`):
  Floating card in top-right or bottom-right of `MainWindow`. Shows notification kind icon, title, message text, and close button. Clicking the toast activates the item.

### 3.3 UI Custom Widgets & Dashboard Layout

#### Custom Widgets
1. `StatCard` (`src/ui/widgets/StatCard.h`):
   - QPainter-rendered card displaying title, big value text, and optional icon/subtitle.
   - Tokens: `surface`, `border`, `text`, `textMuted`. Radius 12px.
   - HiDPI-correct (`devicePixelRatio`), font metrics derived spacing, accessible name and description.
2. `ProgressRing` (`src/ui/widgets/ProgressRing.h`):
   - QPainter-rendered circular progress ring. Background track arc in `surfaceAlt`, foreground arc filled with track color.
   - Displays percentage and track name centered inside or beneath.
   - Anti-aliased, zero per-paint heap allocation, accessible description.
3. `FunnelWidget` (`src/ui/widgets/FunnelWidget.h`):
   - Horizontal pipeline showing the 5 stages: Applied → HR Screen → Technical → Final → Offer.
   - Displays stage count as high-contrast readable text (accessible; not color-dependent).
   - Shows Interview Rate below the funnel.
   - Uses stage status colors from `StatusStyle`.

#### Dashboard Page Layout (`HomePage.cpp`)
```
+-------------------------------------------------------------------------------------------------------+
| [ Applied this week: 5 ]   [ Active processes: 3 ]   [ Interviews 7d: 2 ]   [ Study streak: 6 d ]      |
+---------------------------------------------------+---------------------------------------------------+
| Today                                             | Study Progress                                    |
|   10:00 Technical · Globex                        |   (ProgressRing C++ 42%) (ProgressRing Eng 65%)   |
|   Follow-up · Acme                                |   This week: 4h 20m / 6h [ProgressBar 72%]        |
|   Target · Move semantics                         +---------------------------------------------------+
|                                                   | Hiring Funnel                                     |
| Upcoming Interviews                               |   Applied 18 > Screen 7 > Tech 3 > Final 2 > Offer 1 |
|   Mon 14:00 Final · BigCorp                       |   Interview rate: 38.9%                           |
+---------------------------------------------------+---------------------------------------------------+
```
- Empty state: If no applications and no topics exist, display `Widgets::EmptyState` with "+ Application" and "+ Topic" buttons triggering `MainWindow::newApplicationFromHeader()` and `MainWindow::newTopicFromHeader()`.

### 3.4 Navigation & MainWindow Integration

`MainWindow` exposes public routing slots:
- `void openTopic(int topicId);` (already exists)
- `void openInterview(int interviewId);` (new; opens `InterviewDialog` in Edit mode)
- `void openApplication(int applicationId);` (new; opens `ApplicationDialog` in Edit mode)
- `void bringToFront();` (new; brings window to foreground, guarded against offscreen platform)
- `void openItemFromNotification(Services::Notification::Kind kind, int refId);` (new)

---

## 4. Work Breakdown (Parts with Approval Gates)

Following the established M3/M4 pattern, M5 is divided into 3 reviewable parts:

```
Part A (Data & StatsService) ────────> [Approval Gate]
                                              │
Part B (Dashboard UI & Status Bar) ───> [Approval Gate]
                                              │
Part C (Reminders, Notifiers & Smoke) ─> [Final Approval]
```

### Part A: Data Layer Queries & `StatsService`
1. Add aggregate queries in `ApplicationRepository`: `appliedCountInRange(QDate, QDate)`, `funnelStageCounts()`.
2. Add `SessionRepository::distinctSessionDates(QDate upTo)`.
3. Add `ReminderLogRepository::removeForInterview(int interviewId)`.
4. Ensure `InterviewRepository::nextUpcoming` and 7-day range queries filter out cancelled interviews.
5. Create `Domain::DashboardStats` structs in `src/domain/Structs.h`.
6. Implement `StatsService` with pure aggregation methods and injectable clock.
7. Implement unit tests in `tests/services/tst_StatsService.cpp` and repository test additions.
8. Verify zero warnings and green tests with `./scripts/check.sh`.

### Part B: Dashboard Custom Widgets, Page & Navigation
1. Implement custom QPainter widgets: `StatCard`, `ProgressRing`, `FunnelWidget`.
2. Add `AgendaService::todayItems(QDate)` and `AgendaService::upcomingInterviews(QDateTime, int)`.
3. Add navigation methods to `MainWindow`: `openInterview(int)`, `openApplication(int)`, `bringToFront()`.
4. Build `HomePage`: 4 stat cards, Today list, Upcoming list, Study progress rings + goal bar, Funnel widget, Empty state.
5. Implement coalesced refresh timer in `HomePage` and `MainWindow` status bar updater.
6. Connect `SettingsService::weeklyGoalMinutesChanged` and repository `changed()` signals.
7. Add UI smoke coverage for Dashboard and Status bar.
8. Verify zero warnings and green tests with `./scripts/check.sh`.

### Part C: Reminder System, Notifiers, End-to-End Tests & Final Polish
1. Define `INotifier` interface and `Notification` struct in `src/services/INotifier.h`.
2. Implement `ReminderService`: 60s timer, startup pass, due evaluation, smallest lead selection, date rollover.
3. Implement `TrayNotifier` and `InAppNotifier` (with `Toast` widget) in `src/ui/`.
4. Implement automatic fallback: `QSystemTrayIcon::isSystemTrayAvailable()` selection.
5. Wire notification clicks to `MainWindow::openItemFromNotification`.
6. Implement unit tests in `tests/services/tst_ReminderService.cpp`.
7. Extend `tests/ui_smoke/tst_UiSmoke.cpp`: Dashboard in light/dark themes, empty state buttons, Today item navigation, midnight rollover, toast display/dismiss.
8. Extend `scripts/screenshots.sh` for dashboard (with data, empty, and in-app toast).
9. Update `docs/SPEC.md` §14 changelog for v1.7.

---

## 5. Test Plan & Test Slots

### 5.1 `tests/services/tst_StatsService.cpp`
- Hand-calculated test dataset with arithmetic documented in comments:
  - 10 applications:
    - 2 Wishlist (rank 0)
    - 3 Applied (rank 1)
    - 2 HR Screen (rank 2)
    - 1 Technical (rank 3)
    - 1 Final (rank 4)
    - 1 Offer (rank 5)
    - 1 Rejected with history to Technical (rank 3)
  - Sessions: Monday 60m, Tuesday 120m, Thursday 80m. Total = 260m.
  - Interviews: 2 scheduled in next 7 days, 1 cancelled in next 7 days (ignored), 1 past (ignored).
- **Test Slots:**
  1. `testDashboardStatsHandCalculated()`: fixed clock Monday 2026-10-26, goal 360m, firstDay Monday. Verifies applied count (3), active processes (7), interviews next 7d (2), streak (0 today, 1 yesterday if Sunday had session), funnel counts (Rank 1: 8, Rank 2: 5, Rank 3: 3, Rank 4: 2, Rank 5: 1), interview rate: 5 / 8 = 62.5%.
  2. `testDifferentWeeklyGoalAndWeekStart()`: goal 480m, first day Sunday. Verifies week start boundary shifts.
  3. `testGoalProgressFormattingAndCapping()`: weekMinutes 430m, goal 360m -> progress bar capped at 100%, text formatted as `"7h 10m / 6h"`.
  4. `testFunnelRanksAndInterviewRate()`: verifies accepted = rank 5, rejected uses history; when denominator is 0, interview rate returns `"-"`.
  5. `testStudyStreakScenarios()`:
     - No session today: streak counted starting yesterday.
     - Gap day: breaks streak cleanly.
     - Future-dated sessions: ignored for streak, neither adding nor breaking.
  6. `testTrackProgressAveraging()`: verifies average of topic progresses per track using `domain::topicProgress` and batched subtask counts.
  7. `testPerformanceSanity1000Apps5000Sessions()`: in-memory DB populated with 1,000 applications and 5,000 sessions. `computeStats()` must finish in < 50 ms.

### 5.2 `tests/services/tst_ReminderService.cpp`
- **Test Slots:**
  1. `testDueRemindersHandCalculated()`: checks interview 24h before (1440m) and 1h before (60m).
  2. `testSingleNotificationWhenMultipleLeadsDue()`: app starts 59m before interview; both 1440m and 60m are due; asserts exactly ONE notification fired (for 60m lead) and BOTH keys marked sent in `reminders_sent`.
  3. `testNoRepeatAfterRestart()`: re-running check with existing `reminders_sent` keys emits 0 notifications.
  4. `testCancelledAndPastInterviewsIgnored()`: cancelled or elapsed interviews fire nothing.
  5. `testClosedApplicationsIgnored()`: follow-ups on rejected/withdrawn/ghosted/accepted applications fire nothing.
  6. `testFollowUpAt0900()`: follow-up reminder fires on `next_action_date` at or after 09:00.
  7. `testOverdueFollowUpOnStartupOncePerDay()`: overdue follow-up fires once on startup; restart on same day does not repeat.
  8. `testRescheduledInterviewFiresNewReminder()`: rescheduling interview clears sent keys and fires at new time.
  9. `testAutomaticFallbackToInAppNotifierWhenNoTray()`: simulates no tray available; verifies in-app notifier is selected.
  10. `testNotificationClickDispatchesOpenItem()`: notification activation routes to correct open slot.

### 5.3 `tests/ui_smoke/tst_UiSmoke.cpp` Additions
- `testDashboardVisitedBothThemes()`: navigates to Home page in light and dark themes; verifies zero warnings and visual hierarchy.
- `testDashboardEmptyStateActionButtons()`: on empty database, clicks "+ Application" and "+ Topic"; verifies exactly one dialog opens for each.
- `testDashboardTodayItemClicksOpenRespectiveDialogs()`: clicks interview item (opens `InterviewDialog`), follow-up item (opens `ApplicationDialog`), topic item (navigates to Study page and selects topic).
- `testDashboardMidnightRollover()`: triggers date rollover signal with injected clock; asserts Today list and streak refresh.
- `testInAppReminderToastDisplayedAndDismissed()`: simulates reminder triggering in-app toast; asserts toast renders correctly and dismisses on click.
- `testDashboardScreenshots()`: generates screenshots for light/dark themes:
  - `dashboard-with-data-light.png` / `dashboard-with-data-dark.png`
  - `dashboard-empty-light.png` / `dashboard-empty-dark.png`
  - `dashboard-toast-light.png` / `dashboard-toast-dark.png`

---

## 6. Risks, Files Touched, SPEC §14 Entries & User Verification

### 6.1 Risks & Mitigations
| Risk | Severity | Mitigation |
|---|---|---|
| Query performance degradation with 1,000 apps / 5,000 sessions | High | Aggregate SQL queries in `data` layer (no in-memory filtering of full tables). Automated performance sanity test in `tst_StatsService`. |
| Offscreen test warnings from `raise()` violating zero-warning gate | High | Encapsulate window raising in `MainWindow::bringToFront()` with platform guard (`platformName() != "offscreen"`). |
| Notification spam on startup for multiple overdue/due items | Medium | Pick smallest lead time per interview; mark all elapsed leads as sent; overdue follow-ups fire once per calendar day. |
| In-app toast lost if window minimized | Medium | `InAppNotifier` queues notifications and keeps them visible until the window is displayed and user interacts. |
| Stale dashboard across midnight | Low | Date rollover detection in 60s timer emitting `dateChanged(QDate)` to trigger coalesced UI refresh. |

### 6.2 Files Touched / Added

#### Added:
- `src/domain/DashboardStats.h` (or in `Structs.h`)
- `src/services/StatsService.h`, `src/services/StatsService.cpp`
- `src/services/INotifier.h`
- `src/services/ReminderService.h`, `src/services/ReminderService.cpp`
- `src/ui/notifiers/TrayNotifier.h`, `src/ui/notifiers/TrayNotifier.cpp`
- `src/ui/notifiers/InAppNotifier.h`, `src/ui/notifiers/InAppNotifier.cpp`
- `src/ui/widgets/StatCard.h`, `src/ui/widgets/StatCard.cpp`
- `src/ui/widgets/ProgressRing.h`, `src/ui/widgets/ProgressRing.cpp`
- `src/ui/widgets/FunnelWidget.h`, `src/ui/widgets/FunnelWidget.cpp`
- `src/ui/widgets/Toast.h`, `src/ui/widgets/Toast.cpp`
- `tests/services/tst_StatsService.cpp`
- `tests/services/tst_ReminderService.cpp`

#### Modified:
- `CMakeLists.txt` (register new source files)
- `tests/services/CMakeLists.txt` (add `tst_StatsService`, `tst_ReminderService`)
- `src/domain/Structs.h` (add dashboard and notification structs)
- `src/data/ApplicationRepository.h`, `src/data/ApplicationRepository.cpp` (add aggregate queries)
- `src/data/SessionRepository.h`, `src/data/SessionRepository.cpp` (add `distinctSessionDates`)
- `src/data/InterviewRepository.h`, `src/data/InterviewRepository.cpp` (filter non-cancelled)
- `src/data/ReminderLogRepository.h`, `src/data/ReminderLogRepository.cpp` (add `removeForInterview`)
- `src/services/AgendaService.h`, `src/services/AgendaService.cpp` (add `todayItems`, `upcomingInterviews`)
- `src/ui/pages/HomePage.h`, `src/ui/pages/HomePage.cpp` (full dashboard implementation)
- `src/ui/MainWindow.h`, `src/ui/MainWindow.cpp` (status bar, open routing, notifier wiring)
- `tests/ui_smoke/tst_UiSmoke.cpp` (dashboard and toast acceptance tests)
- `scripts/screenshots.sh` (include `testDashboardScreenshots`)
- `docs/SPEC.md` (§14 changelog for v1.7)

### 6.3 SPEC §14 Entries Expected (Version 1.7)
- **1.7 — Layering of Notifiers:** `INotifier` interface and `Notification` struct live in `src/services/`, while concrete implementations `TrayNotifier`, `InAppNotifier`, and `Toast` live in `src/ui/`, adhering strictly to AGENTS §5 dependency rules.
- **1.7 — Rescheduled Interview Reminders:** When an interview's `start_at` is updated, `ReminderLogRepository::removeForInterview(int id)` deletes previously logged keys (`interview:<id>:%`) so reminders trigger reliably for the new schedule without altering the migration 001 schema.
- **1.7 — Due Reminder Coalescing at Startup:** When multiple reminder leads are due upon startup (e.g., app launched 59 minutes before an interview), only the smallest due lead generates a user notification, and all elapsed due leads are marked sent in `reminders_sent` to prevent notification spam.
- **1.7 — Overdue Follow-up Notification Timing:** Overdue follow-ups fire once per calendar day on initial startup pass using key format `overdue:<appId>:<yyyy-MM-dd>`; subsequent 60s timer passes and restarts on the same date do not repeat the notification.
- **1.7 — "Interviews in Next 7 Days" Definition:** Defined as all non-cancelled interviews scheduled within `[now, QDateTime(today.addDays(7), QTime(23, 59, 59))]`.
- **1.7 — Study Streak and Future-Dated Sessions:** Study streak counts consecutive days with sessions strictly up to and including `today`; future-dated sessions neither count toward nor break the streak. Weekly study minutes sum sessions within `[weekStart, weekEnd]`.
- **1.7 — AgendaService Today and Upcoming Extensions:** `AgendaService` provides `todayItems(QDate today)` returning today's interviews, overdue/due follow-ups, and overdue/due topics (excluding Done topics and closed applications), and `upcomingInterviews(QDateTime from, int limit)` returning the next 5 interviews.
- **1.7 — MainWindow Cross-Page Navigation & BringToFront:** `MainWindow` exposes public routing methods `openInterview(int id)`, `openApplication(int id)`, and `bringToFront()` (guarded against offscreen platform plugins to prevent test warnings).
- **1.7 — Close to Tray Behavior:** Setting `app/closeToTray` is supported at runtime in `MainWindow::closeEvent` if a system tray is available; Settings page UI configuration is deferred to M6.

### 6.4 Not Verified by Agent, User to Check
1. Visual appearance and popup animation of native desktop notifications in OS system tray (GNOME notification banner, KDE Plasma, or Windows Notification Center).
2. Behavior when minimizing or closing window with "Close to tray" enabled in live desktop session.
3. Human check of dashboard aesthetics, typography, and card spacing at 100%, 125%, and 150% display scaling under live X11/Wayland desktop.
