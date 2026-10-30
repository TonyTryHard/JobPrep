# JobPrep — Product & Architecture Spec

Version 1.0 · Qt 6 Widgets · C++20 · SQLite. Rules for how to code are in `AGENTS.md`; this file says WHAT to build.

## 1. Purpose and scope

**Goal.** One desktop app that answers three questions at a glance: *What should I study today? Where is each job application? What is coming up in the calendar?*

**In scope (v1).** Study tracks with topics/checklists/time log; job application table with status pipeline; interviews and follow-ups in a calendar; dashboard with stats; reminders; light/dark themes; CSV/JSON/ICS export and JSON restore.

**Out of scope (v1).** Cloud sync, accounts, multi-user, web scraping of job boards, e-mail integration, AI features, mobile, localization.

**Primary user.** A single developer preparing for C++ jobs, improving English, sending resumes and tracking responses.

**Platforms.** Windows 10/11 x64 and Linux x86_64 (X11 and Wayland) from one code base; both are first-class.

## 2. Key scenarios

1. Open the app in the morning: see today's agenda (interview at 14:00, follow-up to Acme, study topic due) and the study streak.
2. Add a new application in under 20 seconds (company + position + date are enough).
3. Move an application from *Applied* to *HR screen*; schedule the interview; get a reminder 24 h and 1 h before.
4. Study "Move semantics": tick checklist items, log a 45-minute session, watch track progress grow.
5. See in the calendar which days are packed with interviews and which have study targets.
6. At week's end review: applications sent, response/interview rate, hours studied.

## 3. Functional requirements

### 3.1 Dashboard (page "Home")
- **D1** Four stat cards: *Applied this week*, *Active processes*, *Interviews in next 7 days*, *Study streak (days)*.
- **D2** "Today" list: interviews today (with time), follow-ups due today/overdue, topics whose target date is today/overdue. Click opens the item.
- **D3** "Study progress": one `ProgressRing` per track (percentage) and minutes studied this week vs the weekly goal (default 6 h = 360 min; user-adjustable, see §3.6).
- **D4** "Hiring funnel": Applied → HR screen → Technical → Final → Offer with counts (custom painted `FunnelWidget`) and the *interview rate* (see §6).
- **D5** "Upcoming" list: next 5 interviews.
- **D6** Empty state when there is no data, with buttons "Add application" and "Add topic".

### 3.2 Study (page "Study")
- **S1** Tracks: `English` and `C++` are seeded; the user can add/rename/recolor/delete tracks (delete asks confirmation, cascades topics).
- **S2** Topic fields: title, track, status (Backlog / In progress / Review / Done), priority (Low/Normal/High), target date (optional), notes (plain text/Markdown, edited as text), resources (list of title+URL), tags (comma separated).
- **S3** Checklist (subtasks) per topic: add, edit, tick, reorder (drag), delete.
- **S4** Study sessions: "Log time" button on a topic opens a small dialog (date default today, minutes, note). Sessions can also be logged without a topic.
- **S5** Board view: 4 columns by status; cards show title, track color stripe, progress bar, target date chip (red when overdue), priority marker. Drag & drop between columns changes the status. A filter chip bar selects track (All / each track).
- **S6** List view (toggle): table-like list grouped by track, sortable by status/priority/target date.
- **S7** Detail panel on the right (collapsible) edits the selected topic inline; auto-saves on focus loss / Ctrl+S.
- **S8** Quick add: Enter in the "New topic…" line edit at the top of a column creates a topic in that status and track.
- **S9** Delete topic with confirmation. Sessions of a deleted topic stay (topic_id becomes NULL).

### 3.3 Applications (page "Jobs")
- **A1** Table columns: Company, Position, Status (badge), Applied, Next step (next interview or next action + date), Salary, Source, Updated. Sortable; default sort by Updated desc.
- **A2** Search box (company/position/notes) and status chip filter: All / Active / Interviewing / Offer / Closed.
- **A3** Add/Edit via `ApplicationDialog` with tabs: *Overview* (all fields), *Interviews* (list + add/edit/delete), *Timeline* (read-only status history), *Notes*.
- **A4** Fields: company*, position*, URL, source (editable combo: LinkedIn, DOU, Djinni, Referral, Company site, Other), resume version, location, work mode (Remote/Hybrid/Onsite), salary min/max + currency, status, applied date, next action text + date, contact name/e-mail, notes.
- **A5** Changing status (dialog, context menu, or keyboard shortcut) writes a `status_history` row. Setting status to *Applied* with an empty applied date sets it to today.
- **A6** Context menu on a row: Edit, Change status ▸, Add interview…, Open posting URL, Duplicate, Delete (confirm).
- **A7** Double-click opens the dialog. Footer shows counts per status.
- **A8** Export visible rows to CSV from the toolbar.

### 3.4 Calendar (page "Calendar")
- **C1** Custom-painted `MonthView` (Monday-first by default, setting to switch to Sunday): month title, prev/next/today buttons, 6×7 grid, today highlighted, selected day outlined, out-of-month days dimmed.
- **C2** Each day cell shows up to 3 colored dots/pills for items and "+N" overflow. Colors: interview = accent/orange, follow-up = blue, study target = green.
- **C3** Layer toggles (chips): Interviews, Follow-ups, Study targets.
- **C4** Right-hand agenda panel lists items of the selected day sorted by time with kind icon, title, subtitle (company/position, place), and quick actions (open, edit). Buttons: "+ Interview", "+ Follow-up".
- **C5** Double-click a day opens "New interview" prefilled with that date (asks which application via combo).
- **C6** Items come exclusively from `AgendaService` (no ad-hoc queries in the UI).

### 3.5 Reminders
- **R1** `ReminderService` checks every 60 s. Interview reminders at configurable lead times (default 1440 and 60 minutes). Follow-up reminders at 09:00 on `next_action_date`. Overdue follow-ups are mentioned once on startup.
- **R2** Each reminder fires once (persisted in `reminders_sent`).
- **R3** Notification goes through `INotifier`. `TrayNotifier` uses `QSystemTrayIcon::showMessage`; when no system tray is available (common on GNOME/Linux) `InAppNotifier` shows a toast/banner inside the window instead. The choice is automatic at startup. Clicking a notification raises the window and opens the item.
- **R4** Setting "Close to tray" (default off). Disabled with an explanatory tooltip when no system tray is available.

### 3.6 Settings
- Theme (System / Light / Dark), accent color (6 presets), first day of week (default Monday), reminder lead times, close-to-tray.
- **Weekly study goal**: default **6 h (360 min)**, editable in hours with 15-minute steps (range 1–40 h). Stored in `SettingsService` under `study/weeklyGoalMinutes`. Changing it updates the dashboard bar immediately and applies to the current week (history is not rewritten).
- Data: *Export CSV*, *Export ICS*, *Backup JSON*, *Restore JSON* (replace, with confirmation), *Load sample data* (only when DB is empty or on explicit confirm), *Open data folder*.

### 3.7 Global
- Shortcuts (use `QKeySequence::StandardKey` where one exists): Ctrl+1…4 switch pages, New (Ctrl+N) new item for the current page, Find (Ctrl+F) focus search, Preferences (Ctrl+,) settings, Save (Ctrl+S) in detail panels, Esc closes dialogs/panels.
- Window geometry and splitter states are remembered via `QSettings`.
- First launch: offer "Load sample data?" (seed topics only, never fake applications).

## 4. Domain model

Enums (`enum class`, stored as snake_case text):

| Enum | Values |
|---|---|
| `TopicStatus` | backlog, in_progress, review, done |
| `Priority` | low, normal, high |
| `ApplicationStatus` | wishlist, applied, hr_screen, technical, final, offer, accepted, rejected, withdrawn, ghosted |
| `WorkMode` | remote, hybrid, onsite |
| `InterviewType` | hr, technical, live_coding, system_design, behavioral, final, other |
| `InterviewOutcome` | pending, passed, failed, cancelled |
| `AgendaKind` | interview, follow_up, study_target |

Structs: `Track`, `Topic`, `Subtask`, `StudySession`, `JobApplication` (named to avoid clashing with `QApplication`), `StatusChange`, `Interview`, `AgendaItem{kind, date, time(optional), title, subtitle, refId, color}`.

