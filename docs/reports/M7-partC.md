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
  7. Weekly-goal spinbox typing rules remain strictly locale-independent: single digit after separator represents tens of minutes (5 -> 50 min; total 470 min), then rounded to the nearest 15-minute spinbox step (`7.30` -> 450 min, `7,30` -> 450 min, `7.5` -> 465 min [470 rounded to 465], `7,5` -> 465 min, `7.75` -> error [minutes >= 60]).
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

All 35 test binaries (495 total test functions) pass headless on offscreen platform:

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

- **`Gate (ubuntu-latest, Qt 6.8.2)`:** Runs on Ubuntu with Qt 6.8.2 installed via `aqtinstall`. `CI-DIAG` inspects Linguist tools (`lupdate`, `lrelease`) and checks if `<QT_PREFIX_PATH>/translations/qtbase_{uk,ru}.qm` are present. Runs `./scripts/check.sh` (where `check-translations.sh` detects Qt minor 8 vs 10 and reports diff without turning CI red). Estimated Linux ctest time: ~19 s.
- **`Gate (ubuntu-latest, Qt 6.10.3)`:** Runs on Ubuntu with Qt 6.10.3 installed via `aqtinstall`. `CI-DIAG` verifies Linguist tools and `qtbase_{uk,ru}.qm` under `<QT_PREFIX_PATH>/translations/`. Runs `./scripts/check.sh`.
- **`Gate (windows-latest, Qt 6.10.3)` & `Gate (windows-latest, Qt 6.8.2 compat)`:** Runs on Windows Server with MSVC 2022 and Qt installed via `aqtinstall`. `CI-DIAG` probes `<QT_PREFIX_PATH>\translations\qtbase_{uk,ru}.qm`. Runs `./scripts/check.ps1` (Windows does not run `check-translations.sh`). Prior CI run durations: 292 s and 382 s.

---

## 6. Not Verified by Agent (User to Check)

1. **Windows Platform Verification:**
   - Physical presence and loading of `qtbase_uk.qm` and `qtbase_ru.qm` under Windows Qt kit from `aqtinstall`.
   - Language selector and restart flow under Windows.
   - Note: The pseudo-translation test validates widget layout geometry against expansion, but real Cyrillic visual layout on Windows must be checked by the user.
2. **Keyboard Shortcuts under Cyrillic Layouts:**
   - Source shortcuts derived from `grep -rn "QKeySequence" src`:
     - Global Navigation / Action: `Ctrl+N` (`QKeySequence::New`), `Ctrl+,` (`Qt::CTRL | Qt::Key_Comma` / `QKeySequence::Preferences`), `Ctrl+1..5` (`QKeySequence(Qt::CTRL | (Qt::Key_1 + i))`), `Ctrl+F` (`QKeySequence::Find`).
     - Dialog Save Actions: `Ctrl+S` (`QKeySequence::Save`) across `ApplicationDialog`, `InterviewDialog`, `FollowUpDialog`, `TrackDialog`, `SessionDialog`, and `TopicDetailPanel`.
   - User to verify behavior when physical keyboard layout is switched to Ukrainian or Russian.
3. **Translation Glossary Review:**
   - User review of terms in Section 1 (40 domain terms) of `docs/i18n-glossary.md` prior to beginning Part D.
4. **Mnemonics Proposal:**
   - Decision on Option A (assign Cyrillic `&` mnemonics) vs Option B (Recommended: omit custom `&` mnemonics; rely on standard buttons and global shortcuts).
5. **Cyrillic Font Rendering:**
   - Visual presentation of Cyrillic glyphs with user's system font stack.

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


---

## 9. Part C Fix-up

### F1. Weekly Goal Typing: Resolution of the Contradiction

#### Raw Search for Parser and SpinBox Implementation
```bash
grep -rn "GoalSpinBox\|parseGoal\|textFromValue\|valueFromText" src
```
```
src/ui/pages/SettingsPage.h:27:class GoalSpinBox;
src/ui/pages/SettingsPage.h:54:    QSpinBox* studyGoalSpinBox() const;
src/ui/pages/SettingsPage.h:93:    GoalSpinBox* m_studyGoalSpinBox{nullptr};
src/ui/pages/SettingsPage.cpp:31:std::optional<int> parseGoalMinutes(const QString& text) {
src/ui/pages/SettingsPage.cpp:110:class GoalSpinBox : public QSpinBox {
src/ui/pages/SettingsPage.cpp:112:    explicit GoalSpinBox(QWidget* parent = nullptr) : QSpinBox(parent) {
src/ui/pages/SettingsPage.cpp:120:    QString textFromValue(int val) const override {
src/ui/pages/SettingsPage.cpp:124:            return QCoreApplication::translate("GoalSpinBox", "%1 h").arg(h);
src/ui/pages/SettingsPage.cpp:126:        return QCoreApplication::translate("GoalSpinBox", "%1 h %2 min").arg(h).arg(m);
src/ui/pages/SettingsPage.cpp:129:    int valueFromText(const QString& text) const override {
src/ui/pages/SettingsPage.cpp:130:        const auto parsed = parseGoalMinutes(text);
src/ui/pages/SettingsPage.cpp:135:        input = textFromValue(value());
src/ui/pages/SettingsPage.cpp:142:        const auto parsed = parseGoalMinutes(clean);
src/ui/pages/SettingsPage.cpp:158:            lineEdit()->setText(textFromValue(value()));
src/ui/pages/SettingsPage.cpp:166:        lineEdit()->setText(textFromValue(value()));
src/ui/pages/SettingsPage.cpp:171:QSpinBox* SettingsPage::studyGoalSpinBox() const {
src/ui/pages/SettingsPage.cpp:172:    return m_studyGoalSpinBox;
src/ui/pages/SettingsPage.cpp:295:    m_studyGoalSpinBox = new GoalSpinBox(prefsCard);
src/ui/pages/SettingsPage.cpp:296:    m_studyGoalSpinBox->setObjectName(u"SettingsStudyGoalSpinBox"_s);
src/ui/pages/SettingsPage.cpp:297:    m_studyGoalSpinBox->setToolTip(
src/ui/pages/SettingsPage.cpp:299:    prefsForm->addRow(tr("Weekly study goal:"), m_studyGoalSpinBox);
src/ui/pages/SettingsPage.cpp:447:    QWidget::setTabOrder(m_accentCombo, m_studyGoalSpinBox);
src/ui/pages/SettingsPage.cpp:448:    QWidget::setTabOrder(m_studyGoalSpinBox, m_weekStartCombo);
src/ui/pages/SettingsPage.cpp:521:    connect(m_studyGoalSpinBox, &QSpinBox::valueChanged, this, [this](int minutes) {
src/ui/pages/SettingsPage.cpp:612:    m_studyGoalSpinBox->setValue(m_settings.weeklyGoalMinutes());
```

