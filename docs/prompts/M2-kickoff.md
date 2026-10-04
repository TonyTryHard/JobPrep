New session. M0 and M1 are done and committed. Execute the task in @docs/prompts/M2.md (Jobs page).

Before planning, read @AGENTS.md, @docs/SPEC.md §3.3 (A1–A8), §5, §6, §8, §10 (CSV) and the M2 entry in §12. Write a short plan (files, risks), save it to @docs/plans/M2-plan.md  and WAIT for my approval before changing anything.

Constraints on the plan
- Layering: ui -> models -> services -> data -> domain. Models read repositories and reload on changed(); no SQL outside src/data/.
- No schema change and no new migration. If InterviewRepository or ApplicationRepository lacks a query you need, add a method with tests.
- Build with at most 2 parallel jobs (export CMAKE_BUILD_PARALLEL_LEVEL=2). Do not start a build you cannot finish; tell me if a command is too long.

Decisions already made (state any other assumption in the report)
1. Keep Rule A5 and the initial status_history row in ApplicationRepository. Do NOT move them into a service now: the history row must stay in the same transaction as the application write.
2. "Next step" column (A1): the earliest upcoming interview with outcome = pending; if none, next action text + date. Cancelled, passed and failed interviews never count.
3. Status chips (A2): Active = {applied, hr_screen, technical, final, offer}; Closed = {rejected, withdrawn, ghosted, accepted} (SPEC §2 rules). Interviewing = {hr_screen, technical, final}; Offer = {offer}. Wishlist appears only under All.
4. Sorting uses real values, not display strings: dates by date, salary numerically, status by pipeline order (enum order), default Updated descending.
5. Interviews tab is a read-only list until M3. The "Add interview…" context-menu entry is present but disabled until M3.
6. CSV export: ExportService in services (not in a widget). Exports the VISIBLE rows in their current sort order. UTF-8 with BOM, comma separator, header row, columns = A1 plus all remaining fields, dates yyyy-MM-dd. Quote fields containing comma, quote or newline; double embedded quotes. Use a save dialog; the file must open in Excel with correct columns.
7. Use QKeySequence::StandardKey for New and Find. Delete asks for confirmation. Status colors only through StatusStyle; no hard-coded colors; empty state (icon + hint + primary action).

Verify first (add a test if missing)
- When a write happens inside a nested transaction (savepoint), changed() is emitted only after the OUTERMOST commit, and never after a rollback. Report the result in the plan.

Tests required
- ExportService with a fixed list: commas, quotes, newlines in notes, Cyrillic text, empty optional fields; BOM present; header order.
- Proxy filtering and sorting (chips, search over company/position/notes, numeric salary sort, status order).
- ApplicationTableModel reloads on changed(); "Next step" rule from decision 2.
- Extend ui_smoke: Jobs page with empty DB and with data, ApplicationDialog opened (every tab), both themes, no unexpected qWarning/qCritical.

Acceptance (SPEC §12, M2): add/edit/delete/duplicate applications; sort and filter work; a status change appears in Timeline; CSV export opens in Excel with correct columns. List each criterion in the report.

Commits: local only, never push. Prefer 2–3 small commits (service + model + tests; UI; ui_smoke + docs). Report in AGENTS.md §10 format, including "Suggested next step".
