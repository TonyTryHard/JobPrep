# Milestone 6 Part D1 Report: Part C Carry-overs, Keyboard Shortcuts, Empty States, Tab Order

**Environment:**
- OS: Linux tony-laptop 7.0.0-38-generic #38-Ubuntu SMP PREEMPT_DYNAMIC Fri Sep 4 09:10:14 UTC 2026 x86_64 GNU/Linux
- Compiler: g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0
- Build tool: cmake version 4.2.3, ninja 1.13.2
- Qt: Qt version 6.10.2 in /usr/lib/x86_64-linux-gnu
- Git HEAD commit: `5f3d44a` (`docs(spec): document shortcuts, empty state inventory, and tab order in SPEC §14`)
- Test suite total: **33 binaries, 445 functions** (baseline after Part C: 31 binaries, 401 functions)
- Test runtime: `Total Test time (real) = 46.76 sec`

---

## 1. Plan vs. Result

### Plan
1. **C0 Carry-overs:** Address all items from C0:
   - C0.1: Verify backup completeness across all SQLite schema columns, dynamic `PRAGMA table_info` verification, comprehensive non-default roundtrip test, correct errata report `docs/reports/M6-partC-errata.md`.
   - C0.2: Provide missing evidence from Part C (ICS `cat -A`, JSON backup head, `.gitattributes`, function lists).
   - C0.3: Execute clean proxy build with `-Wshadow -Wconversion -Wsign-conversion`.
   - C0.4: Safe quit during active restore worker without dangling threads or leaked transactions.
   - C0.5: Demonstrate maximize facts with before/after comparison and widget size numbers.
   - C0.6: Provide analysis paragraph on typed goal `7.30` without changing code behavior.
2. **D1 Keyboard Shortcuts:**
   - Implement Ctrl+1..4, Ctrl+, (working globally and inside line/text edits).
   - Implement Ctrl+N per current active page (Home -> app, Study -> topic, Jobs -> app, Calendar -> interview, Settings -> unregistered).
   - Implement Ctrl+F for Study and Jobs.
   - Implement Ctrl+S for Study detail panel and dialog Save buttons.
   - Esc routing across dialogs, detail panel, search edits, quick-add, and toasts.
   - Handle and test all shortcut collisions.
3. **D2 Empty States Inventory:**
   - Comprehensive inventory of all 15 visual lists/tables/panels.
   - Distinguish empty from filtered-empty states.
   - Regenerate and verify distinct screenshots in light and dark themes.
4. **D3 Tab Order:**
   - Documented tab order sequences for all dialogs and pages.
   - Implement visible focus rings via tokens in theme stylesheet.
   - Automated tests asserting complete focus traversal.

### Result
- All tasks in C0, D1, D2, and D3 were completed.
- Zero warnings under `-Wall -Wextra -Wpedantic -Werror` on dev-linux.
- All 33 test binaries passed (100% green gate).
- Zero push was performed (local commits only).

---

## 2. C0. Part C Carry-overs

### C0.1 Backup Completeness
Schema inspection verified from `src/data/Migrations.cpp`:
```
$ grep -n "CREATE TABLE" src/data/Migrations.cpp
27:                      u"CREATE TABLE tracks (\n"
32:                      u"CREATE TABLE topics (\n"
41:                      u"CREATE TABLE subtasks (\n"
47:                      u"CREATE TABLE study_sessions (\n"
53:                      u"CREATE TABLE applications (\n"
63:                      u"CREATE TABLE status_history (\n"
71:                      u"CREATE TABLE interviews (\n"
80:                      u"CREATE TABLE reminders_sent (\n"
```

Full definition of every table from `src/data/Migrations.cpp`:
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

CREATE TABLE reminders_sent (key TEXT PRIMARY KEY, sent_at TEXT NOT NULL);
```

Migrations that alter tables:
```
$ grep -n "ALTER TABLE\|user_version" src/data/Migrations.cpp
(no output)
```
*(Migration version 1 is the sole migration; no ALTER TABLE statements exist).*

Real schema columns for all tables from `PRAGMA table_info`:
```
=== Table: tracks (5 columns) ===
  cid=0: id (INTEGER) notnull=0 dflt=None pk=1
  cid=1: name (TEXT) notnull=1 dflt=None pk=0
  cid=2: color (TEXT) notnull=1 dflt=None pk=0
  cid=3: icon (TEXT) notnull=1 dflt='' pk=0
  cid=4: position (INTEGER) notnull=1 dflt=0 pk=0
=== Table: topics (12 columns) ===
  cid=0: id (INTEGER) notnull=0 dflt=None pk=1
  cid=1: track_id (INTEGER) notnull=1 dflt=None pk=0
  cid=2: title (TEXT) notnull=1 dflt=None pk=0
  cid=3: status (TEXT) notnull=1 dflt='backlog' pk=0
  cid=4: priority (TEXT) notnull=1 dflt='normal' pk=0
  cid=5: target_date (TEXT) notnull=0 dflt=None pk=0
  cid=6: notes (TEXT) notnull=1 dflt='' pk=0
  cid=7: resources (TEXT) notnull=1 dflt='[]' pk=0
  cid=8: tags (TEXT) notnull=1 dflt='' pk=0
  cid=9: position (INTEGER) notnull=1 dflt=0 pk=0
  cid=10: created_at (TEXT) notnull=1 dflt=None pk=0
  cid=11: updated_at (TEXT) notnull=1 dflt=None pk=0
=== Table: subtasks (5 columns) ===
  cid=0: id (INTEGER) notnull=0 dflt=None pk=1
  cid=1: topic_id (INTEGER) notnull=1 dflt=None pk=0
  cid=2: text (TEXT) notnull=1 dflt=None pk=0
  cid=3: done (INTEGER) notnull=1 dflt=0 pk=0
  cid=4: position (INTEGER) notnull=1 dflt=0 pk=0
=== Table: study_sessions (5 columns) ===
  cid=0: id (INTEGER) notnull=0 dflt=None pk=1
  cid=1: topic_id (INTEGER) notnull=0 dflt=None pk=0
  cid=2: session_date (TEXT) notnull=1 dflt=None pk=0
  cid=3: minutes (INTEGER) notnull=1 dflt=None pk=0
  cid=4: note (TEXT) notnull=1 dflt='' pk=0
=== Table: applications (20 columns) ===
  cid=0: id (INTEGER) notnull=0 dflt=None pk=1
  cid=1: company (TEXT) notnull=1 dflt=None pk=0
  cid=2: position (TEXT) notnull=1 dflt=None pk=0
  cid=3: url (TEXT) notnull=1 dflt='' pk=0
  cid=4: source (TEXT) notnull=1 dflt='' pk=0
  cid=5: resume_version (TEXT) notnull=1 dflt='' pk=0
  cid=6: location (TEXT) notnull=1 dflt='' pk=0
  cid=7: work_mode (TEXT) notnull=1 dflt='remote' pk=0
  cid=8: salary_min (INTEGER) notnull=0 dflt=None pk=0
  cid=9: salary_max (INTEGER) notnull=0 dflt=None pk=0
  cid=10: currency (TEXT) notnull=1 dflt='USD' pk=0
  cid=11: status (TEXT) notnull=1 dflt='applied' pk=0
  cid=12: applied_date (TEXT) notnull=0 dflt=None pk=0
  cid=13: next_action (TEXT) notnull=1 dflt='' pk=0
  cid=14: next_action_date (TEXT) notnull=0 dflt=None pk=0
  cid=15: contact_name (TEXT) notnull=1 dflt='' pk=0
  cid=16: contact_email (TEXT) notnull=1 dflt='' pk=0
  cid=17: notes (TEXT) notnull=1 dflt='' pk=0
  cid=18: created_at (TEXT) notnull=1 dflt=None pk=0
  cid=19: updated_at (TEXT) notnull=1 dflt=None pk=0
=== Table: status_history (6 columns) ===
  cid=0: id (INTEGER) notnull=0 dflt=None pk=1
  cid=1: application_id (INTEGER) notnull=1 dflt=None pk=0
  cid=2: from_status (TEXT) notnull=0 dflt=None pk=0
  cid=3: to_status (TEXT) notnull=1 dflt=None pk=0
  cid=4: changed_at (TEXT) notnull=1 dflt=None pk=0
  cid=5: note (TEXT) notnull=1 dflt='' pk=0
=== Table: interviews (9 columns) ===
  cid=0: id (INTEGER) notnull=0 dflt=None pk=1
  cid=1: application_id (INTEGER) notnull=1 dflt=None pk=0
  cid=2: start_at (TEXT) notnull=1 dflt=None pk=0
  cid=3: duration_min (INTEGER) notnull=1 dflt=60 pk=0
  cid=4: type (TEXT) notnull=1 dflt='technical' pk=0
  cid=5: place (TEXT) notnull=1 dflt='' pk=0
  cid=6: interviewer (TEXT) notnull=1 dflt='' pk=0
  cid=7: notes (TEXT) notnull=1 dflt='' pk=0
  cid=8: outcome (TEXT) notnull=1 dflt='pending' pk=0
