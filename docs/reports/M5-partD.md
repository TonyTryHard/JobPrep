# Milestone 5 Part D & Milestone 5 Final Report: Notifiers, Toasts, Wiring and Milestone Close

**Operating System:** Linux (Ubuntu 26.04.1 LTS x86_64, GCC 15.2.0, Qt 6.10.2, offscreen QPA for tests)  
**Date:** 2026-10-25  
**Code Commits:**
- `69a1571`: `fix(reminders): address C0 carry-overs for errata, reports and reminder service tests` (Part C carry-overs)
- `3f10ad1`: `feat(reminders): implement TrayNotifier, InAppNotifier, toasts, click-to-open and ReminderCoordinator` (Part D implementation)

---

## 1. Environment Verification

Commands run in this session:

```bash
$ lsb_release -d
Description:	Ubuntu 26.04.1 LTS
```

```bash
$ g++ --version | head -1
g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0
```

```bash
$ ./build/dev-linux/tests/ui/tst_Notifiers -functions | head -n 2
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
makeNotifierSelection()
```

---

## 2. C0 Outcomes (Part C Carry-overs)

### 2.1 Errata for the Errata (Font Bundling)
`resources/fonts` contains only `README.md` and there are zero calls to `addApplicationFont` in `src`:

```bash
$ ls resources/fonts
README.md
```

```bash
$ grep -rn "addApplicationFont" src
```
(Exit code 1, 0 matches)

As corrected in `docs/reports/M5-partB-errata.md` in commit `69a1571`, Inter font bundling is scheduled for Milestone M6 per `AGENTS.md` §12. All application widgets and captured screenshots run with the host operating system's default UI font (`Ubuntu` font / system sans-serif on Ubuntu 26.04).

### 2.2 Environment Line in M5 Part C Report
The operating system, GCC, and Qt versions in `docs/reports/M5-partC.md` were corrected in commit `69a1571` from Ubuntu 24.04/GCC 13.3.0/Qt 6.8.3 to `Ubuntu 26.04.1 LTS x86_64, GCC 15.2.0, Qt 6.10.2`.

### 2.3 Raw `-functions` Output for Part C Binaries
Raw output of `-functions` for `tst_SettingsService`, `tst_ReminderLogRepository`, and `tst_ReminderService`:

```bash
$ ./build/dev-linux/tests/services/tst_SettingsService -functions
defaultLeadMinutesReturns1440And60()
sanitizingRemovesNonPositiveDuplicatesAndSortsDescending()
emptyOrInvalidStoredValueFallsBackToDefault()
signalEmittedOncePerRealChange()
```

```bash
$ ./build/dev-linux/tests/data/tst_ReminderLogRepository -functions
keyIsRememberedWithTimestamp()
markSentTwiceKeepsFirstTimestamp()
removeForgetsKey()
sentKeysListsEveryKey()
emptyKeyIsRejected()
sentAtReturnsTimestampWhenPresent()
sentAtReturnsNulloptWhenAbsent()
```

```bash
$ ./build/dev-linux/tests/services/tst_ReminderService -functions
interview59MinutesAheadFiresOnceAndRecordsBothKeysNoRepeatOnRestart()
leads24hAnd1hFireSeparatelyWhenRunning()
smallerLeadAlreadyDeliveredLargerAddedLaterMarkedNotNotified()
cancelledStartedPastInterviewsIgnoredRejectedApplicationReminded()
rescheduleNotRescheduledDoesNotRepeat()
rescheduleMovedLaterFiresAgain()
rescheduleMovedEarlierIntoWindowFires()
rescheduleDeletedInterviewDoesNotFire()
rescheduleMarkSentFailsAfterRemoveNextCheckDeliversAgain()
notifierReturnsFalseNothingRecordedNextTickDeliversOnce()
followUpBefore0900NothingAt0900OnceClosedApplicationNothing()
overdueNormalCheckDoesNotEvaluate()
overdueStartupPassSingleAppProducesSingleNotification()
overdueStartupPassMultipleAppsProducesSummaryNotification()
overdueRestartSameDayDoesNotRepeatDelivered()
overdueNextDayDeliversAgain()
leadMinutesChangeTakesEffectOnNextCheck()
dayBorderInterviewAt0030With1440LeadDuePreviousDay()
```

