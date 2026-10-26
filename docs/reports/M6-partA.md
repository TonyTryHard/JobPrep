# Milestone 6 Part A Report: Test Split, CI, Windows Audit & Key Infrastructure

- **Date:** 2026-10-26
- **Status:** Complete (Part A only, awaiting user review before Part B)
- **Document Version:** 1.0

---

## 1. Environment Line

```
$ lsb_release -d && uname -r && g++ --version | head -1
Description:	Ubuntu 26.04.1 LTS
7.0.0-38-generic
g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0
```

```
$ ./build/dev-linux/tests/ui_smoke/ui_smoke testFormatHelper > /tmp/out.txt && grep "Config:" /tmp/out.txt
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
```

---

## 2. Pre-Flight Evidence (Plan Audit with Raw Outputs)

The M6 plan contained pre-flight items that lacked full raw output or contained ellipsis. Direct verification results:

### 1. CMake & Windows Flags
```
$ grep -n -C 5 "MSVC\|WIN32\|add_executable(jobprep" CMakeLists.txt
33-
34-# Quality gate: high warning level and zero warnings option
35-option(JOBPREP_WERROR "Treat compiler warnings as errors" ON)
36-
37-function(jobprep_apply_warnings target)
38:    if(MSVC)
39-        target_compile_options(${target} PRIVATE /W4 /permissive-)
40-        if(JOBPREP_WERROR)
41-            target_compile_options(${target} PRIVATE /WX)
42-        endif()
43-    else()
--
235-)
236-
237-jobprep_apply_warnings(jobprep_core)
238-
239-# --- Main executable target ---
240:add_executable(jobprep
241-    src/main.cpp
242-)
243-
244-target_link_libraries(jobprep PRIVATE
245-    jobprep_core
```

### 2. CI & Documentation Files
```
$ ls -la .github docs/RUNNING.md src/platform 2>&1
ls: cannot access 'docs/RUNNING.md': No such file or directory
.github:
total 12
drwxr-xr-x 3 tony tony 4096 Oct 26 16:49 .
drwxrwxr-x 9 tony tony 4096 Oct 26 16:49 ..
drwxr-xr-x 2 tony tony 4096 Oct 26 16:49 workflows

src/platform:
total 12
drwxr-xr-x 2 tony tony 4096 Oct  3 19:28 .
drwxr-xr-x 9 tony tony 4096 Oct 26 16:49 ..
-rw-r--r-- 1 tony tony  109 Oct  3 19:28 .gitkeep
```

### 3. Native Platform Guards
```
$ grep -rn "Q_OS_" src
(exit code 1, no matches)
```

### 4. CMake Presets
```
$ cat CMakePresets.json
{
  "version": 3,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 21,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "base",
      "hidden": true,
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
        "JOBPREP_WERROR": "ON",
        "CMAKE_PREFIX_PATH": "$penv{QT_PREFIX_PATH}"
      }
    },
    {
      "name": "dev-linux",
      "displayName": "Linux Development",
      "description": "Configure for Linux with Ninja and Qt 6",
      "inherits": "base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    },
    {
      "name": "dev-windows",
      "displayName": "Windows Development",
      "description": "Configure for Windows with Ninja/MSVC and Qt 6",
      "inherits": "base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "dev-linux",
      "displayName": "Build Linux Development",
      "configurePreset": "dev-linux"
    },
    {
      "name": "dev-windows",
      "displayName": "Build Windows Development",
      "configurePreset": "dev-windows"
    }
  ],
  "testPresets": [
    {
      "name": "dev-linux",
      "displayName": "Test Linux Development",
      "configurePreset": "dev-linux",
      "output": {
        "outputOnFailure": true
      },
      "execution": {
        "noTestsAction": "error"
      }
    },
    {
      "name": "dev-windows",
      "displayName": "Test Windows Development",
      "configurePreset": "dev-windows",
      "output": {
        "outputOnFailure": true
      },
      "execution": {
        "noTestsAction": "error"
      }
    }
  ]
}
```

