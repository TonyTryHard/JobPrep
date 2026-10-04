## Plan — M2 (Jobs page) (Revised per feedback)

### Goals
Implement M2 from SPEC §12 and §3.3, §7.4. Jobs page with full application management (add/edit/delete/duplicate), table model + proxy with filtering/sorting, status badge delegate, toolbar (search, chips, CSV export), ApplicationDialog with Overview + Notes + Timeline tabs (Interviews tab read-only until M3 per decision 5), context menu, empty states (no data / no matches), footer with counts per status. CSV export via ExportService. Follow architecture (ui -> models -> services -> data -> domain), no schema changes.

### Prerequisites & verification (nested transactions)
Database uses depth-based transaction joining (not SQLite SAVEPOINTs). When transactionDepth>0, transaction() returns a non-owner Transaction that does not call BEGIN/COMMIT/ROLLBACK; only the outermost owner does. Repos emit changed() immediately after transaction.commit() returns true. This means: if an inner "nested" transaction (non-owner) is committed, commit() returns true but the DB transaction is still open - changed() would be emitted prematurely. Also if outer rolls back later, we already emitted. That’s the opposite of the requirement.

Action: Fix Transaction/commit semantics so changed() is emitted only on outermost successful commit. Options:
1. Have repos track whether they are inside an outer transaction? Or have Database emit a signal on outermost commit/rollback? Or change repo pattern: repos don’t emit in nested context; instead, the code that owns the outer transaction emits changed() after outer commit. But repos currently own their own transactions.
2. Alternatively, add a way: Transaction could have a callback or Database could track pending changes. Or change repos to accept an optional Transaction* parameter for nested writes (so only the top-level creator emits). Or simpler: add a "deferred emit" mechanism on Database/repo when nested - count pending emits and flush on outermost commit.

But repos call transaction() themselves. Need to adjust: when joining (non-owner), the repo still needs to do its work; but we must not emit changed() until outermost commit. So modify repos: track if we created an owner transaction. Let’s say:
```cpp
auto tx = m_database.transaction();
bool owner = tx.isOwner(); // add method
... do work ...
if (!tx.commit()) return false;
if (owner) emit changed();
return true;
```
Also need Transaction::isOwner(). And Database needs to know if there are pending changes? Or just emit on outermost commit boundary from the owner’s perspective.

Also add Transaction::isOwner() accessor. Update all repos to only emit changed() when the transaction they created is the owner (outermost). That ensures: inner non-owner commits -> no emit; outer owner commits -> emit once (after its commit). Rollback of outer (owner) means we never emitted. This matches "emitted only after the OUTERMOST commit, never after a rollback".

Also verify with the requested test: seed inside an outer transaction -> changed() count 0 (if seed uses repos that see non-owner? Or if we do it properly); after outer commit -> +1. Roll outer back -> 0 emissions. Will add this test in tst_SeedService or a new focused test (tst_TransactionBehavior) covering repo emissions under nested joins.

Also read Database.cpp more - add isOwner() to Transaction (expose m_owner). Fix all repo write paths accordingly.

