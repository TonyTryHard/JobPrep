## Plan — M1 (Data layer) — REVISED after review against SPEC.md and AGENTS.md

### Review amendments (binding; they replace anything below that disagrees)
1. **Seed counts:** SPEC §9 lists **12 C++ + 10 English = 22 topics** (not 13 + 10 = 23). Tests assert 22.
2. **Migrations:** each migration is a list of single statements run one by one (a multi-statement `QSqlQuery::exec` is rejected by the SQLite driver).
3. **Transaction guard:** explicit `bool commit()`; the destructor **rolls back** if `commit()` was not called. No commit in the destructor.
4. **No nested transactions:** one `Transaction` per public write; shared logic lives in private helpers that do not open one. `changed()` is emitted only **after** a successful commit.
5. **Seeding is idempotent:** dedupe by track name and by (track, topic title).
6. **Status history:** `insert()` writes the initial row (`from_status` NULL → initial status), because the schema makes `from_status` nullable; every later change adds one row.
7. **Rule A5 (SPEC §3.3):** "Applied + empty `applied_date` → today" applies in `insert()`, `update()` **and** `setStatus()`.
8. **`topics.resources` must round-trip unchanged** through `insert/update` (stored JSON), so an update can never wipe user data (AGENTS §8). Parsing it stays M4 work.
9. **Style (AGENTS §6):** `u"..."_s` literals (not `QStringLiteral`); logging category name `jobprep.data`; only Qt APIs available in Qt 6.5 (no 6.10-only APIs).
10. **Commits are split** (see Verification).

### Files to add

**`src/data/`** (namespace `JobPrep::Data`, all `QObject` repos emit `changed()`)
```
Database.h/.cpp            open/close, pragmas, RAII Transaction, lastError()
Migrations.h/.cpp          ordered Migration list + runner (PRAGMA user_version)
DbFormat.h/.cpp            ISO text <-> QDate/QDateTime/std::optional helpers
TrackRepository.h/.cpp
TopicRepository.h/.cpp     topics + subtasks
SessionRepository.h/.cpp
ApplicationRepository.h/.cpp  applications + status_history
InterviewRepository.h/.cpp
ReminderLogRepository.h/.cpp
DataLogging.h              (already untracked; Q_DECLARE_LOGGING_CATEGORY(lcData), category "jobprep.data", defined in Database.cpp)
```
**`src/services/SeedService.h/.cpp`**, **`resources/seed/topics.json`** (+ `resources.qrc` entry → `:/seed/topics.json`). Track name + color live in the JSON (`tracks` array), not as literals in C++.
**`tests/data/`**: `TestDatabase.h` helper + 8 test exes (`tst_Database`, `tst_TrackRepository`, `tst_TopicRepository`, `tst_SessionRepository`, `tst_ApplicationRepository`, `tst_InterviewRepository`, `tst_ReminderLogRepository`, `tst_SeedService`), each `QTEST_GUILESS_MAIN`, one `CMakeLists.txt`. Every test sets `QT_QPA_PLATFORM=offscreen` in its ENVIRONMENT property (AGENTS §3) and links the same library/object set that `ui_smoke` uses (sources are not compiled twice).

**Changed**: `src/app/AppContext.h/.cpp` (own `Database` + 6 repos + `SeedService`, ctor takes optional path), `src/main.cpp` (DB open check → first-launch prompt → `MainWindow`), `src/services/SettingsService.h/.cpp` (`sampleDataPrompted()`), `CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/ui_smoke/tst_UiSmoke.cpp` (in-memory DB so it never touches user data), `docs/SPEC.md` §14 changelog (only the entries listed under "Spec changelog" below; no other part of the spec is touched). Also commit the untracked `src/domain/Structs.h` after reading it.

### Database / migration runner
```cpp
class Database {
  explicit Database(QString connectionName = u"jobprep"_s);
  bool open(const QString& path);            // ":memory:" skips mkpath; otherwise QDir().mkpath(dir)
  bool isOpen() const;  void close();  QString lastError() const;
  QSqlDatabase connection() const;  QString connectionName() const;
  class Transaction {                   // RAII: rolls back in dtor unless commit() succeeded
    bool commit(); void rollback(); bool isActive() const;
  };
  Transaction transaction();            // BEGIN IMMEDIATE; commit/rollback via exec("COMMIT"/"ROLLBACK") only
};
```
`open()`: verify `QSQLITE` driver, create the folder (not for `:memory:`), named connection (`QSqlDatabase::addDatabase`), `PRAGMA foreign_keys=ON`, `PRAGMA journal_mode=WAL` (set before any transaction; tolerate `memory` for `:memory:`), then `runMigrations`. Destructor closes + `removeDatabase` so no "connection still in use" warning (ui_smoke fails on any warning). `lastError()` fed by a shared `execOrLog()` helper under `qCWarning(lcData)`. Transactions use raw `BEGIN IMMEDIATE` / `COMMIT` / `ROLLBACK` consistently; `QSqlDatabase::transaction()/commit()` are not mixed in.