### 5. Quality Check Scripts
```
$ cat scripts/check.ps1
# JobPrep Windows Quality Check Script
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
Set-Location $repoRoot

Write-Host "==> Configuring JobPrep with dev-windows preset..." -ForegroundColor Cyan
cmake --preset dev-windows
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Building JobPrep with dev-windows preset..." -ForegroundColor Cyan
cmake --build --preset dev-windows
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Running tests with dev-windows preset..." -ForegroundColor Cyan
ctest --preset dev-windows --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Quality check PASSED with zero warnings and all tests green!" -ForegroundColor Green
```

```
$ cat scripts/check.sh
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${REPO_ROOT}"

echo "==> Configuring JobPrep with dev-linux preset..."
cmake --preset dev-linux

echo "==> Building JobPrep with dev-linux preset..."
cmake --build --preset dev-linux

echo "==> Running tests with dev-linux preset..."
ctest --preset dev-linux --output-on-failure

echo "==> Quality check PASSED with zero warnings and all tests green!"
```

### 6. Action and Shortcut Bindings
```
$ grep -rn "QShortcut\|QAction\|QKeySequence" src/
src/ui/widgets/TopicBoardColumn.cpp:124:        QAction* action = moveMenu->addAction(Theme::StatusStyle::label(target));
src/ui/widgets/TopicBoardColumn.cpp:126:        connect(action, &QAction::triggered, this, [this, i]() {
src/ui/widgets/TopicBoardColumn.cpp:134:    QAction* deleteAction = m_contextMenu->addAction(tr("Delete topic"));
src/ui/widgets/TopicBoardColumn.cpp:137:    connect(deleteAction, &QAction::triggered, this, [this]() {
src/ui/widgets/TopicDetailPanel.cpp:19:#include <QShortcut>
src/ui/widgets/TopicDetailPanel.cpp:309:    auto* saveShortcut = new QShortcut(QKeySequence::Save, this);
src/ui/widgets/TopicDetailPanel.cpp:311:    connect(saveShortcut, &QShortcut::activated, this, [this]() { flush(); });
src/ui/MainWindow.h:101:    QAction* m_searchAction{nullptr};
src/ui/MainWindow.cpp:4:#include <QKeySequence>
src/ui/MainWindow.cpp:8:#include <QShortcut>
src/ui/MainWindow.cpp:194:        auto* shortcut = new QShortcut(QKeySequence(Qt::CTRL | (Qt::Key_1 + i)), this);
src/ui/MainWindow.cpp:195:        connect(shortcut, &QShortcut::activated, this, [this, i]() {
src/ui/MainWindow.cpp:201:    auto* prefsShortcut = new QShortcut(QKeySequence::Preferences, this);
src/ui/MainWindow.cpp:202:    connect(prefsShortcut, &QShortcut::activated, this, [this]() {
src/ui/MainWindow.cpp:208:    auto* ctrlCommaShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma), this);
src/ui/MainWindow.cpp:209:    connect(ctrlCommaShortcut, &QShortcut::activated, this, [this]() {
src/ui/MainWindow.cpp:215:    auto* findShortcut = new QShortcut(QKeySequence::Find, this);
src/ui/MainWindow.cpp:216:    connect(findShortcut, &QShortcut::activated, this, &MainWindow::focusCurrentPageSearch);
src/ui/MainWindow.cpp:218:    auto* newShortcut = new QShortcut(QKeySequence::New, this);
src/ui/MainWindow.cpp:219:    connect(newShortcut, &QShortcut::activated, this, &MainWindow::triggerCurrentPageNew);
src/ui/MainWindow.cpp:226:    QAction* application = menu->addAction(tr("Application"));
src/ui/MainWindow.cpp:228:    connect(application, &QAction::triggered, this, &MainWindow::newApplicationFromHeader);
src/ui/MainWindow.cpp:230:    QAction* interview = menu->addAction(tr("Interview"));
src/ui/MainWindow.cpp:232:    connect(interview, &QAction::triggered, this, &MainWindow::newInterviewFromHeader);
src/ui/MainWindow.cpp:234:    QAction* topic = menu->addAction(tr("Topic"));
src/ui/MainWindow.cpp:236:    connect(topic, &QAction::triggered, this, &MainWindow::newTopicFromHeader);
src/ui/MainWindow.cpp:238:    QAction* session = menu->addAction(tr("Study session"));
src/ui/MainWindow.cpp:242:    connect(session, &QAction::triggered, this, &MainWindow::newSessionFromHeader);
src/ui/TrayNotifier.cpp:32:    connect(showAction, &QAction::triggered, this, [this]() {
src/ui/TrayNotifier.cpp:37:    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
src/ui/pages/JobsPage.cpp:3:#include <QAction>
src/ui/pages/JobsPage.cpp:247:    QAction* addInterview = m_contextMenu->addAction(tr("Add interview..."));
src/ui/pages/JobsPage.cpp:249:    connect(addInterview, &QAction::triggered, this, [this]() {
src/ui/pages/JobsPage.cpp:253:    QAction* openUrl = m_contextMenu->addAction(tr("Open posting URL"));
src/ui/pages/JobsPage.cpp:255:    connect(openUrl, &QAction::triggered, this, [this]() {
src/ui/PageBase.h:20:    /// Moves focus into the page's own search box (QKeySequence::Find). Pages without a
src/ui/PageBase.h:24:    /// Opens the page's primary "add" flow (QKeySequence::New).
```

