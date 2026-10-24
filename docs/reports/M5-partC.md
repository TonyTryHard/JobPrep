# Milestone 5 Part C Report: Reminder Core Service

**Operating System:** Linux (Ubuntu 24.04 x86_64, GCC 13.3.0, Qt 6.8.3, offscreen QPA for tests)  
**Date:** 2026-10-24  
**Commits:**
- `8b1ca12`: `refactor(ui): remove unused FormatHelper overloads and add Part B errata report` (C0 carry-overs)
- `49bf09e`: `feat(reminders): implement ReminderService, INotifier contract and settings` (Part C implementation)
- `c3c5a3b`: `docs(spec): document Part C reminder core, notifier contract and settings in section 14 (v1.7)` (SPEC §14 updates)

---

## 1. Plan vs. Result & Scope Gaps

| Planned Item | Status | Notes / Deviations |
| :--- | :--- | :--- |
| **C0 Part B Carry-overs** | Complete | Errata report committed, unused `FormatHelper` overloads pruned, `tst_DayWatcher` slots verified, rendered funnel rate verified. |
| **`INotifier` & `Notification` Struct** | Complete | Defined in `src/services/INotifier.h`. `Notification` carries pure domain metadata (kind, refId, company, position, interviewType, when, leadMinutes, count), free of UI presentation. |
| **`INotifier` Delivery Contract** | Complete | `notify(...)` returns `true` only if displayed. On `false`, nothing is written to DB; tick retries on next check. |
| **`ReminderLogRepository::sentAt`** | Complete | Added `std::optional<QDateTime> sentAt(QStringView key) const` to support reschedule check without cross-repository writes. |
| **`ReminderService` Core** | Complete | Pure `computePlan` function separating pure logic from side-effects; injectable clock; reschedule validity rule; smallest-lead coalescing; silent recording of larger leads; 09:00 follow-ups; single/summary overdue follow-ups on startup pass. |
| **`SettingsService` Lead Minutes** | Complete | `interviewLeadMinutes` / `setInterviewLeadMinutes` with `interviewLeadMinutesChanged` signal, sanitized (positive, unique, descending). Custom constructor allowing injected `QSettings`. |
| **Unit Tests** | Complete | `tst_ReminderService` (10 slots), `tst_ReminderLogRepository` (+2 slots), `tst_SettingsService` (4 slots). |
| **UI Widgets / Composition** | Intentionally Excluded | As required by Part C, no UI widgets (`TrayNotifier`, `InAppNotifier`, `Toast`) or `MainWindow` timer wiring was added. Composition belongs to Part D. |

**Honest gaps:**
- `ReminderService` is not yet instantiated or wired in `main.cpp` or `MainWindow`; this is deferred to Part D where the visual notifiers exist.
- Settings UI for `interviewLeadMinutes` is deferred to Milestone M6.

---

## 2. C0 Outcomes (Part B Carry-overs)

### 2.1 Errata Summary
Created `docs/reports/M5-partB-errata.md` in commit `8b1ca12` documenting:
- Funnel stages: corrected to `Applied, HR Screen, Technical, Final, Offer` (not `Offer, Accepted`).
- `TopicDialog`: acknowledged absence; topic quick-add navigates to `StudyPage`.
- `DashboardController`: confirmed explicit constructor references (`ApplicationRepository&`, `InterviewRepository&`, etc.), not `AppContext&`.
- Inter typography: typography uses standard Qt font metric calculations with design tokens; Inter font bundling is handled at application startup.
- Test counts: 14 test slots exist across dashboard tests; `cbefc94` fixed the `* 100.0` interview rate percentage calculation.

### 2.2 `tst_DayWatcher` Slots
Exact command output:
```bash
$ grep -n "void " tests/services/tst_DayWatcher.cpp
12:    void noSignalOnSameDay();
13:    void oneSignalWhenDateChanges();
14:    void noRepeatOnSameChangedDay();
15:    void dateMovingBackwardsEmitsSignal();
18:void tst_DayWatcher::noSignalOnSameDay() {
29:void tst_DayWatcher::oneSignalWhenDateChanges() {
42:void tst_DayWatcher::noRepeatOnSameChangedDay() {
57:void tst_DayWatcher::dateMovingBackwardsEmitsSignal() {
```

