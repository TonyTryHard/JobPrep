M2 plan: approved with these binding amendments.
1. First, read Database.h/.cpp and quote the lines showing depth counting vs SAVEPOINT. State which one is true.
2. Joined transactions: a failed or rolled-back joined transaction marks the outermost one as failed (its commit() returns false and rolls back everything). A failed write never queues changed().
3. Deferred changed(): one shared helper for all repos (base class or Database method), QPointer not raw QObject*, once per repo per flush. Tests: 0 emissions before outer commit, 1 after, 0 after rollback, plus the failure case from item 2.
4. Next step: one InterviewRepository::all() grouped by application id, no per-application queries. Upcoming = pending && start >= now. Injectable clock; deterministic tests. Keep selection (by id) across reloads.
5. Layering: models/services never include ui/. The model exposes a StatusRole (enum); the delegate paints label/color via StatusStyle; JobsPage builds CSV labels. WorkMode/InterviewType labels live on the UI side. Verify with: grep -rn "ui/" src/models src/services (must be empty).
6. JobsPage owns the model and proxy. AppContext owns services only (ExportService).
7. CSV: no id column, no duplicate Updated column; header order documented in code and pinned by a test.
8. Search via QString::contains(..., Qt::CaseInsensitive); never user text in a QRegularExpression. Include a Cyrillic test.
9. Dialog: inline validation messages; confirm discard on Cancel when edited; Timeline empty/disabled for a new application.
10. Delegate: drawRoundedRect + QFontMetrics from the style option, no per-paint allocations; check contrast in both themes.
11. Three local commits, never push: (a) nested-tx fix + tests, (b) models + services + tests, (c) UI + ui_smoke + CMake.
12. Keep the final report short, AGENTS.md section 10 format, including "Suggested next step".
