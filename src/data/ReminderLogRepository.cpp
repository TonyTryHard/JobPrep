#include "data/ReminderLogRepository.h"

#include <QSqlQuery>
#include <QVariant>
#include "data/Database.h"
#include "data/DbFormat.h"
#include "data/SqlStatement.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Data {

ReminderLogRepository::ReminderLogRepository(Database& database, QObject* parent)
    : Repository(parent), m_database(database) {}

ReminderLogRepository::~ReminderLogRepository() = default;

bool ReminderLogRepository::wasSent(QStringView key) const {
    SqlStatement statement(m_database, u"SELECT 1 FROM reminders_sent WHERE key = ?"_s);
    statement.bind(1, key.toString());
    if (!statement.exec()) return false;
    return statement.query().next();
}

bool ReminderLogRepository::markSent(const QString& key, const QDateTime& sentAt) {
    if (key.isEmpty()) {
        m_database.setExpectedError(u"A reminder key must not be empty."_s);
        return false;
    }

    // Nothing to do when the key is already logged: no transaction, no notification.
    if (wasSent(key)) return true;

    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database,
                           u"INSERT OR IGNORE INTO reminders_sent (key, sent_at) VALUES (?, ?)"_s);
    statement.bind(1, key);
    statement.bind(2, sentAt.isValid() ? sentAt : DbFormat::now());
    if (!statement.exec()) return false;
    return commitAndNotify(m_database, transaction);
}

bool ReminderLogRepository::remove(const QString& key) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database, u"DELETE FROM reminders_sent WHERE key = ?"_s);
    statement.bind(1, key);
    if (!statement.exec()) return false;
    if (statement.rowsAffected() != 1) {
        m_database.setExpectedError(u"No reminder sent for key %1."_s.arg(key));
        return false;
    }
    return commitAndNotify(m_database, transaction);
}

QList<QString> ReminderLogRepository::sentKeys() const {
    QList<QString> keys;
    SqlStatement statement(
        m_database, u"SELECT key FROM reminders_sent ORDER BY sent_at, key"_s);
    if (!statement.exec()) return keys;
    while (statement.query().next()) keys.append(statement.query().value(0).toString());
    return keys;
}

QString ReminderLogRepository::lastError() const {
    return m_database.lastError();
}

Database& ReminderLogRepository::database() const {
    return m_database;
}

}  // namespace JobPrep::Data
