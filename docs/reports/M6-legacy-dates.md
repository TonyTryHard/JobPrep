# M6 Legacy Timestamps Report

## 1. Task Baseline & Git History

```
$ git log --oneline -8
5fb0f8a (HEAD -> main) docs(reports): add M6 Part D2 fix-ups final report
2a5f61c docs(spec): document M6 Part D2 fix-ups and supersede header
6e453b9 docs: document isolated data usage, update running guide, and add agent safety rule (F2, F3)
d3198d5 test(ui): strengthen HiDpi test with widgetText, QGroupBox advance, and virtual screen sizing (F4)
373ea26 fix(tests): guard against real AppData access in safety backup restore test (F7)
a77abcd fix(ui): eliminate fixed pixel sizes in pages and dialogs using font metrics (F5)
6ee3067 docs(reports): add M6 CI fix-up 5 report and update SPEC §14
9bfab8d ci(windows): keep CI-DIAG output lines in diagnostic log filtering
```

---

## 2. Evidence Audit (Date & Timestamp Presentation)

Grep command executed across `src/`:
```
$ grep -rn "createdAt\|updatedAt\|appliedAt\|fromMSecsSinceEpoch\|fromSecsSinceEpoch\|toString(" src | cut -c1-160
```

### Classification Table

| Source Location | Field / Expression | Category | Affected by Cutoff? | Code Distinction & Handling |
| :--- | :--- | :--- | :--- | :--- |
| `src/models/ApplicationTableModel.cpp:119` | `app.updatedAt` | System timestamp | **Yes (Masked)** | DisplayRole returns `DbFormat::formatStoredTimestamp(app.updatedAt)` (`"—"` if `< 2013-01-01`). SortRole returns stored `app.updatedAt` for chronological sorting. |
| `src/ui/dialogs/ApplicationDialog.cpp:463` | `change.changedAt` | System timestamp | **Yes (Masked)** | Status history change timestamp. Uses `DbFormat::formatStoredTimestamp(change.changedAt)` (`"—"` if `< 2013-01-01`). |
| `src/models/ApplicationTableModel.cpp:108` | `app.appliedDate` | User-entered date | **No (Visible)** | Explicit date entered by user when applying. Stays visible even if before 2013. Formatted via `DbFormat::toText()`. |
| `src/models/ApplicationTableModel.cpp:88` | `app.nextActionDate` | User-entered date | **No (Visible)** | Explicit date scheduled by user for next follow-up. Stays visible. Formatted via `DbFormat::toText()`. |
| `src/models/ApplicationTableModel.cpp:85` | `nextInterviewAt` | User-scheduled date-time | **No (Visible)** | Explicit interview date/time scheduled by user. Stays visible. Formatted via `toString("yyyy-MM-dd HH:mm")`. |
| `src/models/TopicListModel.cpp:137` | `topic.targetDate` | User-entered date | **No (Visible)** | Explicit study topic target date chosen by user. Stays visible. Formatted via `DbFormat::toText()`. |
| `src/ui/dialogs/ApplicationDialog.cpp:500` | `interview.startAt` | User-scheduled date-time | **No (Visible)** | Scheduled interview start date/time. Stays visible in interviews table. Formatted via `DbFormat::toText(date)`. |
| `src/ui/dialogs/FollowUpDialog.cpp:68, 122` | `m_date`, `nextActionDate` | User-entered date | **No (Visible)** | Follow-up action dialog dates chosen by user. Unaffected. |
| `src/ui/pages/CalendarPage.cpp:403` | `day` | User-selected date | **No (Visible)** | Calendar navigation day viewed in agenda header. Unaffected. |
| `src/ui/pages/JobsPage.cpp:496, 511, 512` | `stored->createdAt/updatedAt` | Data export | **No (Raw ISO)** | CSV/TSV table export via `buildExportRows()`. Writes raw stored ISO-8601 values unchanged. |
| `src/services/ExportService.cpp:111, 124` | `app.createdAt/updatedAt` | Data export | **No (Raw ISO)** | CSV and JSON backup export. Writes raw stored ISO-8601 values unchanged. |
| `src/services/RestoreService.cpp:233, 326` | `createdAt`, `updatedAt` | Data restore | **No (Raw ISO)** | Validates ISO-8601 string and writes directly to database; does not mutate legacy dates. |