### 2.3 `firstDayOfWeek` Mapping
Exact command output showing mapping from stored `int` to `Qt::DayOfWeek`:
```bash
$ grep -n "firstDayOfWeek" src/ui/DashboardController.cpp src/services/StatsService.cpp src/services/SettingsService.cpp
src/ui/DashboardController.cpp:58:    connect(&m_settings, &JobPrep::Services::SettingsService::firstDayOfWeekChanged, this,
src/ui/DashboardController.cpp:77:        (m_settings.firstDayOfWeek() == 7) ? Qt::Sunday : Qt::Monday;
src/services/StatsService.cpp:39:std::pair<QDate, QDate> StatsService::weekBounds(QDate date, Qt::DayOfWeek firstDayOfWeek) {
src/services/StatsService.cpp:42:    const int startDay = static_cast<int>(firstDayOfWeek);
src/services/StatsService.cpp:49:DashboardStats StatsService::computeStats(int weeklyGoalMinutes, Qt::DayOfWeek firstDayOfWeek) const {
src/services/StatsService.cpp:56:    const auto [weekStart, weekEnd] = weekBounds(today, firstDayOfWeek);
src/services/SettingsService.cpp:91:int SettingsService::firstDayOfWeek() const {
src/services/SettingsService.cpp:93:    return m_settings->value(u"calendar/firstDayOfWeek"_s, 1).toInt();
src/services/SettingsService.cpp:97:    if (firstDayOfWeek() == day) return;
src/services/SettingsService.cpp:98:    m_settings->setValue(u"calendar/firstDayOfWeek"_s, day);
src/services/SettingsService.cpp:99:    emit firstDayOfWeekChanged(day);
```

### 2.4 Pruned `FormatHelper` Overloads and Rendered Funnel Rate
Dead overloads `formatStreakDays(int)` and `formatNextInterview(const std::optional<Domain::Interview>&, ...)` were deleted from `src/ui/FormatHelper.h` and `FormatHelper.cpp` in commit `8b1ca12`.

Exact command output showing active callers:
```bash
$ grep -rn "formatNextInterview\|formatStreakDays\|formatStreak(" src/ui
src/ui/MainWindow.cpp:176:                m_statusStreakLabel->setText(FormatHelper::formatStreak(snap.stats.studyStreakDays));
src/ui/MainWindow.cpp:179:                    FormatHelper::formatNextInterview(snap.nextInterview));
src/ui/MainWindow.cpp:183:    m_statusStreakLabel->setText(FormatHelper::formatStreak(initialSnap.stats.studyStreakDays));
src/ui/MainWindow.cpp:186:        FormatHelper::formatNextInterview(initialSnap.nextInterview));
src/ui/FormatHelper.cpp:35:QString FormatHelper::formatStreak(int days) {
src/ui/FormatHelper.cpp:46:QString FormatHelper::formatNextInterview(const std::optional<Domain::AgendaItem>& item,
src/ui/pages/HomePage.cpp:242:    m_cardStreak->setValue(FormatHelper::formatStreak(snapshot.stats.studyStreakDays));
src/ui/FormatHelper.h:28:    static QString formatStreak(int days);
src/ui/FormatHelper.h:36:    static QString formatNextInterview(const std::optional<Domain::AgendaItem>& item,
```