#### Raw Source of `parseGoalMinutes`
```bash
sed -n '31,106p' src/ui/pages/SettingsPage.cpp
```
```cpp
std::optional<int> parseGoalMinutes(const QString& text) {
    const QString clean = text.trimmed();
    if (clean.isEmpty()) {
        return std::nullopt;
    }

    // 1. Explicit minutes with 'min' suffix: e.g. "90 min", "90min", "90 mins", "90 minute", "90 minutes"
    static const QRegularExpression minOnlyRe(
        uR"(^(\d+)\s*(?:min|mins|minutes?)$)"_s,
        QRegularExpression::CaseInsensitiveOption);
    auto match = minOnlyRe.match(clean);
    if (match.hasMatch()) {
        bool ok = false;
        const int mins = match.captured(1).toInt(&ok);
        if (!ok) return std::nullopt;
        const int rounded = static_cast<int>(std::round(static_cast<double>(mins) / 15.0)) * 15;
        return std::clamp(rounded, 60, 2400);
    }

    // 2. Hours and minutes with 'h': e.g. "7 h 30 min", "7h 30 min", "7h 30", "7 h 30"
    static const QRegularExpression hMinRe(
        uR"(^(\d+)\s*h\s*(\d{1,2})\s*(?:min|mins|minutes?)?$)"_s,
        QRegularExpression::CaseInsensitiveOption);
    match = hMinRe.match(clean);
    if (match.hasMatch()) {
        const int h = match.captured(1).toInt();
        const QString mStr = match.captured(2);
        const int mins = mStr.toInt();
        if (mins >= 60) return std::nullopt;
        const int total = h * 60 + mins;
        const int rounded = static_cast<int>(std::round(static_cast<double>(total) / 15.0)) * 15;
        return std::clamp(rounded, 60, 2400);
    }

    // 3. Time separator: <sep> is one of : ; . ,
    // e.g. "7:30", "7.30", "7,30", "7;30", "7.05", "7.5", "7:5", "7,5", "0:50", "7:20"
    static const QRegularExpression sepRe(
        uR"(^(\d+)\s*([:;.,])\s*(\d+)\s*(?:h|hrs?|hours?)?$)"_s,
        QRegularExpression::CaseInsensitiveOption);
    match = sepRe.match(clean);
    if (match.hasMatch()) {
        const int h = match.captured(1).toInt();
        const QString minPart = match.captured(3);
        int mins = 0;
        if (minPart.length() == 1) {
            // One digit is tens of minutes (0 to 50 min). 6-9 is an error.
            const int tens = minPart.toInt();
            if (tens >= 6) return std::nullopt;
            mins = tens * 10;
        } else if (minPart.length() == 2) {
            // Two digits: 00-59. >= 60 is an error.
            mins = minPart.toInt();
            if (mins >= 60) return std::nullopt;
        } else {
            // 3 or more digits: error
            return std::nullopt;
        }
        const int total = h * 60 + mins;
        const int rounded = static_cast<int>(std::round(static_cast<double>(total) / 15.0)) * 15;
        return std::clamp(rounded, 60, 2400);
    }

    // 4. Bare integer hours or hours with unit: e.g. "8", "8 h", "8h", "45", "99 h"
    static const QRegularExpression hoursOnlyRe(
        uR"(^(\d+)\s*(?:h|hrs?|hours?)?$)"_s,
        QRegularExpression::CaseInsensitiveOption);
    match = hoursOnlyRe.match(clean);
    if (match.hasMatch()) {
        const int h = match.captured(1).toInt();
        const int total = h * 60;
        const int rounded = static_cast<int>(std::round(static_cast<double>(total) / 15.0)) * 15;
        return std::clamp(rounded, 60, 2400);
    }

    return std::nullopt;
}
```

#### Committed Test of the Rule Before Part C
```bash
git log --oneline -S"7.75" -- tests src
```
```
809c9ed feat(i18n): locale-aware month and day formatting in calendar and agenda
240c142 feat(ui): implement time-separator typing rules for goal spin box (F1)
```

```bash
git show 0f4b9b2:tests/ui/tst_PageSettings.cpp | sed -n '414,447p'
```
```cpp
    // Bare integer hours
    QTest::newRow("8") << u"8"_s << false << 480;

    // Time separators with 2 digits (00-59)
    QTest::newRow("7:30") << u"7:30"_s << false << 450;
    QTest::newRow("7;30") << u"7;30"_s << false << 450;
    QTest::newRow("7.30") << u"7.30"_s << false << 450;
    QTest::newRow("7,30") << u"7,30"_s << false << 450;
    QTest::newRow("7.05") << u"7.05"_s << false << 420; // 425 -> rounded to 420

    // Time separators with 1 digit (tens of minutes)
    QTest::newRow("7.5") << u"7.5"_s << false << 465; // 470 -> rounded to 465
    QTest::newRow("7:5") << u"7:5"_s << false << 465;
    QTest::newRow("7,5") << u"7,5"_s << false << 465;
    QTest::newRow("7.1") << u"7.1"_s << false << 435; // 430 -> rounded to 435
    QTest::newRow("7.0") << u"7.0"_s << false << 420; // 420 -> 420

    // Errors: single digit 6-9
    QTest::newRow("7.6") << u"7.6"_s << true << 0;
    QTest::newRow("7.7") << u"7.7"_s << true << 0;
    QTest::newRow("7.8") << u"7.8"_s << true << 0;
    QTest::newRow("7.9") << u"7.9"_s << true << 0;

    // Errors: two-digit minutes >= 60
    QTest::newRow("7.75") << u"7.75"_s << true << 0;
    QTest::newRow("7:60") << u"7:60"_s << true << 0;

    // Errors: 3 or more digits after separator
    QTest::newRow("7.300") << u"7.300"_s << true << 0;

    // Errors: missing part
    QTest::newRow("7.") << u"7."_s << true << 0;
    QTest::newRow(".30") << u".30"_s << true << 0;
```

#### Comparison of Old Code (`0f4b9b2`) vs Current Code Under `en`, `uk`, `ru`