### Key design changes from feedback
1. **Nested transactions fix**: Add `Transaction::isOwner()` (or track). In all repos (Application, Interview, Session, Topic, Track, ReminderLog), only emit `changed()` if the transaction committed was the owner (outermost). This prevents premature emission on joined transactions.
2. **"Next step" computation**: Not computed on-the-fly in data() per cell. Compute once per reload and cache per application (array indexed by row). Reload triggered on both ApplicationRepository::changed() AND InterviewRepository::changed() (model listens to both). For each app, get next pending interview (earliest startAt with outcome == pending; cancelled/passed/failed excluded). If none, build next-step text from next_action + next_action_date. Sort role for Next step uses interview startAt (QDateTime) if exists, else next_action_date (QDate), else far future/empty. 
3. **Notes tab**: Add Notes tab to ApplicationDialog (A3). Include the footer with counts per status (A7) in JobsPage. 
4. **Separation of concerns (no UI in services/models)**: ExportService receives (a) list of export rows (struct with all fields needed), (b) a status label resolver function (or just labels precomputed) - labels come from StatusStyle (theme/domain concern) but service is in services; better pass precomputed display strings from UI/model layer, or have ExportService take a small translator for enums? Or ExportService is pure data -> CSV; for Status use enum-to-stored/display form? SPEC states CSV columns include Status (badge text) as shown - use display label (e.g. "HR Screen"). But services must not depend on ui/. So either move label mapping to domain (add to EnumStrings with tr() not needed? Or have non-UI label function) or UI prepares rows. Better: UI/model prepares a list of CSV row values (already formatted for display where needed) and passes to ExportService. ExportService does only CSV serialization (BOM, quoting, commas). That keeps services clean.
5. **Labels for enums**: WorkMode/InterviewType display labels - add to EnumStrings or StatusStyle? StatusStyle currently has labels for ApplicationStatus/TopicStatus/Priority. Add labels for WorkMode and InterviewType (non-UI in naming but lives in theme/ui layer? Or better domain/strings). But to avoid UI dep in services, keep label helpers in a neutral place - EnumStrings could have display label helpers (using QCoreApplication::translate) or create ui/theme/EnumLabels. But easier: add display label methods to StatusStyle for completeness (it already has status labels). Or just compute in UI. For ExportService input preparation, UI computes display strings.
6. **CSV export**: Pass prepared rows (QList of QStringList or struct) to ExportService. ExportService takes headers list and data rows. Does not know about StatusStyle - keeps it UI-agnostic.
7. **ChipBar**: Check existing widgets - none found. Add minimal ChipBar (FlowLayout or QHBoxLayout with checkable QPushButtons/QToolButtons). 
8. **Validation**: Dialog validates company/position required, salary min <= max (show tooltip/error). 
9. **Empty states**: Two states in JobsPage - (1) no applications at all (show EmptyState with action "Add Application"), (2) filtered to no matches (show different EmptyState: "No matches" with hint and maybe "Clear filters"). 
10. **Open URL**: http(s) only. Validate with QUrl::isValid() and scheme http/https; if not, show warning toast (non-blocking). 
11. **Sorting**: explicit default Updated desc (set in proxy/model). Status sort follows enum order (pipeline order by enum value as defined). Add pinning test. 
12. **Search**: Unicode case-insensitive using QRegularExpression with case-insensitive or QString::compare with Qt::CaseInsensitive and Unicode. Add Cyrillic test. 
13. **Keyboard**: Enter on table opens selected application (edit). Delete key triggers delete with confirmation. 
14. **QSaveFile** for CSV write (atomic). 
15. **Model reloads**: ApplicationTableModel connects to both ApplicationRepository::changed() and InterviewRepository::changed() and reloads cache + model on either. 

### Concrete changes per component

**Database/Transaction**
- Add `bool isOwner() const` to `Transaction` (expose m_owner). Header updated.
- Nested behavior fix: repos must only emit changed() when the owner transaction commits. Add method or just check. Also consider that if someone does nested writes via repos inside outer transaction they didn’t create - but repos create their own transactions. To properly test "seed inside an outer transaction", we need to either (a) have Database allow external transaction control, or (b) test by calling repo methods after starting an outer tx - but each repo write will call transaction() and get non-owner; so with fix above, changed() won’t emit until outer commits - but who owns outer? If outer is started via m_database.transaction() by test code, then only that outer owner commit will cause emission - but emissions come from repos. So each repo write that was joined (non-owner tx) won’t emit. After outer owner commits, none of those inner writes emitted. That’s wrong if we want "after OUTERMOST commit" semantics. Wait - the requirement says: "changed() is emitted only after the OUTERMOST commit, and never after a rollback. Report the result in the plan."

Also "When a write happens inside a nested transaction (savepoint), changed() is emitted only after the OUTERMOST commit..." So multiple writes inside outer tx - each write might normally emit changed() after its own commit; with nesting, we defer all emissions until outermost commits. That means we need to accumulate pending change notifications and flush once.

