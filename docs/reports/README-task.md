# Task Report: GitHub README & Repository Hygiene

**Date:** 2026-11-24  
**Author:** Agent  
**Repository:** `TonyTryHard/JobPrep`  
**Host Environment:** Ubuntu 26.04 x86_64, GCC 15.2.0, Qt 6.10.2, CMake 4.2.3, Ninja 1.13.2  
**RAM Constraint:** 8 GB (builds executed with `CMAKE_BUILD_PARALLEL_LEVEL=2` / `-j2`)

---

## 1. Facts First

### 1.1 App Features & Claim-to-Source Mapping

Every statement and feature claim in `README.md` is derived directly from the application code and documentation:

| README Feature Claim | Source File(s) / Code Evidence | Documentation Reference |
| :--- | :--- | :--- |
| **Preparation Tracks & Topics** (C++, English, customizable topics, priority, target dates, resources, checklists) | `src/domain/Structs.h`, `src/data/TrackRepository.cpp`, `src/data/TopicRepository.cpp`, `src/ui/pages/StudyPage.cpp` | `docs/SPEC.md §3.2`, `§4` |
| **Kanban Board & List Views** (*Backlog*, *In Progress*, *Review*, *Done* columns, sortable list view) | `src/models/TopicBoardProxy.cpp`, `src/models/TopicTableProxy.cpp`, `src/ui/widgets/TopicBoardColumn.cpp`, `src/ui/widgets/TopicListView.cpp` | `docs/SPEC.md §3.2` |
| **Study Sessions & Weekly Goal** (session logging with minute duration, adjustable weekly goal default 6h) | `src/data/SessionRepository.cpp`, `src/services/StatsService.cpp`, `src/ui/dialogs/SessionDialog.cpp`, `src/ui/pages/SettingsPage.cpp` | `docs/SPEC.md §3.1`, `§3.2`, `§3.5` |
| **Application Pipeline** (10 statuses: wishlist through ghosted, salary range, work mode, follow-up dates) | `src/domain/Enums.h`, `src/data/ApplicationRepository.cpp`, `src/ui/pages/JobsPage.cpp`, `src/ui/dialogs/ApplicationDialog.cpp` | `docs/SPEC.md §3.3`, `§4` |
| **Interview Management** (rounds, dates, durations, interviewer, outcome, notes) | `src/data/InterviewRepository.cpp`, `src/ui/dialogs/InterviewDialog.cpp`, `src/ui/pages/CalendarPage.cpp` | `docs/SPEC.md §3.3`, `§3.4` |
| **Unified Calendar & Agenda** (month grid with per-kind colored markers, daily agenda list) | `src/services/AgendaService.cpp`, `src/ui/widgets/MonthView.cpp`, `src/ui/pages/CalendarPage.cpp` | `docs/SPEC.md §3.4` |
| **Reminders & System Tray Notifications** (lead times, tray notification, in-app toast fallback when tray absent) | `src/services/ReminderService.cpp`, `src/ui/InAppNotifier.cpp`, `src/ui/Toast.cpp`, `src/ui/MainWindow.cpp` | `docs/SPEC.md §3.5`, `§8` |
| **Data Portability & Safety** (JSON backup/restore with pre-restore auto backups, CSV application export, ICS calendar export) | `src/services/ExportService.cpp`, `src/services/RestoreService.cpp`, `src/ui/DataToolsController.cpp` | `docs/SPEC.md §3.5`, `§7` |
| **Starter Sample Data** (modern C++ and English interview topics with checklists) | `resources/seed/topics.json`, `src/services/SeedService.cpp` | `docs/SPEC.md §3.5`, `§3.7` |
| **Localization** (English, Ukrainian `uk`, Russian `ru`, Slavic plurals `%n`, system default auto-detection) | `src/app/Localization.cpp`, `resources/translations/jobprep_uk.ts`, `resources/translations/jobprep_ru.ts` | `docs/reports/M7.md`, `docs/SPEC.md §12 (M7)` |
| **Themes & Appearance** (Light/Dark mode, 6 accent presets: Indigo, Teal, Rose, Amber, Emerald, Sky) | `src/ui/theme/ThemeManager.cpp`, `src/ui/theme/Tokens.h` | `docs/SPEC.md §3.5`, `§6` |
| **Keyboard Accessibility** (Ctrl+1..4, Ctrl+,, Ctrl+N, Ctrl+F, Ctrl+S, Esc, Alt+Down/F4) | `src/ui/MainWindow.cpp`, `tests/ui/tst_Shortcuts.cpp` | `docs/SPEC.md §3.7`, `§12 (M7)` |

---

### 1.2 Technical Facts

#### Qt Version Check
```bash
$ grep -n "find_package(Qt6" CMakeLists.txt
25:find_package(Qt6 6.8 REQUIRED COMPONENTS
```

#### CMake Presets
```bash
$ cmake --list-presets
Available configure presets:

  "dev-linux"   - Linux Development
  "dev-windows" - Windows Development
```

#### CTest Test Count & Suites by Area
```bash
$ (cd build/dev-linux && ctest -N)
Test project /home/tony/projects/JobPrep/build/dev-linux
  Test  #1: tst_Database
  Test  #2: tst_TrackRepository
  Test  #3: tst_TopicRepository
  Test  #4: tst_SessionRepository
  Test  #5: tst_ApplicationRepository
  Test  #6: tst_InterviewRepository
  Test  #7: tst_ReminderLogRepository
  Test  #8: tst_SeedService
  Test  #9: tst_TransactionBehavior
  Test #10: ui_smoke
  Test #11: tst_MonthView
  Test #12: tst_AgendaListModel
  Test #13: tst_ApplicationTableModel
  Test #14: tst_ApplicationFilterProxy
  Test #15: tst_TopicListModel
  Test #16: tst_TopicBoardProxy
  Test #17: tst_TopicTableProxy
  Test #18: tst_AgendaService
  Test #19: tst_DayWatcher
  Test #20: tst_ExportService
  Test #21: tst_ImportExport
  Test #22: tst_ReminderService
  Test #23: tst_SettingsService
  Test #24: tst_StatsService
  Test #25: tst_Notifiers
  Test #26: tst_StudyRegression
  Test #27: tst_PageJobs
  Test #28: tst_PageCalendar
  Test #29: tst_PageStudy
  Test #30: tst_PageHome
  Test #31: tst_PageSettings
  Test #32: tst_Shortcuts
  Test #33: tst_EmptyStatesAndFocus
  Test #34: tst_HiDpi
  Test #35: tst_Translations

Total Tests: 35
```

**Test Suites by Area (35 total, verified by CTest labels):**
```bash
$ for lbl in data model service ui i18n; do echo -n "$lbl: "; (cd build/dev-linux && ctest -L "^$lbl$" -N) | grep -c "Test #"; done
data: 9
model: 6
service: 7
ui: 12
i18n: 1
```

- **Data Layer (9):** `tst_Database`, `tst_TrackRepository`, `tst_TopicRepository`, `tst_SessionRepository`, `tst_ApplicationRepository`, `tst_InterviewRepository`, `tst_ReminderLogRepository`, `tst_SeedService`, `tst_TransactionBehavior`
- **Models (6):** `tst_AgendaListModel`, `tst_ApplicationTableModel`, `tst_ApplicationFilterProxy`, `tst_TopicListModel`, `tst_TopicBoardProxy`, `tst_TopicTableProxy`
- **Services (7):** `tst_AgendaService`, `tst_DayWatcher`, `tst_ExportService`, `tst_ImportExport`, `tst_ReminderService`, `tst_SettingsService`, `tst_StatsService`
- **UI & Presentation (12):** `ui_smoke`, `tst_MonthView`, `tst_Notifiers`, `tst_StudyRegression`, `tst_PageJobs`, `tst_PageCalendar`, `tst_PageStudy`, `tst_PageHome`, `tst_PageSettings`, `tst_Shortcuts`, `tst_EmptyStatesAndFocus`, `tst_HiDpi`
- **Internationalization (1):** `tst_Translations`

#### Compilers & Platforms in CI (`.github/workflows/ci.yml`)
- `Gate (ubuntu-latest, Qt 6.8.2)`: GCC 11+ on Linux x86_64
- `Gate (ubuntu-latest, Qt 6.10.3)`: GCC on Linux x86_64
- `Gate (windows-latest, Qt 6.10.3)`: MSVC 2022 (cl.exe v143) on Windows x64
- `Gate (windows-latest, Qt 6.8.2 compat)`: Scheduled weekly check for Qt 6.8 LTS on Windows

