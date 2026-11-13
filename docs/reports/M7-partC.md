# M7 Part C Report: Translation Infrastructure, Audit, and Pseudo-Translation Verification

**Date:** 2026-11-13  
**Status:** Complete (Ready for User Review / Part D)  
**Platform / Environment:** Linux x86_64 (Ubuntu 26.04), GCC 15.2.0, Qt 6.10.2 shared, CMake 3.31.6, Ninja 1.13.1  
**Build Configuration:** `dev-linux` preset (`-g -std=c++20 -Wall -Wextra -Wpedantic -Wshadow -Werror -DJOBPREP_WERROR=ON`)  
**Parallelism:** `CMAKE_BUILD_PARALLEL_LEVEL=2`  

---

## 1. Facts as Given & Decisions

- **Baseline:** Part B closed green across CI (Ubuntu 6.8.2, Ubuntu 6.10.3, Windows 6.10.3). Offscreen filter suppresses native style `OpenThemeData` warnings on Windows 10/11.
- **Decisions (Binding):**
  1. Default language resolves to system UI language if `uk` or `ru`, otherwise falls back to English (`en`).
  2. Setting values in `SettingsService`: `system`, `en`, `uk`, `ru` (stored under `ui/language`, empty/unknown resolves to `system`).
  3. Language switching takes effect upon application restart (inline note in `SettingsPage`).
  4. Language installation occurs **strictly before any widget or MainWindow instantiation** via `installLanguage()`.
  5. Sample dataset remains in English.
  6. Exports (CSV, ICS, JSON) stay language-neutral and byte-equivalent across locales.
  7. Weekly-goal spinbox typing rules remain strictly locale-independent (`7.30` = 7:30, `7,30` = 7:30, `7.5` = 7:50, `7.75` = error).
  8. Language selector displays native language names without translation: `English`, `Українська`, `Русский`.
  9. English UI behavior remains 100% identical as before. Real `uk` and `ru` translations are deferred to Part D.

---

## 2. C0 Translation Audit & Reconciliation

### C0.1 Exact Translation Call Counts

Command executed over `src/`:
```bash
grep -rnE "(^|[^A-Za-z_])tr\(|QCoreApplication::translate|[^A-Za-z_]translate\(|QT_TR_NOOP|QT_TRANSLATE_NOOP" src
```
**Raw Counts in `src/`:**
- `tr(` calls: 485
- `QCoreApplication::translate(` / `translate(` calls: 51
- `QT_TR_NOOP` / `QT_TRANSLATE_NOOP`: 0
- **Total translation calls in `src/`:** 536 across 31 files (out of 153 inspected files).

Command executed over `tests/`:
```bash
grep -rnE "(^|[^A-Za-z_])tr\(|QCoreApplication::translate|[^A-Za-z_]translate\(|QT_TR_NOOP|QT_TRANSLATE_NOOP" tests
```
**Raw Counts in `tests/`:**
- `tr(` calls: 13
- `translate(` calls: 2
- **Total translation calls in `tests/`:** 15 across 3 files (out of 48 inspected files; tests are not translated).

**Reconciliation:**
- Initial draft count (472) omitted non-page UI classes (`FormatHelper`, `Toast`, `TrayNotifier`, `StatusStyle`).
- Amended plan count (482 + 50 = 532).
- Part B added 4 calls in `src/ui/widgets/SingleTabDateTime.h` (3 `tr(` including class declaration, 1 `translate`), bringing the exact total to **536**.

### C0.2 Classes Calling `tr()` Without `Q_OBJECT`

Scanned classes calling `tr()` without `Q_OBJECT`:
1. `GoalSpinBox` (`src/ui/pages/SettingsPage.cpp`): Inherited `QSpinBox` without `Q_OBJECT`. `lupdate` warned and assigned context `JobPrep::Ui::Pages::GoalSpinBox`, while runtime Qt invoked base-class `QSpinBox::tr()`.
   - **Verdict:** Real context defect.
   - **Fix:** Switched calls to `QCoreApplication::translate("GoalSpinBox", "%1 h")` and `QCoreApplication::translate("GoalSpinBox", "%1 h %2 min")`.
   - **Verification:** Proven with `RecordingTranslator` in `tst_PageSettings::testGoalSpinBoxTranslationContext`.