So better approach:
- Add to Database (or each repo) a pending changed count/flag. When in nested context (non-owner tx active?) or more generally, track if any write happened during an outer transaction. Alternatively, Database tracks `m_hasPendingChanges` that gets set by repos on successful writes when joined; on outermost commit, if set, emit a global signal or have repos flush.
- Or simpler: have repos check - if current transaction depth>0 and not the one they created as owner? Maybe easier: add `bool hasActiveOwnerTransaction()` or just track globally. Alternatively, modify pattern:
```cpp
auto tx = m_database.transaction();
bool owner = tx.isOwner();
bool ok = doWork();
if (!tx.commit()) return false;
if (owner) emit changed();
else { m_database.markPendingChange(this); } // track
// on outermost commit, Database needs to flush pending - but how? repos are different objects
```
But easier to have repos connect to Database’s commit/rollback signals? Or have Database expose a way. Maybe add to Database: `void requestFlush(QObject* repo)` and emit `pendingFlushNeeded` on outer commit? Or simpler design: in nested case, don’t emit; require caller to handle. But not matching current API.

Alternative: add to Database a counter `m_pendingEmits` (or per-repo not directly). Or change all public repo methods to take an optional `Transaction*` - if provided, use it (joined); only emit if we own the transaction. That means nested writes done inside outer tx don’t emit - and outer code must emit after its tx commits. That shifts responsibility. But repos currently self-manage tx. Let’s see how it’s used.

But M1 tests expect changed() emitted after write. For nested case as specified, better to fix by deferring: add a method on Database to check if we’re in an outer transaction? Or track: when a non-owner tx commits successfully, set a "pending changed" flag on Database. When an owner tx commits successfully and pending flags exist, after commit, trigger flushes. Also on rollback of owner, clear pending flags.

So extend Database:
- add `mutable QSet<QObject*> m_pendingChangeRepos;` (or just count)
- add methods: `void deferChanged(QObject* repo) const; void flushPendingChanges() const; void clearPendingChanges() const;`

Transaction::commit() when owner succeeds: after commitInternal, call finish(), then m_database->flushPendingChanges(). When owner rolls back: after rollbackInternal, call finish(), then m_database->clearPendingChanges(). When non-owner commits: just finish() (don’t flush/clear DB-level except depth), and if it committed successfully in effect? non-owner commit() just returns true and becomes inactive - no DB commit happened. But the work was done inside outer DB tx; so any successful inner write should count as a pending change to be emitted after outer DB commit. So in repos, when tx.commit() returns true and !owner: repo calls m_database.deferChanged(this). When tx.commit() returns true and owner: emit changed() (and also flush any deferred? maybe not needed if we only defer on non-owner). Also need to handle case where no deferred existed.

So repo pattern becomes:
```cpp
auto tx = m_database.transaction();
bool owner = tx.isOwner();
// ... do work, set up m_lastError if fail ...
if (!tx.commit()) {
  return false;
}
if (owner) emit changed();
else m_database.deferChanged(this);
return true;
```

Database::flushPendingChanges() iterates m_pendingChangeRepos and emits changed() on each (qobject cast) then clears. clearPendingChanges() just clears. Need to include QObject headers as needed. This ensures all pending emits happen once on outermost commit.

Also add `bool Transaction::isOwner() const { return m_owner; }` and expose it. Update Transaction header.

Add a focused test (e.g. in tests/data) for nested transaction behavior with changed() emissions - e.g. have a TestRepo-like setup or use actual repos; create two applications in outer tx via ApplicationRepository - each write gets non-owner tx, so no immediate emit; after outer tx commits, both should have emitted once total? Or each repo emits when flushed - so ApplicationRepository would emit changed() twice? No wait - if two separate repo instances had changes deferred, flush would call emit changed() on each. But typically same repo instance; or different repos. The requirement says "changed() is emitted..." - each repo emits its own signal. So flushPendingChanges should call changed() on each repo QObject that requested defer. That’s fine.

Also need to add defer/flush methods to Database. Let’s add them.

