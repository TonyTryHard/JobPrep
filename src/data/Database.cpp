#include "data/Database.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include "data/DataLogging.h"
#include "data/Migrations.h"

Q_LOGGING_CATEGORY(lcData, "jobprep.data")

using namespace Qt::StringLiterals;

namespace JobPrep::Data {

namespace {

bool executeOn(const QSqlDatabase& connection, const QString& sql, QString* error) {
    QSqlQuery query(connection);
    if (query.exec(sql)) return true;
    if (error) {
        *error = u"%1 (while running: %2)"_s.arg(query.lastError().text(), sql.left(120));
    }
    return false;
}

}  // namespace

Database::Database(QString connectionName)
    : m_connectionName(connectionName.isEmpty() ? u"jobprep"_s : std::move(connectionName)) {}

Database::~Database() {
    close();
}

bool Database::open(const QString& path) {
    close();
    m_lastError.clear();
    m_path = path;

    if (!QSqlDatabase::isDriverAvailable(u"QSQLITE"_s)) {
        setError(u"The QSQLITE driver is not available."_s);
        return false;
    }

    if (path != u":memory:"_s) {
        const QString folder = QFileInfo(path).absolutePath();
        if (!folder.isEmpty() && !QDir().mkpath(folder)) {
            setError(u"Cannot create the data folder %1."_s.arg(folder));
            return false;
        }
    }

    {
        QSqlDatabase connection = QSqlDatabase::addDatabase(u"QSQLITE"_s, m_connectionName);
        connection.setDatabaseName(path);
        if (!connection.open()) {
            const QString message =
                u"Cannot open %1: %2"_s.arg(path, connection.lastError().text());
            QSqlDatabase::removeDatabase(m_connectionName);
            setError(message);
            return false;
        }
    }

    m_open = true;

    // WAL has to be set before any transaction; an in-memory database keeps "memory".
    execute(u"PRAGMA journal_mode=WAL"_s);
    execute(u"PRAGMA foreign_keys=ON"_s);

    QVariant foreignKeys;
    if (!readPragma(u"foreign_keys"_s, &foreignKeys) || foreignKeys.toInt() != 1) {
        setError(u"Cannot enable foreign keys support."_s);
        close();
        return false;
    }

    QString migrationError;
    if (!runMigrations(*this, migrations(), &migrationError)) {
        setError(migrationError);
        close();
        return false;
    }
    return true;
}

void Database::close() {
    if (!m_open) return;
    m_open = false;
    m_transactionDepth = 0;
    {
        // Qualified so the local name does not hide the member function.
        QSqlDatabase db = Database::connection();
        if (db.isOpen()) db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool Database::isOpen() const {
    return m_open;
}

QSqlDatabase Database::connection() const {
    return QSqlDatabase::database(m_connectionName, /*open=*/false);
}

QString Database::connectionName() const {
    return m_connectionName;
}

QString Database::path() const {
    return m_path;
}

QString Database::lastError() const {
    return m_lastError;
}

void Database::setError(const QString& message) const {
    m_lastError = message;
    qCWarning(lcData) << message;
}

void Database::setExpectedError(const QString& message) const {
    m_lastError = message;
}

bool Database::execute(const QString& sql) const {
    if (!m_open) {
        setError(u"Cannot run a statement on a closed database."_s);
        return false;
    }
    QString error;
    if (!executeOn(connection(), sql, &error)) {
        setError(error);
        return false;
    }
    return true;
}

Database::Transaction Database::transaction() const {
    if (!m_open) {
        setError(u"Cannot start a transaction on a closed database."_s);
        return Transaction();
    }
    if (m_transactionDepth > 0) return Transaction(const_cast<Database*>(this), false);
    if (!begin()) return Transaction();
    return Transaction(const_cast<Database*>(this), true);
}

bool Database::begin() const {
    QString error;
    if (!executeOn(connection(), u"BEGIN IMMEDIATE"_s, &error)) {
        setError(error);
        return false;
    }
    ++m_transactionDepth;
    return true;
}

bool Database::commitInternal() const {
    QString error;
    if (!executeOn(connection(), u"COMMIT"_s, &error)) {
        setError(error);
        return false;
    }
    return true;
}

void Database::rollbackInternal() const {
    QString error;
    if (!executeOn(connection(), u"ROLLBACK"_s, &error)) {
        setError(error);
    }
}

bool Database::readPragma(const QString& pragma, QVariant* value) const {
    QSqlQuery query(connection());
    if (!query.exec(u"PRAGMA "_s + pragma) || !query.next()) {
        setError(u"Cannot read PRAGMA %1."_s.arg(pragma));
        return false;
    }
    *value = query.value(0);
    return true;
}

Database::Transaction::Transaction(Database* database, bool owner)
    : m_database(database), m_active(true), m_owner(owner) {}

Database::Transaction::Transaction(Transaction&& other) noexcept
    : m_database(other.m_database), m_active(other.m_active), m_owner(other.m_owner) {
    other.m_database = nullptr;
    other.m_active = false;
    other.m_owner = false;
}

Database::Transaction& Database::Transaction::operator=(Transaction&& other) noexcept {
    if (this != &other) {
        if (m_active) rollback();
        m_database = other.m_database;
        m_active = other.m_active;
        m_owner = other.m_owner;
        other.m_database = nullptr;
        other.m_active = false;
        other.m_owner = false;
    }
    return *this;
}

Database::Transaction::~Transaction() {
    if (m_active) rollback();
}

bool Database::Transaction::commit() {
    if (!m_active) return true;
    if (!m_owner) {
        m_active = false;
        m_database = nullptr;
        return true;
    }

    bool ok = m_database->commitInternal();
    if (!ok) {
        m_database->rollbackInternal();
        Database* dbp = m_database;
        finish();
        if (dbp) dbp->clearPendingChanges();
        return false;
    }
    Database* dbp = m_database;
    finish();
    if (dbp) dbp->flushPendingChanges();
    return true;
}

void Database::Transaction::rollback() {
    if (!m_active) return;
    Database* dbp = m_database;
    if (m_owner && m_database) m_database->rollbackInternal();
    finish();
    if (dbp) dbp->clearPendingChanges();
}

bool Database::Transaction::isActive() const {
    return m_active;
}

bool Database::Transaction::isOwner() const {
    return m_owner;
}

void Database::Transaction::finish() {
    if (m_owner && m_database) --m_database->m_transactionDepth;
    m_database = nullptr;
    m_active = false;
    m_owner = false;
}

void Database::deferChanged(QObject* repo) const {
    if (!repo) return;
    m_pendingChangeRepos.insert(repo);
}

void Database::flushPendingChanges() const {
    const auto repos = m_pendingChangeRepos;
    for (const auto& ptr : repos) {
        if (ptr) {
            QMetaObject::invokeMethod(ptr, "changed", Qt::DirectConnection);
        }
    }
    m_pendingChangeRepos.clear();
}

void Database::clearPendingChanges() const {
    m_pendingChangeRepos.clear();
}

}  // namespace JobPrep::Data
