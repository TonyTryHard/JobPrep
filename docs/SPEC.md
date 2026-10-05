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