=== Table: reminders_sent (2 columns) ===
  cid=0: key (TEXT) notnull=0 dflt=None pk=1
  cid=1: sent_at (TEXT) notnull=1 dflt=None pk=0
```

Summary Table (Real Schema vs JSON Backup vs Restore INSERT):
| Table | Schema Columns | Exported JSON Keys | Restore INSERT Columns | Match? |
|---|---|---|---|---|
| tracks | 5 columns | 5 keys | 5 columns | Yes |
| topics | 12 columns | 12 keys | 12 columns | Yes |
| subtasks | 5 columns | 5 keys | 5 columns | Yes |
| study_sessions | 5 columns | 5 keys | 5 columns | Yes |
| applications | 20 columns | 20 keys | 20 columns | Yes |
| status_history | 6 columns | 6 keys | 6 columns | Yes |
| interviews | 9 columns | 9 keys | 9 columns | Yes |

Column-by-Column Comparison for `applications` Table:
| Schema Column | JSON Backup Key (`ExportService`) | Restore `INSERT` Column (`RestoreService`) | Match? |
|---|---|---|---|
| `id` | `"id"` | `id` | Yes |
| `company` | `"company"` | `company` | Yes |
| `position` | `"position"` | `position` | Yes |
| `url` | `"url"` | `url` | Yes |
| `source` | `"source"` | `source` | Yes |
| `resume_version` | `"resume_version"` | `resume_version` | Yes |
| `location` | `"location"` | `location` | Yes |
| `work_mode` | `"work_mode"` | `work_mode` | Yes |
| `salary_min` | `"salary_min"` | `salary_min` | Yes |
| `salary_max` | `"salary_max"` | `salary_max` | Yes |
| `currency` | `"currency"` | `currency` | Yes |
| `status` | `"status"` | `status` | Yes |
| `applied_date` | `"applied_date"` | `applied_date` | Yes |
| `next_action` | `"next_action"` | `next_action` | Yes |
| `next_action_date` | `"next_action_date"` | `next_action_date` | Yes |
| `contact_name` | `"contact_name"` | `contact_name` | Yes |
| `contact_email` | `"contact_email"` | `contact_email` | Yes |
| `notes` | `"notes"` | `notes` | Yes |
| `created_at` | `"created_at"` | `created_at` | Yes |
| `updated_at` | `"updated_at"` | `updated_at` | Yes |

Schema-Driven Comparison Helper (`dumpDatabase` & `compareDumps` in `tests/services/tst_ImportExport.cpp`):
```cpp
QList<tst_ImportExport::TableDump> tst_ImportExport::dumpDatabase(Database& db) {
    const QStringList tables = {
        u"tracks"_s,
        u"topics"_s,
        u"subtasks"_s,
        u"study_sessions"_s,
        u"applications"_s,
        u"status_history"_s,
        u"interviews"_s
    };
    QList<TableDump> dumps;
    for (const auto& table : tables) {
        TableDump td;
        td.tableName = table;
        QSqlQuery q(db.connection());
        q.exec(u"SELECT * FROM %1 ORDER BY id ASC;"_s.arg(table));
        const auto record = q.record();
        for (int i = 0; i < record.count(); ++i) {
            td.columnNames.append(record.fieldName(i));
        }
        while (q.next()) {
            QList<QVariant> row;
            for (int i = 0; i < record.count(); ++i) {
                row.append(q.value(i));
            }
            td.rows.append(row);
        }
        dumps.append(td);
    }
    return dumps;
}

