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

**Test Suites by Area (35 total):**
- **Data Layer (9):** `tst_Database`, `tst_TrackRepository`, `tst_TopicRepository`, `tst_SessionRepository`, `tst_ApplicationRepository`, `tst_InterviewRepository`, `tst_ReminderLogRepository`, `tst_SeedService`, `tst_TransactionBehavior`
- **Models & Proxies (6):** `tst_MonthView`, `tst_AgendaListModel`, `tst_ApplicationTableModel`, `tst_ApplicationFilterProxy`, `tst_TopicListModel`, `tst_TopicBoardProxy`, `tst_TopicTableProxy`
- **Services (7):** `tst_AgendaService`, `tst_DayWatcher`, `tst_ExportService`, `tst_ImportExport`, `tst_ReminderService`, `tst_SettingsService`, `tst_StatsService`
- **UI & Presentation (12):** `ui_smoke`, `tst_Notifiers`, `tst_StudyRegression`, `tst_PageJobs`, `tst_PageCalendar`, `tst_PageStudy`, `tst_PageHome`, `tst_PageSettings`, `tst_Shortcuts`, `tst_EmptyStatesAndFocus`, `tst_HiDpi`
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
- **Windows (derived from code, not verified on real PC by agent):** `%LOCALAPPDATA%\JobPrep\JobPrep\jobprep.sqlite`
- **Windows Config (derived from code):** Registry `HKEY_CURRENT_USER\Software\JobPrep\JobPrep`
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
> **Notice:** No licence file currently exists in the repository. As directed by section 1.6 of the specification, no licence file was invented or added. A question is flagged for the user in Section 6.

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

### Raw Output & Timings:
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
...
[240/240] Linking CXX executable tests/i18n/tst_Translations
Test project /tmp/jobprep-clean-test/build/dev-linux
      Start  1: tst_Database
      Start  2: tst_TrackRepository
 1/35 Test  #2: tst_TrackRepository ..............   Passed    0.05 sec
...
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
```bash
$ git ls-files docs | wc -l
204
```
**Breakdown of docs directories:**
- `docs/`: 4 root documents (`RUNNING.md`, `SPEC.md`, `i18n-glossary.md`, `i18n-glossary-raw.tsv`)
- `docs/images/`: 6 public screenshots referenced by `README.md`
- `docs/plans/`: 9 milestone planning documents (`M1-plan.md` through `M7-plan.md`)
- `docs/prompts/`: 72 working prompt notes and amendments
- `docs/reports/`: 49 milestone completion and verification reports
- `docs/reports/img/`: 26 historical report images

> **Note for user:** `docs/prompts/`, `docs/plans/`, and `docs/reports/` hold detailed internal engineering records and instructions from the development milestones. If the repository is intended for a lean public showcase, the user may consider archiving or moving `docs/prompts/` and `docs/plans/` out of the primary tree.

### 5.2 Private-Looking Content Audit
```bash
$ git grep -n "/home/\|C:\\\\Users\|@gmail\|@users.noreply" -- . ':!docs/reports'
docs/RUNNING.md:180:See [SPEC.md §3.7](file:///home/tony/projects/JobPrep/docs/SPEC.md#37-global) for full shortcut specifications and focus handling rules.
docs/prompts/M4-partC-fixups.md:13:4. No absolute local paths or `file:///home/...` links in committed docs. Use repo-relative paths.
docs/prompts/M6-ci-fixup5.md:21:1. **Evidence.** `git log --oneline -8`; `grep -rn '"/tmp\|u"/tmp\|"/home/\|u"/home/' tests src`; `grep -rn "QTemporaryFile" tests`; `grep -rln "QSettings\|SettingsService\|AppContext" tests`; and for each file that constructs a default-constructed `QSettings`, `SettingsService()` (no argument) or an `AppContext` outside the UI fixture: which one sets organization/application names and which does not (a short table, from commands).
docs/prompts/M7-partB-fixup2-windows-styles.md:36:   a. §3 "whole project `-Wshadow`": the count command reads `/home/tony/.gemini/antigravity-cli/brain/.../output.txt`, a tool-internal file that nobody else can reproduce, and the sentence "all 227 build steps compiled and linked with exit code 0" is hand-written. Redo: `cmake -B /tmp/jobprep-shadow ...` and `ninja -C /tmp/jobprep-shadow -j 2 2>&1 | tee /tmp/jobprep-shadow.log; echo "exit=${PIPESTATUS[0]}"` (raw), then `wc -l /tmp/jobprep-shadow.log`, `grep -c "Wshadow" /tmp/jobprep-shadow.log`, `grep -c "warning:" /tmp/jobprep-shadow.log` (raw). Use `-DJOBPREP_WERROR=OFF` and say whether `-Wshadow` was already in the flags (it is now permanent, so this run also proves the committed flag).
```
- **Hit in `docs/RUNNING.md:180`**: Resolved in commit `578e720` (fixed to `SPEC.md#37-global`).
- **Hits in `docs/prompts/`**: Working prompt instructions instructing agents not to leak paths. Harmless in prompt notes.

