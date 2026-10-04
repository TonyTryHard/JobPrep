# M2 part 2 — transaction correctness, model/service polish, Jobs page UI

Status: written, waiting for approval before any code change.
Two commits, each only after `./scripts/check.sh` passes. Local commits only, never push.

Environment: Linux, Qt 6.10.2 (`/usr/lib/x86_64-linux-gnu/cmake/Qt6`), Ninja / Debug preset
`dev-linux`, `JOBPREP_WERROR=ON`. Build with `CMAKE_BUILD_PARALLEL_LEVEL=2`. A live display is
available (X11 `:0` + Wayland session).

## 0. Findings that drive the work (verified against the code)

| Item | State |
| --- | --- |
| 1a outer-failed flag | Absent. `Database.cpp` owner `commit()` never rolls back because of an inner failure; joined `commit()` returns `true` unconditionally. |
| 1b joined rollback clears pending | Bug present. `Transaction::rollback()` calls `clearPendingChanges()` for owner *and* non-owner. |
| 2 QPointer / non-const / shared helper | Not done. `QSet<QObject*> m_pendingChangeRepos`; `transaction() const` + `const_cast`; the owner/else pattern duplicated ~40x across 6 repositories; 4 of them (`InterviewRepository`, `SessionRepository`, `TopicRepository`, `TrackRepository`) never defer and emit unconditionally. |
| 3 injectable clock | Not done. `ApplicationTableModel::buildCache()` hardcodes `QDateTime::currentDateTime()`. |
| 4 status label switch | Present. `ApplicationTableModel::data()` duplicates `StatusStyle::label()` for the status column. |
| 5 CSV header pinned | Not done. The list exists twice, unpinned: inside `tst_ExportService.cpp` and as the fallback inside `ExportService::exportToFile`. |
| Part B | Greenfield. `JobsPage` is the M0 placeholder; `ApplicationDialog` has four empty tabs. `StatusBadgeDelegate`, `ChipBar`, `ApplicationFilterProxy`, `ExportService` are done. |

Also for the report: transaction depth is **counted**, not savepointed — no `SAVEPOINT` /
`ROLLBACK TO` statement exists in `Database.cpp`.

## Part A — small fixes (commit 1)

1. **`Database` outer-failure flag** (`src/data/Database.h`, `src/data/Database.cpp`)
   - Add `bool m_outerFailed` plus a private `markOuterFailed()`.
   - `Transaction::rollback()`: owner → `rollbackInternal()` + clear pending + reset flag;
     **non-owner → `markOuterFailed()` only, no `clearPendingChanges()`**.
   - `Transaction::commit()` (owner): if `m_outerFailed`, skip `COMMIT`, issue `ROLLBACK`, clear
     pending notifications, reset the flag, return `false`.
   - Reset the flag in `Database::close()` and after a successful owner commit.
2. **Pending set / const-correctness**
   - `QSet<QPointer<QObject>>` (drops the raw `QObject*`; `<QPointer>` already included).
   - `transaction()`, `deferChanged()`, `flushPendingChanges()`, `clearPendingChanges()`,
     `begin()`, `commitInternal()`, `rollbackInternal()` become non-const; remove the
     `const_cast` and the `mutable` qualifiers (`m_lastError` stays `mutable` for the public
     const `setError()`).
3. **`Data::Repository` base class** (`src/data/Repository.h`, `src/data/Repository.cpp`)
   - `Q_OBJECT` base on `QObject` with the single `changed()` signal and
     `protected bool commitAndNotify(Database&, Database::Transaction&)`.
   - All six repositories derive from it, drop their own `changed()` signal, and collapse every
     write path to `return commitAndNotify(m_database, transaction);`.
   - Side effect: the four repositories that emitted unconditionally when joining now defer
     correctly (one `changed()` per repository per outer commit).
   - `Repository.h` must be listed in the target sources for AUTOMOC.
