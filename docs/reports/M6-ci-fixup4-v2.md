# Milestone 6 CI Fix-up 4 v2 Report

**Date:** November 3, 2026  
**OS / Platform:** Linux x86_64 (Ubuntu 26.04)  
**Compiler:** GCC 15.2.0 (`/usr/bin/c++`)  
**Qt Version:** Qt 6.10.2 (local dev) / Qt 6.8.2 LTS (CI target)  
**CMake Generator:** Ninja 1.12.1  

---

## 1. Facts from Full Windows Logs (Run 3 Diagnostics)

From the Windows run 3 diagnostics artifact (`diag/*.txt` under Qt 6.8.2, MSVC 2022, Windows Server 2025; Linux 33/33 green, 13 Windows tests failed):

- **A. Font directory (cause of at least 9 failures):**
  `QFontDatabase: Cannot find font directory D:/a/JobPrep/Qt/6.8.2/msvc2022_64/lib/fonts`.
  On Windows, `offscreen` QPA loads fonts from `<prefix>/lib/fonts` or `$env:QT_QPA_FONTDIR`. The test message guard turned that warning into `FAIL!` in `tst_UiSmoke::testZeroWarnings` and in `cleanupTestCase` or the first slot of `tst_PageJobs`, `tst_PageCalendar`, `tst_PageStudy`, `tst_PageHome`, `tst_PageSettings`, `tst_Shortcuts`, `tst_ImportExport`, `tst_Notifiers`, and `tst_StudyRegression`.
- **B. Qt resources unavailable in tests without `AppContext` (two tests):**
  - `tst_AgendaListModel::agendaKindStyle`: `'!icon.isNull()' returned FALSE` (empty icon returned by `IconProvider` when `:/icons/*.svg` cannot open).
  - `tst_ImportExport::testRestoreEqualitySeedDataset`: `'seed.loadSampleTopics(&seedError)' returned FALSE` (`:/seed/topics.json` cannot open).
  On MSVC, static library resource objects (`jobprep_core`) are not pulled into binaries that do not reference them directly unless `Q_INIT_RESOURCE` is invoked.
- **C. Two tests assumed Linux behavior:**
  - `tst_ImportExport::testRestoreSafetyBackupCreatedPrunedAndAbortsWhenReadOnly`: `QFile::setPermissions` on directories is ignored by Windows NTFS/Win32 APIs.
  - `tst_ImportExport::testRestoreSafetyBackupProductionPathWithCustomXdgDataHome`: `QStandardPaths` ignores `XDG_DATA_HOME` on Windows.
- **D. QSettings values are not read back (five cases, cause unknown):**
  - `tst_SettingsService::testTrayHintShownKey`: second `QSettings(path, IniFormat)` does not see written value.
  - `tst_PageSettings::testWeeklyGoalPersistence`: same pattern (`QTemporaryFile` + `IniFormat`), expected 540, got 360.
  - `tst_TopicListModel::settingsKeysAndFallbacks`: default registry `QSettings`: `setStudyViewMode("list")` then `studyViewMode()` in the same object returns `"board"`.
  - `tst_StudyRegression::testChecklistStartupContrastDarkAndLight`: `SettingsService settings; settings.setThemeMode(mode);` then `AppContext` reads `"system"`.
  - `tst_Notifiers::coordinatorEndToEndSettingsLeadMinutesChangeTakesEffect`: default `{1440, 60}` remains in effect after `setInterviewLeadMinutes({60})`.
- **E. Unexplained geometry/widget:**
  `tst_StudyRegression::testWidgetAtReturnsRealControlsNotOverlay`: `QApplication::widgetAt()` at the centre of `StudyDetailCheckAddBtn` returns `StudyDetailPanelCard`. May be caused by font fallback/size changes. Left untouched for re-evaluation once fonts are active.
- **F. Test duration on Windows:**
  UI tests take 60 to 114 s on Windows offscreen (vs 1 to 5 s on Linux), likely due to `UiTestHelpers::activateWindow` waiting 1000 ms per call. Left untouched per instruction.

