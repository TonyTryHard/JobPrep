# Errata Report: Milestone 6 Part C (Data & Backups)

This errata document records and corrects documentation discrepancies identified in `docs/reports/M6-partC.md` §5 regarding the SQLite schema and backup/restore column mapping.

---

## 1. Discrepancy in `docs/reports/M6-partC.md` §5

In the summary table of `docs/reports/M6-partC.md` §5 ("JSON Backup & Restore Verification"), the column list for the `applications` table was transcribed with three incorrect column names:
- Listed `target_date`, `salary_currency`, `contact_phone`.
- These columns did not exist in the `applications` table schema in `src/data/Migrations.cpp`.

## 2. Verified Real Schema from `src/data/Migrations.cpp`

The canonical SQLite schema defined in migration v1 (`src/data/Migrations.cpp`) specifies exactly 20 columns for `applications`:

```sql
CREATE TABLE applications (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    company TEXT NOT NULL,
    position TEXT NOT NULL,
    url TEXT,
    source TEXT,
    resume_version TEXT,
    location TEXT,
    work_mode TEXT,
    salary_min INTEGER,
    salary_max INTEGER,
    currency TEXT,
    status TEXT NOT NULL,
    applied_date TEXT NOT NULL,
    next_action TEXT,
    next_action_date TEXT,
    contact_name TEXT,
    contact_email TEXT,
    notes TEXT,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);
```

The three real column names in place of the erroneous ones are:
1. `work_mode` (in place of `target_date`)
2. `currency` (in place of `salary_currency`)
3. `next_action` (in place of `contact_phone`)

Note: `contact_phone` does not exist in JobPrep domain or schema; `next_action` and `next_action_date` capture follow-up actions; and `work_mode` captures remote/hybrid/onsite settings.

## 3. Implementation Code Alignment

Inspection of `src/services/ExportService.cpp` and `src/services/RestoreService.cpp` confirms:
- Both services have always matched `Migrations.cpp` exactly.
- All 20 canonical columns were already correctly serialized into JSON during backup export and deserialized/inserted during restore.
- The discrepancy was strictly in the human narrative report table of `docs/reports/M6-partC.md` §5, not in the production or test code.

## 4. Automated Regression Guards

To ensure schema, backup export, and restore serialization remain synchronized across future migrations and refactorings without human drift:
- Added `testBackupCoversEveryColumn()` in `tests/services/tst_ImportExport.cpp`. This test queries `PRAGMA table_info` directly from SQLite for all domain tables and verifies dynamically that every database column is present as a key in the exported JSON object.
- Added `testRoundTripEveryColumnNonDefault()` in `tests/services/tst_ImportExport.cpp`. This test populates a comprehensive fixture exercising non-default values across all 20 columns of `applications` and all columns of `tracks`, `topics`, `subtasks`, `resources`, `study_sessions`, `application_status_history`, and `interviews` (including non-ASCII / Ukrainian text: `"Компанія 'Технології майбутнього'"`). It executes a full backup -> wipe -> restore cycle and asserts strict value equality for every column.

## 5. Specification Updates

`docs/SPEC.md` §14 has been updated with entry `1.15 — Data & Backups completeness, safe quit, and warning fixes (C0)` documenting this correction, the safe-quit synchronization, and compiler warning resolutions.