Rendered interview-rate assert in `tests/ui_smoke/tst_UiSmoke.cpp`:
```bash
$ grep -n -C 3 "Interview rate" tests/ui_smoke/tst_UiSmoke.cpp
3484-    QCOMPARE(FormatHelper::formatGoalText(260, 360), u"4h 20m / 6h"_s);
3485-    QCOMPARE(FormatHelper::formatGoalText(430, 360), u"7h 10m / 6h"_s);
3486-
3487:    // Interview rate
3488-    QCOMPARE(FormatHelper::formatInterviewRate(std::nullopt), QString::fromUtf8("\u2013"));
3489-    QCOMPARE(FormatHelper::formatInterviewRate(6.0 / 9.0), u"66.7%"_s);
3490-
--
3632-    QCOMPARE(FormatHelper::funnelStageLabel(funnelWidget->stages()[4].status), u"Offer"_s);
3633-    QCOMPARE(funnelWidget->stages()[4].reachedCount, 1);
3634-    QCOMPARE(FormatHelper::formatInterviewRate(funnelWidget->interviewRate()), u"66.7%"_s);
3635:    QVERIFY(funnelWidget->accessibleDescription().contains(u"Interview rate: 66.7%"_s));
3636-}
3637-
3638-void tst_UiSmoke::testDashboardEmptyStateActionButtons() {
```

---

## 3. Amendment Checklist (C1–C7)

| Amendment | Scope & Description | Refinements in Part C | Status |
| :--- | :--- | :--- | :--- |
| **C1** | `INotifier` delivery contract: `bool notify(const Notification&)` returns true only if presented; on false, nothing recorded; tick retries. | Refined: `Notification` carries pure data (`kind`, `refId`, `company`, `position`, `interviewType`, `when`, `leadMinutes`, `count`), NOT presentation `title`/`message`. Services do not depend on UI `tr()` or `StatusStyle`. | **Compliant** |
| **C2** | Reschedule check without cross-repo writes: key `interview:<id>:<lead>` is valid only if stored `sent_at >= start_at - lead`. | Implemented in `ReminderService::computePlan` via injected lookup function; no writes from `InterviewRepository`. | **Compliant** |
| **C3** | Reminder rules: due when `now >= start - L` and `now < start`. Smallest-lead coalescing. Cancelled excluded. Follow-up at >= 09:00. Overdue follow-up on startup pass. | Refined: Overdue sends single `Notification` for 1 overdue app (`refId`), or summary notification (`count`, `refId = 0`) for multiple apps. | **Compliant** |
| **C4** | Settings key `reminders/interviewLeadMinutes`: sanitized list of ints (default `{1440, 60}`). | Implemented in `SettingsService` with `interviewLeadMinutesChanged` signal and custom settings injection constructor. | **Compliant** |
| **C5** | Composition design documented: `ReminderService` created after `MainWindow`, owned by Qt parent; notifier needs window. | Documented below in Section 6. | **Compliant** |
| **C6** | `tst_ReminderService` unit tests with `FakeNotifier` and fixed clock. | 10 comprehensive slots testing all edge cases. | **Compliant** |
| **C7** | Close to tray deferred to M6. | Documented in SPEC §14. | **Compliant** |

---

## 4. Reminder Rules & Verification Table

| Rule # | Business Rule Description | Proving Test Slot in `tst_ReminderService` |
| :--- | :--- | :--- |
| **Rule 1** | Interview lead L is due when `now >= start - L` and `now < start`. Started/past interviews skipped. Cancelled interviews skipped. Rejected applications' interviews **are** reminded. | `cancelledStartedPastInterviewsIgnoredRejectedApplicationReminded` |
| **Rule 2** | Reschedule: `interview:<id>:<lead>` valid only if `sent_at >= start_at - lead`. Moved later -> fires again; moved earlier into window -> fires; deleted -> nothing; not rescheduled -> no repeat. | `rescheduleMovedLaterFiresAgainMovedEarlierFiresDeletedNothingNotRescheduledNoRepeat` |
| **Rule 3** | Smallest-lead coalescing: one notification for smallest due lead, all due leads marked sent. If smaller lead already delivered, larger due lead marked sent silently without notifying. | `smallerLeadAlreadyDeliveredLargerAddedLaterMarkedNotNotified`<br>`interview59MinutesAheadFiresOnceAndRecordsBothKeysNoRepeatOnRestart`<br>`leads24hAnd1hFireSeparatelyWhenRunning` |
| **Rule 4** | Follow-up: due when `next_action_date == today`, `now >= 09:00`, application not closed; key `followup:<appId>:<yyyy-MM-dd>`. | `followUpBefore0900NothingAt0900OnceClosedApplicationNothing` |
| **Rule 5** | Overdue follow-ups: startup pass only, non-closed applications with `next_action_date < today`. Exactly one notification: single app has `refId`, multiple has summary `count` and `refId = 0`. Key `overdue:<appId>:<today>` prevents repeats on same-day restart. | `overdueStartupPassOnlyOneAppSingleMultipleSummaryRestartSameDayNothingNextDayAgain` |
| **Rule 6** | Delivery failure: if `notify()` returns `false`, record nothing; next tick tries again. | `notifierReturnsFalseNothingRecordedNextTickDeliversOnce` |
| **Rule 7** | Day border / DST: interview at 00:30 with 1440 lead is due on the previous day. | `dstDayBorderInterviewAt0030With1440LeadDuePreviousDay` |
| **Extra** | Dynamic lead minutes change takes effect on the next check. | `leadMinutesChangeTakesEffectOnNextCheck` |