---

### 1.3 Where Data Lives

From `src/main.cpp`:
```bash
$ grep -n -E "setOrganizationName|setApplicationName" src/main.cpp
23:    QCoreApplication::setOrganizationName(u"JobPrep"_s);
25:    QCoreApplication::setApplicationName(u"JobPrep"_s);
```
Standard data location resolution (`QStandardPaths::AppDataLocation`):
- **Linux:** `${XDG_DATA_HOME:-~/.local/share}/JobPrep/JobPrep/jobprep.sqlite`
- **Linux Config:** `${XDG_CONFIG_HOME:-~/.config}/JobPrep/JobPrep.conf`
- **Windows:** `%APPDATA%\JobPrep\JobPrep\jobprep.sqlite` *(derived from code and Qt documentation, not run on Windows)*
- **Windows Config:** Registry `HKEY_CURRENT_USER\Software\JobPrep\JobPrep`
- **Safety Backups:** `.../JobPrep/JobPrep/backups/pre-restore-<timestamp>.json`

---

### 1.4 Repository Coordinates & Badges

```bash
$ git remote -v
origin	git@github.com:TonyTryHard/JobPrep.git (fetch)
origin	git@github.com:TonyTryHard/JobPrep.git (push)
```
- **Owner / Repo:** `TonyTryHard/JobPrep`
- **CI Workflow:** `.github/workflows/ci.yml`
- **CI Workflow Name:** `CI`
- **Badge Markdown:** `[![CI](https://github.com/TonyTryHard/JobPrep/actions/workflows/ci.yml/badge.svg)](https://github.com/TonyTryHard/JobPrep/actions/workflows/ci.yml)`

---

### 1.5 Licence Check

```bash
$ ls LICENSE* COPYING* 2>&1
ls: cannot access 'LICENSE*': No such file or directory
ls: cannot access 'COPYING*': No such file or directory
```
> **Notice:** No licence file currently exists in the repository. As confirmed by user decision, no licence file is required.

---

## 2. Screenshots

Six screenshots were generated offscreen at 100% display scaling and standard 1280x800 window geometry using clean sample data and an isolated test environment.

### 2.1 File Evidence & Sizes
```bash
$ ls -la docs/images && du -sh docs/images
total 532
drwxrwxr-x 2 tony tony   4096 Nov 24 09:37 .
drwxrwxr-x 6 tony tony   4096 Nov 24 09:37 ..
-rw-rw-r-- 1 tony tony  80879 Nov 24 09:38 screenshot-calendar.png
-rw-rw-r-- 1 tony tony  98794 Nov 24 09:38 screenshot-dark.png
-rw-rw-r-- 1 tony tony  97316 Nov 24 09:38 screenshot-home.png
-rw-rw-r-- 1 tony tony  80750 Nov 24 09:38 screenshot-jobs.png
-rw-rw-r-- 1 tony tony  55113 Nov 24 09:38 screenshot-settings.png
-rw-rw-r-- 1 tony tony 112673 Nov 24 09:38 screenshot-study.png
528K	docs/images
```
Every file is well below the 250 KB budget (largest is 112 KB; total directory is 528 KB).

### 2.2 Privacy & Data Audit
- **Data Source:** Study tracks and topics loaded from bundled seed dataset `resources/seed/topics.json`. Job applications, interviews, and study sessions populated with standard fictional mock entities: *Acme Corp*, *Globex Systems*, *Initech*, *Stark Industries*, *Umbrella Corp*, and *Hooli*.
- **Private Data Check:** Inspected each image; zero personal names, zero private email addresses, zero real company contacts, and zero file paths from host machines appear in the rendered frames.
- **Visual Inspection:** Each generated PNG was verified via the visual Read tool for crisp rendering, correct alignment, and absence of visual clipping.
- **Font Used:** Rendered using the system UI font (FreeType / DejaVu Sans offscreen fallback).

---

## 3. Evidence of Linux Clean Clone Build & Test

To verify that the build instructions published in the README work from scratch, a clean clone was executed in `/tmp/jobprep-clean-test` and timed:

```bash
$ rm -rf /tmp/jobprep-clean-test && \
  git clone /home/tony/projects/JobPrep /tmp/jobprep-clean-test && \
  cd /tmp/jobprep-clean-test && \
  time ( cmake --preset dev-linux && \
         cmake --build --preset dev-linux -j2 && \
         ctest --preset dev-linux -j2 --timeout 600 --output-on-failure ) && \
  cd /home/tony/projects/JobPrep && \
  rm -rf /tmp/jobprep-clean-test
```

