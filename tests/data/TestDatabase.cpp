#include "TestDatabase.h"

#include <QSqlError>
#include <QSqlQuery>
#include "data/Migrations.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Tests {

TestDatabase::TestDatabase(QString connectionName)
    : m_database(std::make_unique<JobPrep::Data::Database>(std::move(connectionName))) {}

TestDatabase::~TestDatabase() = default;

bool TestDatabase::open(const QString& path) {
    return m_database->open(path);
}

JobPrep::Data::Database& TestDatabase::db() {
    return *m_database;
}

QString TestDatabase::connectionName() const {
    return m_database->connectionName();
}

int TestDatabase::userVersion() const {
    return JobPrep::Data::currentUserVersion(*m_database);
}

QVariant TestDatabase::scalar(const QString& sql) const {
    QSqlQuery query(m_database->connection());
    if (!query.exec(sql) || !query.next()) return {};
    return query.value(0);
}

QList<QSqlRecord> TestDatabase::records(const QString& sql) const {
    QList<QSqlRecord> result;
    QSqlQuery query(m_database->connection());
    if (!query.exec(sql)) return result;
    while (query.next()) result.append(query.record());
    return result;
}

bool TestDatabase::hasTable(QStringView name) const {
    return scalar(u"SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='"_s + name +
                  u"'"_s) == 1;
}

bool TestDatabase::hasIndex(QStringView name) const {
    return scalar(u"SELECT COUNT(*) FROM sqlite_master WHERE type='index' AND name='"_s + name +
                  u"'"_s) == 1;
}

bool TestDatabase::exec(const QString& sql) const {
    return m_database->execute(sql);
}

int TestDatabase::countRows(const QString& table) const {
    return scalar(u"SELECT COUNT(*) FROM "_s + table).toInt();
}

}  // namespace JobPrep::Tests