Counting total hits across the repository (including `docs/reports/`):
```bash
$ git grep -n "/home/\|C:\\\\Users\|@gmail\|@users.noreply" -- . | wc -l
189
```
**Verdict:** The 185 occurrences inside `docs/reports/` are historical compilation logs and command transcript proofs capturing the development environment path (`/home/tony/...` and Windows CI `C:\Users\...`). They contain no API credentials or private data.

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

$ grep -i -l -E "gemini|chatgpt|copilot|claude|antigravity" docs/prompts/*
docs/prompts/M7-partB-fixup2-windows-styles.md
docs/prompts/PROMPT.md
```
- `AGENTS.md` does not name any AI assistant or product.
- Only two prompt files in `docs/prompts/` reference internal tool paths (`antigravity-cli`).

---

## 6. Allowed Edits Made Outside README

- **File Modified:** `docs/RUNNING.md`
- **Change:** Fixed absolute link URI on line 180:
  ```diff
  -See [SPEC.md §3.7](file:///home/tony/projects/JobPrep/docs/SPEC.md#37-global) for full shortcut specifications and focus handling rules.
  +See [SPEC.md §3.7](SPEC.md#37-global) for full shortcut specifications and focus handling rules.
  ```
  Verified by `git diff docs/RUNNING.md`.

---

## 7. Quality Gate Execution on Working Tree

Ran full quality check on current tree with `CMAKE_BUILD_PARALLEL_LEVEL=2`:
```bash
$ CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.7s)
-- Generating done (1.0s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
ninja: no work to do.
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start 30: tst_PageHome
      Start 27: tst_PageJobs
 1/35 Test #27: tst_PageJobs .....................   Passed   15.62 sec
      Start 34: tst_HiDpi
 2/35 Test #30: tst_PageHome .....................   Passed   19.93 sec
      Start 28: tst_PageCalendar
 3/35 Test #28: tst_PageCalendar .................   Passed    8.68 sec
      Start 29: tst_PageStudy
 4/35 Test #34: tst_HiDpi ........................   Passed   21.59 sec
      Start 33: tst_EmptyStatesAndFocus
 5/35 Test #33: tst_EmptyStatesAndFocus ..........   Passed   12.48 sec
      Start 32: tst_Shortcuts
 6/35 Test #29: tst_PageStudy ....................   Passed   22.40 sec
      Start 25: tst_Notifiers
 7/35 Test #25: tst_Notifiers ....................   Passed    2.80 sec
      Start 26: tst_StudyRegression
 8/35 Test #32: tst_Shortcuts ....................   Passed    8.71 sec
      Start 31: tst_PageSettings
 9/35 Test #31: tst_PageSettings .................   Passed    5.70 sec
      Start 10: ui_smoke
10/35 Test #26: tst_StudyRegression ..............   Passed   11.66 sec
      Start 24: tst_StatsService
11/35 Test #24: tst_StatsService .................   Passed    1.34 sec
      Start 21: tst_ImportExport
12/35 Test #21: tst_ImportExport .................   Passed    0.86 sec
      Start  8: tst_SeedService
13/35 Test  #8: tst_SeedService ..................   Passed    0.40 sec
      Start 23: tst_SettingsService
14/35 Test #23: tst_SettingsService ..............   Passed    0.07 sec
      Start 22: tst_ReminderService
15/35 Test #22: tst_ReminderService ..............   Passed    0.28 sec
      Start 17: tst_TopicTableProxy
16/35 Test #17: tst_TopicTableProxy ..............   Passed    0.22 sec
      Start 35: tst_Translations
17/35 Test #35: tst_Translations .................   Passed    0.24 sec
      Start 18: tst_AgendaService
18/35 Test #18: tst_AgendaService ................   Passed    0.15 sec
      Start  5: tst_ApplicationRepository
19/35 Test  #5: tst_ApplicationRepository ........   Passed    0.21 sec
      Start 15: tst_TopicListModel
20/35 Test #15: tst_TopicListModel ...............   Passed    0.15 sec
      Start 12: tst_AgendaListModel
21/35 Test #12: tst_AgendaListModel ..............   Passed    0.23 sec
      Start 13: tst_ApplicationTableModel
22/35 Test #13: tst_ApplicationTableModel ........   Passed    0.26 sec
      Start  6: tst_InterviewRepository
23/35 Test  #6: tst_InterviewRepository ..........   Passed    0.26 sec
      Start  3: tst_TopicRepository
24/35 Test  #3: tst_TopicRepository ..............   Passed    0.23 sec
      Start 19: tst_DayWatcher
25/35 Test #19: tst_DayWatcher ...................   Passed    0.15 sec
      Start 16: tst_TopicBoardProxy
26/35 Test #16: tst_TopicBoardProxy ..............   Passed    0.27 sec
      Start 11: tst_MonthView
27/35 Test #11: tst_MonthView ....................   Passed    0.32 sec
      Start  4: tst_SessionRepository
28/35 Test  #4: tst_SessionRepository ............   Passed    0.23 sec
      Start 14: tst_ApplicationFilterProxy
29/35 Test #14: tst_ApplicationFilterProxy .......   Passed    0.23 sec
      Start  2: tst_TrackRepository
30/35 Test  #2: tst_TrackRepository ..............   Passed    0.16 sec
      Start  7: tst_ReminderLogRepository
31/35 Test  #7: tst_ReminderLogRepository ........   Passed    0.33 sec
      Start  9: tst_TransactionBehavior
32/35 Test  #9: tst_TransactionBehavior ..........   Passed    0.25 sec
      Start  1: tst_Database
33/35 Test  #1: tst_Database .....................   Passed    0.25 sec
      Start 20: tst_ExportService
34/35 Test #20: tst_ExportService ................   Passed    0.04 sec
35/35 Test #10: ui_smoke .........................   Passed   14.09 sec

100% tests passed, 0 tests failed out of 35

Label Time Summary:
data       =   2.34 sec*proc (9 tests)
i18n       =   0.24 sec*proc (1 test)
model      =   1.36 sec*proc (6 tests)
service    =   2.90 sec*proc (7 tests)
ui         = 144.00 sec*proc (12 tests)

Total Test time (real) =  78.29 sec
==> lupdate not found, skipping translation sync check.
==> Quality check PASSED with zero warnings and all tests green!
```

---

## 8. Not Verified by Agent (User to Check)

1. **Windows Build on Physical Windows PC:** The Windows configure line, MSVC paths, and `windeployqt` deployment command were cross-checked against `.github/workflows/ci.yml` and `docs/RUNNING.md`. They are verified in CI offscreen, but not on a live Windows machine by this agent.
2. **Screenshots vs. Real Screen:** The screenshots were captured headlessly via the Qt offscreen platform plugin using DejaVu Sans font. When the user is at his Windows PC, he may replace them with live desktop captures if desired.
3. **GitHub Badges:** The workflow badge URL points to `TonyTryHard/JobPrep/actions/workflows/ci.yml/badge.svg`. It will activate live on GitHub once these commits are pushed.
4. **Tone and Presentation:** The README text was composed without marketing superlatives or decorative emojis, adhering to a factual engineering tone.

---

## 9. Questions for User Decision

1. **Licence Selection:** No licence file currently exists in the repository. Which licence would you like to apply (e.g. MIT, Apache 2.0, GPLv3, or Proprietary / All rights reserved)?
2. **Contributing Guidelines:** Should the repository invite community contributions and issue reports, or remain explicitly a personal showcase repository?
3. **Working Documents Visibility:** The repository currently has 72 prompt records in `docs/prompts/` and 9 plans in `docs/plans/`. Would you prefer to keep these in the repository for full auditability, or move them into a private branch/archive before public presentation?
4. **Agent Mentions in Prompts:** Two files in `docs/prompts/` mention `antigravity-cli`. Do you wish to redact or keep them as historical working artifacts?

---

## 10. Git Status and Worktree State

```bash
$ git worktree list
/home/tony/projects/JobPrep 578e720 [main]
```

### Git Commit Log for this Task:
```
658f5c5 docs(images): add application screenshots for README
05a5e6b docs: write comprehensive GitHub README
578e720 docs(running): replace absolute file URI with relative link
```
