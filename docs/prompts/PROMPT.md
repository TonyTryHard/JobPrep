# PROMPT.md — Antigravity kickoff prompts for JobPrep

## 0. Workspace setup (do once)

1. Create an empty folder `JobPrep/` and put these files in it:
   - `AGENTS.md` → project root (Antigravity loads it as a workspace rule)
   - `docs/SPEC.md`
   - `docs/RUNNING.md` (human guide: install, build, run, troubleshoot)
2. Install Qt 6.8 LTS or newer (Qt online installer or `aqtinstall`) and set the environment variable `QT_PREFIX_PATH` to the kit folder:
   - Windows (PowerShell): `$env:QT_PREFIX_PATH = "C:\Qt\6.8.x\msvc2022_64"` (or your MinGW kit); needs CMake, Ninja, and MSVC 2022 or MinGW.
   - Linux (bash): `export QT_PREFIX_PATH=~/Qt/6.8.x/gcc_64`; needs `build-essential cmake ninja-build libgl1-mesa-dev libxkbcommon-x11-0 libxcb-cursor0` (Debian/Ubuntu names). Distro Qt works only if it is Qt >= 6.5.
3. Open the folder in Antigravity. Use **Planning** mode with a conservative review policy for M0, then relax it once you trust the output. Terminal users: `cd JobPrep && agy --mode plan` (see `docs/RUNNING.md` A5b; `/agents` opens the agent panel).
4. Run `git init` before the first prompt so every milestone is a reviewable commit.
5. Paste the **Master prompt** below as the first task. Afterwards use the **Milestone prompts** one by one, only after you have reviewed and accepted the previous milestone.

---

## 1. Master prompt (first task)

```text
You are the lead developer of "JobPrep", a small, polished, offline desktop
application built with C++20, Qt 6 Widgets, CMake and SQLite.

Context
- Read `AGENTS.md` (rules, architecture, conventions, definition of done) and
  `docs/SPEC.md` (product spec, schema, UI, milestones) completely before doing anything.
- The app helps one user track interview preparation (English, C++ study tracks)
  and job applications (company, status, interviews, calendar, funnel stats).

Your task now: Milestone M0 (Scaffold & shell) from docs/SPEC.md §12.

Step 1 — Plan (stop and wait for my approval)
- Produce an implementation plan as an artifact: file list with one-line
  purposes, CMake structure (targets: jobprep_core static lib + jobprep app +
  tests), class sketch for MainWindow/Sidebar/PageBase/ThemeManager/
  SettingsService, how themes (QSS templates + QPalette + tokens) will work,
  and any risks or assumptions.
- Check that Qt is discoverable (QT_PREFIX_PATH) and report the OS, exact
  compiler and generator you will use. The app must target Windows AND Linux
  (see AGENTS.md §12); say how you will keep the other platform from breaking.

Step 2 — Implement (after I approve the plan)
- Implement exactly M0, nothing from later milestones. Placeholder pages show
  a title and an EmptyState-style label only.
- The UI must already look good: sidebar with accent pill for the active page,
  header with page title, status bar, light/dark/accent switching at runtime,
  bundled SVG icons tinted by theme, 8 px spacing grid.
- Add the `ui_smoke` test (offscreen: create MainWindow, visit every page,
  toggle both themes, fail on any qWarning/qCritical), `scripts/check.sh` and
  `scripts/check.ps1` (configure + build + ctest), `.clang-format`,
  `.gitignore`, `.gitattributes`, `resources.qrc`, and `CMakePresets.json` with
  `dev-windows` and `dev-linux` presets that read `$env{QT_PREFIX_PATH}`.
  Create the empty `src/platform/` folder.

Step 3 — Verify and report
- Run the scripts/check script (zero warnings, all tests green), launch the
  app, and exercise the M0
  acceptance criteria.
- Finish with the report format from AGENTS.md §10 and a screenshot of both
  themes if your environment can capture one.
- Commit with message `feat(app): scaffold JobPrep shell (M0)`.
- Do not start M1.
```

---

## 2. Milestone prompts (use one at a time)

Ready-made copies live in `docs/prompts/M1.md` … `M6.md` (they already say "read AGENTS.md and SPEC.md first"). In `agy`: start a fresh session and send `Execute the task in @docs/prompts/M1.md`.

Each prompt below assumes `AGENTS.md` and `docs/SPEC.md` are already in context. Every milestone must also extend `ui_smoke` for the UI it adds and finish with a passing `scripts/check`.

