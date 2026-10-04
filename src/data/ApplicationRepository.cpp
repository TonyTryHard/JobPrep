#include "data/ApplicationRepository.h"

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

QString selectApplications() {
    static const QString sql =
        u"SELECT id, company, position, url, source, resume_version, location, work_mode, "
        u"salary_min, salary_max, currency, status, applied_date, next_action, next_action_date, "
        u"contact_name, contact_email, notes, created_at, updated_at FROM applications"_s;
    return sql;
}

QString selectHistory() {
    static const QString sql =
        u"SELECT id, application_id, from_status, to_status, changed_at, note FROM status_history"_s;
    return sql;
}

const QString& orderByHistoryChange() {
    static const QString sql = u" ORDER BY changed_at, id"_s;
    return sql;
}

std::optional<Domain::JobApplication> readApplication(Database& database,
                                                      const QSqlQuery& query) {
    const auto status = Domain::EnumStrings::applicationStatusFromString(query.value(11).toString());
    const auto workMode = Domain::EnumStrings::workModeFromString(query.value(7).toString());
    if (!status || !workMode) {
        database.setError(u"Unknown application status or work mode in row %1."_s.arg(
            QString::number(query.value(0).toInt())));
        return std::nullopt;
    }

    Domain::JobApplication application;
    application.id = query.value(0).toInt();
    application.company = query.value(1).toString();
    application.position = query.value(2).toString();
    application.url = query.value(3).toString();
    application.source = query.value(4).toString();
    application.resumeVersion = query.value(5).toString();
    application.location = query.value(6).toString();
    application.workMode = *workMode;
    application.salaryMin = query.value(8).isNull() ? std::optional<int>()
                                                    : std::optional<int>(query.value(8).toInt());
    application.salaryMax = query.value(9).isNull() ? std::optional<int>()
                                                    : std::optional<int>(query.value(9).toInt());
    application.currency = query.value(10).toString();
    application.status = *status;
    application.appliedDate = DbFormat::dateFromValue(query.value(12));
    application.nextAction = query.value(13).toString();
    application.nextActionDate = DbFormat::dateFromValue(query.value(14));
    application.contactName = query.value(15).toString();
    application.contactEmail = query.value(16).toString();
    application.notes = query.value(17).toString();
    application.createdAt = DbFormat::dateTimeFromValue(query.value(18)).value_or(QDateTime());
    application.updatedAt = DbFormat::dateTimeFromValue(query.value(19)).value_or(QDateTime());
    return application;
}

Domain::StatusChange readStatusChange(const QSqlQuery& query) {
    Domain::StatusChange change;
    change.id = query.value(0).toInt();
    change.applicationId = query.value(1).toInt();
    change.fromStatus = Domain::EnumStrings::applicationStatusFromString(query.value(2).toString());
    change.toStatus = Domain::EnumStrings::applicationStatusFromString(query.value(3).toString())
                          .value_or(Domain::ApplicationStatus::Applied);
    change.changedAt = DbFormat::dateTimeFromValue(query.value(4)).value_or(QDateTime());
    change.note = query.value(5).toString();
    return change;
}

/// SPEC §3.3 A5: reaching the applied stage without an applied date stamps today.
std::optional<QDate> appliedDateFor(Domain::ApplicationStatus status,
                                    const std::optional<QDate>& appliedDate) {
    if (status != Domain::ApplicationStatus::Applied || appliedDate.has_value()) return appliedDate;
    return QDate::currentDate();
}

void bindApplication(SqlStatement& statement, const Domain::JobApplication& application) {
    statement.bind(1, application.company);
    statement.bind(2, application.position);
    statement.bind(3, application.url);
    statement.bind(4, application.source);
    statement.bind(5, application.resumeVersion);
    statement.bind(6, application.location);
    statement.bind(7, Domain::EnumStrings::toString(application.workMode));
    statement.bind(8, application.salaryMin);
    statement.bind(9, application.salaryMax);
    statement.bind(10, application.currency);
    statement.bind(11, Domain::EnumStrings::toString(application.status));
    statement.bind(12, appliedDateFor(application.status, application.appliedDate));
    statement.bind(13, application.nextAction);
    statement.bind(14, application.nextActionDate);
    statement.bind(15, application.contactName);
    statement.bind(16, application.contactEmail);
    statement.bind(17, application.notes);
}

}  // namespace

ApplicationRepository::ApplicationRepository(Database& database, QObject* parent)
    : QObject(parent), m_database(database) {}

ApplicationRepository::~ApplicationRepository() = default;

