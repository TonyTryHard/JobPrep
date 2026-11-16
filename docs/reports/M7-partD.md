# M7 Part D Report: Real Ukrainian & Russian Translations, Strict Quality Gate, and Localization Review

**Date:** 2026-11-14  
**Status:** Complete  
**Platform / Environment:** Linux x86_64 (Ubuntu 26.04), GCC 15.2.0, Qt 6.10.2 shared, CMake 3.31.6, Ninja 1.13.1  
**Build Configuration:** `dev-linux` preset (`-g -std=c++20 -Wall -Wextra -Wpedantic -Wshadow -Werror -DJOBPREP_WERROR=ON`)  
**Parallelism:** `CMAKE_BUILD_PARALLEL_LEVEL=2`  

---

## 1. Facts as Given & Decisions

- **Baseline:** Part C accepted with D0 carry-overs. CI green on all three jobs after Part B.
- **Glossary & Mnemonics:** Section 1 of `docs/i18n-glossary.md` was provisional; in Part D all terms were applied and marked as "Applied". Option B (no custom `&` accelerators in `uk`/`ru`) was adopted to prevent layout collisions, untranslatable shortcuts on Latin layout, and shortcut collisions; standard dialog buttons (*OK*, *Cancel*, *Yes*, *No*) continue using Qt's built-in Cyrillic mnemonics via `qtbase_*.qm`.
- **Weekly Goal Behavior:** `7.5` produces 465 min because the typing rule (one digit = tens of minutes: 7:50) is rounded to the 15-minute step. That is the established behavior since M6, not a regression; all tests kept.
- **Binding Decisions:** Default language resolves to system UI language if `uk`/`ru`, otherwise English (`en`); setting values `system`/`en`/`uk`/`ru`; language switches after restart; sample data remains English; exports neutral and byte-identical across locales; native language names in selector.

---

## 2. D0. Carry-Overs from Part C Review

### D0.1 Dead Plural Fallback Strings

- **Issue:** English plural fallback called `tr("1 day")`, `tr("%1 days")`, etc. These strings were emitted into `.ts` catalogs as dead entries, causing unnecessary translation requirements.
- **Fix:** Replaced with literal helper `englishPlural(n, one, otherPattern)` in `FormatHelper.cpp` and `NotificationText.cpp`.
- **Raw commands & outputs before and after:**

Grep before (`d63daf1:src`):
```bash
git grep -rn "tr(\"1 \|tr(\"%1 " d63daf1:src
```
```
d63daf1:src:ui/FormatHelper.cpp:25:    return tr("%1 / %2").arg(formatDuration(weekMinutes), formatDuration(goalMinutes));
d63daf1:src:ui/FormatHelper.cpp:38:        return days == 1 ? tr("1 day") : tr("%1 days").arg(days);
d63daf1:src:ui/FormatHelper.cpp:46:        return hours == 1 ? tr("1 hour") : tr("%1 hours").arg(hours);
d63daf1:src:ui/FormatHelper.cpp:52:    return minutes == 1 ? tr("1 minute") : tr("%1 minutes").arg(minutes);
d63daf1:src:ui/FormatHelper.cpp:72:        return tr("1 active process");
d63daf1:src:ui/FormatHelper.cpp:74:    return tr("%1 active processes").arg(count);
d63daf1:src:ui/dialogs/FollowUpDialog.cpp:121:        tr("%1 already has a follow-up planned for %2. Replace it with %3?")
d63daf1:src:ui/pages/SettingsPage.cpp:334:    m_leadPresetCombo->addItem(tr("1 hour"), 60);
d63daf1:src:ui/pages/SettingsPage.cpp:336:    m_leadPresetCombo->addItem(tr("1 day"), 1440);
d63daf1:src:ui/widgets/GoalProgressBar.cpp:34:        tr("%1 of %2 (%3%)")
```

Grep after (current `src/`):
```bash
grep -rn "tr(\"1 \|tr(\"%1 " src
```
```
src/ui/widgets/GoalProgressBar.cpp:34:        tr("%1 of %2 (%3%)")
src/ui/dialogs/FollowUpDialog.cpp:121:        tr("%1 already has a follow-up planned for %2. Replace it with %3?")
src/ui/FormatHelper.cpp:34:    return tr("%1 / %2").arg(formatDuration(weekMinutes), formatDuration(goalMinutes));
src/ui/pages/SettingsPage.cpp:334:    m_leadPresetCombo->addItem(tr("1 hour"), 60);
src/ui/pages/SettingsPage.cpp:336:    m_leadPresetCombo->addItem(tr("1 day"), 1440);
```

Diff stat of commit `b1e4899`:
```bash
git show --stat b1e4899
```
```
 resources/translations/jobprep_ru.ts | 56 --------------------
 resources/translations/jobprep_uk.ts | 56 --------------------
 src/ui/FormatHelper.cpp              | 18 +++----
 src/ui/NotificationText.cpp          | 27 ++++------
 4 files changed, 23 insertions(+), 134 deletions(-)
```

Verification of translation sync:
```bash
./scripts/check-translations.sh
```
```
==> Checking translation sync with /usr/lib/qt6/bin/lupdate...
==> Translation files are in sync.
```

---

### D0.2 Default Locale Dependency Audit & English Isolation

- **Audit by command:**
  - `QDateEdit`/`QDateTimeEdit` display format: `SingleTabDateTime.h` explicitly sets `setDisplayFormat(QString::fromLatin1("yyyy-MM-dd"))` and syncs `calendarWidget()->setFirstDayOfWeek()` from user preference `calendar/firstDayOfWeek`.
  - `QDoubleSpinBox`: Checked via `grep -rn "QDoubleSpinBox" src tests` -> `(no output)` (no double spinboxes used).
  - `arg(double)`: `FormatHelper::formatInterviewRate` uses `QString::number(*rate * 100.0, 'f', 1)` which explicitly produces standard period decimal points without locale deviation.
  - `toString` without format: Date/time conversions throughout domain and data layers strictly use explicit ISO formatting (`DbFormat::toText(day)` -> `yyyy-MM-dd`).
- **Behavior comparison (`uk_UA`, `de_DE`, `en_US` system locales with `en` UI language):**
  - In commit `18ae6fa`, explicit display formats and first-day-of-week settings were enforced in `SingleTabDateEdit`.
  - Tested across `uk_UA`, `de_DE`, and `en_US` via `tst_Shortcuts::testDateEditDisplayFormatAndFirstDayOfWeek`.
- **Command & raw output:**
```bash
./build/dev-linux/tests/ui/tst_Shortcuts testDateEditDisplayFormatAndFirstDayOfWeek -o -,txt
```
```
********* Start testing of tst_Shortcuts *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_Shortcuts::initTestCase()
PASS   : tst_Shortcuts::testDateEditDisplayFormatAndFirstDayOfWeek()
PASS   : tst_Shortcuts::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 317ms
********* Finished testing of tst_Shortcuts *********
```

---

### D0.3 Language Setting Key Unified in `main.cpp` & `SettingsService`

- **Implementation:**
  - `SettingsService::kLanguageKey` (`QString::fromLatin1("ui/language")`) is exposed publicly.
  - `SettingsService::readLanguageSetting()` reads the setting with standard fallback to `"system"`.
  - `main.cpp` calls `SettingsService::readLanguageSetting()` strictly **after** `setOrganizationName` and `setApplicationName`.
- **Test:** `tst_SettingsService::testLanguageSettingSharedKeyAndScope` and `tst_SettingsService::testMainCallsInstallLanguageBeforeWidgets`.
- **Command & raw output:**
```bash
./build/dev-linux/tests/services/tst_SettingsService testLanguageSettingSharedKeyAndScope -o -,txt
```
```
********* Start testing of tst_SettingsService *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_SettingsService::initTestCase()
PASS   : tst_SettingsService::testLanguageSettingSharedKeyAndScope()
PASS   : tst_SettingsService::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 1ms
********* Finished testing of tst_SettingsService *********
```

---

### D0.4 Sync Check on CI

- **`lupdate` discovery in `scripts/check-translations.sh`:**
  - Checks `${QT_PREFIX_PATH}/bin/lupdate`, `command -v lupdate`, `/usr/lib/qt6/bin/lupdate`, and `/var/tmp/qt682/6.8.2/gcc_64/bin/lupdate`.
  - If `lupdate` is missing, prints `==> lupdate not found, skipping translation sync check.` and exits with code 0 (does not break CI).
  - Version tolerance: If `lupdate` minor version differs from `JOBPREP_TS_QT_MINOR` (default `10`), diff is printed with `report only` and exits with code 0.
  - **Enforcing CI job:** Only `Gate (ubuntu-latest, Qt 6.10.3)` enforces translation sync strictly (exits 1 on uncommitted diff). `Gate (ubuntu-latest, Qt 6.8.2)` reports only. Windows CI runs `check.ps1` and does not run `check-translations.sh`.

---

### D0.5 Report Hygiene

- Reviewer corrections appended to `docs/reports/M7-partC.md` under Section "Reviewer corrections (Part D, D0)" in commit `3b2e6b5`.
- All outputs in Part D include full headers, `initTestCase`/`cleanupTestCase`, exact executed commands, and raw results.

---

## 3. D1. The Ukrainian and Russian Translations

Authored complete, natural Ukrainian (`resources/translations/jobprep_uk.ts`) and Russian (`resources/translations/jobprep_ru.ts`) catalogs for all UI messages.

### Message Counts by Command

Total messages:
```bash
grep -c '<message' resources/translations/jobprep_uk.ts && grep -c '<message' resources/translations/jobprep_ru.ts
```
```
506
506
```

Unfinished messages:
```bash
grep -c 'type="unfinished"' resources/translations/jobprep_uk.ts || echo "0" ; grep -c 'type="unfinished"' resources/translations/jobprep_ru.ts || echo "0"
```
```
0
0
0
0
```

Plural (`numerus="yes"`) messages:
```bash
grep -c 'numerus="yes"' resources/translations/jobprep_uk.ts && grep -c 'numerus="yes"' resources/translations/jobprep_ru.ts
```
```
12
12
```

Plural forms (`<numerusform>`):
```bash
grep -c '<numerusform>' resources/translations/jobprep_uk.ts && grep -c '<numerusform>' resources/translations/jobprep_ru.ts
```
```
36
36
```
(Exactly 3 forms for all 12 plural messages in both catalogs: 12 * 3 = 36).

Empty translations:
```bash
grep -c '<translation></translation>' resources/translations/jobprep_uk.ts || echo "0"; grep -c '<translation></translation>' resources/translations/jobprep_ru.ts || echo "0"
```
```
0
0
0
0
```

### Clean `lrelease` Compilation (Zero Warnings)

Command:
```bash
/usr/lib/qt6/bin/lrelease resources/translations/jobprep_uk.ts resources/translations/jobprep_ru.ts
```
Raw Output:
```
Updating 'resources/translations/jobprep_uk.qm'...
    Generated 506 translation(s) (506 finished and 0 unfinished)
Updating 'resources/translations/jobprep_ru.qm'...
    Generated 506 translation(s) (506 finished and 0 unfinished)
```

---

## 4. D2. Strict Translation Verification (`tst_Translations`)

1. **Completeness enforcement:** Enabled by default in `tests/i18n/CMakeLists.txt` via `ENVIRONMENT "QT_QPA_PLATFORM=offscreen;JOBPREP_REQUIRE_COMPLETE_TRANSLATIONS=1"`.
2. **Broken fixture negative controls:** Validated against broken fixtures in `tests/i18n/fixtures/`:
   - `broken_unfinished_uk.ts` -> Unfinished message detected
   - `broken_empty_uk.ts` -> Empty translation tag detected
   - `broken_numerus_uk.ts` -> Wrong numerus form count (2 instead of 3) detected
   - `broken_placeholders_uk.ts` -> Missing placeholder `%2` detected
   - `broken_html_uk.ts` -> Missing HTML tag `<b>` detected
   - `broken_accelerator_uk.ts` -> Forbidden Option B `&` accelerator detected
   - `broken_leftover_uk.ts` -> Leftover English plural helper string detected
3. **Plural Table Verification:** Table-driven test across numbers `1, 2, 5, 11, 21, 22, 25, 101` in `tst_Translations::testRealCatalogsPluralTable`.

### Plural Rendering Output Table

#### Ukrainian (`uk`) Plural Renderings:
| Number | `formatLeadTime` (Days) | `formatLeadTime` (Hours) | `formatLeadTime` (Minutes) | `formatActiveProcesses` | `NotificationText` (Hours) | `NotificationText` (Minutes) | `OverdueFollowUps` | `JobsPage` Applications |
| :---: | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **1** | 1 день | 1 година | 1 хвилина | 1 активний процес | через 1 годину | через 1 хвилину | 1 прострочений фолоу-ап | 1 заявка |
| **2** | 2 дні | 2 години | 2 хвилини | 2 активні процеси | через 2 години | через 2 хвилини | 2 прострочені фолоу-апи | 2 заявки |
| **5** | 5 днів | 5 годин | 5 хвилин | 5 активних процесів | через 5 годин | через 5 хвилин | 5 прострочених фолоу-апів | 5 заявок |
| **11** | 11 днів | 11 годин | 11 хвилин | 11 активних процесів | через 11 годин | через 11 хвилин | 11 прострочених фолоу-апів | 11 заявок |
| **21** | 21 день | 21 година | 21 хвилина | 21 активний процес | через 21 годину | через 21 хвилину | 21 прострочений фолоу-ап | 21 заявка |
| **22** | 22 дні | 22 години | 22 хвилини | 22 активні процеси | через 22 години | через 22 хвилини | 22 прострочені фолоу-апи | 22 заявки |
| **25** | 25 днів | 25 годин | 25 хвилин | 25 активних процесів | через 25 годин | через 25 хвилин | 25 прострочених фолоу-апів | 25 заявок |
| **101** | 101 день | 101 година | 101 хвилина | 101 активний процес | через 101 годину | через 101 хвилину | 101 прострочений фолоу-ап | 101 заявка |