### Complete Untrimmed Output & Timings:
```
Cloning into '/tmp/jobprep-clean-test'...
done.
-- The CXX compiler identification is GNU 15.2.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Success
-- Found Threads: TRUE
-- Performing Test HAVE_STDATOMIC
-- Performing Test HAVE_STDATOMIC - Success
-- Found WrapAtomic: TRUE
-- Found OpenGL: /usr/lib/x86_64-linux-gnu/libOpenGL.so
-- Found WrapOpenGL: TRUE
-- Found WrapVulkanHeaders: /usr/include
-- Configuring done (1.8s)
-- Generating done (0.4s)
-- Build files have been written to: /tmp/jobprep-clean-test/build/dev-linux
[1/240] Generating jobprep_ru.qm
Updating '/tmp/jobprep-clean-test/build/dev-linux/jobprep_ru.qm'...
    Generated 506 translation(s) (506 finished and 0 unfinished)
[2/240] Generating jobprep_uk.qm
Updating '/tmp/jobprep-clean-test/build/dev-linux/jobprep_uk.qm'...
    Generated 506 translation(s) (506 finished and 0 unfinished)
[3/240] Building CXX object CMakeFiles/jobprep_core_resources_1.dir/build/dev-linux/.qt/rcc/qrc_jobprep_core_translations_init.cpp.o
[4/240] Automatic MOC and UIC for target jobprep_core
[5/240] Automatic RCC for resources/resources.qrc
[6/240] Running rcc for resource jobprep_core_translations
[7/240] Building CXX object CMakeFiles/jobprep_core.dir/src/app/AppContext.cpp.o
[8/240] Building CXX object CMakeFiles/jobprep_core.dir/src/app/Resources.cpp.o
[9/240] Building CXX object CMakeFiles/jobprep_core.dir/jobprep_core_autogen/mocs_compilation.cpp.o
[10/240] Building CXX object CMakeFiles/jobprep_core.dir/src/app/Localization.cpp.o
[11/240] Building CXX object CMakeFiles/jobprep_core.dir/src/domain/TopicProgress.cpp.o
[12/240] Building CXX object CMakeFiles/jobprep_core.dir/src/domain/EnumStrings.cpp.o
[13/240] Building CXX object CMakeFiles/jobprep_core.dir/src/domain/StageRank.cpp.o
[14/240] Building CXX object CMakeFiles/jobprep_core.dir/src/data/DbFormat.cpp.o
[15/240] Building CXX object CMakeFiles/jobprep_core.dir/src/data/Database.cpp.o
[16/240] Building CXX object CMakeFiles/jobprep_core.dir/src/data/Repository.cpp.o
[17/240] Building CXX object CMakeFiles/jobprep_core.dir/src/data/Migrations.cpp.o
[18/240] Building CXX object CMakeFiles/jobprep_core.dir/src/data/SqlStatement.cpp.o
[19/240] Building CXX object CMakeFiles/jobprep_core.dir/src/data/TrackRepository.cpp.o
[20/240] Building CXX object CMakeFiles/jobprep_core.dir/src/data/TopicRepository.cpp.o
[21/240] Building CXX object CMakeFiles/jobprep_core.dir/src/data/SessionRepository.cpp.o
[22/240] Building CXX object CMakeFiles/jobprep_core.dir/src/data/ApplicationRepository.cpp.o
[23/240] Building CXX object CMakeFiles/jobprep_core.dir/src/data/InterviewRepository.cpp.o
[24/240] Building CXX object CMakeFiles/jobprep_core.dir/src/data/ReminderLogRepository.cpp.o
[25/240] Building CXX object CMakeFiles/jobprep_core.dir/src/services/AgendaService.cpp.o
[26/240] Building CXX object CMakeFiles/jobprep_core.dir/src/services/DayWatcher.cpp.o
[27/240] Building CXX object CMakeFiles/jobprep_core.dir/src/services/ReminderService.cpp.o
[28/240] Building CXX object CMakeFiles/jobprep_core.dir/src/services/StatsService.cpp.o
[29/240] Building CXX object CMakeFiles/jobprep_core.dir/src/services/SettingsService.cpp.o
[30/240] Building CXX object CMakeFiles/jobprep_core.dir/src/services/SeedService.cpp.o
[31/240] Building CXX object CMakeFiles/jobprep_core.dir/src/services/ExportService.cpp.o
[32/240] Building CXX object CMakeFiles/jobprep_core.dir/src/services/RestoreService.cpp.o
[33/240] Building CXX object CMakeFiles/jobprep_core.dir/src/models/ApplicationTableModel.cpp.o
[34/240] Building CXX object CMakeFiles/jobprep_core.dir/src/models/ApplicationFilterProxy.cpp.o
[35/240] Building CXX object CMakeFiles/jobprep_core.dir/src/models/AgendaListModel.cpp.o
[36/240] Building CXX object CMakeFiles/jobprep_core.dir/src/models/TopicListModel.cpp.o
[37/240] Building CXX object CMakeFiles/jobprep_core.dir/src/models/TopicBoardProxy.cpp.o
[38/240] Building CXX object CMakeFiles/jobprep_core.dir/src/models/TopicTableProxy.cpp.o
[39/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/theme/Tokens.cpp.o
[40/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/theme/StatusStyle.cpp.o
[41/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/theme/IconProvider.cpp.o
[42/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/theme/ThemeManager.cpp.o
[43/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/theme/AgendaKindStyle.cpp.o
[44/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/delegates/StatusBadgeDelegate.cpp.o
[45/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/delegates/AgendaItemDelegate.cpp.o
[46/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/delegates/TopicCardDelegate.cpp.o
[47/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/delegates/ProgressBarDelegate.cpp.o
[48/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/StudyLogging.cpp.o
[49/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/FormatHelper.cpp.o
[50/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/DashboardController.cpp.o
[51/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/DataToolsController.cpp.o
[52/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/widgets/EmptyState.cpp.o
[53/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/widgets/ChipBar.cpp.o
[54/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/widgets/MonthView.cpp.o
[55/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/widgets/SelectionRestorer.cpp.o
[56/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/widgets/StatCard.cpp.o
[57/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/widgets/ProgressRing.cpp.o
[58/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/widgets/FunnelWidget.cpp.o
[59/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/widgets/GoalProgressBar.cpp.o
[60/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/widgets/TopicBoardColumn.cpp.o
[61/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/widgets/TopicListView.cpp.o
[62/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/PageBase.cpp.o
[63/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/widgets/TopicDetailPanel.cpp.o
[64/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/dialogs/ApplicationDialog.cpp.o
[65/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/dialogs/InterviewDialog.cpp.o
[66/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/dialogs/FollowUpDialog.cpp.o
[67/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/dialogs/SessionDialog.cpp.o
[68/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/dialogs/TrackDialog.cpp.o
[69/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/pages/HomePage.cpp.o
[70/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/pages/JobsPage.cpp.o
[71/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/pages/StudyPage.cpp.o
[72/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/pages/CalendarPage.cpp.o
[73/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/pages/SettingsPage.cpp.o
[74/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/Sidebar.cpp.o
[75/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/MainWindow.cpp.o
[76/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/NotificationText.cpp.o
[77/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/Toast.cpp.o
[78/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/InAppNotifier.cpp.o
[79/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/TrayNotifier.cpp.o
[80/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/NotifierFactory.cpp.o
[81/240] Building CXX object CMakeFiles/jobprep_core.dir/build/dev-linux/.qt/rcc/qrc_jobprep_core_translations.cpp.o
[82/240] Building CXX object CMakeFiles/jobprep_core.dir/jobprep_core_autogen/3YJK5W5UP7/qrc_resources.cpp.o
[83/240] Building CXX object CMakeFiles/jobprep_core.dir/src/ui/ReminderCoordinator.cpp.o
[84/240] Linking CXX static library libjobprep_core.a
[85/240] Automatic MOC and UIC for target jobprep_test_helpers
[86/240] Building CXX object tests/data/CMakeFiles/jobprep_test_helpers.dir/jobprep_test_helpers_autogen/mocs_compilation.cpp.o
[87/240] Automatic MOC and UIC for target jobprep_ui_test_helpers
[88/240] Building CXX object tests/ui_test_helpers/CMakeFiles/jobprep_ui_test_helpers.dir/jobprep_ui_test_helpers_autogen/mocs_compilation.cpp.o
[89/240] Building CXX object tests/data/CMakeFiles/jobprep_test_helpers.dir/MessageCapture.cpp.o
[90/240] Building CXX object tests/ui_test_helpers/CMakeFiles/jobprep_ui_test_helpers.dir/PseudoTranslator.cpp.o
[91/240] Building CXX object tests/ui_test_helpers/CMakeFiles/jobprep_ui_test_helpers.dir/UiTestHelpers.cpp.o
[92/240] Linking CXX static library tests/ui_test_helpers/libjobprep_ui_test_helpers.a
[93/240] Automatic MOC and UIC for target tst_Translations
[94/240] Automatic MOC and UIC for target jobprep
[95/240] Building CXX object tests/data/CMakeFiles/jobprep_test_helpers.dir/TestDatabase.cpp.o
[96/240] Linking CXX static library tests/data/libjobprep_test_helpers.a
[97/240] Automatic MOC and UIC for target ui_smoke
[98/240] Automatic MOC and UIC for target tst_Database
[99/240] Automatic MOC and UIC for target tst_TrackRepository
[100/240] Automatic MOC and UIC for target tst_TopicRepository
[101/240] Automatic MOC and UIC for target tst_SessionRepository
[102/240] Automatic MOC and UIC for target tst_ApplicationRepository
[103/240] Automatic MOC and UIC for target tst_InterviewRepository
[104/240] Automatic MOC and UIC for target tst_ReminderLogRepository
[105/240] Automatic MOC and UIC for target tst_SeedService
[106/240] Automatic MOC and UIC for target tst_TransactionBehavior
[107/240] Automatic MOC and UIC for target tst_AgendaListModel
[108/240] Automatic MOC and UIC for target tst_MonthView
[109/240] Automatic MOC and UIC for target tst_ApplicationFilterProxy
[110/240] Automatic MOC and UIC for target tst_ApplicationTableModel
[111/240] Automatic MOC and UIC for target tst_TopicListModel
[112/240] Automatic MOC and UIC for target tst_TopicBoardProxy
[113/240] Automatic MOC and UIC for target tst_TopicTableProxy
[114/240] Automatic MOC and UIC for target tst_AgendaService
[115/240] Automatic MOC and UIC for target tst_ExportService
[116/240] Automatic MOC and UIC for target tst_DayWatcher
[117/240] Automatic MOC and UIC for target tst_ImportExport
[118/240] Automatic MOC and UIC for target tst_ReminderService
[119/240] Automatic MOC and UIC for target tst_SettingsService
[120/240] Automatic MOC and UIC for target tst_StatsService
[121/240] Automatic MOC and UIC for target tst_Notifiers
[122/240] Automatic MOC and UIC for target tst_StudyRegression
[123/240] Automatic MOC and UIC for target tst_PageJobs
[124/240] Automatic MOC and UIC for target tst_PageCalendar
[125/240] Automatic MOC and UIC for target tst_PageStudy
[126/240] Automatic MOC and UIC for target tst_PageHome
[127/240] Automatic MOC and UIC for target tst_PageSettings
[128/240] Automatic MOC and UIC for target tst_Shortcuts
[129/240] Automatic MOC and UIC for target tst_EmptyStatesAndFocus
[130/240] Generating plural_uk.qm
Updating '/tmp/jobprep-clean-test/build/dev-linux/tests/i18n/plural_uk.qm'...
    Generated 10 translation(s) (10 finished and 0 unfinished)
[131/240] Generating plural_ru.qm
Updating '/tmp/jobprep-clean-test/build/dev-linux/tests/i18n/plural_ru.qm'...
    Generated 10 translation(s) (10 finished and 0 unfinished)
[132/240] Building CXX object CMakeFiles/jobprep.dir/jobprep_autogen/mocs_compilation.cpp.o
[133/240] Automatic MOC and UIC for target tst_HiDpi
[134/240] Building CXX object tests/data/CMakeFiles/tst_Database.dir/tst_Database_autogen/mocs_compilation.cpp.o
[135/240] Building CXX object CMakeFiles/jobprep.dir/src/main.cpp.o
[136/240] Building CXX object tests/data/CMakeFiles/tst_TrackRepository.dir/tst_TrackRepository_autogen/mocs_compilation.cpp.o
[137/240] Building CXX object tests/data/CMakeFiles/tst_Database.dir/tst_Database.cpp.o
[138/240] Building CXX object tests/data/CMakeFiles/tst_TopicRepository.dir/tst_TopicRepository_autogen/mocs_compilation.cpp.o
[139/240] Building CXX object tests/data/CMakeFiles/tst_TrackRepository.dir/tst_TrackRepository.cpp.o
[140/240] Building CXX object tests/data/CMakeFiles/tst_SessionRepository.dir/tst_SessionRepository_autogen/mocs_compilation.cpp.o
[141/240] Building CXX object tests/data/CMakeFiles/tst_TopicRepository.dir/tst_TopicRepository.cpp.o
[142/240] Building CXX object tests/data/CMakeFiles/tst_ApplicationRepository.dir/tst_ApplicationRepository_autogen/mocs_compilation.cpp.o
[143/240] Building CXX object tests/data/CMakeFiles/tst_SessionRepository.dir/tst_SessionRepository.cpp.o
[144/240] Building CXX object tests/data/CMakeFiles/tst_InterviewRepository.dir/tst_InterviewRepository_autogen/mocs_compilation.cpp.o
[145/240] Building CXX object tests/data/CMakeFiles/tst_ApplicationRepository.dir/tst_ApplicationRepository.cpp.o
[146/240] Building CXX object tests/data/CMakeFiles/tst_ReminderLogRepository.dir/tst_ReminderLogRepository_autogen/mocs_compilation.cpp.o
[147/240] Building CXX object tests/data/CMakeFiles/tst_InterviewRepository.dir/tst_InterviewRepository.cpp.o
[148/240] Building CXX object tests/data/CMakeFiles/tst_SeedService.dir/tst_SeedService_autogen/mocs_compilation.cpp.o
[149/240] Building CXX object tests/data/CMakeFiles/tst_ReminderLogRepository.dir/tst_ReminderLogRepository.cpp.o
[150/240] Building CXX object tests/data/CMakeFiles/tst_TransactionBehavior.dir/tst_TransactionBehavior_autogen/mocs_compilation.cpp.o
[151/240] Building CXX object tests/data/CMakeFiles/tst_SeedService.dir/tst_SeedService.cpp.o
[152/240] Building CXX object tests/ui_smoke/CMakeFiles/ui_smoke.dir/ui_smoke_autogen/mocs_compilation.cpp.o
[153/240] Building CXX object tests/data/CMakeFiles/tst_TransactionBehavior.dir/tst_TransactionBehavior.cpp.o
[154/240] Building CXX object tests/ui_smoke/CMakeFiles/tst_MonthView.dir/tst_MonthView_autogen/mocs_compilation.cpp.o
[155/240] Building CXX object tests/ui_smoke/CMakeFiles/ui_smoke.dir/tst_UiSmoke.cpp.o
[156/240] Building CXX object tests/models/CMakeFiles/tst_AgendaListModel.dir/tst_AgendaListModel_autogen/mocs_compilation.cpp.o
[157/240] Building CXX object tests/ui_smoke/CMakeFiles/tst_MonthView.dir/tst_MonthView.cpp.o
[158/240] Building CXX object tests/models/CMakeFiles/tst_ApplicationTableModel.dir/tst_ApplicationTableModel_autogen/mocs_compilation.cpp.o
[159/240] Building CXX object tests/models/CMakeFiles/tst_AgendaListModel.dir/tst_AgendaListModel.cpp.o
[160/240] Building CXX object tests/models/CMakeFiles/tst_ApplicationFilterProxy.dir/tst_ApplicationFilterProxy_autogen/mocs_compilation.cpp.o
[161/240] Building CXX object tests/models/CMakeFiles/tst_ApplicationTableModel.dir/tst_ApplicationTableModel.cpp.o
[162/240] Building CXX object tests/models/CMakeFiles/tst_TopicListModel.dir/tst_TopicListModel_autogen/mocs_compilation.cpp.o
[163/240] Building CXX object tests/models/CMakeFiles/tst_ApplicationFilterProxy.dir/tst_ApplicationFilterProxy.cpp.o
[164/240] Building CXX object tests/models/CMakeFiles/tst_TopicBoardProxy.dir/tst_TopicBoardProxy_autogen/mocs_compilation.cpp.o
[165/240] Building CXX object tests/models/CMakeFiles/tst_TopicListModel.dir/tst_TopicListModel.cpp.o
[166/240] Building CXX object tests/models/CMakeFiles/tst_TopicTableProxy.dir/tst_TopicTableProxy_autogen/mocs_compilation.cpp.o
[167/240] Building CXX object tests/models/CMakeFiles/tst_TopicBoardProxy.dir/tst_TopicBoardProxy.cpp.o
[168/240] Building CXX object tests/services/CMakeFiles/tst_AgendaService.dir/tst_AgendaService_autogen/mocs_compilation.cpp.o
[169/240] Building CXX object tests/models/CMakeFiles/tst_TopicTableProxy.dir/tst_TopicTableProxy.cpp.o
[170/240] Building CXX object tests/services/CMakeFiles/tst_DayWatcher.dir/tst_DayWatcher_autogen/mocs_compilation.cpp.o
[171/240] Building CXX object tests/services/CMakeFiles/tst_AgendaService.dir/tst_AgendaService.cpp.o
[172/240] Building CXX object tests/services/CMakeFiles/tst_ExportService.dir/tst_ExportService_autogen/mocs_compilation.cpp.o
[173/240] Building CXX object tests/services/CMakeFiles/tst_DayWatcher.dir/tst_DayWatcher.cpp.o
[174/240] Building CXX object tests/services/CMakeFiles/tst_ImportExport.dir/tst_ImportExport_autogen/mocs_compilation.cpp.o
[175/240] Building CXX object tests/services/CMakeFiles/tst_ExportService.dir/tst_ExportService.cpp.o
[176/240] Building CXX object tests/services/CMakeFiles/tst_ReminderService.dir/tst_ReminderService_autogen/mocs_compilation.cpp.o
[177/240] Building CXX object tests/services/CMakeFiles/tst_ImportExport.dir/tst_ImportExport.cpp.o
[178/240] Building CXX object tests/services/CMakeFiles/tst_SettingsService.dir/tst_SettingsService_autogen/mocs_compilation.cpp.o
[179/240] Building CXX object tests/services/CMakeFiles/tst_ReminderService.dir/tst_ReminderService.cpp.o
[180/240] Building CXX object tests/services/CMakeFiles/tst_StatsService.dir/tst_StatsService_autogen/mocs_compilation.cpp.o
[181/240] Building CXX object tests/services/CMakeFiles/tst_SettingsService.dir/tst_SettingsService.cpp.o
[182/240] Building CXX object tests/ui/CMakeFiles/tst_Notifiers.dir/tst_Notifiers_autogen/mocs_compilation.cpp.o
[183/240] Building CXX object tests/services/CMakeFiles/tst_StatsService.dir/tst_StatsService.cpp.o
[184/240] Building CXX object tests/ui/CMakeFiles/tst_StudyRegression.dir/tst_StudyRegression_autogen/mocs_compilation.cpp.o
[185/240] Building CXX object tests/ui/CMakeFiles/tst_Notifiers.dir/tst_Notifiers.cpp.o
[186/240] Building CXX object tests/ui/CMakeFiles/tst_PageJobs.dir/tst_PageJobs_autogen/mocs_compilation.cpp.o
[187/240] Building CXX object tests/ui/CMakeFiles/tst_StudyRegression.dir/tst_StudyRegression.cpp.o
[188/240] Building CXX object tests/ui/CMakeFiles/tst_PageCalendar.dir/tst_PageCalendar_autogen/mocs_compilation.cpp.o
[189/240] Building CXX object tests/ui/CMakeFiles/tst_PageJobs.dir/tst_PageJobs.cpp.o
[190/240] Building CXX object tests/ui/CMakeFiles/tst_PageStudy.dir/tst_PageStudy_autogen/mocs_compilation.cpp.o
[191/240] Building CXX object tests/ui/CMakeFiles/tst_PageCalendar.dir/tst_PageCalendar.cpp.o
[192/240] Building CXX object tests/ui/CMakeFiles/tst_PageHome.dir/tst_PageHome_autogen/mocs_compilation.cpp.o
[193/240] Building CXX object tests/ui/CMakeFiles/tst_PageStudy.dir/tst_PageStudy.cpp.o
[194/240] Building CXX object tests/ui/CMakeFiles/tst_PageSettings.dir/tst_PageSettings_autogen/mocs_compilation.cpp.o
[195/240] Building CXX object tests/ui/CMakeFiles/tst_PageHome.dir/tst_PageHome.cpp.o
[196/240] Building CXX object tests/ui/CMakeFiles/tst_Shortcuts.dir/tst_Shortcuts_autogen/mocs_compilation.cpp.o
[197/240] Building CXX object tests/ui/CMakeFiles/tst_PageSettings.dir/tst_PageSettings.cpp.o
[198/240] Building CXX object tests/ui/CMakeFiles/tst_EmptyStatesAndFocus.dir/tst_EmptyStatesAndFocus_autogen/mocs_compilation.cpp.o
[199/240] Building CXX object tests/ui/CMakeFiles/tst_Shortcuts.dir/tst_Shortcuts.cpp.o
[200/240] Building CXX object tests/ui/CMakeFiles/tst_HiDpi.dir/tst_HiDpi_autogen/mocs_compilation.cpp.o
[201/240] Building CXX object tests/ui/CMakeFiles/tst_EmptyStatesAndFocus.dir/tst_EmptyStatesAndFocus.cpp.o
[202/240] Building CXX object tests/i18n/CMakeFiles/tst_Translations.dir/tst_Translations_autogen/mocs_compilation.cpp.o
[203/240] Building CXX object tests/ui/CMakeFiles/tst_HiDpi.dir/tst_HiDpi.cpp.o
[204/240] Linking CXX executable jobprep
[205/240] Linking CXX executable tests/data/tst_Database
[206/240] Linking CXX executable tests/data/tst_TrackRepository
[207/240] Linking CXX executable tests/data/tst_TopicRepository
[208/240] Building CXX object tests/i18n/CMakeFiles/tst_Translations.dir/tst_Translations.cpp.o
[209/240] Linking CXX executable tests/data/tst_SessionRepository
[210/240] Linking CXX executable tests/data/tst_ApplicationRepository
[211/240] Linking CXX executable tests/data/tst_InterviewRepository
[212/240] Linking CXX executable tests/data/tst_ReminderLogRepository
[213/240] Linking CXX executable tests/data/tst_SeedService
[214/240] Linking CXX executable tests/data/tst_TransactionBehavior
[215/240] Linking CXX executable tests/ui_smoke/ui_smoke
[216/240] Linking CXX executable tests/ui_smoke/tst_MonthView
[217/240] Linking CXX executable tests/models/tst_AgendaListModel
[218/240] Linking CXX executable tests/models/tst_ApplicationTableModel
[219/240] Linking CXX executable tests/models/tst_ApplicationFilterProxy
[220/240] Linking CXX executable tests/models/tst_TopicListModel
[221/240] Linking CXX executable tests/models/tst_TopicBoardProxy
[222/240] Linking CXX executable tests/models/tst_TopicTableProxy
[223/240] Linking CXX executable tests/services/tst_AgendaService
[224/240] Linking CXX executable tests/services/tst_DayWatcher
[225/240] Linking CXX executable tests/services/tst_ExportService
[226/240] Linking CXX executable tests/services/tst_ImportExport
[227/240] Linking CXX executable tests/services/tst_ReminderService
[228/240] Linking CXX executable tests/services/tst_SettingsService
[229/240] Linking CXX executable tests/services/tst_StatsService
[230/240] Linking CXX executable tests/ui/tst_Notifiers
[231/240] Linking CXX executable tests/ui/tst_StudyRegression
[232/240] Linking CXX executable tests/ui/tst_PageJobs
[233/240] Linking CXX executable tests/ui/tst_PageCalendar
[234/240] Linking CXX executable tests/ui/tst_PageStudy
[235/240] Linking CXX executable tests/ui/tst_PageHome
[236/240] Linking CXX executable tests/ui/tst_PageSettings
[237/240] Linking CXX executable tests/ui/tst_Shortcuts
[238/240] Linking CXX executable tests/ui/tst_EmptyStatesAndFocus
[239/240] Linking CXX executable tests/ui/tst_HiDpi
[240/240] Linking CXX executable tests/i18n/tst_Translations
Test project /tmp/jobprep-clean-test/build/dev-linux
      Start  1: tst_Database
      Start  2: tst_TrackRepository
 1/35 Test  #2: tst_TrackRepository ..............   Passed    0.05 sec
      Start  3: tst_TopicRepository
 2/35 Test  #1: tst_Database .....................   Passed    0.06 sec
      Start  4: tst_SessionRepository
 3/35 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  5: tst_ApplicationRepository
 4/35 Test  #3: tst_TopicRepository ..............   Passed    0.10 sec
      Start  6: tst_InterviewRepository
 5/35 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start  7: tst_ReminderLogRepository
 6/35 Test  #5: tst_ApplicationRepository ........   Passed    0.11 sec
      Start  8: tst_SeedService
 7/35 Test  #7: tst_ReminderLogRepository ........   Passed    0.02 sec
      Start  9: tst_TransactionBehavior
 8/35 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start 10: ui_smoke
 9/35 Test  #8: tst_SeedService ..................   Passed    0.31 sec
      Start 11: tst_MonthView
10/35 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 12: tst_AgendaListModel
11/35 Test #12: tst_AgendaListModel ..............   Passed    0.03 sec
      Start 13: tst_ApplicationTableModel
12/35 Test #13: tst_ApplicationTableModel ........   Passed    0.06 sec
      Start 14: tst_ApplicationFilterProxy
13/35 Test #14: tst_ApplicationFilterProxy .......   Passed    0.02 sec
      Start 15: tst_TopicListModel
14/35 Test #15: tst_TopicListModel ...............   Passed    0.04 sec
      Start 16: tst_TopicBoardProxy
15/35 Test #16: tst_TopicBoardProxy ..............   Passed    0.06 sec
      Start 17: tst_TopicTableProxy
16/35 Test #17: tst_TopicTableProxy ..............   Passed    0.07 sec
      Start 18: tst_AgendaService
17/35 Test #18: tst_AgendaService ................   Passed    0.09 sec
      Start 19: tst_DayWatcher
18/35 Test #19: tst_DayWatcher ...................   Passed    0.01 sec
      Start 20: tst_ExportService
19/35 Test #20: tst_ExportService ................   Passed    0.04 sec
      Start 21: tst_ImportExport
20/35 Test #21: tst_ImportExport .................   Passed    0.73 sec
      Start 22: tst_ReminderService
21/35 Test #22: tst_ReminderService ..............   Passed    0.08 sec
      Start 23: tst_SettingsService
22/35 Test #23: tst_SettingsService ..............   Passed    0.06 sec
      Start 24: tst_StatsService
23/35 Test #24: tst_StatsService .................   Passed    1.04 sec
      Start 25: tst_Notifiers
24/35 Test #25: tst_Notifiers ....................   Passed    3.11 sec
      Start 26: tst_StudyRegression
25/35 Test #10: ui_smoke .........................   Passed   14.40 sec
      Start 27: tst_PageJobs
26/35 Test #26: tst_StudyRegression ..............   Passed   10.89 sec
      Start 28: tst_PageCalendar
27/35 Test #28: tst_PageCalendar .................   Passed    9.37 sec
      Start 29: tst_PageStudy
28/35 Test #27: tst_PageJobs .....................   Passed   15.69 sec
      Start 30: tst_PageHome
29/35 Test #29: tst_PageStudy ....................   Passed   22.38 sec
      Start 31: tst_PageSettings
30/35 Test #30: tst_PageHome .....................   Passed   19.71 sec
      Start 32: tst_Shortcuts
31/35 Test #31: tst_PageSettings .................   Passed    5.23 sec
      Start 33: tst_EmptyStatesAndFocus
32/35 Test #32: tst_Shortcuts ....................   Passed    9.04 sec
      Start 34: tst_HiDpi
33/35 Test #33: tst_EmptyStatesAndFocus ..........   Passed   12.53 sec
      Start 35: tst_Translations
34/35 Test #35: tst_Translations .................   Passed    0.07 sec
35/35 Test #34: tst_HiDpi ........................   Passed   20.54 sec

100% tests passed, 0 tests failed out of 35

Label Time Summary:
data       =   0.76 sec*proc (9 tests)
i18n       =   0.07 sec*proc (1 test)
model      =   0.28 sec*proc (6 tests)
service    =   2.05 sec*proc (7 tests)
ui         = 142.97 sec*proc (12 tests)

Total Test time (real) =  79.63 sec

real	7m46.427s
user	14m12.331s
sys	1m8.710s
```

