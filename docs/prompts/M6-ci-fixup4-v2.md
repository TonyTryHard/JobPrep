# M6: CI fix-up 4 v2 (Windows: fix what is now proven, diagnose the QSettings problem)

This file replaces `M6-ci-fixup4.md` and is self-contained. If you already did part of the first version (font variable, diagnostic step v2, `CI-DIAG` lines), keep it, do not duplicate it, and say in the report what was already there.

Read `AGENTS.md` (§2, §9, §12), `.github/workflows/ci.yml`, `scripts/check.ps1`, `docs/reports/M6-ci-fixup3.md`, `tests/ui_test_helpers/` (the "unexpected Qt warnings" guard and `UiTestHelpers.cpp` where `QSettings::setDefaultFormat` is set), `src/services/SettingsService.cpp`, `src/services/SeedService.cpp`, `src/ui/theme/IconProvider.cpp`, `src/app/AppContext.cpp` (see the comment and `Q_INIT_RESOURCE` near line 23), and the failing tests named below.
Local commits only, never push. Rules as always: raw output of real commands with the command above it, nothing trimmed, empty output as `(no output)`, hashes only from `git log --oneline`, no claim without a command. Separate commits in this order: (1) workflow, (2) `fix(core)` resources, (3) `fix(tests)` portable tests, (4) `CI-DIAG` diagnostics (every added line marked `// CI-DIAG`, so the user can revert that one commit), (5) report last. Do not touch anything else.

## Facts from the full Windows logs (artifact `diag/*.txt` of the third run; Qt 6.8.2, MSVC 2022, Windows Server 2025; Linux 33/33 green)

**A. Font directory (sure cause of at least 9 failures).** `QFontDatabase: Cannot find font directory D:/a/JobPrep/Qt/6.8.2/msvc2022_64/lib/fonts`. The `offscreen` platform on Windows reads fonts from `<prefix>/lib/fonts` or from the directory in `QT_QPA_FONTDIR`. The test guard turns that Qt warning into `FAIL!` in `tst_UiSmoke::testZeroWarnings` and in `cleanupTestCase` or the first slot of `tst_PageJobs`, `tst_PageCalendar`, `tst_PageStudy`, `tst_PageHome`, `tst_PageSettings`, `tst_Shortcuts`. The same QWARN is in `tst_ImportExport`, `tst_Notifiers`, `tst_StudyRegression`.

**B. Qt resources not available in tests that never build an `AppContext`** (strong evidence, two tests):
- `tst_AgendaListModel::agendaKindStyle`: `'!icon.isNull()' returned FALSE` (`IconProvider` returns an empty icon when `:/icons/*.svg` cannot be opened).
- `tst_ImportExport::testRestoreEqualitySeedDataset`: `'seed.loadSampleTopics(&seedError)' returned FALSE` (seed data is a resource). `tst_SeedService`, which calls `Q_INIT_RESOURCE(resources)` itself, passes on Windows.
`Q_INIT_RESOURCE` is called in the `AppContext` constructor only. On MSVC the resource object of the static library `jobprep_core` is not pulled into binaries that do not reference it; on Linux it happens to work.

**C. Two tests assume Linux behaviour:**
- `tst_ImportExport::testRestoreSafetyBackupCreatedPrunedAndAbortsWhenReadOnly`: `'!roRes.success' returned FALSE`. The test makes a directory "read-only" with `QFile::setPermissions`; Windows ignores that flag on directories, so the restore succeeds.
- `tst_ImportExport::testRestoreSafetyBackupProductionPathWithCustomXdgDataHome`: `res.safetyBackupPath.startsWith(customXdg)` FALSE. `QStandardPaths` ignores `XDG_DATA_HOME` on Windows.

**D. QSettings values are not read back** (five cases, cause unknown):
- `tst_SettingsService::testTrayHintShownKey`: second `QSettings(path, IniFormat)` on the same file does not see the value written by the first (the file comes from a `QTemporaryFile` that was closed).
- `tst_PageSettings::testWeeklyGoalPersistence`: same pattern (`QTemporaryFile` + `IniFormat`), expected 540, got 360.
- `tst_TopicListModel::settingsKeysAndFallbacks` (line 319): default `QSettings` (this test does not call `setDefaultFormat`, so the **registry** on Windows): `setStudyViewMode("list")` then `studyViewMode()` in the **same** object returns `"board"`.
- `tst_StudyRegression::testChecklistStartupContrastDarkAndLight` (line 855): `Services::SettingsService settings; settings.setThemeMode(mode);` then `AppContext` reads `themeMode()` and gets `"system"`.
- `tst_Notifiers::coordinatorEndToEndSettingsLeadMinutesChangeTakesEffect` (line 654): after `ctx.settings().setInterviewLeadMinutes({60})` a toast is still produced (the default `{1440, 60}` is still in effect).
No `QSettings` warning is in any log. Qt documents that a default-constructed `QSettings` without organization and application name cannot read or write and reports `AccessError`; whether `QCoreApplication::applicationName()` is empty or malformed in these test processes on Windows is **not known**. Do not assume it.

**E. Not yet explained:** `tst_StudyRegression::testWidgetAtReturnsRealControlsNotOverlay` (line 267): `QApplication::widgetAt()` at the centre of `StudyDetailCheckAddBtn` returns `StudyDetailPanelCard`. May be a consequence of A (no fonts, other sizes). Re-check after the font fix; do not change it now.

**F. Not failures but noted:** the UI tests take 60 to 114 s on Windows (about 1 to 5 s on Linux); `UiTestHelpers::activateWindow` waits up to 1000 ms per call when the offscreen window never becomes active (hypothesis). Do not change in this task.