#### Russian (`ru`) Plural Renderings:
| Number | `formatLeadTime` (Days) | `formatLeadTime` (Hours) | `formatLeadTime` (Minutes) | `formatActiveProcesses` | `NotificationText` (Hours) | `NotificationText` (Minutes) | `OverdueFollowUps` | `JobsPage` Applications |
| :---: | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **1** | 1 день | 1 час | 1 минута | 1 активный процесс | через 1 час | через 1 минуту | 1 просроченный фоллоу-ап | 1 заявка |
| **2** | 2 дня | 2 часа | 2 минуты | 2 активных процесса | через 2 часа | через 2 минуты | 2 просроченных фоллоу-апа | 2 заявки |
| **5** | 5 дней | 5 часов | 5 минут | 5 активных процессов | через 5 часов | через 5 минут | 5 просроченных фоллоу-апов | 5 заявок |
| **11** | 11 дней | 11 часов | 11 минут | 11 активных процессов | через 11 часов | через 11 минут | 11 просроченных фоллоу-апов | 11 заявок |
| **21** | 21 день | 21 час | 21 минута | 21 активный процесс | через 21 час | через 21 минуту | 21 просроченный фоллоу-ап | 21 заявка |
| **22** | 22 дня | 22 часа | 22 минуты | 22 активных процесса | через 22 часа | через 22 минуты | 22 просроченных фоллоу-апа | 22 заявки |
| **25** | 25 дней | 25 часов | 25 минут | 25 активных процессов | через 25 часов | через 25 минут | 25 просроченных фоллоу-апов | 25 заявок |
| **101** | 101 день | 101 час | 101 минута | 101 активный процесс | через 101 час | через 101 минуту | 101 просроченный фоллоу-ап | 101 заявка |

### Full `tst_Translations` Execution

Command:
```bash
./build/dev-linux/tests/i18n/tst_Translations -o -,txt
```
Raw Output:
```
********* Start testing of tst_Translations *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_Translations::initTestCase()
PASS   : tst_Translations::testTsParsingAndCompletenessFixtures()
PASS   : tst_Translations::testRealTsFilesCompleteness()
PASS   : tst_Translations::testRealTsFilesSyncWithSource()
PASS   : tst_Translations::testPluralMechanismUkrainian()
PASS   : tst_Translations::testPluralMechanismRussian()
PASS   : tst_Translations::testEmbeddedQmFilesExist()
PASS   : tst_Translations::testMissingQtBaseNoWarningNoCrash()
PASS   : tst_Translations::testEnglishFallbackAndPseudoTranslator()
PASS   : tst_Translations::testRealCatalogsPluralTable()
PASS   : tst_Translations::cleanupTestCase()
Totals: 11 passed, 0 failed, 0 skipped, 0 blacklisted, 25ms
********* Finished testing of tst_Translations *********
```

---

## 5. D3. Layout Verification under Real Translations

Executed `ui_smoke` and `tst_HiDpi` with active Ukrainian and Russian catalogs under Light and Dark themes, 100% and 150% scaling, across standard (1100x700) and compact (1000x540) window sizes:

- **Initial Run Hits:** Zero clipping violations, zero ink overlap violations, zero warnings.
- **Execution Durations (reported from `-o -,xunitxml`):**
  - `tst_UiSmoke::testRealTranslationUkSmoke`: 2.838s
  - `tst_UiSmoke::testRealTranslationRuSmoke`: 2.891s
  - `tst_HiDpi::testRealTranslationUkHiDpi`: 3.032s
  - `tst_HiDpi::testRealTranslationRuHiDpi`: 2.966s

---

## 6. D4. Localization Review Materials

1. **Sentence Review Document (`docs/reports/i18n-review.md`):**
   - Rows: 188 non-trivial strings cataloged across Home (10), Study (22), Jobs (8), Calendar (6), Settings (61), Dialogs (52), Notifications (7), General/Shell (22).
   - Command:
     ```bash
     grep -c '^| ' docs/reports/i18n-review.md
     ```
     Raw count: `221` (including summary table and header rows; exactly 188 string rows).
2. **Visual Artifacts (`docs/reports/images/i18n/`):**
   - 20 PNG screenshots capturing all 5 pages and 5 dialogs under `uk` and `ru` (Light, 100%).
   - Total directory size:
     ```bash
     du -sh docs/reports/images/i18n/
     ```
     Raw output: `1008K`
   - File listing:
     ```bash
     ls -la docs/reports/images/i18n/
     ```
     ```
     total 1012
     drwxrwxr-x 2 tony tony  4096 Nov 13 23:49 .
     drwxrwxr-x 3 tony tony  4096 Nov 13 23:49 ..
     -rw-rw-r-- 1 tony tony 76703 Nov 13 23:49 ru-calendar.png
     -rw-rw-r-- 1 tony tony 56058 Nov 13 23:49 ru-dialog-application.png
     -rw-rw-r-- 1 tony tony 13700 Nov 13 23:49 ru-dialog-followup.png
     -rw-rw-r-- 1 tony tony 28804 Nov 13 23:49 ru-dialog-interview.png
     -rw-rw-r-- 1 tony tony 15170 Nov 13 23:49 ru-dialog-session.png
     -rw-rw-r-- 1 tony tony 18288 Nov 13 23:49 ru-dialog-track.png
     -rw-rw-r-- 1 tony tony 84285 Nov 13 23:49 ru-home.png
     -rw-rw-r-- 1 tony tony 53876 Nov 13 23:49 ru-jobs.png
     -rw-rw-r-- 1 tony tony 60768 Nov 13 23:49 ru-settings.png
     -rw-rw-r-- 1 tony tony 98746 Nov 13 23:49 ru-study.png
     -rw-rw-r-- 1 tony tony 73341 Nov 13 23:49 uk-calendar.png
     -rw-rw-r-- 1 tony tony 52165 Nov 13 23:49 uk-dialog-application.png
     -rw-rw-r-- 1 tony tony 13005 Nov 13 23:49 uk-dialog-followup.png
     -rw-rw-r-- 1 tony tony 27147 Nov 13 23:49 uk-dialog-interview.png
     -rw-rw-r-- 1 tony tony 14883 Nov 13 23:49 uk-dialog-session.png
     -rw-rw-r-- 1 tony tony 16811 Nov 13 23:49 uk-dialog-track.png
     -rw-rw-r-- 1 tony tony 79144 Nov 13 23:49 uk-home.png
     -rw-rw-r-- 1 tony tony 49601 Nov 13 23:49 uk-jobs.png
     -rw-rw-r-- 1 tony tony 57042 Nov 13 23:49 uk-settings.png
     -rw-rw-r-- 1 tony tony 94742 Nov 13 23:49 uk-study.png
     ```
   - *Notice:* Offscreen rendering is generated in a headless test environment and may differ subtly in font metrics from live native desktop screens.
3. **Glossary Update (`docs/i18n-glossary.md`):**
   - Section 1 rows updated to status `Applied`.
   - Open questions for user decision clearly highlighted (Option B accelerators vs Option A, polite plural address forms).

---

## 7. D5. Documentation Updates

- `docs/SPEC.md`: Continued numbering with entry `1.28` in §14 and updated M7 line in milestone status list.
- `docs/RUNNING.md`: Updated Section 11 detailing how to choose UI language, restart requirement, runtime location of `qtbase_*.qm`, and translation sync command.

---

## 8. CI Predictions

Labelled predictions for each CI matrix job:
1. `Gate (ubuntu-latest, Qt 6.8.2)`: **PASS (GREEN)** — Qt Linguist Tools available; `check-translations.sh` detects 6.8 minor version and runs in report-only mode; strict completeness tests pass.
2. `Gate (ubuntu-latest, Qt 6.10.3)`: **PASS (GREEN)** — Qt Linguist Tools available; `check-translations.sh` enforces exact translation catalog sync with zero diff; all 35 tests green.
3. `Gate (windows-latest, Qt 6.10.3)`: **PASS (GREEN)** — Windows offscreen theme warning filter active; tests pass with standard Windows font metrics; builds cleanly with MSVC.
*(Note: Windows Qt 6.8.2 runs only on manual dispatch or weekly schedule).*

---

## 9. Not Verified by Agent, User to Check

The following items cannot be fully verified in the headless Linux agent environment and require manual verification by the user:
1. **Translations quality:** Review `docs/reports/i18n-review.md` and `docs/i18n-glossary.md` to confirm Ukrainian and Russian wording matches preferences.
2. **Real screens under `uk` and `ru`:** Launch JobPrep on Windows PC and laptop at 100% and 150% scaling to inspect native font rendering and spacing.
3. **Language selector and restart on Windows:** Change language in Settings, verify restart notice, close, relaunch, and verify UI switches language properly.
4. **`qtbase_uk.qm` / `qtbase_ru.qm` presence on Windows:** Verify that standard dialog buttons (OK, Cancel) display Ukrainian / Russian text on Windows.
5. **Shortcuts under Ukrainian and Russian keyboard layouts:** Verify accelerators and global shortcuts (`Ctrl+S`, `Ctrl+N`) when Cyrillic keyboard layouts are active.
6. **Alt+Down and F4 in date controls:** Verify calendar popups open smoothly via keyboard in the 5 date editing locations on Windows.
7. **Dashboard "Today" rows:** Inspect home page agenda rows with live interview and study items under Ukrainian and Russian locales.

---

## 10. Verification Command & Full Raw Output

Command executed:
```bash
CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
```

Raw Output:
```
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (2.3s)
-- Generating done (3.6s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
ninja: no work to do.
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start 34: tst_HiDpi
      Start 29: tst_PageStudy
 1/35 Test #34: tst_HiDpi ........................   Passed   20.44 sec
      Start 30: tst_PageHome
 2/35 Test #29: tst_PageStudy ....................   Passed   21.66 sec
      Start 33: tst_EmptyStatesAndFocus
 3/35 Test #33: tst_EmptyStatesAndFocus ..........   Passed   10.73 sec
      Start 27: tst_PageJobs
 4/35 Test #30: tst_PageHome .....................   Passed   17.19 sec
      Start 32: tst_Shortcuts
 5/35 Test #32: tst_Shortcuts ....................   Passed    6.49 sec
      Start 28: tst_PageCalendar
 6/35 Test #27: tst_PageJobs .....................   Passed   12.89 sec
      Start 26: tst_StudyRegression
 7/35 Test #28: tst_PageCalendar .................   Passed    7.25 sec
      Start 10: ui_smoke
 8/35 Test #26: tst_StudyRegression ..............   Passed    7.24 sec
      Start 25: tst_Notifiers
 9/35 Test #25: tst_Notifiers ....................   Passed    2.57 sec
      Start 31: tst_PageSettings
10/35 Test #31: tst_PageSettings .................   Passed    4.79 sec
      Start 24: tst_StatsService
11/35 Test #24: tst_StatsService .................   Passed    1.06 sec
      Start 35: tst_Translations
12/35 Test #35: tst_Translations .................   Passed    0.25 sec
      Start 21: tst_ImportExport
13/35 Test #21: tst_ImportExport .................   Passed    0.82 sec
      Start 23: tst_SettingsService
14/35 Test #23: tst_SettingsService ..............   Passed    0.07 sec
      Start  8: tst_SeedService
15/35 Test  #8: tst_SeedService ..................   Passed    0.44 sec
      Start 17: tst_TopicTableProxy
16/35 Test #17: tst_TopicTableProxy ..............   Passed    0.12 sec
      Start 22: tst_ReminderService
17/35 Test #22: tst_ReminderService ..............   Passed    0.32 sec
      Start 18: tst_AgendaService
18/35 Test #18: tst_AgendaService ................   Passed    0.25 sec
      Start  3: tst_TopicRepository
19/35 Test  #3: tst_TopicRepository ..............   Passed    0.15 sec
      Start 16: tst_TopicBoardProxy
20/35 Test #16: tst_TopicBoardProxy ..............   Passed    0.23 sec
      Start  5: tst_ApplicationRepository
21/35 Test  #5: tst_ApplicationRepository ........   Passed    0.25 sec
      Start 15: tst_TopicListModel
22/35 Test #15: tst_TopicListModel ...............   Passed    0.17 sec
      Start 19: tst_DayWatcher
23/35 Test #10: ui_smoke .........................   Passed   12.94 sec
      Start 13: tst_ApplicationTableModel
24/35 Test #19: tst_DayWatcher ...................   Passed    0.43 sec
      Start 12: tst_AgendaListModel
25/35 Test #13: tst_ApplicationTableModel ........   Passed    0.46 sec
      Start  6: tst_InterviewRepository
26/35 Test #12: tst_AgendaListModel ..............   Passed    0.46 sec
      Start  9: tst_TransactionBehavior
27/35 Test  #6: tst_InterviewRepository ..........   Passed    0.46 sec
      Start  1: tst_Database
28/35 Test  #9: tst_TransactionBehavior ..........   Passed    0.29 sec
      Start  2: tst_TrackRepository
29/35 Test  #1: tst_Database .....................   Passed    0.26 sec
      Start  7: tst_ReminderLogRepository
30/35 Test  #2: tst_TrackRepository ..............   Passed    0.43 sec
      Start  4: tst_SessionRepository
31/35 Test  #7: tst_ReminderLogRepository ........   Passed    0.43 sec
      Start 14: tst_ApplicationFilterProxy
32/35 Test  #4: tst_SessionRepository ............   Passed    0.47 sec
      Start 11: tst_MonthView
33/35 Test #14: tst_ApplicationFilterProxy .......   Passed    0.50 sec
      Start 20: tst_ExportService
34/35 Test #11: tst_MonthView ....................   Passed    0.40 sec
35/35 Test #20: tst_ExportService ................   Passed    0.17 sec

100% tests passed, 0 tests failed out of 35

Label Time Summary:
data       =   3.20 sec*proc (9 tests)
i18n       =   0.25 sec*proc (1 test)
model      =   1.94 sec*proc (6 tests)
service    =   3.12 sec*proc (7 tests)
ui         = 124.59 sec*proc (12 tests)

Total Test time (real) =  66.63 sec
==> Checking translation sync with /usr/lib/qt6/bin/lupdate...
==> Translation files are in sync.
==> Quality check PASSED with zero warnings and all tests green!
```

---

## 11. Totals by Command

- CTest test count:
  ```bash
  ctest --preset dev-linux -N | grep -c "Test #"
  ```
  `35`
- Total messages per catalog:
  ```bash
  grep -c '<message' resources/translations/jobprep_uk.ts
  ```
  `506`
  ```bash
  grep -c '<message' resources/translations/jobprep_ru.ts
  ```
  `506`
- Finished messages per catalog:
  ```bash
  grep -c '<translation>' resources/translations/jobprep_uk.ts
  ```
  `494` (494 single + 12 numerus = 506 messages)
  ```bash
  grep -c '<translation>' resources/translations/jobprep_ru.ts
  ```
  `494` (494 single + 12 numerus = 506 messages)
- Numerus forms per catalog:
  ```bash
  grep -c '<numerusform>' resources/translations/jobprep_uk.ts
  ```
  `36`
  ```bash
  grep -c '<numerusform>' resources/translations/jobprep_ru.ts
  ```
  `36`