### 7. Data Folder & Database Path
```
$ grep -rn "QStandardPaths\|jobprep.sqlite\|databasePath" src/
src/app/AppContext.h:32:/// theme manager. `databasePath` defaults to the standard app-data folder; tests
src/app/AppContext.h:36:    explicit AppContext(const QString& databasePath = QString());
src/app/AppContext.cpp:4:#include <QStandardPaths>
src/app/AppContext.cpp:33:/// `QStandardPaths::AppDataLocation` + `/jobprep.sqlite` (SPEC §8, AGENTS §12).
src/app/AppContext.cpp:35:    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
src/app/AppContext.cpp:36:        .filePath(u"jobprep.sqlite"_s);
src/app/AppContext.cpp:41:AppContext::AppContext(const QString& databasePath) {
src/app/AppContext.cpp:44:    m_database->open(databasePath.isEmpty() ? defaultDatabasePath() : databasePath);
src/main.cpp:18:    // Organization and application name must be set early for QSettings and QStandardPaths
```

### 8. `reminders_sent` Bookkeeping
```
$ grep -rn "reminders_sent\|ReminderLogRepository" src/data/
src/data/Migrations.cpp:93:                      u"CREATE TABLE reminders_sent (key TEXT PRIMARY KEY, sent_at TEXT NOT "
src/data/ReminderLogRepository.cpp:1:#include "data/ReminderLogRepository.h"
src/data/ReminderLogRepository.cpp:13:ReminderLogRepository::ReminderLogRepository(Database& database, QObject* parent)
src/data/ReminderLogRepository.cpp:16:ReminderLogRepository::~ReminderLogRepository() = default;
src/data/ReminderLogRepository.cpp:18:bool ReminderLogRepository::wasSent(QStringView key) const {
src/data/ReminderLogRepository.cpp:19:    SqlStatement statement(m_database, u"SELECT 1 FROM reminders_sent WHERE key = ?"_s);
src/data/ReminderLogRepository.cpp:25:std::optional<QDateTime> ReminderLogRepository::sentAt(QStringView key) const {
src/data/ReminderLogRepository.cpp:26:    SqlStatement statement(m_database, u"SELECT sent_at FROM reminders_sent WHERE key = ?"_s);
src/data/ReminderLogRepository.cpp:33:bool ReminderLogRepository::markSent(const QString& key, const QDateTime& sentAt) {
src/data/ReminderLogRepository.cpp:46:                           u"INSERT OR IGNORE INTO reminders_sent (key, sent_at) VALUES (?, ?)"_s);
src/data/ReminderLogRepository.cpp:53:bool ReminderLogRepository::remove(const QString& key) {
src/data/ReminderLogRepository.cpp:57:    SqlStatement statement(m_database, u"DELETE FROM reminders_sent WHERE key = ?"_s);
src/data/ReminderLogRepository.cpp:67:QList<QString> ReminderLogRepository::sentKeys() const {
src/data/ReminderLogRepository.cpp:70:        m_database, u"SELECT key FROM reminders_sent ORDER BY sent_at, key"_s);
src/data/ReminderLogRepository.cpp:76:QString ReminderLogRepository::lastError() const {
src/data/ReminderLogRepository.cpp:80:Database& ReminderLogRepository::database() const {
src/data/ReminderLogRepository.h:13:/// Bookkeeping of already delivered reminders (SPEC §5 `reminders_sent`, §3.5 R2).
src/data/ReminderLogRepository.h:15:class ReminderLogRepository : public Repository {
src/data/ReminderLogRepository.h:18:    explicit ReminderLogRepository(Database& database, QObject* parent = nullptr);
src/data/ReminderLogRepository.h:19:    ~ReminderLogRepository() override;
```