Derived rules:
- **Active application**: status in {applied, hr_screen, technical, final, offer}. **Closed**: rejected, withdrawn, ghosted, accepted. *Wishlist* is neither.
- **Topic progress**: if it has subtasks → done/total; otherwise by status: backlog 0, in_progress 50, review 80, done 100. Moving a topic to *Done* does not tick subtasks automatically.
- **Track progress**: average progress of its topics (0 if none).

## 5. SQLite schema (migration 001)

```sql
CREATE TABLE tracks (
  id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE,
  color TEXT NOT NULL, icon TEXT NOT NULL DEFAULT '', position INTEGER NOT NULL DEFAULT 0);

CREATE TABLE topics (
  id INTEGER PRIMARY KEY,
  track_id INTEGER NOT NULL REFERENCES tracks(id) ON DELETE CASCADE,
  title TEXT NOT NULL, status TEXT NOT NULL DEFAULT 'backlog',
  priority TEXT NOT NULL DEFAULT 'normal', target_date TEXT,
  notes TEXT NOT NULL DEFAULT '', resources TEXT NOT NULL DEFAULT '[]', -- JSON [{title,url}]
  tags TEXT NOT NULL DEFAULT '', position INTEGER NOT NULL DEFAULT 0,
  created_at TEXT NOT NULL, updated_at TEXT NOT NULL);

CREATE TABLE subtasks (
  id INTEGER PRIMARY KEY,
  topic_id INTEGER NOT NULL REFERENCES topics(id) ON DELETE CASCADE,
  text TEXT NOT NULL, done INTEGER NOT NULL DEFAULT 0, position INTEGER NOT NULL DEFAULT 0);

CREATE TABLE study_sessions (
  id INTEGER PRIMARY KEY,
  topic_id INTEGER REFERENCES topics(id) ON DELETE SET NULL,
  session_date TEXT NOT NULL, minutes INTEGER NOT NULL CHECK (minutes > 0),
  note TEXT NOT NULL DEFAULT '');

CREATE TABLE applications (
  id INTEGER PRIMARY KEY,
  company TEXT NOT NULL, position TEXT NOT NULL,
  url TEXT NOT NULL DEFAULT '', source TEXT NOT NULL DEFAULT '',
  resume_version TEXT NOT NULL DEFAULT '', location TEXT NOT NULL DEFAULT '',
  work_mode TEXT NOT NULL DEFAULT 'remote',
  salary_min INTEGER, salary_max INTEGER, currency TEXT NOT NULL DEFAULT 'USD',
  status TEXT NOT NULL DEFAULT 'applied', applied_date TEXT,
  next_action TEXT NOT NULL DEFAULT '', next_action_date TEXT,
  contact_name TEXT NOT NULL DEFAULT '', contact_email TEXT NOT NULL DEFAULT '',
  notes TEXT NOT NULL DEFAULT '',
  created_at TEXT NOT NULL, updated_at TEXT NOT NULL);
CREATE INDEX idx_applications_status ON applications(status);

CREATE TABLE status_history (
  id INTEGER PRIMARY KEY,
  application_id INTEGER NOT NULL REFERENCES applications(id) ON DELETE CASCADE,
  from_status TEXT, to_status TEXT NOT NULL,
  changed_at TEXT NOT NULL, note TEXT NOT NULL DEFAULT '');

CREATE TABLE interviews (
  id INTEGER PRIMARY KEY,
  application_id INTEGER NOT NULL REFERENCES applications(id) ON DELETE CASCADE,
  start_at TEXT NOT NULL, duration_min INTEGER NOT NULL DEFAULT 60,
  type TEXT NOT NULL DEFAULT 'technical', place TEXT NOT NULL DEFAULT '',
  interviewer TEXT NOT NULL DEFAULT '', notes TEXT NOT NULL DEFAULT '',
  outcome TEXT NOT NULL DEFAULT 'pending');
CREATE INDEX idx_interviews_start ON interviews(start_at);

CREATE TABLE reminders_sent (key TEXT PRIMARY KEY, sent_at TEXT NOT NULL);
```

`key` format: `interview:<id>:<leadMinutes>`, `followup:<appId>:<yyyy-MM-dd>`, `overdue:<appId>:<yyyy-MM-dd>`.

## 6. Computed values (StatsService)

- **Week** always means the current week starting on the configured first day of week (default Monday).
- **Stage rank**: wishlist 0, applied 1, hr_screen 2, technical 3, final 4, offer 5, accepted 5. An application "reached" stage N if its current status or any `status_history.to_status` has rank ≥ N (rejected/withdrawn/ghosted have no rank; use history).
- **Funnel counts**: number of applications that reached each of ranks 1–5.
- **Interview rate**: reached(rank ≥ 2) / reached(rank ≥ 1); show "–" when denominator is 0.
- **Study streak**: consecutive days with ≥ 1 study session, counted back from today (if today has none yet, start from yesterday so the streak is not shown broken during the morning).
- **Applied this week**: applications with `applied_date` in the current week.
- **Week minutes**: sum of `minutes` for sessions in the current week. **Goal progress** = week minutes / `study/weeklyGoalMinutes` (default 360), capped at 100% for the bar but the real value is shown in text (e.g. "7h 10m / 6h"). `StatsService` receives the goal as a parameter, never reads settings itself.

## 7. UI design

### 7.1 Shell
```
+----------+---------------------------------------------------------------+
| (logo)   |  Page title                          [ Search ] [ + New v ]   |
|          +---------------------------------------------------------------+
| Home     |                                                               |
| Study    |                 QStackedWidget (current page)                 |
| Jobs     |                                                               |
| Calendar |                                                               |
|          |                                                               |
| Settings |                                                               |
+----------+---------------------------------------------------------------+
| status bar:  6-day streak  |  3 active processes  |  Next: Fri 14:00 Globex |
+--------------------------------------------------------------------------+
```
- Left `Sidebar`: 72 px collapsed width (icons) / 208 px expanded (icon + label), checkable buttons, active item has accent pill. Default expanded; toggle with a chevron.
- Header: page title, global search (jumps to Jobs/Study filter), "+ New" menu (Application, Topic, Interview, Study session).
- Pages derive from `PageBase` with `onActivated()` to refresh cheap derived data.

### 7.2 Dashboard
```
[Applied this week 5] [Active 3] [Interviews 7d: 2] [Streak 6 d]
+---------------------------+  +------------------------------------+
| Today                     |  | Study progress                     |
|  09:00 Follow up: Acme    |  |  (ring C++ 42%)  (ring English 65%)|
|  14:00 Interview: Globex  |  |  This week 4h 20m / 6h  [bar]      |
|  Topic due: Move semantics|  +------------------------------------+
+---------------------------+  | Hiring funnel                      |
| Upcoming interviews       |  |  Applied 18 > Screen 7 > Tech 3 ...|
+---------------------------+  +------------------------------------+
```

### 7.3 Study (board)
```
[All] [C++] [English] [+ track]              [Board | List]  [+ Topic]
+-- Backlog ----+-- In progress --+-- Review ------+-- Done --------+ | Detail   |
| ▌RAII         | ▌Move semantics | ▌STAR stories  | ▌Self intro    | | title    |
|  ███░░ 40%    |  ██░░░ 35%      |  ████░ 80%     |  █████ 100%    | | status.. |
|  [+ New topic]|                 |                |                | | notes    |
+---------------+-----------------+----------------+----------------+ | checklist|
                                                                      | Log time |
```

### 7.4 Jobs
```
[ Search…        ] [All][Active][Interviewing][Offer][Closed]      [Export] [+ Application]
+---------+------------+-----------+---------+-----------------------+---------+
| Company | Position   | Status    | Applied | Next step             | Salary  |
+---------+------------+-----------+---------+-----------------------+---------+
| Globex  | C++ Dev    | (Technical)| 12 Sep | Interview Fri 14:00   | 4–5k USD|
| Acme    | Qt Engineer| (Applied) | 28 Sep  | Follow up 05 Oct      |         |
+---------+------------+-----------+---------+-----------------------+---------+
```
Rows are 44 px high, zebra off, hover highlight, status shown by `StatusBadgeDelegate` (rounded pill with status color at ~15% background and full-color text).