2. `SingleTabDateEdit` (`src/ui/widgets/SingleTabDateTime.h`): Inherited `QDateEdit` without `Q_OBJECT`.
   - **Verdict:** Real context defect.
   - **Fix:** Switched to `QCoreApplication::translate("SingleTabDateEdit", "Press Alt+Down or F4 to open calendar")`.

### C0.3 Untranslated Literals Reaching the UI (E3)

Audited 229 hits across all UI setters (`setText`, `setWindowTitle`, `setToolTip`, `setStatusTip`, `setPlaceholderText`, `addItem`, `headerData`, `QLabel`, `QPushButton`, `QAction`, `drawText`):
- All 229 hits were evaluated: 100% are visual glyphs (`"—"`, `" · "`, `"+"`, `"×"`), numeric formats (`"%1"`), technical URLs/QSS identifiers, or settings keys.
- **Verdict:** Zero missing translations for end-user text.

### C0.4 Concatenation & Placeholders

Command:
```bash
grep -rnE '\+ *tr\(|tr\([^)]*\) *\+' src/
```
**Raw output:**
```
(no output)
```
- **Verdict:** Zero concatenated sentences in code. All dynamic texts use parametrized `.arg()` placeholders.

### C0.5 Static & Early Translation

Command:
```bash
grep -rnE "static +(const +)?[A-Za-z:<>_ ,]+.*(tr\(|translate\()" src/
```
**Raw output:**
```
(no output)
```
- **Verdict:** Zero global or static translation calls executed before `main()` or before `installLanguage()`.

### C0.6 Plurals Inventory & Implementation

Audit identified 10 count-dependent call sites:
- `src/ui/FormatHelper.cpp`: `formatLeadTime` (days, hours, minutes), `formatActiveProcesses`, `formatStreak`
- `src/ui/NotificationText.cpp`: `formatLeadTime` (hours, minutes), `formatRemainingTime` (minutes, hours), `formatMessage` (overdue follow-up count)
- `src/ui/InAppNotifier.cpp`: `updateLayout` (`+%n more`)
- `src/ui/dialogs/TrackDialog.cpp`: `onDeleteClicked` (track deletion topic count)

**Implementation & English Fallback Preservation:**
To ensure the English UI behaves 100% identically to before while exposing `numerus="yes"` messages to `lupdate` for `uk` and `ru`, calls invoke `tr("%n ...", "", count)`. If the returned string equals the untranslated English fallback, the clean English singular/plural (`1 day` / `2 days`) is returned. When a translated catalog or test translator is active, the catalog's plural form is returned.

### C0.7 Dates, Numbers & Locales

- `src/ui/widgets/MonthView.cpp`: Replaced `englishLocale()` with default `QLocale()`. Added `static QString monthTitle(int year, int month)` and `monthTitle() const` with standalone title-casing. Weekday headers capitalized and formatted via `QLocale().dayName(..., QLocale::ShortFormat)`.
- `src/ui/delegates/AgendaItemDelegate.cpp`: Updated `dateVal.toString(u"ddd "_s)` to `QLocale().toString(dateVal, u"ddd"_s) + u" "_s` with title-casing.
- **First Day of Week:** Retains Monday default as established in Settings (`m_settings.firstDayOfWeek()`), completely preserving user control.

### C0.8 Keyboard Mnemonics & Shortcuts

- English dialogs contain 16 `&` mnemonics (`&Save`, `&Delete`, etc.).
- Cyrillic UI (`uk`, `ru`) will intentionally omit all `&` mnemonics.
- Marked as: **Not verified by agent, user to check** (shortcuts behavior under non-Latin layouts).

---

## 3. Sections C1 through C7 Detailed Results

### C1. Tooling & Build (`LinguistTools`)

- `lupdate version 6.10.2`, `lrelease version 6.10.2`.
- Options selected:
  - `-no-obsolete`: drops obsolete strings to keep catalogs clean.
  - `-locations none`: strips line numbers from `.ts` to avoid Git churn on every line edit.
  - `-source-language en`: explicitly marks source strings as English.
  - `-target-language uk` / `ru`: targets Ukrainian and Russian.