---

## 4. README Verification & Markdown Rendering

### 4.1 Relative Link & Image Resolution
```bash
$ python3 -c "
import re, os
with open('README.md') as f:
    text = f.read()
matches = re.findall(r'!\[([^\]]*)\]\(([^)]+)\)', text)
for alt, src in matches:
    if not src.startswith('http'):
        print(f'{alt}: {src} -> exists? {os.path.exists(src)} ({os.path.getsize(src)} bytes)')
"
Dashboard: docs/images/screenshot-home.png -> exists? True (97316 bytes)
Study: docs/images/screenshot-study.png -> exists? True (112673 bytes)
Jobs: docs/images/screenshot-jobs.png -> exists? True (80750 bytes)
Calendar: docs/images/screenshot-calendar.png -> exists? True (80879 bytes)
Settings: docs/images/screenshot-settings.png -> exists? True (55113 bytes)
Dark Theme: docs/images/screenshot-dark.png -> exists? True (98794 bytes)
```

Relative document links checked:
- `docs/RUNNING.md`: OK
- `docs/SPEC.md#37-global`: OK

### 4.2 Markdown Rendering Check
Rendered with `python-markdown-it` (`markdown_it.MarkdownIt('gfm-like')`):
```bash
Rendered HTML length: 16149 characters
Status: Clean semantic HTML produced, valid tags, closed tables and code blocks.
```