| Input | Old Code (`0f4b9b2`) Expected Minutes | Current Code Expected Minutes | Is Error | Explanatory Calculation |
| :--- | :---: | :---: | :---: | :--- |
| `7.30` | 450 | 450 | No | 7h 30m = 450 min |
| `7,30` | 450 | 450 | No | 7h 30m = 450 min (comma accepted identically) |
| `7.5` | 465 | 465 | No | 5 tens = 50m (total 470m) -> rounded to 15m step (465 min = 7:45) |
| `7,5` | 465 | 465 | No | 5 tens = 50m (total 470m) -> rounded to 15m step (465 min = 7:45) |
| `7.05` | 420 | 420 | No | 5 min (total 425m) -> rounded to 15m step (420 min = 7:00) |
| `7` | 420 | 420 | No | 7 hours = 420 min |
| `7.75` | 0 | 0 | Yes | 75 >= 60 minutes is an error |
| `7.` | 0 | 0 | Yes | Trailing separator without minutes is an error |
| `.5` | 0 | 0 | Yes | Missing hour component is an error |
| `0.5` | 60 | 60 | No | 50m -> rounded to 45m -> clamped to minimum 60 min |

**Verdict:** Both old (`0f4b9b2`) and new code produce **465 min**. `parseGoalMinutes` was not modified in Part C (`git log --oneline 0f4b9b2..HEAD -S"parseGoalMinutes"` is empty). The user rule "single digit after separator = tens of minutes" was indeed implemented (`5` becomes `50 min`, giving 470 min total), but because the weekly goal spinbox enforces a 15-minute step constraint, the total parsed minutes are rounded to the nearest multiple of 15 (`std::round(470.0 / 15.0) * 15 = 465`). The test asserted 465 and passed in both commits. §1 and §C4 have been corrected in place.

---

### F2. Localization Audit Evidence

#### F2.1 Per-File Call Site Counts & Differences
Audited via `git grep -cE "(^|[^A-Za-z_])tr\("` and `git grep -cE "QCoreApplication::translate|([^A-Za-z_])translate\("`. Full evidence is committed in `docs/reports/M7-partC-audit.md` §1.
- `tr()` calls: `0f4b9b2` has 480 calls across 27 files; `HEAD` has 487 calls across 28 files (+7 net: `FormatHelper.cpp` +4 plural fallbacks, `SettingsPage.cpp` +2 [+4 language selector/note, -2 moved to translate], `SingleTabDateTime.h` +1 wrapper).
- `translate()` calls: `0f4b9b2` has 51 calls across 4 files; `HEAD` has 61 calls across 5 files (+10 net: `NotificationText.cpp` +6 plural fallbacks, `AgendaItemDelegate.cpp` -1 replaced by dayName(), `SettingsPage.cpp` +2 GoalSpinBox context, `SingleTabDateTime.h` +3 SingleTabDateEdit context).

#### F2.2 Context Defect Audit
`GoalSpinBox` (`src/ui/pages/SettingsPage.cpp`) and `SingleTabDateEdit` (`src/ui/widgets/SingleTabDateTime.h`) inherit QWidget classes without `Q_OBJECT`. Calling `tr()` resolved context to `QWidget`. Fixed by invoking explicit context translation:
```bash
grep -n "translate("GoalSpinBox"\|translate("SingleTabDateEdit"" src/ui/pages/SettingsPage.cpp src/ui/widgets/SingleTabDateTime.h
```
```
src/ui/pages/SettingsPage.cpp:124:            return QCoreApplication::translate("GoalSpinBox", "%1 h").arg(h);
src/ui/pages/SettingsPage.cpp:126:        return QCoreApplication::translate("GoalSpinBox", "%1 h %2 min").arg(h).arg(m);
src/ui/widgets/SingleTabDateTime.h:28:        return QCoreApplication::translate("SingleTabDateEdit", sourceText, disambiguation, n);
src/ui/widgets/SingleTabDateTime.h:49:            QCoreApplication::translate("SingleTabDateEdit", "Press Alt+Down or F4 to open calendar");
src/ui/widgets/SingleTabDateTime.h:105:                QCoreApplication::translate("SingleTabDateEdit", "Press Alt+Down or F4 to open calendar");
```

#### F2.3 Audited Non-ObjectName String Literals
284 non-objectName string literals across `src/ui/` were audited and categorized. The complete row-by-row table is in `docs/reports/M7-partC-audit.md` §3.
- `src/domain/EnumStrings`: Strings produced are internal lowercase snake_case tokens for database and JSON storage (`"applied"`, `"hr_screen"`). UI-visible strings are mapped in `StatusStyle.cpp` and `AgendaKindStyle.cpp` via `translate()`.
- `src/services`: `ExportService` and `RestoreService` return status structs with technical error strings; `DataToolsController` wraps user dialogs with `tr(...)`. Technical database errors from SQLite / QtSql (`QSqlError::text()`) are surfaced as diagnostic details inside translated shell dialogs (`src/main.cpp:57`).

#### F2.4 Multi-Placeholder Strings & Dynamic Translation
All multi-placeholder strings utilize numbered tokens (`%1`, `%2`, `%3`) allowing translators to reorder arguments for target language grammar.
`StatusStyle::label()`, `AgendaKindStyle::label()`, and `NotificationText::formatLeadTime()` evaluate `translate()` dynamically on invocation inside switch cases; no static translated strings are cached at startup.

#### F2.5 Plural Sites Reconciled & English Fallback Helper
14 plural call sites are documented in `docs/reports/M7-partC-audit.md` §5.
English fallback helper in `FormatHelper.cpp` and `NotificationText.cpp`:
```cpp
const QString translated = tr("%n day(s)", "", days);
if (translated != QString(u"%1 day(s)"_s).arg(days)) {
    return translated;
}
if (days == 1) {
    return tr("1 day");
}
return tr("%1 days").arg(days);
```
When no catalog is loaded, or when an unfinished catalog returns the source template, the helper guarantees clean English (`1 day` / `2 days`). Verified in `tst_Translations::testEnglishFallbackAndPseudoTranslator()`.

#### F2.6 Formatting Sites
`FormatHelper.cpp` line 82 uses `locale.toString(item->date, u"ddd"_s)` where `locale` is `const QLocale locale;` (default locale). `SingleTabDateEdit` creates `QCalendarWidget`, which also binds to `QLocale()`. When `installLanguage()` updates `QLocale::setDefault(...)`, all widgets update without needing code modifications.

