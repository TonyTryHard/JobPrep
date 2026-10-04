#include <QFileInfo>
#include <QHash>
#include <QSqlRecord>
#include <QTemporaryDir>
#include <QTest>
#include "MessageCapture.h"
#include "TestDatabase.h"
#include "data/Migrations.h"

using namespace Qt::StringLiterals;
using JobPrep::Data::Migration;
using JobPrep::Tests::MessageCapture;
using JobPrep::Tests::TestDatabase;

class tst_Database : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void openInMemoryEnablesPragmas();
    void closeMakesConnectionUnavailable();
    void opensFileDatabaseWithWalAndReopens();
    void transactionCommitPersists();
    void transactionRollbackReverts();
    void transactionDestructorRollsBack();
    void nestedTransactionJoinsOuter();
    void migrationsFromEmptyDatabase();
    void migrationsAreIdempotent();
    void migrationsRunOnlyPendingVersions();
    void failedMigrationRollsBackCompletely();
    void checkConstraintRejectsNonPositiveMinutes();
    void foreignKeyDeleteActionsMatchSchema();
    void foreignKeysCascadeAndSetNull();
    void failedStatementReportsError();

private:
    std::unique_ptr<TestDatabase> m_db;
};

void tst_Database::init() {
    m_db = std::make_unique<TestDatabase>(u"tst_database"_s);
    QVERIFY(m_db->open(u":memory:"_s));
}

void tst_Database::cleanup() {
    m_db.reset();
}

void tst_Database::openInMemoryEnablesPragmas() {
    QVERIFY(m_db->db().isOpen());
    QCOMPARE(m_db->db().path(), u":memory:"_s);
    QCOMPARE(m_db->db().connectionName(), u"tst_database"_s);
    QCOMPARE(m_db->scalar(u"PRAGMA foreign_keys"_s).toInt(), 1);
    // WAL is impossible for an in-memory database; it must not fail the open.
    QCOMPARE(m_db->scalar(u"PRAGMA journal_mode"_s).toString(), u"memory"_s);
    QCOMPARE(m_db->scalar(u"SELECT 1"_s).toInt(), 1);
}

void tst_Database::closeMakesConnectionUnavailable() {
    m_db->db().close();
    QVERIFY(!m_db->db().isOpen());

    auto transaction = m_db->db().transaction();
    QVERIFY(!transaction.isActive());

    MessageCapture capture;
    QVERIFY(!m_db->exec(u"SELECT 1"_s));
    QVERIFY(!m_db->db().lastError().isEmpty());
}

void tst_Database::opensFileDatabaseWithWalAndReopens() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(u"jobprep.sqlite"_s);

    {
        TestDatabase first(u"tst_database_file"_s);
        QVERIFY(first.open(path));
        QCOMPARE(first.scalar(u"PRAGMA journal_mode"_s).toString(), u"wal"_s);
        QCOMPARE(first.userVersion(), 1);
        QVERIFY(QFileInfo::exists(path));
    }

    // Reopening proves the migrations are not replayed: migration 001 has no
    // IF NOT EXISTS clauses, so a second run would fail on the existing tables.
    TestDatabase second(u"tst_database_file_2"_s);
    QVERIFY2(second.open(path), qPrintable(second.db().lastError()));
    QCOMPARE(second.userVersion(), 1);
    QVERIFY(second.hasTable(u"applications"_s));
}

void tst_Database::transactionCommitPersists() {
    auto transaction = m_db->db().transaction();
    QVERIFY(transaction.isActive());
    QVERIFY(m_db->exec(u"CREATE TABLE committed(id INTEGER)"_s));
    QVERIFY(m_db->exec(u"INSERT INTO committed VALUES (1)"_s));
    QVERIFY(transaction.commit());
    QVERIFY(!transaction.isActive());

    QCOMPARE(m_db->countRows(u"committed"_s), 1);
    QCOMPARE(m_db->scalar(u"PRAGMA foreign_keys"_s).toInt(), 1);
}

void tst_Database::transactionRollbackReverts() {
    auto transaction = m_db->db().transaction();
    QVERIFY(transaction.isActive());
    QVERIFY(m_db->exec(u"CREATE TABLE rolled_back(id INTEGER)"_s));
    transaction.rollback();
    QVERIFY(!transaction.isActive());

    QCOMPARE(m_db->countRows(u"rolled_back"_s), 0);
}