---

## 3. Split Proof & Slot Parity

### 1. Old Sorted List vs. Union of New Binaries
The sorted union of test functions over the new binaries (`ui_smoke`, `tst_PageJobs`, `tst_PageCalendar`, `tst_PageStudy`, `tst_PageHome`) was verified against the baseline 80 functions taken from the pre-split binary:

```
$ diff -u /tmp/ui_smoke_baseline_functions.txt /tmp/ui_smoke_split_functions.txt
```
(Command returned exit code 0 with completely empty diff output).

```
$ wc -l /tmp/ui_smoke_baseline_functions.txt /tmp/ui_smoke_split_functions.txt
  80 /tmp/ui_smoke_baseline_functions.txt
  80 /tmp/ui_smoke_split_functions.txt
 160 total
```

### 2. Binary and Function Counts

**Baseline (Pre-Split):** 25 binaries, 324 functions.
```
tst_ApplicationRepository                18
tst_Database                             16
tst_InterviewRepository                  9
tst_ReminderLogRepository                7
tst_SeedService                          5
tst_SessionRepository                    10
tst_TopicRepository                      18
tst_TrackRepository                      8
tst_TransactionBehavior                  8
tst_AgendaListModel                      5
tst_ApplicationFilterProxy               2
tst_ApplicationTableModel                6
tst_TopicBoardProxy                      9
tst_TopicListModel                       7
tst_TopicTableProxy                      6
tst_AgendaService                        24
tst_DayWatcher                           4
tst_ExportService                        4
tst_ReminderService                      18
tst_SettingsService                      4
tst_StatsService                         10
tst_MonthView                            13
ui_smoke                                 80
tst_Notifiers                            21
tst_StudyRegression                      12
TOTAL: 324
```

**After Split + 2 New Key Shortcut Slots:** 29 binaries, 326 functions.
```
$ total=0
for t in $(find build/dev-linux/tests -maxdepth 2 -type f -executable | sort); do
    count=$("$t" -functions | wc -l)
    total=$((total + count))
    printf "%-40s %d\n" "$(basename "$t")" "$count"
done
echo "TOTAL: $total"

tst_ApplicationRepository                18
tst_Database                             16
tst_InterviewRepository                  9
tst_ReminderLogRepository                7
tst_SeedService                          5
tst_SessionRepository                    10
tst_TopicRepository                      18
tst_TrackRepository                      8
tst_TransactionBehavior                  8
tst_AgendaListModel                      5
tst_ApplicationFilterProxy               2
tst_ApplicationTableModel                6
tst_TopicBoardProxy                      9
tst_TopicListModel                       7
tst_TopicTableProxy                      6
tst_AgendaService                        24
tst_DayWatcher                           4
tst_ExportService                        4
tst_ReminderService                      18
tst_SettingsService                      4
tst_StatsService                         10
tst_MonthView                            13
ui_smoke                                 7
tst_Notifiers                            21
tst_PageCalendar                         12
tst_PageHome                             16
tst_PageJobs                             21
tst_PageStudy                            26
tst_StudyRegression                      12
TOTAL: 326
```

