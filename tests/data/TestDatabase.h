#pragma once

#include <memory>
#include <QList>
#include <QSqlRecord>
#include <QString>
#include <QStringView>
#include <QVariant>
#include "data/Database.h"

namespace JobPrep::Tests {

/// Owns one database for a data-layer test and exposes read-only introspection
/// helpers. Tests use `:memory:` so nothing is written to disk.
class TestDatabase {
public:
    explicit TestDatabase(QString connectionName);
    ~TestDatabase();

    TestDatabase(const TestDatabase&) = delete;
    TestDatabase& operator=(const TestDatabase&) = delete;
    TestDatabase(TestDatabase&&) = delete;
    TestDatabase& operator=(TestDatabase&&) = delete;

    /// Opens `path` and applies all shipped migrations.
    bool open(const QString& path);

    JobPrep::Data::Database& db();
    QString connectionName() const;

    int userVersion() const;

    /// First column of the first row of `sql`; an invalid QVariant on failure.
    QVariant scalar(const QString& sql) const;

    /// All rows of `sql`, useful for PRAGMA queries.
    QList<QSqlRecord> records(const QString& sql) const;

    bool hasTable(QStringView name) const;
    bool hasIndex(QStringView name) const;

    bool exec(const QString& sql) const;
    int countRows(const QString& table) const;

private:
    std::unique_ptr<JobPrep::Data::Database> m_database;
};

}  // namespace JobPrep::Tests