### 7.5 Calendar
```
< October 2026 >  [Today]   (Interviews)(Follow-ups)(Study targets)  |  Fri, 2 Oct
 Mo  Tu  We  Th  Fr  Sa  Su                                         |  14:00 Technical · Globex
 28  29  30   1   2   3   4                                         |         Zoom · Anna K.
                 ••                                                 |  Follow-up · Acme
  5   6  ...                                                        |  [+ Interview] [+ Follow-up]
```

### 7.6 Design tokens

| Token | Light | Dark |
|---|---|---|
| `bg` | #F6F7FB | #0F1117 |
| `surface` | #FFFFFF | #171A23 |
| `surfaceAlt` | #EEF0F6 | #1E2230 |
| `border` | #E3E6EF | #2A2F3F |
| `text` | #1F2430 | #E8EAF2 |
| `textMuted` | #6B7385 | #9AA3B8 |
| `accent` (default indigo) | #5B6CFF | #7C8AFF |
| `danger` / `warning` / `success` | #EF4444 / #F59E0B / #10B981 | same, slightly lighter |

Accent presets: indigo #5B6CFF, teal #14B8A6, rose #F43F5E, amber #F59E0B, emerald #10B981, sky #0EA5E9.

Status colors (same in both themes): wishlist #8A93A6 · applied #3B82F6 · hr_screen #8B5CF6 · technical #F59E0B · final #F97316 · offer #10B981 · accepted #059669 · rejected #EF4444 · withdrawn #64748B · ghosted #94A3B8.

Seeded track colors: C++ #5B6CFF, English #F59E0B.

Typography: bundle Inter (OFL) in `resources/fonts/` together with its license text so Windows and Linux look identical; if the files are not available, fall back to the system UI font. Sizes: page title 18 pt semibold, section title 12 pt semibold, body 10 pt, caption 9 pt.

QSS templates use placeholders like `@bg`, `@surface`, `@accent` replaced by `ThemeManager` at load; the QPalette is also set so native-painted parts follow the theme. Base style: `Fusion`.

## 8. Architecture

```
            +---------------------------- ui ---------------------------+
            | MainWindow  Sidebar  pages/  dialogs/  delegates/  widgets/ |
            +-------------------------------+---------------------------+
                                            | uses
                         +------------------v------------------+
                         | models  (Qt item models, proxies)   |
                         +------------------+------------------+
                                            |
                         +------------------v------------------+
                         | services (Stats, Agenda, Reminder,  |
                         |  Export/Import, Seed, Settings)     |
                         +------------------+------------------+
                                            |
                         +------------------v------------------+
                         | data (Database, Migrations, Repos)  |
                         +------------------+------------------+
                                            |
                         +------------------v------------------+
                         | domain (enums, structs)             |
                         +-------------------------------------+
```

**Composition root.** `main.cpp` → `QApplication` → `ThemeManager` → `AppContext` (opens DB, runs migrations, creates repositories and services) → `MainWindow(AppContext&)`.

**Key classes**

| Layer | Class | Responsibility |
|---|---|---|
| data | `Database` | open/close connection, pragmas, migrations, `transaction()` RAII helper |
| data | `TrackRepository`, `TopicRepository` (topics + subtasks), `SessionRepository`, `ApplicationRepository` (applications + status_history), `InterviewRepository`, `ReminderLogRepository` | CRUD, queries returning domain types, `changed()` signal after any write |
| services | `StatsService` | all numbers in §6, pure functions over repository data (testable with fixed "today") |
| services | `AgendaService` | `QList<AgendaItem> items(QDate from, QDate to)` merging interviews, follow-ups, topic target dates |
| services | `ReminderService` + `INotifier` (`TrayNotifier`, `InAppNotifier`) | timer, due-reminder computation (pure, testable), dispatch through the notifier chosen by `QSystemTrayIcon::isSystemTrayAvailable()` |
| platform | `src/platform/*` | the only place for `Q_OS_*` code; empty until really needed |
| services | `ExportService` / `ImportService` | CSV, ICS, JSON backup/restore (version field, validation before write) |
| services | `SeedService` | loads `resources/seed/topics.json` |
| services | `SettingsService` | typed wrapper over `QSettings` with change signals |
| models | `ApplicationTableModel`, `ApplicationFilterProxy` | table data/roles (`SortRole`, `StatusRole`), text + status filtering |
| models | `TopicListModel`, `TopicBoardProxy` | one flat model; one proxy instance per board column (status + track filter); drag & drop with MIME `application/x-jobprep-topic-id` |
| models | `AgendaListModel` | items of the selected day |
| ui/widgets | `StatCard`, `ProgressRing`, `FunnelWidget`, `MonthView`, `ChipBar`, `EmptyState`, `Toast` | reusable painted/composite widgets |
| ui/delegates | `StatusBadgeDelegate`, `TopicCardDelegate`, `ProgressBarDelegate` | custom item painting |
| ui/theme | `Tokens`, `ThemeManager`, `StatusStyle`, `IconProvider` | design tokens, runtime theme switch, tinted SVG icons |

**Data flow.** Dialog/page → repository write → repository emits `changed()` → models reload (full reset is fine for v1) → views repaint; pages that show derived values (dashboard, status bar) recompute from services on `changed()` or `onActivated()`.

**Errors.** Repositories return `bool`/`std::optional` and keep `lastError()`; UI shows a non-blocking toast for recoverable errors and a message box for fatal DB open/migration failures. Log with `qCWarning` categories (`jobprep.data`, `jobprep.ui`, ...).

## 9. Seed data (`resources/seed/topics.json`)

**C++** — Modern C++ essentials (auto, structured bindings, constexpr, optional/variant, concepts) · RAII & smart pointers (unique/shared/weak, custom deleters, rule of 0/5) · Move semantics & value categories · Templates & metaprogramming · STL containers & algorithms (complexity, iterator invalidation, ranges) · Concurrency & memory model · OOP & design patterns · Memory, performance & UB · Qt essentials (signals/slots, model/view, event loop) · Build & tooling (CMake, sanitizers, debugging, testing) · Algorithms practice · System design basics.

**English** — Self-introduction (60 s pitch) · Describing my projects (STAR stories) · Behavioral questions · Technical vocabulary and explaining code aloud · Clarifying questions & small talk · Grammar refresh (tenses, articles, prepositions) · Listening practice (talks, shadowing) · Writing: resume, cover letter, follow-up e-mails · Salary & offer negotiation phrases · Mock interviews (record and review).

Each topic gets 3–5 checklist items (e.g., "Read cppreference page", "Write a 30-line example", "Explain it aloud in 2 minutes", "Solve 2 related tasks"). All seed topics start as *backlog*.

## 10. Import / export formats

- **CSV (applications)**: UTF-8 with BOM, `,` separator, header row, columns = §3.3 A1 plus all remaining fields, dates `yyyy-MM-dd`.
- **ICS (interviews)**: one `VEVENT` per interview with CRLF line endings, floating local `DTSTART/DTEND`, `SUMMARY` "Interview: <company> — <type>", `LOCATION`, `DESCRIPTION` (position, interviewer, notes), plus `VALARM` 60 minutes before.
- **JSON backup**: `{ "format": "jobprep-backup", "version": 1, "exportedAt": "...", "tracks": [], "topics": [], "subtasks": [], "sessions": [], "applications": [], "statusHistory": [], "interviews": [] }`. Restore validates version and referential integrity, runs in one transaction, and rolls back on error.

## 11. Non-functional requirements

- Cold start < 1 s on a normal laptop with 1,000 applications and 5,000 sessions.
- UI never freezes > 100 ms; no work on the GUI thread beyond simple queries.
- Memory < 150 MB idle.
- Works at 100–200% display scaling on both OSes.
- Quality gate: `scripts/check` passes (zero warnings, all tests green, `ui_smoke` free of Qt warnings); every milestone extends `ui_smoke` for the UI it adds.
- Builds and passes tests on Windows (MSVC 2022 or MinGW) and Linux (GCC 11+ or Clang) with Qt 6.8+; no `Q_OS_*` outside `src/platform/`.
- Data lives in the standard per-user app-data folder (`%APPDATA%` on Windows, `~/.local/share` on Linux) via `QStandardPaths`.
- No data loss on crash: writes are transactional; WAL on.