Arithmetic: 244 (unit/model/service/notifiers/regression) + 5 (smoke) + 21 (jobs) + 12 (calendar) + 26 (study) + 16 (home) + 2 (real key event shortcut tests) = 326.

---

## 4. Timings Before and After

### Parallel CTest (`ctest -j4`)
- **Baseline (Before Split):**
  `Total Test time (real) = 96.33 sec`
  `real 1m36.349s, user 1m44.470s, sys 0m0.790s` (limited by monolithic `ui_smoke` running for 96.33s).
- **After Split (`ctest --preset dev-linux -j4 --output-on-failure`):**
  `Total Test time (real) = 36.76 sec`
  `real 0m36.775s, user 1m48.331s, sys 0m0.940s`
  **Speedup:** 2.6x faster parallel execution.

### Sequential Gate (`scripts/check.sh`)
- **Baseline:**
  `Total Test time (real) = 110.98 sec`
  `real 1m53.131s, user 1m43.709s, sys 0m1.049s`
- **After Split:**
  `Total Test time (real) = 95.59 sec`
  `real 1m35.995s, user 1m33.946s, sys 0m0.970s`

---

## 5. Screenshot Script Verification

```
$ ./scripts/screenshots.sh
==> Building screenshot test binaries...
ninja: no work to do.
==> Running screenshot generation with JOBPREP_REPORT_IMG_DIR=/home/tony/projects/JobPrep/docs/reports/img...
********* Start testing of tst_PageCalendar *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_PageCalendar::initTestCase()
PASS   : tst_PageCalendar::testCalendarScreenshots()
PASS   : tst_PageCalendar::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 4118ms
********* Finished testing of tst_PageCalendar *********
********* Start testing of tst_PageStudy *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_PageStudy::initTestCase()
PASS   : tst_PageStudy::testStudyScreenshots()
PASS   : tst_PageStudy::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 15579ms
********* Finished testing of tst_PageStudy *********
********* Start testing of tst_PageHome *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_PageHome::initTestCase()
PASS   : tst_PageHome::testDashboardScreenshots()
PASS   : tst_PageHome::testToastScreenshots()
PASS   : tst_PageHome::cleanupTestCase()
Totals: 4 passed, 0 failed, 0 skipped, 0 blacklisted, 10953ms
********* Finished testing of tst_PageHome *********
==> Screenshots successfully regenerated in /home/tony/projects/JobPrep/docs/reports/img!
```

> [!NOTE]
> Screenshot PNG file byte hashes change slightly upon re-generation because current system dates ("today" markers in calendar and dashboard) and font anti-aliasing reflect the execution date and environment.

---

## 6. Windows Audit & Proxy Warning Resolution

### 1. Proxy Build Flags (`-Wshadow -Wconversion -Wsign-conversion`)
A clean audit build was executed on `jobprep_core` using extra flags to simulate MSVC `/W4 /WX` diagnostics (C4456/C4457/C4458 shadowing, C4244/C4267 narrowing, and C4389 sign mismatch).