---

## 2. Task 1 Evidence

### `git log --oneline -6`
```
$ git log --oneline -6
9af6489 docs(reports): add M6 Part D2 final report
19e865f docs(spec): document M6 Part D2 changes in SPEC §14
c5a1726 docs: add comprehensive RUNNING.md guide
fc45f4d feat(scripts): add desktop entry installer and HiDpi screenshot support
c95b5ba build(cmake): set Qt 6.8 minimum requirement and Windows GUI subsystem
1852168 feat(ui): add app icon, Windows resources, HiDpi 150% scaling, and layout fixes
```

### `git reflog -n 10`
```
$ git reflog -n 10
9af6489 (HEAD -> main) HEAD@{0}: commit: docs(reports): add M6 Part D2 final report
19e865f HEAD@{1}: commit: docs(spec): document M6 Part D2 changes in SPEC §14
c5a1726 HEAD@{2}: commit: docs: add comprehensive RUNNING.md guide
fc45f4d HEAD@{3}: commit: feat(scripts): add desktop entry installer and HiDpi screenshot support
c95b5ba HEAD@{4}: commit: build(cmake): set Qt 6.8 minimum requirement and Windows GUI subsystem
1852168 HEAD@{5}: commit: feat(ui): add app icon, Windows resources, HiDpi 150% scaling, and layout fixes
7f3ff74 HEAD@{6}: commit: fix(ui): adjust minimum window size to 1000x540 and add date/timestamp regression tests
6b366c5 (origin/main, origin/HEAD) HEAD@{7}: commit: add prompts m6: partd2 and ci-fixup3
87193dc HEAD@{8}: commit (amend): docs(reports): add M6 CI fix-up 3 report
949b6c7 HEAD@{9}: commit: docs(reports): add M6 CI fix-up 3 report
```
**Explanation of hash difference (`949b6c7` vs `87193dc`):**
In fix-up 3, commit `949b6c7` was amended locally (`commit (amend)`) to finalize the report, resulting in new commit hash `87193dc`. The report text originally cited `949b6c7` prior to the amend.

### `grep -n -A40 "Diagnose failed tests" .github/workflows/ci.yml` (Pre-task baseline)
```
$ grep -n -A40 "Diagnose failed tests" .github/workflows/ci.yml
157:      - name: Diagnose failed tests (Windows)
158-        if: failure() && runner.os == 'Windows'
159-        continue-on-error: true
160-        shell: pwsh
161-        run: |
162-          $logdir = "build/dev-windows/Testing/Temporary/diag"
163-          New-Item -ItemType Directory -Force -Path $logdir | Out-Null
164-          $logFile = "build/dev-windows/Testing/Temporary/LastTestsFailed.log"
165-          $testNames = @()
166-          if (Test-Path $logFile) {
167-            Get-Content $logFile | ForEach-Object {
168-              if ($_ -match '^\d+:(.+)$') {
169-                $testNames += $Matches[1].Trim()
170-              }
171-            }
172-          } else {
173-            Write-Host "LastTestsFailed.log missing"
174-            Get-ChildItem -Path "build/dev-windows/tests" -Recurse -Filter "*.exe" | ForEach-Object {
175-              if ($_.BaseName -like "tst_*" -or $_.BaseName -eq "ui_smoke") {
176-                $testNames += $_.BaseName
177-              }
178-            }
179-          }
180-          $env:QT_QPA_PLATFORM = "offscreen"
181-          foreach ($name in $testNames) {
182-            $exe = Get-ChildItem -Path "build/dev-windows" -Recurse -Filter "$name.exe" | Select-Object -First 1
183-            if (-not $exe) {
184-              Write-Host "Executable for $name not found"
185-              continue
186-            }
187-            $outFile = "$logdir/$name.txt"
188-            $proc = Start-Process -FilePath $exe.FullName -ArgumentList @("-v2", "-o", "$outFile,txt") -PassThru -NoNewWindow
189-            $finished = $proc.WaitForExit(150000)
190-            if (-not $finished) {
191-              $proc.Kill()
192-              $proc.WaitForExit()
193-              Write-Host "TIMEOUT"
194-            }
195-            $code = $proc.ExitCode
196-            Write-Host "=== TEST: $name ==="
197-            Write-Host ('{0} (0x{0:X8})' -f $code)
```