---

## 5. Header Signatures (Real Grep Output)

```bash
$ grep -n -E "class |struct |virtual |bool |void |std::optional|QList<int>|explicit " src/services/INotifier.h src/services/ReminderService.h src/services/SettingsService.h src/data/ReminderLogRepository.h
src/services/INotifier.h:11:enum class NotificationKind {
src/services/INotifier.h:19:struct Notification {
src/services/INotifier.h:24:    std::optional<Domain::InterviewType> interviewType{std::nullopt};
src/services/INotifier.h:31:class INotifier {
src/services/INotifier.h:33:    virtual ~INotifier() = default;
src/services/INotifier.h:37:    virtual bool notify(const Notification& notification) = 0;
src/services/ReminderService.h:13:class QTimer;
src/services/ReminderService.h:16:class ApplicationRepository;
src/services/ReminderService.h:17:class InterviewRepository;
src/services/ReminderService.h:18:class ReminderLogRepository;
src/services/ReminderService.h:24:struct CandidateInterview {
src/services/ReminderService.h:31:struct DueReminder {
src/services/ReminderService.h:37:struct ReminderPlan {
src/services/ReminderService.h:43:class ReminderService : public QObject {
src/services/ReminderService.h:48:    using SentAtLookup = std::function<std::optional<QDateTime>(QStringView key)>;
src/services/ReminderService.h:58:    void setLeadMinutes(const QList<int>& leadMinutes);
src/services/ReminderService.h:59:    QList<int> leadMinutes() const { return m_leadMinutes; }
src/services/ReminderService.h:62:    void setClock(Clock clock);
src/services/ReminderService.h:65:    void start();
src/services/ReminderService.h:69:    void checkNow(bool startupPass = false);
src/services/ReminderService.h:73:                                    bool startupPass,
src/services/ReminderService.h:74:                                    const QList<int>& leadMinutes,
src/services/ReminderService.h:87:    QList<int> m_leadMinutes{1440, 60};
src/services/SettingsService.h:8:class QSettings;
src/services/SettingsService.h:13:class SettingsService : public QObject {
src/services/SettingsService.h:17:    explicit SettingsService(QObject* parent = nullptr);
src/services/SettingsService.h:18:    explicit SettingsService(std::unique_ptr<QSettings> settings, QObject* parent = nullptr);
src/services/SettingsService.h:22:    void setWindowGeometry(const QByteArray& geometry);
src/services/SettingsService.h:25:    void setWindowState(const QByteArray& state);
src/services/SettingsService.h:27:    bool isSidebarCollapsed() const;
src/services/SettingsService.h:28:    void setSidebarCollapsed(bool collapsed);
src/services/SettingsService.h:31:    void setThemeMode(const QString& mode);
src/services/SettingsService.h:34:    void setAccentPreset(const QString& preset);
src/services/SettingsService.h:37:    void setWeeklyGoalMinutes(int minutes);
src/services/SettingsService.h:40:    void setFirstDayOfWeek(int day);
src/services/SettingsService.h:42:    QList<int> interviewLeadMinutes() const;
src/services/SettingsService.h:43:    void setInterviewLeadMinutes(const QList<int>& minutes);
src/services/SettingsService.h:45:    bool closeToTray() const;
src/services/SettingsService.h:46:    void setCloseToTray(bool enabled);
src/services/SettingsService.h:48:    bool sampleDataPrompted() const;
src/services/SettingsService.h:49:    void setSampleDataPrompted(bool prompted);
src/services/SettingsService.h:51:    bool calendarLayerInterviews() const;
src/services/SettingsService.h:52:    void setCalendarLayerInterviews(bool enabled);
src/services/SettingsService.h:54:    bool calendarLayerFollowUps() const;
src/services/SettingsService.h:55:    void setCalendarLayerFollowUps(bool enabled);
src/services/SettingsService.h:57:    bool calendarLayerTargets() const;
src/services/SettingsService.h:58:    void setCalendarLayerTargets(bool enabled);
src/services/SettingsService.h:61:    void setCalendarSplitterState(const QByteArray& state);
src/services/SettingsService.h:64:    void setStudySplitterState(const QByteArray& state);
src/services/SettingsService.h:67:    void setStudyViewMode(const QString& mode);
src/services/SettingsService.h:70:    void setStudyTrackFilter(int trackId);
src/services/SettingsService.h:73:    void themeModeChanged(const QString& mode);
src/services/SettingsService.h:74:    void accentPresetChanged(const QString& preset);
src/services/SettingsService.h:75:    void sidebarCollapsedChanged(bool collapsed);
src/services/SettingsService.h:76:    void weeklyGoalMinutesChanged(int minutes);
src/services/SettingsService.h:77:    void firstDayOfWeekChanged(int day);
src/services/SettingsService.h:78:    void interviewLeadMinutesChanged(const QList<int>& minutes);
src/services/SettingsService.h:79:    void closeToTrayChanged(bool enabled);
src/services/SettingsService.h:80:    void calendarLayerInterviewsChanged(bool enabled);
src/services/SettingsService.h:81:    void calendarLayerFollowUpsChanged(bool enabled);
src/services/SettingsService.h:82:    void calendarLayerTargetsChanged(bool enabled);
src/services/SettingsService.h:83:    void studySplitterStateChanged(const QByteArray& state);
src/services/SettingsService.h:84:    void studyViewModeChanged(const QString& mode);
src/services/SettingsService.h:85:    void studyTrackFilterChanged(int trackId);
src/data/ReminderLogRepository.h:11:class Database;
src/data/ReminderLogRepository.h:15:class ReminderLogRepository : public Repository {
src/data/ReminderLogRepository.h:18:    explicit ReminderLogRepository(Database& database, QObject* parent = nullptr);
src/data/ReminderLogRepository.h:21:    bool wasSent(QStringView key) const;
src/data/ReminderLogRepository.h:24:    std::optional<QDateTime> sentAt(QStringView key) const;
src/data/ReminderLogRepository.h:27:    bool markSent(const QString& key, const QDateTime& sentAt = QDateTime());
src/data/ReminderLogRepository.h:28:    bool remove(const QString& key);
```