| File | Warning Count | Issue | Resolution |
|---|---|---|---|
| `src/models/ApplicationTableModel.cpp` | 1 | `m_cache.size()` (`qsizetype`) -> `int` | `static_cast<int>` |
| `src/models/TopicBoardProxy.cpp` | 2 | `visibleIds.size()`, `indexOf` -> `int` | `static_cast<int>` |
| `src/ui/InAppNotifier.cpp` | 2 | `m_toasts.size()` -> `int` | `static_cast<int>` |
| `src/ui/delegates/StatusBadgeDelegate.cpp` | 1 | `0.15` double literal in `setAlphaF(float)` | `0.15f` |
| `src/ui/dialogs/ApplicationDialog.cpp` | 2 | `history.size()`, `interviews.size()` -> `int` | `static_cast<int>` |
| `src/ui/dialogs/TrackDialog.cpp` | 4 | `swatches.size()`, `visibleButtons.size()` -> `int` | `static_cast<int>` / `qsizetype` |
| `src/ui/pages/JobsPage.cpp` | 1 | `rows.size()` -> `int` | `static_cast<int>` |
| `src/ui/pages/StudyPage.cpp` | 2 | sign conversion indexing `kColumnNames`; `indexOf` -> `int` | `static_cast<size_t>`, `static_cast<int>` |
| `src/ui/widgets/MonthView.cpp` | 1 | `qMin(markers.kinds.size(), 3)` narrowing | `qMin<qsizetype>` + `static_cast<int>` |
| **Total Warnings Remaining** | **0** | All 16 sites resolved cleanly | |

### 2. Windows Traps Check
- `QIODevice::Text` / `QFile::Text`: Audited in `ExportService.cpp`. Files are opened without `QIODevice::Text` and write CRLF (`\r\n`) explicitly. `SeedService`, `ThemeManager`, and `IconProvider` read text resources safely with `ReadOnly | Text`.
- Hard-coded drive letters / separators: Clean; only `https://` placeholder URLs exist in text boxes.
- `NOMINMAX`: Added `add_compile_definitions(NOMINMAX)` under `if(WIN32)` in root `CMakeLists.txt`.
- Subsystem recommendation for Part D: Debug builds keep console subsystem (`add_executable(jobprep ...)` without `WIN32`) to preserve debug logging categories; Release builds switch to GUI subsystem (`WIN32`) with `Qt6::EntryPoint` linkage.

---

## 7. `QSignalBlocker` Re-Audit

| File | Target Object | Signal Class | Risk Verdict | Safety Justification & Test Coverage |
|---|---|---|---|---|
| `src/ui/widgets/TopicBoardColumn.cpp:183` | `m_list->selectionModel()` | Selection Model | Risky | Followed immediately by `m_list->viewport()->update()`. Covered by `testBoardSelectionPaintedOutlinesBothThemes` in `tst_StudyRegression`. |
| `src/ui/pages/StudyPage.cpp:712` | `activeColumn->list()->selectionModel()` | Selection Model | Risky | Synchronizes column current index; followed by `clearSelection()` across all peer columns. Added explicit `viewport()->update()`. Covered by `testStudyBoardSingleSelectionAcrossColumns`. |
| `src/ui/widgets/TopicDetailPanel.cpp` | Form edits (`QLineEdit`, `QComboBox`, `QDateEdit`) | Widget Controls | Safe | Setting programmatic form values during selection load to prevent triggering `textChanged`/`currentIndexChanged` autosave handlers. No view repaints depend on these signals. |
| `src/ui/dialogs/ApplicationDialog.cpp` | Form edits (`QLineEdit`, `QComboBox`, `QSpinBox`) | Widget Controls | Safe | Initializing dialog values on load before modal user interaction begins. |
| `src/ui/dialogs/InterviewDialog.cpp` | Form edits (`QLineEdit`, `QComboBox`, `QSpinBox`) | Widget Controls | Safe | Initializing dialog values on load before modal user interaction begins. |
| `src/ui/pages/JobsPage.cpp` | `m_searchEdit` | Widget Control | Safe | Clearing search filter without re-triggering proxy filter invalidation. |
| `src/ui/pages/CalendarPage.cpp` | `m_interviewsChip` | Widget Control | Safe | Restoring persisted chip state without re-triggering layer filter cascade. |

