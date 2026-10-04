M2 part 2. A first M2 run produced models, ExportService and tests, but the Jobs page UI is NOT done. Read @AGENTS.md, @docs/SPEC.md (§3.3, §7.4, §12 M2), @docs/plans/M2-plan.md and @docs/prompts/M2-amendments.md. Plan briefly, wait for approval, then work in two parts. Build with at most 2 jobs (CMAKE_BUILD_PARALLEL_LEVEL=2).

Part A (small fixes; commit after ./scripts/check.sh passes). Verify each against the code, fix if missing, and report what you found:
1. Database::Transaction (Database.cpp ~lines 210-250):
   a. A joined (non-owner) Transaction that is destroyed without commit() or calls rollback() must mark the outermost transaction as failed. Add an "outer failed" flag on Database. The owner's commit() then rolls back everything, clears pending notifications, resets the flag and returns false.
   b. A joined rollback must NOT call clearPendingChanges(). Today it discards notifications for earlier joined writes that still get committed.
   Tests: (i) a joined failure -> outer commit() returns false, earlier joined writes are gone, zero changed(); (ii) all joined writes succeed -> exactly one changed() per touched repo after the outer commit; (iii) failure after a successful joined write (e.g. UPDATE then failing INSERT) leaves nothing behind.
2. Pending set: QPointer, no raw QObject* (QPointer is already included but unused). Make transaction() and deferChanged() non-const; remove the const_cast and mutable. Use one shared commit-and-notify helper for all repositories.
3. ApplicationTableModel gets an injectable clock; deterministic tests: past pending interview ignored, future pending wins, cancelled/passed/failed ignored, fallback to next action.
4. ApplicationTableModel: delete the status label switch (data(), lines ~93-102). Expose StatusRole only; the delegate gets label and color from StatusStyle. Column header tr() strings stay.
5. A test pins the exact CSV header order.

Part B (the actual M2 UI; commit after check.sh passes):
JobsPage wired with model + proxy (owned by the page) + StatusBadgeDelegate, toolbar (search, ChipBar chips, Export CSV, + Application), default sort Updated desc, context menu (Edit, Change status, Add interview… disabled, Open URL http/https only, Duplicate, Delete with confirm), Enter opens edit, Delete key deletes with confirm, footer with counts per status, two empty states (no data / no matches), CSV export via QFileDialog + ExportService (visible rows in current order), full ApplicationDialog (Overview/Interviews read-only/Timeline/Notes, inline validation, confirm discard on Cancel, Timeline empty for a new application), selection kept by id across reloads. Extend ui_smoke: Jobs page empty and with data, dialog tabs, both themes, zero warnings.

Then walk through each M2 acceptance criterion in the running app (fresh XDG_DATA_HOME) and list each with how you verified it. If anything is not done, say NOT DONE on the first line of the report. If you run out of quota, commit what passes check.sh and state exactly what remains. Local commits only, never push.