## 12. Milestones and acceptance criteria

**M0 — Scaffold & shell.**
- CMake project builds on Windows and Linux through `CMakePresets.json` (`dev-windows`, `dev-linux`, reading `$env{QT_PREFIX_PATH}`); `scripts/check.sh` and `scripts/check.ps1` run configure + build + ctest; `ctest` includes a `ui_smoke` test (offscreen) that creates `MainWindow`, visits all pages, toggles both themes and fails on any `qWarning`/`qCritical`; `.clang-format`, `.gitignore`, `.gitattributes`; resources set up (bundled UI font and license if the files are available).
- `MainWindow` with sidebar navigation to 5 empty pages, status bar, ThemeManager with light/dark/accent switching at runtime, `SettingsService`, remembered geometry.
- Acceptance: app starts; switching pages and theme works instantly; window state persists after restart; no OS-specific code outside `src/platform/`.

**M1 — Data layer.**
- `Database` + migration 001 + all repositories + `AppContext`; `SeedService`.
- Tests: migrations from empty DB, CRUD for each repository, cascade/`SET NULL` behavior, `changed()` emitted once per write, status history written on status change.
- Acceptance: all tests green; first-launch "Load sample data" creates tracks and topics.

**M2 — Jobs page.**
- `ApplicationTableModel`, proxy, `StatusBadgeDelegate`, toolbar (search, chips, export CSV), `ApplicationDialog` (Overview + Timeline; Interviews tab may be read-only list until M3), context menu, empty state.
- Acceptance: add/edit/delete/duplicate applications; sort and filter work; status change appears in Timeline; CSV export opens in Excel with correct columns.

**M3 — Calendar & interviews.**
- `InterviewDialog`, Interviews tab complete, `AgendaService`, `MonthView`, agenda panel, layer toggles.
- Tests: `AgendaService` for ranges across month borders, follow-ups and target dates.
- Acceptance: an interview created from Jobs appears on the right day; clicking an agenda item opens it; month navigation and "Today" work; dots match items.

**M4 — Study page.**
- `TopicListModel`, `TopicBoardProxy`, board with drag & drop, list view, detail panel with checklist and resources, session logging, track management, filter chips.
- Acceptance: dragging a card changes status and persists; progress bars follow checklist ticks; a logged session changes weekly minutes.

**M5 — Dashboard & reminders.**
- `StatsService` + tests (fixed clock), `StatCard`, `ProgressRing`, `FunnelWidget`, Today/Upcoming lists, status bar summary, `ReminderService` + `INotifier` with `TrayNotifier` and automatic `InAppNotifier` fallback, `reminders_sent`. The weekly goal is read from `SettingsService` (default 360 min).
- Acceptance: numbers match hand-calculated test data; goal progress uses 360 min by default and a different goal passed in tests; a reminder for an interview in 59 minutes fires once and clicking it opens the interview; with no system tray the reminder appears as an in-app toast.

**M6 — Polish & data tools.**
- Settings page complete (theme, accent, week start, **weekly study goal control**, reminder lead times, close-to-tray), JSON backup/restore, ICS export, empty states everywhere, keyboard shortcuts, app icon, tab order pass, HiDPI check.
- Run instructions: walk through `docs/RUNNING.md` step by step on the OS available and fix anything inaccurate. No installers and no AppImage; the app runs from the build folder.
- Optional, only if the repo is hosted on GitHub: a CI workflow building and testing on `windows-latest` and `ubuntu-latest`.
- Acceptance: backup → wipe → restore reproduces identical data; all shortcuts from §3.7 work; changing the weekly goal from 6 h to 8 h updates the dashboard instantly and survives a restart; no clipped text at 150% scaling in either theme; app starts on Windows and on Linux (X11 or Wayland).

## 13. Future ideas (not v1)
Kanban for applications, per-company contacts directory, salary comparison chart, spaced-repetition flashcards for English vocabulary, pomodoro timer linked to topics, Markdown preview for notes, optional encrypted backup.