### Resource File Grep (`grep -rn '":/\|u":/\|QStringLiteral(":/' src | cut -c1-140`)
```
$ grep -rn '":/\|u":/\|QStringLiteral(":/' src | cut -c1-140
src/services/SeedService.cpp:37:    QFile file(u":/seed/topics.json"_s);
src/main.cpp:27:    appIcon.addPixmap(QIcon(u":/icons/app-color.svg"_s).pixmap(16, 16));
src/main.cpp:28:    appIcon.addPixmap(QIcon(u":/icons/app-color.svg"_s).pixmap(32, 32));
src/main.cpp:29:    appIcon.addFile(u":/icons/app-256.png"_s);
src/ui/theme/ThemeManager.cpp:88:    QFile file(u":/styles/theme.qss"_s);
src/ui/theme/IconProvider.cpp:21:    const QString path = u":/icons/"_s + name + u".svg"_s;
```

---

## 3. Workflow Commit (Commit 1: `e8f08ba`)

### Changes Made to `.github/workflows/ci.yml`
1. Added `QT_QPA_FONTDIR: C:\Windows\Fonts` to both "Run quality gate (Windows)" and "Diagnose failed tests (Windows)".
2. Diagnostic step v2:
   - Invokes tests without `-v2` (`-ArgumentList @("-o", "$outFile,txt")`).
   - Filters out all INFO lines (`$lines = @($rawLines | Where-Object { $_ -notmatch '^\s*(QINFO|INFO)\s*:' })`).
   - If `$lines.Count -le 300`, outputs all non-INFO lines.
   - If `$lines.Count > 300`, prints every line matching `FAIL!|XFAIL|QWARN|Totals:` and the 8 subsequent lines after each `FAIL!`.
3. Added new step "Registry write probe (Windows)" (`if: failure() && runner.os == 'Windows'`, `shell: pwsh`):
   - Prints `$env:USERNAME`, `whoami`, and `(Get-Item HKCU:\Software).Name`.
   - Creates `HKCU:\Software\JobPrepProbe`, writes property `TestValue = "Probe123"`, reads it back, deletes the key, and prints results/exceptions.

### Confirmation regarding `QT_QPA_FONTDIR`
In Qt QtGui source code (`qfontdatabase.cpp` and offscreen platform font loaders), Qt checks `$env{QT_QPA_FONTDIR}` when the kit font directory (`<prefix>/lib/fonts`) is absent. Setting `QT_QPA_FONTDIR: C:\Windows\Fonts` tells Qt's offscreen font database to populate system TrueType fonts directly from Windows' standard font directory.

---

## 4. Resource Initialization Independent of AppContext (Commit 2: `a1ffd14`)

Added `JobPrep::App::ensureResources()` in `src/app/Resources.cpp/.h` and registered it in `jobprep_core`:
```cpp
// src/app/Resources.cpp
#include "app/Resources.h"
#include <QtGlobal>

void initializeResourcesGlobal() {
    Q_INIT_RESOURCE(resources);
}

namespace JobPrep::App {

void ensureResources() {
    static const bool initialized = []() {
        initializeResourcesGlobal();
        return true;
    }();
    Q_UNUSED(initialized);
}

}  // namespace JobPrep::App
```
`JobPrep::App::ensureResources()` is now called from:
- `AppContext::AppContext()`
- `SeedService` (`readSeedFile` before opening `:/seed/topics.json`)
- `IconProvider::icon()` (before opening `:/icons/*.svg`)
- `ThemeManager::loadTheme()` (before opening `:/styles/theme.qss`)
- `main.cpp` (before registering window icon)

