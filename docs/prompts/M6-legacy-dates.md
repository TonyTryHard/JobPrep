# M6: legacy 1970 timestamps: show "—" for dates before 2013 (display only, no data change)

Run this **after** `M6-ci-fixup5.md` (and `M6-partD2-fixups.md` if not yet done). Read `AGENTS.md` (§2, §9, §12), `SPEC.md` (data model, date formatting), `src/data/DbFormat.*`, `src/models/ApplicationTableModel.cpp`, `src/ui/pages/JobsPage.cpp`, `src/services/ExportService.cpp`.
Local commits only, never push. Same evidence rules as always: raw output of real commands with the command above it, nothing trimmed, empty output as `(no output)`, hashes only from `git log --oneline`. Separate commits: (1) code, (2) tests, (3) SPEC, (4) report last. Never run the app binary without isolated `HOME`/`XDG_*`.

## Decision (user, 2026-11-03)

Before the `DbFormat::now()` fix (M6 D1) new rows were stored with timestamp 1970-01-01. Existing databases still contain such rows. The user decided: **in the UI, any timestamp earlier than 2013-01-01 is shown as "—" (em dash). The stored data is not modified, no migration, no repair.**

## Tasks

1. **Evidence.** `git log --oneline -8`. List every place where a stored timestamp or date is turned into text or shown to the user: `grep -rn "createdAt\|updatedAt\|appliedAt\|fromMSecsSinceEpoch\|fromSecsSinceEpoch\|toString(" src | cut -c1-160`. Make a short table: place, which field, user-entered date (applied date, interview date) or system timestamp (created/updated). **Only system timestamps and legacy-looking values are affected; do not hide a date the user typed on purpose** (an application date or interview date earlier than 2013 stays visible; say in the table how you tell them apart from the code).

2. **One rule, one place (code, commit 1).** Add a single function (for example `DbFormat::isPlausibleDate(const QDateTime&)` or a display helper `formatStoredTimestamp(...)`, next to the existing formatting code) with the cutoff as one named constant (`2013-01-01`, UTC). Use it in every place from the table that shows a system timestamp (table columns, tooltips, details, "last updated" labels). Sorting must keep working on the real stored value (the "—" is display only). No `#ifdef`, no fixed pixel sizes. Export/backup/ICS must keep writing the real stored values unchanged (say so and show it with a test or a grep of the export code).

3. **Tests (commit 2).** Slots that prove: (a) a row with timestamp 1970 shows "—" in the model's display role, (b) a row with a timestamp in 2013 or later shows the formatted date, (c) the boundary (2012-12-31T23:59:59Z gives "—", 2013-01-01T00:00:00Z shows the date), (d) sorting a column that contains legacy and real values is still by the stored value, (e) JSON backup and restore of a legacy row keeps the 1970 value (equality of stored data). Run each new slot by its real name (`-functions`), raw.

4. **SPEC (commit 3).** One short paragraph in the section about dates: the cutoff, that it is display-only, and that legacy rows are not repaired.

5. **Local gate.** `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` full raw output; totals by command (binaries and functions; arithmetic from `-functions` per binary, no fitting); `git status --short` empty; `git log --oneline` of this task.

## Report `docs/reports/M6-legacy-dates.md` (short)

Evidence for tasks 1 to 5. "Not verified by agent, user to check" must contain: how an old database from the user's real data looks in the UI (user opens it with his own data, **not** the agent; the agent never touches real data), and that Windows behaves the same (next CI run). Do not tag and do not push.