---

## 3. Implementation Details

- Single cutoff constant defined in `src/data/DbFormat.h` and `src/data/DbFormat.cpp`:
  `const QDateTime kLegacyTimestampCutoffUtc{QDate(2013, 1, 1), QTime(0, 0, 0), QTimeZone::UTC};`
- Plausibility check and presentation helper:
  - `bool isPlausibleTimestamp(const QDateTime& dateTime)` (compares UTC or local date against 2013-01-01 cutoff).
  - `bool isPlausibleTimestamp(const QDate& date)`.
  - `QString formatStoredTimestamp(const QDateTime& dateTime)` (returns `u"—"_s` if legacy; otherwise `yyyy-MM-dd`).
  - `QString formatStoredTimestamp(const QDate& date)`.
- Applied to UI presentation:
  - `ApplicationTableModel::data()`: `UpdatedColumn` `Qt::DisplayRole` formats via `formatStoredTimestamp(app.updatedAt)`. `ApplicationTableModel::SortRole` continues to return raw `app.updatedAt` (`QDateTime`).
  - `ApplicationDialog::reloadReadOnlyTabs()`: timeline status history table column 0 formats via `formatStoredTimestamp(change.changedAt)`.
- Export/Backup preservation:
  - `ExportService.cpp` and `JobsPage.cpp` export methods continue to write raw stored ISO-8601 values directly via `DbFormat::toText()`. No repair or mutation of SQLite records occurs.

---

## 4. Test Verification (Raw Execution Outputs)

All 5 required slots executed individually by name (`-functions`):

### (a) Slot: `testLegacyTimestampDisplayRoleShowsEmDash`
```
$ QT_QPA_PLATFORM=offscreen ./build/dev-linux/tests/models/tst_ApplicationTableModel testLegacyTimestampDisplayRoleShowsEmDash
********* Start testing of tst_ApplicationTableModel *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_ApplicationTableModel::initTestCase()
PASS   : tst_ApplicationTableModel::testLegacyTimestampDisplayRoleShowsEmDash()
PASS   : tst_ApplicationTableModel::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 7ms
********* Finished testing of tst_ApplicationTableModel *********
```

### (b) Slot: `testModernTimestampDisplayRoleShowsFormattedDate`
```
$ QT_QPA_PLATFORM=offscreen ./build/dev-linux/tests/models/tst_ApplicationTableModel testModernTimestampDisplayRoleShowsFormattedDate
********* Start testing of tst_ApplicationTableModel *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_ApplicationTableModel::initTestCase()
PASS   : tst_ApplicationTableModel::testModernTimestampDisplayRoleShowsFormattedDate()
PASS   : tst_ApplicationTableModel::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 7ms
********* Finished testing of tst_ApplicationTableModel *********
```

### (c) Slot: `testLegacyTimestampCutoffBoundary`
```
$ QT_QPA_PLATFORM=offscreen ./build/dev-linux/tests/models/tst_ApplicationTableModel testLegacyTimestampCutoffBoundary
********* Start testing of tst_ApplicationTableModel *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_ApplicationTableModel::initTestCase()
PASS   : tst_ApplicationTableModel::testLegacyTimestampCutoffBoundary()
PASS   : tst_ApplicationTableModel::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 8ms
********* Finished testing of tst_ApplicationTableModel *********
```