### Linux Test Verification for Commit 2

**`./build/dev-linux/tests/models/tst_AgendaListModel`:**
```
********* Start testing of tst_AgendaListModel *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_AgendaListModel::initTestCase()
PASS   : tst_AgendaListModel::emptyModel()
PASS   : tst_AgendaListModel::rolesForInterviewItem()
PASS   : tst_AgendaListModel::rolesForFollowUpAndTarget()
PASS   : tst_AgendaListModel::pastFlag()
PASS   : tst_AgendaListModel::agendaKindStyle()
PASS   : tst_AgendaListModel::cleanupTestCase()
Totals: 7 passed, 0 failed, 0 skipped, 0 blacklisted, 2ms
********* Finished testing of tst_AgendaListModel *********
```

**`./build/dev-linux/tests/data/tst_SeedService`:**
```
********* Start testing of tst_SeedService *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_SeedService::initTestCase()
PASS   : tst_SeedService::loadsTracksTopicsAndSubtasks()
PASS   : tst_SeedService::countsMatchSeedFile()
PASS   : tst_SeedService::secondRunIsIdempotent()
PASS   : tst_SeedService::existingTopicIsNotDuplicated()
QWARN  : tst_SeedService::invalidDatabaseFailsWithoutPartialData() jobprep.data: "Cannot start a transaction on a closed database."
PASS   : tst_SeedService::invalidDatabaseFailsWithoutPartialData()
PASS   : tst_SeedService::cleanupTestCase()
Totals: 7 passed, 0 failed, 0 skipped, 0 blacklisted, 263ms
********* Finished testing of tst_SeedService *********
```

**`./build/dev-linux/tests/services/tst_ImportExport`:**
```
********* Start testing of tst_ImportExport *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_ImportExport::initTestCase()
...
PASS   : tst_ImportExport::testRestoreEqualitySeedDataset()
...
Totals: 34 passed, 0 failed, 0 skipped, 0 blacklisted, 717ms
********* Finished testing of tst_ImportExport *********
```

---

## 5. Portable Tests (Commit 3: `c2bfab1`)

### Diff in `tests/services/tst_ImportExport.cpp`
```diff
--- a/tests/services/tst_ImportExport.cpp
+++ b/tests/services/tst_ImportExport.cpp
@@ -920,11 +920,14 @@ void tst_ImportExport::testRestoreSafetyBackupCreatedPrunedAndAbortsWhenReadOnly
     QCOMPARE(preRestores.size(), 5);
     QVERIFY(unrelated.exists());
 
-    // 3. Read-only safety backup dir aborts restore and leaves database untouched
+    // 3. Unwritable safety backup dir (parent is a regular file) aborts restore and leaves database untouched
     const auto dumpBefore = dumpDatabase(testDb.db());
-    const QString roDir = tempDir.filePath(u"ro_folder"_s);
-    QVERIFY(d.mkdir(u"ro_folder"_s));
-    QFile::setPermissions(roDir, QFileDevice::ReadOwner | QFileDevice::ReadUser | QFileDevice::ExeOwner | QFileDevice::ExeUser);
+    const QString blockingFilePath = tempDir.filePath(u"blocking_file.txt"_s);
+    QFile blockingFile(blockingFilePath);
+    QVERIFY(blockingFile.open(QIODevice::WriteOnly));
+    blockingFile.write("blocking");
+    blockingFile.close();
+    const QString roDir = blockingFilePath + u"/safety_subfolder"_s;
 
     const auto roRes = RestoreService::restoreDatabase(testDb.db(), validJson, roDir);
     QVERIFY(!roRes.success);
@@ -933,8 +936,6 @@ void tst_ImportExport::testRestoreSafetyBackupCreatedPrunedAndAbortsWhenReadOnly
     const auto dumpAfter = dumpDatabase(testDb.db());
     QString diff;
     QVERIFY2(compareDumps(dumpBefore, dumpAfter, &diff), qPrintable(diff));
-
-    QFile::setPermissions(roDir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
 }
 
 void tst_ImportExport::testRestoreSafetyBackupProductionPathWithCustomXdgDataHome() {
@@ -961,9 +962,13 @@ void tst_ImportExport::testRestoreSafetyBackupProductionPathWithCustomXdgDataHom
     QVERIFY(res.success);
     QVERIFY(!res.safetyBackupPath.isEmpty());
     QVERIFY(QFile::exists(res.safetyBackupPath));
-    QVERIFY(res.safetyBackupPath.startsWith(customXdg));
+    QVERIFY(res.safetyBackupPath.startsWith(prodSafetyDir));
+    if (QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation).startsWith(customXdg)) {
+        QVERIFY(res.safetyBackupPath.startsWith(customXdg));
+    }
 
     qunsetenv("XDG_DATA_HOME");
+    QDir(customXdg).removeRecursively();
     QStandardPaths::setTestModeEnabled(true);
     QCoreApplication::setOrganizationName(origOrg);
     QCoreApplication::setApplicationName(origApp);
```