```cpp
struct Migration { int version; QStringList statements; };   // Migrations.cpp, ordered; one statement per entry
QList<Migration> migrations();
int currentUserVersion(Database&);
bool runMigrations(Database&, const QList<Migration>&, QString* error);  // production overload passes migrations()
// per pending migration: BEGIN, exec each statement, PRAGMA user_version=N, COMMIT; any failure -> ROLLBACK, user_version unchanged
```
§5 SQL copied verbatim (no `IF NOT EXISTS` added) and split into its 10 statements (8 `CREATE TABLE` + 2 `CREATE INDEX`) for version 1.

### Repository interfaces (all `bool`/`std::optional` + `lastError()`)
```cpp
// TrackRepository
QList<Track> all() const; std::optional<Track> byId(int) const;
std::optional<int> idByName(QStringView) const;
bool insert(Track&);        // fills id
bool update(const Track&);  bool remove(int); int count() const; bool isEmpty() const;

// TopicRepository  (topics + subtasks, one changed())
QList<Topic> all() const; QList<Topic> byTrack(int) const; std::optional<Topic> byId(int) const;
bool insert(Topic&); bool update(const Topic&); bool setStatus(int, TopicStatus);
bool updatePositions(const QList<int>& orderedIds);   // board reorder
bool remove(int); int count() const; bool isEmpty() const;
QList<Subtask> subtasks(int topicId) const;
bool addSubtask(Subtask&); bool updateSubtask(const Subtask&); bool setSubtaskDone(int, bool);
bool removeSubtask(int); bool reorderSubtasks(int topicId, const QList<int>& orderedIds);
int subtaskCount(int topicId) const; int doneSubtaskCount(int topicId) const;

// SessionRepository
QList<StudySession> all() const; QList<StudySession> byTopic(int) const;
QList<StudySession> inRange(QDate from, QDate to) const; std::optional<StudySession> byId(int) const;
bool insert(StudySession&); bool update(const StudySession&); bool remove(int);
int totalMinutesInRange(QDate from, QDate to) const;

// ApplicationRepository  (applications + status_history)
QList<JobApplication> all() const; std::optional<JobApplication> byId(int) const;
QList<JobApplication> byStatuses(const QList<ApplicationStatus>&) const;  // empty list -> empty result
bool insert(JobApplication&);   // + initial status_history row (NULL -> status); Applied + empty applied_date -> today (§3.3 A5)
bool update(const JobApplication&);  // status differs -> one status_history row, same transaction; A5 applies too
bool setStatus(int id, ApplicationStatus to, const QString& note = {});  // A5 applies too
bool setNextAction(int id, const QString& text, std::optional<QDate> date);
bool duplicate(int id, JobApplication& copy);   // copy keeps fields; gets its own initial history row
bool remove(int);   // cascades status_history + interviews
int count() const; bool isEmpty() const;
QHash<ApplicationStatus, int> countsByStatus() const;
QList<StatusChange> statusHistory(int applicationId) const;   // ordered
QList<StatusChange> allStatusHistory() const;                  // for StatsService §6

// InterviewRepository
QList<Interview> all() const; QList<Interview> byApplication(int) const;
QList<Interview> inRange(const QDateTime& from, const QDateTime& to) const;
std::optional<Interview> byId(int) const; std::optional<Interview> nextUpcoming(const QDateTime&) const;
bool insert(Interview&); bool update(const Interview&); bool remove(int);

// ReminderLogRepository
bool wasSent(QStringView key) const;
bool markSent(const QString& key, const QDateTime& sentAt = {});   // key format §5; idempotent (INSERT OR IGNORE keeps first sent_at)
bool remove(const QString& key); QStringList sentKeys() const;
```
Each public write opens **one** `Database::Transaction` (even single-row writes that touch two tables, e.g. status change + history), calls private non-transactional helpers, commits, and only then emits `changed()` exactly once. `update()` always bumps `updated_at`; dates stored as `yyyy-MM-dd` / `yyyy-MM-ddTHH:mm:ss` (local) via `DbFormat`. `setStatus`/`update` must not call each other through their public, transactional versions.