---

## 5. Repository Hygiene Audit

### 5.1 Tracked Files in `docs/`
At initial audit, 204 files were tracked in `docs/`.
Following user direction, all 72 prompts and 9 milestone plans have been relocated to the dedicated archive branch `archive/dev-docs`.

### 5.2 Private-Looking Content Audit
```bash
$ git grep -n "/home/\|C:\\\\Users\|@gmail\|@users.noreply" -- . ':!docs/reports'
docs/RUNNING.md:180:See [SPEC.md §3.7](file:///home/tony/projects/JobPrep/docs/SPEC.md#37-global) for full shortcut specifications and focus handling rules.
```
- **Hit in `docs/RUNNING.md:180`**: Resolved in commit `578e720` (fixed to `SPEC.md#37-global`).

Counting total hits across the repository (including `docs/reports/`):
```bash
$ git grep -n "/home/\|C:\\\\Users\|@gmail\|@users.noreply" -- . | wc -l
189
```
**Verdict:** The occurrences inside `docs/reports/` are historical compilation logs and command transcript proofs capturing the development environment path (`/home/tony/...` and Windows CI `C:\Users\...`). They contain no API credentials or private data.

### 5.3 Large or Binary Files in Repository
```bash
$ git ls-files -z | xargs -0 du -ch | tail -1
11M	total

$ git ls-files -z | xargs -0 du -h | sort -hr | head -10
152K	docs/reports/img/study-list-light.png
152K	docs/reports/img/study-list-dark.png
148K	docs/reports/img/hidpi150-study-dark.png
144K	docs/reports/img/hidpi150-study-light.png
132K	docs/reports/img/hidpi150-home-dark.png
128K	docs/reports/img/hidpi150-home-light.png
124K	docs/reports/img/hidpi150-calendar-light.png
124K	docs/reports/img/hidpi150-calendar-dark.png
120K	docs/reports/M7-partB.md
116K	docs/reports/M7-partD.md
```
Total tracked repository size is only 11 MB. No large binaries or unexpected objects exist in git tracking.

