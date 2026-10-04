#include "data/InterviewRepository.h"

#include <QSqlQuery>
#include <QVariant>
#include "data/Database.h"
#include "data/DbFormat.h"
#include "data/SqlStatement.h"
#include "domain/EnumStrings.h"
#include "domain/Structs.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Data {

namespace {

QString selectInterviews() {
    static const QString sql =
        u"SELECT id, application_id, start_at, duration_min, type, place, interviewer, notes, "
        u"outcome FROM interviews"_s;
    return sql;
}

std::optional<Domain::Interview> readInterview(Database& database, const QSqlQuery& query) {
    const auto type = Domain::EnumStrings::interviewTypeFromString(query.value(4).toString());
    const auto outcome =
        Domain::EnumStrings::interviewOutcomeFromString(query.value(8).toString());
    if (!type || !outcome) {
        database.setError(u"Unknown interview type or outcome in row %1."_s.arg(
            QString::number(query.value(0).toInt())));
        return std::nullopt;
    }

    Domain::Interview interview;
    interview.id = query.value(0).toInt();
    interview.applicationId = query.value(1).toInt();
    interview.startAt = DbFormat::dateTimeFromValue(query.value(2)).value_or(QDateTime());
    interview.durationMin = query.value(3).toInt();
    interview.type = *type;
    interview.place = query.value(5).toString();
    interview.interviewer = query.value(6).toString();
    interview.notes = query.value(7).toString();
    interview.outcome = *outcome;
    return interview;
}

void bindInterview(SqlStatement& statement, const Domain::Interview& interview) {
    statement.bind(1, interview.applicationId);
    statement.bind(2, interview.startAt);
    statement.bind(3, interview.durationMin);
    statement.bind(4, Domain::EnumStrings::toString(interview.type));
    statement.bind(5, interview.place);
    statement.bind(6, interview.interviewer);
    statement.bind(7, interview.notes);
    statement.bind(8, Domain::EnumStrings::toString(interview.outcome));
}

QList<Domain::Interview> collect(Database& database, SqlStatement& statement) {
    QList<Domain::Interview> interviews;
    if (!statement.exec()) return interviews;
    while (statement.query().next()) {
        if (auto interview = readInterview(database, statement.query())) {
            interviews.append(*interview);
        }
    }
    return interviews;
}

}  // namespace

InterviewRepository::InterviewRepository(Database& database, QObject* parent)
    : QObject(parent), m_database(database) {}

InterviewRepository::~InterviewRepository() = default;

QList<Domain::Interview> InterviewRepository::all() const {
    SqlStatement statement(m_database, selectInterviews() + u" ORDER BY start_at, id"_s);
    return collect(m_database, statement);
}

QList<Domain::Interview> InterviewRepository::byApplication(int applicationId) const {
    SqlStatement statement(m_database, selectInterviews() +
                                          u" WHERE application_id = ? ORDER BY start_at, id"_s);
    statement.bind(1, applicationId);
    return collect(m_database, statement);
}

QList<Domain::Interview> InterviewRepository::inRange(const QDateTime& from,
                                                      const QDateTime& to) const {
    SqlStatement statement(m_database, selectInterviews() +
                                          u" WHERE start_at >= ? AND start_at <= ? "
                                          u"ORDER BY start_at, id"_s);
    statement.bind(1, from);
    statement.bind(2, to);
    return collect(m_database, statement);
}

std::optional<Domain::Interview> InterviewRepository::byId(int id) const {
    SqlStatement statement(m_database, selectInterviews() + u" WHERE id = ?"_s);
    statement.bind(1, id);
    if (!statement.exec() || !statement.query().next()) return std::nullopt;
    return readInterview(m_database, statement.query());
}

std::optional<Domain::Interview> InterviewRepository::nextUpcoming(const QDateTime& now) const {
    SqlStatement statement(m_database, selectInterviews() +
                                          u" WHERE start_at >= ? ORDER BY start_at, id LIMIT 1"_s);
    statement.bind(1, now);
    if (!statement.exec() || !statement.query().next()) return std::nullopt;
    return readInterview(m_database, statement.query());
}

bool InterviewRepository::insert(Domain::Interview& interview) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database,
                           u"INSERT INTO interviews (application_id, start_at, duration_min, "
                           u"type, place, interviewer, notes, outcome) "
                           u"VALUES (?, ?, ?, ?, ?, ?, ?, ?)"_s);
    bindInterview(statement, interview);
    if (!statement.exec() || !transaction.commit()) return false;

    interview.id = statement.lastInsertId();
    emit changed();
    return true;
}

bool InterviewRepository::update(const Domain::Interview& interview) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database,
                           u"UPDATE interviews SET application_id = ?, start_at = ?, "
                           u"duration_min = ?, type = ?, place = ?, interviewer = ?, notes = ?, "
                           u"outcome = ? WHERE id = ?"_s);
    bindInterview(statement, interview);
    statement.bind(9, interview.id);
    if (!statement.exec()) return false;
    if (statement.rowsAffected() != 1) {
        m_database.setExpectedError(
            u"No interview with id %1."_s.arg(QString::number(interview.id)));
        return false;
    }
    bool owner = transaction.isOwner();
    if (!transaction.commit()) return false;
    if (owner) emit changed();
    else m_database.deferChanged(this);
    return true;
}

bool InterviewRepository::remove(int id) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database, u"DELETE FROM interviews WHERE id = ?"_s);
    statement.bind(1, id);
    if (!statement.exec()) return false;
    if (statement.rowsAffected() != 1) {
        m_database.setExpectedError(u"No interview with id %1."_s.arg(QString::number(id)));
        return false;
    }
    bool owner = transaction.isOwner();
    if (!transaction.commit()) return false;
    if (owner) emit changed();
    else m_database.deferChanged(this);
    return true;
}

QString InterviewRepository::lastError() const {
    return m_database.lastError();
}

Database& InterviewRepository::database() const {
    return m_database;
}

}  // namespace JobPrep::Data