- Resources: `resources/translations/jobprep_uk.ts` and `jobprep_ru.ts` compiled to `.qm` and embedded into `jobprep_core` under `/i18n/`.
- Proof of resource embedding:
  ```bash
  nm -C build/dev-linux/jobprep | grep -i "jobprep_core_translations"
  ```
  Output:
  ```
  0000000000109720 r qInitResources_jobprep_core_translations()::initializer
  0000000000088820 t qInitResources_jobprep_core_translations()
  0000000000088860 t qCleanupResources_jobprep_core_translations()
  ```
- Sync verification script: `scripts/check-translations.sh` verifies working directory `.ts` against source code in a tempdir without touching tracked files. Fails with code 1 if out of sync, passes with 0 when synchronized. Wired into `scripts/check.sh`.

### C2. Language Settings & Resolution

- Added `ui/language` key in `SettingsService` (`system`, `en`, `uk`, `ru`).
- Pure function `resolveLanguage(setting, uiLanguages)` implemented and unit tested:
  - `system` with `uk-UA` -> `uk`
  - `system` with `ru-RU` -> `ru`
  - `system` with `en-US`, `pl-PL`, `be-BY` -> `en`
  - Explicit settings `en`, `uk`, `ru` override system locale.
- `installLanguage()` installs catalog and `QLocale::setDefault()`.
- Verified line order in `src/main.cpp`:
  ```cpp
  const QString effectiveLang = JobPrep::App::installLanguage(settings);
  // ...
  MainWindow mainWindow(ctx);
  ```
  `installLanguage()` executes strictly before `MainWindow` is created.
- `SettingsPage`: Language selector combobox populated with untranslated native names: `English`, `Українська`, `Русский`. Shows inline restart notice when value changes.

### C3. Qt Standard Strings & Diagnostics

- Inventory: `QMessageBox::StandardButton`, `QDialogButtonBox`, `QFileDialog`, `QLineEdit` context menus.
- System search for `qtbase_uk.qm`:
  - Path: `/usr/share/qt6/translations/qtbase_uk.qm`
  - `QLibraryInfo::path(QLibraryInfo::TranslationsPath)` prints `/usr/share/qt6/translations`.
- Guard: `installLanguage()` checks `QFile::exists()`. When absent, skips loading silently without warning or crash.
- CI Workflow: Updated `.github/workflows/ci.yml` across Linux and Windows `CI-DIAG` steps to output availability of `<QT_PREFIX_PATH>/translations/qtbase_uk.qm` and `qtbase_ru.qm`.

### C4. Locale-Aware Formatting & Neutrality

- Month & Weekday: `MonthView::monthTitle(2026, 5)` returns `"Травень 2026"` under Ukrainian locale and `"Май 2026"` under Russian locale. Agenda day names format as `"пн"` instead of `"Mon"`.
- Weekly Goal Typing Rules: Verified under `en`, `uk`, `ru` locales in `tst_PageSettings::testGoalSpinBoxTypingRulesUnderLocales`:
  - `7.30` -> 450 min (7:30)
  - `7,30` -> 450 min (7:30)
  - `7.5` -> 465 min (7:45)
  - `7,5` -> 465 min (7:45)
  - `7.75` -> rejected (error, value restored)
- Export Neutrality: Verified in `tst_ImportExport::testExportsStayNeutralUnderUkLocale`:
  - CSV bytes identical between `en` and `uk`.
  - ICS bytes identical between `en` and `uk`.
  - JSON backup bytes identical between `en` and `uk`.
  - Database backed up under `uk` and restored under `en` matches byte-for-byte across all tables.

### C5. Pseudo-Translation & Layout Verification

- `JobPrep::UiTest::PseudoTranslator` expands English source text by ~40% with Cyrillic lookalikes while preserving `%1..%9`, `%n`, HTML tags, and accelerators.
- Integrated into `ui_smoke` (`testPseudoTranslationSmoke`) and `tst_HiDpi` (`testPseudoTranslationHiDpi`).
- Executed at 100% and 150% scaling across Light and Dark themes, loading all 5 pages and modal dialogs (`ApplicationDialog`, `InterviewDialog`, `FollowUpDialog`, `TrackDialog`, `SessionDialog`).
- **Results:** 0 clipping violations, 0 text advance overflows, 0 layout collisions, 0 warnings.