### Verification of Both Slots on Linux

**`./build/dev-linux/tests/services/tst_ImportExport testRestoreSafetyBackupCreatedPrunedAndAbortsWhenReadOnly`:**
```
********* Start testing of tst_ImportExport *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_ImportExport::initTestCase()
PASS   : tst_ImportExport::testRestoreSafetyBackupCreatedPrunedAndAbortsWhenReadOnly()
PASS   : tst_ImportExport::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 8ms
********* Finished testing of tst_ImportExport *********
```

**`./build/dev-linux/tests/services/tst_ImportExport testRestoreSafetyBackupProductionPathWithCustomXdgDataHome`:**
```
********* Start testing of tst_ImportExport *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_ImportExport::initTestCase()
PASS   : tst_ImportExport::testRestoreSafetyBackupProductionPathWithCustomXdgDataHome()
PASS   : tst_ImportExport::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 6ms
********* Finished testing of tst_ImportExport *********
```

---

## 6. QSettings Diagnostics (Commit 4: `23c7477`)

### Unexpected Warnings Proof
In `tests/ui_test_helpers/UiTestHelpers.cpp:21`:
```cpp
void testMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    Q_UNUSED(context);
    if (type != QtWarningMsg && type != QtCriticalMsg && type != QtFatalMsg) {
        if (s_previousHandler) {
            s_previousHandler(type, context, msg);
        } else {
            fprintf(stderr, "QINFO  : %s\n", qPrintable(msg));
        }
        return;
    }
    ...
    s_capturedWarnings.append(msg);
}
```
`s_capturedWarnings` only captures `QtWarningMsg`, `QtCriticalMsg`, and `QtFatalMsg`. `qInfo()` (`QtInfoMsg`) is never appended to `s_capturedWarnings` and therefore cannot trigger `testZeroWarnings` failure.

### Diagnostic Output on Linux ("Good" Reference)

**`./build/dev-linux/tests/services/tst_SettingsService testTrayHintShownKey`:**
```
********* Start testing of tst_SettingsService *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_SettingsService::initTestCase()
QINFO  : tst_SettingsService::testTrayHintShownKey() CI-DIAG testTrayHintShownKey file exists: true size: 25
QINFO  : tst_SettingsService::testTrayHintShownKey() CI-DIAG testTrayHintShownKey L 1 : [app]
QINFO  : tst_SettingsService::testTrayHintShownKey() CI-DIAG testTrayHintShownKey L 2 : trayHintShown=true
QINFO  : tst_SettingsService::testTrayHintShownKey() CI-DIAG testTrayHintShownKey org:  app: tst_SettingsService arg0: ./build/dev-linux/tests/services/tst_SettingsService fileName: /tmp/tst_SettingsService.NwxfFf format: 1 status: 0 writable: true value: QVariant(bool, true)
PASS   : tst_SettingsService::testTrayHintShownKey()
PASS   : tst_SettingsService::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 2ms
********* Finished testing of tst_SettingsService *********
```