Confirmation: `tst_SettingsService` covers default (`defaultLeadMinutesReturns1440And60`), sanitizing (`sanitizingRemovesNonPositiveDuplicatesAndSortsDescending`), empty/invalid fallback (`emptyOrInvalidStoredValueFallsBackToDefault`), and signal emission once per real change (`signalEmittedOncePerRealChange`).

### 2.4 Splitting Bundled Scenarios & Day Border Rename
The multi-scenario slots in `tst_ReminderService` were split into individual named slots:
- Reschedule cases split into: `rescheduleNotRescheduledDoesNotRepeat`, `rescheduleMovedLaterFiresAgain`, `rescheduleMovedEarlierIntoWindowFires`, `rescheduleDeletedInterviewDoesNotFire`.
- Overdue cases split into: `overdueNormalCheckDoesNotEvaluate`, `overdueStartupPassSingleAppProducesSingleNotification`, `overdueStartupPassMultipleAppsProducesSummaryNotification`, `overdueRestartSameDayDoesNotRepeatDelivered`, `overdueNextDayDeliversAgain`.
- Renamed `dstDayBorder...` to `dayBorderInterviewAt0030With1440LeadDuePreviousDay` (testing calendar day border, not DST).

### 2.5 Reschedule Two-Write Path and Failure Test
Exact command showing `remove` then `markSent` in `ReminderService.cpp`:

```bash
$ grep -n "remove(\|markSent(" src/services/ReminderService.cpp
110:            m_reminderLog.remove(key);
112:        m_reminderLog.markSent(key, now);
120:                    m_reminderLog.remove(key);
122:                m_reminderLog.markSent(key, now);
```

Test slot `rescheduleMarkSentFailsAfterRemoveNextCheckDeliversAgain` verifies that if `markSent` fails after `remove` (tested via a temporary SQLite trigger causing abort), the app does not crash, the key remains unrecorded, and the next check delivers the reminder again with no silent loss.

---

## 3. Part D Implementation & Architecture

| Component | Files | Description & Responsibilities |
| :--- | :--- | :--- |
| **`NotificationText`** | `src/ui/NotificationText.h`, `.cpp` | Presentation text builder converting `Notification` data payloads into natural language strings via `tr()`, `StatusStyle::label(InterviewType)` and `FormatHelper`. Formats natural leads ("in 1 hour", "in 24 hours", "in 90 minutes") with singular/plural support, single overdue follow-ups ("Company — Position"), and summary overdue counts ("N follow-ups are overdue"). |
| **`Toast`** | `src/ui/Toast.h`, `.cpp` | In-app sticky toast notification widget. Features 12px rounded borders, surface contrast, themed SVG icon, bold title and muted message labels, flat close button, and focus border. Never takes keyboard focus on show; responds to Esc or close button to dismiss; Enter or mouse click opens item. Theme changes update colors dynamically at runtime. |
| **`InAppNotifier`** | `src/ui/InAppNotifier.h`, `.cpp` | Implements `INotifier`. Returns `false` when `MainWindow` is hidden or minimized. Stacks up to 3 visible toasts plus an overflow indicator ("+N more") above the status bar; automatically repositions on window resize via event filter. Intercepts `Escape` to dismiss newest/focused toast. |
| **`TrayNotifier`** | `src/ui/TrayNotifier.h`, `.cpp` | Implements `INotifier` using `QSystemTrayIcon`. Loads monochrome `app.svg` tinted by `tokens.accent`. Caches last notification for payload-free `messageClicked` dispatch. Provides minimal tray context menu ("Show JobPrep" and "Quit") and left-click trigger that calls `MainWindow::bringToFront()`. |
| **`NotifierFactory`** | `src/ui/NotifierFactory.h`, `.cpp` | Pure factory function `makeNotifier(bool trayAvailable, MainWindow&, clickHandler)` returning `std::unique_ptr<INotifier>`. Returns `TrayNotifier` when tray is available and `InAppNotifier` as fallback. |
| **`ReminderCoordinator`** | `src/ui/ReminderCoordinator.h`, `.cpp` | Composition coordinator living in `src/ui/`. Owns `std::unique_ptr<INotifier> m_notifier` and `std::unique_ptr<ReminderService> m_service` declared in order (service destroyed before notifier). Created in `main.cpp` after `MainWindow` and started after `window.show()`. Connects `SettingsService::interviewLeadMinutesChanged` to `ReminderService::setLeadMinutes`. Accepts injectable clock for headless tests without auto-starting timers. |
| **Click-to-Open Routing** | `src/ui/MainWindow.h`, `.cpp` | Added `bringToFront()`, `routeNotification(const Notification&)`, and `openItemFromNotification(const Notification&)`. Routes `Interview` to `openInterview(refId)`, `FollowUp` and single `OverdueFollowUps` to `openApplication(refId)`, and summary (`refId == 0`) to `JobsPage`. |