void tst_Database::transactionDestructorRollsBack() {
    {
        auto transaction = m_db->db().transaction();
        QVERIFY(transaction.isActive());
        QVERIFY(m_db->exec(u"CREATE TABLE dropped(id INTEGER)"_s));
    }
    QCOMPARE(m_db->countRows(u"dropped"_s), 0);

    // The connection is usable again afterwards.
    auto next = m_db->db().transaction();
    QVERIFY(next.isActive());
    QVERIFY(next.commit());
}

void tst_Database::nestedTransactionJoinsOuter() {
    auto outer = m_db->db().transaction();
    QVERIFY(outer.isActive());

    auto inner = m_db->db().transaction();
    QVERIFY(inner.isActive());
    QVERIFY(m_db->exec(u"CREATE TABLE joined(id INTEGER)"_s));
    // Committing the joiner must not end the outer unit of work.
    QVERIFY(inner.commit());

    QVERIFY(m_db->exec(u"INSERT INTO joined VALUES (1)"_s));
    outer.rollback();

    QCOMPARE(m_db->countRows(u"joined"_s), 0);
}

void tst_Database::migrationsFromEmptyDatabase() {
    QCOMPARE(m_db->userVersion(), 1);
    QVERIFY(m_db->hasTable(u"tracks"_s));
    QVERIFY(m_db->hasTable(u"topics"_s));
    QVERIFY(m_db->hasTable(u"subtasks"_s));
    QVERIFY(m_db->hasTable(u"study_sessions"_s));
    QVERIFY(m_db->hasTable(u"applications"_s));
    QVERIFY(m_db->hasTable(u"status_history"_s));
    QVERIFY(m_db->hasTable(u"interviews"_s));
    QVERIFY(m_db->hasTable(u"reminders_sent"_s));

    QVERIFY(m_db->hasIndex(u"idx_applications_status"_s));
    QVERIFY(m_db->hasIndex(u"idx_interviews_start"_s));

    QCOMPARE(JobPrep::Data::migrations().size(), 1);
    QCOMPARE(JobPrep::Data::migrations().first().version, 1);
    // Migration 001 is 8 CREATE TABLE plus 2 CREATE INDEX statements.
    QCOMPARE(JobPrep::Data::migrations().first().statements.size(), 10);
}

void tst_Database::migrationsAreIdempotent() {
    QString error;
    QVERIFY2(JobPrep::Data::runMigrations(m_db->db(), JobPrep::Data::migrations(), &error),
             qPrintable(error));
    QCOMPARE(m_db->userVersion(), 1);
    QCOMPARE(m_db->countRows(u"topics"_s), 0);
}

void tst_Database::migrationsRunOnlyPendingVersions() {
    QList<Migration> injected{
        Migration{1, {u"CREATE TABLE injected_one(id INTEGER)"_s}},
        Migration{2, {u"CREATE TABLE injected_two(id INTEGER)"_s}},
    };

    QString error;
    QVERIFY2(JobPrep::Data::runMigrations(m_db->db(), injected, &error), qPrintable(error));

    QCOMPARE(m_db->userVersion(), 2);
    // Version 1 was already applied, so its (different) statement must not run.
    QVERIFY(!m_db->hasTable(u"injected_one"_s));
    QVERIFY(m_db->hasTable(u"injected_two"_s));
    QVERIFY(m_db->hasTable(u"applications"_s));
}

void tst_Database::failedMigrationRollsBackCompletely() {
    QList<Migration> good{Migration{2, {u"CREATE TABLE injected_two(id INTEGER)"_s}}};
    QString error;
    QVERIFY(JobPrep::Data::runMigrations(m_db->db(), good, &error));
    QCOMPARE(m_db->userVersion(), 2);

    QList<Migration> broken{
        Migration{3,
                  {u"CREATE TABLE injected_three(id INTEGER)"_s, u"CREATE TABLE nope("_s}}};
    error.clear();
    QVERIFY(!JobPrep::Data::runMigrations(m_db->db(), broken, &error));
    QVERIFY(!error.isEmpty());

    QCOMPARE(m_db->userVersion(), 2);
    QVERIFY(!m_db->hasTable(u"injected_three"_s));
    QVERIFY(m_db->hasTable(u"injected_two"_s));
}

