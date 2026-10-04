#pragma once

#include <optional>
#include <QDate>
#include <QDateTime>
#include <QSqlQuery>
#include <QString>
#include <QVariant>

namespace JobPrep::Data {

class Database;

/// Prepared statement with positional bindings. Every failure is reported through
/// `Database::lastError()`; the statement must not outlive the database.
///
/// Values are appended with `QSqlQuery::addBindValue()`, so the `index` arguments
/// must run 1..N in the order the placeholders appear. Binding out of order is
/// reported and makes `exec()` fail instead of silently filling the wrong column.
class SqlStatement {
public:
    SqlStatement(const Database& database, const QString& sql);
    ~SqlStatement();

    SqlStatement(const SqlStatement&) = delete;
    SqlStatement& operator=(const SqlStatement&) = delete;
    SqlStatement(SqlStatement&&) = delete;
    SqlStatement& operator=(SqlStatement&&) = delete;

    void bind(int index, std::nullptr_t);
    void bind(int index, int value);
    void bind(int index, bool value);
    void bind(int index, const QString& value);
    void bind(int index, const QDate& value);
    void bind(int index, const QDateTime& value);
    void bind(int index, const std::optional<int>& value);
    void bind(int index, const std::optional<QDate>& value);
    void bind(int index, const std::optional<QDateTime>& value);

    /// Executes the prepared statement once.
    bool exec();

    bool isValid() const;
    QSqlQuery& query() { return m_query; }
    const QSqlQuery& query() const { return m_query; }
    int lastInsertId() const;
    int rowsAffected() const;

private:
    /// Accepts `index` only when it is the next expected placeholder, and returns
    /// whether the following value may be appended.
    bool acceptBindIndex(int index);
    void addBoundValue(const QVariant& value);

    const Database& m_database;
    QSqlQuery m_query;
    int m_nextBindIndex{1};
    bool m_prepared{false};
    bool m_bindOrderOk{true};
};

}  // namespace JobPrep::Data