**`./build/dev-linux/tests/models/tst_TopicListModel settingsKeysAndFallbacks`:**
```
********* Start testing of tst_TopicListModel *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_TopicListModel::initTestCase()
QINFO  : tst_TopicListModel::settingsKeysAndFallbacks() CI-DIAG TopicListModel start nativeSettings org:  app: tst_TopicListModel arg0: ./build/dev-linux/tests/models/tst_TopicListModel fileName: /home/tony/.config/Unknown Organization/tst_TopicListModel.conf format: 0 status: 1 writable: true
QINFO  : tst_TopicListModel::settingsKeysAndFallbacks() CI-DIAG TopicListModel start probe status: 0 val: QVariant(int, 1)
QINFO  : tst_TopicListModel::settingsKeysAndFallbacks() CI-DIAG TopicListModel before line 319 org:  app: tst_TopicListModel arg0: ./build/dev-linux/tests/models/tst_TopicListModel fileName: /home/tony/.config/Unknown Organization/tst_TopicListModel.conf format: 0 status: 1 writable: true value: QVariant(QString, list)
PASS   : tst_TopicListModel::settingsKeysAndFallbacks()
PASS   : tst_TopicListModel::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 272ms
********* Finished testing of tst_TopicListModel *********
```

---

## 7. Local Quality Gate Results

Command executed:
```bash
CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
```