**ApplicationTableModel** (src/models/ApplicationTableModel.h/.cpp)
- Inherits QAbstractTableModel. Takes ApplicationRepository&, InterviewRepository&. 
- Columns (A1 order): Company, Position, Status, Applied, Next step, Salary, Source, Updated.
- Roles: Qt::DisplayRole, Qt::EditRole, Qt::DecorationRole (for status badge), SortRole for custom sorting.
- Cache: struct RowCache { JobApplication app; std::optional<QDateTime> nextInterviewAt; QString nextStepText; } per row. Recompute on reload() called from both repos’ changed() signals (connect in ctor).
- reload(): clear cache, fetch all applications (or in order consistent with default view) and for each, find earliest pending interview: query interviews for app via InterviewRepository::byApplication(app.id) and filter outcome==InterviewOutcome::Pending, startAt >= now? Or just earliest startAt among pending (SPEC states "earliest upcoming interview with outcome = pending"). "Upcoming" means startAt >= current time? Or any future? Decision 2 says "earliest upcoming interview..." so compare to QDateTime::currentDateTime() (or just earliest by startAt if we treat as upcoming). Also cancelled/passed/failed excluded. 
- Build nextStepText: if has next pending interview, format as "Type @ Place (YYYY-MM-DD HH:MM)" or similar compact form? Or include company context not needed; just descriptive. But keep it short for table. If no pending interview, use next_action text + next_action_date: if both non-empty, "text (YYYY-MM-DD)"; if only date, date string; if only text, text. Empty if neither.
- SortRole values: for dates use QDateTime/QDate (Applied as QDate, Next step uses nextInterviewAt if set else next_action_date as QDateTime midnight? or just the date), Salary uses max if both set else min else 0, Status uses pipeline order key (define mapping by enum: wishlist=0, applied=1, hr_screen=2, technical=3, final=4, offer=5, accepted=5, rejected=10, withdrawn=11, ghosted=12 - or follow logical progression; but "status by pipeline order (enum order)" - use enum value order as declared? Check Enums.h order: Wishlist=0, Applied=1, HrScreen=2, Technical=3, Final=4, Offer=5, Accepted=6, Rejected=7, Withdrawn=8, Ghosted=9? Or look up actual enum. But easier to define an explicit int key per status for sorting to be stable.) Salary sorting numeric: primary by effective salary - if salary_max set use salary_max, else if salary_min set use salary_min, else treat as 0 or very large for ascending? Define consistently.
- data() for DisplayRole returns formatted strings; for Status role returns ApplicationStatus (for delegate/proxy). 
- headerData() matches column names.

**ApplicationFilterProxy** (src/models/ApplicationFilterProxy.h/.cpp)
- QSortFilterProxyModel. Status chip filter (All/Active/Interviewing/Offer/Closed). Text search over company/position/notes (case-insensitive, Unicode). 
- Chip groups: Active = {applied,hr_screen,technical,final,offer}; Interviewing={hr_screen,technical,final}; Offer={offer}; Closed={rejected,withdrawn,ghosted,accepted}. Wishlist only under All (so if chip is Active/Interviewing/Offer/Closed, wishlist rows hidden).
- filterAcceptsRow checks status group AND text. Text: concatenate company, position, notes; search substring case-insensitively (Qt::CaseInsensitive, QString::contains or QRegularExpression with case insensitive Unicode).
- lessThan overridden: for Status column use status pipeline order key; for Salary use numeric value; for dates use QDate/QDateTime; Next step sorts by nextInterviewAt if cached else next_action_date. Default sort is Updated desc (setSortRole for Updated column role - Updated is stored as QDateTime in app.updatedAt; expose via SortRole).

**ExportService** (src/services/ExportService.h/.cpp, no UI deps)
- Struct for export row data (all fields needed for CSV): company, position, statusLabel, applied (yyyy-MM-dd or empty), nextStepText, salaryText (formatted e.g. "4–5k USD" or just numbers if present), source, updated (yyyy-MM-dd), url, resumeVersion, location, workModeLabel, salaryMin, salaryMax, currency, nextAction, nextActionDate, contactName, contactEmail, notes, createdAt, updatedAt, id.
- Method: `bool exportToFile(const QList<ExportRow>& rows, const QStringList& headers, const QString& filePath, QString* error = nullptr)` writes CSV with BOM (\xEF\xBB\xBF), comma separator. For each field: if contains comma, quote, CR, or LF -> quote the field and double any internal quotes; else no quotes. Dates already formatted. 
- Use QSaveFile for atomic write.
- UI prepares ExportRow list from visible proxy rows (in current sort order) and passes display labels (status/work mode etc computed in UI using StatusStyle/EnumStrings as appropriate).

**ChipBar** (src/ui/widgets/ChipBar.h/.cpp)
- Horizontal bar of toggleable chips. Holds a list of chip ids/labels. Signal `chipChanged(int id, bool checked)` or just current selection mode? For status filter we need single selection (All + groups) or multiple? Decision 3 says chips: All/Active/Interviewing/Offer/Closed - likely exclusive selection (like tabs/chips group). Use QButtonGroup with QToolButton or QPushButton, checkable, autoExclusive. Simple layout (QHBoxLayout with spacing). No hard-coded colors - follow theme via palette/QSS later or use existing style patterns.