void tst_Database::checkConstraintRejectsNonPositiveMinutes() {
    MessageCapture capture;
    QVERIFY(!m_db->exec(
        u"INSERT INTO study_sessions (topic_id, session_date, minutes, note) "
        u"VALUES (NULL, '2026-10-04', 0, '')"_s));
    QVERIFY(!m_db->db().lastError().isEmpty());
    QCOMPARE(m_db->countRows(u"study_sessions"_s), 0);

    QVERIFY(m_db->exec(
        u"INSERT INTO study_sessions (topic_id, session_date, minutes, note) "
        u"VALUES (NULL, '2026-10-04', 45, '')"_s));
    QCOMPARE(m_db->countRows(u"study_sessions"_s), 1);
}

void tst_Database::foreignKeyDeleteActionsMatchSchema() {
    const auto actions = [this](const QString& child) {
        QHash<QString, QString> result;
        for (const QSqlRecord& record : m_db->records(u"PRAGMA foreign_key_list("_s + child +
                                                        u")"_s)) {
            result.insert(record.value(2).toString(), record.value(6).toString());
        }
        return result;
    };

    const QHash<QString, QString> topics = actions(u"topics"_s);
    QCOMPARE(topics.value(u"tracks"_s), u"CASCADE"_s);

    const QHash<QString, QString> sessions = actions(u"study_sessions"_s);
    QCOMPARE(sessions.value(u"topics"_s), u"SET NULL"_s);

    const QHash<QString, QString> history = actions(u"status_history"_s);
    QCOMPARE(history.value(u"applications"_s), u"CASCADE"_s);

    const QHash<QString, QString> interviews = actions(u"interviews"_s);
    QCOMPARE(interviews.value(u"applications"_s), u"CASCADE"_s);
}

void tst_Database::foreignKeysCascadeAndSetNull() {
    QVERIFY(m_db->exec(u"INSERT INTO tracks (name, color) VALUES ('C++', '#5B6CFF')"_s));
    const int trackId = m_db->scalar(u"SELECT last_insert_rowid()"_s).toInt();
    QVERIFY(m_db->exec(
        u"INSERT INTO topics (track_id, title, created_at, updated_at) VALUES ("_s +
        QString::number(trackId) + u", 'RAII', '2026-10-04T10:00:00', '2026-10-04T10:00:00')"_s));
    const int topicId = m_db->scalar(u"SELECT last_insert_rowid()"_s).toInt();
    QVERIFY(m_db->exec(u"INSERT INTO subtasks (topic_id, text) VALUES ("_s +
                       QString::number(topicId) + u", 'Read docs')"_s));
    QVERIFY(m_db->exec(
        u"INSERT INTO study_sessions (topic_id, session_date, minutes, note) VALUES ("_s +
        QString::number(topicId) + u", '2026-10-04', 30, '')"_s));

    QVERIFY(m_db->exec(u"DELETE FROM tracks WHERE id = "_s + QString::number(trackId)));

    QCOMPARE(m_db->countRows(u"topics"_s), 0);
    QCOMPARE(m_db->countRows(u"subtasks"_s), 0);
    QCOMPARE(m_db->countRows(u"study_sessions"_s), 1);
    QVERIFY(m_db->scalar(u"SELECT topic_id FROM study_sessions"_s).isNull());
}

void tst_Database::failedStatementReportsError() {
    QCOMPARE(m_db->db().lastError(), QString());
    MessageCapture capture;
    QVERIFY(!m_db->exec(u"SELECT * FROM does_not_exist"_s));
    QVERIFY(m_db->db().lastError().contains(u"does_not_exist"_s));
    // A failing statement is a real data-layer problem, so it reaches qCWarning.
    QVERIFY2(!capture.messages().isEmpty(), "a failed statement must log a warning");
    QVERIFY(capture.contains(u"does_not_exist"_s));
}

QTEST_GUILESS_MAIN(tst_Database)
#include "tst_Database.moc"
