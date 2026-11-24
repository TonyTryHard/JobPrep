# README fix-up 2: Release recipe, requirements wording, status wording, links broken by the docs move

Read `docs/reports/README-task.md` (section 11), `README.md`, `docs/RUNNING.md`, `CMakePresets.json`, `CMakeLists.txt`, `AGENTS.md`. Same rules as before: local commits only, never push, never tag; raw output with the exact command above it, nothing trimmed (no `...`), `(no output)` for empty, hashes only from `git log --oneline`; scratch files outside the repository; Windows items you did not run are "not verified by agent". No source code changes.

## 1. Windows Release recipe (README and `docs/RUNNING.md` if it repeats it)

The recipe `cmake -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release` has no Qt location. Show how the project finds Qt: `grep -n "QT_PREFIX_PATH\|CMAKE_PREFIX_PATH" CMakeLists.txt CMakePresets.json -r` (raw) and what the `dev-windows` preset sets (`cacheVariables`, `environment`, `inherits`). Then:
- if the project reads `QT_PREFIX_PATH` from CMake itself, say so and keep the lines, adding the `set QT_PREFIX_PATH=...` line it needs;
- if it is only the preset that wires Qt, add `-DCMAKE_PREFIX_PATH=%QT_PREFIX_PATH%` to the configure line, and say in the report that this form is derived from the files, not run on Windows;
- show what makes `windeployqt` resolvable (`%QT_PREFIX_PATH%\bin` on `PATH`, or the full path `%QT_PREFIX_PATH%\bin\windeployqt.exe`) and write that in the recipe;
- say in the text that the recipe starts from the "x64 Native Tools Command Prompt for VS 2022" (needed for the build only, not for running the result), and that the deployed folder runs by double-click without Qt on `PATH`.

Keep the "maintainer recipe, not part of CI" note.

## 2. Requirements wording

Replace "GCC 11+ or Clang 13+ (Linux), MSVC 2022 v143+ (Windows)" by what is proven: `A C++20 compiler (CI uses GCC on ubuntu-latest and MSVC 2022 on windows-latest)`. Show the grep that proves the CI compilers (`grep -n "gcc\|g++\|msvc\|cl.exe\|vcvars\|ilammy" .github/workflows/ci.yml`, raw). Keep CMake 3.21 and Qt 6.8 (both are in the files). Check that the apt package list in the README equals the CI install step: show `grep -n -A8 "apt-get install" .github/workflows/ci.yml`, raw, and make the README list match it (or say "as in CI").

## 3. Status wording

Replace the first sentence of "Status and Scope" ("fully implemented and verified across both Linux and Windows") by a plain one: JobPrep is a working application; all five pages are implemented and covered by the test suites; CI runs on Linux and Windows. No "fully", no "verified".

## 4. Links broken by moving `docs/prompts` and `docs/plans` to `archive/dev-docs`

The directories were removed from `main`. Find every remaining reference on `main`:
`git grep -n "docs/prompts\|docs/plans\|M[0-9]-plan\|prompts/" -- . ':!docs/reports'` and the same count including `docs/reports` (`| wc -l`), raw.
- In files that are **living documents** (`README.md`, `AGENTS.md`, `docs/RUNNING.md`, `docs/SPEC.md`, `docs/i18n-glossary.md`): fix or reword each reference (for example "see the `archive/dev-docs` branch" or remove the sentence); list every change.
- In `docs/reports/*` (historical records): do **not** edit; give the count and say they are historical.
- Check that no link in the living documents points to a missing path: for each relative Markdown link, test the path with `ls`, raw (as you did for the README images).
- Report whether `scripts/` or CI reference `docs/prompts` or `docs/plans` (`grep -rn "docs/prompts\|docs/plans" scripts .github`, raw).

Also correct the commit list in the report: `71d64dd docs(readme): add contributing section` is listed as "(branch: archive/dev-docs)" but appears in the `main` log. Show `git branch --contains 71d64dd` and `git log --oneline archive/dev-docs -5` (raw) and write the real order.

## Commits (separate)

(1) README (sections 1 to 3), (2) docs link fixes (section 4), (3) report last: append "Fix-up 2" to `docs/reports/README-task.md` with the evidence, then the final `git status --short`, `git worktree list`, `git log --oneline` of this work including the report commit, and one full `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` raw run.