Raw Output:
```
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.2s)
-- Generating done (0.2s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
[1/7] Linking CXX executable tests/ui/tst_PageJobs
[2/7] Linking CXX executable tests/ui/tst_PageCalendar
[3/7] Linking CXX executable tests/ui/tst_PageStudy
[4/7] Linking CXX executable tests/ui/tst_PageHome
[5/7] Linking CXX executable tests/ui/tst_Shortcuts
[6/7] Linking CXX executable tests/ui/tst_EmptyStatesAndFocus
[7/7] Linking CXX executable tests/ui/tst_HiDpi
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start 34: tst_HiDpi
      Start 29: tst_PageStudy
 1/34 Test #34: tst_HiDpi ........................   Passed    3.28 sec
      Start 33: tst_EmptyStatesAndFocus
 2/34 Test #33: tst_EmptyStatesAndFocus ..........   Passed   10.79 sec
      Start 30: tst_PageHome
 3/34 Test #29: tst_PageStudy ....................   Passed   19.46 sec
      Start 27: tst_PageJobs
 4/34 Test #30: tst_PageHome .....................   Passed   17.36 sec
      Start 28: tst_PageCalendar
 5/34 Test #27: tst_PageJobs .....................   Passed   12.43 sec
      Start 26: tst_StudyRegression
 6/34 Test #28: tst_PageCalendar .................   Passed    7.44 sec
      Start 32: tst_Shortcuts
 7/34 Test #26: tst_StudyRegression ..............   Passed    7.71 sec
      Start 31: tst_PageSettings
 8/34 Test #32: tst_Shortcuts ....................   Passed    3.16 sec
      Start 25: tst_Notifiers
 9/34 Test #31: tst_PageSettings .................   Passed    4.42 sec
      Start 10: ui_smoke
10/34 Test #25: tst_Notifiers ....................   Passed    2.75 sec
      Start 21: tst_ImportExport
11/34 Test #21: tst_ImportExport .................   Passed    0.67 sec
      Start 24: tst_StatsService
12/34 Test #24: tst_StatsService .................   Passed    0.98 sec
      Start  8: tst_SeedService
13/34 Test  #8: tst_SeedService ..................   Passed    0.29 sec
      Start 22: tst_ReminderService
14/34 Test #22: tst_ReminderService ..............   Passed    0.08 sec
      Start 15: tst_TopicListModel
15/34 Test #15: tst_TopicListModel ...............   Passed    0.30 sec
      Start 17: tst_TopicTableProxy
16/34 Test #17: tst_TopicTableProxy ..............   Passed    0.07 sec
      Start 18: tst_AgendaService
17/34 Test #18: tst_AgendaService ................   Passed    0.09 sec
      Start  3: tst_TopicRepository
18/34 Test  #3: tst_TopicRepository ..............   Passed    0.09 sec
      Start  5: tst_ApplicationRepository
19/34 Test  #5: tst_ApplicationRepository ........   Passed    0.09 sec
      Start 16: tst_TopicBoardProxy
20/34 Test #16: tst_TopicBoardProxy ..............   Passed    0.05 sec
      Start 11: tst_MonthView
21/34 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 13: tst_ApplicationTableModel
22/34 Test #13: tst_ApplicationTableModel ........   Passed    0.04 sec
      Start 12: tst_AgendaListModel
23/34 Test #12: tst_AgendaListModel ..............   Passed    0.03 sec
      Start  6: tst_InterviewRepository
24/34 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start  9: tst_TransactionBehavior
25/34 Test  #9: tst_TransactionBehavior ..........   Passed    0.04 sec
      Start  7: tst_ReminderLogRepository
26/34 Test  #7: tst_ReminderLogRepository ........   Passed    0.02 sec
      Start 23: tst_SettingsService
27/34 Test #23: tst_SettingsService ..............   Passed    0.02 sec
      Start  1: tst_Database
28/34 Test  #1: tst_Database .....................   Passed    0.04 sec
      Start 19: tst_DayWatcher
29/34 Test #19: tst_DayWatcher ...................   Passed    0.01 sec
      Start  4: tst_SessionRepository
30/34 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  2: tst_TrackRepository
31/34 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start 14: tst_ApplicationFilterProxy
32/34 Test #14: tst_ApplicationFilterProxy .......   Passed    0.02 sec
      Start 20: tst_ExportService
33/34 Test #20: tst_ExportService ................   Passed    0.01 sec
34/34 Test #10: ui_smoke .........................   Passed    5.73 sec

100% tests passed, 0 tests failed out of 34

Label Time Summary:
data       =   0.65 sec*proc (9 tests)
model      =   0.53 sec*proc (6 tests)
service    =   1.86 sec*proc (7 tests)
ui         =  94.63 sec*proc (12 tests)

Total Test time (real) =  49.78 sec
==> Quality check PASSED with zero warnings and all tests green!
```

### Slot Count Reconciliation
- Prior to Part D2, the repository had 33 binaries and 454 pure test slots.
- Part D2 added `tst_HiDpi` (+1 binary, 4 slots) and several regression tests across existing suites, bringing the totals to 34 binaries and 463 test slots.

### `git status --short`
```
$ git status --short
(no output)
```

---

## 8. Static Checks

- **YAML syntax validation:** Passed via Python `yaml.safe_load`.
- **Static analysis tools (`command -v pwsh actionlint`):** `pwsh` and `actionlint` are missing on this Linux dev machine ("not run").

### Git Diff Statistics per Commit

**Commit 1: `e8f08ba` (`ci: add font directory, diagnostic v2, and registry probe on Windows`)**
```
 .github/workflows/ci.yml | 59 ++++++++++++++++++++++++++++++++++++++++++------
 1 file changed, 52 insertions(+), 7 deletions(-)
```

**Commit 2: `a1ffd14` (`fix(core): make resource initialization independent of AppContext`)**
```
 CMakeLists.txt                |  2 ++
 src/app/AppContext.cpp        | 10 ++--------
 src/app/Resources.cpp         | 21 +++++++++++++++++++++
 src/app/Resources.h           | 10 ++++++++++
 src/main.cpp                  |  2 ++
 src/services/SeedService.cpp  |  2 ++
 src/ui/theme/IconProvider.cpp |  2 ++
 src/ui/theme/ThemeManager.cpp |  2 ++
 8 files changed, 43 insertions(+), 8 deletions(-)
```