- Captured screenshots:
  ```bash
  ls -1 docs/reports/images/i18n/*.png | wc -l
  ```
  `20`

---

## 12. Working Tree & Worktrees

```bash
git status --short
```
`(no output)`

```bash
git worktree list
```
```
/home/tony/projects/JobPrep 95d4ceb [main]
```

---

## 13. Git Log of Part D

```bash
git log --oneline b1e4899~1..HEAD
```
```
d7db228 (HEAD -> main) docs(reports): add M7 milestone report and M7 Part D report
95d4ceb prompts for c & d steps of m7
370c66b docs(spec): add §14 entry 1.28 and update RUNNING guide for localization (D5)
979bc7a docs(i18n): add user translation review document, screenshots, and updated glossary (D4)
a5a9397 test(ui): add layout and smoke tests under real Ukrainian and Russian translations (D3)
a442c3d test(i18n): enable strict translation checks, broken fixture tests, and plural tables (D2)
2bed625 feat(i18n): author full Russian translation catalog (D1)
2b0affd feat(i18n): author full Ukrainian translation catalog (D1)
3b2e6b5 docs(reports): add reviewer corrections section to M7-partC report (D0.5)
a13a356 fix(services): unify language setting key and read function between main and service (D0.3)
18ae6fa fix(ui): enforce ISO date format and user first-day-of-week on date edits (D0.2)
b1e4899 fix(i18n): replace dead English plural fallback strings with literal helper (D0.1)
```


---

## 14. M7 Closeout: CI Red on Linux Fix, Translation Corrections, Small Leftovers

### Facts (CI run `118200252685` of Part C/D commits, 2026-11-15)

- **`Gate (windows-latest, Qt 6.10.3)`**: **green**, 35 of 35 tests passed, `Total Test time (real) = 533.21 sec`; the full "Run quality gate" step took ~14 minutes (within 30m timeout). Slowest tests: `tst_PageStudy` 150.9 s, `tst_HiDpi` 134.7 s, `tst_PageHome` 133.7 s, `ui_smoke` 108.8 s. Real translations (`uk`, `ru`) and pseudo-translation passed cleanly on Windows.
- **`Gate (ubuntu-latest, Qt 6.8.2)` and `Gate (ubuntu-latest, Qt 6.10.3)`**: **red**, with the identical single failure in `tst_HiDpi::testPseudoTranslationHiDpi` (`Actual (allViolations.count()): 8, Expected 0`, `tst_HiDpi.cpp(923)`):
  ```
  CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1246x700] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвг': width (151) < minimumSizeHint.width (158)
  CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1246x700] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абв': width (151) < minimumSizeHint.width (157)
  CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1246x540] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвг': width (151) < minimumSizeHint.width (158)
  CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1246x540] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абв': width (151) < minimumSizeHint.width (157)
  CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1246x700] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвг': width (151) < minimumSizeHint.width (158)
  CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1246x700] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абв': width (151) < minimumSizeHint.width (157)
  CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1246x540] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвг': width (151) < minimumSizeHint.width (158)
  CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1246x540] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абв': width (151) < minimumSizeHint.width (157)
  ```
- `testRealTranslationUkHiDpi` and `testRealTranslationRuHiDpi` **passed** on both Ubuntu jobs. The Ubuntu runner installs `fonts-dejavu-core` in the workflow step; fontconfig resolves sans-serif to `DejaVu Sans`. Because ctest failed, `check.sh` stopped before the translation-sync step; the sync check has not yet executed on CI.
- The Part D report stated "0 clipping violations" for pseudo-translation because it ran against the local default font (`Noto Sans`), where the pseudo strings happened to fit within 151 px.
- CI diagnostic verification: `lupdate`, `lrelease`, and `Qt6LinguistTools` exist in all three jobs; `qtbase_uk.qm` (158,274 bytes) and `qtbase_ru.qm` (203,767 bytes) exist in `<QT_PREFIX_PATH>/translations` across all three CI aqt kits (Ubuntu Qt 6.8.2, Ubuntu Qt 6.10.3, Windows Qt 6.10.3). Presence on the user's local Windows installation remains unverified.

---

### A. Make the CI-Font Failure Reproducible, Then Fix It

#### A.1 Check Producing the Clipping Violation Message

Command executed:
```bash
grep -n "CLIPPING_VIOLATION\|minimumSizeHint" tests/ui/tst_HiDpi.cpp tests/ui_test_helpers/*
```
Raw Output:
```
tests/ui/tst_HiDpi.cpp:449:        // 1. widget->width() >= widget->minimumSizeHint().width() and height
tests/ui/tst_HiDpi.cpp:450:        const QSize minHint = w->minimumSizeHint();
tests/ui/tst_HiDpi.cpp:457:                                   QString(u"width (%1) < minimumSizeHint.width (%2)"_s)
tests/ui/tst_HiDpi.cpp:466:                                   QString(u"height (%1) < minimumSizeHint.height (%2)"_s)
tests/ui/tst_HiDpi.cpp:565:                  win.minimumSizeHint().width(), win.minimumSizeHint().height());
tests/ui/tst_HiDpi.cpp:583:        fprintf(stderr, "CLIPPING_VIOLATION: [%s] %s (%s) '%s': %s\n",
tests/ui/tst_HiDpi.cpp:628:            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
tests/ui/tst_HiDpi.cpp:663:                          dlg.minimumSizeHint().width(), dlg.minimumSizeHint().height(),
tests/ui/tst_HiDpi.cpp:668:                          bar ? bar->minimumSizeHint().width() : -1, bar ? bar->minimumSizeHint().height() : -1,
tests/ui/tst_HiDpi.cpp:698:            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
tests/ui/tst_HiDpi.cpp:726:            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
tests/ui/tst_HiDpi.cpp:754:            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
tests/ui/tst_HiDpi.cpp:782:            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
tests/ui/tst_HiDpi.cpp:803:        fprintf(stderr, "CLIPPING_VIOLATION: [%s] %s (%s) '%s': %s\n",
tests/ui/tst_HiDpi.cpp:939:        fprintf(stderr, "CLIPPING_VIOLATION: [%s] %s (%s) '%s': %s\n",
tests/ui/tst_HiDpi.cpp:1053:        fprintf(stderr, "CLIPPING_VIOLATION: [%s] %s (%s) '%s': %s\n",
tests/ui/tst_HiDpi.cpp:1130:            const QSize msh = dlg.minimumSizeHint();
tests/ui/tst_HiDpi.cpp:1138:        QVERIFY2(dlg.minimumSizeHint().height() <= kMaxSmallScreenHeight,
tests/ui/tst_HiDpi.cpp:1139:                 qPrintable(QString("ApplicationDialog minimumSizeHint.height %1 > %2")
tests/ui/tst_HiDpi.cpp:1140:                                .arg(dlg.minimumSizeHint().height()).arg(kMaxSmallScreenHeight)));
tests/ui/tst_HiDpi.cpp:1159:        const QSize msh = dlg.minimumSizeHint();
tests/ui/tst_HiDpi.cpp:1167:                 qPrintable(QString("InterviewDialog minimumSizeHint.height %1 > %2")
tests/ui/tst_HiDpi.cpp:1186:        const QSize msh = dlg.minimumSizeHint();
tests/ui/tst_HiDpi.cpp:1194:                 qPrintable(QString("FollowUpDialog minimumSizeHint.height %1 > %2")
tests/ui/tst_HiDpi.cpp:1213:        const QSize msh = dlg.minimumSizeHint();
tests/ui/tst_HiDpi.cpp:1221:                 qPrintable(QString("SessionDialog minimumSizeHint.height %1 > %2")
tests/ui/tst_HiDpi.cpp:1240:        const QSize msh = dlg.minimumSizeHint();
tests/ui/tst_HiDpi.cpp:1248:                 qPrintable(QString("TrackDialog minimumSizeHint.height %1 > %2")
```

Function implementation in `tests/ui/tst_HiDpi.cpp`:
```bash
sed -n '445,475p' tests/ui/tst_HiDpi.cpp
```
Raw Output:
```cpp
            if (lineEdit) return lineEdit->text();
            return QString();
        }();

        // 1. widget->width() >= widget->minimumSizeHint().width() and height
        const QSize minHint = w->minimumSizeHint();
        if (minHint.isValid()) {
            if (minHint.width() > 0 && w->width() < minHint.width()) {
                violations.append({className,
                                   w->objectName(),
                                   widgetText,
                                   contextDescription,
                                   QString(u"width (%1) < minimumSizeHint.width (%2)"_s)
                                       .arg(w->width())
                                       .arg(minHint.width())});
            }
            if (minHint.height() > 0 && w->height() < minHint.height()) {
                violations.append({className,
                                   w->objectName(),
                                   widgetText,
                                   contextDescription,
                                   QString(u"height (%1) < minimumSizeHint.height (%2)"_s)
                                       .arg(w->height())
                                       .arg(minHint.height())});
            }
        }

        const bool elide = w->property("jobprepElide").toBool();

        // 2. Non-wrapping QLabel and buttons: fontMetrics().horizontalAdvance(text) <= contentsRect().width()
        if (label) {
```

#### A.2 Local Reproduction with CI Font

Environment variable check:
```bash
grep -rn JOBPREP_TEST_FONT_FAMILY tests
```
Raw Output:
```
tests/ui_test_helpers/UiTestHelpers.cpp:120:    const QByteArray family = qgetenv("JOBPREP_TEST_FONT_FAMILY");
tests/ui_test_helpers/UiTestHelpers.h:62:/// Overrides the application font if the JOBPREP_TEST_FONT_FAMILY environment variable
tests/ui/tst_HiDpi.cpp:242:    const QString envFont = qEnvironmentVariable("JOBPREP_TEST_FONT_FAMILY");
```

Font family resolution:
- Development machine: `fc-match` outputs `NotoSans-Regular.ttf: "Noto Sans" "Regular"`.
- Ubuntu CI runner: installs package `fonts-dejavu-core`, resolving sans-serif to `DejaVu Sans`.

Failing-before run (executed prior to commit `9ea04cb`):
Command:
```bash
JOBPREP_TEST_FONT_FAMILY="DejaVu Sans" ./build/dev-linux/tests/ui/tst_HiDpi testPseudoTranslationHiDpi
```
Raw Output:
```
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1246x700] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвг': width (151) < minimumSizeHint.width (158)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1246x700] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абв': width (151) < minimumSizeHint.width (157)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1246x540] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвг': width (151) < minimumSizeHint.width (158)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1246x540] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абв': width (151) < minimumSizeHint.width (157)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1246x700] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвг': width (151) < minimumSizeHint.width (158)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1246x700] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абв': width (151) < minimumSizeHint.width (157)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1246x540] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвг': width (151) < minimumSizeHint.width (158)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1246x540] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абв': width (151) < minimumSizeHint.width (157)
FAIL!  : tst_HiDpi::testPseudoTranslationHiDpi() 'allViolations.isEmpty()' returned FALSE. (Actual (allViolations.count()): 8, Expected: 0)
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_HiDpi.cpp(933)]
```
The exact 8 violations observed on CI reproduced locally.

#### A.3 Cause & Raw Geometry

Diagnostic probe output under `DejaVu Sans` pseudo-translation:
- Button `CalendarAddInterviewBtn`:
  - `minimumSizeHint()`: `QSize(158, 32)`
  - `sizeHint()`: `QSize(158, 32)`
  - `actual width()`: `151`
  - `sizePolicy`: `QSizePolicy(Horizontal: Preferred, Vertical: Fixed)`
- Button `CalendarAddFollowUpBtn`:
  - `minimumSizeHint()`: `QSize(157, 32)`
  - `sizeHint()`: `QSize(157, 32)`
  - `actual width()`: `151`
  - `sizePolicy`: `QSizePolicy(Horizontal: Preferred, Vertical: Fixed)`
- Buttons container (`btnRow` / `btnLayout`):
  - 2-column grid layout with `spacing = 6 px`, side margins `16 px` each (`contentsMargins(16, 0, 16, 0)`).
  - Required minimum width: `16 + 158 + 6 + 157 + 16 = 353 px`.
- Splitter & Agenda panel:
  - `QSplitter` child widget `panel`: had `panel->setMinimumWidth(panelMinWidth)` where `panelMinWidth = std::max(fontMetrics().averageCharWidth() * 32, 340) = 340 px`.
  - When the splitter allocated 340 px to `panel`, available button row width was `340 - 32 - 6 = 302 px`.
  - Shared equally between 2 buttons: `302 / 2 = 151 px` per button.
  - Because `151 < 158` and `151 < 157`, both buttons were squeezed below their `minimumSizeHint`.
- **Verdict**: **Layout defect**. The agenda panel hard-coded minimum width of 340 px was narrower than the content demanded under wider fonts or longer translations.

#### A.4 Fix in Code (Not in Test)

Diff applied in commit `9ea04cb` (`src/ui/pages/CalendarPage.cpp`):
```diff
--- a/src/ui/pages/CalendarPage.cpp
+++ b/src/ui/pages/CalendarPage.cpp
@@ -211,8 +211,16 @@ QWidget* CalendarPage::setupAgendaPanel()
     m_agendaList->setSelectionMode(QAbstractItemView::SingleSelection);
     panelLayout->addWidget(m_agendaList, 1);
 
-    const int panelMinWidth = std::max(fontMetrics().averageCharWidth() * 32, 340);
-    panel->setMinimumWidth(panelMinWidth);
+    // Ensure panel width accommodates action buttons row at minimumSizeHint
+    const int buttonsMinWidth = m_agendaCardLayout->contentsMargins().left()
+                              + m_agendaCardLayout->contentsMargins().right()
+                              + addInterviewBtn->minimumSizeHint().width()
+                              + btnLayout->horizontalSpacing()
+                              + addFollowUpBtn->minimumSizeHint().width();
+    const int panelMinWidth = std::max({fontMetrics().averageCharWidth() * 32,
+                                        340,
+                                        buttonsMinWidth});
+    panel->setMinimumWidth(panelMinWidth);
```