---

## 4. Part D Amendments Checklist (D1–D5)

- [x] **D1. `TrayNotifier`:** Uses existing `app.svg` tinted by accent token; caches last notification for `messageClicked`; returns true after `showMessage`; provides context menu with "Show JobPrep" and "Quit"; left click calls `bringToFront()`. Recorded in SPEC §14 (1.7).
- [x] **D2. `InAppNotifier` and `Toast`:** Returns false when main window is hidden or minimized; sticky (no auto-dismiss timer); at most 3 visible plus "+N more" line; repositioned on resize; border and surface contrast (no `QGraphicsDropShadowEffect`); never steals focus; Esc/close button dismisses; Enter/click opens item; accessible name and description; tokens only; both themes.
- [x] **D3. Click to open:** `MainWindow::openItemFromNotification` calls `bringToFront()` then `routeNotification(Notification)`. `bringToFront()` performs `showNormal()` if minimized, `raise()`, `activateWindow()`, with no `platformName()` branch. Offscreen tests call `routeNotification` directly. No QPA noise filter widening.
- [x] **D4. End-to-end tests:** Interview 59 minutes ahead fires once and clicking opens `InterviewDialog`; overdue summary switches to Jobs page; minimized window delays delivery until `showNormal()`; no tray falls back to in-app toast; lead minutes setting change takes effect on next check.
- [x] **D5. Final M5 report:** SPEC §12 acceptance criteria quoted verbatim and checked against exact test slot names; screenshots for dashboard with data, empty, single toast, and toast stack in light and dark themes; honest "Not verified by agent, user to check" list.

---

## 5. Milestone 5 Acceptance Criteria (SPEC §12)

Quoted verbatim from `docs/SPEC.md`:

```bash
$ grep -n -A8 "M5 — Dashboard" docs/SPEC.md
363:**M5 — Dashboard & reminders.**
364-- `StatsService` + tests (fixed clock), `StatCard`, `ProgressRing`, `FunnelWidget`, Today/Upcoming lists, status bar summary, `ReminderService` + `INotifier` with `TrayNotifier` and automatic `InAppNotifier` fallback, `reminders_sent`. The weekly goal is read from `SettingsService` (default 360 min).
365-- Acceptance: numbers match hand-calculated test data; goal progress uses 360 min by default and a different goal passed in tests; a reminder for an interview in 59 minutes fires once and clicking it opens the interview; with no system tray the reminder appears as an in-app toast.
366-
367-**M6 — Polish & data tools.**
368-- Settings page complete (theme, accent, week start, **weekly study goal control**, reminder lead times, close-to-tray), JSON backup/restore, ICS export, empty states everywhere, keyboard shortcuts, app icon, tab order pass, HiDPI check.
369-- Run instructions: walk through `docs/RUNNING.md` step by step on the OS available and fix anything inaccurate. No installers and no AppImage; the app runs from the build folder.
370-- Optional, only if the repo is hosted on GitHub: a CI workflow building and testing on `windows-latest` and `ubuntu-latest`.
371-- Acceptance: backup → wipe → restore reproduces identical data; all shortcuts from §3.7 work; changing the weekly goal from 6 h to 8 h updates the dashboard instantly and survives a restart; no clipped text at 150% scaling in either theme; app starts on Windows and on Linux (X11 or Wayland).
```

### Verification Against Acceptance Criteria:

1. **"numbers match hand-calculated test data"**
   - Verified in service layer: `tst_StatsService::testHandCalculatedDataset` (Part A) asserting applied this week (2), active processes (8), funnel counts (9, 6, 4, 2, 1), interview rate (66.7%), weekly minutes (260), streak (4).
   - Verified in UI smoke: `tst_UiSmoke::testDashboardNumbersMatchHandCalculatedDataset` (Part B) asserting rendered StatCards, FunnelWidget, and progress bars.
2. **"goal progress uses 360 min by default and a different goal passed in tests"**
   - Verified in service layer: `tst_StatsService::testHandCalculatedDataset` checking 360 min (72%) and 480 min (54%).
   - Verified in UI smoke: `tst_UiSmoke::testDashboardNumbersMatchHandCalculatedDataset` and `tst_UiSmoke::testWeeklyGoalChangeUpdatesBarAndTextImmediately` testing live updates when goal changes to 480 and 300 minutes.
3. **"a reminder for an interview in 59 minutes fires once and clicking it opens the interview"**
   - Verified in core reminder service: `tst_ReminderService::interview59MinutesAheadFiresOnceAndRecordsBothKeysNoRepeatOnRestart` (Part C).
   - Verified in coordinator end-to-end: `tst_Notifiers::coordinatorEndToEndInterview59MinutesAheadOneToastAndOneDbPair` asserting exactly one toast delivered and both keys (`interview:N:60` and `interview:N:1440`) recorded in `reminders_sent`.
   - Verified in click-to-open: `tst_Notifiers::coordinatorEndToEndClickOpensInterviewDialog` asserting clicking the toast opens `InterviewDialog` for that interview.
4. **"with no system tray the reminder appears as an in-app toast"**
   - Verified in notifier factory: `tst_Notifiers::makeNotifierSelection` asserting `makeNotifier(false, ...)` yields `InAppNotifier`.
   - Verified in coordinator fallback: `tst_Notifiers::coordinatorEndToEndNoTrayFallbackToToast` asserting coordinator uses `InAppNotifier` when tray is unavailable.
   - Verified visually and functionally: `tst_Notifiers::inAppNotifierVisibleWindowShowsToastAndNoAutoDismiss` and `tst_UiSmoke::testToastNotificationSmoke`.

---

## 6. SPEC §14 (1.7) Entries Verification

Exact command showing all 1.7 changelog entries in `docs/SPEC.md`:

```bash
$ grep -n "^- 1\.7" docs/SPEC.md | cut -c1-160
410:- 1.7 — `domain::stageRank`: pure domain helper mapping `ApplicationStatus` to stage rank 0..5 (wishlist 0, applied 1, hr_screen 2, technical 3, final 4, of
411:- 1.7 — `DashboardStats` in `src/services/StatsService.h` holds numeric and domain types only (`appliedThisWeek`, `activeProcesses`, `interviewsNext7Days`, 
412:- 1.7 — "Interviews in next 7 days" window is defined as `[now, QDateTime(today.addDays(6), 23:59:59)]`, strictly covering today plus the next 6 calendar da
413:- 1.7 — Only `outcome == cancelled` excludes an interview from next 7 days, Upcoming, Today, status bar "Next:", and reminders. Interviews of closed applica
414:- 1.7 — Study streak counts consecutive days with sessions strictly up to and including `today`; future-dated sessions neither count toward nor break the st
415:- 1.7 — `AgendaService::todayItems` returns today's non-cancelled interviews, overdue/due follow-ups, and overdue/due open topics. Deterministic ordering: o
416:- 1.7 — `AgendaService::upcomingInterviews` returns upcoming non-cancelled interviews starting on or after `from`, ordered by start_at, id.
417:- 1.7 — `InterviewRepository::inRange` behavior is untouched. `countUpcoming(from, to)` and `upcoming(from, limit)` added to data layer, both excluding canc
418:- 1.7 — Data layer queries for stats and agenda: `ApplicationRepository::appliedCountInRange`, `funnelStageCounts`, `byNextActionDateUpTo`, `SessionReposito
419:- 1.7 — `DayWatcher` in `src/services/`: periodic 60-second timer service detecting calendar day rollover across midnight, emitting `dayChanged(QDate)` with
420:- 1.7 — `DashboardController` in `src/ui/`: centralized refresh owner batching mutations from repositories (`apps`, `interviews`, `sessions`, `topics`, `tra
421:- 1.7 — `FormatHelper` in `src/ui/`: presentation formatting utility for durations, weekly goal text, streak wording, interview rates (`std::optional<double
422:- 1.7 — `GoalProgressBar` in `src/ui/widgets/`: custom-painted horizontal widget for weekly study goal progress; visual fill clamped to 100% while actual pe
423:- 1.7 — `EmptyState` in `src/ui/widgets/`: supports optional secondary action button for dual quick-actions (`+ Add application` and `+ Add topic`) on `Home
424:- 1.7 — `MainWindow::openInterview(int)` and `MainWindow::openApplication(int)`: switch page stack to owning page (`CalendarPage` or `JobsPage`) and delegat
425:- 1.7 — Notifier layering: `INotifier` abstract interface and `Notification` struct reside in `src/services/` free of UI/Widgets dependencies; concrete visu
426:- 1.7 — `INotifier` delivery contract: `bool notify(const Notification&)` returns true only if presented to the user; false indicates delivery failure (e.g.
427:- 1.7 — `Notification` carries pure domain metadata (`kind`, `refId`, `company`, `position`, `interviewType`, `when`, `leadMinutes`, `count`), not presentat
428:- 1.7 — Interview reminder reschedule: key `interview:<id>:<lead>` in `reminders_sent` is valid only if stored `sent_at >= start_at - lead`; otherwise it is
429:- 1.7 — Smallest-lead coalescing: when multiple leads are due simultaneously, exactly one notification for the smallest lead is delivered, and all due leads
430:- 1.7 — Overdue follow-ups: evaluated exclusively during the startup pass (`startupPass = true`) for non-closed applications with `next_action_date < today`
431:- 1.7 — `reminders/interviewLeadMinutes` setting in `SettingsService`: integer list (default `{1440, 60}`), sanitized to positive unique values sorted desce
432:- 1.7 — Requirement R4 "Close to tray" deferred to Milestone M6.
433:- 1.7 — `NotificationText` in `src/ui/`: presentation text helper constructing localized titles and messages from `Notification` payloads; formats natural l
434:- 1.7 — `TrayNotifier` in `src/ui/`: `INotifier` implementation using `QSystemTrayIcon` with themed `app.svg` icon; displays OS notifications via `showMessa
435:- 1.7 — `InAppNotifier` and `Toast` in `src/ui/`: fallback in-app notification system displaying up to 3 sticky toasts with an overflow indicator (`+N more`
436:- 1.7 — `NotifierFactory` (`makeNotifier`) in `src/ui/`: testable selection function returning `TrayNotifier` when system tray is available or `InAppNotifie
437:- 1.7 — `ReminderCoordinator` in `src/ui/`: owns `std::unique_ptr<INotifier>` and `std::unique_ptr<ReminderService>` in declaration order ensuring service d
438:- 1.7 — `MainWindow::bringToFront()`, `routeNotification(const Notification&)` and `openItemFromNotification(const Notification&)`: routes `Interview` to `o
```

---

## 7. Automated Verification & Quality Checks

### 7.1 Layering Check
No UI headers, widgets, or theme references leaked upward into domain, data, services, or models:

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
*(All matches are settings key string literals or explanatory doc comments)*

### 7.2 No Hard-coded Colors
Zero hard-coded hex colors in `src/ui` outside `src/ui/theme`:

```bash
$ grep -rnE "#[0-9a-fA-F]{6}" src/ui | grep -v "^src/ui/theme/"
```
*(Exit code 1, 0 matches)*

### 7.3 No Fixed Sizes Added
Zero calls to `setFixedSize`, `setFixedWidth`, or `setFixedHeight` in new code:

```bash
$ grep -rnE "setFixedSize|setFixedWidth|setFixedHeight" src/ui
src/ui/widgets/TopicDetailPanel.cpp:88:    m_closeBtn->setFixedSize(28, 28);
src/ui/dialogs/TrackDialog.cpp:133:        btn->setFixedSize(32, 32);
src/ui/dialogs/TrackDialog.cpp:148:    m_customSwatchBtn->setFixedSize(32, 32);
src/ui/Sidebar.cpp:26:        setFixedHeight(42);
src/ui/Sidebar.cpp:173:    m_toggleBtn->setFixedHeight(36);
src/ui/Sidebar.cpp:189:    setFixedWidth(collapsed ? kWidthCollapsed : kWidthExpanded);
```

### 7.4 No Double Ownership
Exactly one `make_unique<Services::ReminderService>` owned without a Qt parent inside `ReminderCoordinator`:

```bash
$ grep -rn "make_unique<.*ReminderService\|new ReminderService" src
src/ui/ReminderCoordinator.cpp:24:    m_service = std::make_unique<Services::ReminderService>(
```

### 7.5 Test Counts and Totals by Command

```bash
$ find ./build/dev-linux/tests -maxdepth 2 -type f -executable ! -name "*.sh" | sort | while read -r f; do echo "$(basename "$f"): $("$f" -functions | wc -l)"; done
tst_ApplicationRepository: 18
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
tst_ApplicationTableModel: 6
tst_TopicBoardProxy: 9
tst_TopicListModel: 7
tst_TopicTableProxy: 6
tst_AgendaService: 24
tst_DayWatcher: 4
tst_ExportService: 4
tst_ReminderService: 18
tst_SettingsService: 4
tst_StatsService: 10
tst_MonthView: 13
ui_smoke: 80
tst_Notifiers: 19
```

Sum across all 24 binaries:

```bash
$ find ./build/dev-linux/tests -maxdepth 2 -type f -executable ! -name "*.sh" | sort | while read -r f; do "$f" -functions | wc -l; done | awk '{s+=$1} END {print "TOTAL TEST FUNCTIONS:", s}'
TOTAL TEST FUNCTIONS: 310
```

Summary comparison against baseline:
- Test binaries: 23 → **24** (+1: `tst_Notifiers`)
- `ui_smoke` test functions: 78 → **80** (+2: `testToastNotificationSmoke`, `testToastScreenshots`)
- `tst_ReminderService` test functions: 10 → **18** (+8: split reschedule/overdue slots and SQL failure test)
- `tst_Notifiers` test functions: 0 → **19** (+19 new notifier functions)
- Total test functions: 281 → **310** (+29)

### 7.6 Full Test Suite Run (`check.sh`)

```bash
$ CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.5s)
-- Generating done (0.2s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
ninja: no work to do.
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start  1: tst_Database
 1/24 Test  #1: tst_Database .....................   Passed    0.04 sec
      Start  2: tst_TrackRepository
 2/24 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start  3: tst_TopicRepository
 3/24 Test  #3: tst_TopicRepository ..............   Passed    0.19 sec
      Start  4: tst_SessionRepository
 4/24 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  5: tst_ApplicationRepository
 5/24 Test  #5: tst_ApplicationRepository ........   Passed    0.15 sec
      Start  6: tst_InterviewRepository
 6/24 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start  7: tst_ReminderLogRepository
 7/24 Test  #7: tst_ReminderLogRepository ........   Passed    0.03 sec
      Start  8: tst_SeedService
 8/24 Test  #8: tst_SeedService ..................   Passed    0.85 sec
      Start  9: tst_TransactionBehavior
 9/24 Test  #9: tst_TransactionBehavior ..........   Passed    0.04 sec
      Start 10: ui_smoke
10/24 Test #10: ui_smoke .........................   Passed   99.77 sec
      Start 11: tst_MonthView
11/24 Test #11: tst_MonthView ....................   Passed    0.10 sec
      Start 12: tst_AgendaListModel
12/24 Test #12: tst_AgendaListModel ..............   Passed    0.04 sec
      Start 13: tst_ApplicationTableModel
13/24 Test #13: tst_ApplicationTableModel ........   Passed    0.08 sec
      Start 14: tst_ApplicationFilterProxy
14/24 Test #14: tst_ApplicationFilterProxy .......   Passed    0.03 sec
      Start 15: tst_TopicListModel
15/24 Test #15: tst_TopicListModel ...............   Passed    0.22 sec
      Start 16: tst_TopicBoardProxy
16/24 Test #16: tst_TopicBoardProxy ..............   Passed    0.12 sec
      Start 17: tst_TopicTableProxy
17/24 Test #17: tst_TopicTableProxy ..............   Passed    0.13 sec
      Start 18: tst_AgendaService
18/24 Test #18: tst_AgendaService ................   Passed    0.15 sec
      Start 19: tst_DayWatcher
19/24 Test #19: tst_DayWatcher ...................   Passed    0.02 sec
      Start 20: tst_ExportService
20/24 Test #20: tst_ExportService ................   Passed    0.02 sec
      Start 21: tst_ReminderService
21/24 Test #21: tst_ReminderService ..............   Passed    0.12 sec
      Start 22: tst_SettingsService
22/24 Test #22: tst_SettingsService ..............   Passed    0.02 sec
      Start 23: tst_StatsService
23/24 Test #23: tst_StatsService .................   Passed    2.27 sec
      Start 24: tst_Notifiers
24/24 Test #24: tst_Notifiers ....................   Passed    2.16 sec

100% tests passed, 0 tests failed out of 24

Total Test time (real) = 106.67 sec
==> Quality check PASSED with zero warnings and all tests green!
```

---

## 8. Screenshot Generation

Screenshots regenerated via `scripts/screenshots.sh`:

```bash
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
PASS   : tst_UiSmoke::testToastScreenshots()
PASS   : tst_UiSmoke::cleanupTestCase()
Totals: 6 passed, 0 failed, 0 skipped, 0 blacklisted, 34990ms
********* Finished testing of tst_UiSmoke *********
==> Screenshots successfully regenerated in /home/tony/projects/JobPrep/docs/reports/img!
```

MD5 hashes of generated toast screenshots:

```bash
$ md5sum docs/reports/img/dashboard-toast*.png
9974869052d5879056e3a832876810c9  docs/reports/img/dashboard-toast-dark.png
59e2cf507900c672fea51f64f61888af  docs/reports/img/dashboard-toast-light.png
9eafc9970410800dab442dca0d5edc6a  docs/reports/img/dashboard-toasts-stack-dark.png
d4097cf49a5db2b91abbb39d18471005  docs/reports/img/dashboard-toasts-stack-light.png
```
*(All 4 hashes are distinct)*

---

## 9. Git Status and Commit History

```bash
$ git status --short
```
*(Working tree clean before writing this report)*

```bash
$ git log --oneline -n 6
3f10ad1 (HEAD -> main) feat(reminders): implement TrayNotifier, InAppNotifier, toasts, click-to-open and ReminderCoordinator
69a1571 fix(reminders): address C0 carry-overs for errata, reports and reminder service tests
e1be951 docs(prompts): add M5 Part D go
66ffdae docs(reports): add M5 Part C report
c3c5a3b docs(spec): document Part C reminder core, notifier contract and settings in section 14 (v1.7)
49bf09e feat(reminders): implement ReminderService, INotifier contract and settings
```

```bash
$ git show --stat 69a1571
commit 69a1571fc6094bceaf4871ab5b34bb5ae88b9af2
Author: Tony <group.ik31@gmail.com>
Date:   Sun Oct 25 00:01:34 2026 +0300

    fix(reminders): address C0 carry-overs for errata, reports and reminder service tests

 docs/reports/M5-partB-errata.md        |   9 ++
 docs/reports/M5-partC.md               |   4 +-
 tests/services/tst_ReminderService.cpp | 225 ++++++++++++++++++++++++++++-----
 3 files changed, 205 insertions(+), 33 deletions(-)
```

```bash
$ git show --stat 3f10ad1
commit 3f10ad1c86cd35b5dc3f1f1a3ed18c8e83bb568e (HEAD -> main)
Author: Tony <group.ik31@gmail.com>
Date:   Sun Oct 25 08:33:59 2026 +0200

    feat(reminders): implement TrayNotifier, InAppNotifier, toasts, click-to-open and ReminderCoordinator

 CMakeLists.txt                                    |  14 +
 docs/SPEC.md                                      |   6 +
 docs/reports/img/dashboard-toast-dark.png         | Bin 0 -> 84218 bytes
 docs/reports/img/dashboard-toast-light.png        | Bin 0 -> 82796 bytes
 docs/reports/img/dashboard-toasts-stack-dark.png  | Bin 0 -> 75887 bytes
 docs/reports/img/dashboard-toasts-stack-light.png | Bin 0 -> 74597 bytes
 scripts/screenshots.sh                            |   2 +-
 src/main.cpp                                      |  14 +
 src/ui/InAppNotifier.cpp                          | 195 +++++++
 src/ui/InAppNotifier.h                            |  58 ++
 src/ui/MainWindow.cpp                             |  37 ++
 src/ui/MainWindow.h                               |  20 +
 src/ui/NotificationText.cpp                       |  93 ++++
 src/ui/NotificationText.h                         |  16 +
 src/ui/NotifierFactory.cpp                        |  19 +
 src/ui/NotifierFactory.h                          |  16 +
 src/ui/ReminderCoordinator.cpp                    |  68 +++
 src/ui/ReminderCoordinator.h                      |  57 ++
 src/ui/Toast.cpp                                  | 159 ++++++
 src/ui/Toast.h                                    |  50 ++
 src/ui/TrayNotifier.cpp                           |  75 +++
 src/ui/TrayNotifier.h                             |  45 ++
 tests/CMakeLists.txt                              |   1 +
 tests/ui/CMakeLists.txt                           |   9 +
 tests/ui/tst_Notifiers.cpp                        | 616 ++++++++++++++++++++++
 tests/ui_smoke/tst_UiSmoke.cpp                    | 160 ++++++
 26 files changed, 1729 insertions(+), 1 deletion(-)
```

---

## 10. Milestone 5 Summary: Plan vs. Result

Across all four parts (A, B, C, D), Milestone 5 delivered:
1. **Part A (Data & Core Services):** Added `domain::stageRank`, aggregate SQL for hiring funnel and application counts, streak and weekly minutes calculation, `AgendaService::todayItems` and `upcomingInterviews`, plus the comprehensive hand-calculated test suite.
2. **Part B (Dashboard Widgets & Navigation):** Added `StatCard`, `ProgressRing`, `FunnelWidget`, `GoalProgressBar`, and `EmptyState` dual-action. Wired single-coalesced refresh via `DashboardController`, `DayWatcher` midnight monitoring, status bar next-interview label, and page navigation routing.
3. **Part C (Reminder Core & Delivery Contract):** Added `ReminderService`, `INotifier` abstract interface, and `Notification` data struct. Implemented smallest-lead coalescing, reschedule staleness check without cross-table writes, 09:00 follow-ups, overdue startup summaries, and `reminders/interviewLeadMinutes` setting.
4. **Part D (Visual Notifiers & Milestone Close):** Added `NotificationText` formatting, `Toast` custom widget, `InAppNotifier` sticky toast stack with overflow indicator, `TrayNotifier` with context menu, `makeNotifier` factory, and `ReminderCoordinator` composition root. Integrated click-to-open routing and end-to-end offscreen test suite.

---

## 11. Not Verified by Agent, User to Check

As required by prompt rules and `AGENTS.md` §12:
1. **Real desktop system tray path:** Under offscreen QPA testing, system tray notification bubbles cannot pop on a real desktop environment. On Ubuntu 26.04 GNOME, Ubuntu bundles the AppIndicator extension so `QSystemTrayIcon::isSystemTrayAvailable()` returns true; on other desktop environments or with AppIndicator disabled, it falls back to `InAppNotifier`. The user should test real desktop notification bubbles and menu behavior on GNOME, KDE, and Windows.
2. **Notification click interaction on GNOME/KDE/Windows:** Verify that clicking the desktop tray bubble activates the main window and routes to the correct target dialog/page.
3. **Wayland raise / window focus:** On native Wayland, compositors may disallow background or unprompted window raising (`raise()` / `activateWindow()`). Verify behavior when clicking a toast or tray notification on Wayland.
4. **Reminders over extended wall-clock time and sleep/wake cycles:** Automated tests use injected fixed clocks; test real periodic reminder ticks over several hours and after laptop suspend/resume.
5. **High-DPI scaling (100%–200%):** Verify toast layouts and "+N more" badges scale cleanly on 125%, 150%, and 200% displays in both light and dark themes.
6. **Toast stack at minimum window dimensions:** Verify that on the minimum window size (1100x700), toast stacks do not obstruct primary action areas.
7. **Midnight rollover with live timer:** Verify the live application running across 00:00 triggers `DayWatcher::dayChanged` and refreshes dashboard streaks and agenda items.