### 5.4 AI Agent Product Mentions
```bash
$ grep -i -E "gemini|chatgpt|copilot|claude|antigravity" AGENTS.md || echo "(no output)"
(no output)
```
- `AGENTS.md` does not name any AI assistant or product.

---

## 6. Allowed Edits Made Outside README

- **File Modified:** `docs/RUNNING.md`
- **Changes Made:**
  1. Fixed absolute link URI on line 180 to `SPEC.md#37-global` (commit `578e720`).
  2. Corrected Windows data path to `%APPDATA%\JobPrep\JobPrep\` with label *(derived from code, not run on Windows)* and removed unsafe Windows isolation snippet (commit `e2b4851`).

---

## 7. Quality Gate Execution on Working Tree

Ran full quality check on current tree with `QT_PREFIX_PATH=/usr/lib/qt6` and `CMAKE_BUILD_PARALLEL_LEVEL=2`:
```bash
$ QT_PREFIX_PATH=/usr/lib/qt6 CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.2s)
-- Generating done (0.3s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
ninja: no work to do.
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start 30: tst_PageHome
      Start 27: tst_PageJobs
 1/35 Test #27: tst_PageJobs .....................   Passed   13.32 sec
      Start 34: tst_HiDpi
 2/35 Test #30: tst_PageHome .....................   Passed   17.57 sec
      Start 29: tst_PageStudy
 3/35 Test #34: tst_HiDpi ........................   Passed   19.04 sec
      Start 28: tst_PageCalendar
 4/35 Test #29: tst_PageStudy ....................   Passed   19.86 sec
      Start 33: tst_EmptyStatesAndFocus
 5/35 Test #28: tst_PageCalendar .................   Passed    7.34 sec
      Start 32: tst_Shortcuts
 6/35 Test #32: tst_Shortcuts ....................   Passed    7.93 sec
      Start 26: tst_StudyRegression
 7/35 Test #33: tst_EmptyStatesAndFocus ..........   Passed   11.04 sec
      Start 25: tst_Notifiers
 8/35 Test #25: tst_Notifiers ....................   Passed    2.60 sec
      Start 31: tst_PageSettings
 9/35 Test #31: tst_PageSettings .................   Passed    4.92 sec
      Start 10: ui_smoke
10/35 Test #26: tst_StudyRegression ..............   Passed   10.17 sec
      Start 24: tst_StatsService
11/35 Test #24: tst_StatsService .................   Passed    0.98 sec
      Start 21: tst_ImportExport
12/35 Test #21: tst_ImportExport .................   Passed    0.69 sec
      Start 35: tst_Translations
13/35 Test #35: tst_Translations .................   Passed    0.05 sec
      Start  8: tst_SeedService
14/35 Test  #8: tst_SeedService ..................   Passed    0.28 sec
      Start 22: tst_ReminderService
15/35 Test #22: tst_ReminderService ..............   Passed    0.08 sec
      Start 23: tst_SettingsService
16/35 Test #23: tst_SettingsService ..............   Passed    0.02 sec
      Start 17: tst_TopicTableProxy
17/35 Test #17: tst_TopicTableProxy ..............   Passed    0.07 sec
      Start  5: tst_ApplicationRepository
18/35 Test  #5: tst_ApplicationRepository ........   Passed    0.09 sec
      Start 18: tst_AgendaService
19/35 Test #18: tst_AgendaService ................   Passed    0.10 sec
      Start 13: tst_ApplicationTableModel
20/35 Test #13: tst_ApplicationTableModel ........   Passed    0.06 sec
      Start 12: tst_AgendaListModel
21/35 Test #12: tst_AgendaListModel ..............   Passed    0.03 sec
      Start  3: tst_TopicRepository
22/35 Test  #3: tst_TopicRepository ..............   Passed    0.09 sec
      Start 16: tst_TopicBoardProxy
23/35 Test #16: tst_TopicBoardProxy ..............   Passed    0.05 sec
      Start  6: tst_InterviewRepository
24/35 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start 15: tst_TopicListModel
25/35 Test #15: tst_TopicListModel ...............   Passed    0.04 sec
      Start 11: tst_MonthView
26/35 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 19: tst_DayWatcher
27/35 Test #19: tst_DayWatcher ...................   Passed    0.02 sec
      Start  4: tst_SessionRepository
28/35 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  7: tst_ReminderLogRepository
29/35 Test  #7: tst_ReminderLogRepository ........   Passed    0.02 sec
      Start 14: tst_ApplicationFilterProxy
30/35 Test #14: tst_ApplicationFilterProxy .......   Passed    0.02 sec
      Start  2: tst_TrackRepository
31/35 Test  #2: tst_TrackRepository ..............   Passed    0.02 sec
      Start  9: tst_TransactionBehavior
32/35 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start  1: tst_Database
33/35 Test  #1: tst_Database .....................   Passed    0.04 sec
      Start 20: tst_ExportService
34/35 Test #20: tst_ExportService ................   Passed    0.01 sec
35/35 Test #10: ui_smoke .........................   Passed   12.54 sec

100% tests passed, 0 tests failed out of 35

Label Time Summary:
data       =   0.62 sec*proc (9 tests)
i18n       =   0.05 sec*proc (1 test)
model      =   0.28 sec*proc (6 tests)
service    =   1.90 sec*proc (7 tests)
ui         = 126.39 sec*proc (12 tests)

Total Test time (real) =  68.52 sec
==> Checking translation sync with /usr/lib/qt6/bin/lupdate...
==> Translation files are in sync.
==> Quality check PASSED with zero warnings and all tests green!
```

---

## 8. Not Verified by Agent (User to Check)

1. **Windows Build on Physical Windows PC:** The Windows configure line and MSVC paths were cross-checked against `.github/workflows/ci.yml` and `docs/RUNNING.md` (the build and test pass in Windows CI offscreen). The `windeployqt` packaging recipe is the maintainer's procedure, is not part of CI, and was not verified on a physical Windows machine by this agent.
2. **Screenshots vs. Real Screen:** The screenshots were captured headlessly via the Qt offscreen platform plugin using DejaVu Sans font. When the user is at his Windows PC, he may replace them with live desktop captures if desired.
3. **GitHub Badges:** The workflow badge URL points to `TonyTryHard/JobPrep/actions/workflows/ci.yml/badge.svg`. It will activate live on GitHub once these commits are pushed.
4. **Tone and Presentation:** The README text was composed without marketing superlatives or decorative emojis, adhering to a factual engineering tone.

---

## 9. User Decisions Recorded

1. **Licence Selection:** User decided not to add any licence file. Omitted from repository and `README.md`.
2. **Contributing Guidelines:** User requested inviting community participation. Added dedicated `## Contributing` section.
3. **Working Documents Relocation:** User approved moving internal prompts and plans to an archive branch. Branch `archive/dev-docs` created, and `docs/prompts/` and `docs/plans/` removed from `main`.

---

## 10. Git Status and Worktree State

```bash
$ git status --short
(no output)

$ git worktree list
/home/tony/projects/JobPrep 79dc311 [main]

$ git log --oneline -9
79dc311 docs(reports): update README task report with fix-up audit and untrimmed logs
e2b4851 docs(running): correct Windows AppData paths and remove unsafe isolation snippet
9634205 docs(readme): fix Windows data paths, isolation guidance, and status section
b3ae01f chore(docs): move dev prompts and plans to archive branch
71d64dd docs(readme): add contributing section
6a274a0 docs(reports): add README task verification and hygiene report
578e720 docs(running): replace absolute file URI with relative link
05a5e6b docs: write comprehensive GitHub README
658f5c5 docs(images): add application screenshots for README
```

---

## 11. README Fix-up Verification (`docs/README-fixup.md`)

### 11.1 Windows Data Path & Isolation Snippet Audit

#### Code Proof: AppDataLocation is Used Everywhere
```bash
$ grep -rn "AppDataLocation\|AppLocalDataLocation" src
src/app/AppContext.cpp:28:/// `QStandardPaths::AppDataLocation` + `/jobprep.sqlite` (SPEC §8, AGENTS §12).
src/app/AppContext.cpp:30:    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
src/ui/DataToolsController.cpp:280:        ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + u"/backups"_s
src/ui/DataToolsController.cpp:388:                               ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
```

#### Qt Documentation on `AppDataLocation` vs `AppLocalDataLocation`
According to official Qt 6 documentation (`QStandardPaths::StandardLocation` enum):
- `QStandardPaths::AppDataLocation`:
  > "Returns a directory location where persistent application data can be stored. This is an application-specific directory. To the paths returned by `GenericDataLocation`, `/organization/application` is typically appended. On Windows, this is typically `C:/Users/<USER>/AppData/Roaming/<AppName>`."
- `QStandardPaths::AppLocalDataLocation`:
  > "Returns the local settings path on the Windows operating system. On Windows, this is typically `C:/Users/<USER>/AppData/Local/<AppName>`."

Because JobPrep calls `AppDataLocation`, the folder on Windows is `%APPDATA%\JobPrep\JobPrep` (Roaming), NOT `%LOCALAPPDATA%`.

#### Grep Before and After Across `README.md` and `docs/RUNNING.md`
**Before:**
```
README.md:110:  - Windows: `%LOCALAPPDATA%\JobPrep\JobPrep\jobprep.sqlite`
README.md:116:  - Windows: `%LOCALAPPDATA%\JobPrep\JobPrep\backups\pre-restore-<timestamp>.json`
README.md:131:$env:LOCALAPPDATA = "$env:TEMP\jp-isolated"; .\build\dev-windows\jobprep.exe
docs/RUNNING.md:106:  - Windows: `%LOCALAPPDATA%\JobPrep\JobPrep\jobprep.sqlite`
docs/RUNNING.md:112:  - Windows: `%LOCALAPPDATA%\JobPrep\JobPrep\backups\pre-restore-<timestamp>.json`
docs/RUNNING.md:123:$env:LOCALAPPDATA = "$env:TEMP\jp-isolated"; .\build\dev-windows\jobprep.exe
```

**After:**
```bash
$ grep -n "LOCALAPPDATA\|APPDATA" README.md docs/RUNNING.md
README.md:110:  - Windows: `%APPDATA%\JobPrep\JobPrep\jobprep.sqlite` *(derived from code, not run on Windows)*
README.md:116:  - Windows: `%APPDATA%\JobPrep\JobPrep\backups\pre-restore-<timestamp>.json` *(derived from code, not run on Windows)*
docs/RUNNING.md:106:  - Windows: `%APPDATA%\JobPrep\JobPrep\jobprep.sqlite` *(derived from code, not run on Windows)*
docs/RUNNING.md:112:  - Windows: `%APPDATA%\JobPrep\JobPrep\backups\pre-restore-<timestamp>.json` *(derived from code, not run on Windows)*
```

#### Windows Isolation Cannot Be Redirected Via Environment
```bash
$ grep -n "cannot be redirected" tests -r
tests/services/tst_ImportExport.cpp:1001:        QSKIP("AppDataLocation cannot be redirected via environment on this platform; "
```
`tests/services/tst_ImportExport.cpp` explicitly skips safety directory verification on Windows because Windows shell APIs (`SHGetKnownFolderPath`) ignore environment variables. The unsafe PowerShell snippet has been completely removed from both `README.md` and `docs/RUNNING.md`, replaced with:
> "On Windows, the application data directory is resolved via the Windows shell API and cannot be redirected using environment variables; back up your database first (**Settings** -> **Data** -> **Backup**) before running a development build."

#### Linux Isolation Verification (Scratch Program Outside Repo)
```bash
$ python3 -c "
import os, subprocess
code = '''#include <QCoreApplication>
#include <QStandardPaths>
#include <QSettings>
#include <iostream>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(\"JobPrep\");
    QCoreApplication::setApplicationName(\"JobPrep\");
    std::cout << \"AppDataLocation: \" << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString() << std::endl;
    QSettings s;
    std::cout << \"QSettings fileName: \" << s.fileName().toStdString() << std::endl;
    return 0;
}'''
with open('/tmp/test_isolation.cpp', 'w') as f:
    f.write(code)
subprocess.run(['c++', '-std=c++20', '/tmp/test_isolation.cpp', '-o', '/tmp/test_isolation', '-lQt6Core', '-isystem', '/usr/include/x86_64-linux-gnu/qt6/QtCore', '-isystem', '/usr/include/x86_64-linux-gnu/qt6'], check=True)
env = os.environ.copy()
env['HOME'] = '/tmp/jp-isolated'
env['XDG_DATA_HOME'] = '/tmp/jp-isolated/.local/share'
env['XDG_CONFIG_HOME'] = '/tmp/jp-isolated/.config'
res = subprocess.run(['/tmp/test_isolation'], env=env, capture_output=True, text=True, check=True)
print(res.stdout)
" && rm -f /tmp/test_isolation.cpp /tmp/test_isolation
AppDataLocation: /tmp/jp-isolated/.local/share/JobPrep/JobPrep
QSettings fileName: /tmp/jp-isolated/.config/JobPrep/JobPrep.conf
```
Verified that on Linux, `HOME`, `XDG_DATA_HOME`, and `XDG_CONFIG_HOME` completely redirect both the SQLite database path and the `QSettings` configuration path.

---

### 11.2 Status Section & Internal Jargon Removal

#### Real Milestone Mapping from `docs/SPEC.md`
```bash
$ grep -n -E "^\*\*M[0-9]" docs/SPEC.md
342:**M0 — Scaffold & shell.**
347:**M1 — Data layer.**
352:**M2 — Jobs page.**
356:**M3 — Calendar & interviews.**
361:**M4 — Study page.**
365:**M5 — Dashboard & reminders.**
369:**M6 — Polish & data tools.**
375:**M7 — Localization, Compatibility & UI Quality (Completed).**
```
The previous milestone checklist has been removed in favor of a concise public status summary:
> "JobPrep is fully implemented and verified across both Linux and Windows. Core functionality across all five pages (Dashboard, Study, Job Applications, Calendar, Settings) is complete, covered by 35 test suites, and localized in English, Ukrainian, and Russian."

Internal references such as "(Option B)" have been stripped from the Known Limitations section:
> "- **Keyboard Accelerators**: Custom mnemonic accelerators are not defined for Ukrainian and Russian; standard dialog buttons use Qt's built-in translations, and field navigation uses Tab, Shift+Tab, Enter, and Esc."

---

### 11.3 Claims Evidence

#### 1. Telemetry and Network Sockets
```bash
$ grep -rn "QNetwork\|QTcpSocket\|QUrl\|QDesktopServices" src CMakeLists.txt
src/ui/widgets/TopicDetailPanel.h:19:class QUrl;
src/ui/widgets/TopicDetailPanel.h:39:    /// Test seam: the app passes an opener around QDesktopServices::openUrl.
src/ui/widgets/TopicDetailPanel.h:40:    using OpenUrlOpener = std::function<bool(const QUrl&)>;
src/ui/widgets/TopicDetailPanel.cpp:7:#include <QDesktopServices>
src/ui/widgets/TopicDetailPanel.cpp:23:#include <QUrl>
src/ui/widgets/TopicDetailPanel.cpp:51:bool isHttpOrHttps(const QUrl& url) {
src/ui/widgets/TopicDetailPanel.cpp:67:        m_opener = [](const QUrl& url) { return QDesktopServices::openUrl(url); };
src/ui/widgets/TopicDetailPanel.cpp:871:        const QUrl parsed(url, QUrl::StrictMode);
src/ui/widgets/TopicDetailPanel.cpp:892:    const QUrl url(text, QUrl::StrictMode);
src/ui/DataToolsController.h:9:#include <QUrl>
src/ui/DataToolsController.h:70:    using UrlOpener = std::function<bool(const QUrl&)>;
src/ui/DataToolsController.cpp:4:#include <QDesktopServices>
src/ui/DataToolsController.cpp:392:    const QUrl url = QUrl::fromLocalFile(folder);
src/ui/DataToolsController.cpp:393:    const bool ok = m_urlOpener ? m_urlOpener(url) : QDesktopServices::openUrl(url);
src/ui/pages/JobsPage.cpp:4:#include <QDesktopServices>
src/ui/pages/JobsPage.cpp:19:#include <QUrl>
src/ui/pages/JobsPage.cpp:56:    const QUrl parsed{url.toString()};
src/ui/pages/JobsPage.cpp:274:        QDesktopServices::openUrl(QUrl(stored->url));

$ grep -n "Network" CMakeLists.txt || echo "(no output)"
(no output)
```
**Verdict:** Zero Qt Network modules, zero sockets, zero telemetry. `QUrl` and `QDesktopServices::openUrl` are used strictly to launch user-clicked external study links and job URLs in the system web browser, or to open the local folder in the file manager. Clarified in the README.

#### 2. CSV Export 22 Columns Header Row
```bash
$ grep -n -A 10 "QStringList ExportService::headers()" src/services/ExportService.cpp
55:QStringList ExportService::headers() {
56:    return {u"Company"_s,          u"Position"_s,      u"Status"_s,       u"Applied"_s,
57:            u"Next step"_s,        u"Salary"_s,        u"Source"_s,       u"Updated"_s,
58:            u"URL"_s,              u"Resume version"_s, u"Location"_s,    u"Work mode"_s,
59:            u"Salary min"_s,       u"Salary max"_s,    u"Currency"_s,     u"Next action"_s,
60:            u"Next action date"_s, u"Contact name"_s,  u"Contact email"_s, u"Notes"_s,
61:            u"Created at"_s,       u"Updated at"_s};
62:}
```
Header row explicitly lists all 22 standard columns in order.

#### 3. Reminder Lead Times and Service Scope
```bash
$ grep -n "m_leadMinutes" src/services/ReminderService.h
93:    QList<int> m_leadMinutes{1440, 60};
```
Default lead times in `ReminderService` and `SettingsService` are `{1440, 60}` (24 hours and 1 hour).
`ReminderService::computePlan` evaluates:
- Upcoming interviews (with configurable lead times)
- Applications with follow-up actions due today
- Applications with overdue follow-up actions
Study target dates appear on the Calendar agenda but do not generate desktop notifications.

#### 4. Accent Color Presets & Runtime Switching
```bash
$ grep -n -A 8 "enum class AccentPreset" src/ui/theme/Tokens.h
17:enum class AccentPreset {
18-    Indigo,
19-    Teal,
20-    Rose,
21-    Amber,
22-    Emerald,
23-    Sky,
24-};
```
Verified in `tst_UiSmoke::testThemeAndAccentSwitching` where all 6 presets and Light/Dark themes cycle dynamically without restarting the application.

#### 5. Build Requirements
```bash
$ grep -n "cmake_minimum_required" CMakeLists.txt && grep -n "CMAKE_CXX_STANDARD" CMakeLists.txt
1:cmake_minimum_required(VERSION 3.21)
10:set(CMAKE_CXX_STANDARD 20)
```
- CMake >= 3.21, C++20 standard required.
- CI compilers: GCC on `ubuntu-latest` and MSVC 2022 on `windows-latest`.

#### 6. The Four CI Job Names
From `.github/workflows/ci.yml`:
- `Gate (ubuntu-latest, Qt 6.8.2)`
- `Gate (ubuntu-latest, Qt 6.10.3)`
- `Gate (windows-latest, Qt 6.10.3)`
- `Gate (windows-latest, Qt 6.8.2 compat)`

#### 7. Translation String Count
The hardcoded "506" string count was replaced with "all UI strings" in `README.md` to prevent stale documentation as messages evolve.

#### 8. Windows Deployment Recipe & CI Check
```bash
$ grep -n "windeployqt" .github/workflows/ci.yml scripts -r || echo "(no output)"
(no output)
```
`windeployqt` is not part of the automated CI workflow. The recipe in `README.md` has been clarified as the maintainer's deployment procedure:
```cmd
:: Release build using Ninja
cmake -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --target jobprep

:: Bundle runtime dependencies with windeployqt
mkdir deploy && copy build\release\jobprep.exe deploy\
windeployqt --release deploy\jobprep.exe
```
Because `CMakePresets.json` defines `dev-windows` with `"CMAKE_BUILD_TYPE": "Debug"`, the recipe uses an explicit Release build directory (`-B build/release -DCMAKE_BUILD_TYPE=Release`).

---

### 11.4 Final Working Tree State & Commit Sequence

#### Working Tree Status:
```bash
$ git status --short
(no output)
```

#### Git Worktree:
```bash
$ git worktree list
/home/tony/projects/JobPrep 79dc311 [main]
```

#### Commit Sequence for this Task:
The task was performed across distinct logical commits in conventional style:
1. `658f5c5` — `docs(images): add application screenshots for README`
2. `05a5e6b` — `docs: write comprehensive GitHub README`
3. `578e720` — `docs(running): replace absolute file URI with relative link`
4. `6a274a0` — `docs(reports): add README task verification and hygiene report`
5. `71d64dd` — `docs(readme): add contributing section` (branch: `archive/dev-docs`)
6. `b3ae01f` — `chore(docs): move dev prompts and plans to archive branch`
7. `9634205` — `docs(readme): fix Windows data paths, isolation guidance, and status section`
8. `e2b4851` — `docs(running): correct Windows AppData paths and remove unsafe isolation snippet`
9. `79dc311` — `docs(reports): update README task report with fix-up audit and untrimmed logs`