4. **Injectable clock** in `ApplicationTableModel` (`using Clock = std::function<QDateTime()>`,
   defaults to `currentDateTime`, read once per `buildCache()`), plus deterministic tests:
   past pending interview ignored, future pending wins, cancelled/passed/failed ignored,
   fallback to next action (text + date, date only, text only).
5. **Delete the status label switch** in `ApplicationTableModel::data()` (~lines 93-102);
   `StatusRole` only — the delegate takes label and color from `StatusStyle`. The `SortRole`
   status key stays, as a file-local order table. Column header `tr()` strings unchanged.
6. **CSV header pinning**: `ExportService::applicationHeaders()` (documented order) and
   `ExportRow::toFields()`; `exportToFile` falls back to the canonical list. The test pins the
   exact order and that the field count matches the header count.
7. **Tests**
   - Update `tst_TransactionBehavior::testFailedWriteDoesNotQueue`: a failed joined write now
     poisons the outer unit of work, so line 74 changes from `QVERIFY(outer.commit())` to
     `QVERIFY(!outer.commit())`, plus `QCOMPARE(apps.count(), 0)`. This is the only intentional
     change to an existing assertion; `tst_Database::nestedTransactionJoinsOuter` is unaffected
     because its joined transaction commits successfully.
   - New: joined failure → outer `commit()` returns false, earlier joined writes are gone, zero
     `changed()`; all joined writes succeed → exactly one `changed()` per touched repository
     after the outer commit; successful joined write followed by a failing one → nothing left
     behind; joined rollback does not discard notifications for writes that still commit.

## Part B — the Jobs page UI (commit 2)

**PageBase hooks** (`src/ui/PageBase.h`): add `virtual void focusSearch() {}` and
`virtual void triggerNew() {}` — the header dispatches, pages implement, no business logic in
the header.

**JobsPage** (`src/ui/pages/JobsPage.{h,cpp}`, constructor takes `ApplicationRepository&`,
`InterviewRepository&`, `ExportService&`, matching the `SettingsPage` injection style;
`MainWindow.cpp:115` updated):
- Owns the model and proxy (QObject-parented) plus `StatusBadgeDelegate`.
- Toolbar: page search `QLineEdit` + `ChipBar` (All/Active/Interviewing/Offer/Closed) +
  Export CSV + `+ Application` (no shortcut of its own, tooltip only).
- Default sort Updated desc; footer with per-status counts from `countsByStatus()` and
  `StatusStyle::label()`.
- `QStackedWidget` with "no data" / table / "no matches" empty states.
- Context menu (`customContextMenuRequested`): Edit, Change status submenu, **Add interview…
  disabled**, Open URL enabled only for `http(s)`, Duplicate, Delete with confirm. Enter opens
  edit, Delete confirms, through an event filter on the view.
- Selection restored **by id** across model resets.
- `void setSearchText(const QString&)` — the single filter state. The page's own `textChanged`
  handler and the header forward both call it; the receiving line edit is updated under
  `QSignalBlocker` so no loop forms.
- `void newApplication()` — builds and `exec()`s `ApplicationDialog`; the single entry point for
  both create buttons.
- Narrow test seams: `int currentViewIndex()`, `int visibleRowCount()`,
  `bool exportTo(const QString& path, int* exportedRows, QString* error)` (extracted from the
  toolbar handler so tests never need a file dialog).

**ApplicationDialog** (`src/ui/dialogs/ApplicationDialog.{h,cpp}`): full Overview form with all
A4 fields (editable source combo, work-mode and status combos, salary min/max + currency,
applied date, next action + date, contacts) with inline validation messages (company/position
required, salary min <= max, optional e-mail check, A5 applied-date default); Notes `QTextEdit`;
read-only Interviews table; read-only Timeline table from `statusHistory()` that is empty and
disabled for a new application; dirty tracking with a "Discard changes?" confirmation on
Cancel/close.

**MainWindow** (`src/ui/MainWindow.{h,cpp}`):
- Header search `textChanged` → if the current page is not Jobs, switch the sidebar to Jobs
  first, then `jobsPage->setSearchText(text)`; the page box mirrors the text under
  `QSignalBlocker`.
