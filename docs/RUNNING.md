# Running JobPrep

JobPrep is an offline desktop application built with C++20 and Qt 6 Widgets for planning interview preparation and tracking job applications. All data is stored locally in an SQLite database. No network access or accounts are required.

---

## 1. System Requirements

- **C++20 Compiler**:
  - Linux: GCC 11+ or Clang 13+
  - Windows: MSVC 2022 (Visual Studio 2022 17.0+) or MinGW-w64 (GCC 11+)
- **CMake**: Version >= 3.21
- **Build Tool**: Ninja (recommended)
- **Qt 6**: Version **6.8 LTS** or newer (with modules `Core`, `Gui`, `Widgets`, `Sql`, `Svg`, `Test`).

### Installing Qt 6 via `aqtinstall` (Recommended)

To install the exact Qt version used by CI (Qt 6.8.2):

```bash
# Install aqtinstall via pip
python3 -m pip install aqtinstall

# Linux x86_64:
python3 -m aqt install-qt linux desktop 6.8.2 gcc_64 -m qtsvg

# Windows x64:
python -m aqt install-qt windows desktop 6.8.2 win64_msvc2022_64 -m qtsvg
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

JobPrep stores all user data in standard OS application data directories (`QStandardPaths::AppDataLocation`):

- **SQLite Database**:
  - Linux: `${XDG_DATA_HOME:-~/.local/share}/JobPrep/jobprep.sqlite`
  - Windows: `%LOCALAPPDATA%\JobPrep\jobprep.sqlite`
- **Application Settings**:
  - Linux: `${XDG_CONFIG_HOME:-~/.config}/JobPrep/JobPrep.conf`
  - Windows: Registry `HKEY_CURRENT_USER\Software\JobPrep\JobPrep`
- **Automatic Safety Backups**:
  - Linux: `${XDG_DATA_HOME:-~/.local/share}/JobPrep/backups/pre-restore-<timestamp>.json`
  - Windows: `%LOCALAPPDATA%\JobPrep\backups\pre-restore-<timestamp>.json`

---

## 5. Backup and Restore

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

JobPrep includes comprehensive keyboard navigation across all pages and dialogs. See [SPEC.md §3.7](file:///home/tony/projects/JobPrep/docs/SPEC.md#37-global) for the complete list of shortcuts (Ctrl+1..4, Ctrl+N, Ctrl+F, Ctrl+S, Esc).

---

## 9. Troubleshooting

- **Linux: `qt.qpa.plugin: Could not load the Qt platform plugin "xcb"`**:
  - Install the XCB platform dependencies:
    ```bash
    sudo apt-get install libxcb-cursor0 libxkbcommon-x11-0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-render-util0
    ```
- **Linux: Missing System Tray (GNOME Desktop)**:
  - JobPrep checks `QSystemTrayIcon::isSystemTrayAvailable()`. If the desktop lacks a tray area, reminders automatically fall back to styled in-app banners/toasts, and the "Close to tray" preference is cleanly disabled with an explanatory tooltip.
- **Linux: Wayland vs X11 Issues**:
  - If encountering issues on a Wayland compositor, set `QT_QPA_PLATFORM=xcb` before running.
- **Windows: `Qt6Core.dll was not found`**:
  - Add the Qt binary directory to your `PATH`:
    ```cmd
    set PATH=%QT_PREFIX_PATH%\bin;%PATH%
    ```

---

## 10. Quality Gate and CI

- **Local Validation**: Run `./scripts/check.sh` (Linux) or `.\scripts\check.ps1` (Windows) before reporting changes.
- **Continuous Integration**: GitHub Actions workflow `.github/workflows/ci.yml` runs on every pull request and push to `main`, validating GCC on `ubuntu-latest` and MSVC on `windows-latest` under `-Werror`.