`ReminderService` constructor signature:
```cpp
ReminderService(Data::InterviewRepository& interviews,
                Data::ApplicationRepository& apps,
                Data::ReminderLogRepository& reminderLog,
                INotifier& notifier,
                QObject* parent = nullptr);
```

### Note on `markSent` on an Existing Key
In `ReminderLogRepository::markSent(const QString& key, const QDateTime& sentAt)`:
```cpp
if (wasSent(key)) {
    return true;
}
```
If a key already exists, `markSent` does not overwrite the row, preserving `markSentTwiceKeepsFirstTimestamp`.
When an interview is rescheduled and fires again because `sent_at < start_at - lead`, `ReminderService` calls `m_reminderLog.remove(key)` before `m_reminderLog.markSent(key, now)`. This cleanly updates `sent_at` to the new timestamp without requiring cross-table deletes from `InterviewRepository`.

---

## 6. Composition Design for Part D

In Part D:
1. `INotifier` implementations (`TrayNotifier`, `InAppNotifier`) are visual widgets/delegates located in `src/ui/notifiers/`.
2. Because notifiers require either an active top-level `QWidget* parent` (for `InAppNotifier` toasts overlaid on `MainWindow`) or `QSystemTrayIcon` connected to `MainWindow` actions, `ReminderService` **cannot** be instantiated inside `AppContext` (which is the purely headless composition root).
3. Instead, `ReminderService` will be instantiated in `main.cpp` **after** `MainWindow` is constructed:
   ```cpp
   MainWindow mainWindow(context);
   mainWindow.show();

   auto notifier = JobPrep::UI::makeNotifier(mainWindow, ...);
   auto reminderService = std::make_unique<JobPrep::Services::ReminderService>(
       context.interviews(),
       context.applications(),
       context.reminderLog(),
       *notifier,
       &mainWindow
   );
   reminderService->setLeadMinutes(context.settings().interviewLeadMinutes());
   QObject::connect(&context.settings(), &SettingsService::interviewLeadMinutesChanged,
                    reminderService.get(), &ReminderService::setLeadMinutes);
   reminderService->start();
   ```