## Tasks

1. **Evidence.** `git log --oneline -6`, `git reflog -n 10` (the fix-up 3 report cites `949b6c7`, the log shows `87193dc`: explain, do not rewrite history), `grep -n -A40 "Diagnose failed tests" .github/workflows/ci.yml`, and the list of every place that opens a resource: `grep -rn '":/\|u":/\|QStringLiteral(":/' src | cut -c1-140` (raw).

2. **Workflow commit.**
   - In the steps "Run quality gate (Windows)" and "Diagnose failed tests (Windows)" add `QT_QPA_FONTDIR: C:\Windows\Fonts`. Keep the two stderr variables. Linux job untouched. Say what you could confirm about `QT_QPA_FONTDIR` in the Qt sources or docs you have; otherwise "unconfirmed until the next run".
   - Diagnostic step v2: run each failed test **without `-v2`** (default verbosity), still `-o "<logdir>/<name>.txt,txt"`, same 150 s limit and exit-code line; then print every line matching `FAIL!|XFAIL|QWARN|Totals:` and the 8 lines after each `FAIL!`; if the file has at most 300 lines print all of it. Never print `INFO` lines. Keep `continue-on-error: true` and the artifact paths.
   - New step "Registry write probe (Windows)", `if: failure() && runner.os == 'Windows'`, `continue-on-error: true`, `shell: pwsh`: create `HKCU:\Software\JobPrepProbe`, write a string value, read it back, delete the key, print each result and any exception text. Also print `$env:USERNAME`, `whoami`, and `(Get-Item HKCU:\Software).Name`. This tells us whether the runner registry is writable at all.

3. **`fix(core)`: make resource initialisation independent of `AppContext`.** Add one small function (for example `JobPrep::App::ensureResources()` in a new `src/app/Resources.cpp/.h`, added to `jobprep_core`) that calls `Q_INIT_RESOURCE(resources)` the way `AppContext.cpp` does today (read the comment there about the namespace mangling and keep it correct); it must be safe to call many times. Call it from `IconProvider::icon`, from `SeedService` before it reads a resource, and from every other place the grep of task 1 shows that opens `:/` (theme/stylesheet loaders included), and keep the `AppContext` call. No `#ifdef` per platform. Show the diff. On Linux run `tst_AgendaListModel`, `tst_SeedService` and `tst_ImportExport` (raw totals) to show nothing broke. Say plainly that the Windows effect is verified only by the next CI run.

4. **`fix(tests)`: portable tests, no weaker assertions.**
   - Read-only test: replace the directory-permission trick by something that fails on every OS, for example a safety directory path whose parent is a regular file (so `mkpath` must fail). It must still assert `!roRes.success`, `errorKind == SafetyBackupFailed` and that the database dump is unchanged.
   - XDG test: assert the part that is true everywhere (`safetyBackupPath` is under `QStandardPaths::writableLocation(AppDataLocation) + "/backups"` computed **after** setting the variable) and assert the `customXdg` prefix only when `QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation).startsWith(customXdg)` (that is, when the variable took effect). Restore the environment variable at the end of the test.
   - Show both diffs and run both slots on Linux (raw).

5. **`CI-DIAG` commit: QSettings diagnostics** (use `qInfo()` or `QTest::qWarn` only after you show from `tests/ui_test_helpers/` that the unexpected-warnings guard does not count it; show the lines proving that). In each of the five D-cases, right before the failing assertion, log for every `QSettings` object involved (and for a fresh `QSettings probe;` in the registry cases): `QCoreApplication::organizationName()`, `QCoreApplication::applicationName()`, `QCoreApplication::arguments().value(0)`, `fileName()`, `int(format())`, `int(status())` (after calling `sync()`), `isWritable()`, `value("<the key>")`. In the two `QTemporaryFile` cases also log `QFileInfo(path).exists()`, `.size()`, and the first 20 lines of the file, once after the first scope ends. Also add the same facts once at the start of `tst_TopicListModel::settingsKeysAndFallbacks` for a native-format `QSettings`, and a one-time write/read probe there: `QSettings probe(QSettings::NativeFormat, QSettings::UserScope, "JobPrepProbe", "probe"); probe.setValue("k", 1); probe.sync();` then log `status()` and `value("k")`. Diagnostics must compile warning-free on GCC and MSVC and must not change any assertion.

6. **Local gate.** `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` full raw output; totals by command (binaries and functions) must equal 33 / 454 (say so if a slot count changed and why); `git status --short` empty. Paste the raw output of `build/dev-linux/tests/services/tst_SettingsService testTrayHintShownKey` and `build/dev-linux/tests/models/tst_TopicListModel settingsKeysAndFallbacks` so the user can see the diagnostic lines on Linux (the "good" reference).

7. **Static checks.** YAML parse; `git diff --stat` per commit; `command -v pwsh actionlint` (say "not run" if missing). The existing diagnostic step ran correctly on the runner, so change its structure as little as possible.

8. `git log --oneline` of this task.

## Report `docs/reports/M6-ci-fixup4-v2.md` (short)

Facts A to F as given; evidence for tasks 1 to 7; honesty section. "Not verified by agent, user to check" must contain: that `QT_QPA_FONTDIR` removes the font warning (next run), that `ensureResources()` fixes the two resource failures on MSVC (next run), that the portable tests pass on Windows (next run), that the PowerShell changes work, and that **the cause of the five QSettings failures (D) and of E is unknown until the diagnostic lines and the registry probe are read**. List which of the 13 failed Windows tests you expect to turn green after this task and which you expect to stay red; that is a prediction, label it so.
