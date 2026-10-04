#include "data/SessionRepository.h"

#include <QSqlQuery>
#include <QVariant>
#include "data/Database.h"
#include "data/DbFormat.h"
#include "data/SqlStatement.h"
#include "domain/Structs.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Data {

namespace {

QString selectSessions() {
    static const QString sql =
        u"SELECT id, topic_id, session_date, minutes, note FROM study_sessions"_s;
    return sql;
}

Domain::StudySession readSession(const QSqlQuery& query) {
    Domain::StudySession session;
    session.id = query.value(0).toInt();
    session.topicId = query.value(1).isNull() ? std::optional<int>()
                                              : std::optional<int>(query.value(1).toInt());
    session.sessionDate = DbFormat::dateFromValue(query.value(2)).value_or(QDate());
    session.minutes = query.value(3).toInt();
    session.note = query.value(4).toString();
    return session;
}

void bindSession(SqlStatement& statement, const Domain::StudySession& session) {
    statement.bind(1, session.topicId);
    statement.bind(2, session.sessionDate);
    statement.bind(3, session.minutes);
    statement.bind(4, session.note);
}

}  // namespace

SessionRepository::SessionRepository(Database& database, QObject* parent)
    : QObject(parent), m_database(database) {}

SessionRepository::~SessionRepository() = default;

QList<Domain::StudySession> SessionRepository::all() const {
    QList<Domain::StudySession> sessions;
    SqlStatement statement(m_database, selectSessions() + u" ORDER BY session_date, id"_s);
    if (!statement.exec()) return sessions;
    while (statement.query().next()) sessions.append(readSession(statement.query()));
    return sessions;
}

QList<Domain::StudySession> SessionRepository::byTopic(int topicId) const {
    QList<Domain::StudySession> sessions;
    SqlStatement statement(m_database,
                           selectSessions() + u" WHERE topic_id = ? ORDER BY session_date, id"_s);
    statement.bind(1, topicId);
    if (!statement.exec()) return sessions;
    while (statement.query().next()) sessions.append(readSession(statement.query()));
    return sessions;
}

QList<Domain::StudySession> SessionRepository::inRange(QDate from, QDate to) const {
    QList<Domain::StudySession> sessions;
    SqlStatement statement(m_database, selectSessions() +
                                              u" WHERE session_date >= ? AND session_date <= ? "
                                              u"ORDER BY session_date, id"_s);
    statement.bind(1, from);
    statement.bind(2, to);
    if (!statement.exec()) return sessions;
    while (statement.query().next()) sessions.append(readSession(statement.query()));
    return sessions;
}

std::optional<Domain::StudySession> SessionRepository::byId(int id) const {
    SqlStatement statement(m_database, selectSessions() + u" WHERE id = ?"_s);
    statement.bind(1, id);
    if (!statement.exec() || !statement.query().next()) return std::nullopt;
    return readSession(statement.query());
}

bool SessionRepository::insert(Domain::StudySession& session) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database,
                           u"INSERT INTO study_sessions (topic_id, session_date, minutes, note) "
                           u"VALUES (?, ?, ?, ?)"_s);
    bindSession(statement, session);
    if (!statement.exec() || !transaction.commit()) return false;

    session.id = statement.lastInsertId();
    emit changed();
    return true;
}

bool SessionRepository::update(const Domain::StudySession& session) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(
        m_database,
        u"UPDATE study_sessions SET topic_id = ?, session_date = ?, minutes = ?, note = ? "
        u"WHERE id = ?"_s);
    bindSession(statement, session);
    statement.bind(5, session.id);
    if (!statement.exec()) return false;
    if (statement.rowsAffected() != 1) {
        m_database.setExpectedError(
            u"No study session with id %1."_s.arg(QString::number(session.id)));
        return false;
    }
    if (!transaction.commit()) return false;

    emit changed();
    return true;
}

bool SessionRepository::remove(int id) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database, u"DELETE FROM study_sessions WHERE id = ?"_s);
    statement.bind(1, id);
    if (!statement.exec()) return false;
    if (statement.rowsAffected() != 1) {
        m_database.setExpectedError(u"No study session with id %1."_s.arg(QString::number(id)));
        return false;
    }
    if (!transaction.commit()) return false;

    emit changed();
    return true;
}

int SessionRepository::count() const {
    SqlStatement statement(m_database, u"SELECT COUNT(*) FROM study_sessions"_s);
    if (!statement.exec() || !statement.query().next()) return 0;
    return statement.query().value(0).toInt();
}

int SessionRepository::totalMinutesInRange(QDate from, QDate to) const {
    SqlStatement statement(m_database,
                           u"SELECT COALESCE(SUM(minutes), 0) FROM study_sessions "
                           u"WHERE session_date >= ? AND session_date <= ?"_s);
    statement.bind(1, from);
    statement.bind(2, to);
    if (!statement.exec() || !statement.query().next()) return 0;
    return statement.query().value(0).toInt();
}

QString SessionRepository::lastError() const {
    return m_database.lastError();
}

Database& SessionRepository::database() const {
    return m_database;
}

}  // namespace JobPrep::Data