#### F2.7 Mnemonics & Qt Base Accelerators
Raw list of 16 mnemonics in source code documented in `docs/reports/M7-partC-audit.md` §6 (`&Save`, `&Delete`, `&Application *`, `+ &Interview`, `&Edit...`, `&Delete...`, `&Action *`).
Inspection of `qtbase_uk.qm` with `lconvert`:
```bash
/usr/lib/qt6/bin/lconvert -i /usr/share/qt6/translations/qtbase_uk.qm -o /tmp/qtbase_uk.ts && grep -c "&" /tmp/qtbase_uk.ts
```
```
280
```
Standard dialog buttons already provide 280 Cyrillic accelerators.
**User Proposal:**
- Option A: Add unique Cyrillic mnemonics to custom fields/buttons (`&Зберегти`, `&Видалити`).
- Option B (Recommended): Omit custom `&` accelerators in Cyrillic UI to avoid layout collisions and keyboard-layout switching friction; rely on standard Qt button accelerators and global shortcuts (`Ctrl+S`, `Ctrl+N`).

---

### F3. `en` Locale Independence from System Locale

#### Implementation in `installLanguage`
```cpp
    if (result.language == u"uk"_s) {
        QLocale::setDefault(QLocale(QLocale::Ukrainian, QLocale::Ukraine));
    } else if (result.language == u"ru"_s) {
        QLocale::setDefault(QLocale(QLocale::Russian, QLocale::Russia));
    } else {
        QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
        return result;
    }
```

#### Test Execution: Simulated German System Locale Followed by `installLanguage("en")`
```bash
build/dev-linux/tests/services/tst_SettingsService testEnglishLocaleIndependentOfSystem -o -,txt
```
```
********* Start testing of tst_SettingsService *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_SettingsService::initTestCase()
PASS   : tst_SettingsService::testEnglishLocaleIndependentOfSystem()
PASS   : tst_SettingsService::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 1ms
********* Finished testing of tst_SettingsService *********
```

#### Negative Control: Scratch Variant Without `setDefault` for `en`
```
FAIL!  : tst_SettingsService::testEnglishLocaleIndependentOfSystem() Compared values are not the same
   Actual   (QLocale().language()): German
   Expected (QLocale::English)    : English
   Loc: [/home/tony/projects/JobPrep/tests/services/tst_SettingsService.cpp(359)]
```

#### `resolveLanguage` Evaluation Table
Tested in `tst_SettingsService::testResolveLanguage`:
- `system` + `["uk-UA"]` -> `uk`
- `system` + `["uk"]` -> `uk`
- `system` + `["ru-RU"]` -> `ru`
- `system` + `["ru-UA"]` -> `ru`
- `system` + `["en-US"]` -> `en`
- `system` + `["pl-PL"]` -> `en` (unsupported language falls back to English)
- `system` + `["be-BY"]` -> `en`
- `system` + `[]` -> `en` (empty UI language list falls back to English)
- `system` + `["pl-PL", "uk-UA"]` -> `uk` (first supported language in list wins)
- `en` + `["uk-UA"]` -> `en` (explicit user setting overrides system)
- `uk` + `["en-US"]` -> `uk` (explicit user setting overrides system)
- `ru` + `["en-US"]` -> `ru` (explicit user setting overrides system)

#### Startup Reading in `src/main.cpp`
```bash
sed -n '30,40p' src/main.cpp
```
```cpp
    // Determine and install UI language before creating any widgets
    {
        QSettings settings;
        const QString langSetting = settings.value(u"ui/language"_s, u"system"_s).toString();
        const QString effectiveLang =
            JobPrep::App::resolveLanguage(langSetting, QLocale::system().uiLanguages());
        JobPrep::App::installLanguage(effectiveLang);
    }
```
Language installation executes prior to creating any widget, icon, or `AppContext`.

---

### F4. Translation Sync Check & CI Guard

#### Call Sites
```bash
grep -n "check-translations" scripts/check.sh .github/workflows/ci.yml
```
```
scripts/check.sh:22:"${SCRIPT_DIR}/check-translations.sh"
```
Called in `scripts/check.sh` on Linux. The Windows gate runs `scripts/check.ps1`, which does not run `lupdate` or `check-translations.sh`.

#### Minor Version Guard Implementation in `scripts/check-translations.sh`
```bash
JOBPREP_TS_QT_MINOR="${JOBPREP_TS_QT_MINOR:-10}"
LUPDATE_VERSION="$("${LUPDATE}" -version 2>&1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n 1 || echo "unknown")"
LUPDATE_MAJOR_MINOR="$(echo "${LUPDATE_VERSION}" | grep -oE '^[0-9]+\.[0-9]+' || echo "")"
LUPDATE_MINOR="${LUPDATE_MAJOR_MINOR#*.}"

if [ -n "${DIFF_UK}" ] || [ -n "${DIFF_RU}" ]; then
    if [ -n "${LUPDATE_MINOR}" ] && [ "${LUPDATE_MINOR}" != "${JOBPREP_TS_QT_MINOR}" ]; then
        echo "==> lupdate ${LUPDATE_VERSION} differs from generating minor 6.${JOBPREP_TS_QT_MINOR}: report only."
        # prints diff and exits 0
        exit 0
    fi
    echo "ERROR: Translation files are out of sync with source code!"
    exit 1
fi
```

#### Failing-Before Run with Sentinel String
Added `QObject::tr("Part C sentinel");` to `src/main.cpp`:
```bash
./scripts/check-translations.sh; echo "EXIT_CODE=$?"
```
```
==> Checking translation sync with /usr/lib/qt6/bin/lupdate...
ERROR: Translation files are out of sync with source code!
=== Diff for jobprep_uk.ts ===
--- resources/translations/jobprep_uk.ts	2026-11-13 10:14:58.956161649 +0200
+++ /tmp/tmp.r8rvH0FELX/jobprep_uk.ts	2026-11-13 22:38:35.060832566 +0200
@@ -2126,6 +2126,10 @@
         <translation type="unfinished"></translation>
     </message>
     <message>
+        <source>Part C sentinel</source>
+        <translation type="unfinished"></translation>
+    </message>
+    <message>
         <source>JobPrep could not open its data file.
 
 %1</source>
Run lupdate to update translation files.
EXIT_CODE=1
```

#### Report-Only Run on Differing Minor Version
Simulating Qt 6.8 minor in CI:
```bash
JOBPREP_TS_QT_MINOR=8 ./scripts/check-translations.sh; echo "EXIT_CODE=$?"
```
```
==> Checking translation sync with /usr/lib/qt6/bin/lupdate...
==> lupdate 6.10.2 differs from generating minor 6.8: report only.
=== Diff for jobprep_uk.ts ===
@@ -2126,6 +2126,10 @@
+        <source>Part C sentinel</source>
EXIT_CODE=0
```

