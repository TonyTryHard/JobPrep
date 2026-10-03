# AGENTS.md — JobPrep (Qt 6 Widgets desktop app)

Read this file fully before any task. Detailed requirements live in `docs/SPEC.md`; read the sections relevant to your milestone before planning.

## 1. Project
JobPrep is a small, polished, offline desktop app for ONE user to (a) plan and track interview preparation (study tracks: English, C++, extensible) and (b) track job applications, interviews and hiring status with a calendar. Local SQLite only, no network, no accounts.

## 2. Stack (fixed — do not change without asking)
- C++20, CMake >= 3.21, Ninja (or the generator already used in `build/`).
- Qt 6, target 6.8 LTS or newer; avoid APIs newer than 6.5 unless version-guarded. Modules: Core, Gui, Widgets, Sql (SQLite), Svg; Qt Test for tests.
- No QML/Qt Quick, no Qt Charts, no third-party libraries. Custom visuals use QPainter.
- Platforms: Windows 10/11 x64 (MSVC 2022 or MinGW) and Linux x86_64 (GCC 11+ or Clang; X11 and Wayland). Both are first-class; follow §12.
- UI language: English.

## 3. Build & test
Never hard-code the Qt path. Use env `QT_PREFIX_PATH` (Qt kit folder, e.g. `C:\Qt\6.8.x\msvc2022_64` or `~/Qt/6.8.x/gcc_64`), an existing `build/CMakeCache.txt`, or ask the user. On Linux, distro Qt packages are acceptable only if they are Qt >= 6.5 (Ubuntu 22.04/24.04 ship older Qt: use the Qt online installer or aqtinstall).

`CMakePresets.json` (created in M0) provides `dev-windows` and `dev-linux` configure/build/test presets that read `$env{QT_PREFIX_PATH}`:
```
cmake --preset dev-linux            # or dev-windows
cmake --build --preset dev-linux
ctest --preset dev-linux --output-on-failure
```
`scripts/check.sh` and `scripts/check.ps1` (created in M0) run configure, build and ctest in one go and exit non-zero on any failure. Run the one for your OS before every report.
- Tests run headless: CMake sets `QT_QPA_PLATFORM=offscreen` in each test's ENVIRONMENT property.
- High warning level, zero warnings in our code (CMake option `JOBPREP_WERROR`, ON in dev). Fix warnings, never disable them.
- If you cannot build or run (Qt missing, etc.) say so explicitly. Never claim something works without building and running it.

## 4. Layout
```
CMakeLists.txt  CMakePresets.json  AGENTS.md  docs/SPEC.md  .clang-format  .gitignore  .gitattributes
resources/  resources.qrc  icons/  styles/  fonts/  seed/
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
scripts/      check.sh, check.ps1 (configure + build + ctest in one command)
```

## 5. Architecture rules
Dependency direction is strict, never upward: `ui -> models -> services -> data -> domain`.
- domain: value types only (structs, `enum class`). No widgets, no SQL.
- data: the only layer that touches QtSql. Repositories return domain types and emit `changed()` after writes. All SQL parametrized (`prepare` + `bindValue`). Schema changes = a new numbered migration applied through `PRAGMA user_version`. Never edit a shipped migration.
- services: business logic (stats, agenda aggregation, reminders, export/import, seeding). System tray access goes through an injected `INotifier` so tests can fake it.
- models: Qt item models and proxies. They read from repositories and reload on `changed()`.
- ui: widgets, dialogs, delegates, theme. No SQL and no business rules in widgets.
- `main.cpp` builds `AppContext` (db, repositories, services) and passes it by reference via constructors. No singletons, no globals, no mutable statics.
- Ownership: Qt parent-child for QObjects/widgets, `std::unique_ptr` otherwise. No owning raw pointers; no `new` without a parent or smart pointer.
- Signals/slots use pointer-to-member syntax only (no `SIGNAL()`/`SLOT()`).
- Work longer than ~100 ms (import/export) must not run on the GUI thread (QtConcurrent or a worker QObject).

## 6. Coding conventions
- Classes PascalCase; methods and variables camelCase; members `m_name`; constants `kName`; `enum class`; one class per file named after it.
- `#pragma once`; forward-declare in headers; include what you use; no `using namespace` in headers.
- Use `QString`, `QDate`, `QDateTime`, `QList` (not `QVector`), `std::optional` for "maybe".
- String literals: `u"..."_s` with `using namespace Qt::StringLiterals` in .cpp files; user-visible text through `tr()`.
- Qt 6 APIs only: `QRegularExpression`, `QStringView`, `Qt::SkipEmptyParts`, `std::as_const`. No `foreach`/`Q_FOREACH`, no `QRegExp`.
- Functions short (aim < 50 lines). Comments explain "why". Each public class gets a one-line doc comment.
- Format with the repo `.clang-format` (4 spaces, 100 cols); create it in M0.