4. Clicking a notification dispatches `MainWindow::openItemFromNotification(...)`, raising the window and navigating to the target item.

---

## 7. SPEC §14 Entries Committed

Committed in `c3c5a3b` to `docs/SPEC.md` under section 14:
- **1.7** — Notifier layering: `INotifier` abstract interface and `Notification` struct reside in `src/services/` free of UI/Widgets dependencies; concrete visual notifiers (`TrayNotifier`, `InAppNotifier`, `Toast`) live in `src/ui/` (Part D).
- **1.7** — `INotifier` delivery contract: `bool notify(const Notification&)` returns true only if presented to the user; false indicates delivery failure (e.g. window hidden or minimized), recording nothing in `reminders_sent` and retrying on the next periodic tick.
- **1.7** — `Notification` carries pure domain metadata (`kind`, `refId`, `company`, `position`, `interviewType`, `when`, `leadMinutes`, `count`), not presentation text. Formatted text is generated in `ui/`.
- **1.7** — Interview reminder reschedule: key `interview:<id>:<lead>` in `reminders_sent` is valid only if stored `sent_at >= start_at - lead`; otherwise it is treated as unsent, firing again without requiring cross-table deletes from `InterviewRepository`.
- **1.7** — Smallest-lead coalescing: when multiple leads are due simultaneously, exactly one notification for the smallest lead is delivered, and all due leads are marked sent. If a smaller lead was already delivered for an interview, larger due leads are marked sent silently without notifying.
- **1.7** — Overdue follow-ups: evaluated exclusively during the startup pass (`startupPass = true`) for non-closed applications with `next_action_date < today`. Produces a single notification with `refId` if 1 application is overdue, or a summary notification (`refId = 0`, `count = N`) if multiple are overdue. Key `overdue:<appId>:<today>` prevents repeats on same-day restarts.
- **1.7** — `reminders/interviewLeadMinutes` setting in `SettingsService`: integer list (default `{1440, 60}`), sanitized to positive unique values sorted descending (falls back to default if empty/invalid), with `interviewLeadMinutesChanged` signal.
- **1.7** — Requirement R4 "Close to tray" deferred to Milestone M6.

---

## 8. Verification and Raw Checks

### 8.1 Layering Grep Check
```bash
$ grep -rnE "StatusStyle|Theme::|Tokens|ui/|QtGui|QIcon|QPixmap|QColor|QPainter|QFont|QtWidgets|QSystemTrayIcon" src/domain src/data src/services src/models
src/services/SettingsService.cpp:51:    return m_settings->value(u"ui/sidebarCollapsed"_s, false).toBool();
src/services/SettingsService.cpp:56:    m_settings->setValue(u"ui/sidebarCollapsed"_s, collapsed);
src/services/SettingsService.cpp:61:    return m_settings->value(u"ui/themeMode"_s, u"system"_s).toString();
src/services/SettingsService.cpp:66:    m_settings->setValue(u"ui/themeMode"_s, mode);
src/services/SettingsService.cpp:71:    return m_settings->value(u"ui/accentPreset"_s, u"indigo"_s).toString();
src/services/SettingsService.cpp:76:    m_settings->setValue(u"ui/accentPreset"_s, preset);
src/services/INotifier.h:18:/// Presentation text is built exclusively by notifiers in ui/.
src/models/ApplicationTableModel.cpp:105:            // The label lives in StatusStyle, which the status badge delegate paints;
src/models/TopicListModel.h:20:/// are handled by delegates in ui/.
```
Zero forbidden visual or UI imports/symbols across non-UI layers.