**StatusBadgeDelegate** (src/ui/delegates/StatusBadgeDelegate.h/.cpp)
- QStyledItemDelegate override paint(). For Status role (or column), get status and draw rounded pill: fill with color from StatusStyle::color(status) at ~15% opacity background and full-color text? "rounded pill with status color at ~15% background and full-color text" (SPEC §7.4). So background is same color with alpha ~38 (15% of 255) or use QBrush with color.setAlphaF(0.15). Text color is the status color (full). Draw rounded rect, center text. Respect devicePixelRatio, avoid allocations in paint (use local QPainterPath/QFontMetrics, no new big objects). Also handle selection state if needed.

**ApplicationDialog** (src/ui/dialogs/ApplicationDialog.h/.cpp)
- QDialog, modal or non-modal? Standard edit dialog. Has QTabWidget with tabs: Overview, Notes, Timeline (A3 says Overview + Timeline; feedback adds Notes tab - A3 lists "Overview (all fields), Interviews (list + add/edit/delete), Timeline (read-only status history), Notes"). So tabs: Overview, Interviews, Timeline, Notes. Interviews read-only list until M3; "Add interview…" disabled until M3 (decision 5). 
- Overview: form fields for A4 (company*, position*, URL, source editable combo: LinkedIn, DOU, Djinni, Referral, Company site, Other, resume version, location, work mode combo, salary min/max + currency, status combo, applied date, next action text + date, contact name/e-mail). Validation: company/position required; salary min <= max if both set. 
- Notes: QTextEdit for notes. 
- Timeline: QListView or QTableView showing status history (read-only). Columns: From, To, Changed at, Note. 
- Save/Cancel. On save, create or update via ApplicationRepository. Status change triggers history write (repo already does). 
- Opened from JobsPage (add/edit). 

**JobsPage** (src/ui/pages/JobsPage.h/.cpp)
- Owns model, proxy, table view, delegate, toolbar, chips, empty states, footer. Gets repos/services via AppContext (composition root). Pass AppContext ref to JobsPage constructor (or specific refs). 
- Toolbar: QLineEdit search (Find shortcut focuses it), ChipBar with chips All/Active/Interviewing/Offer/Closed, "Export CSV" button, "+ Application" button (triggers New). 
- Table: QTableView with ApplicationFilterProxy as source. Set model, install StatusBadgeDelegate on Status column. Enable sorting (click headers). Set default sort: Updated descending (proxy sort by Updated column desc). 
- Context menu on selection: Edit, Change status submenu (all ApplicationStatus values or sensible set), Add interview… (disabled until M3), Open posting URL (http(s)-only; validate), Duplicate, Delete (confirm). Double-click opens edit. Delete key deletes with confirm. Enter opens edit. 
- Empty states: if total applications == 0, show EmptyState "No Applications Yet" with action "Add Application". If total > 0 but proxy filtered to empty, show "No matches" empty state (different text) with hint like "Try adjusting search or filters" and maybe "Clear filters" action. 
- Footer: shows counts per status (All, Applied, HR Screen, Technical, Final, Offer, Active total, Closed total, etc. as useful) using ApplicationRepository::countsByStatus() or compute from all(). 
- Wire changed() from applications and interviews to model->reload() (model already listens; page can also refresh footer). 
- CSV export: get visible rows from proxy in current sort order (iterate proxy.mapToSource for each row), build ExportRow list with display labels, call ExportService::exportToFile with headers. Use QFileDialog for save (csv). Show success/error toast? Non-blocking; follow existing patterns.
- "New" action uses QKeySequence::StandardKey::New; Find uses StandardKey::Find.

**AppContext** (src/app/AppContext.h/.cpp)
- Add owned ApplicationTableModel (or just construct in JobsPage? better own as member so it lives with repos; also can be passed). Also ExportService. 
- Add getters: models::ApplicationTableModel& applicationsModel() const (if placed in models namespace), services::ExportService& exportService() const. 
- Wire: connect applications()->changed() and interviews()->changed() to applicationsModel()->reload() if owned here. Or let JobsPage connect - both fine; composition root can wire.
- Include new headers.