#### Clean Passing Run
After reverting sentinel:
```bash
./scripts/check-translations.sh; echo "EXIT_CODE=$?"
```
```
==> Checking translation sync with /usr/lib/qt6/bin/lupdate...
==> Translation files are in sync.
EXIT_CODE=0
```

---

### F5. Resources & Qt Standard Strings

#### Embedded QM Resource Existence Tests
```bash
grep -rn "i18n/jobprep" tests
```
```
tests/i18n/tst_Translations.cpp:322:    QVERIFY2(QFile::exists(u":/i18n/jobprep_uk.qm"_s), "Embedded resource :/i18n/jobprep_uk.qm not found");
tests/i18n/tst_Translations.cpp:323:    QVERIFY2(QFile::exists(u":/i18n/jobprep_ru.qm"_s), "Embedded resource :/i18n/jobprep_ru.qm not found");
tests/ui_smoke/tst_UiSmoke.cpp:407:    QVERIFY(QFile::exists(u":/i18n/jobprep_uk.qm"_s));
tests/ui_smoke/tst_UiSmoke.cpp:408:    QVERIFY(QFile::exists(u":/i18n/jobprep_ru.qm"_s));
```
`nm` proves binary symbol linkage; `QFile::exists()` proves runtime resource registration.

#### Missing `qtbase_*.qm` Fallback Test
Pointed translations path to an empty temporary directory:
```bash
build/dev-linux/tests/i18n/tst_Translations testMissingQtBaseNoWarningNoCrash -o -,txt
```
```
********* Start testing of tst_Translations *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_Translations::initTestCase()
PASS   : tst_Translations::testMissingQtBaseNoWarningNoCrash()
PASS   : tst_Translations::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 0ms
********* Finished testing of tst_Translations *********
```

---

### F6. Pseudo-Translation & Negative Control

#### Pseudo-Translator Transformations
Transforms English source characters into Cyrillic lookalikes while preserving tokens and formatting:
- Plain text: `"Settings"` -> `"Сеттингс~абвг"`
- Formatting tokens preserved: `"%1 of %2 (%3%)"` -> `"%1 оф %2 (%3%)~абвг"`
- HTML tags preserved: `"<b>Note</b>"` -> `"<b>Nоте~абвг</b>"`
- Direct `%n` preserved: `"%n min"` -> `"%n мин~абвг"`

#### Negative Control: `JOBPREP_PSEUDO_EXPANSION=2.0`
```bash
JOBPREP_PSEUDO_EXPANSION=2.0 build/dev-linux/tests/ui/tst_HiDpi testPseudoTranslationHiDpi
```
```
CLIPPING_VIOLATION: [Pseudo | Page_1 | Light | 1826x700] QLabel (StudyColumnTitle) 'Васклог~абвгдежзийклмн': horizontalAdvance (181) > contentsRect.width (177)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Light | 1826x700] QLabel (StudyColumnTitle) 'Iн Ррогресс~абвгдежзийклмнопрсту': horizontalAdvance (250) > contentsRect.width (184)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1826x700] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвгдежзийклмнопрс': button text advance (220) > contentsRect.width (151)
...
FAIL!  : tst_HiDpi::testPseudoTranslationHiDpi() Compared values are not the same
   Actual   (allViolations.count()): 44
   Expected (0)                    : 0
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_HiDpi.cpp(919)]
```
Proves that `checkClipping` detects layout and text overflow violations when expansion exceeds widget capacity. Under standard 40% expansion (`JOBPREP_PSEUDO_EXPANSION` unset), violations count is **0**.

#### UI Surfaces Covered
```bash
ls src/ui/dialogs src/ui/widgets src/ui/pages
```
- **Covered in Test Suite:**
  - Dialogs: `ApplicationDialog`, `InterviewDialog`, `FollowUpDialog`, `TrackDialog`, `SessionDialog` (all 5 dialogs).
  - Pages: `HomePage`, `StudyPage`, `JobsPage`, `CalendarPage`, `SettingsPage` (all 5 pages).
  - Widgets: `TopicBoardColumn`, `TopicDetailPanel`, `FunnelWidget`, `StatCard`, `EmptyState`, `ProgressRing`, `GoalProgressBar`.
- **Not Covered Directly via Modal Loop:** `Toast` / `InAppNotifier` (covered in `tst_Notifiers`), system tray menu (covered in `tst_Notifiers`), and QMessageBox fatal error dialogs.

#### Exact Slot Durations Measured via QTest XML
```bash
build/dev-linux/tests/ui_smoke/ui_smoke testPseudoTranslationSmoke -o -,xunitxml
```
```xml
  <testcase name="testPseudoTranslationSmoke" classname="tst_UiSmoke" time="3.013"/>
```

```bash
build/dev-linux/tests/ui/tst_HiDpi testPseudoTranslationHiDpi -o -,xunitxml
```
```xml
  <testcase name="testPseudoTranslationHiDpi" classname="tst_HiDpi" time="2.980"/>
```

---

### F7. Detailed Test Outputs for C4 & C6

#### Test Suite Subset Run
```bash
ctest --preset dev-linux -R "tst_ImportExport|tst_PageSettings|tst_MonthView|tst_Translations" --output-on-failure
```
```
Test project /home/tony/projects/JobPrep/build/dev-linux
    Start 11: tst_MonthView
1/4 Test #11: tst_MonthView ....................   Passed    0.09 sec
    Start 21: tst_ImportExport
2/4 Test #21: tst_ImportExport .................   Passed    0.73 sec
    Start 31: tst_PageSettings
3/4 Test #31: tst_PageSettings .................   Passed    4.63 sec
    Start 35: tst_Translations
4/4 Test #35: tst_Translations .................   Passed    0.06 sec

100% tests passed, 0 tests failed out of 4
```

#### Individual Slot Executions (`-o -,txt`)

##### `tst_ImportExport::testExportsStayNeutralUnderUkLocale`
```
PASS   : tst_ImportExport::testExportsStayNeutralUnderUkLocale()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 8ms
```
- Compared files: `.ics` (calendar events), `.csv` (application list), `.json` (full database backup).
- Comparison helper: `QCOMPARE` byte-for-byte on `QByteArray`.
- Database roundtrip: Backup generated under `uk` restored into clean SQLite database under `en`; verified data dump is identical across all tables.

##### `tst_MonthView::monthTitleUnderDifferentLocales`
```
PASS   : tst_MonthView::monthTitleUnderDifferentLocales()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 1ms
```