### 8.2 `removeForInterview` and `markSent(` Grep Check
```bash
$ grep -rn "removeForInterview\|markSent(" src
src/services/ReminderService.cpp:112:        m_reminderLog.markSent(key, now);
src/services/ReminderService.cpp:122:                m_reminderLog.markSent(key, now);
src/data/ReminderLogRepository.cpp:33:bool ReminderLogRepository::markSent(const QString& key, const QDateTime& sentAt) {
src/data/ReminderLogRepository.h:27:    bool markSent(const QString& key, const QDateTime& sentAt = QDateTime());
```
Zero `removeForInterview` calls anywhere in the codebase. Both `markSent` calls in `ReminderService` explicitly pass the injected clock value `now`—no default clock parameter calls.

### 8.3 Test Counts per Binary and Total
```bash
$ for bin in $(find build/dev-linux/tests -maxdepth 2 -type f -executable ! -name "*.sh" ! -name "*.cmake" | sort); do
    name=$(basename "$bin")
    count=$("$bin" -functions | wc -l)
    printf "%-30s %3d\n" "$name" "$count"
done | awk '{print $0; sum += $2} END {printf "%-30s %3d\n", "TOTAL", sum}'
tst_ApplicationRepository       18
tst_Database                    16
tst_InterviewRepository          9
tst_ReminderLogRepository        7
tst_SeedService                  5
tst_SessionRepository           10
tst_TopicRepository             18
tst_TrackRepository              8
tst_TransactionBehavior          8
tst_AgendaListModel              5
tst_ApplicationFilterProxy       2
tst_ApplicationTableModel        6
tst_TopicBoardProxy              9
tst_TopicListModel               7
tst_TopicTableProxy              6
tst_AgendaService               24
tst_DayWatcher                   4
tst_ExportService                4
tst_ReminderService             10
tst_SettingsService              4
tst_StatsService                10
tst_MonthView                   13
ui_smoke                        78
TOTAL                          281
```
Total test functions: **281 across 23 binaries**.
Baseline was 265 across 21 binaries (+2 in `tst_ReminderLogRepository`, +10 in `tst_ReminderService`, +4 in `tst_SettingsService` = +16 total).