bool tst_ImportExport::compareDumps(const QList<TableDump>& d1, const QList<TableDump>& d2, QString* diff) {
    if (d1.size() != d2.size()) {
        if (diff) *diff = u"Table count mismatch: %1 vs %2"_s.arg(d1.size()).arg(d2.size());
        return false;
    }
    for (int i = 0; i < d1.size(); ++i) {
        const auto& t1 = d1[i];
        const auto& t2 = d2[i];
        if (t1.tableName != t2.tableName) {
            if (diff) *diff = u"Table name mismatch at %1: %2 vs %3"_s.arg(i).arg(t1.tableName, t2.tableName);
            return false;
        }
        if (t1.columnNames != t2.columnNames) {
            if (diff) *diff = u"Columns mismatch in table %1"_s.arg(t1.tableName);
            return false;
        }
        if (t1.rows.size() != t2.rows.size()) {
            if (diff) *diff = u"Row count mismatch in table %1: %2 vs %3"_s.arg(t1.tableName).arg(t1.rows.size()).arg(t2.rows.size());
            return false;
        }
        for (int r = 0; r < t1.rows.size(); ++r) {
            const auto& r1 = t1.rows[r];
            const auto& r2 = t2.rows[r];
            for (int c = 0; c < r1.size(); ++c) {
                const auto& v1 = r1[c];
                const auto& v2 = r2[c];
                if (v1.isNull() != v2.isNull()) {
                    if (diff) *diff = u"Nullness mismatch in %1 row %2 col %3"_s.arg(t1.tableName).arg(r).arg(t1.columnNames[c]);
                    return false;
                }
                if (!v1.isNull() && v1 != v2) {
                    if (diff) *diff = u"Value mismatch in %1 row %2 col %3: '%4' vs '%5'"_s
                        .arg(t1.tableName).arg(r).arg(t1.columnNames[c], v1.toString(), v2.toString());
                    return false;
                }
            }
        }
    }
    return true;
}
```
*(The comparison executes dynamically across all schema columns via `SELECT *` and `record.fieldName(i)` without hardcoded column lists).*

Automated regression coverage added in `tests/services/tst_ImportExport.cpp`:
- `testBackupCoversEveryColumn`: reads `PRAGMA table_info` dynamically and asserts every column is present in the exported JSON record.
- `testRoundTripEveryColumnNonDefault`: seeds non-default values in every column of every table, exports to JSON, wipes SQLite database, restores from JSON, and verifies equality column by column via schema-driven helpers.

Errata report `docs/reports/M6-partC-errata.md` published and committed in `5ad0390`.

---

### C0.2 Evidence Missing from Part C

**`tst_ImportExport -functions` (raw):**
```
$ ./build/dev-linux/tests/services/tst_ImportExport -functions
testIcsCrlfOnly()
testIcsFolding200CharsAndUtf8Boundaries()
testIcsEscapingSpecialCharacters()
testIcsValarmAndProperties()
testIcsCancelledInterviewsExcluded()
testIcsEmptyListProducesValidCalendar()
testJsonBackupStructureAndHeader()
testJsonBackupEveryColumnPresent()
testBackupCoversEveryColumn()
testJsonBackupAtomicWriteAndReadOnlyError()
testRestoreEqualityHandBuiltDataset()
testRestoreEqualitySeedDataset()
testRoundTripEveryColumnNonDefault()
testRestoreInvalidFormatLeavesDbUntouched()
testRestoreInvalidVersionLeavesDbUntouched()
testRestoreTruncatedJsonLeavesDbUntouched()
testRestoreEmptyFileLeavesDbUntouched()
testRestoreBadEnumLeavesDbUntouched()
testRestoreBrokenReferenceLeavesDbUntouched()
testRestoreDuplicateIdLeavesDbUntouched()
testRestoreOversizedFileLeavesDbUntouched()
testRestoreSafetyBackupCreatedPrunedAndAbortsWhenReadOnly()
testRestoreSafetyBackupProductionPathWithCustomXdgDataHome()
testRestoreRemindersSentWiped()
testRestoreRemindersPausedDuringRestore()
testRestoreIdAllocationAfterRestore()
testRestoreAsyncEventLoopAlive()
testQuitOrDestructionDuringRestoreWorker()
testRestorePerformanceSanity()
testRestoreCoalescedRefresh()
testRestoreStudyPanelWithVanishedTopic()
```

**`tst_PageSettings -functions | grep -E "Data|GoalSpin|CloseToTray|Maximize|FirstDay"` (raw):**
```
$ ./build/dev-linux/tests/ui/tst_PageSettings -functions | grep -E "Data|GoalSpin|CloseToTray|Maximize|FirstDay"
testWeeklyGoalSpinBox()
testGoalSpinBoxTypedEight()
testGoalSpinBoxTypedDecimals()
testGoalSpinBoxTypedSeparators()
testGoalSpinBoxTypedFractionRounding()
testGoalSpinBoxTypedHoursSuffix()
testGoalSpinBoxTypedHoursAndMinutes()
testGoalSpinBoxTypedMinutesSuffix()
testGoalSpinBoxClamping()
testGoalSpinBoxInvalidText()
testGoalSpinBoxPageSteps()
testCloseToTrayUnavailable()
testCloseToTrayAvailableAndHide()
testCloseToTrayRestore()
testCloseToTrayQuitSkipsCloseEventAndFlushesEdits()
testCloseToTrayLiveProvider()
testCloseToTrayFlushOnHide()
testCloseToTrayActivationReasonsAndRestore()
testCloseToTrayFirstTimeHintOnlyWhenRequested()
testCloseToTraySessionSaving()
testFirstDayOfWeekComboAndNormalization()
testFirstDayOfWeekSettingsToDashboard()
testDataCardStructureAndControls()
testDataExportCsvWithSeams()
testDataExportIcsWithSeams()
testDataBackupJsonWithSeams()
testDataRestoreJsonWithSeams()
testDataLoadSampleDataTwiceNoDuplicates()
testDataOpenFolderWithSeams()
testDataCardScreenshots()
```

**Restore Performance Test (`testRestorePerformanceSanity`):**
```
$ ./build/dev-linux/tests/services/tst_ImportExport testRestorePerformanceSanity
********* Start testing of tst_ImportExport *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_ImportExport::initTestCase()
QINFO  : tst_ImportExport::testRestorePerformanceSanity() Restore performance sanity: 134 ms for 1,000 applications and 5,000 sessions
PASS   : tst_ImportExport::testRestorePerformanceSanity()
PASS   : tst_ImportExport::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 201ms
********* Finished testing of tst_ImportExport *********
```

**Real ICS Export (`cat -A jobprep-interviews-2026-10-28.ics`):**
```
$ cat -A jobprep-interviews-2026-10-28.ics
BEGIN:VCALENDAR^M$
VERSION:2.0^M$
PRODID:-//JobPrep//JobPrep Desktop//EN^M$
CALSCALE:GREGORIAN^M$
BEGIN:VEVENT^M$
UID:jobprep-interview-7@jobprep.local^M$
DTSTAMP:20261028T105448Z^M$
DTSTART:20261011T190000^M$
DTEND:20261011T200000^M$
SUMMARY:Interview: asdf M-bM-^@M-^T Technical^M$
DESCRIPTION:Position: ccc^M$
BEGIN:VALARM^M$
ACTION:DISPLAY^M$
DESCRIPTION:Interview: asdf M-bM-^@M-^T Technical^M$
TRIGGER:-PT60M^M$
END:VALARM^M$
END:VEVENT^M$
...
END:VCALENDAR^M$
```
*Note on `SUMMARY` line:* The line `SUMMARY:Interview: asdf M-bM-^@M-^T Technical` contains `M-bM-^@M-^T`, which is the exact UTF-8 byte sequence (`0xE2 0x80 0x94`) for the em-dash `—`. This matches SPEC §10 (`Interview: <company> — <type>`) precisely. All line endings are explicit CRLF (`^M$`).

**First 15 lines of real JSON backup:**
```
$ head -n 15 jobprep-backup-2026-10-28.json
{
    "applications": [
        {
            "applied_date": "2026-10-11",
            "company": "asdf",
            "contact_email": "",
            "contact_name": "",
            "created_at": "1970-01-01T03:00:00",
            "currency": "",
            "id": 1,
            "location": "",
            "next_action": "",
            "next_action_date": null,
            "notes": "",
            "position": "ccc",
```

**`.gitattributes` Content:**
```
$ cat .gitattributes
# Auto detect text files and normalize line endings; enforce LF in working trees
* text=auto eol=lf

# Enforce LF on Linux/Unix scripts and sources
*.sh text eol=lf
*.cpp text eol=lf
*.h text eol=lf
*.cmake text eol=lf
CMakeLists.txt text eol=lf
CMakePresets.json text eol=lf
*.qss text eol=lf
*.qrc text eol=lf
*.json text eol=lf
*.md text eol=lf

# Enforce CRLF on Windows PowerShell scripts and batch files
*.ps1 text eol=crlf
*.bat text eol=crlf
*.cmd text eol=crlf

# Binary files
*.png binary
*.jpg binary
*.ico binary
*.woff2 binary
*.ttf binary
*.otf binary
```

**Status of the nine C0.5 items:**
1. `prepareForShutdown` stops debounce timer and flushes edits: Corrected in `M6-partB-errata.md` §1 and implemented.
2. First-day-of-week logic in `SettingsService::toDayOfWeek`: Corrected in `M6-partB-errata.md` §2.
3. Preferences key sequence (`Ctrl+,`) fallback on Linux: Corrected in `M6-partB-errata.md` §3.
4. Unreported files in commit `e2730a5`: Corrected in `M6-partB-errata.md` §4.
5. Proxy build proof with `-Wshadow -Wconversion -Wsign-conversion`: Provided in `M6-partB-errata.md` §5 and re-verified in Part D1.
6. `CTEST_PARALLEL_LEVEL` default in scripts: Corrected in `M6-partB-errata.md` §6.
7. `formatLeadTime` assertion slot: Corrected in `M6-partB-errata.md` §7.
8. Section heading reference to "Part A": Clarified in `M6-partB-errata.md` §8.
9. Application schema discrepancies: Fully documented and resolved in `docs/reports/M6-partC-errata.md`.

---

### C0.3 Proxy Build with Shadow and Conversion Flags

Executed full scratch build in isolated directory `/tmp/build-proxy-d1` with flags:
`-Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wsign-conversion`

```
$ cmake -B /tmp/build-proxy-d1 -S . -G Ninja -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wsign-conversion"
$ cmake --build /tmp/build-proxy-d1 -j2
[218/222] Linking CXX executable tests/ui/tst_PageStudy
[219/222] Linking CXX executable tests/ui/tst_PageHome
[220/222] Linking CXX executable tests/ui/tst_PageSettings
[221/222] Linking CXX executable tests/ui/tst_Shortcuts
[222/222] Linking CXX executable tests/ui/tst_EmptyStatesAndFocus
$ cmake --build /tmp/build-proxy-d1; echo "exit=$?"
ninja: no work to do.
exit=0
```
All 222 build targets compiled cleanly with zero compiler warnings.

---

### C0.4 Quit During Restore Safety

- In `src/services/RestoreService.cpp`, `RestoreService::waitForRestore(int timeoutMs)` was implemented to pump `QCoreApplication::processEvents()` synchronously until the restore worker thread terminates.
- In `src/ui/MainWindow.cpp`, `MainWindow::prepareForShutdown()` calls `RestoreService::waitForRestore(5000)` before closing database connections or stopping services.
- `DataToolsController::restoreJson` guards against re-entrancy and concurrent restore operations.
- Test `testQuitOrDestructionDuringRestoreWorker` and `testRestoreRemindersPausedDuringRestore` in `tst_ImportExport` verify:
  1. No `QThread: Destroyed while thread is still running` crash occurs upon immediate destruction of `RestoreService`.
  2. The SQLite database remains intact and consistent (single transaction atomicity).
  3. `ReminderService` is safely resumed without remaining in a paused state.

---

### C0.5 Maximize Fix Facts & Verification

**Test body (`tests/ui_smoke/tst_UiSmoke.cpp`):**
```cpp
void tst_UiSmoke::testWindowMaximumSizeAndMaximize() {
    QVERIFY(m_window != nullptr);
    QVERIFY(m_window->isVisible());

    // Check maximumSize across all pages (C0.7: no maximum width/height reaches top window)
    for (int i = 0; i < m_window->pageStack()->count(); ++i) {
        m_window->sidebar()->setCurrentPage(i);
        emit m_window->sidebar()->pageSelected(i);
        QCoreApplication::processEvents();
        QCOMPARE(m_window->maximumSize(), QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    }

    const QRect prevGeo = m_window->geometry();
    m_window->showMaximized();
    QVERIFY(m_window->isMaximized());

    m_window->showNormal();
    QVERIFY(!m_window->isMaximized());
    QCOMPARE(m_window->geometry(), prevGeo);
}
```

**Before/After Demonstration (with and without `Qt::AlignVCenter`):**
Both runs output identical numbers and both pass:
```
window->maximumSize(): 16777215x16777215
windowHandle()->maximumSize(): 16777215x16777215
centralWidget->maximumSize(): 16777215x16777215
centralWidget->layout()->totalMaximumSize(): 524287x524287
page 0 JobPrep::Ui::Pages::HomePage maximumSize: 16777215x16777215, layout totalMaximumSize: 524287x524287
page 1 JobPrep::Ui::Pages::StudyPage maximumSize: 16777215x16777215, layout totalMaximumSize: 524287x524287
page 2 JobPrep::Ui::Pages::JobsPage maximumSize: 16777215x16777215, layout totalMaximumSize: 524287x524287
page 3 JobPrep::Ui::Pages::CalendarPage maximumSize: 16777215x16777215, layout totalMaximumSize: 524287x524287
page 4 JobPrep::Ui::Pages::SettingsPage maximumSize: 16777215x16777215, layout totalMaximumSize: 524287x524287
Study toolbar layout totalMaximumSize: 524287x34
Jobs toolbar layout totalMaximumSize: 524287x34
```
**Conclusion:**
In Qt, `QLayout::totalMaximumSize()` on child layouts does not propagate upward to constrain the top-level window's `maximumSize()` unless `QLayout::SetFixedSize` or a explicit window size constraint is set. Therefore, `testWindowMaximumSizeAndMaximize` passes identically with or without `Qt::AlignVCenter`. The test does not prove window manager behavior on a real desktop; any window maximize restrictions observed on Linux are compositor- or Wayland-specific (the user will test under `QT_QPA_PLATFORM=xcb`).

---

### C0.6 Typed Goal `7.30` Analysis

In existing code, `GoalSpinBox` treats typed input as fractional decimal hours: `7.30` is interpreted as `7.3` hours ($7 \times 60 + 0.3 \times 60 = 438$ minutes), which rounds to the nearest 15-minute step ($435$ minutes), displaying `7 h 15 min`.

**Two implementation options for user consideration:**
1. **Option A (Preserve decimal hours):** Keep existing decimal hours parsing (`7.5` = `7 h 30 min`, `7.25` = `7 h 15 min`, `7.30` = `7.3 h` = `7 h 15 min`).
   - *Cost:* 0 lines of code; consistent with standard mathematical parsing; intuitive for inputs like `7.5`.
   - *Downside:* Users typing `7.30` meaning "7 hours 30 minutes" are rounded down to `7 h 15 min`.
2. **Option B (Contextual time parsing):** Check if user typed a period or colon followed by 2 digits: treat `HH.MM` with 2 fraction digits as hours and minutes (`7.30` $\to$ 7 h 30 min).
   - *Cost:* ~15 lines in `GoalSpinBox::valueFromText()`.
   - *Downside:* Semantic ambiguity on values like `7.50` (is it 7 h 50 min, or 7.5 hours = 7 h 30 min?).
*Per prompt instructions, behavior was NOT modified in this milestone; user decision is requested.*

---

## 3. D1. Keyboard Shortcuts

### Existing UI Shortcuts & Key Handlers (Raw Grep)
```
$ grep -rn "QShortcut\|QKeySequence\|setShortcut\|keyPressEvent\|Key_" src/ui
src/ui/Toast.cpp:144:void Toast::keyPressEvent(QKeyEvent* event) {
src/ui/Toast.cpp:145:    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
src/ui/Toast.cpp:151:    if (event->key() == Qt::Key_Escape) {
src/ui/Toast.cpp:156:    QWidget::keyPressEvent(event);
src/ui/widgets/MonthView.h:74:    void keyPressEvent(QKeyEvent* event) override;
src/ui/widgets/TopicBoardColumn.cpp:220:            if (keyEvent->key() == Qt::Key_Escape) {
src/ui/widgets/TopicBoardColumn.cpp:237:            case Qt::Key_Left:
src/ui/widgets/TopicBoardColumn.cpp:244:            case Qt::Key_Right:
src/ui/widgets/TopicBoardColumn.cpp:251:            case Qt::Key_Up:
src/ui/widgets/TopicBoardColumn.cpp:257:            case Qt::Key_Down:
src/ui/widgets/TopicBoardColumn.cpp:263:            case Qt::Key_Delete:
src/ui/widgets/TopicDetailPanel.h:78:    void keyPressEvent(QKeyEvent* event) override;
src/ui/widgets/TopicDetailPanel.cpp:19:#include <QShortcut>
src/ui/widgets/TopicDetailPanel.cpp:309:    auto* saveShortcut = new QShortcut(QKeySequence::Save, this);
src/ui/widgets/TopicDetailPanel.cpp:311:    connect(saveShortcut, &QShortcut::activated, this, [this]() { flush(); });
src/ui/widgets/TopicDetailPanel.cpp:432:            (keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace)) {
src/ui/widgets/TopicDetailPanel.cpp:438:        if (keyEvent->key() == Qt::Key_Escape) {
src/ui/widgets/TopicDetailPanel.cpp:444:        if (watched == m_checkList && keyEvent->key() == Qt::Key_Space &&
src/ui/widgets/TopicDetailPanel.cpp:455:            (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down)) {
src/ui/widgets/TopicDetailPanel.cpp:456:            moveChecklistItem(keyEvent->key() == Qt::Key_Up ? -1 : 1);
src/ui/widgets/TopicDetailPanel.cpp:459:        if (watched == m_checkList && keyEvent->key() == Qt::Key_Delete) {
src/ui/widgets/TopicDetailPanel.cpp:474:void TopicDetailPanel::keyPressEvent(QKeyEvent* event) {
src/ui/widgets/TopicDetailPanel.cpp:475:    if (event->key() == Qt::Key_Escape && m_topicId) {
src/ui/widgets/TopicDetailPanel.cpp:481:    QWidget::keyPressEvent(event);
src/ui/widgets/MonthView.cpp:180:void MonthView::keyPressEvent(QKeyEvent* event) {
src/ui/widgets/MonthView.cpp:182:        case Qt::Key_Left:
src/ui/widgets/MonthView.cpp:185:        case Qt::Key_Right:
src/ui/widgets/MonthView.cpp:188:        case Qt::Key_Up:
src/ui/widgets/MonthView.cpp:191:        case Qt::Key_Down:
src/ui/widgets/MonthView.cpp:194:        case Qt::Key_PageUp:
src/ui/widgets/MonthView.cpp:197:        case Qt::Key_PageDown:
src/ui/widgets/MonthView.cpp:200:        case Qt::Key_Home:
src/ui/widgets/MonthView.cpp:203:        case Qt::Key_End:
src/ui/widgets/MonthView.cpp:206:        case Qt::Key_Return:
src/ui/widgets/MonthView.cpp:207:        case Qt::Key_Enter:
src/ui/widgets/MonthView.cpp:208:        case Qt::Key_Space:
src/ui/widgets/MonthView.cpp:214:    QWidget::keyPressEvent(event);
src/ui/MainWindow.h:11:class QShortcut;
src/ui/MainWindow.h:141:    QShortcut* m_newShortcut{nullptr};
src/ui/MainWindow.cpp:6:#include <QKeySequence>
src/ui/MainWindow.cpp:13:#include <QShortcut>
src/ui/MainWindow.cpp:215:        if (keyEvent->key() == Qt::Key_Escape && !m_searchEdit->text().isEmpty()) {
src/ui/MainWindow.cpp:344:        auto* shortcut = new QShortcut(QKeySequence(Qt::CTRL | (Qt::Key_1 + i)), this);
src/ui/MainWindow.cpp:345:        connect(shortcut, &QShortcut::activated, this, [this, i]() {
src/ui/MainWindow.cpp:351:    auto* prefsShortcut = new QShortcut(QKeySequence::Preferences, this);
src/ui/MainWindow.cpp:352:    connect(prefsShortcut, &QShortcut::activated, this, [this]() {
src/ui/MainWindow.cpp:357:    // On Linux/X11, QKeySequence::Preferences maps to Qt::Key_Settings rather than
src/ui/MainWindow.cpp:359:    auto* ctrlCommaShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma), this);
src/ui/MainWindow.cpp:360:    connect(ctrlCommaShortcut, &QShortcut::activated, this, [this]() {
src/ui/MainWindow.cpp:366:    auto* findShortcut = new QShortcut(QKeySequence::Find, this);
src/ui/MainWindow.cpp:367:    connect(findShortcut, &QShortcut::activated, this, &MainWindow::focusCurrentPageSearch);
src/ui/MainWindow.cpp:369:    m_newShortcut = new QShortcut(QKeySequence::New, this);
src/ui/MainWindow.cpp:370:    connect(m_newShortcut, &QShortcut::activated, this, &MainWindow::triggerCurrentPageNew);
src/ui/dialogs/InterviewDialog.cpp:13:#include <QShortcut>
src/ui/dialogs/InterviewDialog.cpp:175:    auto* saveShortcut = new QShortcut(QKeySequence::Save, this);
src/ui/dialogs/InterviewDialog.cpp:176:    connect(saveShortcut, &QShortcut::activated, this, &InterviewDialog::onSave);
src/ui/dialogs/ApplicationDialog.cpp:15:#include <QShortcut>
src/ui/dialogs/ApplicationDialog.cpp:103:    auto* saveShortcut = new QShortcut(QKeySequence::Save, this);
src/ui/dialogs/ApplicationDialog.cpp:104:    connect(saveShortcut, &QShortcut::activated, this, &ApplicationDialog::onSave);
src/ui/dialogs/FollowUpDialog.cpp:10:#include <QShortcut>
src/ui/dialogs/FollowUpDialog.cpp:91:    auto* saveShortcut = new QShortcut(QKeySequence::Save, this);
src/ui/dialogs/FollowUpDialog.cpp:92:    connect(saveShortcut, &QShortcut::activated, this, &FollowUpDialog::accept);
src/ui/dialogs/TrackDialog.cpp:16:#include <QShortcut>
src/ui/dialogs/TrackDialog.cpp:189:    auto* saveShortcut = new QShortcut(QKeySequence::Save, this);
src/ui/dialogs/TrackDialog.cpp:190:    connect(saveShortcut, &QShortcut::activated, this, &TrackDialog::onSaveClicked);
src/ui/dialogs/TrackDialog.cpp:254:            if (keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Up) {
src/ui/dialogs/TrackDialog.cpp:259:            } else if (keyEvent->key() == Qt::Key_Right || keyEvent->key() == Qt::Key_Down) {
src/ui/dialogs/SessionDialog.cpp:10:#include <QShortcut>
src/ui/dialogs/SessionDialog.cpp:95:    auto* saveShortcut = new QShortcut(QKeySequence::Save, this);
src/ui/dialogs/SessionDialog.cpp:96:    connect(saveShortcut, &QShortcut::activated, this, &SessionDialog::onSave);
src/ui/pages/StudyPage.cpp:379:        if (key->key() == Qt::Key_Escape && !m_searchEdit->text().isEmpty()) {
src/ui/pages/StudyPage.cpp:386:        if (keyEvent->key() == Qt::Key_Delete) {
src/ui/pages/StudyPage.cpp:391:        } else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
src/ui/pages/SettingsPage.cpp:555:        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
src/ui/pages/JobsPage.cpp:281:        if (key->key() == Qt::Key_Escape && !m_searchEdit->text().isEmpty()) {
src/ui/pages/JobsPage.cpp:289:        if (id.has_value() && key->key() == Qt::Key_Return && key->modifiers() == Qt::NoModifier) {
src/ui/pages/JobsPage.cpp:293:        if (id.has_value() && key->key() == Qt::Key_Delete) {
src/ui/PageBase.h:20:    /// Moves focus into the page's own search box (QKeySequence::Find). Pages without a
src/ui/PageBase.h:24:    /// Opens the page's primary "add" flow (QKeySequence::New).
src/ui/Toast.h:36:    void keyPressEvent(QKeyEvent* event) override;
```

### Shortcuts Table (SPEC §3.7 & Implementation)

| Shortcut | Scope | Action | Test Slot |
|---|---|---|---|
| `Ctrl+1` | Global Window | Navigate to Home page | `tst_Shortcuts::testPageShortcutsCtrl1To4` |
| `Ctrl+2` | Global Window | Navigate to Study page | `tst_Shortcuts::testPageShortcutsCtrl1To4` |
| `Ctrl+3` | Global Window | Navigate to Jobs page | `tst_Shortcuts::testPageShortcutsCtrl1To4` |
| `Ctrl+4` | Global Window | Navigate to Calendar page | `tst_Shortcuts::testPageShortcutsCtrl1To4` |
| `Ctrl+,` | Global Window | Navigate to Settings page | `tst_Shortcuts::testPreferencesShortcutFromAllPages` |
| `Ctrl+N` | Home Page | Open `ApplicationDialog` | `tst_Shortcuts::testCtrlNPerCurrentPage` |
| `Ctrl+N` | Study Page | Focus Backlog quick-add line (`newTopic`) | `tst_Shortcuts::testCtrlNPerCurrentPage` |
| `Ctrl+N` | Jobs Page | Open `ApplicationDialog` | `tst_Shortcuts::testCtrlNPerCurrentPage` |
| `Ctrl+N` | Calendar Page | Open `InterviewDialog` | `tst_Shortcuts::testCtrlNPerCurrentPage` |
| `Ctrl+N` | Settings Page | Unregistered / no-op | `tst_Shortcuts::testCtrlNPerCurrentPage` |
| `Ctrl+F` | Study Page | Focus & select search box | `tst_Shortcuts::testCtrlFPerCurrentPage` |
| `Ctrl+F` | Jobs Page | Focus & select search box | `tst_Shortcuts::testCtrlFPerCurrentPage` |
| `Ctrl+F` | Other Pages | No-op (does not switch focus) | `tst_Shortcuts::testCtrlFPerCurrentPage` |
| `Ctrl+S` | Study Detail | Immediate auto-flush to DB without focus loss | `tst_Shortcuts::testCtrlSSavesInStudyDetailPanel` |
| `Ctrl+S` | `ApplicationDialog` | Triggers Save button (`onSave`) | `tst_Shortcuts::testCtrlSSavesInDialogs` |
| `Ctrl+S` | `InterviewDialog` | Triggers Save button (`onSave`) | `tst_Shortcuts::testCtrlSSavesInDialogs` |
| `Ctrl+S` | `FollowUpDialog` | Triggers Save button (`accept`) | `tst_Shortcuts::testCtrlSSavesInDialogs` |
| `Ctrl+S` | `SessionDialog` | Triggers Save button (`onSave`) | `tst_Shortcuts::testCtrlSSavesInDialogs` |
| `Ctrl+S` | `TrackDialog` | Triggers Save button (`onSaveClicked`) | `tst_Shortcuts::testCtrlSSavesInDialogs` |
| `Esc` | Search edits | Clear text while retaining edit focus | `tst_Shortcuts::testEscHandlingAcrossWidgets` |
| `Esc` | Quick-add line | Clear text while retaining edit focus | `tst_Shortcuts::testEscHandlingAcrossWidgets` |
| `Esc` | Study Detail | Flushes pending edits and collapses panel | `tst_Shortcuts::testEscHandlingAcrossWidgets` |
| `Esc` | Modal Dialogs | Rejects dialog (confirms if dirty changes exist) | `tst_Shortcuts::testEscHandlingAcrossWidgets` |
| `Esc` | Toast | Dismisses toast only when focused | `tst_Shortcuts::testEscHandlingAcrossWidgets` |

### Collision Resolutions

| Collision Scenario | Rule / Resolution | Test Slot |
|---|---|---|
| `Alt+Arrows` on Board | Handled in `TopicBoardColumn` to reorder cards or switch columns; consumed so window menus ignore it | `tst_Shortcuts::testCollisionAltArrowsOnBoard` |
| `Ctrl+F` on Study page | Study page event filter and PageBase intercept `Ctrl+F` to focus page-local search, ignoring header edit | `tst_Shortcuts::testCtrlFPerCurrentPage` |
| `Ctrl+,` / `Ctrl+1..4` in modal dialog | Dialog event loop captures all input; global window shortcuts are inactive and cannot switch page behind dialog | `tst_Shortcuts::testCollisionModalBlocksWindowShortcuts` |
| `Ctrl+S` inside `QTextEdit` | Event filter / shortcut saves data and handles key event so `\x13` control character is not inserted | `tst_Shortcuts::testCollisionCtrlSInsideTextEdit` |
| `Ctrl+N` inside `QLineEdit` | Shortcut invokes page creation flow without inserting `n` into the focused text edit | `tst_Shortcuts::testCollisionCtrlNInsideLineEdit` |

---

## 4. D2. Empty States Complete Inventory

| Container / View | Empty Text Present? | Primary Action | Filtered-Empty Text | Action in Filtered State | Test Slot |
|---|---|---|---|---|---|
| Dashboard (overall) | Yes ("No Data Yet") | "+ Application", "+ Topic" | N/A (unfiltered) | N/A | `testEmptyDashboardAsAWhole` |
| Dashboard Today list | Yes ("No interviews or tasks scheduled for today.") | None | N/A | N/A | `testEmptyDashboardTodayAndUpcoming` |
| Dashboard Upcoming list | Yes ("No upcoming interviews in next 7 days.") | None | N/A | N/A | `testEmptyDashboardTodayAndUpcoming` |
| Dashboard Funnel | Yes ("No active applications to show in pipeline.") | None | N/A | N/A | `testEmptyDashboardFunnelAndStudyProgress` |
| Dashboard Study progress | Yes ("No tracks defined. Add tracks in Study page to track progress.") | None | N/A | N/A | `testEmptyDashboardFunnelAndStudyProgress` |
| Study Page (0 tracks) | Yes ("No Tracks Defined") | "+ Add Track" | N/A | N/A | `testEmptyStudyPageNoTracks` |
| Study Column (0 topics) | Yes ("No topics") | "+ Add topic to <track>" | "No topics match filter" | Clear filter | `testEmptyStudyPageNoTopicsInColumnAndList` |
| Study List view (0 topics) | Yes ("No topics in current track") | None | "No topics match filter" | Clear filter | `testEmptyStudyPageNoTopicsInColumnAndList` |
| Study Page (Filtered-empty) | N/A | N/A | "No Matches" | "Clear filters" | `testEmptyStudyPageFilteredEmpty` |
| Topic Checklist (0 items) | Yes ("No checklist items.") | "+ Add item" edit | N/A | N/A | `testEmptyTopicChecklistAndResources` |
| Topic Resources (0 items) | Yes ("No resources added.") | "+ Add" button | N/A | N/A | `testEmptyTopicChecklistAndResources` |
| Jobs Page (0 applications) | Yes ("No Applications") | "+ New Application" | N/A | N/A | `testEmptyJobsPageNoApplications` |
| Jobs Page (Filtered-empty) | N/A | N/A | "No Matches" | "Clear filters" | `testEmptyJobsPageFilteredEmpty` |
| ApplicationDialog Interviews | Yes ("No interviews scheduled") | "+ Add Interview" | N/A | N/A | `testEmptyApplicationDialogInterviewsTab` |
| ApplicationDialog Timeline | Yes ("No status transitions recorded") | None | N/A | N/A | `testEmptyApplicationDialogTimelineTab` |
| Calendar Agenda (no day) | Yes ("Select a date to view agenda") | None | N/A | N/A | `testEmptyCalendarAgendaNoDaySelected` |
| Calendar Agenda (empty day) | Yes ("No events for this date") | "+ Add Interview" | N/A | N/A | `testEmptyCalendarAgendaSelectedEmptyDay` |
| TrackDialog (0 tracks) | Yes ("No tracks") | None | N/A | N/A | `testEmptyTrackDialogNoTracks` |

### Empty vs. Filtered-Empty Rule
When records exist in SQLite but are hidden by active filters (search keywords or status chips), views display "No Matches" with a primary "Clear filters" action button. Initial onboarding messaging ("No applications yet", "No study topics") is strictly reserved for databases containing zero records.

### Empty States Screenshot Generation (`scripts/screenshots.sh`)
```
$ ./scripts/screenshots.sh
==> Building screenshot test binaries...
ninja: no work to do.
==> Running screenshot generation with JOBPREP_REPORT_IMG_DIR=/home/tony/projects/JobPrep/docs/reports/img...
********* Start testing of tst_PageCalendar *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_PageCalendar::initTestCase()
PASS   : tst_PageCalendar::testCalendarScreenshots()
PASS   : tst_PageCalendar::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 4602ms
********* Finished testing of tst_PageCalendar *********
********* Start testing of tst_PageStudy *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_PageStudy::initTestCase()
PASS   : tst_PageStudy::testStudyScreenshots()
PASS   : tst_PageStudy::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 17198ms
********* Finished testing of tst_PageStudy *********
********* Start testing of tst_PageHome *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_PageHome::initTestCase()
PASS   : tst_PageHome::testDashboardScreenshots()
PASS   : tst_PageHome::testToastScreenshots()
PASS   : tst_PageHome::cleanupTestCase()
Totals: 4 passed, 0 failed, 0 skipped, 0 blacklisted, 12245ms
********* Finished testing of tst_PageHome *********
********* Start testing of tst_PageSettings *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : tst_PageSettings::initTestCase()
PASS   : tst_PageSettings::testSettingsScreenshots()
PASS   : tst_PageSettings::testDataCardScreenshots()
PASS   : tst_PageSettings::cleanupTestCase()
Totals: 4 passed, 0 failed, 0 skipped, 0 blacklisted, 232ms
********* Finished testing of tst_PageSettings *********
==> Screenshots successfully regenerated in /home/tony/projects/JobPrep/docs/reports/img!
```

### Screenshot Hashes (`md5sum docs/reports/img/*empty*.png`):
```
$ md5sum docs/reports/img/*empty*.png
4117446c6c502c64a80b33acb817c301  docs/reports/img/calendar-empty-dark.png
4a9b743964a04e292962a2c533cbf8d9  docs/reports/img/calendar-empty-light.png
357a12e67de9f292d336a55652e023ed  docs/reports/img/dashboard-empty-dark.png
75b1798cfdd5c3802970e4919765780b  docs/reports/img/dashboard-empty-light.png
044bebd90706a1999df9ae13ff6c099a  docs/reports/img/study-board-empty-dark.png
cb9ff5ada5bd30a31378060db846e3fd  docs/reports/img/study-board-empty-light.png
c4473c3730f221e613ad6aad27d8b3ae  docs/reports/img/study-empty-tracks-dark.png
cba38a3bdf3e16ac8497dfe154235738  docs/reports/img/study-empty-tracks-light.png
```
All checksums are distinct across views and theme modes.

---

## 5. D3. Tab Order

### Documented Tab Order Sequences

1. **`ApplicationDialog` (Overview Tab):**
   `FieldCompany` $\to$ `FieldPosition` $\to$ `FieldStatus` $\to$ `FieldAppliedDate` $\to$ `FieldNextAction` $\to$ `FieldNextActionDate` $\to$ `FieldWorkMode` $\to$ `FieldLocation` $\to$ `FieldUrl` $\to$ `FieldResumeVersion` $\to$ `FieldSource` $\to$ `FieldSalaryMin` $\to$ `FieldSalaryMax` $\to$ `FieldCurrency` $\to$ `FieldContactName` $\to$ `FieldContactEmail` $\to$ `DialogCancelBtn` $\to$ `DialogSaveBtn` $\to$ (loops to `FieldCompany`).
2. **`InterviewDialog`:**
   `InterviewRound` $\to$ `InterviewType` $\to$ `InterviewDate` $\to$ `InterviewTime` $\to$ `InterviewPlace` $\to$ `InterviewOutcome` $\to$ `InterviewNotes` $\to$ `DialogCancelBtn` $\to$ `DialogSaveBtn` $\to$ (loops to `InterviewRound`).
3. **`FollowUpDialog`:**
   `FollowUpActionEdit` $\to$ `FollowUpDateEdit` $\to$ `DialogCancelBtn` $\to$ `DialogSaveBtn` $\to$ (loops to `FollowUpActionEdit`).
4. **`SessionDialog`:**
   `SessionTopicCombo` $\to$ `SessionDateEdit` $\to$ `SessionDurationSpin` $\to$ `SessionNoteEdit` $\to$ `DialogCancelBtn` $\to$ `DialogSaveBtn` $\to$ (loops to `SessionTopicCombo`).
5. **`TrackDialog`:**
   `TrackNameEdit` $\to$ `TrackColorPalette` $\to$ `TrackDeleteBtn` $\to$ `TrackCloseBtn` $\to$ (loops to `TrackNameEdit`).
6. **`HomePage`:**
   `HeaderSearchEdit` $\to$ `HeaderNewBtn` $\to$ `DashboardEmptyActionBtn` $\to$ `DashboardEmptySecondaryActionBtn`.
7. **`JobsPage`:**
   `JobsSearchEdit` $\to$ `JobsStatusChips` $\to$ `JobsNewBtn` $\to$ `JobsTableView`.
8. **`CalendarPage`:**
   `CalendarPrevBtn` $\to$ `CalendarNextBtn` $\to$ `CalendarTodayBtn` $\to$ `CalendarLayerInterviewBtn` $\to$ `CalendarLayerTargetBtn` $\to$ `CalendarMonthView` $\to$ `CalendarNewInterviewBtn` $\to$ `CalendarAgendaList`.
9. **`SettingsPage`:**
   `SettingsThemeModeCombo` $\to$ `SettingsAccentPalette` $\to$ `SettingsFirstDayCombo` $\to$ `SettingsGoalSpinBox` $\to$ `SettingsCloseToTrayCheck` $\to$ `SettingsExportCsvBtn` $\to$ `SettingsExportIcsBtn` $\to$ `SettingsBackupJsonBtn` $\to$ `SettingsRestoreJsonBtn` $\to$ `SettingsLoadSampleBtn` $\to$ `SettingsOpenFolderBtn`.

### Visible Focus Indicator
`resources/styles/theme.qss` defines explicit `:focus` states:
```css
QPushButton:focus, QToolButton:focus, QLineEdit:focus, QTextEdit:focus, QComboBox:focus {
    border: 2px solid @accent;
    outline: none;
}
```
Verified via automated pixel difference tests `testVisibleFocusIndicatorPushButton`, `testVisibleFocusIndicatorLineEdit`, and `testVisibleFocusIndicatorChip` in `tst_EmptyStatesAndFocus`.

---

## 6. Verification Commands & Outputs

### Layering Check
```
$ grep -rnE "StatusStyle|Theme::|Tokens|ui/|QtGui|QIcon|QPixmap|QColor|QPainter|QFont|QtWidgets|QSystemTrayIcon|QMessageBox|QFileDialog" src/domain src/data src/services src/models
src/services/SettingsService.cpp:51:    return m_settings->value(u"ui/sidebarCollapsed"_s, false).toBool();
src/services/SettingsService.cpp:56:    m_settings->setValue(u"ui/sidebarCollapsed"_s, collapsed);
src/services/SettingsService.cpp:61:    return m_settings->value(u"ui/themeMode"_s, u"system"_s).toString();
src/services/SettingsService.cpp:66:    m_settings->setValue(u"ui/themeMode"_s, mode);
src/services/SettingsService.cpp:71:    return m_settings->value(u"ui/accentPreset"_s, u"indigo"_s).toString();
src/services/SettingsService.cpp:76:    m_settings->setValue(u"ui/accentPreset"_s, preset);
src/services/INotifier.h:18:/// Presentation text is built exclusively by notifiers in ui/.
src/models/ApplicationTableModel.cpp:105:            // The label lives in StatusStyle, which the status badge delegate paints;
src/models/TopicListModel.h:20:/// are handled by delegates in ui/.
```
*(No layering violations: matches are QSettings path strings and comments).*

### No Hardcoded Colors in UI
```
$ grep -rnE "#[0-9a-fA-F]{3,8}|rgb\(|rgba\(" src/ui | grep -v "^src/ui/theme/"
(no output)
```

### No Fixed Sizes in UI
```
$ grep -rnE "setFixedSize|setFixedWidth|setFixedHeight" src/ui
src/ui/widgets/TopicDetailPanel.cpp:90:    m_closeBtn->setFixedSize(28, 28);
src/ui/dialogs/TrackDialog.cpp:134:        btn->setFixedSize(32, 32);
src/ui/dialogs/TrackDialog.cpp:149:    m_customSwatchBtn->setFixedSize(32, 32);
src/ui/Sidebar.cpp:26:        setFixedHeight(42);
src/ui/Sidebar.cpp:173:    m_toggleBtn->setFixedHeight(36);
src/ui/Sidebar.cpp:189:    setFixedWidth(collapsed ? kWidthCollapsed : kWidthExpanded);
```

### Git Status
```
$ git status --short
?? docs/prompts/M6-ci-fixup2.md
```

### Git Log
```
$ git log --oneline -8
5f3d44a docs(spec): document shortcuts, empty state inventory, and tab order in SPEC §14
366a1fc test(ui): add test suites for shortcuts, empty states, and tab order (D1, D2, D3)
66e74c5 feat(ui): enforce accessible names and tab order chains with visible focus indicator (D3)
3756e99 feat(ui): complete empty state inventory and distinguish filtered-empty (D2)
09d5bf5 feat(ui): implement keyboard shortcuts and key event routing (D1)
cde261f chore(git): enforce LF line endings on Windows working tree in .gitattributes
5ad0390 docs(spec): document C0 carry-overs in spec and errata report
d963272 fix(services): backup column completeness, quit-during-restore safety, and warning fixes
```

### Git Show Stat of Each Code Commit
```
$ git show --stat d963272 cde261f 09d5bf5 3756e99 66e74c5 366a1fc
commit d963272f3fae7c559293f075a69fcb37884d526b
Author: Tony <group.ik31@gmail.com>
Date:   Wed Oct 28 12:50:21 2026 +0200

    fix(services): backup column completeness, quit-during-restore safety, and warning fixes

 src/services/ExportService.cpp      |   4 +-
 src/services/RestoreService.cpp     |  76 ++++++++++++----
 src/services/RestoreService.h       |  18 ++++
 src/ui/DataToolsController.cpp      |   9 ++
 src/ui/DataToolsController.h        |   1 +
 src/ui/MainWindow.cpp               |   1 +
 tests/services/tst_ImportExport.cpp | 175 ++++++++++++++++++++++++++++++++++++
 tests/ui/tst_PageSettings.cpp       |   4 +-
 8 files changed, 269 insertions(+), 19 deletions(-)

commit cde261ff0bdb870b513d06a82b44459fce1c3004
Author: Tony <group.ik31@gmail.com>
Date:   Thu Oct 29 17:21:50 2026 +0200

    chore(git): enforce LF line endings on Windows working tree in .gitattributes

 .gitattributes | 4 ++--
 1 file changed, 2 insertions(+), 2 deletions(-)

commit 09d5bf5bd2e854d76a0a335762467106c80c7988
Author: Tony <group.ik31@gmail.com>
Date:   Thu Oct 29 17:22:11 2026 +0200

    feat(ui): implement keyboard shortcuts and key event routing (D1)

 src/ui/MainWindow.cpp                | 88 +++++++++++++++++++++++++++++++----
 src/ui/MainWindow.h                  | 10 ++++
 src/ui/dialogs/ApplicationDialog.cpp | 89 ++++++++++++++++++++++++++++++++++--
 src/ui/dialogs/ApplicationDialog.h   |  9 ++++
 src/ui/dialogs/FollowUpDialog.cpp    | 10 +++-
 src/ui/dialogs/InterviewDialog.cpp   | 20 +++++++-
 src/ui/dialogs/SessionDialog.cpp     | 12 +++++
 src/ui/pages/CalendarPage.cpp        | 28 ++++++++++--
 src/ui/pages/CalendarPage.h          |  2 +
 src/ui/pages/HomePage.cpp            | 30 ++++++++----
 src/ui/pages/HomePage.h              |  6 ++-
 src/ui/pages/StudyPage.cpp           | 15 +++++-
 12 files changed, 288 insertions(+), 31 deletions(-)

commit 3756e9937fa5be9d633d5cd61a38e76c912eb0e1
Author: Tony <group.ik31@gmail.com>
Date:   Thu Oct 29 17:22:18 2026 +0200

    feat(ui): complete empty state inventory and distinguish filtered-empty (D2)

 src/ui/dialogs/TrackDialog.cpp | 14 +++++++++++++-
 src/ui/pages/JobsPage.cpp      | 18 ++++++++++++++----
 src/ui/widgets/EmptyState.cpp  |  2 ++
 src/ui/widgets/MonthView.cpp   |  8 +++++++-
 4 files changed, 36 insertions(+), 6 deletions(-)

commit 66e74c5d2c6a0966b0097511ea604385f9287dc6
Author: Tony <group.ik31@gmail.com>
Date:   Thu Oct 29 17:22:22 2026 +0200

    feat(ui): enforce accessible names and tab order chains with visible focus indicator (D3)

 resources/styles/theme.qss | 10 ++++++++++
 src/ui/Sidebar.cpp         |  1 +
 2 files changed, 11 insertions(+)

commit 366a1fcca94a2211636c67c7d34a6f27eeb1a6e7
Author: Tony <group.ik31@gmail.com>
Date:   Thu Oct 29 17:22:27 2026 +0200

    test(ui): add test suites for shortcuts, empty states, and tab order (D1, D2, D3)

 docs/reports/img/dashboard-toast-dark.png         |  Bin 89937 -> 87254 bytes
 docs/reports/img/dashboard-toast-light.png        |  Bin 88363 -> 85703 bytes
 docs/reports/img/dashboard-toasts-stack-dark.png  |  Bin 80653 -> 89511 bytes
 docs/reports/img/dashboard-toasts-stack-light.png |  Bin 79075 -> 88095 bytes
 docs/reports/img/dashboard-with-data-dark.png     |  Bin 69245 -> 65763 bytes
 docs/reports/img/dashboard-with-data-light.png    |  Bin 68108 -> 64568 bytes
 docs/reports/img/study-board-dark.png             |  Bin 68539 -> 68932 bytes
 docs/reports/img/study-board-detail-dark.png      |  Bin 83111 -> 83539 bytes
 docs/reports/img/study-board-detail-light.png     |  Bin 82095 -> 82460 bytes
 docs/reports/img/study-board-drag-dark.png        |  Bin 93117 -> 93527 bytes
 docs/reports/img/study-board-drag-light.png       |  Bin 90661 -> 91031 bytes
 docs/reports/img/study-board-empty-dark.png       |  Bin 27385 -> 27797 bytes
 docs/reports/img/study-board-empty-light.png      |  Bin 27103 -> 27464 bytes
 docs/reports/img/study-board-light.png            |  Bin 67683 -> 68028 bytes
 docs/reports/img/study-board-selected-dark.png    |  Bin 48802 -> 49203 bytes
 docs/reports/img/study-board-selected-light.png   |  Bin 48087 -> 48437 bytes
 docs/reports/img/study-empty-tracks-dark.png      |  Bin 24293 -> 24737 bytes
 docs/reports/img/study-empty-tracks-light.png     |  Bin 24079 -> 24442 bytes
 docs/reports/img/study-list-dark.png              |  Bin 154070 -> 154448 bytes
 docs/reports/img/study-list-light.png             |  Bin 153304 -> 153633 bytes
 docs/reports/img/study-session-dialog-dark.png    |  Bin 15234 -> 15212 bytes
 docs/reports/img/study-session-dialog-light.png   |  Bin 15102 -> 15083 bytes
 docs/reports/img/study-tracks-dialog-dark.png     |  Bin 15252 -> 15396 bytes
 docs/reports/img/study-tracks-dialog-light.png    |  Bin 14896 -> 15021 bytes
 tests/services/tst_ImportExport.cpp               |   34 +
 tests/ui/CMakeLists.txt                           |    2 +
 tests/ui/tst_EmptyStatesAndFocus.cpp              | 1045 +++++++++++++++++++++
 tests/ui/tst_PageStudy.cpp                        |   47 +-
 tests/ui/tst_Shortcuts.cpp                        |  630 +++++++++++++
 tests/ui_smoke/tst_UiSmoke.cpp                    |    7 +
 tests/ui_test_helpers/UiTestHelpers.h             |    9 +
 31 files changed, 1769 insertions(+), 5 deletions(-)
```

### Qt APIs Introduced
All code adheres to the Qt 6.8+ baseline floor. No `QString + QStringView` concatenation operators were used anywhere in the codebase.
The following Qt APIs were utilized or introduced in Part D1:
- `QShortcut`: constructor `QShortcut(QKeySequence, QWidget*)`, signal `activated()` (standard Qt 5/6 baseline).
- `QKeySequence::StandardKey`: `QKeySequence::New`, `QKeySequence::Find`, `QKeySequence::Save` (standard Qt 5/6 baseline).
- `QWidget::setTabOrder(QWidget*, QWidget*)`: explicit focus chain link configuration (standard Qt 5/6 baseline).
- `QWidget::setAccessibleName(const QString&)` and `QWidget::accessibleName()`: accessibility naming for icon-only buttons (standard Qt 5/6 baseline).
- `QWidget::focusNextChild()`: programmatic focus chain traversal for automated tab order validation (standard Qt 5/6 baseline).
- `QWidget::setFocusPolicy(Qt::FocusPolicy)`: setting focus traversal policies (standard Qt 5/6 baseline).
- `QKeyEvent`: constructor `QKeyEvent(QEvent::Type, int, Qt::KeyboardModifiers, const QString&)` used for synthesized key event testing (standard Qt 5/6 baseline).
*(No post-Qt 6.8 APIs or experimental symbols were introduced).*

### Final Check Suite Execution
```
$ CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
==> Configuring JobPrep with dev-linux preset...
-- Configuring done (0.2s)
-- Generating done (0.2s)
-- Build files have been written to: /home/tony/projects/JobPrep/build/dev-linux
==> Building JobPrep with dev-linux preset...
ninja: no work to do.
==> Running tests with dev-linux preset...
Test project /home/tony/projects/JobPrep/build/dev-linux
      Start 29: tst_PageStudy
      Start 30: tst_PageHome
 1/33 Test #30: tst_PageHome .....................   Passed   17.24 sec
      Start 27: tst_PageJobs
 2/33 Test #29: tst_PageStudy ....................   Passed   20.18 sec
      Start 33: tst_EmptyStatesAndFocus
 3/33 Test #33: tst_EmptyStatesAndFocus ..........   Passed    8.02 sec
      Start 28: tst_PageCalendar
 4/33 Test #27: tst_PageJobs .....................   Passed   12.52 sec
      Start 26: tst_StudyRegression
 5/33 Test #28: tst_PageCalendar .................   Passed    7.40 sec
      Start 31: tst_PageSettings
 6/33 Test #26: tst_StudyRegression ..............   Passed    7.29 sec
      Start 32: tst_Shortcuts
 7/33 Test #31: tst_PageSettings .................   Passed    4.03 sec
      Start 25: tst_Notifiers
 8/33 Test #32: tst_Shortcuts ....................   Passed    3.08 sec
      Start 24: tst_StatsService
 9/33 Test #25: tst_Notifiers ....................   Passed    2.70 sec
      Start 21: tst_ImportExport
10/33 Test #24: tst_StatsService .................   Passed    2.24 sec
      Start 10: ui_smoke
11/33 Test #21: tst_ImportExport .................   Passed    0.79 sec
      Start  8: tst_SeedService
12/33 Test  #8: tst_SeedService ..................   Passed    0.86 sec
      Start 22: tst_ReminderService
13/33 Test #22: tst_ReminderService ..............   Passed    0.12 sec
      Start 15: tst_TopicListModel
14/33 Test #15: tst_TopicListModel ...............   Passed    0.22 sec
      Start 17: tst_TopicTableProxy
15/33 Test #17: tst_TopicTableProxy ..............   Passed    0.12 sec
      Start  3: tst_TopicRepository
16/33 Test  #3: tst_TopicRepository ..............   Passed    0.20 sec
      Start 18: tst_AgendaService
17/33 Test #18: tst_AgendaService ................   Passed    0.15 sec
      Start 16: tst_TopicBoardProxy
18/33 Test #16: tst_TopicBoardProxy ..............   Passed    0.11 sec
      Start  5: tst_ApplicationRepository
19/33 Test  #5: tst_ApplicationRepository ........   Passed    0.16 sec
      Start 11: tst_MonthView
20/33 Test #11: tst_MonthView ....................   Passed    0.08 sec
      Start 23: tst_SettingsService
21/33 Test #23: tst_SettingsService ..............   Passed    0.02 sec
      Start 13: tst_ApplicationTableModel
22/33 Test #13: tst_ApplicationTableModel ........   Passed    0.07 sec
      Start 12: tst_AgendaListModel
23/33 Test #12: tst_AgendaListModel ..............   Passed    0.03 sec
      Start 19: tst_DayWatcher
24/33 Test #19: tst_DayWatcher ...................   Passed    0.01 sec
      Start  6: tst_InterviewRepository
25/33 Test  #6: tst_InterviewRepository ..........   Passed    0.04 sec
      Start  1: tst_Database
26/33 Test  #1: tst_Database .....................   Passed    0.03 sec
      Start  7: tst_ReminderLogRepository
27/33 Test  #7: tst_ReminderLogRepository ........   Passed    0.02 sec
      Start  4: tst_SessionRepository
28/33 Test  #4: tst_SessionRepository ............   Passed    0.03 sec
      Start  9: tst_TransactionBehavior
29/33 Test  #9: tst_TransactionBehavior ..........   Passed    0.03 sec
      Start 14: tst_ApplicationFilterProxy
30/33 Test #14: tst_ApplicationFilterProxy .......   Passed    0.02 sec
      Start  2: tst_TrackRepository
31/33 Test  #2: tst_TrackRepository ..............   Passed    0.03 sec
      Start 20: tst_ExportService
32/33 Test #20: tst_ExportService ................   Passed    0.01 sec
33/33 Test #10: ui_smoke .........................   Passed    4.39 sec

100% tests passed, 0 tests failed out of 33

Label Time Summary:
data       =   1.39 sec*proc (9 tests)
model      =   0.58 sec*proc (6 tests)
service    =   3.34 sec*proc (7 tests)
ui         =  86.92 sec*proc (11 tests)

Total Test time (real) =  46.76 sec
==> Quality check PASSED with zero warnings and all tests green!
```

---

## 7. Not Verified by Agent, User to Check

1. **Ukrainian Keyboard Layout:** Ctrl+N, Ctrl+F, and Ctrl+S shortcuts under a non-Latin / Ukrainian keyboard layout. Tests ran exclusively under standard offscreen default key mappings.
2. **Real Window Maximize on Wayland & XCB:** Check window maximize and unmaximize behavior interactively under both Wayland and `QT_QPA_PLATFORM=xcb ./build/dev-linux/jobprep`.
3. **ICS Calendar Import:** Import exported `.ics` file into external calendar software (Google Calendar, Apple Calendar, Thunderbird, Outlook) to verify calendar display and 60-minute alarm triggers.
4. **Restore on Real Production Data:** Execute full JSON backup and restore on real user data via the Settings UI.
5. **Visible Focus Ring Look:** Inspect the 2px accent focus ring on the physical desktop monitor in both Light and Dark modes.
6. **Screen Reader Accessibility:** Test screen reader (e.g. Orca on Linux or NVDA/Narrator on Windows) announcements of icon-only buttons with accessible names.
7. **Windows Execution:** Cross-platform verification under native MSVC 2022 / Windows 10/11 environment.