### C6. Translation Test Suite (`tst_Translations`)

- Added `tests/i18n/tst_Translations.cpp` and fixtures in `tests/i18n/fixtures/`.
- Completeness Check: Validates `.ts` XML structure. Skips real files with count when `JOBPREP_REQUIRE_COMPLETE_TRANSLATIONS` is unset (Part C baseline); enforces completeness when set to 1.
- Proven failure detection on test fixtures:
  - `broken_unfinished_uk.ts`: detected 1 unfinished message.
  - `broken_empty_uk.ts`: detected 1 empty translation tag.
  - `broken_numerus_uk.ts`: detected 1 message with invalid plural form count (2 instead of 3).
- Slavic Plural Mechanism Test:
  - Verified forms for `1, 2, 5, 11, 21, 22, 25, 101` in Ukrainian and Russian using compiled fixtures:
    - `1`, `21`, `101` -> Form 0 (one): `1 день` / `1 час` / `1 тему`
    - `2`, `22` -> Form 1 (few): `2 дні` / `2 часа` / `2 теми`
    - `5`, `11`, `25` -> Form 2 (many): `5 днів` / `5 часов` / `5 тем`
  - Covered real call sites: `FormatHelper`, `NotificationText`, `InAppNotifier`, `TrackDialog`.

### C7. Documentation & Glossary

- Generated `docs/i18n-glossary.md` with 321 terms derived from code via `scripts/build_glossary.py`.
- Updated `docs/RUNNING.md` with section on localization and languages.
- Appended entry `1.27` to `docs/SPEC.md` §14 changelog.

---

## 4. Test Inventory & Slot Durations (Linux)

All 35 test suites pass headless on offscreen platform:

| Test Target | Category | Duration | Status |
| :--- | :--- | :--- | :--- |
| `tst_Database` | data | 0.03 s | Passed |
| `tst_TrackRepository` | data | 0.03 s | Passed |
| `tst_TopicRepository` | data | 0.10 s | Passed |
| `tst_SessionRepository` | data | 0.03 s | Passed |
| `tst_ApplicationRepository` | data | 0.09 s | Passed |
| `tst_InterviewRepository` | data | 0.03 s | Passed |
| `tst_ReminderLogRepository` | data | 0.02 s | Passed |
| `tst_SeedService` | data | 0.27 s | Passed |
| `tst_TransactionBehavior` | data | 0.03 s | Passed |
| `tst_AgendaListModel` | model | 0.03 s | Passed |
| `tst_ApplicationTableModel` | model | 0.06 s | Passed |
| `tst_ApplicationFilterProxy` | model | 0.02 s | Passed |
| `tst_TopicListModel` | model | 0.04 s | Passed |
| `tst_TopicBoardProxy` | model | 0.05 s | Passed |
| `tst_TopicTableProxy` | model | 0.07 s | Passed |
| `tst_AgendaService` | service | 0.09 s | Passed |
| `tst_DayWatcher` | service | 0.01 s | Passed |
| `tst_ExportService` | service | 0.01 s | Passed |
| `tst_ImportExport` | service | 0.68 s | Passed |
| `tst_ReminderService` | service | 0.08 s | Passed |
| `tst_SettingsService` | service | 0.02 s | Passed |
| `tst_StatsService` | service | 0.95 s | Passed |
| `ui_smoke` | ui | 6.60 s | Passed |
| `tst_MonthView` | ui | 0.09 s | Passed |
| `tst_Notifiers` | ui | 2.52 s | Passed |
| `tst_StudyRegression` | ui | 6.85 s | Passed |
| `tst_PageJobs` | ui | 12.86 s | Passed |
| `tst_PageCalendar` | ui | 7.26 s | Passed |
| `tst_PageStudy` | ui | 19.88 s | Passed |
| `tst_PageHome` | ui | 17.20 s | Passed |
| `tst_PageSettings` | ui | 4.78 s | Passed |
| `tst_Shortcuts` | ui | 6.29 s | Passed |
| `tst_EmptyStatesAndFocus` | ui | 10.63 s | Passed |
| `tst_HiDpi` | ui | 11.04 s | Passed |
| **`tst_Translations` (New)** | **i18n** | **0.04 s** | **Passed** |

