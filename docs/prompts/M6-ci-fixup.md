# M6: CI workflow fix-up (small, `.github/` only)

Read `AGENTS.md` (§2, §9, §12), `docs/prompts/M6-amendments.md` (A2), `docs/reports/M6-partA.md`, `.github/workflows/ci.yml`, `CMakePresets.json`, `scripts/check.sh`, `scripts/check.ps1`.
The workflow from Part A has never run. The user checked its dependencies by running commands on a real machine; the findings below are facts, not guesses. Fix the workflow accordingly. Touch only `.github/` and the report; no production code, no tests. Local commit only, never push. Fix commit and report commit are separate.

## Facts verified by the user (raw output exists, do not re-derive)

1. `aqtinstall==3.2.0` exists and installs under Python 3.14 (`pip install aqtinstall==3.2.0`: "Successfully installed aqtinstall-3.2.0 ...").
2. `aqt list-qt linux desktop --arch 6.8.2` prints exactly **`linux_gcc_64`**. The workflow's `gcc_64` is invalid for Qt 6.8.2. (The installed folder is normally still named `6.8.2/gcc_64`; do not rely on that, see task 2.)
3. `aqt list-qt windows desktop --arch 6.8.2` prints `win64_llvm_mingw win64_mingw win64_msvc2022_64 win64_msvc2022_arm64_cross_compiled`. Use **`win64_msvc2022_64`**.
4. The add-on module list for Qt 6.8.2 on Linux (`--modules 6.8.2 linux_gcc_64`) contains no `qtsvg`, `qtbase` or `qtsql`; they are part of the base install. **Remove `-m qtsvg`** (and any other `-m` for base modules) on both OS. The module list does contain `qtimageformats`; do not add it unless a test needs it.
5. Action pins from `git ls-remote` (annotated vs lightweight tags checked):
   - `actions/checkout` v4.2.2 = `11bd71901bbe5b1630ceea73d27597364c9af683`
   - `actions/setup-python` v5.4.0 = `42375524e23c412d93fb67b49958b491fce71c38`
   - `actions/cache` v4.2.2 = `d4323d4df104b026a6aa633fdb11d772146be0bf`
   - `actions/upload-artifact` v4.6.1 = `4cec3d8aa04e39d1a68397de0c4cd6fb9dce8ec1`
   - `ilammy/msvc-dev-cmd` v1.13.0 is an **annotated** tag: the tag object is `a102174a2b586eec2ea151a69e6fd14404a8ce7c`, the **commit** to pin is `0b201ec74fa43914dc39ae48a89fd1d8cb592756`. The workflow currently pins `cbc9c03b...`, which does not match any of these: replace it.

## Tasks

1. **Pins.** Show `grep -n "uses:" .github/workflows/ci.yml` before and after. Every `uses:` line is a full commit SHA from the list above with the version in a trailing comment (`# v4.2.2`). No other third-party action.
2. **Qt install.**
   - `aqt install-qt` with the arch names from the facts (`linux_gcc_64`, `win64_msvc2022_64`), pinned `aqtinstall==3.2.0`, pinned Qt `6.8.2`, no `-m` flags.
   - Do not hard-code the install subfolder. After the install, locate the prefix with a search for `lib/cmake/Qt6/Qt6Config.cmake` under the install dir (bash on Linux, PowerShell on Windows), derive `QT_PREFIX_PATH` from it (three directories up), print it, and fail with a clear message if nothing is found.
   - Keep the Qt cache but make the cache key include OS, Qt version, arch and the pinned `aqtinstall` version.
3. **Windows `PATH`.** The official Qt binaries are not found by test executables on Windows unless `<prefix>\bin` is on `PATH` (no RPATH on Windows). Append it to `$GITHUB_PATH` **after** the MSVC environment is set up, and print `where Qt6Core.dll` (or `Get-Command`) as a diagnostic step. Do not copy DLLs and do not call `windeployqt` in CI.
4. **Platform plugin for tests.** Show `grep -rn "QT_QPA_PLATFORM\|offscreen" CMakeLists.txt tests scripts cmake 2>/dev/null` and say where the offscreen platform is selected for the test binaries. If it is not guaranteed, set `QT_QPA_PLATFORM=offscreen` for the test step in the workflow (not in production code).
5. **Linux system packages.** Keep what is installed now and add `fonts-dejavu-core` (painted tests need a font). Do **not** add other packages from memory; package names differ between Ubuntu releases. If a shared library is missing, the first CI run will say so and the next fix-up will add it. Say that in the report.
6. **Windows job.** `timeout-minutes: 45` for the first run (Qt download plus MSVC build). Show how the MSVC environment reaches the build (the `ilammy/msvc-dev-cmd` step, `arch: x64`), how `check.ps1` selects the Ninja/MSVC preset, and how `QT_PREFIX_PATH` reaches CMake (`CMAKE_PREFIX_PATH` or the preset). Raw output of `cat CMakePresets.json` and `cat scripts/check.ps1`. If `check.ps1` reads Qt from a different variable than the workflow sets, fix one of them.
7. **Diagnostics for the first run.** Add a short step before the build on both OS that prints: `cmake --version`, `ninja --version`, the compiler version (`g++ --version | head -1` on Linux, `cl` on Windows via the MSVC environment), `QT_PREFIX_PATH` and a listing of `<prefix>/plugins/platforms` and `<prefix>/plugins/sqldrivers`. No secrets are printed.
8. **Static checks (run them, paste raw output).**
   - YAML parses: `python3 -c "import yaml,sys; yaml.safe_load(open('.github/workflows/ci.yml')); print('yaml ok')"` (if `yaml` is missing, say so; do not install packages).
   - `actionlint .github/workflows/ci.yml` if it is installed (`which actionlint`); if not, say "not run".
   - `git diff --stat` and `git diff .github/workflows/ci.yml` of the fix commit.
9. **Honesty.** The agent cannot run this workflow and cannot reach GitHub. In the report state: nothing here is verified by running; what each fix is based on (the user's facts above); the most likely remaining failure points for the first run (do not invent more than you can justify from the files); and that the user will push and paste the Actions log.

## Report `docs/reports/M6-ci-fixup.md` (short)
Facts used; the `uses:` before/after; the final workflow file (`cat .github/workflows/ci.yml`, nothing trimmed); outputs of task 4, 6 and 8; hashes from `git log --oneline`; "Not verified by agent, user to check": the first GitHub Actions run on both OS, the Qt cache hit on the second run, whether the test executables find the Qt DLLs on Windows, font availability for painted tests on the Linux runner.