QList<Domain::JobApplication> ApplicationRepository::all() const {
    QList<Domain::JobApplication> applications;
    SqlStatement statement(m_database,
                           selectApplications() + u" ORDER BY updated_at DESC, id DESC"_s);
    if (!statement.exec()) return applications;
    while (statement.query().next()) {
        if (auto application = readApplication(m_database, statement.query())) {
            applications.append(*application);
        }
    }
    return applications;
}

std::optional<Domain::JobApplication> ApplicationRepository::byId(int id) const {
    SqlStatement statement(m_database, selectApplications() + u" WHERE id = ?"_s);
    statement.bind(1, id);
    if (!statement.exec() || !statement.query().next()) return std::nullopt;
    return readApplication(m_database, statement.query());
}

QList<Domain::JobApplication> ApplicationRepository::byStatuses(
    const QList<Domain::ApplicationStatus>& statuses) const {
    QList<Domain::JobApplication> applications;
    if (statuses.isEmpty()) return applications;

    QString placeholders;
    for (int i = 0; i < statuses.size(); ++i) {
        if (i > 0) placeholders += u", "_s;
        placeholders += u"?"_s;
    }

    SqlStatement statement(m_database, selectApplications() + u" WHERE status IN ("_s +
                                              placeholders + u") ORDER BY updated_at DESC, "
                                                              u"id DESC"_s);
    for (int i = 0; i < statuses.size(); ++i) {
        statement.bind(i + 1, Domain::EnumStrings::toString(statuses.at(i)));
    }
    if (!statement.exec()) return applications;
    while (statement.query().next()) {
        if (auto application = readApplication(m_database, statement.query())) {
            applications.append(*application);
        }
    }
    return applications;
}

bool ApplicationRepository::insert(Domain::JobApplication& application) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;
    if (!insertRow(application)) return false;
    if (!transaction.commit()) return false;

    emit changed();
    return true;
}

bool ApplicationRepository::update(const Domain::JobApplication& application) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    const auto stored = byId(application.id);
    if (!stored) {
        m_database.setExpectedError(u"No application with id %1."_s.arg(QString::number(application.id)));
        return false;
    }

    if (!updateRow(application)) return false;
    if (stored->status != application.status &&
        !addHistoryRow(application.id, stored->status, application.status, DbFormat::now(),
                       QString())) {
        return false;
    }
    if (!transaction.commit()) return false;

    emit changed();
    return true;
}

bool ApplicationRepository::setStatus(int id, Domain::ApplicationStatus status,
                                      const QString& note) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    const auto stored = byId(id);
    if (!stored) {
        m_database.setExpectedError(u"No application with id %1."_s.arg(QString::number(id)));
        return false;
    }

    if (!updateStatusRow(id, status, stored->appliedDate)) return false;
    if (stored->status != status &&
        !addHistoryRow(id, stored->status, status, DbFormat::now(), note)) {
        return false;
    }
    if (!transaction.commit()) return false;

    emit changed();
    return true;
}

bool ApplicationRepository::setNextAction(int id, const QString& text, std::optional<QDate> date) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database,
                           u"UPDATE applications SET next_action = ?, next_action_date = ?, "
                           u"updated_at = ? WHERE id = ?"_s);
    statement.bind(1, text);
    statement.bind(2, date);
    statement.bind(3, DbFormat::now());
    statement.bind(4, id);
    if (!statement.exec()) return false;
    if (statement.rowsAffected() != 1) {
        m_database.setExpectedError(u"No application with id %1."_s.arg(QString::number(id)));
        return false;
    }
    if (!transaction.commit()) return false;

    emit changed();
    return true;
}

bool ApplicationRepository::duplicate(int id, Domain::JobApplication& copy) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    const auto stored = byId(id);
    if (!stored) {
        m_database.setExpectedError(u"No application with id %1."_s.arg(QString::number(id)));
        return false;
    }

    copy = *stored;
    copy.id = 0;
    if (!insertRow(copy)) return false;
    if (!transaction.commit()) return false;

    emit changed();
    return true;
}

bool ApplicationRepository::remove(int id) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database, u"DELETE FROM applications WHERE id = ?"_s);
    statement.bind(1, id);
    if (!statement.exec()) return false;
    if (statement.rowsAffected() != 1) {
        m_database.setExpectedError(u"No application with id %1."_s.arg(QString::number(id)));
        return false;
    }
    if (!transaction.commit()) return false;

    emit changed();
    return true;
}

int ApplicationRepository::count() const {
    SqlStatement statement(m_database, u"SELECT COUNT(*) FROM applications"_s);
    if (!statement.exec() || !statement.query().next()) return 0;
    return statement.query().value(0).toInt();
}