### Proposed Convention for `AGENTS.md`
> Never block signals of a model (`QAbstractItemModel`) or a selection model (`QItemSelectionModel`). Signal blockers must only target leaf control widgets (e.g. `QLineEdit`, `QComboBox`). If a selection model or item model operation must suppress re-entrant callbacks, use an explicit boolean re-entrancy flag (e.g. `m_isSyncing`) rather than blocking Qt internal model signals that view viewports rely on for painting and layout updates.

---

## 8. Real Key-Event Test Infrastructure

### Helper Design (`UiTestHelpers.h`, `UiTestHelpers.cpp`)
- `JobPrep::UiTest::activateWindow(QWidget* window)`: shows, raises, and waits for `window` to be active via `QTest::qWaitForWindowActive(window, 1000)`.
- `JobPrep::UiTest::sendKeyClick(QWidget* window, Qt::Key key, Qt::KeyboardModifiers modifiers)`: finds `window->focusWidget()` (falling back to `window`) and dispatches `QTest::keyClick(target, key, modifiers)`.
- `JobPrep::UiTest::sendKeySequence(QWidget* window, const QKeySequence& sequence)`: iterates decomposed key combinations and dispatches real key clicks.

### Verification Slots in `ui_smoke`
1. `testPageSwitchShortcutsRealKeyEvents`: Dispatches `Ctrl+2` (Jobs), `Ctrl+3` (Calendar), `Ctrl+4` (Study), `Ctrl+1` (Home). Successfully switches `m_window->pageStack()->currentIndex()` through real `QTest::keyClick` dispatches under `offscreen`.
2. `testPreferencesShortcutRealKeyEvents`: Dispatches `Ctrl+,` to switch to Settings page (index 4).
   - **Key Finding & Fix:** Under X11/FreeDesktop, Qt maps `QKeySequence::Preferences` to `Qt::Key_Settings` rather than `Ctrl+,`. `MainWindow::setupShortcuts()` was updated to bind `Ctrl+,` explicitly alongside `QKeySequence::Preferences`, fulfilling SPEC §3.7 across both Linux and Windows.

---

## 9. CI Workflow Summary (`.github/workflows/ci.yml`)

- **Triggers:** `push` and `pull_request` on `main`.
- **Permissions:** `contents: read`.
- **Concurrency:** Cancels superseded runs for the same branch ref.
- **Job Matrix:** `ubuntu-latest` and `windows-latest` with `timeout-minutes: 30`.
- **Qt Installation:** Uses pinned `aqtinstall==3.2.0` installing official Qt `6.8.2` LTS binaries with modules `qtsvg`.
- **Toolchains & Scripts:**
  - Linux runs `./scripts/check.sh` with Ninja, GCC, Mesa GL, fontconfig, DBus, and xkbcommon.
  - Windows runs `scripts/check.ps1` with MSVC environment via `ilammy/msvc-dev-cmd@cbc9c03b73288a1da9438908613a776c21e6495e` (pinned SHA).
- **Artifacts:** On failure, captures `Testing/Temporary/LastTest.log` and associated test logs.

### Expected Failure Points for CI (User Check)
1. GitHub runners may lack specific Wayland/X11 runtime libraries if Qt 6.8 offscreen plugin dynamically probes display servers on `ubuntu-latest`.
2. On `windows-latest`, `aqtinstall` network download or archive unpacking could exceed default timeouts if Qt mirrors experience rate limiting.
3. MSVC `/WX` on Windows may trigger new warnings in MSVC standard library headers if newer MSVC compilers introduce deprecation notices.

---

## 10. Architectural Layering Verification