**Total Test Time (real):** 54.56 s  
**New Slots Duration:**
- `tst_Translations`: 0.04 s
- `ui_smoke` pseudo-translation slot addition: ~0.4 s (total test duration 6.60 s, well under 10 s)
- `tst_HiDpi` pseudo-translation slot addition: ~1.5 s (total test duration 11.04 s)

---

## 5. CI Run Predictions per Job

- **Ubuntu 24.04 (Qt 6.10.3 / 6.10.2):** Expected time ~1.5 minutes. Full quality gate passes green. `CI-DIAG` will report `lupdate`, `lrelease`, and `qtbase_{uk,ru}.qm` present in `/usr/share/qt6/translations`.
- **Ubuntu 22.04 (Qt 6.8.2 LTS via aqtinstall):** Expected time ~1.5 minutes. Full quality gate passes green. `CI-DIAG` will report presence of Linguist tools and `qtbase` translations under Qt prefix.
- **Windows (Qt 6.10.3 MSVC 2022 x64):** Expected time ~8.5 minutes (Windows test execution is ~7x slower). New test `tst_Translations` is extremely fast (0.04 s) and will add negligible time (<1 s). `CI-DIAG` will report whether `qtbase_uk.qm` and `qtbase_ru.qm` are bundled in official Windows Qt packages.

---

## 6. Not Verified by Agent (User to Check)

1. **Windows Platform Verification:**
   - Physical presence and loading of `qtbase_uk.qm` and `qtbase_ru.qm` under Windows Qt kit.
   - Language selector and restart flow under Windows.
   - Note: The pseudo-translation test validates widget layout geometry against expansion, but real Cyrillic visual layout on Windows must be checked by the user.
2. **Keyboard Shortcuts under Cyrillic Layouts:**
   - Behavior of `Ctrl+N`, `Ctrl+,`, `Ctrl+1..5` when user's physical input method is set to Ukrainian or Russian layout (differs between Windows, X11, and Wayland).
3. **Translation Glossary Review:**
   - User review of terms in `docs/i18n-glossary.md` prior to beginning Part D.
4. **Cyrillic Font Rendering:**
   - Visual presentation of Cyrillic characters with user's system font stack.

---

## 7. Full Quality Gate Output (`scripts/check.sh`)