**CMakeLists.txt**
- Add src/models/ApplicationTableModel.cpp/h, src/models/ApplicationFilterProxy.cpp/h. 
- Add src/services/ExportService.cpp/h. 
- Add src/ui/delegates/StatusBadgeDelegate.cpp/h. 
- Add src/ui/dialogs/ApplicationDialog.cpp/h. 
- Add src/ui/widgets/ChipBar.cpp/h (if new). 
- Ensure include dirs cover src/models, src/ui/delegates, src/ui/dialogs, src/ui/widgets.

### Tests

**tests/services/tst_ExportService.cpp** (new)
- Create fixed list of ExportRow with: commas in company/notes, quotes, newlines in notes, Cyrillic text (UTF-8), empty optional fields. 
- Call exportToFile to temp path. Read back as bytes and check BOM present (\xEF\xBB\xBF). Parse CSV by lines; header order matches. Fields correctly quoted/doubled. Cyrillic preserved. Dates yyyy-MM-dd.

**tests/models/tst_ApplicationFilterProxy.cpp** (new)
- Setup in-memory DB, repos, model+proxy. Test chip filtering (All/Active/Interviewing/Offer/Closed, wishlist only under All), text search over company/position/notes (case-insensitive), numeric salary sort, status pipeline order, pinning test for status order.

**tests/models/tst_ApplicationTableModel.cpp** (new)
- Reload on changed() from ApplicationRepository and from InterviewRepository. "Next step" rule: earliest upcoming pending interview wins; cancelled/passed/failed ignored; else next action text+date. Sort role behavior.

**tests/data/tst_TransactionBehavior.cpp** (new, or extend existing)
- Nested transaction behavior: start outer transaction via Database::transaction() (owner). Call repo write (e.g. insert application) - with fix, repo will get non-owner tx and defer emit; after inner tx.commit() in repo returns, changed() not emitted yet? Also test: do write inside outer tx; check signal spy count is 0; outer tx.commit() -> spy count becomes 1. Roll back outer -> no emissions. Also multiple writes inside outer -> one flush gives emits? Each deferred repo emits once on flush; so ApplicationRepository emits changed() once per flush - so if same repo did multiple writes, each of those writes would have called deferChanged(this) multiple times? Or better track "has pending" per repo. Better to track per repo: `m_pendingRepos` is a set of repo QObjects - so each repo emits once even if multiple writes happened. That matches "changed() emitted..." semantics (signal fires to indicate change). So deferChanged adds repo to set once.

Also add Transaction::isOwner() test coverage.

**ui_smoke** (extend tst_UiSmoke.cpp)
- Jobs page with empty DB and with data, ApplicationDialog opened (every tab), both themes, no unexpected qWarning/qCritical. 
- With in-memory DB, create some applications via repos, switch to Jobs page, open ApplicationDialog (Overview, Notes, Interviews, Timeline tabs), close, toggle themes.

### Acceptance (M2 per SPEC §12)
- add/edit/delete/duplicate applications
- sort and filter work
- a status change appears in Timeline
- CSV export opens in Excel with correct columns

List each in report. Also verify all tabs open, footer counts, empty states, keyboard shortcuts.

### Commits (local only)
1. `fix(data): add Transaction::isOwner and defer changed() emissions on nested tx (M2)`
2. `feat(models): add ApplicationTableModel and ApplicationFilterProxy with Next step logic (M2)`
3. `feat(services): add ExportService for CSV export (M2)`
4. `feat(ui): add StatusBadgeDelegate, ChipBar, ApplicationDialog, Jobs page wiring (M2)`
5. `test(jobs): add ExportService, model, transaction behavior tests and extend ui_smoke (M2)`
6. `docs: update CMake, AppContext, SPEC/plan if needed (M2)`

But prefer 2–3 as suggested; can group logically.

### Risks
- Nested tx fix touches all repos - need to be careful and add test.
- Computing Next step requires loading interviews per app on reload; for typical sizes fine. Can optimize later if needed.
- StatusBadgeDelegate must paint correctly with alpha and respect HiDPI. 
- CSV with newlines/quotes needs careful handling. 
- ChipBar styling must not hard-code colors. 
- Cyrillic in search/sort - use Unicode-aware comparisons.
