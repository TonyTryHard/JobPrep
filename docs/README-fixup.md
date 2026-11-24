# README fix-up: wrong Windows data path, unsafe isolation snippet, wrong roadmap, unverified claims

Read `docs/reports/README-task.md`, `README.md`, `docs/RUNNING.md`, `docs/reports/M6-partD2-fixups.md` (the section about `AppDataLocation` that cannot be redirected on Windows), `docs/SPEC.md`. All rules of the README task prompt (archived at `docs/prompts/README-task.md` on the `archive/dev-docs` branch) apply: local commits only, never push, never tag; raw output with the exact command above it, **nothing trimmed (no `...` inside any output block)**, `(no output)` for empty, hashes only from `git log --oneline`; scratch files outside the repository. Windows items you did not run are "not verified by agent". No source code changes.

## 1. Data paths on Windows are wrong, and the isolation snippet is unsafe (blocking)

- The code uses `QStandardPaths::AppDataLocation`. On Windows that is the **Roaming** profile (`%APPDATA%\JobPrep\JobPrep\`), not `%LOCALAPPDATA%` (that would be `AppLocalDataLocation`). Prove it from the code (`grep -rn "AppDataLocation\|AppLocalDataLocation" src`, raw) and from the Qt documentation for the Qt version in use (quote the relevant sentence and the URL or the local doc path you read; if you cannot read it, say so and use `%APPDATA%` with the label "derived, not run on Windows"). Fix **every** Windows path in `README.md` (database, backups) and in `docs/RUNNING.md` if it has the same mistake (`grep -n "LOCALAPPDATA\|APPDATA" README.md docs/RUNNING.md`, raw before and after). The settings in the registry (`HKEY_CURRENT_USER\Software\JobPrep\JobPrep`) stay.
- The README section "Running in Complete Isolation" gives a Windows snippet (`$env:LOCALAPPDATA = ...`) that does not isolate anything: Qt resolves this folder with the shell API, not from the environment (the repo's own test skips for this reason: show `grep -n "cannot be redirected" tests -r`, raw). A reader who follows it would still open and modify the real database. **Remove the Windows snippet.** Keep the Linux one only after you have shown (raw, in a throw-away `HOME`, without starting the real app against real data: for example with the existing test fixture or `strace`-free check of `QStandardPaths` in a scratch program outside the repository) that `HOME` plus `XDG_DATA_HOME`/`XDG_CONFIG_HOME` really redirect both the database and the settings. For Windows write one honest sentence instead: the data folder cannot be redirected by environment variables on Windows; back it up first (Settings -> Data -> backup) before trying a dev build.

## 2. Roadmap is wrong and carries internal jargon

- The milestone list does not match the project history (for example the job pipeline is not M4 and backup/restore is not M5; calendar and interviews were M3, the Study page M4, dashboard and reminders M5, polish and data tools such as ICS and JSON backup M6, translations and Qt 6.10 compatibility M7). Take the real mapping from `docs/reports/M*.md` and `docs/SPEC.md` (show the greps), or, better, **replace the milestone checklist with a short status paragraph** (what works, what is deliberately out of scope) with no milestone numbers: public readers do not need them.
- Remove "(Option B)" and other internal labels from the Known Limitations text; state the fact only: custom mnemonic accelerators are not defined for Ukrainian and Russian, standard buttons use Qt's own.

## 3. Claims that need command evidence (each one shown, or removed/softened)

For each, a command and the raw result; if the code disagrees, fix the README:
1. "no remote servers, no user accounts, no background telemetry": `grep -rn "QNetwork\|QTcpSocket\|QUrl\|QDesktopServices" src CMakeLists.txt` and `grep -n "Network" CMakeLists.txt`, raw, and a verdict (a link opened in the browser is not telemetry; say so if present).
2. "22 standard columns" of the CSV export, with the header row the code writes.
3. Reminder lead times "24 hours, 1 hour": the real lead-time options in code/SPEC and what the reminder service covers (interviews only, or also follow-ups, study targets?).
4. "6 accent colour presets (Indigo, Teal, Rose, Amber, Emerald, Sky)" and "themes switch instantly at runtime without restart": the code or test that shows both.
5. Build requirements: "GCC 11+ or Clang 13+", "MSVC 2022 v143+", "CMake >= 3.21": show `cmake_minimum_required`, the compilers the CI actually uses, and the minimum you can justify; write only what the project requires or what CI proves ("CI uses ...").
6. The four CI job names: show the `name:` lines of `ci.yml`, and write the real names only.
7. "all 506 UI strings": this number goes stale; remove the number or say "all UI strings".
8. The Windows deployment recipe: the report says it is "verified in CI offscreen"; show whether `ci.yml` runs `windeployqt` at all (`grep -n windeployqt .github/workflows/ci.yml scripts -r`, raw). If not, remove that sentence from the report. In the README keep the recipe in the form the maintainer actually ran on his PC: Release build, copy `jobprep.exe` into a folder, `windeployqt --release <folder>\jobprep.exe` with the Qt `bin` directory on `PATH` or the full path, no `--no-translations` (so `qtbase_*.qm` are copied by the tool), and a note that it is the maintainer's recipe, not part of CI. Check that `cmake --preset dev-windows -DCMAKE_BUILD_TYPE=Release` really produces a Release build with this preset (read the preset in `CMakePresets.json`, raw); if the preset forces another build type or Werror/test targets, say what the recipe must use instead.

## 4. Report hygiene (`docs/reports/README-task.md`)

- Section 3 trims output with `...` twice; replace by the real, complete output of the clean-clone run (re-run it if needed, timed) or move the full log to `docs/reports/img/`-style file is NOT allowed: paste it fully.
- The suite counts by area are inconsistent (the "Models" group says 6 and lists 7, "UI" says 12 and lists 11): count by command (`ctest -N | grep -c ...` per group) and fix.
- The final `git log --oneline` must include **the report commit** (it is missing), and the order of commits must be stated.
- The final `check.sh` again says `lupdate not found, skipping`; show `QT_PREFIX_PATH=/usr/lib/qt6 CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` (or whatever makes the translation sync run on your machine) once, raw, so that the sync check is shown to run here.

## 5. Commits (separate)

(1) README fixes (sections 1 to 3), (2) `docs/RUNNING.md` path fixes if any, (3) report last (`docs/reports/README-task.md` updated with this fix-up as a new section, evidence raw), with the final `git status --short`, `git worktree list`, `git log --oneline` of this work including the report commit.