## 14. Changelog
- 1.0 — initial spec.
- 1.2 — AppImage/packaging dropped (run from build folder, `docs/RUNNING.md` is the guide); build quality gate added (`scripts/check`, `ui_smoke`).
- 1.1 — Windows + Linux as first-class targets (tray fallback, platform folder, packaging, presets); weekly study goal default 6 h (360 min), user-adjustable.
- 1.3 — `sampleDataPrompted` setting added for the first-launch sample-data prompt.
- 1.3 — `insert()` writes an initial `status_history` row (NULL → initial status).
- 1.3 — applications CSV column set fixed to 22 English names: §3.3 A1 first, then every remaining `applications` field; no id column.
- 1.4 — `AgendaItem` loses `color` (handled by UI `AgendaKindStyle`) and gains `past` (boolean).
- 1.5 — `AgendaService` requires a `TrackRepository&` (was an optional pointer); it only supplies the study-target subtitle.
- 1.5 — `InterviewRepository::inRange` keeps inclusive `start_at >= ? AND start_at <= ?` bounds; `AgendaService` passes the last second `23:59:59`. Stored date-times are whole-second `yyyy-MM-ddTHH:mm:ss` (no fractional seconds, no offset), so text comparison is exact and the half-open form was not needed.
- 1.5 — `CalendarPage` takes explicit repository/service references (no `AppContext&`), matching `JobsPage`.
- 1.5 — `StatusStyle::label(InterviewType)` / `label(InterviewOutcome)` centralize interview type and outcome labels for tables and dialogs.
- 1.5 — `AgendaItem::past` is a neutral fact; the UI dims only past interviews. Overdue follow-ups and study targets still need attention and are never drawn as "done".
- 1.5 — `CalendarPage` gains a trailing defaulted `std::function<QDate()>` "today" provider (and `MonthView` takes one); the default is `QDate::currentDate`. Fixed-clock tests inject a date instead of relying on the real clock.
- 1.5 — Calendar layer chips are checkable `QPushButton`s using the shared chip QSS with a kind-colored dot; the per-kind SVG icons (`AgendaKindStyle::icon`) are used in the agenda rows, not on the chips. Layer visibility persists via `SettingsService` (`calendarLayer*`).
- 1.5 — `FollowUpDialog` writes `applications.next_action_date`: it offers only non-closed applications and asks for confirmation before replacing an existing planned follow-up.
- 1.5 — `MonthView` keyboard: arrows move by one day/week, PageUp/PageDown by a month, Home/End jump to the first/last day of the week (respecting `firstDayOfWeek`), Enter/Space activate. `MonthView` uses `Qt::StrongFocus`, so it is reachable with Tab and shows a filled focus cell.
- 1.6 — `topicProgress()` helper lives in `domain`: pure derived-rule helper function so models and services can both calculate topic progress without coupling.
- 1.6 — Done topics are never overdue (SPEC §3.2 S5): `TopicListModel::OverdueRole` evaluates to false when topic status is Done, matching `AgendaService` which already excludes Done topics from study targets.
- 1.6 — `TopicRepository::moveTopic` and `subtaskProgress()` added to the data layer: `moveTopic` provides atomic status and position updates across columns and track filters with relative order preservation for hidden topics; `subtaskProgress()` provides batched subtask counts to avoid N+1 queries.
- 1.6 — Settings keys `study/splitterState`, `study/viewMode` (default "board", unknown values fall back to "board"), and `study/trackFilter` (default -1) added to `SettingsService`.
- 1.6 — Header search is not used on the Study page (SPEC §7.1): header search stays Jobs-only; Study provides a local search box (Ctrl+F) that filters topics by title and tags across columns.
- 1.6 — Detail panel autosaves on selection change, focus loss, page hide, and window close, and rejects an empty title with an inline error message and no write. This replaces the modal "Discard changes?" confirmation pattern of `ApplicationDialog` for study topic editing.
- 1.6 — Resource links accept only http and https URLs; other schemes (such as `file:` or `javascript:`) are rejected on addition and refused on open.
- 1.6 — `StudyPage` exposes public page API `newTopic()` (focuses Backlog quick-add, or opens `TrackDialog` when 0 tracks exist), `selectTopic(int id)` (resets track filters and search text if they hide the topic, selects the topic card, opens the detail panel, and returns false for unknown IDs), and `triggerNew()` (delegates to `newTopic()`).
- 1.6 — List view is implemented as a flat sortable `QTableView` backed by `TopicTableProxy` in `src/models` with `StatusBadgeDelegate` (Topic mode) and `ProgressBarDelegate` (amendment C4), rather than a grouped list, maintaining visual and behavioral consistency with the Jobs table. Drag-and-drop is disabled in the list view; column sorting supports Track, Title, Status, Priority (by rank), Progress, and Target Date (empty dates placed last in both directions).
- 1.6 — Track colors use 8 curated preset swatches in `Theme::Tokens::trackPalette()` (including seeded C++ `#5B6CFF` and English `#F59E0B`, amendment C2); if a track has an existing non-preset color, `TrackDialog` displays and selects an additional custom swatch preserving that color.
- 1.6 — Track deletion cascade: deleting a track deletes all topics and their checklist items under that track with confirmation (stating the number of deleted topics and that logged study time is kept), while retaining all logged study sessions (`topic_id` set to NULL) to preserve total study time; the track filter resets to All (amendment C2).
- 1.6 — Header search on the Study page is explicitly disabled with tooltip `"Use the search box on this page, Ctrl+F"` (amendment B8), pointing users to the page-local search input.
- 1.6 — Header `+ New` menu routes directly: `Topic` switches to the Study page and focuses the Backlog quick-add (or opens `TrackDialog` if no tracks exist); `Study session` opens `SessionDialog` preselected to the currently selected topic (if any) (amendment B7). Ctrl+N calls the current page's `triggerNew()`.
- 1.6 — Activating a Study Target from the Calendar page agenda list emits `studyTopicActivated(int)` which calls `MainWindow::openTopic(int)`: switches to the Study page (in board or list view), clears any search or track filter hiding the topic, selects the topic, and opens the detail panel (amendment B9). If the topic was deleted in the meantime, a non-fatal message is shown without crashing. Double-clicking a day in `MonthView` continues to open "New interview" per SPEC C5.
- 1.6 — No session edit/delete UI in M4 (amendment C1; SPEC §3.2 S4 requires logging only; a wrong session cannot be corrected until M6). Session duration is capped at 1440 minutes (24 hours) with inline validation requiring >= 1 minute.
- 1.6 — `CalendarPage` takes `TrackRepository&` (amendment C3): listens to `TrackRepository::changed` so deleting a track cascades topic deletion and clears Calendar study-target markers immediately.
- 1.6 — Board view single selection: exactly one topic card is selected across the entire board at any time; selecting a card in one column clears the selection in other columns, and moving a card (via drag-and-drop, Alt+Left/Right, or context menu) preserves selection in the destination column (SPEC §3.2 S7).
- 1.7 — `domain::stageRank`: pure domain helper mapping `ApplicationStatus` to stage rank 0..5 (wishlist 0, applied 1, hr_screen 2, technical 3, final 4, offer/accepted 5; rejected/withdrawn/ghosted `std::nullopt`). `statusesWithRankAtLeast(minRank)` dynamically derives the status list per rank without duplicating status names in SQL.
- 1.7 — `DashboardStats` in `src/services/StatsService.h` holds numeric and domain types only (`appliedThisWeek`, `activeProcesses`, `interviewsNext7Days`, `studyStreakDays`, `weekMinutes`, `goalMinutes`, `goalPercentReal`, `goalPercentCapped`, `funnel`, `interviewRate`, `trackProgress`). No presentation strings or UI formatting in `services`.
- 1.7 — "Interviews in next 7 days" window is defined as `[now, QDateTime(today.addDays(6), 23:59:59)]`, strictly covering today plus the next 6 calendar days (7 calendar days). Only non-cancelled interviews count.
- 1.7 — Only `outcome == cancelled` excludes an interview from next 7 days, Upcoming, Today, status bar "Next:", and reminders. Interviews of closed applications remain visible until cancelled.
- 1.7 — Study streak counts consecutive days with sessions strictly up to and including `today`; future-dated sessions neither count toward nor break the streak. Weekly minutes sum sessions within `[weekStart, weekEnd]`, including future sessions inside the current week.
- 1.7 — `AgendaService::todayItems` returns today's non-cancelled interviews, overdue/due follow-ups, and overdue/due open topics. Deterministic ordering: overdue items first (oldest date first, FollowUp < StudyTarget, then title), followed by today's items (timed first by time, then non-timed by kind Interview < FollowUp < StudyTarget, then title). Overdue is indicated by `past = true` and `date < today`.
- 1.7 — `AgendaService::upcomingInterviews` returns upcoming non-cancelled interviews starting on or after `from`, ordered by start_at, id.
- 1.7 — `InterviewRepository::inRange` behavior is untouched. `countUpcoming(from, to)` and `upcoming(from, limit)` added to data layer, both excluding cancelled interviews.
- 1.7 — Data layer queries for stats and agenda: `ApplicationRepository::appliedCountInRange`, `funnelStageCounts`, `byNextActionDateUpTo`, `SessionRepository::distinctSessionDates`, `TopicRepository::byTargetDateUpTo`.
- 1.7 — `DayWatcher` in `src/services/`: periodic 60-second timer service detecting calendar day rollover across midnight, emitting `dayChanged(QDate)` with an injectable clock seam.
- 1.7 — `DashboardController` in `src/ui/`: centralized refresh owner batching mutations from repositories (`apps`, `interviews`, `sessions`, `topics`, `tracks`), `SettingsService` (`weeklyGoalMinutesChanged`, `firstDayOfWeekChanged`), `DayWatcher::dayChanged`, and `HomePage::onActivated` via a single-shot 0 ms timer; assembles immutable `DashboardSnapshot` shared between `HomePage` and `MainWindow` status bar; exposes `refreshCount()` test seam.
- 1.7 — `FormatHelper` in `src/ui/`: presentation formatting utility for durations, weekly goal text, streak wording, interview rates (`std::optional<double>` to formatted percent or en-dash), next interview status bar text, and funnel stage labels.
- 1.7 — `GoalProgressBar` in `src/ui/widgets/`: custom-painted horizontal widget for weekly study goal progress; visual fill clamped to 100% while actual percentage is displayed in text.
- 1.7 — `EmptyState` in `src/ui/widgets/`: supports optional secondary action button for dual quick-actions (`+ Add application` and `+ Add topic`) on `HomePage` (SPEC §3.1 D6).
- 1.7 — `MainWindow::openInterview(int)` and `MainWindow::openApplication(int)`: switch page stack to owning page (`CalendarPage` or `JobsPage`) and delegate to page methods (`CalendarPage::openInterview(int)` selecting date in `MonthView` and opening `InterviewDialog`; `JobsPage::openApplication(int)` selecting row and opening `ApplicationDialog`). Non-fatal status bar notification shown if item was deleted.
- 1.7 — Notifier layering: `INotifier` abstract interface and `Notification` struct reside in `src/services/` free of UI/Widgets dependencies; concrete visual notifiers (`TrayNotifier`, `InAppNotifier`, `Toast`) live in `src/ui/` (Part D).
- 1.7 — `INotifier` delivery contract: `bool notify(const Notification&)` returns true only if presented to the user; false indicates delivery failure (e.g. window hidden or minimized), recording nothing in `reminders_sent` and retrying on the next periodic tick.
- 1.7 — `Notification` carries pure domain metadata (`kind`, `refId`, `company`, `position`, `interviewType`, `when`, `leadMinutes`, `count`), not presentation text. Formatted text is generated in `ui/`.
- 1.7 — Interview reminder reschedule: key `interview:<id>:<lead>` in `reminders_sent` is valid only if stored `sent_at >= start_at - lead`; otherwise it is treated as unsent, firing again without requiring cross-table deletes from `InterviewRepository`.
- 1.7 — Smallest-lead coalescing: when multiple leads are due simultaneously, exactly one notification for the smallest lead is delivered, and all due leads are marked sent. If a smaller lead was already delivered for an interview, larger due leads are marked sent silently without notifying.
- 1.7 — Overdue follow-ups: evaluated exclusively during the startup pass (`startupPass = true`) for non-closed applications with `next_action_date < today`. Produces a single notification with `refId` if 1 application is overdue, or a summary notification (`refId = 0`, `count = N`) if multiple are overdue. Key `overdue:<appId>:<today>` prevents repeats on same-day restarts.
- 1.7 — `reminders/interviewLeadMinutes` setting in `SettingsService`: integer list (default `{1440, 60}`), sanitized to positive unique values sorted descending (falls back to default if empty/invalid), with `interviewLeadMinutesChanged` signal.
- 1.7 — Requirement R4 "Close to tray" deferred to Milestone M6.
- 1.7 — `NotificationText` in `src/ui/`: presentation text helper constructing localized titles and messages from `Notification` payloads; formats natural lead times ("in 1 hour", "in 24 hours", "in 90 minutes") with singular/plural support, single overdue follow-ups (company and position), and multi-application summaries ("N follow-ups are overdue").
- 1.7 — `TrayNotifier` in `src/ui/`: `INotifier` implementation using `QSystemTrayIcon` with themed `app.svg` icon; displays OS notifications via `showMessage()`; caches last notification to support payload-free `messageClicked` activation; provides a minimal tray context menu ("Show JobPrep" and "Quit") and routes left clicks to `MainWindow::bringToFront()`.
- 1.7 — `InAppNotifier` and `Toast` in `src/ui/`: fallback in-app notification system displaying up to 3 sticky toasts with an overflow indicator (`+N more`), automatically repositioned on `MainWindow` resize; tokens and surface contrast only (no `QGraphicsDropShadowEffect`); never steals focus on appearance; dismissed via close button or `Escape`, activated via click or `Enter`/`Return`; returns false when the window is hidden or minimized.
- 1.7 — `NotifierFactory` (`makeNotifier`) in `src/ui/`: testable selection function returning `TrayNotifier` when system tray is available or `InAppNotifier` as fallback.
- 1.7 — `ReminderCoordinator` in `src/ui/`: owns `std::unique_ptr<INotifier>` and `std::unique_ptr<ReminderService>` in declaration order ensuring service destruction before notifier; created in `main.cpp` as a local after `MainWindow` and started after `show()`; tests construct it with an injectable clock and avoid auto-starting reminders in test harnesses.
- 1.7 — `MainWindow::bringToFront()`, `routeNotification(const Notification&)` and `openItemFromNotification(const Notification&)`: routes `Interview` to `openInterview(refId)`, `FollowUp` and single `OverdueFollowUps` to `openApplication(refId)`, and summary overdue (`refId == 0`) to `JobsPage`.
- 1.8 — Board single selection and mouse click synchronization: `TopicBoardColumn` selection model connections listen to both `currentChanged` and `selectionChanged`; clicking a card in any column establishes single board-wide selection, clears selection and current index across all other columns with immediate viewport update, loads the topic into `TopicDetailPanel`, and outlines strictly the selected card.
- 1.8 — `TopicDetailPanel` empty state: when no topic is selected, the panel is visibly disabled displaying the one-line hint `"Select a topic"`; the target-date editor displays `"No target date"` via `specialValueText` when cleared or unset rather than `2000-01-01`.
- 1.8 — Toast Escape key handling: Esc dismisses a toast only when keyboard focus is inside that toast; removed global window-level `KeyPress` event filter from `InAppNotifier`.
- 1.8 — `NotificationText` natural remaining time: `formatRemainingTime` computes remaining time from `notification.when` relative to the current clock ("today at 17:00", "tomorrow at 10:00", "in 50 minutes") rather than displaying static lead time ("in 24 hours").
- 1.8 — Logging category `jobprep.reminders`: debug-level logging for reminder service start, check ticks with due count, `notify` delivery results, and recorded keys.
- 1.9 — Checklist and TopicDetailPanel focus preservation (F1): `saveNow()` and `TopicRepository::update` perform no-op checks when fields haven't changed, preventing redundant writes, model resets, and reload churn. Model resets guard against clearing panel state during re-selection. Checklist Add input is preserved during focus shifts. Added `jobprep.study` logging category with default Info threshold.
- 1.9 — Target-date field flow (F2): Date editor is always enabled while a topic is selected, displaying "No target date" via `specialValueText` when unset. Calendar popup defaults to current month when opened. "Clear" button and Delete/Backspace key reset the date and keep the field enabled. Removed "Set date" flow.
- 1.9 — TopicListView drag handling (F3): Removed duplicate drag event filtering on viewport; added `m_isDragging` guard in `startDrag` to prevent re-entrant calls and `QDragManager::drag in possibly invalid state` warnings.
- 1.9 — Board selection painted outline verification (F4): Tests verify via offscreen pixel grabs across light and dark themes that strictly one card border on the board is painted with `tokens.accent`.
- 1.9 — Notifications delivery and formatting (F5): `NotificationText` summary indicates what and when ("Interview in 29 minutes", "Interview today at 17:00"), and body contains "Company · Type (position)". `TrayNotifier` uses `Warning` icon (normal OS urgency) for interview and follow-up reminders. `TrayNotifier::notify` validates tray availability and icon visibility, and falls back to `InAppNotifier` if `supportsMessages()` is false. Logging category `jobprep.reminders` defaults to Info threshold with concise tick, notify, and recorded key lines.
- 1.10 — Checklist display and startup visibility: removed signal blocking on `m_checkList->model()` in `TopicDetailPanel` (replacing with `m_syncingChecklist` guard on `persistChecklistOrder`), restoring Qt's model-to-view signals so `QListView` lays out items on fresh start without requiring a theme switch; explicit `color: @text` configured on `#StudyDetailCheckList`, `#StudyDetailCheckList::item`, and `#StudyDetailResourcesTable` in `theme.qss`; checklist minimum height accommodates at least 5 rows dynamically via font metrics without fixed pixel heights and expands with spare panel height (stretch 1, Expanding policy); checklist shows an internal scrollbar when overflowing in both themes; adding a checklist item refreshes from repository, scrolls to the new row, clears input, and preserves keyboard focus (Enter adds subsequent items); logged `checklist refreshed: count=N` in `jobprep.study`.
- 1.11 — `ui_smoke` split & test helpers: Lean `ui_smoke` keeps global smoke tests (window creation and navigation, theme/accent switching, sidebar collapse, new entry points modal sanity, zero warnings, and real key event shortcut verification) while page-specific tests are split into dedicated test executables `tst_PageJobs` (21 slots), `tst_PageCalendar` (12 slots), `tst_PageStudy` (26 slots), and `tst_PageHome` (16 slots). Reusable UI test fixtures and helpers (`WindowFixture`, `JobsFixture`, `CalendarFixture`, `StudyFixture`, warning filter, modal handlers, widget finders, `isolateSettings` using `QTemporaryDir`) reside in static library `jobprep_ui_test_helpers`. CTest `LABELS` (`data`, `model`, `service`, `ui`) added across test suites.
- 1.11 — Real key-event dispatch infrastructure: `JobPrep::UiTest::sendKeySequence` and `sendKeyClick` dispatch real `QTest::keyClick` events to the active window's focused widget under the offscreen QPA platform plugin with window activation validation (`QTest::qWaitForWindowActive`). `MainWindow` explicitly registers `Ctrl+,` as an alternative shortcut for Settings alongside `QKeySequence::Preferences` because under X11/Linux generic platform themes `Preferences` maps to `Qt::Key_Settings`.
- 1.11 — Selection model signal blocking audit & rule: Re-audited `QSignalBlocker` on selection models in `TopicBoardColumn` and `StudyPage`. A view listening to a selection model does not repaint when signals are blocked unless manually forced; `TopicBoardColumn::clearSelection()` explicitly calls `viewport()->update()`, and `StudyPage::onColumnSelectionChanged` explicitly triggers `viewport()->update()`. Documented rule: never block signals of a model or selection model; block widget's own signals only, or use a re-entrancy flag.
- 1.11 — Continuous integration workflow: Added `.github/workflows/ci.yml` matrix workflow targeting `ubuntu-latest` and `windows-latest` with pinned `aqtinstall` (Qt 6.8.2 LTS), caching, headless offscreen dependencies, MSVC environment, concurrency groups, timeout guards, and artifact upload on failure.
- 1.11 — Windows compiler audit & NOMINMAX: Defined `NOMINMAX` under `if(WIN32)` in `CMakeLists.txt`. Audited code with GCC `-Wshadow -Wconversion -Wsign-conversion` proxy flags to eliminate MSVC C4456/C4458/C4244/C4267/C4389 warnings in `src/`.
- 1.12 — Settings page behavior and instant persistence (B1, B2, B4): Weekly study goal spinbox (60-2400 min, step 15) formatted as natural hours/minutes with instant updates to `SettingsService`, `DashboardController`, and `GoalProgressBar` without restart; reminder lead times editor with presets and custom spinbox, deduplicated and sorted descending, protected against emptying, with "Reset to default" ({1440, 60}) and immediate notification tick recalculation; first day of week normalization helper `SettingsService::toDayOfWeek` unifying Monday/Sunday conversion across `DashboardController`, `MonthView`, `CalendarPage`, and `StatsService`.
- 1.12 — Close to tray requirement R4 (B3): `MainWindow` accepts system tray availability status once at startup; when `closeToTray` is enabled and tray is available, window close hides the window instead of quitting and sets `quitOnLastWindowClosed(false)` while showing a one-time tray notification hint; closing when disabled or without tray executes normal shutdown. System tray "Quit" action runs unified `prepareForShutdown()` path (autosaving active panel edits, persisting window geometry/splitter state, stopping timers) before quitting `qApp`. Reminders continue firing while hidden to tray; pending reminders on Quit are re-evaluated during the startup pass of the next launch. Restoring from tray uses `bringToFront()` (noting Wayland compositor focus-stealing prevention).
- 1.12 — System color scheme listener (B5): `ThemeManager` connects to `QGuiApplication::styleHints()->colorSchemeChanged()` to dynamically re-evaluate and apply dark/light stylesheets when configured in `System` mode.
- 1.12 — Dedicated Settings test suite `tst_PageSettings` & visual verification (B6): Added 19 test slots validating weekly goal persistence/instant updates, lead times management, tray availability/hide/restore/quit-flush semantics, day of week bounds, theme switching, tab focus chain navigation, keyboard shortcuts, and painted contrast (>= 3:1 across all 6 accents in light/dark themes) and screenshot generation (`settings-light.png`, `settings-dark.png`).
- 1.13 — Weekly goal control keyboard entry & rounding (C0.3): `GoalSpinBox` accepts bare numbers as hours (`8` -> 480 min), decimals with dot or comma (`7.5`, `7,5` -> 450 min), colon or semicolon hours/minutes separators (`7:30`, `7;30` -> 450 min), decimal fraction of an hour (`7.30` = 7.3 h = 438 min rounded to nearest 15 min -> 435 min), explicit hour suffixes (`8 h`, `8h`, `7 h 30 min`), and minutes only with explicit `min` suffix (`450 min`, `90 min`). Values are rounded to nearest 15 minutes, clamped to range [60, 2400], and invalid text restores the previous value.
- 1.13 — Reminder lead-times editor (B2, C0.5): Presets (15 min, 30 min, 1 h, 2 h, 1 day, 2 days) plus custom minute spinbox (1–10080 min), deduplicated and sorted descending, protected against emptying (remove button disabled on last entry), with "Reset to default" ({1440, 60}) and immediate notification tick recalculation. Formatting via `FormatHelper::formatLeadTime` ("1 day", "2 days", "1 hour", "90 minutes").
- 1.13 — Close-to-tray hardening & shutdown synchronization (C0.1): `closeEvent` queries a live availability provider (`isTrayAvailableLive()`, verifying tray icon visibility and `QSystemTrayIcon::isSystemTrayAvailable()`) rather than frozen startup state; if tray becomes unavailable while running, window closes normally and runs `prepareForShutdown()`. Accepts close without hiding when `qGuiApp->isSavingSession()` is true. Hiding to tray flushes pending active editor edits (`flushStateAndEdits()`) and persists window geometry/state to prevent data loss on crash or kill. First-time hint stored under `app/trayHintShown` is set only after delivery has actually been requested through the tray message callback, and uses `Warning` icon (normal OS urgency on GNOME/freedesktop). Restoring from tray uses `bringToFront()` which shows hidden windows and unminimizes minimized windows, triggered on all activation reasons (`Trigger`, `DoubleClick`, `MiddleClick`). Tray context menu "Quit" action and regular shutdown both invoke `MainWindow::prepareForShutdown()`.
- 1.13 — First day of week normalization helper (B4, C0.6): Centralized helper `SettingsService::toDayOfWeek` normalizes integer week start (1 -> `Qt::Monday`, 7 -> `Qt::Sunday`, other -> `Qt::Monday`) unifying week boundary calculations across `DashboardController`, `MonthView`, and `CalendarPage`.
- 1.13 — Runtime system color scheme tracking (B5): `ThemeManager` dynamically listens to `QGuiApplication::styleHints()->colorSchemeChanged()` to switch stylesheets without app restart when configured in `System` mode.
- 1.13 — Window maximize and constraint isolation (C0.7): Fixed `m_searchEdit` in `StudyPage` and `JobsPage` to be added to toolbars with `Qt::AlignVCenter` alignment, ensuring child maximum width constraints (320 px) do not propagate to the top-level `MainWindow` layout. `MainWindow::maximumSize()` equals `QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX)` across all pages, allowing window managers to maximize cleanly to available screen geometry.
- 1.14 — ICS calendar export (C5): `ExportService::exportInterviewsToIcs` serializes interviews to standard iCalendar format (`VCALENDAR` 2.0 with PRODID, Gregorian calendar) with stable UIDs `jobprep-interview-<id>@jobprep.local`, injected UTC `DTSTAMP`, floating local start/end times (`DTSTART`, `DTEND`), escaped special characters (`\`, `;`, `,`, newlines), strict 75-octet folding without UTF-8 boundary truncation, `VALARM` display reminder 60 minutes prior, and explicit CRLF line endings written via binary streams without `QIODevice::Text`. Cancelled interviews are omitted.
- 1.14 — JSON database backup serialization (SPEC §10): Complete export of all 8 application tables (`tracks`, `topics`, `subtasks`, `resources`, `study_sessions`, `applications`, `application_status_history`, `interviews`) with format `jobprep-backup` and version 1. Exports every schema column (including autoincrement IDs, sort positions, timestamps, notes, and lead time lists) in a single SQLite read transaction. Atomically persisted via `QSaveFile`.
- 1.14 — JSON database restore & pre-flight validation (C1–C3, C6, C7): Multi-stage restore workflow with upfront validation prior to modifying database: verifies JSON structure, header version, table array types, positive unique IDs, valid ISO dates/datetimes, domain enum strings, and foreign key referential integrity (topics->tracks, subtasks->topics, resources->topics, sessions->topics, status_history->applications, interviews->applications). Rejects unknown versions, broken references, and payloads exceeding 64 MB.
- 1.14 — Safety backup creation and retention (C6): Automatically creates a timestamped safety backup in `AppDataLocation/backups/pre-restore-<timestamp>.json` prior to wiping SQLite data. Aborts restore with error if safety backup cannot be written. Automatically prunes the backup folder to retain only the 5 most recent `pre-restore-*.json` files.
- 1.14 — Database restore worker and synchronization (C1, C2, C4): Executes restore on a dedicated worker thread with an isolated SQLite connection (`PRAGMA busy_timeout=5000`, `PRAGMA foreign_keys=ON`). Flushes pending Study edits and pauses `ReminderService` prior to worker start. Executes table wipe and restore inside a single transaction with cascading parent/child order. On success, clears `reminders_sent`, resumes reminders, emits unified repository `changed()` signals triggering coalesced UI refresh, safely clears `TopicDetailPanel` if the selected topic vanished, and preserves user application settings. SQLite sequences update so newly inserted records receive IDs greater than restored data.
- 1.14 — Data & Backups Settings section (C8): `SettingsDataCard` in Settings page exposes 6 primary operations: Export CSV, Export ICS, Backup JSON, Restore JSON, Load sample data, and Open data folder. Managed by `DataToolsController` with decoupled test seams (`IFileChooser`, `IMessagePresenter`, `IBusyIndicator`). Modal progress indicator displayed during async restore, with upfront replacement confirmation detailing the safety backup destination. Sample data loading prevents duplicate tracks/topics and requires confirmation if database already contains records. Full keyboard navigation and tab order support.
- 1.15 — Data & Backups completeness, safe quit, and warning fixes (C0): Verified schema completeness across all 7 domain tables (including all 20 canonical columns of `applications`) with dynamic `PRAGMA table_info` verification in `testBackupCoversEveryColumn` and full non-default round-trip equality testing in `testRoundTripEveryColumnNonDefault`. `RestoreService::waitForRestore` provides a synchronous wait mechanism with event loop pumping (`QCoreApplication::processEvents`) ensuring queued completion signals dispatch safely if the app shuts down during active restore; `MainWindow::prepareForShutdown` blocks up to 5 seconds for restore completion before shutdown. `DataToolsController::restoreJson` guards against concurrent restore requests. Proxy build warning audit using `-Wshadow -Wconversion -Wsign-conversion` resolved narrowing conversions in `ExportService`, `RestoreService`, and `tst_PageSettings`. Published errata report `docs/reports/M6-partC-errata.md` correcting the human-report column transcript in `docs/reports/M6-partC.md` §5.
- 1.16 — Keyboard shortcuts, empty state inventory, and accessible tab order (Part D1):
  - **Keyboard Shortcuts (SPEC §3.7 / D1):** Standard shortcuts bound using `QKeySequence::StandardKey` where available (`New`, `Find`, `Save`). Ctrl+1..4 navigate to Home, Study, Jobs, Calendar pages; Ctrl+, navigates to Settings. Page navigation shortcuts work globally even when focus is inside a `QLineEdit` or `QTextEdit`. Ctrl+N triggers the primary "New" action of the current active page (Home: open `ApplicationDialog`; Study: invoke `newTopic()`; Jobs: open `ApplicationDialog`; Calendar: open `InterviewDialog`; Settings: unregistered/no-op). Ctrl+F focuses and selects the search box on Study and Jobs pages, and is ignored on other pages. Ctrl+S triggers immediate auto-flush in `TopicDetailPanel` without loss of focus and triggers the default Save/OK button on dialogs (`ApplicationDialog`, `InterviewDialog`, `FollowUpDialog`, `SessionDialog`). Esc rejects modal dialogs (preserving existing dirty discard confirmations), collapses the Study detail panel, clears the Backlog quick-add line, clears search text without dropping focus, and dismisses active in-app notifications/toasts when focused.
  - **Collision Resolution (SPEC §14 / D1):** Alt+Left/Right on the Study board navigate focus between columns without triggering window-level menu traversal; Alt+Up/Down reorders cards within a column. Ctrl+F on the Study page prioritizes the page-local search edit over any global header search. Ctrl+, and Ctrl+1..4 pressed inside modal dialogs are completely blocked by the modal event loop and cannot trigger background page switching. Ctrl+S inside `QTextEdit` saves persisted state while blocking character insertion (`\x13`). Ctrl+N inside `QLineEdit` triggers the creation flow without appending the `n` character.
  - **Empty State Inventory & Filter Rules (SPEC §3.1 D6 / D2):** All primary lists, tables, tabs, and panels provide consistent empty-state visuals via the shared `EmptyState` widget styled through tokens. Crucially distinguishes absolute empty from filtered empty: when records exist in SQLite but are hidden by active status chips or search filters, the view presents a "No matches" header with a primary "Clear filters" action rather than an initial-onboarding message ("No applications yet").
  - **Tab Order and Visible Focus (SPEC §3.7 / D3):** Complete, deterministic tab order chains established across all dialogs (`ApplicationDialog`, `InterviewDialog`, `FollowUpDialog`, `SessionDialog`, `TrackDialog`) and pages (Home, Study, Jobs, Calendar, Settings). Icon-only buttons provide accessible names via `tr()`. Theme stylesheet defines a distinct 2px solid accent focus ring across buttons, input fields, and filter chips in both Light and Dark themes.
- 1.17 — Goal spin box typing rules (F1, superseding 1.13):
  The `GoalSpinBox` typing parser interprets user input as a time duration rather than a decimal float. The dot or comma acts strictly as a time separator (`<hours>.<minutes>`), identical to a colon or semicolon.
  - `<hours>.<minutes>` where minutes is two digits `00`..`59`: `<hours> * 60 + <minutes>`. For example: `7.30` -> 450 min (7 h 30 min), `7.05` -> 425 min (7 h 5 min).
  - `<hours>.<minutes>` where minutes is a single digit `0`..`5`: interpreted as tens of minutes (`.5` = 50 min, `.1` = 10 min). For example: `7.5` -> 470 min (7 h 50 min).
  - Single-digit minutes 6..9 (e.g. `7.6`) or two-digit minutes >= 60 (e.g. `7.75`, `7.60`, `7.99`) are invalid time syntax and rejected with an error.
  - Suffixes `h`, `hrs`, `hour`, `hours`, `m`, `min`, `mins`, `minutes` are accepted and stripped before parsing.
  - Parsed values are clamped to the range [60, 2400] minutes and rounded to the nearest 15 minutes.
  - Invalid text or out-of-range syntax restores the previous valid value on Enter or focus loss (`fixup()` and `validate()`), never committing garbage.
- 1.17 — Tab order chains and focus stability (F4):
  Explicit keyboard navigation order (`QWidget::setTabOrder`) established across all main pages, dialogs, and panels:
  - Sidebar: collapse toggle -> navigation buttons (Home, Study, Jobs, Calendar, Settings).
  - Home Page: Quick-action buttons when empty; or summary KPI cards, Next action, Weekly goal progress, and recent activity when populated.
  - Study Page: track chips -> search box -> Backlog quick-add -> board columns (Backlog, In Progress, Review, Done) -> column quick-adds -> TopicDetailPanel (18 controls in logical top-to-bottom order from title, status, priority, track, target date, clear date, notes, add item, checklist items, add link, link items, delete topic).
  - Jobs Page: status chips -> search box -> + Add Application button -> applications table view.
  - Calendar Page: layer chips (Interviews, Follow-ups, Study Targets) -> MonthView calendar grid -> navigation buttons (<, Today, >) -> agenda list view.
  - Settings Page: weekly goal spin box -> lead time presets -> custom lead time spin box -> add lead time button -> lead time list -> first day combo -> theme combo -> accent combo -> close to tray checkbox -> sample data button -> backup button -> restore button -> export CSV -> export ICS -> open data folder.
  - Dialogs (`ApplicationDialog`, `InterviewDialog`, `FollowUpDialog`, `SessionDialog`, `TrackDialog`): strictly ordered from primary inputs, date/time pickers, notes, to secondary/destructive action and standard dialog buttons (Cancel, Save/OK).
  - Focus Ring Stability: CSS `:focus` styling uses internal 2px borders with matching negative padding, verified by test to never change `geometry()` or `sizeHint()` upon gaining or losing focus.
- 1.17 — Diagnostic window logging category `jobprep.window` (F5):
  Registered `jobprep.window` logging category with default Info severity (`lcWindow().isDebugEnabled()` returns false). When enabled via `QT_LOGGING_RULES="jobprep.window.debug=true"`, `MainWindow` logs state changes on `resizeEvent`, `changeEvent(WindowStateChange)`, `screenChanged`, and after `show()`, outputting trigger name, `windowState()`, `geometry()`, `frameGeometry()`, `minimumSize()`, `maximumSize()`, `minimumSizeHint()`, `sizeHint()`, `screen()->availableGeometry()`, and `devicePixelRatio()`.
- 1.17 — Timestamp precision and `created_at` epoch fix (F3):
  Fixed `DbFormat::now()` where `current.setMSecsSinceEpoch(0)` incorrectly reset the timestamp to the 1970 Unix epoch (UTC+3 1970-01-01T03:00:00) instead of zeroing the sub-second millisecond component. Replaced with `current.setSecsSinceEpoch(current.toSecsSinceEpoch())`, ensuring all records (`applications.created_at`, `applications.updated_at`, `status_history.changed_at`, etc.) receive current local ISO-8601 date-times.