- In English with default font, `buttonsMinWidth` is ~216 px <= 340 px; `panelMinWidth` evaluates to exactly 340 px, leaving the English layout 100% unchanged.
- Under `DejaVu Sans` pseudo-translation, `buttonsMinWidth` is 353 px; `panelMinWidth` expands to 353 px, ensuring the buttons receive their full required width without clipping.
- Added test slot `testAgendaPanelRespectsLayoutMinimumSize()` in `tests/ui/tst_PageCalendar.cpp`.
- Captured before/after offscreen grabs: `docs/reports/images/calendar-pseudo-before-light.png` and `docs/reports/images/calendar-pseudo-after-light.png`. *(Note: offscreen rendering is headless and not the user's physical screen).*

Passing-after run:
Command:
```bash
JOBPREP_TEST_FONT_FAMILY="DejaVu Sans" ./build/dev-linux/tests/ui/tst_HiDpi testPseudoTranslationHiDpi
```
Raw Output:
```
********* Start testing of tst_HiDpi *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_HiDpi::initTestCase()
PASS   : tst_HiDpi::testPseudoTranslationHiDpi()
PASS   : tst_HiDpi::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 4569ms
********* Finished testing of tst_HiDpi *********
```

#### A.5 Wire Pseudo-Translation Over Font Matrix

In commit `5d6dfe3`, `testPseudoTranslationHiDpi` in `tests/ui/tst_HiDpi.cpp` was wired over a font matrix:
1. Pass 1: System default font (full matrix: Light/Dark, 1100x700 / 1100x540 / 1246x700 / 1246x540).
2. Pass 2: `DejaVu Sans` font pass (kept cheap: Light mode, 1100x700 and 1246x700) to detect CI font regressions locally without blowing up test execution time on slow runners.

Slot duration before and after (measured with `-o -,xunitxml`):
- Before (`5d6dfe3~1`):
  ```xml
  <testcase name="testPseudoTranslationHiDpi" result="pass" time="2.966392"/>
  ```
- After (`5d6dfe3`):
  ```xml
  <testcase name="testPseudoTranslationHiDpi" result="pass" time="4.569480"/>
  ```
Net increase is ~1.6 s locally.

#### A.6 Real-Translation Slots Run Under DejaVu Sans

Commands executed:
```bash
JOBPREP_TEST_FONT_FAMILY="DejaVu Sans" ./build/dev-linux/tests/ui/tst_HiDpi testRealTranslationUkHiDpi
```
Raw Output:
```
********* Start testing of tst_HiDpi *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_HiDpi::initTestCase()
PASS   : tst_HiDpi::testRealTranslationUkHiDpi()
PASS   : tst_HiDpi::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 3185ms
********* Finished testing of tst_HiDpi *********
```

```bash
JOBPREP_TEST_FONT_FAMILY="DejaVu Sans" ./build/dev-linux/tests/ui/tst_HiDpi testRealTranslationRuHiDpi
```
Raw Output:
```
********* Start testing of tst_HiDpi *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_HiDpi::initTestCase()
PASS   : tst_HiDpi::testRealTranslationRuHiDpi()
PASS   : tst_HiDpi::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 3181ms
********* Finished testing of tst_HiDpi *********
```
Both real Ukrainian and Russian translation slots pass cleanly with 0 violations under `DejaVu Sans`.

---

### B. Translation Corrections & Review Implementation

Applied in commit `5e092b2`.

#### B.1 Ukrainian: "сесія" -> "заняття"
In Ukrainian, "сесія" denotes an academic exam period; for study sessions, the proper term is **"заняття"** (neuter; gen. pl. "занять").
All 6 occurrences in `resources/translations/jobprep_uk.ts` were replaced:
- `Log a study session` -> `Записати заняття`
- `Log study session for this topic` -> `Записати заняття для цієї теми`
- `Log a study session or follow up on an application.` -> `Запишіть заняття або виконайте фолоу-ап за заявкою.`
- `Could not log session: %1` -> `Не вдалося записати заняття: %1`
- `Study session` -> `Заняття`
- `session` -> `заняття`

Post-edit verification:
```bash
grep -n "сесі" resources/translations/jobprep_uk.ts
```
Raw Output:
`(no output, exit code 1)`

#### B.2 Ukrainian Grammar
In `InterviewDialog` and `SessionDialog`:
- Old: `Тривалість має бути щонайменше 1 хвилину.`
- Corrected: `Тривалість має бути не менше 1 хвилини.` (Russian `не менее 1 минуты` retained).

#### B.3 Restore Summary List
Replaced genitive plurals with label-colon structured items:
- Ukrainian:
  ```
  - Напрями: %1<br>- Теми: %2<br>- Підзавдання: %3<br>- Заняття: %4<br>- Заявки: %5<br>- Записи історії статусів: %6<br>- Співбесіди: %7
  ```
- Russian:
  ```
  - Направления: %1<br>- Темы: %2<br>- Подзадачи: %3<br>- Занятия: %4<br>- Заявки: %5<br>- Записи истории статусов: %6<br>- Собеседования: %7
  ```

#### B.4 Register of Language Note
Replaced legalistic register (`набуде чинності` / `вступит в силу`):
- Ukrainian: `Мову буде змінено після перезапуску JobPrep.`
- Russian: `Язык изменится после перезапуска JobPrep.`

#### B.5 Consistency Across Catalogs
1. **"Custom (%1)"**: Replaced `Користувацький` / `Пользовательский` with `Власний (%1)` / `Свой (%1)` in `TrackDialog`, matching Settings.
2. **"sample data"**: Unified all occurrences to `демонстраційні дані` / `демонстрационные данные` (including titles "Confirm Load Sample Data", "Load Sample Data Failed").
3. **"Discard"**: Inspected Qt standard translation via `lconvert`:
   ```bash
   /usr/lib/qt6/bin/lconvert -i /usr/share/qt6/translations/qtbase_ru.qm -o /tmp/qtbase_ru.ts && /usr/lib/qt6/bin/lconvert -i /usr/share/qt6/translations/qtbase_uk.qm -o /tmp/qtbase_uk.ts && grep -n -B2 -A3 "Discard" /tmp/qtbase_*.ts
   ```
   Raw Output:
   ```
   /tmp/qtbase_ru.ts-1723-    <message>
   /tmp/qtbase_ru.ts:1724:        <source>Discard</source>
   /tmp/qtbase_ru.ts-1725-        <translation>Отклонить</translation>
   /tmp/qtbase_ru.ts-1726-    </message>
   --
   /tmp/qtbase_uk.ts-1726-    <message>
   /tmp/qtbase_uk.ts:1727:        <source>Discard</source>
   /tmp/qtbase_uk.ts-1728-        <translation>Відхилити</translation>
   /tmp/qtbase_uk.ts-1729-    </message>
   ```
   Qt's standard `qtbase` buttons translate Discard as **`Відхилити`** (uk) and **`Отклонить`** (ru). Aligned our strings accordingly (`Відхилити зміни?`, `Отклонить изменения?`).
4. **Addressing & Possessives**:
   - Impersonal addressing in calendar notes: uk `Що заплановано зробити цього дня`, `З ким відбудеться розмова`; ru `Что запланировано сделать в этот день`, `С кем будет разговор`.
   - Dropped unneeded possessives in `DataToolsController`:
     - uk: `Захисну резервну копію поточної бази даних буде збережено до:`, `Поточну базу даних НЕ змінено.`, `База даних вже містить дані.`
     - ru: `Защитная резервная копия текущей базы данных будет сохранена в:`, `Текущая база данных НЕ изменена.`, `База данных уже содержит данные.`

#### B.6 Non-Positive Identifiers
- uk: `Файл резервної копії містить неприпустимі ідентифікатори (нуль або від'ємні значення).`
- ru: `Файл резервной копии содержит недопустимые идентификаторы (ноль или отрицательные значения).`

#### B.7 Apostrophe Consistency
Audited Ukrainian apostrophe usage in `resources/translations/jobprep_uk.ts`:
```bash
grep -c "’" resources/translations/jobprep_uk.ts; grep -c "&apos;" resources/translations/jobprep_uk.ts
```
Raw Output:
```
0
13
```
Zero typographic `’` characters exist. All 13 Ukrainian apostrophes strictly use standard ASCII apostrophe `'` encoded as `&apos;` in XML (`з&apos;являтимуться`, `обов&apos;язкові`, `Зв&apos;язатися`, `інтерв&apos;юера`, `від&apos;ємні`).

#### B.8 Weekly Study Goal Hint Update
English source string in `src/ui/pages/SettingsPage.cpp` updated from `8, 7.5, 7:30, 7;30, 450 min` to:
`Weekly study goal in hours and minutes (e.g. 8, 7:30, 7;30, 450 min; bare numbers or decimals mean hours)`
Updated corresponding Ukrainian and Russian translations.

Inventory of `7.5` references across the repository:
```bash
grep -rn "7\.5" src tests resources docs
```
Findings and verdicts:
1. `src/ui/widgets/MonthView.cpp:260`: `qMax(7.5, baseFont.pointSizeF() - 1.0)` — font size clamping calculation (unrelated; stays).
2. `src/ui/delegates/AgendaItemDelegate.cpp:34, 133`: `qMax(7.5, ...)` — font size clamping calculation (unrelated; stays).
3. `src/ui/pages/SettingsPage.cpp:66`: code comment listing test cases for the typing parser — stays as internal developer documentation.
4. `tests/ui/tst_HiDpi.cpp:263`: font calculation (unrelated; stays).
5. `tests/ui/tst_PageSettings.cpp:425, 1263`: tests for typing parser `7.5` producing 465 min — stays per specification rule ("Do not change the parsing rules or the tests of the typing rules (7.5 still gives 465)").
6. `docs/reports/*`: historical milestone reports (stays as historical record).
7. `docs/SPEC.md:468`: §14 historical changelog for entry 1.13 (stays as historical record).

Parser analysis for token `min`:
- The parser evaluates `min` as an explicit recognized unit word via regex `(?:min|mins|minutes?)`.
- Tokens `хв` and `мин` are currently **not** recognized (treated as invalid and rejected).
- Phrase *"bare numbers or decimals mean hours"*: The phrase is inaccurate with respect to decimal fractions: the parser treats `.` and `,` strictly as time separators `<hours>.<minutes>` rather than decimal floats, where single-digit minutes represent tens of minutes (`.5` = 50 min).
  - Parser outputs:
    - `7.5` -> 465 min (7h 50m = 470m, rounded to 15m step = 465m)
    - `7,5` -> 465 min (7h 50m = 470m, rounded to 15m step = 465m)
    - `7.25` -> 450 min (7h 25m = 445m, rounded to 15m step = 450m)

#### Raw Translation Tag Diff (Commit `5e092b2`)

Command:
```bash
git diff -U0 5d6dfe3 5e092b2 resources/translations | grep -E '^[-+] *<(translation|numerusform)'
```
Raw Output:
```
-        <translation>Файл резервной копии содержит недопустимые неположительные идентификаторы.</translation>
+        <translation>Файл резервной копии содержит недопустимые идентификаторы (ноль или отрицательные значения).</translation>
-        <translation>Ваша база данных уже содержит данные. Загрузка демонстрационных данных добавит стандартные направления и темы, не удаляя существующие элементы.
+        <translation>База данных уже содержит данные. Загрузка демонстрационных данных добавит стандартные направления и темы, не удаляя существующие элементы.
-        <translation>Подтверждение загрузки демо-данных</translation>
+        <translation>Подтверждение загрузки демонстрационных данных</translation>
-        <translation>Ошибка загрузки демо-данных</translation>
+        <translation>Ошибка загрузки демонстрационных данных</translation>
-        <translation>Отменить изменения (Esc)</translation>
+        <translation>Отклонить изменения (Esc)</translation>
-        <translation>Изменения собеседований сохраняются сразу и не входят в запрос «Отменить изменения?» на вкладке «Обзор».</translation>
+        <translation>Изменения собеседований сохраняются сразу и не входят в запрос «Отклонить изменения?» на вкладке «Обзор».</translation>
-        <translation>Отменить изменения?</translation>
+        <translation>Отклонить изменения?</translation>
-        <translation>В этой заявке есть несохранённые изменения. Отменить их?</translation>
+        <translation>В этой заявке есть несохранённые изменения. Отклонить их?</translation>
-        <translation>Что планируется сделать в этот день</translation>
+        <translation>Что запланировано сделать в этот день</translation>
-        <translation>С кем вы будете общаться</translation>
+        <translation>С кем будет разговор</translation>
-        <translation>Отменить изменения (Esc)</translation>
+        <translation>Отклонить изменения (Esc)</translation>
-        <translation>Отменить изменения?</translation>
+        <translation>Отклонить изменения?</translation>
-        <translation>В этом собеседовании есть несохранённые изменения. Отменить их?</translation>
+        <translation>В этом собеседовании есть несохранённые изменения. Отклонить их?</translation>
-        <translation>Отменить изменения (Esc)</translation>
+        <translation>Отклонить изменения (Esc)</translation>
-        <translation>Пользовательский (%1)</translation>
+        <translation>Свой (%1)</translation>
-        <translation>Язык вступит в силу после перезапуска JobPrep.</translation>
+        <translation>Язык изменится после перезапуска JobPrep.</translation>
-        <translation>Еженедельная учебная цель в часах и минутах (например: 8, 7.5, 7:30, 7;30, 450 min; числа или десятичные дроби означают часы)</translation>
+        <translation>Еженедельная учебная цель в часах и минутах (например: 8, 7:30, 7;30, 450 min; числа или десятичные дроби означают часы)</translation>
-        <translation>Загрузить демо-данные</translation>
+        <translation>Загрузить демонстрационные данные</translation>
-        <translation>Загрузить демо-данные?</translation>
+        <translation>Загрузить демонстрационные данные?</translation>
-        <translation>Демо-данные</translation>
+        <translation>Демонстрационные данные</translation>
-        <translation>Организуйте темы собеседований по C++ и English в колонках канбана и записывайте учебные занятия.</translation>
+        <translation>Организуйте темы собеседований по C++ и English в колонках канбана и записывайте занятия.</translation>
-        <translation>Записать учебное занятие или выполнить фоллоу-ап по заявке.</translation>
+        <translation>Запишите занятие или выполните фоллоу-ап по заявке.</translation>
-        <translation>Защитная резервная копия вашей текущей базы данных будет сохранена в: %1<br><br>Ваша текущая база данных НЕ изменена.</translation>
+        <translation>Защитная резервная копия текущей базы данных будет сохранена в: %1<br><br>Текущая база данных НЕ изменена.</translation>
-        <translation>Восстановление базы данных успешно завершено из резервной копии:<br><b>%8</b><br><br>Восстановлено элементов:<br>- %1 направлений<br>- %2 тем<br>- %3 подзадач<br>- %4 учебных занятий<br>- %5 заявок<br>- %6 записей истории статусов<br>- %7 собеседований</translation>
+        <translation>Восстановление базы данных успешно завершено из резервной копии:<br><b>%8</b><br><br>Восстановлено элементов:<br>- Направления: %1<br>- Темы: %2<br>- Подзадачи: %3<br>- Занятия: %4<br>- Заявки: %5<br>- Записи истории статусов: %6<br>- Собеседования: %7</translation>
-        <translation>Файл резервної копії містить неприпустимі непозитивні ідентифікатори.</translation>
+        <translation>Файл резервної копії містить неприпустимі ідентифікатори (нуль або від&apos;ємні значення).</translation>
-        <translation>Ваша база даних вже містить дані. Завантаження демонстраційних даних додасть стандартні напрями та теми, не видаляючи наявні елементи.
+        <translation>База даних вже містить дані. Завантаження демонстраційних даних додасть стандартні напрями та теми, не видаляючи наявні елементи.
-        <translation>Підтвердження завантаження демо-даних</translation>
+        <translation>Підтвердження завантаження демонстраційних даних</translation>
-        <translation>Помилка завантаження демо-даних</translation>
+        <translation>Помилка завантаження демонстраційних даних</translation>
-        <translation>Відкинути зміни (Esc)</translation>
+        <translation>Відхилити зміни (Esc)</translation>
-        <translation>Зміни співбесід зберігаються відразу й не входять до запиту «Відкинути зміни?» на вкладці «Огляд».</translation>
+        <translation>Зміни співбесід зберігаються відразу й не входять до запиту «Відхилити зміни?» на вкладці «Огляд».</translation>
-        <translation>Відкинути зміни?</translation>
+        <translation>Відхилити зміни?</translation>
-        <translation>Ця заявка має незбережені зміни. Відкинути їх?</translation>
+        <translation>Ця заявка має незбережені зміни. Відхилити їх?</translation>
-        <translation>Що ви плануєте зробити в цей день</translation>
+        <translation>Що заплановано зробити цього дня</translation>
-        <translation>З ким ви спілкуватиметеся</translation>
+        <translation>З ким відбудеться розмова</translation>
-        <translation>Відкинути зміни (Esc)</translation>
+        <translation>Відхилити зміни (Esc)</translation>
-        <translation>Тривалість має бути щонайменше 1 хвилину.</translation>
+        <translation>Тривалість має бути не менше 1 хвилини.</translation>
-        <translation>Відкинути зміни?</translation>
+        <translation>Відхилити зміни?</translation>
-        <translation>Ця співбесіда має незбережені зміни. Відкинути їх?</translation>
+        <translation>Ця співбесіда має незбережені зміни. Відхилити їх?</translation>
-        <translation>Записати сесію навчання</translation>
+        <translation>Записати заняття</translation>
-        <translation>Відкинути зміни (Esc)</translation>
+        <translation>Відхилити зміни (Esc)</translation>
-        <translation>Записати сесію</translation>
+        <translation>Записати заняття</translation>
-        <translation>Записати сесію (Ctrl+S)</translation>
+        <translation>Записати заняття (Ctrl+S)</translation>
-        <translation>Тривалість має бути щонайменше 1 хвилину.</translation>
+        <translation>Тривалість має бути не менше 1 хвилини.</translation>
-        <translation>Не вдалося записати сесію: %1</translation>
+        <translation>Не вдалося записати заняття: %1</translation>
-        <translation>Користувацький (%1)</translation>
+        <translation>Власний (%1)</translation>
-        <translation>Сесія навчання</translation>
+        <translation>Заняття</translation>
-        <translation>Записати сесію навчання</translation>
+        <translation>Записати заняття</translation>
-        <translation>Запишіть сесію навчання або виконайте фолоу-ап за заявкою.</translation>
+        <translation>Запишіть заняття або виконайте фолоу-ап за заявкою.</translation>
-        <translation>Мова набуде чинності після перезапуску JobPrep.</translation>
+        <translation>Мову буде змінено після перезапуску JobPrep.</translation>
-        <translation>Щотижнева навчальна ціль у годинах і хвилинах (наприклад: 8, 7.5, 7:30, 7;30, 450 min; числа чи десяткові дроби означають години)</translation>
+        <translation>Щотижнева навчальна ціль у годинах і хвилинах (наприклад: 8, 7:30, 7;30, 450 min; числа чи десяткові дроби означають години)</translation>
-        <translation>Завантажити демо-дані</translation>
+        <translation>Завантажити демонстраційні дані</translation>
-        <translation>Впорядковуйте теми співбесід із C++ та English у колонках канбану та записуйте сесії навчання.</translation>
+        <translation>Впорядковуйте теми співбесід із C++ та English у колонках канбану та записуйте заняття.</translation>
-        <translation>Записати сесію навчання для цієї теми</translation>
+        <translation>Записати заняття для цієї теми</translation>
-        <translation>Завантажити демо-дані?</translation>
+        <translation>Завантажити демонстраційні дані?</translation>
-        <translation>Демо-дані</translation>
+        <translation>Демонстраційні дані</translation>
-        <translation>Захисну резервну копію вашої поточної бази даних буде збережено до: %1<br><br>Вашу поточну базу даних НЕ змінено.</translation>
+        <translation>Захисну резервну копію поточної бази даних буде збережено до: %1<br><br>Поточну базу даних НЕ змінено.</translation>
-        <translation>Відновлення бази даних успішно завершено з резервної копії:<br><b>%8</b><br><br>Відновлено елементів:<br>- %1 напрямів<br>- %2 тем<br>- %3 підзавдань<br>- %4 сесій навчання<br>- %5 заявок<br>- %6 записів історії статусів<br>- %7 співбесід</translation>
+        <translation>Відновлення бази даних успішно завершено з резервної копії:<br><b>%8</b><br><br>Відновлено елементів:<br>- Напрями: %1<br>- Теми: %2<br>- Підзавдання: %3<br>- Заняття: %4<br>- Заявки: %5<br>- Записи історії статусів: %6<br>- Співбесіди: %7</translation>
```

#### Compilation & Translation Test Verification

`lrelease` compilation check:
```bash
/usr/lib/qt6/bin/lrelease resources/translations/jobprep_uk.ts resources/translations/jobprep_ru.ts
```
Raw Output:
```
Updating 'resources/translations/jobprep_uk.qm'...
    Generated 506 translation(s) (506 finished and 0 unfinished)
Updating 'resources/translations/jobprep_ru.qm'...
    Generated 506 translation(s) (506 finished and 0 unfinished)
```
Zero warnings emitted.

Test suite run:
```bash
./build/dev-linux/tests/i18n/tst_Translations
```
Raw Output:
```
********* Start testing of tst_Translations *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_Translations::initTestCase()
PASS   : tst_Translations::testTsParsingAndCompletenessFixtures()
PASS   : tst_Translations::testRealTsFilesCompleteness()
PASS   : tst_Translations::testRealTsFilesSyncWithSource()
PASS   : tst_Translations::testPluralMechanismUkrainian()
PASS   : tst_Translations::testPluralMechanismRussian()
PASS   : tst_Translations::testEmbeddedQmFilesExist()
PASS   : tst_Translations::testMissingQtBaseNoWarningNoCrash()
PASS   : tst_Translations::testEnglishFallbackAndPseudoTranslator()
PASS   : tst_Translations::testRealCatalogsPluralTable()
PASS   : tst_Translations::cleanupTestCase()
Totals: 11 passed, 0 failed, 0 skipped, 0 blacklisted, 26ms
********* Finished testing of tst_Translations *********
```

---

### C. Leftovers from Part D

#### C.1 `scripts/check-translations.sh` Path Finder
Diff applied in commit `27d0268`:
```diff
--- a/scripts/check-translations.sh
+++ b/scripts/check-translations.sh
@@ -1,7 +1,7 @@
 #!/usr/bin/env bash
 set -euo pipefail
 
-SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
+SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}" 2>/dev/null || echo "${BASH_SOURCE[0]%/*}")" && pwd)"
 REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
 
 cd "${REPO_ROOT}"
@@ -12,10 +12,6 @@ if [ -n "${QT_PREFIX_PATH:-}" ] && [ -x "${QT_PREFIX_PATH}/bin/lupdate" ]; then
     LUPDATE="${QT_PREFIX_PATH}/bin/lupdate"
 elif command -v lupdate >/dev/null 2>&1; then
     LUPDATE="$(command -v lupdate)"
-elif [ -x "/usr/lib/qt6/bin/lupdate" ]; then
-    LUPDATE="/usr/lib/qt6/bin/lupdate"
-elif [ -x "/var/tmp/qt682/6.8.2/gcc_64/bin/lupdate" ]; then
-    LUPDATE="/var/tmp/qt682/6.8.2/gcc_64/bin/lupdate"
 fi
```

Empty PATH test run:
```bash
/bin/bash -c 'env -i PATH="" /bin/bash scripts/check-translations.sh'; echo "exit code: $?"
```
Raw Output:
```
==> lupdate not found, skipping translation sync check.
exit code: 0
```

#### C.2 `.qm` Git Ignore & Build Destination
Commands executed:
```bash
git ls-files '*.qm'
```
Raw Output:
`(no output)`

```bash
git check-ignore -v resources/translations/jobprep_uk.qm
```
Raw Output:
```
.gitignore:33:*.qm	resources/translations/jobprep_uk.qm
```
Where the build writes `.qm` files:
During CMake build, Qt's translation machinery (`qt_add_translations`) compiles `.ts` files into binary `.qm` catalogs strictly within the build directory (`build/dev-linux/resources/translations/` and `${CMAKE_CURRENT_BINARY_DIR}/.rcc/i18n/`), which are subsequently embedded into the `jobprep_core` static library archive. No `.qm` files are written into the source tree.

#### C.3 Empirical Date Comparison: Old (`0f4b9b2`) vs New (`HEAD`)

Tested in a throw-away worktree of `0f4b9b2` against `HEAD` with English UI language:

| Metric / Call | Locale `uk_UA` (Old `0f4b9b2`) | Locale `uk_UA` (New `HEAD`) | Locale `de_DE` (Old `0f4b9b2`) | Locale `de_DE` (New `HEAD`) |
| :--- | :--- | :--- | :--- | :--- |
| `SingleTabDateEdit.text()` (for `2026-11-15`) | `15.11.26` | `2026-11-15` | `15.11.26` | `2026-11-15` |
| `SingleTabDateEdit.displayFormat()` | `dd.MM.yy` | `yyyy-MM-dd` | `dd.MM.yy` | `yyyy-MM-dd` |
| `calendarWidget()->firstDayOfWeek()` | `1` (Monday) | `1` (Monday) | `1` (Monday) | `1` (Monday) |
| `MonthView::monthTitle(2026, 11)` | `November 2026` | `November 2026` | `November 2026` | `November 2026` |
| `MonthView::monthTitle(2026, 1)` | `January 2026` | `January 2026` | `January 2026` | `January 2026` |

**User Impact Analysis**:
In the old code (`0f4b9b2`), an English-speaking user running on a Ukrainian or German OS experienced inconsistent date representations (`15.11.26`) because `QDateEdit` defaulted to the OS locale format. With the D0.2 fix on `HEAD`, `SingleTabDateEdit` explicitly enforces `yyyy-MM-dd`, guaranteeing consistent ISO dates (`2026-11-15`) across all systems.

#### C.4 Documentation Updates
In commit `9bcd633`:
- Updated `docs/RUNNING.md` Section 11 noting CI run ID `118200252685` where `qtbase_uk.qm` (158,274 bytes) and `qtbase_ru.qm` (203,767 bytes) are verified present across Ubuntu Qt 6.8.2, Ubuntu Qt 6.10.3, and Windows Qt 6.10.3 aqt kits, while the user's local Windows installation path (`<Qt>\<version>\msvc2022_64\translations`) is not verified.
- Updated `docs/SPEC.md` §14 Changelog with entry 1.29 documenting the closeout fixes and CI-verified catalog presence.

---

### Labelled Predictions Per CI Job by Real Name

- **`Gate (ubuntu-latest, Qt 6.8.2)`**: the pseudo test now passes with DejaVu locally; the translation sync check runs for the first time on CI at the end of `check.sh`: reports only (Qt minor 8 vs 10); **GREEN**.
- **`Gate (ubuntu-latest, Qt 6.10.3)`**: the pseudo test now passes with DejaVu locally; the translation sync check runs for the first time on CI at the end of `check.sh`: enforces zero diff; **GREEN**.
- **`Gate (windows-latest, Qt 6.10.3)`**: unchanged apart from the layout fix; **GREEN**.

---

### Not Verified by Agent, User to Check

1. The CI run itself.
2. Translation wording in the changed strings.
3. Windows screens under `uk`/`ru` at 100% and 150%.
4. The language selector and restart on Windows.
5. Shortcuts under Ukrainian/Russian layouts.
6. `Alt+Down` / `F4` in the five date places.
7. The dashboard "Today" rows.

---

### Totals by Command

- **Test Binaries Count**:
  ```bash
  ctest --preset dev-linux -N | grep -E "Test #[0-9]+" | wc -l
  ```
  `35`
- **Test Functions Count**:
  ```bash
  for bin in $(find build/dev-linux/tests -maxdepth 2 -type f -executable | sort); do "$bin" -functions; done | grep -v '^[A-Z]' | wc -l
  ```
  `504` (compared to 495 in Part C fix-up; net +9 test functions across test suites).

---

### Full Raw Check Output

Command executed:
```bash
CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
```
Raw Output:
```
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.6s)
-- Generating done (0.7s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
[1/35] Linking CXX executable tests/data/tst_Database
[2/35] Linking CXX executable jobprep
[3/35] Linking CXX executable tests/data/tst_TrackRepository
[4/35] Linking CXX executable tests/data/tst_TopicRepository
[5/35] Linking CXX executable tests/data/tst_SessionRepository
[6/35] Linking CXX executable tests/data/tst_ApplicationRepository
[7/35] Linking CXX executable tests/data/tst_InterviewRepository
[8/35] Linking CXX executable tests/data/tst_ReminderLogRepository
[9/35] Linking CXX executable tests/data/tst_SeedService
[10/35] Linking CXX executable tests/data/tst_TransactionBehavior
[11/35] Linking CXX executable tests/ui_smoke/ui_smoke
[12/35] Linking CXX executable tests/ui_smoke/tst_MonthView
[13/35] Linking CXX executable tests/models/tst_AgendaListModel
[14/35] Linking CXX executable tests/models/tst_ApplicationTableModel
[15/35] Linking CXX executable tests/models/tst_ApplicationFilterProxy
[16/35] Linking CXX executable tests/models/tst_TopicListModel
[17/35] Linking CXX executable tests/models/tst_TopicBoardProxy
[18/35] Linking CXX executable tests/models/tst_TopicTableProxy
[19/35] Linking CXX executable tests/services/tst_AgendaService
[20/35] Linking CXX executable tests/services/tst_DayWatcher
[21/35] Linking CXX executable tests/services/tst_ExportService
[22/35] Linking CXX executable tests/services/tst_ImportExport
[23/35] Linking CXX executable tests/services/tst_ReminderService
[24/35] Linking CXX executable tests/services/tst_SettingsService
[25/35] Linking CXX executable tests/services/tst_StatsService
[26/35] Linking CXX executable tests/ui/tst_Notifiers
[27/35] Linking CXX executable tests/ui/tst_StudyRegression
[28/35] Linking CXX executable tests/ui/tst_PageJobs
[29/35] Linking CXX executable tests/ui/tst_PageCalendar
[30/35] Linking CXX executable tests/ui/tst_PageStudy
[31/35] Linking CXX executable tests/ui/tst_PageHome
[32/35] Linking CXX executable tests/ui/tst_PageSettings
[33/35] Linking CXX executable tests/ui/tst_EmptyStatesAndFocus
[34/35] Linking CXX executable tests/ui/tst_Shortcuts
[35/35] Linking CXX executable tests/ui/tst_HiDpi
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start 34: tst_HiDpi
      Start 29: tst_PageStudy
 1/35 Test #34: tst_HiDpi ........................   Passed   21.64 sec
      Start 30: tst_PageHome
 2/35 Test #29: tst_PageStudy ....................   Passed   22.31 sec
      Start 33: tst_EmptyStatesAndFocus
 3/35 Test #33: tst_EmptyStatesAndFocus ..........   Passed   10.67 sec
      Start 27: tst_PageJobs
 4/35 Test #30: tst_PageHome .....................   Passed   17.07 sec
      Start 32: tst_Shortcuts
 5/35 Test #32: tst_Shortcuts ....................   Passed    6.55 sec
      Start 28: tst_PageCalendar
 6/35 Test #27: tst_PageJobs .....................   Passed   12.85 sec
      Start 26: tst_StudyRegression
 7/35 Test #28: tst_PageCalendar .................   Passed    7.24 sec
      Start 10: ui_smoke
 8/35 Test #26: tst_StudyRegression ..............   Passed    6.78 sec
      Start 31: tst_PageSettings
 9/35 Test #31: tst_PageSettings .................   Passed    4.75 sec
      Start 25: tst_Notifiers
10/35 Test #25: tst_Notifiers ....................   Passed    2.56 sec
      Start 24: tst_StatsService
11/35 Test #24: tst_StatsService .................   Passed    0.95 sec
      Start 35: tst_Translations
12/35 Test #35: tst_Translations .................   Passed    0.07 sec
      Start 21: tst_ImportExport
13/35 Test #21: tst_ImportExport .................   Passed    0.69 sec
      Start  8: tst_SeedService
14/35 Test  #8: tst_SeedService ..................   Passed    0.29 sec
      Start 23: tst_SettingsService
15/35 Test #23: tst_SettingsService ..............   Passed    0.02 sec
      Start 22: tst_ReminderService
16/35 Test #22: tst_ReminderService ..............   Passed    0.08 sec
      Start 17: tst_TopicTableProxy
17/35 Test #17: tst_TopicTableProxy ..............   Passed    0.08 sec
      Start 19: tst_DayWatcher
18/35 Test #19: tst_DayWatcher ...................   Passed    0.01 sec
      Start 18: tst_AgendaService
19/35 Test #18: tst_AgendaService ................   Passed    0.10 sec
      Start 12: tst_AgendaListModel
20/35 Test #12: tst_AgendaListModel ..............   Passed    0.03 sec
      Start  3: tst_TopicRepository
21/35 Test  #3: tst_TopicRepository ..............   Passed    0.09 sec
      Start 13: tst_ApplicationTableModel
22/35 Test #13: tst_ApplicationTableModel ........   Passed    0.06 sec
      Start 16: tst_TopicBoardProxy
23/35 Test #16: tst_TopicBoardProxy ..............   Passed    0.06 sec
      Start  5: tst_ApplicationRepository
24/35 Test  #5: tst_ApplicationRepository ........   Passed    0.09 sec
      Start  6: tst_InterviewRepository
25/35 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start 15: tst_TopicListModel
26/35 Test #15: tst_TopicListModel ...............   Passed    0.04 sec
      Start 14: tst_ApplicationFilterProxy
27/35 Test #14: tst_ApplicationFilterProxy .......   Passed    0.02 sec
      Start  4: tst_SessionRepository
28/35 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  2: tst_TrackRepository
29/35 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start  7: tst_ReminderLogRepository
30/35 Test  #7: tst_ReminderLogRepository ........   Passed    0.03 sec
      Start  9: tst_TransactionBehavior
31/35 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start  1: tst_Database
32/35 Test  #1: tst_Database .....................   Passed    0.03 sec
      Start 11: tst_MonthView
33/35 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 20: tst_ExportService
34/35 Test #20: tst_ExportService ................   Passed    0.01 sec
35/35 Test #10: ui_smoke .........................   Passed   12.78 sec

100% tests passed, 0 tests failed out of 35

Label Time Summary:
data       =   0.66 sec*proc (9 tests)
i18n       =   0.07 sec*proc (1 test)
model      =   0.29 sec*proc (6 tests)
service    =   1.87 sec*proc (7 tests)
ui         = 125.29 sec*proc (12 tests)

Total Test time (real) =  65.46 sec
==> lupdate not found, skipping translation sync check.
==> Quality check PASSED with zero warnings and all tests green!
```

---

### Working Tree & Worktrees

Command executed:
```bash
git status --short
```
Raw Output:
`(no output)`

Command executed:
```bash
git worktree list
```
Raw Output:
```
/home/tony/projects/JobPrep 9bcd633 [main]
```

---

### Git Log of Closeout Work

Command executed:
```bash
git log --oneline f3d79c7..HEAD
```
Raw Output:
```
b5acb34 docs(reports): add M7 closeout report with CI diagnostics and verification (6)
9bcd633 docs: record CI-verified qtbase translation catalog presence and Windows paths (C.4)
27d0268 build: ignore *.qm files and enforce QT_PREFIX_PATH/PATH lupdate finder order (C.1, C.2)
5e092b2 i18n: apply translation corrections, glossary consistency, and weekly goal hint update (B)
5d6dfe3 test(ui): run pseudo-translation check over font matrix (A.5)
9ea04cb fix(ui): ensure calendar agenda panel width accommodates button minimum size hints (A)
```


---

# M7 Manual-Check Fixes

**Date:** 2026-11-16  
**Status:** Complete  
**Platform / Environment:** Linux x86_64 (Ubuntu 26.04), GCC 15.2.0, Qt 6.10.2 shared, CMake 3.31.6, Ninja 1.13.1  
**Build Configuration:** `dev-linux` preset (`-g -std=c++20 -Wall -Wextra -Wpedantic -Wshadow -Werror -DJOBPREP_WERROR=ON`)  
**Parallelism:** `CMAKE_BUILD_PARALLEL_LEVEL=2`  

---

## 1. Facts as Given (User Manual Check on Windows 10, 2026-11-16)

1. **Date popup arrows:** Alt+Down and F4 opened the calendar popup everywhere, but on the **Study page** date control, arrow keys did not move the calendar selection until a date had been picked with the mouse. Opening the popup by mouse click on the drop-down button had the same defect. The spin-box part changed the date with Up/Down as expected. The other four date places behaved correctly. Suspected cause: keyboard focus was not being delivered to the calendar's internal view when the popup opened on an embedded form page.
2. **Language selector item gender:** In Russian, the word `System (default)` must agree with "язык" (masculine, `Системный (по умолчанию)`), whereas it was previously translated in feminine form (`Системная по умолчанию`). In Ukrainian, "мова" is feminine and `Системна за замовчуванням` is correct.
3. **Weekly study goal units:** `450 min` was parsed as 450 minutes, but typing `150мин` (or `150 хв`) was silently ignored as a unit and interpreted as 150 hours, then capped at 40 hours (2400 minutes). An unrecognised unit must be rejected as an error, and Cyrillic minute units must be supported.
4. **Dark theme check box visibility:** In dark theme, the check boxes of the checklist items in `TopicDetailPanel` were almost invisible (dark indicator border/fill on dark background; only the focus ring made the control visible). Light theme was fine.

---

## 2. Item 1: Study Page Date Popup Focus & Arrow Navigation

### Investigation and Comparison

Command executed:
```bash
grep -rn "SingleTabDateEdit\|QDateEdit" src
```
Raw Output:
```
src/ui/widgets/SingleTabDateTime.h:5:#include <QDateEdit>
src/ui/widgets/SingleTabDateTime.h:25:/// A QDateEdit where Tab and Backtab advance to the next or previous widget
src/ui/widgets/SingleTabDateTime.h:28:class SingleTabDateEdit : public QDateEdit {
src/ui/widgets/SingleTabDateTime.h:31:        return QCoreApplication::translate("SingleTabDateEdit", sourceText, disambiguation, n);
src/ui/widgets/SingleTabDateTime.h:34:    explicit SingleTabDateEdit(QWidget* parent = nullptr)
src/ui/widgets/SingleTabDateTime.h:35:        : QDateEdit(parent) {
src/ui/widgets/SingleTabDateTime.h:46:    explicit SingleTabDateEdit(const QDate& date, QWidget* parent = nullptr)
src/ui/widgets/SingleTabDateTime.h:47:        : QDateEdit(date, parent) {
src/ui/widgets/SingleTabDateTime.h:82:        QDateEdit::setCalendarPopup(enable);
src/ui/widgets/SingleTabDateTime.h:90:            QCoreApplication::translate("SingleTabDateEdit", "Press Alt+Down or F4 to open calendar");
src/ui/widgets/SingleTabDateTime.h:92:            QDateEdit::setToolTip(hint);
src/ui/widgets/SingleTabDateTime.h:94:            QDateEdit::setToolTip(tip);
src/ui/widgets/SingleTabDateTime.h:96:            QDateEdit::setToolTip(tip + QString(u'\n') + hint);
src/ui/widgets/SingleTabDateTime.h:184:        return QDateEdit::eventFilter(watched, event);
src/ui/widgets/SingleTabDateTime.h:190:                QCoreApplication::translate("SingleTabDateEdit", "Press Alt+Down or F4 to open calendar");
src/ui/widgets/SingleTabDateTime.h:194:                QDateEdit::setToolTip(combined);
src/ui/widgets/SingleTabDateTime.h:204:        return QDateEdit::event(e);
src/ui/widgets/SingleTabDateTime.h:213:        QDateEdit::keyPressEvent(event);
src/ui/widgets/TopicDetailPanel.h:10:class QDateEdit;
src/ui/widgets/TopicDetailPanel.h:123:    QDateEdit* m_dateEdit{nullptr};
src/ui/widgets/TopicDetailPanel.cpp:6:#include <QDateEdit>
src/ui/widgets/TopicDetailPanel.cpp:143:    m_dateEdit = new SingleTabDateEdit(m_form);
src/ui/widgets/TopicDetailPanel.cpp:295:    connect(m_dateEdit, &QDateEdit::dateChanged, this, [this](const QDate& d) {
src/ui/dialogs/InterviewDialog.cpp:5:#include <QDateEdit>
src/ui/dialogs/InterviewDialog.cpp:98:    m_dateEdit = new Widgets::SingleTabDateEdit(this);
src/ui/dialogs/InterviewDialog.cpp:202:    connect(m_dateEdit, &QDateEdit::dateChanged, this, &InterviewDialog::onFormChanged);
src/ui/dialogs/SessionDialog.h:10:class QDateEdit;
src/ui/dialogs/SessionDialog.h:57:    QDateEdit* m_dateEdit{nullptr};
src/ui/dialogs/ApplicationDialog.cpp:6:#include <QDateEdit>
src/ui/dialogs/ApplicationDialog.cpp:209:    m_appliedDateEdit = new Widgets::SingleTabDateEdit(page);
src/ui/dialogs/ApplicationDialog.cpp:229:    m_nextActionDateEdit = new Widgets::SingleTabDateEdit(page);
src/ui/dialogs/ApplicationDialog.cpp:275:    connect(m_appliedDateEdit, &QDateEdit::dateChanged, this, &ApplicationDialog::onFormChanged);
src/ui/dialogs/ApplicationDialog.cpp:278:    connect(m_nextActionDateEdit, &QDateEdit::dateChanged, this, &ApplicationDialog::onFormChanged);
src/ui/dialogs/ApplicationDialog.cpp:285:    connect(m_appliedDateEnabled, &QCheckBox::toggled, m_appliedDateEdit, &QDateEdit::setEnabled);
src/ui/dialogs/ApplicationDialog.cpp:286:    connect(m_nextActionDateEnabled, &QCheckBox::toggled, m_nextActionDateEdit, &QDateEdit::setEnabled);
src/ui/dialogs/InterviewDialog.h:8:class QDateEdit;
src/ui/dialogs/InterviewDialog.h:89:    QDateEdit* m_dateEdit{nullptr};
src/ui/dialogs/ApplicationDialog.h:9:class QDateEdit;
src/ui/dialogs/ApplicationDialog.h:110:    QDateEdit* m_appliedDateEdit{nullptr};
src/ui/dialogs/ApplicationDialog.h:113:    QDateEdit* m_nextActionDateEdit{nullptr};
src/ui/dialogs/SessionDialog.cpp:4:#include <QDateEdit>
src/ui/dialogs/SessionDialog.cpp:49:    m_dateEdit = new Widgets::SingleTabDateEdit(today, this);
src/ui/dialogs/SessionDialog.cpp:106:    connect(m_dateEdit, &QDateEdit::dateChanged, this, &SessionDialog::onFormChanged);
```

**Control Inventory (5 Date Controls in Total):**
1. `ApplicationDialog`: `m_appliedDateEdit` (object name `ApplicationAppliedDateEdit`)
2. `ApplicationDialog`: `m_nextActionDateEdit` (object name `ApplicationNextActionDateEdit`)
3. `InterviewDialog`: `m_dateEdit` (object name `InterviewDateEdit`)
4. `SessionDialog`: `m_dateEdit` (object name `SessionDateEdit`)
5. Study Page (`TopicDetailPanel`): `m_dateEdit` (object name `StudyDetailDateEdit`)

**Parent Chain Comparison:**
- Working controls (1-4): Embedded in top-level modal dialog windows (`ApplicationDialog`, `InterviewDialog`, `SessionDialog`). When modal dialogs display a calendar popup, the popup's window modality and window activation automatically focus the newly mapped popup child.
- Study page control (5): Embedded in `TopicDetailPanel` -> `QScrollArea (StudyDetailScroll)` -> `StudyPage` -> `QStackedWidget (PageStack)` -> `MainWindow`. Inside `MainWindow` without a modal dialog loop, the internal `QTableView` inside `QCalendarWidget` did not reliably obtain active keyboard focus upon popup mapping until a mouse click forced focus onto the calendar view.

**Side-by-Side Open & Focus Code:**

*Before:*
```cpp
void openCalendarPopup() {
    if (!calendarPopup()) return;
    syncCalendarFirstDayOfWeek();
    if (calendarWidget() && calendarWidget()->isVisible()) return;

    QStyleOptionComboBox opt;
    opt.initFrom(this);
    opt.rect = rect();
    opt.subControls = QStyle::SC_All;
    opt.state |= QStyle::State_Enabled;
    const QRect arrowRect = style()->subControlRect(
        QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, this);
    const QPoint clickPos = arrowRect.center();
    QMouseEvent press(QEvent::MouseButtonPress, clickPos, mapToGlobal(clickPos),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(this, &press);
}
```

*After (Fix in shared `SingleTabDateEdit`):*
```cpp
void setupCalendarPopupFocus() {
    syncCalendarFirstDayOfWeek();
    if (auto* cal = calendarWidget()) {
        cal->setFocusPolicy(Qt::StrongFocus);
        if (auto* calTable = cal->findChild<QTableView*>()) {
            calTable->setFocusPolicy(Qt::StrongFocus);
            cal->setFocusProxy(calTable);
        }
        cal->installEventFilter(this);
        if (auto* win = cal->window()) {
            win->installEventFilter(this);
        }
    }
}

void focusCalendarView() {
    if (auto* cal = calendarWidget()) {
        auto* calTable = cal->findChild<QTableView*>();
        QWidget* target = calTable ? static_cast<QWidget*>(calTable) : static_cast<QWidget*>(cal);
        target->setFocus(Qt::PopupFocusReason);
        QTimer::singleShot(0, target, [this, target]() {
            if (calendarWidget() && calendarWidget()->isVisible()) {
                target->setFocus(Qt::PopupFocusReason);
                if (qEnvironmentVariableIntValue("JOBPREP_DEBUG_DATEPOPUP") == 1) {
                    logDatePopupDebug();
                }
            }
        });
    }
}
```

### Table-Driven Test & Verification

Automated test `tst_Shortcuts::testDateCalendarPopupArrows` runs table-driven across all 5 controls using all 3 opening triggers (Alt+Down, F4, mouse click on the drop-down arrow). It asserts that `QApplication::focusWidget()` is the calendar's internal `QTableView` (`QtPrivate::QCalendarView`), and that `Key_Down` (+7 days), `Key_Right` (+1 day), and `Key_PageDown` (+1 month) update the selected date before closing via Escape.

Command executed:
```bash
JOBPREP_DEBUG_DATEPOPUP=1 /home/tony/projects/JobPrep/build/dev-linux/tests/ui/tst_Shortcuts testDateCalendarPopupArrows
```
Raw Output:
```
********* Start testing of tst_Shortcuts *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_Shortcuts::initTestCase()
JOBPREP_DEBUG_DATEPOPUP: popup=0x55c3071778b0 class=QCalendarPopup flags=0x8800f009 visible=1
JOBPREP_DEBUG_DATEPOPUP: focusWidget=0x55c307170350 class=QtPrivate::QCalendarView name=qt_calendar_calendarview
JOBPREP_DEBUG_DATEPOPUP:   chain[0]: 0x55c307170350 class=QtPrivate::QCalendarView name=qt_calendar_calendarview
JOBPREP_DEBUG_DATEPOPUP:   chain[1]: 0x55c3070f61a0 class=QCalendarWidget name=
JOBPREP_DEBUG_DATEPOPUP:   chain[2]: 0x55c3071778b0 class=QCalendarPopup name=qt_datetimedit_calendar
JOBPREP_DEBUG_DATEPOPUP:   chain[3]: 0x55c306cb4c20 class=QDateEdit name=InterviewDateEdit
ARROWS_TEST: control=InterviewDateEdit method=0 focusWidget=0x55c307170350 (QtPrivate::QCalendarView, name=qt_calendar_calendarview) calTable=0x55c307170350
PASS   : tst_Shortcuts::testDateCalendarPopupArrows(InterviewDateEdit)
PASS   : tst_Shortcuts::testDateCalendarPopupArrows(ApplicationAppliedDateEdit)
PASS   : tst_Shortcuts::testDateCalendarPopupArrows(ApplicationNextActionDateEdit)
PASS   : tst_Shortcuts::testDateCalendarPopupArrows(SessionDateEdit)
ARROWS_TEST: control=TopicDetailPanelDateEdit method=0 focusWidget=0x55c306eb0d60 (QtPrivate::QCalendarView, name=qt_calendar_calendarview) calTable=0x55c306eb0d60
ARROWS_TEST: control=TopicDetailPanelDateEdit method=1 focusWidget=0x55c306eb0d60 (QtPrivate::QCalendarView, name=qt_calendar_calendarview) calTable=0x55c306eb0d60
ARROWS_TEST: control=TopicDetailPanelDateEdit method=2 focusWidget=0x55c306eb0d60 (QtPrivate::QCalendarView, name=qt_calendar_calendarview) calTable=0x55c306eb0d60
PASS   : tst_Shortcuts::testDateCalendarPopupArrows(TopicDetailPanelDateEdit)
PASS   : tst_Shortcuts::cleanupTestCase()
Totals: 7 passed, 0 failed, 0 skipped, 0 blacklisted, 1857ms
********* Finished testing of tst_Shortcuts *********
```
Slot duration: **1857 ms**.

---

## 3. Item 2: Language Selector Item Gender in Russian

### Inspection and Changes

Command executed:
```bash
grep -n "System Default" resources/translations/*.ts
```
Raw Output:
```
resources/translations/jobprep_ru.ts:1480:        <source>System Default</source>
resources/translations/jobprep_uk.ts:1480:        <source>System Default</source>
```

Command executed:
```bash
grep -n "Системн\|системн" resources/translations/*.ts
```
Raw Output:
```
resources/translations/jobprep_ru.ts:1465:        <translation>Оставлять JobPrep работать в системном трее при закрытии окна</translation>
resources/translations/jobprep_ru.ts:1469:        <translation>Системный трей недоступен в этой системе</translation>
resources/translations/jobprep_ru.ts:1481:        <translation>Системный (по умолчанию)</translation>
resources/translations/jobprep_ru.ts:1570:        <translation>Сворачивать / закрывать в системный трей</translation>
resources/translations/jobprep_ru.ts:2227:        <translation>Системный дизайн</translation>
resources/translations/jobprep_uk.ts:1465:        <translation>Залишати JobPrep працювати в системному треї під час закриття вікна</translation>
resources/translations/jobprep_uk.ts:1469:        <translation>Системний трей недоступний у цій системі</translation>
resources/translations/jobprep_uk.ts:1481:        <translation>Системна за замовчуванням</translation>
resources/translations/jobprep_uk.ts:1570:        <translation>Згортати / закривати в системний трей</translation>
resources/translations/jobprep_uk.ts:2227:        <translation>Системний дизайн</translation>
```

- In Russian: `Системная по умолчанию` was corrected to **`Системный (по умолчанию)`** (masculine, agreeing with masculine noun "язык").
- In Ukrainian: `Системна за замовчуванням` is correct (feminine, agreeing with feminine noun "мова") and remains unchanged.
- Documentation in `docs/i18n-glossary.md` and `docs/reports/i18n-review.md` updated to match.

Raw translation diff across commits:
```bash
git diff -U0 origin/main..HEAD resources/translations | grep -E '^[-+] *<(translation|numerusform)'
```
Raw Output:
```
-        <translation>Системная по умолчанию</translation>
+        <translation>Системный (по умолчанию)</translation>
-        <translation>Еженедельная учебная цель в часах и минутах (например: 8, 7:30, 7;30, 450 min; числа или десятичные дроби означают часы)</translation>
+        <translation>Еженедельная учебная цель в часах и минутах (например: 8, 7:30, 7;30, 450 мин; числа или десятичные дроби означают часы)</translation>
-        <translation>Щотижнева навчальна ціль у годинах і хвилинах (наприклад: 8, 7:30, 7;30, 450 min; числа чи десяткові дроби означають години)</translation>
+        <translation>Щотижнева навчальна ціль у годинах і хвилинах (наприклад: 8, 7:30, 7;30, 450 хв; числа чи десяткові дроби означають години)</translation>
```

---

## 4. Item 3: Weekly Goal: Cyrillic Minute Units & Parser Hardening

### Parser Implementation & Behavior

Code in `src/ui/pages/SettingsPage.cpp`:
```cpp
std::optional<int> parseGoalMinutes(const QString& text) {
    const QString clean = text.trimmed();
    if (clean.isEmpty()) return std::nullopt;
    const QString lower = clean.toLower();

    // 1. Explicit minutes with suffix: e.g. "90 min", "90min", "90 mins", "90 minute", "90 minutes",
    //    "150мин", "150 мин", "150 хв", "150хв", "150 минут", "150 минуты", "150 хвилин", "150 хвилини",
    //    case-insensitively with optional trailing dot. 'm' is ambiguous and explicitly not accepted.
    static const QRegularExpression minOnlyRe(
        uR"(^(\d+)\s*(?:min(?:s|utes?)?|мин(?:ут[ы]?)?|хв(?:илин[и]?)?)\.?$)"_s,
        QRegularExpression::CaseInsensitiveOption);
    auto match = minOnlyRe.match(lower);
    if (match.hasMatch()) {
        bool ok = false;
        const int mins = match.captured(1).toInt(&ok);
        if (!ok) return std::nullopt;
        const int rounded = static_cast<int>(std::round(static_cast<double>(mins) / 15.0)) * 15;
        return std::clamp(rounded, 60, 2400);
    }
    // ...
}
```

**Unit Handling & Error Behavior:**
- **Before:** `150мин` and `150 хв` did not match the English-only `minOnlyRe` (`min|mins|minutes?`). The parser fell through to `hoursOnlyRe`, extracting digits `150` as hours (9000 minutes), which was then capped at 40 hours (2400 minutes).
- **After:** `150мин` and `150 хв` match `minOnlyRe`, evaluating to 150 minutes (2 h 30 min).
- **Invalid Units:** Unrecognised unit suffixes (e.g. `150x`, `150m`, `мин150`, `2г`, `2 ч`) return `std::nullopt`. `GoalSpinBox::valueFromText` preserves the prior valid value (`parsed.value_or(value())`), and `fixup()` restores the display string, completely eliminating silent incorrect guessing.
- **Accepted Hour Units:** Currently accepted hour suffixes are English only: `h`, `hrs`, `hr`, `hours`, `hour`.
- **Hour Suffix Proposal:** In a future update, support Ukrainian (`г`, `год`, `годин`, `години`) and Russian (`ч`, `час`, `часов`, `часа`) hour suffixes to allow symmetric localized hour typing.
- **Hint Updates:** In `jobprep_uk.ts` and `jobprep_ru.ts`, the hint examples were updated from `450 min` to `450 хв` and `450 мин`, respectively. English source remains unchanged.

### Test Execution

Command executed:
```bash
/home/tony/projects/JobPrep/build/dev-linux/tests/ui/tst_PageSettings testGoalSpinBoxMinuteUnitsUkRu
```
Raw Output:
```
********* Start testing of tst_PageSettings *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_PageSettings::initTestCase()
PASS   : tst_PageSettings::testGoalSpinBoxMinuteUnitsUkRu()
PASS   : tst_PageSettings::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 102ms
********* Finished testing of tst_PageSettings *********
```
Slot duration: **102 ms**.

---

## 5. Item 4: Dark Theme Check Box Contrast

### Root Cause Analysis

Command executed:
```bash
grep -rn "indicator\|ItemIsUserCheckable\|CheckState" src resources
```
Raw Output:
```
src/ui/widgets/TopicListView.h:14:/// - Full-column drag-over highlight and empty-column drop indicator
src/ui/widgets/TopicDetailPanel.cpp:495:            if (item && (item->flags() & Qt::ItemIsUserCheckable)) {
src/ui/widgets/TopicDetailPanel.cpp:496:                item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
src/ui/widgets/TopicDetailPanel.cpp:680:        item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
src/ui/widgets/TopicDetailPanel.cpp:681:        item->setCheckState(subtask.done ? Qt::Checked : Qt::Unchecked);
resources/styles/theme.qss:175:/* CheckBox and Checkable item indicators */
resources/styles/theme.qss:185:QCheckBox::indicator,
resources/styles/theme.qss:186:QListView::indicator,
resources/styles/theme.qss:187:QListWidget::indicator,
resources/styles/theme.qss:188:#StudyDetailCheckList::indicator {
```

Checklist items in `TopicDetailPanel` are managed by a `QListWidget` (`StudyDetailCheckList`) using `Qt::ItemIsUserCheckable`. Without explicit QSS styling for `::indicator`, Qt rendered default Fusion indicators using dark base colors on the dark `#161b22` card background. This resulted in an unchecked indicator contrast of only **1.03:1 to 1.12:1** (nearly invisible).

### Styling Fix

Added central theme rules in `resources/styles/theme.qss` for `QCheckBox::indicator`, `QListView::indicator`, `QListWidget::indicator`, and `#StudyDetailCheckList::indicator`:
- **Unchecked:** 2px solid border using theme `@accent` on `@surface` neutral fill.
- **Checked:** 2px solid `@accent` border, `@accent` fill, and a crisp white SVG checkmark (`:/icons/check.svg`).
- **Hover / Focus:** Border changes to `@accentHover` or `@accent`.
- **Disabled:** Dimmed `@textMuted` border and `@surfaceAlt` fill.

### Contrast Test Results (WCAG 3:1 Component Threshold)

Automated verification in `tst_StudyRegression::testCheckboxIndicatorContrastAcrossAppDarkAndLight` renders the checklist, Settings checkbox, and ApplicationDialog checkboxes under Dark and Light themes at 1.0x and 1.5x scales:

Command executed:
```bash
/home/tony/projects/JobPrep/build/dev-linux/tests/ui/tst_StudyRegression testCheckboxIndicatorContrastAcrossAppDarkAndLight
```
Raw Output:
```
********* Start testing of tst_StudyRegression *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_StudyRegression::initTestCase()
BEFORE FIX CONTRAST: mode=dark scale=1.0 checked=10.72:1 unchecked=1.12:1
CONTRAST CHECKLIST AFTER: mode=dark scale=1.0 checked=18.87:1 unchecked=6.23:1
DEBUG SETTINGS CB: checked=0 indRect=(0,0 20x20) cbSize=(488x20) bgCb=#0f1117
DEBUG SETTINGS CB: checked=1 indRect=(0,0 20x20) cbSize=(488x20) bgCb=#0f1117
CONTRAST SETTINGS CHECKBOX: mode=dark scale=1.0 checked=18.87:1 unchecked=6.23:1
DEBUG SETTINGS CB: checked=0 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#0f1117
DEBUG SETTINGS CB: checked=1 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#0f1117
DEBUG SETTINGS CB: checked=0 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#0f1117
DEBUG SETTINGS CB: checked=1 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#0f1117
CONTRAST APP DIALOG CHECKBOXES: mode=dark scale=1.0 applied=(18.87, 6.23) next=(18.87, 6.23)
CONTRAST CHECKLIST AFTER: mode=dark scale=1.5 checked=18.87:1 unchecked=6.23:1
DEBUG SETTINGS CB: checked=0 indRect=(0,0 20x20) cbSize=(488x20) bgCb=#0f1117
DEBUG SETTINGS CB: checked=1 indRect=(0,0 20x20) cbSize=(488x20) bgCb=#0f1117
CONTRAST SETTINGS CHECKBOX: mode=dark scale=1.5 checked=18.87:1 unchecked=6.23:1
DEBUG SETTINGS CB: checked=0 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#0f1117
DEBUG SETTINGS CB: checked=1 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#0f1117
DEBUG SETTINGS CB: checked=0 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#0f1117
DEBUG SETTINGS CB: checked=1 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#0f1117
CONTRAST APP DIALOG CHECKBOXES: mode=dark scale=1.5 applied=(18.87, 6.23) next=(18.87, 6.23)
BEFORE FIX CONTRAST: mode=light scale=1.0 checked=15.56:1 unchecked=1.13:1
CONTRAST CHECKLIST AFTER: mode=light scale=1.0 checked=3.90:1 unchecked=3.90:1
DEBUG SETTINGS CB: checked=0 indRect=(0,0 20x20) cbSize=(488x20) bgCb=#f6f7fb
DEBUG SETTINGS CB: checked=1 indRect=(0,0 20x20) cbSize=(488x20) bgCb=#f6f7fb
CONTRAST SETTINGS CHECKBOX: mode=light scale=1.0 checked=3.90:1 unchecked=3.90:1
DEBUG SETTINGS CB: checked=0 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#f6f7fb
DEBUG SETTINGS CB: checked=1 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#f6f7fb
DEBUG SETTINGS CB: checked=0 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#f6f7fb
DEBUG SETTINGS CB: checked=1 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#f6f7fb
CONTRAST APP DIALOG CHECKBOXES: mode=light scale=1.0 applied=(3.90, 3.90) next=(3.90, 3.90)
CONTRAST CHECKLIST AFTER: mode=light scale=1.5 checked=3.90:1 unchecked=3.90:1
DEBUG SETTINGS CB: checked=0 indRect=(0,0 20x20) cbSize=(488x20) bgCb=#f6f7fb
DEBUG SETTINGS CB: checked=1 indRect=(0,0 20x20) cbSize=(488x20) bgCb=#f6f7fb
CONTRAST SETTINGS CHECKBOX: mode=light scale=1.5 checked=3.90:1 unchecked=3.90:1
DEBUG SETTINGS CB: checked=0 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#f6f7fb
DEBUG SETTINGS CB: checked=1 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#f6f7fb
DEBUG SETTINGS CB: checked=0 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#f6f7fb
DEBUG SETTINGS CB: checked=1 indRect=(0,0 20x20) cbSize=(48x20) bgCb=#f6f7fb
CONTRAST APP DIALOG CHECKBOXES: mode=light scale=1.5 applied=(3.90, 3.90) next=(3.90, 3.90)
PASS   : tst_StudyRegression::testCheckboxIndicatorContrastAcrossAppDarkAndLight()
PASS   : tst_StudyRegression::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 4160ms
********* Finished testing of tst_StudyRegression *********
```
Slot duration: **4160 ms**.

**Visual Artifacts Saved:**
- `docs/reports/images/checklist-before-dark.png`
- `docs/reports/images/checklist-after-dark.png`
- `docs/reports/images/checklist-before-light.png`
- `docs/reports/images/checklist-after-light.png`

*(Note: Captured via offscreen QPA rendering; offscreen pixel rasterization is not identical to physical display hardware output on Windows, but provides objective contrast ratios.)*

### Layout & Translation Test Verification

Command executed:
```bash
ctest --preset dev-linux -R "tst_Translations|ui_smoke|tst_HiDpi" --output-on-failure
```
Raw Output:
```
Test project /home/tony/projects/JobPrep/build/dev-linux
    Start 10: ui_smoke
1/3 Test #10: ui_smoke .........................   Passed   12.30 sec
    Start 34: tst_HiDpi
2/3 Test #34: tst_HiDpi ........................   Passed   18.55 sec
    Start 35: tst_Translations
3/3 Test #35: tst_Translations .................   Passed    0.07 sec

100% tests passed, 0 tests failed out of 3

Label Time Summary:
i18n    =   0.07 sec*proc (1 test)
ui      =  30.85 sec*proc (2 tests)

Total Test time (real) =  30.93 sec
```

---

## 6. Predictions per CI Job by Real Name

1. **`Gate (ubuntu-latest, Qt 6.8.2)`**:
   - **Prediction:** **PASS (Green)**
   - **Timing Impact:** The additions add ~6 s of Linux test time (`testDateCalendarPopupArrows`: 1.86 s, `testCheckboxIndicatorContrastAcrossAppDarkAndLight`: 4.16 s, `testGoalSpinBoxMinuteUnitsUkRu`: 0.10 s). Job runtime remains ~6 minutes.
2. **`Gate (ubuntu-latest, Qt 6.10.3)`**:
   - **Prediction:** **PASS (Green)**
   - **Translation Sync:** Translation files (`jobprep_uk.ts`, `jobprep_ru.ts`) remain strictly synchronized with code and `lrelease` emits 0 warnings.
3. **`Gate (windows-latest, Qt 6.10.3)`**:
   - **Prediction:** **PASS (Green)**
   - **Timing Impact on Slowest Windows Tests:**
     - `tst_PageStudy`: Expected **100–120 s** (no new test slots added to this target; remains within historical range).
     - `tst_HiDpi`: Expected **150–165 s** (remains unaffected).
     - `tst_PageHome`: Expected **120–135 s** (remains unaffected).
     - `ui_smoke`: Expected **105–120 s** (remains unaffected).
     - `tst_Shortcuts`: Expected **~45–55 s** (+10–12 s for 5 popup controls x 3 opening methods).
     - `tst_StudyRegression`: Expected **~65–75 s** (+20–25 s for contrast rasterization across 2 themes and 2 scale factors).
     - Total Windows CTest time is projected to increase from ~650 s to ~685 s, well within the 45-minute timeout.
4. **`Gate (windows-latest, Qt 6.8.2)`** *(manual dispatch / weekly schedule only)*:
   - **Prediction:** **PASS (Green)** when manually triggered.

---

## 7. Items Not Verified by Agent (User to Check on Windows 10)

Because this agent operates headless on Linux under `QT_QPA_PLATFORM=offscreen`, the following Windows 10 user-facing items are **not verified by agent** and require human confirmation:
1. **Study page popup arrows:** Open the date popup in `TopicDetailPanel` using `Alt+Down`, `F4`, or a mouse click on the drop-down button; verify that arrow keys (`Down`, `Right`, `PageDown`) navigate the calendar immediately without clicking a date first. Also re-verify the other four date popups (`ApplicationDialog` applied & next action dates, `InterviewDialog` date, `SessionDialog` date).
2. **Russian language selector item:** In Settings -> General -> Language, verify that the default item reads **`Системный (по умолчанию)`** (masculine form).
3. **Weekly study goal Cyrillic input:** In Settings -> General -> Weekly study goal, verify typing `150мин` and `150 хв` evaluates to `2 h 30 min` (clamped/rounded), and verify that invalid syntax (e.g. `150x`) is rejected.
4. **Dark theme check boxes:** In Settings, switch to Dark theme and verify that the subtask check boxes in `TopicDetailPanel` (unchecked and checked states) and all modal dialog checkboxes are distinctly visible at 100% and 150% display scaling.


---

## 8. Verification Commands, Raw Output & Git Status

### Full `./scripts/check.sh` Run

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
[1/4] Automatic MOC and UIC for target tst_StudyRegression
[2/3] Building CXX object tests/ui/CMakeFiles/tst_StudyRegression.dir/tst_StudyRegression.cpp.o
[3/3] Linking CXX executable tests/ui/tst_StudyRegression
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start 30: tst_PageHome
      Start 27: tst_PageJobs
 1/35 Test #27: tst_PageJobs .....................   Passed   13.10 sec
      Start 28: tst_PageCalendar
 2/35 Test #30: tst_PageHome .....................   Passed   16.98 sec
      Start 34: tst_HiDpi
 3/35 Test #28: tst_PageCalendar .................   Passed    7.19 sec
      Start 29: tst_PageStudy
 4/35 Test #34: tst_HiDpi ........................   Passed   19.03 sec
      Start 33: tst_EmptyStatesAndFocus
 5/35 Test #29: tst_PageStudy ....................   Passed   19.57 sec
      Start 32: tst_Shortcuts
 6/35 Test #33: tst_EmptyStatesAndFocus ..........   Passed   11.08 sec
      Start 25: tst_Notifiers
 7/35 Test #32: tst_Shortcuts ....................   Passed    8.00 sec
      Start 26: tst_StudyRegression
 8/35 Test #25: tst_Notifiers ....................   Passed    2.58 sec
      Start 31: tst_PageSettings
 9/35 Test #31: tst_PageSettings .................   Passed    4.92 sec
      Start 10: ui_smoke
10/35 Test #26: tst_StudyRegression ..............   Passed   10.05 sec
      Start 24: tst_StatsService
11/35 Test #24: tst_StatsService .................   Passed    0.97 sec
      Start 21: tst_ImportExport
12/35 Test #21: tst_ImportExport .................   Passed    0.68 sec
      Start  8: tst_SeedService
13/35 Test  #8: tst_SeedService ..................   Passed    0.27 sec
      Start 23: tst_SettingsService
14/35 Test #23: tst_SettingsService ..............   Passed    0.02 sec
      Start 22: tst_ReminderService
15/35 Test #22: tst_ReminderService ..............   Passed    0.08 sec
      Start 17: tst_TopicTableProxy
16/35 Test #17: tst_TopicTableProxy ..............   Passed    0.07 sec
      Start 18: tst_AgendaService
17/35 Test #18: tst_AgendaService ................   Passed    0.09 sec
      Start  5: tst_ApplicationRepository
18/35 Test  #5: tst_ApplicationRepository ........   Passed    0.09 sec
      Start 15: tst_TopicListModel
19/35 Test #15: tst_TopicListModel ...............   Passed    0.04 sec
      Start 12: tst_AgendaListModel
20/35 Test #12: tst_AgendaListModel ..............   Passed    0.03 sec
      Start  6: tst_InterviewRepository
21/35 Test  #6: tst_InterviewRepository ..........   Passed    0.03 sec
      Start 13: tst_ApplicationTableModel
22/35 Test #13: tst_ApplicationTableModel ........   Passed    0.06 sec
      Start 19: tst_DayWatcher
23/35 Test #19: tst_DayWatcher ...................   Passed    0.01 sec
      Start  3: tst_TopicRepository
24/35 Test  #3: tst_TopicRepository ..............   Passed    0.08 sec
      Start 16: tst_TopicBoardProxy
25/35 Test #16: tst_TopicBoardProxy ..............   Passed    0.05 sec
      Start  4: tst_SessionRepository
26/35 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start 14: tst_ApplicationFilterProxy
27/35 Test #14: tst_ApplicationFilterProxy .......   Passed    0.02 sec
      Start  2: tst_TrackRepository
28/35 Test  #2: tst_TrackRepository ..............   Passed    0.02 sec
      Start 11: tst_MonthView
29/35 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start  7: tst_ReminderLogRepository
30/35 Test  #7: tst_ReminderLogRepository ........   Passed    0.02 sec
      Start 35: tst_Translations
31/35 Test #35: tst_Translations .................   Passed    0.07 sec
      Start  9: tst_TransactionBehavior
32/35 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start  1: tst_Database
33/35 Test  #1: tst_Database .....................   Passed    0.03 sec
      Start 20: tst_ExportService
34/35 Test #20: tst_ExportService ................   Passed    0.01 sec
35/35 Test #10: ui_smoke .........................   Passed   12.47 sec

100% tests passed, 0 tests failed out of 35

Label Time Summary:
data       =   0.62 sec*proc (9 tests)
i18n       =   0.07 sec*proc (1 test)
model      =   0.27 sec*proc (6 tests)
service    =   1.85 sec*proc (7 tests)
ui         = 125.05 sec*proc (12 tests)

Total Test time (real) =  67.06 sec
==> lupdate not found, skipping translation sync check.
==> Quality check PASSED with zero warnings and all tests green!
```

### Working Tree & Worktrees

Command executed:
```bash
git status --short
```
Raw Output:
```
(no output)
```

Command executed:
```bash
git worktree list
```
Raw Output:
```
/home/tony/projects/JobPrep 7fce164 [main]
```

### Git Log of Manual Fixes Work

Command executed:
```bash
git log --oneline origin/main..HEAD
```
Raw Output:
```
7fce164 test(ui): write checklist contrast screenshots only when absent
e4c01bc docs(reports): append M7 manual-check fixes report to M7-partD (6)
cdec129 docs: document accepted weekly goal units in RUNNING.md and record SPEC §14 changelog (5)
e6e3cb9 feat(theme): style check box indicators with accent color for dark contrast (4)
ec3aac1 feat(settings): parse Ukrainian and Russian minute units in weekly goal (3)
40e6b0f i18n: fix Russian gender agreement for system language item (2)
769ffbf fix(ui): ensure calendar popup view receives explicit keyboard focus (1)
```