### 8.4 Full Quality Check (`scripts/check.sh`) Output
```bash
$ CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.2s)
-- Generating done (0.2s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
ninja: no work to do.
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start  1: tst_Database
 1/23 Test  #1: tst_Database .....................   Passed    0.03 sec
      Start  2: tst_TrackRepository
 2/23 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start  3: tst_TopicRepository
 3/23 Test  #3: tst_TopicRepository ..............   Passed    0.19 sec
      Start  4: tst_SessionRepository
 4/23 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  5: tst_ApplicationRepository
 5/23 Test  #5: tst_ApplicationRepository ........   Passed    0.16 sec
      Start  6: tst_InterviewRepository
 6/23 Test  #6: tst_InterviewRepository ..........   Passed    0.03 sec
      Start  7: tst_ReminderLogRepository
 7/23 Test  #7: tst_ReminderLogRepository ........   Passed    0.02 sec
      Start  8: tst_SeedService
 8/23 Test  #8: tst_SeedService ..................   Passed    0.83 sec
      Start  9: tst_TransactionBehavior
 9/23 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start 10: ui_smoke
10/23 Test #10: ui_smoke .........................   Passed   80.06 sec
      Start 11: tst_MonthView
11/23 Test #11: tst_MonthView ....................   Passed    0.10 sec
      Start 12: tst_AgendaListModel
12/23 Test #12: tst_AgendaListModel ..............   Passed    0.04 sec
      Start 13: tst_ApplicationTableModel
13/23 Test #13: tst_ApplicationTableModel ........   Passed    0.07 sec
      Start 14: tst_ApplicationFilterProxy
14/23 Test #14: tst_ApplicationFilterProxy .......   Passed    0.03 sec
      Start 15: tst_TopicListModel
15/23 Test #15: tst_TopicListModel ...............   Passed    0.21 sec
      Start 16: tst_TopicBoardProxy
16/23 Test #16: tst_TopicBoardProxy ..............   Passed    0.12 sec
      Start 17: tst_TopicTableProxy
17/23 Test #17: tst_TopicTableProxy ..............   Passed    0.14 sec
      Start 18: tst_AgendaService
18/23 Test #18: tst_AgendaService ................   Passed    0.15 sec
      Start 19: tst_DayWatcher
19/23 Test #19: tst_DayWatcher ...................   Passed    0.02 sec
      Start 20: tst_ExportService
20/23 Test #20: tst_ExportService ................   Passed    0.02 sec
      Start 21: tst_ReminderService
21/23 Test #21: tst_ReminderService ..............   Passed    0.11 sec
      Start 22: tst_SettingsService
22/23 Test #22: tst_SettingsService ..............   Passed    0.02 sec
      Start 23: tst_StatsService
23/23 Test #23: tst_StatsService .................   Passed    2.22 sec

100% tests passed, 0 tests failed out of 23

Total Test time (real) =  84.68 sec
==> Quality check PASSED with zero warnings and all tests green!
```

### 8.5 Git Commit History and Diff Stat
```bash
$ git log --oneline -n 6
c3c5a3b docs(spec): document Part C reminder core, notifier contract and settings in section 14 (v1.7)
49bf09e feat(reminders): implement ReminderService, INotifier contract and settings
8b1ca12 refactor(ui): remove unused FormatHelper overloads and add Part B errata report
2ddfd6a docs(prompts): add M5 Part C go
8a96dd8 docs(prompts): add M5 Part B fixups
0183128 docs(reports): update M5 Part B report with fix-ups and accurate evidence
```

```bash
$ git show --stat 49bf09e
commit 49bf09ee38ec4d77afad09c754cb997ea02be1e7
Author: Tony <group.ik31@gmail.com>
Date:   Sat Oct 24 23:33:59 2026 +0300

    feat(reminders): implement ReminderService, INotifier contract and settings

 CMakeLists.txt                           |   3 +
 src/data/ReminderLogRepository.cpp       |   8 +
 src/data/ReminderLogRepository.h         |   3 +
 src/services/INotifier.h                 |  40 +++
 src/services/ReminderService.cpp         | 275 ++++++++++++++++++
 src/services/ReminderService.h           |  91 ++++++
 src/services/SettingsService.cpp         |  51 +++-
 src/services/SettingsService.h           |   5 +
 tests/data/tst_ReminderLogRepository.cpp |  23 ++
 tests/services/CMakeLists.txt            |   2 +
 tests/services/tst_ReminderService.cpp   | 478 +++++++++++++++++++++++++++++++
 tests/services/tst_SettingsService.cpp   |  90 ++++++
 12 files changed, 1068 insertions(+), 1 deletion(-)
```

---

## 9. Not Verified by Agent (User to Check)

1. **No UI visible in this part:** Part C intentionally adds no user-visible widgets or toasts; verification is 100% via unit tests with mock notifiers. Real visual presentation will be wired in Part D.
2. **Real timer firing over hours:** While periodic 60 s tick triggering was tested via injected clock passes and immediate `checkNow()`, continuous execution over multiple hours in a real GUI session requires manual end-to-end observation.
3. **OS Sleep / Wakeup & Resume:** When a system suspends and resumes hours later, Qt timers may catch up or skip intervals. The startup/tick design ensures that missed due reminders fire on the next tick, but sleep/resume behavior across host OSes should be verified manually.
