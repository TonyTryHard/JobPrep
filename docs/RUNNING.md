# Running JobPrep

JobPrep is an offline desktop application built with C++20 and Qt 6 Widgets for planning interview preparation and tracking job applications. All data is stored locally in an SQLite database. No network access or accounts are required.

---

## 1. System Requirements

- **C++20 Compiler**:
  - Linux: GCC 11+ or Clang 13+
  - Windows: MSVC 2022 (known failing with Qt 6.10: MSVC `19.30.30706` due to internal compiler error C1001 in `QtCore/qrangemodel_impl.h(903)`; known good in CI: MSVC `19.51.36260.0`, builds with Qt 6.8.2 in CI, with Qt 6.10.3 not yet known until the new matrix has run; user's PC: MSVC `19.44.35229` + Qt 6.10.2, `-DJOBPREP_WERROR=OFF`) or MinGW-w64 (GCC 11+)
- **CMake**: Version >= 3.21
- **Build Tool**: Ninja (recommended)
- **Qt 6**: Version **6.8 LTS** or newer (with modules `Core`, `Gui`, `Widgets`, `Sql`, `Svg`, `Test`).

### Installing Qt 6 via `aqtinstall` (Recommended)

To install the exact Qt version used by CI (Qt 6.8.2):

```bash
# Install aqtinstall via pip
python3 -m pip install aqtinstall==3.2.0

# Linux x86_64:
python3 -m aqt install-qt linux desktop 6.8.2 linux_gcc_64

# Windows x64:
python -m aqt install-qt windows desktop 6.8.2 win64_msvc2022_64
```

Set `QT_PREFIX_PATH` in your environment to point to the installed kit (e.g., `~/Qt/6.8.2/gcc_64` or `C:\Qt\6.8.2\msvc2022_64`).

---

## 2. Linux Build and Run

### Step 1: Configure

```bash
export QT_PREFIX_PATH="/path/to/Qt/6.8.x/gcc_64"  # Omit if using distro Qt 6.8+
cmake --preset dev-linux
```

### Step 2: Build

```bash
cmake --build --preset dev-linux
```

### Step 3: Run the Test Suite

```bash
ctest --preset dev-linux --output-on-failure
```

### Step 4: Launch JobPrep

```bash
./build/dev-linux/jobprep
```

Alternatively, run `./scripts/check.sh` to configure, build, and test in one command.

---

## 3. Windows Build and Run

> **Note:** Windows build verified via CI workflow (`.github/workflows/ci.yml`) offscreen; not verified on a real Windows machine by this agent.

### MSVC Toolset Compatibility with Qt 6.10

- **Known failing:** MSVC `19.30.30706` with Qt 6.10 fails with `fatal error C1001: Internal compiler error` in `QtCore/qrangemodel_impl.h(903)` (reported by user).
- **Known good:** MSVC `19.51.36260.0` in CI logs (`-- The CXX compiler identification is MSVC 19.51.36260.0`, builds with Qt 6.8.2 in CI; with Qt 6.10.3 not yet known until the new matrix has run); and MSVC `19.44.35229` on the user's PC.
- **User-reported setup:** Qt 6.10.2 from `D:\Qt_new`, configured with `cmake --preset dev-windows`, built with `-DJOBPREP_WERROR=OFF` ("warnings were off", before the M7 proxy fix), a Debug build that took less than about 10 minutes ("roughly"), app started and works.
- **Warning flag notice:** `-DJOBPREP_WERROR=OFF` was strictly a temporary workaround for the Qt 6.10 `invalidateFilter()` deprecation warning (C4996); with M7's proxy modernization (`beginFilterChange()` / `endFilterChange()`), dev builds build with zero warnings and full `/WX` without flags.

### Step 1: Open Developer Command Prompt

Open the **x64 Native Tools Command Prompt for VS 2022**.

### Step 2: Configure Environment and Build

```cmd
set QT_PREFIX_PATH=C:\Qt\6.8.2\msvc2022_64
set PATH=%QT_PREFIX_PATH%\bin;%PATH%

cmake --preset dev-windows
cmake --build --preset dev-windows
ctest --preset dev-windows --output-on-failure
```

### Step 3: Launch JobPrep

```cmd
.\build\dev-windows\jobprep.exe
```

---

## 4. Where Data Lives

JobPrep stores all user data in standard OS application data directories (`QStandardPaths::AppDataLocation` with organization `JobPrep` and application `JobPrep`):

- **SQLite Database**:
  - Linux: `${XDG_DATA_HOME:-~/.local/share}/JobPrep/JobPrep/jobprep.sqlite`
  - Windows: `%LOCALAPPDATA%\JobPrep\JobPrep\jobprep.sqlite`
- **Application Settings**:
  - Linux: `${XDG_CONFIG_HOME:-~/.config}/JobPrep/JobPrep.conf`
  - Windows: Registry `HKEY_CURRENT_USER\Software\JobPrep\JobPrep`
- **Automatic Safety Backups**:
  - Linux: `${XDG_DATA_HOME:-~/.local/share}/JobPrep/JobPrep/backups/pre-restore-<timestamp>.json`
  - Windows: `%LOCALAPPDATA%\JobPrep\JobPrep\backups\pre-restore-<timestamp>.json`

### Trying It Without Touching Your Data

To test or demo the application in complete isolation without reading or writing to your real database and configuration:

```bash
# Linux (isolated temporary HOME and XDG folders):
HOME=/tmp/jp-isolated XDG_DATA_HOME=/tmp/jp-isolated/.local/share XDG_CONFIG_HOME=/tmp/jp-isolated/.config ./build/dev-linux/jobprep

# Windows (PowerShell with isolated local app data folder):
$env:LOCALAPPDATA = "$env:TEMP\jp-isolated"; .\build\dev-windows\jobprep.exe
```

---

## 5. Backup and Restore

- **Safety Rule**: Before trying a restore on real data, copy `jobprep.sqlite` first:
  ```bash
  cp ~/.local/share/JobPrep/JobPrep/jobprep.sqlite ~/.local/share/JobPrep/JobPrep/jobprep.sqlite.manual-bak
  ```
- **Backup**: Exporting a backup creates a single portable JSON file containing all study tracks, topics, sessions, applications, interviews, and stage histories.
- **Restore**: Restoring replaces the current database with the backup archive after automatically creating a timestamped pre-restore safety copy in the `backups/` directory.

---

## 6. Linux Desktop Launcher

To add JobPrep to your desktop application menu without root permissions:

```bash
# Install launcher for the local build
./scripts/install-desktop-entry.sh

# Preview actions without writing
./scripts/install-desktop-entry.sh --dry-run

# Remove launcher
./scripts/install-desktop-entry.sh --remove
```

This writes only to `${XDG_DATA_HOME:-$HOME/.local/share}/applications/jobprep.desktop` and points directly to the compiled build-folder binary and repository icon.

---

## 7. Useful Environment Variables

- `QT_QPA_PLATFORM`: Forces a specific Qt platform backend:
  - `xcb`: X11 backend (default on X11 desktops)
  - `wayland`: Native Wayland backend
  - `offscreen`: Headless mode (used for automated tests and screenshot generation)
- `QT_SCALE_FACTOR`: Overrides UI display scaling (e.g. `QT_SCALE_FACTOR=1.5` for 150% HiDPI scaling).
- `QT_LOGGING_RULES="jobprep.window.debug=true"`: Enables diagnostic window logging on resize, state changes, DPI scaling, and screen transitions.

---

## 8. Keyboard Shortcuts

JobPrep includes comprehensive keyboard navigation across all pages and dialogs. Key shortcuts include:
- `Ctrl+1` .. `Ctrl+4`: Switch to Home (1), Study (2), Jobs (3), Calendar (4)
- `Ctrl+,`: Open Preferences / Settings
- `Ctrl+N`: New item for current page (+ Application on Jobs, + Interview on Calendar, + Topic on Study)
- `Ctrl+F`: Focus search field
- `Ctrl+S`: Save edits in detail panel or modal dialogs
- `Alt+Down` / `F4`: Open calendar popup in date edit fields
- `Esc`: Close open calendar popup, close modal dialog, dismiss details panel, or clear active search filter

See [SPEC.md §3.7](file:///home/tony/projects/JobPrep/docs/SPEC.md#37-global) for full shortcut specifications and focus handling rules.

---

## 9. Troubleshooting

- **Linux: `qt.qpa.plugin: Could not load the Qt platform plugin "xcb"`**:
  - Install the XCB platform dependencies:
    ```bash
    sudo apt-get install libxcb-cursor0 libxkbcommon-x11-0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-render-util0
    ```
- **Linux: Missing System Tray (GNOME Desktop)**:
  - JobPrep checks `QSystemTrayIcon::isSystemTrayAvailable()`. If the desktop lacks a tray area, reminders automatically fall back to styled in-app banners/toasts, and the "Close to tray" preference is cleanly disabled with an explanatory tooltip.
- **Linux: Wayland vs X11 Plugins**:
  - The desktop session on modern Linux distributions (such as Ubuntu with GNOME) often defaults to Wayland. Qt applications use the `xcb` plugin through XWayland unless the Qt Wayland module (`qt6-wayland`) is installed.
  - To test which platform plugin is available or force a specific backend:
    ```bash
    # Test if native Wayland plugin is available:
    QT_QPA_PLATFORM=wayland ./build/dev-linux/jobprep
    # (If qt6-wayland is not installed, Qt fails with: qt.qpa.plugin: Could not find the Qt platform plugin "wayland")

    # Run via X11 / XWayland (default when wayland plugin is missing):
    QT_QPA_PLATFORM=xcb ./build/dev-linux/jobprep
    ```
- **Windows: `Qt6Core.dll was not found`**:
  - Add the Qt binary directory to your `PATH`:
    ```cmd
    set PATH=%QT_PREFIX_PATH%\bin;%PATH%
    ```

---

## 10. Quality Gate and CI

- **Local Validation**: Run `./scripts/check.sh` (Linux) or `.\scripts\check.ps1` (Windows) before reporting changes.
- **Continuous Integration**: GitHub Actions workflow `.github/workflows/ci.yml` runs on every pull request and push to `main`, validating GCC on `ubuntu-latest` and MSVC on `windows-latest` under `-Werror`.