```
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.2s)
-- Generating done (0.3s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
[1/9] Linking CXX executable tests/ui/tst_Notifiers
[2/9] Linking CXX executable tests/ui/tst_StudyRegression
[3/9] Linking CXX executable tests/ui/tst_PageJobs
[4/9] Linking CXX executable tests/ui/tst_PageCalendar
[5/9] Linking CXX executable tests/ui/tst_PageStudy
[6/9] Linking CXX executable tests/ui/tst_PageHome
[7/9] Linking CXX executable tests/ui/tst_Shortcuts
[8/9] Linking CXX executable tests/ui/tst_PageSettings
[9/9] Linking CXX executable tests/ui/tst_EmptyStatesAndFocus
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start 34: tst_HiDpi
      Start 29: tst_PageStudy
 1/35 Test #34: tst_HiDpi ........................   Passed   11.04 sec
      Start 30: tst_PageHome
 2/35 Test #29: tst_PageStudy ....................   Passed   19.88 sec
      Start 33: tst_EmptyStatesAndFocus
 3/35 Test #30: tst_PageHome .....................   Passed   17.20 sec
      Start 27: tst_PageJobs
 4/35 Test #33: tst_EmptyStatesAndFocus ..........   Passed   10.63 sec
      Start 32: tst_Shortcuts
 5/35 Test #32: tst_Shortcuts ....................   Passed    6.29 sec
      Start 28: tst_PageCalendar
 6/35 Test #27: tst_PageJobs .....................   Passed   12.86 sec
      Start 26: tst_StudyRegression
 7/35 Test #28: tst_PageCalendar .................   Passed    7.26 sec
      Start 31: tst_PageSettings
 8/35 Test #26: tst_StudyRegression ..............   Passed    6.85 sec
      Start 10: ui_smoke
 9/35 Test #31: tst_PageSettings .................   Passed    4.78 sec
      Start 35: tst_Translations
10/35 Test #35: tst_Translations .................   Passed    0.04 sec
      Start 25: tst_Notifiers
11/35 Test #25: tst_Notifiers ....................   Passed    2.52 sec
      Start 21: tst_ImportExport
12/35 Test #21: tst_ImportExport .................   Passed    0.68 sec
      Start 24: tst_StatsService
13/35 Test #24: tst_StatsService .................   Passed    0.95 sec
      Start  8: tst_SeedService
14/35 Test  #8: tst_SeedService ..................   Passed    0.27 sec
      Start  3: tst_TopicRepository
15/35 Test  #3: tst_TopicRepository ..............   Passed    0.10 sec
      Start  5: tst_ApplicationRepository
16/35 Test  #5: tst_ApplicationRepository ........   Passed    0.09 sec
      Start 11: tst_MonthView
17/35 Test #11: tst_MonthView ....................   Passed    0.09 sec
      Start 22: tst_ReminderService
18/35 Test #22: tst_ReminderService ..............   Passed    0.08 sec
      Start 17: tst_TopicTableProxy
19/35 Test #17: tst_TopicTableProxy ..............   Passed    0.07 sec
      Start 18: tst_AgendaService
20/35 Test #18: tst_AgendaService ................   Passed    0.09 sec
      Start  6: tst_InterviewRepository
21/35 Test  #6: tst_InterviewRepository ..........   Passed    0.03 sec
      Start  9: tst_TransactionBehavior
22/35 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start  7: tst_ReminderLogRepository
23/35 Test  #7: tst_ReminderLogRepository ........   Passed    0.02 sec
      Start  2: tst_TrackRepository
24/35 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start  4: tst_SessionRepository
25/35 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start 23: tst_SettingsService
26/35 Test #23: tst_SettingsService ..............   Passed    0.02 sec
      Start 16: tst_TopicBoardProxy
27/35 Test #16: tst_TopicBoardProxy ..............   Passed    0.05 sec
      Start 13: tst_ApplicationTableModel
28/35 Test #13: tst_ApplicationTableModel ........   Passed    0.06 sec
      Start  1: tst_Database
29/35 Test  #1: tst_Database .....................   Passed    0.03 sec
      Start 15: tst_TopicListModel
30/35 Test #15: tst_TopicListModel ...............   Passed    0.04 sec
      Start 12: tst_AgendaListModel
31/35 Test #12: tst_AgendaListModel ..............   Passed    0.03 sec
      Start 19: tst_DayWatcher
32/35 Test #19: tst_DayWatcher ...................   Passed    0.01 sec
      Start 14: tst_ApplicationFilterProxy
33/35 Test #14: tst_ApplicationFilterProxy .......   Passed    0.02 sec
      Start 20: tst_ExportService
34/35 Test #20: tst_ExportService ................   Passed    0.01 sec
35/35 Test #10: ui_smoke .........................   Passed    6.60 sec

100% tests passed, 0 tests failed out of 35

Label Time Summary:
data       =   0.63 sec*proc (9 tests)
i18n       =   0.04 sec*proc (1 test)
model      =   0.27 sec*proc (6 tests)
service    =   1.85 sec*proc (7 tests)
ui         = 105.99 sec*proc (12 tests)

Total Test time (real) =  54.56 sec
==> Checking translation sync with /usr/lib/qt6/bin/lupdate...
==> Translation files are in sync.
==> Quality check PASSED with zero warnings and all tests green!
```

---

## 8. Git Commit Log

```
65451bf docs(i18n): add draft translation glossary and update specifications
0d87eba ci: add qtbase translation file diagnostic check
1d4d1f7 test(i18n): pseudo-translation layout verification and translation completeness tests
809c9ed feat(i18n): locale-aware month and day formatting in calendar and agenda
c5cd2d3 fix(i18n): translation contexts, translator comments, and pluralization
2990934 feat(ui): language selector and restart notice in SettingsPage
d845c50 feat(services): language setting, resolution, and startup installation infrastructure
5146be4 build: configure Qt LinguistTools, .ts skeletons, and check-translations script
```