### (d) Slot: `testSortingPreservesChronologicalOrderWithLegacyAndModernRows`
```
$ QT_QPA_PLATFORM=offscreen ./build/dev-linux/tests/models/tst_ApplicationTableModel testSortingPreservesChronologicalOrderWithLegacyAndModernRows
********* Start testing of tst_ApplicationTableModel *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_ApplicationTableModel::initTestCase()
PASS   : tst_ApplicationTableModel::testSortingPreservesChronologicalOrderWithLegacyAndModernRows()
PASS   : tst_ApplicationTableModel::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 11ms
********* Finished testing of tst_ApplicationTableModel *********
```

### (e) Slot: `testBackupAndRestorePreservesLegacy1970Timestamp`
```
$ QT_QPA_PLATFORM=offscreen ./build/dev-linux/tests/services/tst_ImportExport testBackupAndRestorePreservesLegacy1970Timestamp
********* Start testing of tst_ImportExport *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_ImportExport::initTestCase()
PASS   : tst_ImportExport::testBackupAndRestorePreservesLegacy1970Timestamp()
PASS   : tst_ImportExport::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 9ms
********* Finished testing of tst_ImportExport *********
```

---

## 5. Local Gate (`scripts/check.sh`)

```
$ CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.2s)
-- Generating done (0.2s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
ninja: no work to do.
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start 29: tst_PageStudy
      Start 33: tst_EmptyStatesAndFocus
 1/34 Test #33: tst_EmptyStatesAndFocus ..........   Passed   10.61 sec
      Start 30: tst_PageHome
 2/34 Test #29: tst_PageStudy ....................   Passed   18.87 sec
      Start 34: tst_HiDpi
 3/34 Test #34: tst_HiDpi ........................   Passed    3.34 sec
      Start 27: tst_PageJobs
 4/34 Test #30: tst_PageHome .....................   Passed   17.09 sec
      Start 28: tst_PageCalendar
 5/34 Test #27: tst_PageJobs .....................   Passed   12.20 sec
      Start 26: tst_StudyRegression
 6/34 Test #28: tst_PageCalendar .................   Passed    7.11 sec
      Start 31: tst_PageSettings
 7/34 Test #31: tst_PageSettings .................   Passed    4.23 sec
      Start 32: tst_Shortcuts
 8/34 Test #26: tst_StudyRegression ..............   Passed    6.73 sec
      Start 25: tst_Notifiers
 9/34 Test #32: tst_Shortcuts ....................   Passed    2.96 sec
      Start 10: ui_smoke
10/34 Test #25: tst_Notifiers ....................   Passed    2.56 sec
      Start 21: tst_ImportExport
11/34 Test #21: tst_ImportExport .................   Passed    0.67 sec
      Start 24: tst_StatsService
12/34 Test #24: tst_StatsService .................   Passed    0.94 sec
      Start  8: tst_SeedService
13/34 Test  #8: tst_SeedService ..................   Passed    0.27 sec
      Start 22: tst_ReminderService
14/34 Test #22: tst_ReminderService ..............   Passed    0.08 sec
      Start 17: tst_TopicTableProxy
15/34 Test #17: tst_TopicTableProxy ..............   Passed    0.07 sec
      Start 11: tst_MonthView
16/34 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 18: tst_AgendaService
17/34 Test #18: tst_AgendaService ................   Passed    0.09 sec
      Start  3: tst_TopicRepository
18/34 Test  #3: tst_TopicRepository ..............   Passed    0.08 sec
      Start  5: tst_ApplicationRepository
19/34 Test  #5: tst_ApplicationRepository ........   Passed    0.11 sec
      Start 16: tst_TopicBoardProxy
20/34 Test #16: tst_TopicBoardProxy ..............   Passed    0.05 sec
      Start 15: tst_TopicListModel
21/34 Test #15: tst_TopicListModel ...............   Passed    0.04 sec
      Start 12: tst_AgendaListModel
22/34 Test #12: tst_AgendaListModel ..............   Passed    0.03 sec
      Start 13: tst_ApplicationTableModel
23/34 Test #13: tst_ApplicationTableModel ........   Passed    0.06 sec
      Start 23: tst_SettingsService
24/34 Test #23: tst_SettingsService ..............   Passed    0.02 sec
      Start  1: tst_Database
25/34 Test  #1: tst_Database .....................   Passed    0.03 sec
      Start  6: tst_InterviewRepository
26/34 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start  4: tst_SessionRepository
27/34 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start 19: tst_DayWatcher
28/34 Test #19: tst_DayWatcher ...................   Passed    0.01 sec
      Start  9: tst_TransactionBehavior
29/34 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start  2: tst_TrackRepository
30/34 Test  #2: tst_TrackRepository ..............   Passed    0.02 sec
      Start  7: tst_ReminderLogRepository
31/34 Test  #7: tst_ReminderLogRepository ........   Passed    0.06 sec
      Start 14: tst_ApplicationFilterProxy
32/34 Test #14: tst_ApplicationFilterProxy .......   Passed    0.02 sec
      Start 20: tst_ExportService
33/34 Test #20: tst_ExportService ................   Passed    0.01 sec
34/34 Test #10: ui_smoke .........................   Passed    5.59 sec

100% tests passed, 0 tests failed out of 34

Label Time Summary:
data       =   0.68 sec*proc (9 tests)
model      =   0.27 sec*proc (6 tests)
service    =   1.81 sec*proc (7 tests)
ui         =  91.39 sec*proc (12 tests)

Total Test time (real) =  47.61 sec
==> Quality check PASSED with zero warnings and all tests green!
```