- `+ New` becomes a `QToolButton` with a menu (object name `HeaderNewBtn`): **Application**
  enabled → `triggerNew()` on the current page; **Interview** disabled, tooltip "Arrives in M3";
  **Study session** disabled, tooltip "Arrives in M4".
- Shortcuts registered exactly once in MainWindow: `QKeySequence::Find` → `focusSearch()` on the
  current page (Ctrl+F focuses the Jobs page search box), `QKeySequence::New` → `triggerNew()`.
  No `setShortcut()` on any button, so no ambiguous-shortcut warning can fail `ui_smoke`.
- Typed `Pages::JobsPage* m_jobsPage` member for the dispatch.
- No new public accessors: tests use `findChild<QLineEdit*>(u"HeaderSearchEdit"_s)` and
  `findChild<QToolButton*>(u"HeaderNewBtn"_s)`.

**Tests — `tests/ui_smoke/tst_UiSmoke.cpp` extended, no new target** (the 13 test binaries stay
as they are). New private slots, inserted **before** `testZeroWarnings` so the message capture
still covers everything:

| Slot | Covers |
| --- | --- |
| `testJobsPageEmptyStates` | scoped fresh `:memory:` context + standalone page → `kNoData`; impossible search → `kNoMatches` |
| `testJobsPageAddAndEdit` | add via dialog → row visible; edit company → cell text updated |
| `testJobsPageDuplicateAndDelete` | Duplicate → count +1; Delete with confirm → count −1 and gone from the DB |
| `testJobsPageStatusChangeAppearsInTimeline` | context-menu status change → offer; `statusHistory(id)` grew; the Timeline tab shows the row |
| `testJobsPageSortAndFilter` | header click flips the order; chip `Closed` hides `Offer`; Cyrillic search hits notes |
| `testJobsPageCsvExport` | file header == `ExportService::applicationHeaders()`; data rows == visible rows in current order; empty case → header only |
| `testJobsPageSelectionKeptAcrossReloads` | select row 2, insert through the repository, same id still selected |
| `testApplicationDialogTabs` | Create and Edit mode, all four tabs, inline validation, discard confirmation on Cancel |
| `testHeaderSearchFiltersJobsTable` | header text switches to Jobs if needed and filters; the page box mirrors it |
| `testNewEntryPointsOpenOneDialog` | header menu **Application** and the page `+ Application` each open exactly one dialog; the other two entries disabled with tooltips |
| `testJobsPageThemes` | light and dark with the page and a dialog visible, zero warnings |

Shared-modal handling without weakening production code: a test helper arms
`QTimer::singleShot(0, ...)` to dismiss the next `QApplication::activeModalWidget()`, so `exec()`
still runs normally and "exactly one dialog" is asserted by counting top-level `ApplicationDialog`
widgets. Each acceptance slot starts with a private `resetJobsData()` helper so slots stay
independent.

**Real launch**: `XDG_DATA_HOME=$(mktemp -d) ./jobprep` on the session display — the first-launch
"Load sample data?" prompt from `main.cpp` is the expected fresh-folder behaviour; confirm the
process stays up, stderr has no warnings and `jobprep.sqlite` exists in that folder. Offscreen
fallback if the display refuses the connection, stated in the report.

## Risks

- AUTOMOC needs the new `Repository.h` in the target sources.
- One intentional test-expectation flip (`testFailedWriteDoesNotQueue`).
- Touching six repository headers is the largest single diff in Part A.
- `QToolButton` menu with disabled actions must not emit warnings under `ui_smoke`.
- A Wayland session launch may fall back to xcb.

## Verification and report

- `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` after each part, before committing.
- `docs/reports/M2.md` updated in commit 2; local commits only, never push.
- Final report in AGENTS.md §10 format: What changed / How to run / How verified (commands and
  results) / Deviations and assumptions / Suggested next step, listing each M2 acceptance
  criterion (add, edit, delete, duplicate, sort and filter, status change appears in Timeline,
  CSV export) with how it was verified.