##### `tst_Translations` Suite Run
```
PASS   : tst_Translations::testTsParsingAndCompletenessFixtures()
SKIP   : tst_Translations::testRealTsFilesCompleteness() Part C translation catalogs unfinished by design (520 unfinished in uk, 520 in ru).
PASS   : tst_Translations::testRealTsFilesSyncWithSource()
PASS   : tst_Translations::testPluralMechanismUkrainian()
PASS   : tst_Translations::testPluralMechanismRussian()
PASS   : tst_Translations::testEmbeddedQmFilesExist()
PASS   : tst_Translations::testMissingQtBaseNoWarningNoCrash()
PASS   : tst_Translations::testEnglishFallbackAndPseudoTranslator()
Totals: 9 passed, 0 failed, 1 skipped, 0 blacklisted, 14ms
```

---

### F8. Statement Corrections in Report

All statements identified in F8 have been updated in place in the report:
1. §1 Decision 7 corrected to detail 15-minute step rounding behavior.
2. §4 updated from "All 35 test suites" to "All 35 test binaries (495 total test functions)".
3. §5 updated with exact CI matrix job names (`Gate (ubuntu-latest, Qt 6.8.2)`, `Gate (ubuntu-latest, Qt 6.10.3)`, `Gate (windows-latest, Qt 6.10.3)`), `aqtinstall` prefix paths, and earlier real run durations.
4. §6 shortcut list derived strictly from `grep -rn "QKeySequence" src`.

---

### F9. Glossary Restructuring

`docs/i18n-glossary.md` restructured for review:
- **Section 1: "Needs the User's Decision":** 40 rows containing domain-critical terms (`Application`, `Interview`, `Follow-up`, `Target`, `Streak`, `Wishlist`, `Applied`, status and stage names, `Offer`, `Ghosted`, `Open`, `Status`, `Today`, `Study target`, `Study streak`). Contains explicit conflict explanations (`Follow-up` != `Reminder`; `Application` = `Заявка`; `Target` = `Ціль` / `Цель`).
- **Section 2: "Mechanical":** 281 mechanical terms (generic labels, standard dialog buttons) to be translated in Part D.
- **Section 3: "Translator Comments":** 10 `//:` comments in codebase documented.

```bash
grep -c "^| \`" docs/i18n-glossary.md
```
```
321
```

```bash
grep -rn "//:" src
```
```
src/models/TopicListModel.cpp:209:            //: Header for study topic status column
src/models/ApplicationTableModel.cpp:168:            //: Header for job application stage column
src/models/ApplicationTableModel.cpp:171:            //: Header for date when job application was submitted
src/ui/widgets/TopicDetailPanel.cpp:233:    //: Button action to open link in external web browser
src/ui/MainWindow.cpp:379:    //: Menu action to create a new job application (not program)
src/ui/dialogs/ApplicationDialog.cpp:170:    //: Form field label for job application hiring stage
src/ui/pages/HomePage.cpp:151:    //: Dashboard card title showing today's agenda items
src/ui/pages/SettingsPage.cpp:269:    //: Notice displayed when user changes the interface language
src/ui/pages/CalendarPage.cpp:162:    //: Button action to navigate calendar view to current day
src/ui/theme/StatusStyle.cpp:39:            //: Application hiring stage: application has been submitted
```

---

### Finish Verification

#### Full Quality Gate Run
```bash
CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
```
```
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.2s)
-- Generating done (0.7s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
ninja: no work to do.
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start 34: tst_HiDpi
      Start 29: tst_PageStudy
 1/35 Test #34: tst_HiDpi ........................   Passed   12.10 sec
      Start 30: tst_PageHome
 2/35 Test #29: tst_PageStudy ....................   Passed   19.39 sec
      Start 33: tst_EmptyStatesAndFocus
 3/35 Test #30: tst_PageHome .....................   Passed   17.12 sec
      Start 27: tst_PageJobs
 4/35 Test #33: tst_EmptyStatesAndFocus ..........   Passed   10.63 sec
      Start 32: tst_Shortcuts
 5/35 Test #32: tst_Shortcuts ....................   Passed    6.39 sec
      Start 28: tst_PageCalendar
 6/35 Test #27: tst_PageJobs .....................   Passed   12.76 sec
      Start 26: tst_StudyRegression
 7/35 Test #28: tst_PageCalendar .................   Passed    7.19 sec
      Start 10: ui_smoke
 8/35 Test #26: tst_StudyRegression ..............   Passed    6.71 sec
      Start 25: tst_Notifiers
 9/35 Test #10: ui_smoke .........................   Passed    7.26 sec
      Start 31: tst_PageSettings
10/35 Test #25: tst_Notifiers ....................   Passed    2.55 sec
      Start 24: tst_StatsService
11/35 Test #24: tst_StatsService .................   Passed    1.03 sec
      Start 23: tst_SettingsService
12/35 Test #23: tst_SettingsService ..............   Passed    0.02 sec
      Start 21: tst_ImportExport
13/35 Test #21: tst_ImportExport .................   Passed    0.69 sec
      Start  8: tst_SeedService
14/35 Test  #8: tst_SeedService ..................   Passed    0.28 sec
      Start 17: tst_TopicTableProxy
15/35 Test #17: tst_TopicTableProxy ..............   Passed    0.07 sec
      Start 35: tst_Translations
16/35 Test #35: tst_Translations .................   Passed    0.06 sec
      Start 22: tst_ReminderService
17/35 Test #22: tst_ReminderService ..............   Passed    0.13 sec
      Start 18: tst_AgendaService
18/35 Test #18: tst_AgendaService ................   Passed    0.09 sec
      Start  3: tst_TopicRepository
19/35 Test  #3: tst_TopicRepository ..............   Passed    0.09 sec
      Start 16: tst_TopicBoardProxy
20/35 Test #16: tst_TopicBoardProxy ..............   Passed    0.05 sec
      Start 15: tst_TopicListModel
21/35 Test #15: tst_TopicListModel ...............   Passed    0.04 sec
      Start 19: tst_DayWatcher
22/35 Test #19: tst_DayWatcher ...................   Passed    0.01 sec
      Start  5: tst_ApplicationRepository
23/35 Test  #5: tst_ApplicationRepository ........   Passed    0.09 sec
      Start 12: tst_AgendaListModel
24/35 Test #12: tst_AgendaListModel ..............   Passed    0.03 sec
      Start 13: tst_ApplicationTableModel
25/35 Test #13: tst_ApplicationTableModel ........   Passed    0.06 sec
      Start  6: tst_InterviewRepository
26/35 Test  #6: tst_InterviewRepository ..........   Passed    0.03 sec
      Start  2: tst_TrackRepository
27/35 Test  #2: tst_TrackRepository ..............   Passed    0.02 sec
      Start  9: tst_TransactionBehavior
28/35 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start  1: tst_Database
29/35 Test  #1: tst_Database .....................   Passed    0.03 sec
      Start  7: tst_ReminderLogRepository
30/35 Test  #7: tst_ReminderLogRepository ........   Passed    0.02 sec
      Start  4: tst_SessionRepository
31/35 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start 14: tst_ApplicationFilterProxy
32/35 Test #14: tst_ApplicationFilterProxy .......   Passed    0.02 sec
      Start 20: tst_ExportService
33/35 Test #20: tst_ExportService ................   Passed    0.01 sec
      Start 11: tst_MonthView
34/35 Test #11: tst_MonthView ....................   Passed    0.09 sec
35/35 Test #31: tst_PageSettings .................   Passed    4.79 sec

100% tests passed, 0 tests failed out of 35

Label Time Summary:
data       =   0.63 sec*proc (9 tests)
i18n       =   0.06 sec*proc (1 test)
model      =   0.27 sec*proc (6 tests)
service    =   1.98 sec*proc (7 tests)
ui         = 106.99 sec*proc (12 tests)

Total Test time (real) =  55.68 sec
==> Checking translation sync with /usr/lib/qt6/bin/lupdate...
==> Translation files are in sync.
==> Quality check PASSED with zero warnings and all tests green!
```