```
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
Zero layering violations. Strict dependency order `ui -> models -> services -> data -> domain` verified.

---

## 11. SPEC Section 14 Entries

- **1.11 — `ui_smoke` split & test helpers:** Lean `ui_smoke` keeps global smoke tests (window creation and navigation, theme/accent switching, sidebar collapse, new entry points modal sanity, zero warnings, and real key event shortcut verification) while page-specific tests are split into dedicated test executables `tst_PageJobs` (21 slots), `tst_PageCalendar` (12 slots), `tst_PageStudy` (26 slots), and `tst_PageHome` (16 slots). Reusable UI test fixtures and helpers (`WindowFixture`, `JobsFixture`, `CalendarFixture`, `StudyFixture`, warning filter, modal handlers, widget finders, `isolateSettings` using `QTemporaryDir`) reside in static library `jobprep_ui_test_helpers`. CTest `LABELS` (`data`, `model`, `service`, `ui`) added across test suites.
- **1.11 — Real key-event dispatch infrastructure:** `JobPrep::UiTest::sendKeySequence` and `sendKeyClick` dispatch real `QTest::keyClick` events to the active window's focused widget under the offscreen QPA platform plugin with window activation validation (`QTest::qWaitForWindowActive`). `MainWindow` explicitly registers `Ctrl+,` as an alternative shortcut for Settings alongside `QKeySequence::Preferences` because under X11/Linux generic platform themes `Preferences` maps to `Qt::Key_Settings`.
- **1.11 — Selection model signal blocking audit & rule:** Re-audited `QSignalBlocker` on selection models in `TopicBoardColumn` and `StudyPage`. A view listening to a selection model does not repaint when signals are blocked unless manually forced; `TopicBoardColumn::clearSelection()` explicitly calls `viewport()->update()`, and `StudyPage::onSelectionChanged` explicitly triggers `viewport()->update()`. Documented rule: never block signals of a model or selection model; block widget's own signals only, or use a re-entrancy flag.
- **1.11 — Continuous integration workflow:** Added `.github/workflows/ci.yml` matrix workflow targeting `ubuntu-latest` and `windows-latest` with pinned `aqtinstall` (Qt 6.8.2 LTS), caching, headless offscreen dependencies, MSVC environment, concurrency groups, timeout guards, and artifact upload on failure.
- **1.11 — Windows compiler audit & NOMINMAX:** Defined `NOMINMAX` under `if(WIN32)` in `CMakeLists.txt`. Audited code with GCC `-Wshadow -Wconversion -Wsign-conversion` proxy flags to eliminate MSVC C4456/C4458/C4244/C4267/C4389 warnings in `src/`.

---

## 12. Shared Bootstrap Evaluation (Proposal Only — Not Implemented)

- **Concept:** Extract the dependency injection composition root from `src/main.cpp` into a static helper `JobPrep::App::Bootstrap::create(const QString& dbPath)` returning `std::unique_ptr<JobPrep::Ui::MainWindow>` and owning `AppContext`.
- **Benefits:** Eliminates minor composition boilerplate duplication in full-window test fixtures (`WindowFixture`, `tst_StudyRegression`, `tst_Notifiers`).
- **Risks:** Couples test fixtures to production application lifetime policies (such as first-launch sample data prompt dialogs, startup reminders, and global `QSettings` initialization); reduces fixture isolation flexibility where individual tests need custom mock notifiers or fake clocks.
- **Recommendation:** Keep composition root in `main.cpp` and lightweight test composition in `jobprep_ui_test_helpers`.

---

## 13. Git Log Hashes

```
$ git log -n 6 --oneline
4be3aff docs(spec): document M6 Part A in section 14
0b90cd1 ci: add GitHub Actions CI workflow for Linux and Windows
12a78ff refactor(test): split ui_smoke into page-specific suites with ctest labels and real key shortcuts
24928f0 feat(test): add jobprep_ui_test_helpers library with fixtures and key event helpers
b80ffca fix(build): resolve MSVC proxy conversion warnings and add NOMINMAX
0bd2500 docs(prompts): add M6 plan amendments
```

---

## 14. Not Verified by Agent, User to Check

1. **GitHub Actions Execution:** The workflow file `.github/workflows/ci.yml` is committed locally; pushing to GitHub and observing the Linux and Windows CI runs must be performed by the user.
2. **Native Windows MSVC Build:** GCC with `-Wshadow -Wconversion -Wsign-conversion` was used as a local proxy; native MSVC 2022 `/W4 /WX` compilation behavior on Windows must be confirmed via CI or local Windows build.