**Commit 3: `c2bfab1` (`fix(tests): make safety backup restore and XDG tests portable across OSes`)**
```
 tests/services/tst_ImportExport.cpp | 19 ++++++++++++-------
 1 file changed, 12 insertions(+), 7 deletions(-)
```

**Commit 4: `23c7477` (`test(ci): add CI-DIAG diagnostics for QSettings and test message handler`)**
```
 tests/models/tst_TopicListModel.cpp     | 32 ++++++++++++++++++++++++++++++++
 tests/services/tst_SettingsService.cpp  | 29 +++++++++++++++++++++++++++++
 tests/ui/tst_Notifiers.cpp              | 16 ++++++++++++++++
 tests/ui/tst_PageSettings.cpp           | 27 +++++++++++++++++++++++++++
 tests/ui/tst_StudyRegression.cpp        | 16 ++++++++++++++++
 tests/ui_test_helpers/UiTestHelpers.cpp |  9 ++++++++-
 6 files changed, 128 insertions(+), 1 deletion(-)
```

---

## 9. Predictions for the Next Windows CI Run

Out of the 13 failing Windows tests from Run 3:

| Test Executable | Expected Status | Reason for Prediction |
| :--- | :---: | :--- |
| `ui_smoke` | **GREEN** | Font warning eliminated via `QT_QPA_FONTDIR`. |
| `tst_PageJobs` | **GREEN** | Font warning eliminated via `QT_QPA_FONTDIR`. |
| `tst_PageCalendar` | **GREEN** | Font warning eliminated via `QT_QPA_FONTDIR`. |
| `tst_PageStudy` | **GREEN** | Font warning eliminated via `QT_QPA_FONTDIR`. |
| `tst_PageHome` | **GREEN** | Font warning eliminated via `QT_QPA_FONTDIR`. |
| `tst_Shortcuts` | **GREEN** | Font warning eliminated via `QT_QPA_FONTDIR`. |
| `tst_AgendaListModel` | **GREEN** | Resource icons available via `ensureResources()`. |
| `tst_ImportExport` | **GREEN** | Font warning gone, seed resource available, read-only and XDG assertions now OS-portable. |
| `tst_SettingsService` | **RED** | Fails on QSettings read-back (D1); diagnostics active. |
| `tst_PageSettings` | **RED** | Font warning gone, but weekly goal persistence fails on QSettings read-back (D2); diagnostics active. |
| `tst_TopicListModel` | **RED** | Fails on QSettings native format read-back (D3); diagnostics active. |
| `tst_StudyRegression` | **RED** | Font warning gone, but fails on QSettings theme read-back (D4) and `widgetAt` (E); diagnostics active. |
| `tst_Notifiers` | **RED** | Font warning gone, but fails on QSettings lead minutes read-back (D5); diagnostics active. |

---

## 10. Not Verified by Agent, User to Check

- That `QT_QPA_FONTDIR: C:\Windows\Fonts` removes the font warning on Windows Server 2025 (verified on next CI run).
- That `ensureResources()` fixes the two resource failures on MSVC (verified on next CI run).
- That the portable tests pass on Windows (verified on next CI run).
- That the PowerShell changes in `.github/workflows/ci.yml` execute cleanly on Windows runner.
- **The root cause of the five QSettings failures (D) and of `widgetAt` (E) is unknown until the diagnostic lines and the registry probe are read from the next Windows CI run.**

---

## 11. Git Commit Log of this Task

```
* 23c7477 test(ci): add CI-DIAG diagnostics for QSettings and test message handler
* c2bfab1 fix(tests): make safety backup restore and XDG tests portable across OSes
* a1ffd14 fix(core): make resource initialization independent of AppContext
* e8f08ba ci: add font directory, diagnostic v2, and registry probe on Windows
```
