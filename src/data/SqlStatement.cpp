#include "data/SqlStatement.h"

#include <QSqlError>
#include <QVariant>
#include "data/Database.h"
#include "data/DbFormat.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Data {

SqlStatement::SqlStatement(const Database& database, const QString& sql)
    : m_database(database), m_query(database.connection()) {
    m_prepared = m_query.prepare(sql);
    if (!m_prepared) {
        m_database.setError(u"Cannot prepare statement (%1): %2"_s.arg(
            sql.left(120), m_query.lastError().text()));
    }
}

SqlStatement::~SqlStatement() = default;

bool SqlStatement::acceptBindIndex(int index) {
    if (index == m_nextBindIndex) {
        ++m_nextBindIndex;
        return true;
    }
    m_bindOrderOk = false;
    m_database.setExpectedError(
        u"Statement value must be bound in order: expected index %1 but got %2."_s.arg(
            QString::number(m_nextBindIndex), QString::number(index)));
    return false;
}

void SqlStatement::addBoundValue(const QVariant& value) {
    m_query.addBindValue(value);
}

void SqlStatement::bind(int index, std::nullptr_t) {
    if (!acceptBindIndex(index)) return;
    addBoundValue(QVariant());
}

void SqlStatement::bind(int index, int value) {
    if (!acceptBindIndex(index)) return;
    addBoundValue(value);
}

void SqlStatement::bind(int index, bool value) {
    if (!acceptBindIndex(index)) return;
    addBoundValue(value ? 1 : 0);
}

void SqlStatement::bind(int index, const QString& value) {
    if (!acceptBindIndex(index)) return;
    // A default-constructed QString is a null QVariant, which the SQLite driver binds
    // as SQL NULL, so NOT NULL columns such as tracks.icon reject the row. The schema
    // declares DEFAULT '' for those columns, so store an empty string instead.
    addBoundValue(value.isNull() ? u""_s : value);
}

void SqlStatement::bind(int index, const QDate& value) {
    if (!acceptBindIndex(index)) return;
    addBoundValue(DbFormat::toText(value));
}

void SqlStatement::bind(int index, const QDateTime& value) {
    if (!acceptBindIndex(index)) return;
    addBoundValue(DbFormat::toText(value));
}

void SqlStatement::bind(int index, const std::optional<int>& value) {
    if (!acceptBindIndex(index)) return;
    if (value) {
        addBoundValue(*value);
    } else {
        addBoundValue(QVariant());
    }
}

void SqlStatement::bind(int index, const std::optional<QDate>& value) {
    if (!acceptBindIndex(index)) return;
    if (value) {
        addBoundValue(DbFormat::toText(*value));
    } else {
        addBoundValue(QVariant());
    }
}

void SqlStatement::bind(int index, const std::optional<QDateTime>& value) {
    if (!acceptBindIndex(index)) return;
    if (value) {
        addBoundValue(DbFormat::toText(*value));
    } else {
        addBoundValue(QVariant());
    }
}

bool SqlStatement::exec() {
    if (!m_prepared || !m_bindOrderOk) return false;
    if (!m_query.exec()) {
        m_database.setError(m_query.lastError().text());
        return false;
    }
    // Qt drops the bound values after a successful exec, so rebinding this statement
    // in a loop starts over at the first placeholder.
    m_nextBindIndex = 1;
    return true;
}

bool SqlStatement::isValid() const {
    return m_prepared;
}

int SqlStatement::lastInsertId() const {
    return m_query.lastInsertId().toInt();
}

int SqlStatement::rowsAffected() const {
    return m_query.numRowsAffected();
}

}  // namespace JobPrep::Data
