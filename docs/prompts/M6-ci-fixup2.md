# M6: CI fix-up 2 (first real GitHub run: Qt 6.8.2 compile error, keep-going build)

Read `AGENTS.md` (§2, §9, §12), `docs/SPEC.md` (the section that names the Qt version and toolchain), `docs/reports/M6-ci-fixup.md`, `.github/workflows/ci.yml`, `scripts/check.sh`, `scripts/check.ps1`, `CMakeLists.txt`, `tests/data/TestDatabase.cpp`.
Local commits only, never push. Code fix, CI/scripts change and report are **separate commits**, the report last. Rules as always: raw output of real commands with the command above it, nothing trimmed, no line numbers as evidence, hashes from `git log --oneline`, no claim without a command.

## Facts from the first GitHub Actions run (raw logs exist, do not re-derive)

1. Both jobs (`Gate (ubuntu-latest)`, `Gate (windows-latest)`) passed every setup step: checkout, Python, Qt install via aqtinstall (`linux_gcc_64`, `win64_msvc2022_64`), Qt prefix discovery, diagnostics, `where Qt6Core.dll` found `D:\a\JobPrep\Qt\6.8.2\msvc2022_64\bin\Qt6Core.dll`, CMake configure with the presets. The infrastructure part of the workflow works.
2. Runner toolchains actually used: Linux GCC 15.2.0, CMake 4.4.4, Ninja 1.13.2, Qt **6.8.2**. Windows: MSVC 14.51 (Visual Studio 18), Qt 6.8.2 `msvc2022_64`. The development machine uses Qt **6.10.2**.
3. Both jobs then fail in the build with the **same** compile error, in the target `jobprep_test_helpers`, file `tests/data/TestDatabase.cpp`, in two functions (`hasTable(QStringView)` and the index twin right below it):
   - GCC: `error: no match for 'operator+' (operand types are 'QString' and 'QStringView')`
   - MSVC: `error C2678: binary '+': no operator found which takes a left-hand operand of type 'QString'`
   - the expression is `u"SELECT ... name='"_s + name + ...` where `name` is a `QStringView`. Qt 6.8.2 has no `QString + QStringView`; Qt 6.10.2 accepts it.
4. Ninja stops at the first failing file ("build stopped: subcommand failed" at step 81 of 214 on Linux, 83 of 214 on Windows). **Everything after that file is unknown**: more Qt 6.8.2 incompatibilities and, on Windows, the `/W4 /permissive- /WX` warnings have not been reached yet.
5. The "Upload test logs" step found no `LastTest.log` (the build failed before ctest ran): harmless, but it prints a warning.

## Tasks

1. **Evidence about the code.** Show: `git log --oneline -- tests/data/TestDatabase.cpp`, the function bodies around the failing expressions (`grep -n -B3 -A6 "sqlite_master" tests/data/TestDatabase.cpp`), and the declared Qt version: `grep -n -i "qt" docs/SPEC.md | grep -iE "6\.[0-9]+" | head -20`, `grep -n "find_package(Qt6" CMakeLists.txt`, `grep -rn "6\.8\|6\.10\|6\.5" AGENTS.md CMakeLists.txt CMakePresets.json .github/workflows/ci.yml`. State in one sentence which Qt version the project **promises** to support (quote the SPEC line). If SPEC names no minimum, say so; do not invent one.
2. **Fix the code** so it compiles with Qt 6.8.2 without changing behavior: convert the view explicitly (`name.toString()`) or build the SQL with `QString::arg` / `QStringBuilder` that accepts `QStringView`. Binding a parameter instead of concatenating is better if the helper's contract allows it (these are test helpers with trusted constant names; keep it simple). No behavior change, no test removed.
3. **Scan for the same class of problem** in the whole tree, with commands (paste raw output): `grep -rnE "QStringView" src tests | grep -E "\+|operator"`; `grep -rnE "_s *\+|\+ *u\"" src tests | head -40`. For each hit say whether it is safe on Qt 6.8.2 and why. **You cannot compile against 6.8.2 here** (the machine has 6.10.2): do not claim the scan proves anything, it only finds candidates. If another Qt 6.8-incompatible API is found by reading headers you have, fix it; otherwise leave the rest to the next CI run.
4. **Keep-going build for CI.** Add an environment variable (for example `JOBPREP_KEEP_GOING=1`) to `scripts/check.sh` and `scripts/check.ps1` that adds `-- -k 0` to the `cmake --build` call (Ninja then reports **all** failing files in one run). Without the variable the behavior is unchanged. Set it in the workflow for both OS. Show `git diff` of both scripts and run the gate once **without** it (default behavior) to prove nothing changed.
5. **Upload step.** `if-no-files-found: ignore` on the test-log upload (no warning when ctest never ran). Do not change the pinned SHAs. Do not touch the Node 20 deprecation warning for now (the five actions still work; note it in the report as a future update, pins must then be re-verified with `git ls-remote`).
6. **Do not** change the Qt version in the workflow, and do not add a Qt minimum to `find_package` in this task. Instead, in the report, say whether `find_package(Qt6 <minimum> ...)` would be right according to the SPEC and let the user decide.
7. **Local gate.** Run `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` (default behavior) and paste the full raw output; then test totals by command (binaries and functions) and compare with the totals in `docs/reports/M6-partC.md`: they must be unchanged. After the gate: `git status --short` must be empty (if it is not, say which files changed and why; this is the known screenshot issue from Part C C0.2).
8. **Static checks.** YAML parses (`python3 -c "import yaml; yaml.safe_load(open('.github/workflows/ci.yml')); print('yaml ok')"`), `actionlint` if installed (else "not run"), `git diff --stat` of each commit, `git log --oneline` of this task.

## Report `docs/reports/M6-ci-fixup2.md` (short)

Facts used (the five above, as I gave them); evidence of tasks 1 to 3 with raw output; the diffs; gate output and totals; honesty section.
"Not verified by agent, user to check" must contain: the fix compiles on Qt 6.8.2 (only the next GitHub run can say), the rest of the tree builds on Qt 6.8.2, the Windows `/W4 /WX` flags on the whole tree (not reached so far), ctest on both runners (not reached so far), Qt cache hit on a later run (the cache is only saved after a **successful** job, so the Qt download repeats until the first green run).