#### Totals by Command
- **Test binaries:** 35 (compared to 34 in Part B; +1 binary `tst_Translations`)
- **Test functions:** 495 (compared to 473 in Part B; +22 functions net across all binaries)

---

## 10. Reviewer corrections (Part D, D0)

### 10.1 Complete Untrimmed Negative Control Test Run
```bash
JOBPREP_PSEUDO_EXPANSION=2.0 build/dev-linux/tests/ui/tst_HiDpi testPseudoTranslationHiDpi -o -,txt
```
```
CI-DIAG progress: main start t=0ms
CI-DIAG progress: main before QApplication t=0ms
CI-DIAG progress: main after QApplication t=0ms
********* Start testing of tst_HiDpi *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
QINFO  : tst_HiDpi::initTestCase() CI-DIAG progress: initTestCase start t=0ms
QINFO  : tst_HiDpi::initTestCase() CI-DIAG progress: initTestCase end t=250ms
PASS   : tst_HiDpi::initTestCase()
QINFO  : tst_HiDpi::testPseudoTranslationHiDpi() CI-DIAG progress: testPseudoTranslationHiDpi start t=250ms
CLIPPING_VIOLATION: [Pseudo | Page_1 | Light | 1826x700] QLabel (StudyColumnTitle) 'Васклог~абвгдежзийклмн': width (177) < minimumSizeHint.width (181)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Light | 1826x700] QLabel (StudyColumnTitle) 'Васклог~абвгдежзийклмн': horizontalAdvance (181) > contentsRect.width (177)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Light | 1826x700] QLabel (StudyColumnTitle) 'Iн Ррогресс~абвгдежзийклмнопрсту': width (184) < minimumSizeHint.width (251)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Light | 1826x700] QLabel (StudyColumnTitle) 'Iн Ррогресс~абвгдежзийклмнопрсту': horizontalAdvance (250) > contentsRect.width (184)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1826x700] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвгдежзийклмнопрс': width (151) < minimumSizeHint.width (255)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1826x700] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвгдежзийклмнопрс': button text advance (220) > contentsRect.width (151)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1826x700] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абвгдежзийклмноп': width (151) < minimumSizeHint.width (244)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1826x700] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абвгдежзийклмноп': button text advance (210) > contentsRect.width (151)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1826x700] QLabel (EmptyStateTitle) 'Nотхінг Рланнед~абвгдежзийклмнопрстуфхцчшщаб': width (292) < minimumSizeHint.width (449)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1826x700] QLabel (EmptyStateDescription) 'Uсе + Iнтервіеш ор + Fоллош-ур то ссхедуле сометхінг хере.~абвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийкл': width (292) < minimumSizeHint.width (750)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1826x700] QLabel (EmptyStateDescription) 'Uсе + Iнтервіеш ор + Fоллош-ур то ссхедуле сометхінг хере.~абвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийкл': height (45) < heightForWidth (54)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Light | 1826x540] QLabel (StudyColumnTitle) 'Васклог~абвгдежзийклмн': width (177) < minimumSizeHint.width (181)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Light | 1826x540] QLabel (StudyColumnTitle) 'Васклог~абвгдежзийклмн': horizontalAdvance (181) > contentsRect.width (177)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Light | 1826x540] QLabel (StudyColumnTitle) 'Iн Ррогресс~абвгдежзийклмнопрсту': width (184) < minimumSizeHint.width (251)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Light | 1826x540] QLabel (StudyColumnTitle) 'Iн Ррогресс~абвгдежзийклмнопрсту': horizontalAdvance (250) > contentsRect.width (184)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1826x540] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвгдежзийклмнопрс': width (151) < minimumSizeHint.width (255)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1826x540] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвгдежзийклмнопрс': button text advance (220) > contentsRect.width (151)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1826x540] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абвгдежзийклмноп': width (151) < minimumSizeHint.width (244)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1826x540] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абвгдежзийклмноп': button text advance (210) > contentsRect.width (151)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1826x540] QLabel (EmptyStateTitle) 'Nотхінг Рланнед~абвгдежзийклмнопрстуфхцчшщаб': width (292) < minimumSizeHint.width (449)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1826x540] QLabel (EmptyStateDescription) 'Uсе + Iнтервіеш ор + Fоллош-ур то ссхедуле сометхінг хере.~абвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийкл': width (292) < minimumSizeHint.width (750)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Light | 1826x540] QLabel (EmptyStateDescription) 'Uсе + Iнтервіеш ор + Fоллош-ур то ссхедуле сометхінг хере.~абвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийкл': height (38) < heightForWidth (54)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Dark | 1826x700] QLabel (StudyColumnTitle) 'Васклог~абвгдежзийклмн': width (177) < minimumSizeHint.width (181)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Dark | 1826x700] QLabel (StudyColumnTitle) 'Васклог~абвгдежзийклмн': horizontalAdvance (181) > contentsRect.width (177)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Dark | 1826x700] QLabel (StudyColumnTitle) 'Iн Ррогресс~абвгдежзийклмнопрсту': width (184) < minimumSizeHint.width (251)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Dark | 1826x700] QLabel (StudyColumnTitle) 'Iн Ррогресс~абвгдежзийклмнопрсту': horizontalAdvance (250) > contentsRect.width (184)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1826x700] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвгдежзийклмнопрс': width (151) < minimumSizeHint.width (255)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1826x700] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвгдежзийклмнопрс': button text advance (220) > contentsRect.width (151)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1826x700] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абвгдежзийклмноп': width (151) < minimumSizeHint.width (244)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1826x700] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абвгдежзийклмноп': button text advance (210) > contentsRect.width (151)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1826x700] QLabel (EmptyStateTitle) 'Nотхінг Рланнед~абвгдежзийклмнопрстуфхцчшщаб': width (292) < minimumSizeHint.width (449)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1826x700] QLabel (EmptyStateDescription) 'Uсе + Iнтервіеш ор + Fоллош-ур то ссхедуле сометхінг хере.~абвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийкл': width (292) < minimumSizeHint.width (750)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1826x700] QLabel (EmptyStateDescription) 'Uсе + Iнтервіеш ор + Fоллош-ур то ссхедуле сометхінг хере.~абвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийкл': height (45) < heightForWidth (54)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Dark | 1826x540] QLabel (StudyColumnTitle) 'Васклог~абвгдежзийклмн': width (177) < minimumSizeHint.width (181)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Dark | 1826x540] QLabel (StudyColumnTitle) 'Васклог~абвгдежзийклмн': horizontalAdvance (181) > contentsRect.width (177)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Dark | 1826x540] QLabel (StudyColumnTitle) 'Iн Ррогресс~абвгдежзийклмнопрсту': width (184) < minimumSizeHint.width (251)
CLIPPING_VIOLATION: [Pseudo | Page_1 | Dark | 1826x540] QLabel (StudyColumnTitle) 'Iн Ррогресс~абвгдежзийклмнопрсту': horizontalAdvance (250) > contentsRect.width (184)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1826x540] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвгдежзийклмнопрс': width (151) < minimumSizeHint.width (255)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1826x540] QPushButton (CalendarAddInterviewBtn) '+ Iнтервіеш~абвгдежзийклмнопрс': button text advance (220) > contentsRect.width (151)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1826x540] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абвгдежзийклмноп': width (151) < minimumSizeHint.width (244)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1826x540] QPushButton (CalendarAddFollowUpBtn) '+ Fоллош-ур~абвгдежзийклмноп': button text advance (210) > contentsRect.width (151)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1826x540] QLabel (EmptyStateTitle) 'Nотхінг Рланнед~абвгдежзийклмнопрстуфхцчшщаб': width (292) < minimumSizeHint.width (449)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1826x540] QLabel (EmptyStateDescription) 'Uсе + Iнтервіеш ор + Fоллош-ур то ссхедуле сометхінг хере.~абвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийкл': width (292) < minimumSizeHint.width (750)
CLIPPING_VIOLATION: [Pseudo | Page_3 | Dark | 1826x540] QLabel (EmptyStateDescription) 'Uсе + Iнтервіеш ор + Fоллош-ур то ссхедуле сометхінг хере.~абвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийклмнопрстуфхцчшщабвгдежзийкл': height (38) < heightForWidth (54)
FAIL!  : tst_HiDpi::testPseudoTranslationHiDpi() Compared values are not the same
   Actual   (allViolations.count()): 44
   Expected (0)                    : 0
   Loc: [/home/tony/projects/JobPrep/tests/ui/tst_HiDpi.cpp(919)]
QINFO  : tst_HiDpi::cleanupTestCase() CI-DIAG progress: cleanupTestCase start t=3365ms
QINFO  : tst_HiDpi::cleanupTestCase() CI-DIAG progress: cleanupTestCase end t=3372ms
PASS   : tst_HiDpi::cleanupTestCase()
Totals: 2 passed, 1 failed, 0 skipped, 0 blacklisted, 3502ms
********* Finished testing of tst_HiDpi *********
```