### M1 — Data layer
```text
Implement Milestone M1 (Data layer) from docs/SPEC.md §12.
Start with a short plan listing the repository interfaces (method names and
signatures) and the migration runner design; continue without waiting unless
you find a spec conflict. Follow the schema in §5 exactly. Write Qt Test
coverage first for migrations and repositories using an in-memory database.
Wire `AppContext` into main.cpp and add the first-launch "Load sample data?"
prompt using SeedService and resources/seed/topics.json (§9).
Report using AGENTS.md §10 and commit as `feat(data): sqlite layer and seed (M1)`.
```

### M2 — Jobs page
```text
Implement Milestone M2 (Jobs page) from docs/SPEC.md §12 and §3.3, §7.4.
Deliver a thin vertical slice first (table + add dialog + persistence), then
expand to filters, context menu, Timeline tab, duplicate, CSV export.
Use StatusStyle for all status colors and implement StatusBadgeDelegate as a
rounded pill. Include the empty state. Verify in light and dark themes and at
150% scaling. Report using AGENTS.md §10; commit as `feat(jobs): applications page (M2)`.
```

### M3 — Calendar & interviews
```text
Implement Milestone M3 (Calendar & interviews) from docs/SPEC.md §12, §3.4, §7.5.
Build `AgendaService` with tests first (month borders, follow-ups, topic target
dates), then the custom-painted `MonthView` (no QCalendarWidget), agenda panel,
layer chips, and `InterviewDialog`. Complete the Interviews tab in
ApplicationDialog. The UI may only read items through AgendaService.
Report using AGENTS.md §10; commit as `feat(calendar): month view and interviews (M3)`.
```

### M4 — Study page
```text
Implement Milestone M4 (Study page) from docs/SPEC.md §12, §3.2, §7.3.
Order: TopicListModel + TopicBoardProxy with tests -> board with drag & drop
(custom MIME type) -> detail panel with checklist and resources -> session
logging dialog -> track management -> list view toggle.
Cards use TopicCardDelegate (track stripe, progress bar, due chip, priority).
Report using AGENTS.md §10; commit as `feat(study): topic board (M4)`.
```

### M5 — Dashboard & reminders
```text
Implement Milestone M5 (Dashboard & reminders) from docs/SPEC.md §12, §3.1, §3.5, §6.
Write StatsService tests with an injectable "today" before any UI. Then
StatCard, ProgressRing, FunnelWidget (all QPainter, HiDPI-correct), Today and
Upcoming lists, status bar summary. Then ReminderService with INotifier and a
TrayNotifier plus an InAppNotifier fallback that is chosen automatically when
no system tray is available; unit-test the due-reminder computation without a
real tray. The weekly study goal comes from SettingsService (default 360 min,
that is 6 h) and is passed into StatsService as a parameter.
Report using AGENTS.md §10; commit as `feat(dashboard): stats and reminders (M5)`.
```

### M6 — Polish & data tools
```text
Implement Milestone M6 (Polish & data tools) from docs/SPEC.md §12, §3.6, §3.7, §10.
Do the Settings page (including the weekly study goal: hours, 15-minute steps,
default 6 h, live update of the dashboard), JSON backup/restore (transactional,
validated), ICS export (CRLF), keyboard shortcuts via QKeySequence::StandardKey,
tab-order pass, empty states, app icon, and verify docs/RUNNING.md step by step (fix anything
inaccurate). No installers or AppImage. Disable "Close to tray" when no tray exists. Run a visual QA
pass in both themes at 100% and 150% scaling and fix clipping or alignment
issues you find.
Report using AGENTS.md §10; commit as `feat(app): polish, backup, ics (M6)`.
```

---

## 3. Handy follow-up prompts

**Review before accepting a milestone**
```text
Act as a strict reviewer of the last milestone. Check it against AGENTS.md
(layering, ownership, conventions, definition of done) and the acceptance
criteria in docs/SPEC.md. List violations with file:line, severity, and a
proposed fix. Do not change code yet.
```

**Cross-platform check**
```text
Review the whole codebase for Windows/Linux portability: Q_OS_* usage outside
src/platform/, hard-coded path separators, case-mismatched includes or
resource names, missing tray fallback, line-ending assumptions, compiler-
specific flags outside `if(MSVC)`. List findings with file:line and fix
the ones that are clearly wrong. Report what you could not verify on the
OS you are running.
```

**Fix review findings**
```text
Fix the findings you listed, highest severity first. Keep changes minimal,
re-run build and tests, and update the report.
```

**Visual polish pass**
```text
Do a visual polish pass on <page>. Compare against the design tokens and
spacing rules in docs/SPEC.md §7.6 and AGENTS.md §7. Fix alignment, spacing,
contrast, hover/focus states, and empty states in both themes. No new features.
```

**Bug fix template**
```text
Bug: <what happens>. Expected: <what should happen>. Steps: <1,2,3>.
First write a failing test (or a reproducible manual script if UI-only),
then fix the root cause, then re-run everything. Mention any related risk.
```
