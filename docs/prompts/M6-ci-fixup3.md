# M6: CI fix-up 3 (Windows runner: 13 of 33 tests fail with EMPTY output, make the reason visible)

Read `AGENTS.md` (§2, §9, §12), `.github/workflows/ci.yml`, `scripts/check.ps1`, `scripts/check.sh`, `tests/CMakeLists.txt` and the CMakeLists of `tests/models`, `tests/services`, `tests/ui`, `tests/ui_smoke`, `docs/reports/M6-ci-fixup2.md`.
Local commits only, never push. **This task changes only CI/scripts and the report. Do not touch any test or any source file, do not "fix" tests blindly.** Workflow change and report are **separate commits**, the report last. Rules as always: raw output of real commands with the command above it, nothing trimmed, empty output written as `(no output)`, hashes only from `git log --oneline`, no claim without a command.

## Facts from the second GitHub Actions run (raw logs exist, do not re-derive)

1. Ubuntu job: build ok, `100% tests passed out of 33`, Qt 6.8.2. Linux is green.
2. Windows job: configure and build ok (222 of 222 targets, MSVC `/W4 /permissive- /WX`, zero `warning C` lines). Then: `61% tests passed, 13 tests failed out of 33`, total test time 227 s.
3. Failed on Windows: `ui_smoke`, `tst_AgendaListModel`, `tst_TopicListModel`, `tst_ImportExport`, `tst_SettingsService`, `tst_Notifiers`, `tst_StudyRegression`, `tst_PageJobs`, `tst_PageCalendar`, `tst_PageStudy`, `tst_PageHome`, `tst_PageSettings`, `tst_Shortcuts`. Passed on Windows: all 9 `tests/data` tests, `tst_MonthView`, `tst_ApplicationTableModel`, `tst_ApplicationFilterProxy`, `tst_TopicBoardProxy`, `tst_TopicTableProxy`, `tst_AgendaService`, `tst_DayWatcher`, `tst_ExportService`, `tst_ReminderService`, `tst_StatsService`, `tst_EmptyStatesAndFocus`.
4. `scripts/check.ps1` **already** runs `ctest --preset dev-windows -j N --output-on-failure`, yet after every `***Failed` line the output block is **empty** (one blank line). So the failure text never reaches ctest's pipe. The uploaded artifact `test-logs-windows-latest` is only 3103 bytes (2 files), so it most likely holds the same empty output.
5. Several UI tests take very long on Windows (`tst_PageStudy` 97 s, `tst_PageHome` 88 s, `tst_PageJobs` 59 s, `tst_PageCalendar` 33 s) against about 1 to 5 s on Linux. This looks like many waits timing out (for example `QTRY_*` with the 5 s default), i.e. the tests really run and really fail, only their text is lost. This is a hypothesis, not a fact.
6. Known Qt behavior on Windows (verify in the Qt docs you have, do not just trust me): `qDebug/qWarning/qFatal` output goes to the debugger (`OutputDebugString`) instead of stderr unless stderr is recognized as a console; the environment variables `QT_FORCE_STDERR_LOGGING=1` and `QT_ASSUME_STDERR_HAS_CONSOLE=1` change that. QtTest's own result lines can be written to a file with `-o <file>,txt`, which bypasses every console/pipe problem.

## Goal

The **next** GitHub run must print, for every failing Windows test, the real QtTest output (`FAIL!` lines with file and line of the check, `QWARN`, `Totals`) and the process exit code. One run, no guessing.

## Tasks

1. **Evidence first.** Show: `cat .github/workflows/ci.yml` (full), `cat scripts/check.ps1` (full), and the `add_test` / `set_tests_properties` blocks of the five tests CMakeLists. Show `command -v pwsh actionlint` (say "not installed" if missing).
2. **Environment for the Windows run.** In the workflow, step "Run quality gate (Windows)", add `QT_FORCE_STDERR_LOGGING: 1` and `QT_ASSUME_STDERR_HAS_CONSOLE: 1` to `env`. Do the same for Linux only if it is harmless (it is; but keep the diff minimal and say what you did). Do not change `check.sh`/`check.ps1` for this.
3. **New diagnostic step, Windows only, `if: failure() && runner.os == 'Windows'`, `shell: pwsh`**, placed after the gate step and **before** the artifact upload. It must:
   - read the failed test names from `build/dev-windows/Testing/Temporary/LastTestsFailed.log` (format `<number>:<name>`); if the file is missing, print `LastTestsFailed.log missing` and fall back to listing all `*.exe` under `build/dev-windows/tests` whose name starts with `tst_` or equals `ui_smoke`;
   - for each failed name find its executable with `Get-ChildItem build/dev-windows -Recurse -Filter "<name>.exe"`;
   - run it **directly**, serially, with `$env:QT_QPA_PLATFORM = "offscreen"`, arguments `-v2 -o "<logdir>/<name>.txt,txt"`, and a **per-test time limit of 150 s** (`Start-Process -PassThru` plus `WaitForExit(150000)`, kill on timeout and say `TIMEOUT`);
   - after each run print: a header line with the test name, the exit code in decimal **and** hex (`'{0} (0x{0:X8})' -f $code`), then the content of `<logdir>/<name>.txt` (full if under 200 lines; otherwise the first 120 and the last 60 lines, and say how many lines were cut);
   - never fail the step itself (`continue-on-error: true` on the step, and no `exit 1` inside it), so that the artifact upload still runs.
   - Put `<logdir>` under `build/dev-windows/Testing/Temporary/diag` so that the existing artifact glob picks it up; extend the artifact `path:` list with `build/*/Testing/Temporary/diag/*.txt` and with `build/*/Testing/Temporary/LastTestsFailed.log`.
   - Keep the script inline and short (no new files in the repo), pinned action SHAs unchanged, no new actions.
4. **Static checks.** `python3 -c "import yaml; yaml.safe_load(open('.github/workflows/ci.yml')); print('yaml ok')"`; `actionlint` if installed (else write "not run"); if `pwsh` is installed, parse-check the inline script by copying it into a temp `.ps1` and running `pwsh -NoProfile -Command "[System.Management.Automation.Language.Parser]::ParseFile('<tmp.ps1>',[ref]$null,[ref]$e); $e"` and paste the raw output (empty = ok); if `pwsh` is not installed write "not run". Show `git diff` of the workflow and `git diff --stat`.
5. **Local regression.** Run `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` once and paste the full raw output (nothing in `src/` or `tests/` changed, so the totals must equal the last report; compare by command). Then `git status --short` must be empty (if not, say which files and why).
6. **Do not** try to guess the cause in code. If, while reading the CMake files, you notice something Windows-specific (for example default `QSettings` format, temp-file handling, `\n` vs `\r\n` in compared text, path separators), list it in the report as **candidate, unverified**, with the file and a grep command that shows it. No fixes.
7. `git log --oneline` of this task.

## Report `docs/reports/M6-ci-fixup3.md` (short)

The six facts as I gave them; evidence of tasks 1 to 5 with raw output; the diffs; honesty section.
"Not verified by agent, user to check" must contain: that the diagnostic step really runs on the Windows runner and prints the output (only the next run can say), that the PowerShell inline script is correct (unless the parse check was run), that `QT_FORCE_STDERR_LOGGING` / `QT_ASSUME_STDERR_HAS_CONSOLE` change anything for the lost output, and the **cause of the 13 Windows failures: unknown until that run**.