### 10.2 Complete File Lists for UI Search Surfaces
```bash
ls -1 src/ui src/ui/pages src/ui/dialogs src/ui/widgets
```
```
src/ui:
DashboardController.cpp
DashboardController.h
DataToolsController.cpp
DataToolsController.h
FormatHelper.cpp
FormatHelper.h
InAppNotifier.cpp
InAppNotifier.h
MainWindow.cpp
MainWindow.h
NotificationText.cpp
NotificationText.h
NotifierFactory.cpp
NotifierFactory.h
PageBase.cpp
PageBase.h
ReminderCoordinator.cpp
ReminderCoordinator.h
Sidebar.cpp
Sidebar.h
StudyLogging.cpp
StudyLogging.h
Toast.cpp
Toast.h
TrayNotifier.cpp
TrayNotifier.h
delegates
dialogs
pages
theme
widgets

src/ui/dialogs:
ApplicationDialog.cpp
ApplicationDialog.h
FollowUpDialog.cpp
FollowUpDialog.h
InterviewDialog.cpp
InterviewDialog.h
SessionDialog.cpp
SessionDialog.h
TrackDialog.cpp
TrackDialog.h

src/ui/pages:
CalendarPage.cpp
CalendarPage.h
HomePage.cpp
HomePage.h
JobsPage.cpp
JobsPage.h
SettingsPage.cpp
SettingsPage.h
StudyPage.cpp
StudyPage.h

src/ui/widgets:
ChipBar.cpp
ChipBar.h
EmptyState.cpp
EmptyState.h
FunnelWidget.cpp
FunnelWidget.h
GoalProgressBar.cpp
GoalProgressBar.h
MonthView.cpp
MonthView.h
ProgressRing.cpp
ProgressRing.h
SelectionRestorer.cpp
SelectionRestorer.h
SingleTabDateTime.h
StatCard.cpp
StatCard.h
TopicBoardColumn.cpp
TopicBoardColumn.h
TopicDetailPanel.cpp
TopicDetailPanel.h
TopicListView.cpp
TopicListView.h
```

### 10.3 Total Message Counts by Command
```bash
grep -c "<source>" resources/translations/jobprep_uk.ts resources/translations/jobprep_ru.ts
```
```
resources/translations/jobprep_uk.ts:506
resources/translations/jobprep_ru.ts:506
```

```bash
grep -c 'numerus="yes"' resources/translations/jobprep_uk.ts resources/translations/jobprep_ru.ts
```
```
resources/translations/jobprep_uk.ts:12
resources/translations/jobprep_ru.ts:12
```

### 10.4 Git Status and Git Worktree List
```bash
git status --short
```
```
(no output)
```

```bash
git worktree list
```
```
/home/tony/projects/JobPrep a13a356 [main]
```
