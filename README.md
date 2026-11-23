JobPrep (Qt 6 Widgets desktop app)

## 1. Project
JobPrep is a small, polished, offline desktop app for ONE user to (a) plan and track interview preparation (study tracks: English, C++, extensible) and (b) track job applications, interviews and hiring status with a calendar. Local SQLite only, no network, no accounts.

## 2. Stack
- C++20, CMake >= 3.21, Ninja
- Qt 6, target 6.8 LTS or 6.10.2; (CI builds on 6.8.2 while the development machine has 6.10.2). Modules: Core, Gui, Widgets, Sql (SQLite), Svg; Qt Test for tests.
- No QML/Qt Quick, no Qt Charts, no third-party libraries. Custom visuals use QPainter.
- Platforms: Windows 10/11 x64 (MSVC 2022) and Linux x86_64 (GCC 11+ or Clang; X11 and Wayland).
- UI language: English, Ukrainian.

## 3. Build & test
No hard-coded Qt path. Use env `QT_PREFIX_PATH` (Qt kit folder, e.g. `C:\Qt\6.8.x\msvc2022_64` or `~/Qt/6.8.x/gcc_64`), an existing `build/CMakeCache.txt`. On Linux, distro Qt packages are acceptable only if they are Qt >= 6.8 (Ubuntu 22.04/24.04 ship older Qt: use the Qt online installer or aqtinstall).

`CMakePresets.json`provides `dev-windows` and `dev-linux` configure/build/test presets that read `$env{QT_PREFIX_PATH}`:
```
cmake --preset dev-linux            # or dev-windows
cmake --build --preset dev-linux
ctest --preset dev-linux --output-on-failure
```
`scripts/check.sh` and `scripts/check.ps1` run configure, build and ctest in one go and exit non-zero on any failure. Run the one for your OS before every report.
- Tests run headless: CMake sets `QT_QPA_PLATFORM=offscreen` in each test's ENVIRONMENT property.
- High warning level, zero warnings in our code (CMake option `JOBPREP_WERROR`, ON in dev).

## 4. Layout
```
CMakeLists.txt  CMakePresets.json  AGENTS.md  docs/SPEC.md  .clang-format  .gitignore  .gitattributes
resources/  resources.qrc  icons/  styles/  fonts/  seed/ styles/ translations/ windows/
src/
  main.cpp
  app/        AppContext (composition root)
  domain/     enums + plain structs
  data/       Database, migrations, repositories (only layer using QtSql)
  services/   Stats, Agenda, Reminder, Export/Import, Seed, Settings
  models/     QAbstractItemModel subclasses + proxies
  ui/         MainWindow, Sidebar, pages/, dialogs/, delegates/, widgets/, theme/
  platform/   the only place for OS-specific code (may stay empty)
tests/        Qt Test targets
scripts/      check.sh, check.ps1 check-translations.sh install-desktop-entry.sh screenshots.sh
```