## 7. UI and design rules
Goal: small but pretty — calm, modern, consistent.
- Never hard-code colors, fonts or sizes in widgets. Colors come from tokens in `ui/theme/Tokens` and QSS templates in `resources/styles/`. `ThemeManager` switches light/dark and accent at runtime without restart.
- 8 px spacing grid; card radius 12 px, control radius 8 px; base font 10 pt (bundled Inter if available, else system UI font).
- Icons: monochrome SVG in `resources/icons/`, tinted by theme at load. No emoji as icons.
- Status colors and labels live in one place (`ui/theme/StatusStyle`). Same status = same color everywhere.
- Every list/table has an empty state (icon + one-line hint + primary action).
- Keyboard accessible, sensible tab order, tooltips on icon-only buttons. Minimum window 1100x700; layouts resize cleanly (no fixed sizes except icons).
- Custom painting respects `devicePixelRatio` and avoids per-paint allocations.
- Avoid `QGraphicsDropShadowEffect` on many/large widgets; use border and subtle surface contrast instead.

## 8. Data rules
- DB file: `QStandardPaths::AppDataLocation/jobprep.sqlite`; tests use `:memory:`.
- Dates: ISO-8601 text. Date-only `yyyy-MM-dd`; date-time is local time, no offset, `yyyy-MM-ddTHH:mm:ss`.
- Enums stored as lowercase snake_case text, mapped in one place (`domain/EnumStrings`).
- `PRAGMA foreign_keys=ON`, WAL mode.
- Never lose user data: destructive actions (delete, import-replace) need confirmation; import validates before touching the DB.

## 9. Definition of done (per milestone)
1. Builds with zero warnings on the OS you are running (state OS and compiler in the report). Never write code you know will not build on the other platform (§12).
2. `scripts/check` passes. New data/services logic has Qt Test coverage (repositories on in-memory DB, StatsService, AgendaService, migrations from empty DB and from the previous version). The `ui_smoke` test (offscreen) creates `MainWindow`, visits every page, toggles both themes, opens every dialog added so far, and fails on any unexpected `qWarning`/`qCritical`; extend it in every milestone that adds UI.
3. App launches; every acceptance criterion of the milestone in `docs/SPEC.md` §12 was walked through and is listed in the report.
4. New UI checked in both light and dark themes.
5. No unreported TODO/FIXME, dead code or commented-out blocks.
6. Any deviation from the spec (behavior, schema) is added to the changelog at the end of `docs/SPEC.md`.

## 10. How to work
- First milestone: write a short plan (files to add/change, risks) and wait for approval. Afterwards work one milestone at a time; do not start the next one on your own.
- Prefer a thin vertical slice (UI + model + repository + test) over many half-finished layers.
- Small, reviewable commits in conventional style, e.g. `feat(apps): add status badge delegate`.
- Do not add dependencies, change the stack, rename top-level folders, or change the schema outside a migration without asking.
- If the spec is ambiguous, choose the simplest option consistent with this file, state the assumption in the report, and continue. Ask only when blocked.
- Final report format: What changed / How to run / How verified (commands + results) / Deviations and assumptions / Suggested next step.

## 11. Never
- No network access at runtime, no telemetry, no secrets in the repo.
- Never commit build output, `*.user` files or IDE folders; keep `.gitignore` current.
- No blocking the GUI thread with `sleep` or `processEvents()` loops.
- No deprecated Qt5-era APIs.

## 12. Cross-platform rules (Windows + Linux)
- OS-specific code (`Q_OS_WIN`, `Q_OS_LINUX`, native headers) is allowed only in `src/platform/` behind a small interface. Everything else uses Qt abstractions.
- Paths: `QDir`, `QFileInfo`, `QStandardPaths`. No hard-coded separators or drive letters; `QDir::toNativeSeparators` only for display. Set `QCoreApplication` organization and application name early (needed for correct data/config folders).
- Linux file names and `#include` paths are case-sensitive: match file names exactly.
- Open links and folders with `QDesktopServices::openUrl`. Do not force a Qt platform plugin; X11 and Wayland must both work. Tests use `offscreen`.
- The system tray may be missing (e.g. GNOME). Check `QSystemTrayIcon::isSystemTrayAvailable()`; fall back to an in-app notifier (toast/banner) and disable "Close to tray". A reminder must never be lost because the tray is absent.
- Fonts: bundle the UI font (Inter, OFL license text included) so both OSes look identical; if the files are unavailable, use the system UI font and say so in the report. Never invent font files.
- Shortcuts: use `QKeySequence::StandardKey` where one exists (Save, Find, New, Preferences), otherwise Ctrl-based sequences.
- Text: UTF-8 everywhere. ICS output must use CRLF line endings; write them explicitly. `.gitattributes`: `* text=auto`, LF for shell scripts.
- Compiler-specific flags only inside CMake `if(MSVC) ... else() ... endif()`. No compiler extensions.
- No installers and no AppImage. The app runs from the build folder on both OSes; `docs/RUNNING.md` is the human guide and must stay accurate (verified in M6).