bool ApplicationRepository::isEmpty() const {
    return count() == 0;
}

QHash<Domain::ApplicationStatus, int> ApplicationRepository::countsByStatus() const {
    QHash<Domain::ApplicationStatus, int> counts;
    SqlStatement statement(m_database, u"SELECT status, COUNT(*) FROM applications "
                                      u"GROUP BY status"_s);
    if (!statement.exec()) return counts;
    while (statement.query().next()) {
        const auto status =
            Domain::EnumStrings::applicationStatusFromString(statement.query().value(0).toString());
        if (status) counts.insert(*status, statement.query().value(1).toInt());
    }
    return counts;
}

QList<Domain::StatusChange> ApplicationRepository::statusHistory(int applicationId) const {
    QList<Domain::StatusChange> changes;
    SqlStatement statement(
        m_database, selectHistory() + u" WHERE application_id = ?"_s + orderByHistoryChange());
    statement.bind(1, applicationId);
    if (!statement.exec()) return changes;
    while (statement.query().next()) changes.append(readStatusChange(statement.query()));
    return changes;
}

QList<Domain::StatusChange> ApplicationRepository::allStatusHistory() const {
    QList<Domain::StatusChange> changes;
    SqlStatement statement(m_database, selectHistory() + orderByHistoryChange());
    if (!statement.exec()) return changes;
    while (statement.query().next()) changes.append(readStatusChange(statement.query()));
    return changes;
}

QString ApplicationRepository::lastError() const {
    return m_database.lastError();
}

Database& ApplicationRepository::database() const {
    return m_database;
}

bool ApplicationRepository::insertRow(Domain::JobApplication& application) {
    application.appliedDate = appliedDateFor(application.status, application.appliedDate);

    SqlStatement statement(m_database,
                           u"INSERT INTO applications (company, position, url, source, "
                           u"resume_version, location, work_mode, salary_min, salary_max, "
                           u"currency, status, applied_date, next_action, next_action_date, "
                           u"contact_name, contact_email, notes, created_at, updated_at) "
                           u"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"_s);
    bindApplication(statement, application);
    application.createdAt = DbFormat::now();
    application.updatedAt = application.createdAt;
    statement.bind(18, application.createdAt);
    statement.bind(19, application.updatedAt);
    if (!statement.exec()) return false;

    application.id = statement.lastInsertId();
    return addHistoryRow(application.id, std::nullopt, application.status, application.createdAt,
                         QString());
}

bool ApplicationRepository::updateRow(const Domain::JobApplication& application) {
    SqlStatement statement(m_database,
                           u"UPDATE applications SET company = ?, position = ?, url = ?, source = "
                           u"?, resume_version = ?, location = ?, work_mode = ?, salary_min = ?, "
                           u"salary_max = ?, currency = ?, status = ?, applied_date = ?, "
                           u"next_action = ?, next_action_date = ?, contact_name = ?, "
                           u"contact_email = ?, notes = ?, updated_at = ? WHERE id = ?"_s);
    bindApplication(statement, application);
    statement.bind(18, DbFormat::now());
    statement.bind(19, application.id);
    return statement.exec();
}

bool ApplicationRepository::updateStatusRow(int id, Domain::ApplicationStatus status,
                                           const std::optional<QDate>& appliedDate) {
    SqlStatement statement(m_database, u"UPDATE applications SET status = ?, applied_date = ?, "
                                      u"updated_at = ? WHERE id = ?"_s);
    statement.bind(1, Domain::EnumStrings::toString(status));
    statement.bind(2, appliedDateFor(status, appliedDate));
    statement.bind(3, DbFormat::now());
    statement.bind(4, id);
    return statement.exec();
}

bool ApplicationRepository::addHistoryRow(int applicationId,
                                          std::optional<Domain::ApplicationStatus> from,
                                          Domain::ApplicationStatus to, const QDateTime& changedAt,
                                          const QString& note) {
    SqlStatement statement(m_database,
                           u"INSERT INTO status_history (application_id, from_status, to_status, "
                           u"changed_at, note) VALUES (?, ?, ?, ?, ?)"_s);
    statement.bind(1, applicationId);
    if (from) {
        statement.bind(2, Domain::EnumStrings::toString(*from));
    } else {
        statement.bind(2, nullptr);
    }
    statement.bind(3, Domain::EnumStrings::toString(to));
    statement.bind(4, changedAt);
    statement.bind(5, note);
    return statement.exec();
}

}  // namespace JobPrep::Data