### SeedService + first-launch prompt
`SeedService(TrackRepository&, TopicRepository&, QObject* parent = nullptr)` with `bool loadSampleTopics(QString* error = nullptr)`: parses `:/seed/topics.json`; reuses an existing track by name, else creates it (name + color from the JSON; C++ `#5B6CFF`, English `#F59E0B`); inserts each topic as `backlog` with 3–5 subtasks (§9 lists: **12 + 10 = 22 topics**), **skipping topics whose (track, title) already exists**. Everything in one transaction → one `changed()` (none if nothing was added).

`main.cpp`: `AppContext ctx;` → if `!ctx.database().isOpen()` → `QMessageBox::critical` + `return 1` → if `!ctx.settings().sampleDataPrompted() && ctx.isDataEmpty()` ask `QMessageBox::question("Load sample data?")` → `ctx.seedService().loadSampleTopics()` on Yes → `setSampleDataPrompted(true)` → `MainWindow`. `AppContext::isDataEmpty()` = tracks && topics && applications all empty. Assumption to state in the report: choosing "No" is final until a later milestone adds a way to load samples again.

### Tests (written first)
- **tst_Database**: in-memory open, `foreign_keys` ON, transaction commit / rollback / destructor-rollback without `commit()`; migrations from empty DB → `user_version == 1`, all 8 tables and both `idx_*` indexes present (ignore `sqlite_autoindex_*`); columns/defaults via `PRAGMA table_info`, `ON DELETE CASCADE|SET NULL` via `PRAGMA foreign_key_list`; `CHECK(minutes>0)` verified by an actual insert with `minutes = 0` that must fail (the expected warning is swallowed with `QTest::ignoreMessage`); re-running migrations is a no-op; **previous-version path:** with an injected list (v1 already applied, v2 test migration pending) only the pending one runs; **failure path:** an injected broken migration rolls back completely (`user_version` unchanged, no partial table).
- **Repos**: CRUD + `lastInsertId` propagation per repo; `changed()` emitted once per write (QSignalSpy, incl. status change and reorder) and **not** emitted when a write fails; delete track cascades topics+subtasks; delete topic sets `study_sessions.topic_id` to NULL (row survives); delete application cascades history+interviews; `insert` writes one initial history row (NULL → status), each later status change adds exactly one row with correct `from`/`to`; A5 (Applied + empty date → today) checked for `insert`, `update` and `setStatus`; `byStatuses({})` is empty; `topics.resources` round-trips unchanged through `insert` → `update` → `byId`; `markSent` twice keeps one row and the first `sent_at`; unknown id → `false`/`nullopt`, no warning.
- **tst_SeedService**: `Q_INIT_RESOURCE`; 2 tracks + **22** topics (12 C++, 10 English) + 3–5 subtasks each, all backlog; idempotent (second run adds no tracks, topics or subtasks and emits no `changed()`); a pre-existing topic with the same (track, title) is not duplicated; counts match the JSON.
- **ui_smoke**: `AppContext(u":memory:"_s)` so the smoke test never creates a DB in user data.

### Risks / notes
- Qt 6.10.2 with `libqsqlite.so` present on this box (`QT_PREFIX_PATH` empty → system packages). State this in the report, with the GCC version. Code must stay within Qt 6.5 APIs (AGENTS §2).
- Per-statement migration execution is verified by the first test run.
- `PRAGMA journal_mode=WAL` returns `memory` for `:memory:` → not asserted in tests.
- Seeded `notes`/`resources` stay empty (resources UI is M4); `tags`/`priority` filled lightly.
- Rule A5 and the initial history row live in `ApplicationRepository` because no application service exists yet; state this as an assumption in the report.

### Spec changelog (docs/SPEC.md §14, only these)
- `sampleDataPrompted` setting added for the first-launch prompt.
- `insert()` writes an initial `status_history` row (NULL → initial status).

### Verification
`scripts/check.sh` (configure + build + `ctest`, `-Werror`), then launch the app once to confirm the prompt appears on a fresh `XDG_DATA_HOME`, seed creates 2 tracks / 22 topics, and a restart does not re-ask. Report in AGENTS.md §10 format (What changed / How to run / How verified / Deviations and assumptions / Suggested next step). Local commits only, no push:
1. `feat(domain): add domain structs`
2. `feat(data): sqlite layer, migrations and repositories (M1)` (with its tests)
3. `feat(app): seed service and first-launch prompt (M1)` (with SeedService tests, AppContext/main/SettingsService, ui_smoke, SPEC changelog)