### Totals by Command (Arithmetic from `-functions` per binary)

| Category | Binaries | Functions | Binary Breakdown (Functions) |
| :--- | :--- | :--- | :--- |
| **data** | 9 | 101 | `tst_ApplicationRepository` (20), `tst_Database` (16), `tst_InterviewRepository` (9), `tst_ReminderLogRepository` (7), `tst_SeedService` (5), `tst_SessionRepository` (10), `tst_TopicRepository` (18), `tst_TrackRepository` (8), `tst_TransactionBehavior` (8) |
| **model** | 6 | 39 | `tst_AgendaListModel` (5), `tst_ApplicationFilterProxy` (2), `tst_ApplicationTableModel` (10), `tst_TopicBoardProxy` (9), `tst_TopicListModel` (7), `tst_TopicTableProxy` (6) |
| **service** | 7 | 101 | `tst_AgendaService` (24), `tst_DayWatcher` (4), `tst_ExportService` (4), `tst_ImportExport` (33), `tst_ReminderService` (18), `tst_SettingsService` (8), `tst_StatsService` (10) |
| **ui** | 12 | 227 | `tst_MonthView` (13), `ui_smoke` (11), `tst_EmptyStatesAndFocus` (34), `tst_HiDpi` (4), `tst_Notifiers` (21), `tst_PageCalendar` (12), `tst_PageHome` (16), `tst_PageJobs` (22), `tst_PageSettings` (44), `tst_PageStudy` (26), `tst_Shortcuts` (12), `tst_StudyRegression` (12) |
| **Total** | **34** | **468** | **34 test binaries, 468 test slots, 100% passing** |

### Git Status Check
```
$ git status --short
(no output)
```

### Commits for this Task
```
$ git log --oneline -4
b7843cc docs(spec): document 2013 legacy timestamp display cutoff
8ebfe93 test(data): add tests for legacy timestamp display cutoff, boundary, sorting, and backup equality
1ce1fc6 feat(data): format legacy timestamps before 2013 as em dash in UI
5fb0f8a docs(reports): add M6 Part D2 fix-ups final report
```

---

## 6. Not Verified by Agent, User to Check

1. **User's real legacy database appearance in the UI:**
   The user should open their own existing database containing pre-fix rows with their own data (the agent never accesses or runs on user's personal data per `AGENTS.md` §11 and §12). Verify that previously stored `1970-01-01` dates in the Jobs table "Updated" column and Application history timeline render gracefully as `"—"`, without disrupting sorting or user-entered application/interview dates.
2. **Windows platform behavior:**
   Verify identical display and sorting behavior on Windows in the next CI matrix run.
