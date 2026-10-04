#include "data/Migrations.h"

#include <QSqlError>
#include <QSqlQuery>

using namespace Qt::StringLiterals;

namespace JobPrep::Data {

namespace {

/// Runs one statement and reports the SQLite error, if any.
bool executeStatement(const QSqlDatabase& connection, const QString& sql, QString* error) {
    QSqlQuery query(connection);
    if (query.exec(sql)) return true;
    if (error) *error = query.lastError().text();
    return false;
}

}  // namespace

QList<Migration> migrations() {
    return {
        // SPEC §5, verbatim and split into single statements.
        Migration{1,
                  {
                      u"CREATE TABLE tracks (\n"
                      u"  id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE,\n"
                      u"  color TEXT NOT NULL, icon TEXT NOT NULL DEFAULT '', position INTEGER "
                      u"NOT NULL DEFAULT 0);"_s,

                      u"CREATE TABLE topics (\n"
                      u"  id INTEGER PRIMARY KEY,\n"
                      u"  track_id INTEGER NOT NULL REFERENCES tracks(id) ON DELETE CASCADE,\n"
                      u"  title TEXT NOT NULL, status TEXT NOT NULL DEFAULT 'backlog',\n"
                      u"  priority TEXT NOT NULL DEFAULT 'normal', target_date TEXT,\n"
                      u"  notes TEXT NOT NULL DEFAULT '', resources TEXT NOT NULL DEFAULT '[]', "
                      u"-- JSON [{title,url}]\n"
                      u"  tags TEXT NOT NULL DEFAULT '', position INTEGER NOT NULL DEFAULT 0,\n"
                      u"  created_at TEXT NOT NULL, updated_at TEXT NOT NULL);"_s,

                      u"CREATE TABLE subtasks (\n"
                      u"  id INTEGER PRIMARY KEY,\n"
                      u"  topic_id INTEGER NOT NULL REFERENCES topics(id) ON DELETE CASCADE,\n"
                      u"  text TEXT NOT NULL, done INTEGER NOT NULL DEFAULT 0, position INTEGER "
                      u"NOT NULL DEFAULT 0);"_s,

                      u"CREATE TABLE study_sessions (\n"
                      u"  id INTEGER PRIMARY KEY,\n"
                      u"  topic_id INTEGER REFERENCES topics(id) ON DELETE SET NULL,\n"
                      u"  session_date TEXT NOT NULL, minutes INTEGER NOT NULL CHECK (minutes > "
                      u"0),\n"
                      u"  note TEXT NOT NULL DEFAULT '');"_s,

                      u"CREATE TABLE applications (\n"
                      u"  id INTEGER PRIMARY KEY,\n"
                      u"  company TEXT NOT NULL, position TEXT NOT NULL,\n"
                      u"  url TEXT NOT NULL DEFAULT '', source TEXT NOT NULL DEFAULT '',\n"
                      u"  resume_version TEXT NOT NULL DEFAULT '', location TEXT NOT NULL "
                      u"DEFAULT '',\n"
                      u"  work_mode TEXT NOT NULL DEFAULT 'remote',\n"
                      u"  salary_min INTEGER, salary_max INTEGER, currency TEXT NOT NULL DEFAULT "
                      u"'USD',\n"
                      u"  status TEXT NOT NULL DEFAULT 'applied', applied_date TEXT,\n"
                      u"  next_action TEXT NOT NULL DEFAULT '', next_action_date TEXT,\n"
                      u"  contact_name TEXT NOT NULL DEFAULT '', contact_email TEXT NOT NULL "
                      u"DEFAULT '',\n"
                      u"  notes TEXT NOT NULL DEFAULT '',\n"
                      u"  created_at TEXT NOT NULL, updated_at TEXT NOT NULL);"_s,

                      u"CREATE INDEX idx_applications_status ON applications(status);"_s,

                      u"CREATE TABLE status_history (\n"
                      u"  id INTEGER PRIMARY KEY,\n"
                      u"  application_id INTEGER NOT NULL REFERENCES applications(id) ON DELETE "
                      u"CASCADE,\n"
                      u"  from_status TEXT, to_status TEXT NOT NULL,\n"
                      u"  changed_at TEXT NOT NULL, note TEXT NOT NULL DEFAULT '');"_s,

                      u"CREATE TABLE interviews (\n"
                      u"  id INTEGER PRIMARY KEY,\n"
                      u"  application_id INTEGER NOT NULL REFERENCES applications(id) ON DELETE "
                      u"CASCADE,\n"
                      u"  start_at TEXT NOT NULL, duration_min INTEGER NOT NULL DEFAULT 60,\n"
                      u"  type TEXT NOT NULL DEFAULT 'technical', place TEXT NOT NULL DEFAULT "
                      u"'',\n"
                      u"  interviewer TEXT NOT NULL DEFAULT '', notes TEXT NOT NULL DEFAULT "
                      u"'',\n"
                      u"  outcome TEXT NOT NULL DEFAULT 'pending');"_s,

                      u"CREATE INDEX idx_interviews_start ON interviews(start_at);"_s,

                      u"CREATE TABLE reminders_sent (key TEXT PRIMARY KEY, sent_at TEXT NOT "
                      u"NULL);"_s,
                  }},
    };
}

int currentUserVersion(Database& database) {
    QSqlQuery query(database.connection());
    if (!query.exec(u"PRAGMA user_version"_s) || !query.next()) return 0;
    return query.value(0).toInt();
}

bool runMigrations(Database& database, const QList<Migration>& list, QString* error) {
    const int current = currentUserVersion(database);

    for (const Migration& migration : list) {
        if (migration.version <= current) continue;

        QString failure;
        auto failWith = [&](const QString& reason) {
            QString ignored;
            executeStatement(database.connection(), u"ROLLBACK"_s, &ignored);
            if (error) {
                *error = u"Migration %1 failed: %2"_s.arg(QString::number(migration.version), reason);
            }
            return false;
        };

        if (!executeStatement(database.connection(), u"BEGIN IMMEDIATE"_s, &failure)) {
            return failWith(failure);
        }
        for (const QString& statement : migration.statements) {
            if (!executeStatement(database.connection(), statement, &failure)) {
                return failWith(failure);
            }
        }
        const QString version = u"PRAGMA user_version = "_s + QString::number(migration.version);
        if (!executeStatement(database.connection(), version, &failure)) {
            return failWith(failure);
        }
        if (!executeStatement(database.connection(), u"COMMIT"_s, &failure)) {
            return failWith(failure);
        }
    }
    return true;
}

}  // namespace JobPrep::Data
