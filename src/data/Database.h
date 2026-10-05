#pragma once

#include <QList>
#include <QObject>
#include <QPointer>
#include <QSqlDatabase>
#include <QString>

namespace JobPrep::Data {

class Repository;

/// Owns the SQLite connection: opens it, applies the pragmas and runs migrations.
class Database {
public:
    /// An empty `connectionName` falls back to the default connection name.
    explicit Database(QString connectionName = QString());
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&&) = delete;
    Database& operator=(Database&&) = delete;

    /// Opens `path` and brings the schema up to date. `:memory:` skips folder creation.
    bool open(const QString& path);
    void close();
    bool isOpen() const;

    /// Short-lived handle to the connection; callers must not keep it alive.
    QSqlDatabase connection() const;
    QString connectionName() const;
    QString path() const;

    /// Text of the last failed statement; never cleared by reads.
    QString lastError() const;

    /// Records an error and logs it: reserved for real SQL and migration failures.
    void setError(const QString& message) const;

    /// Records an expected error without logging. A missing row is a normal answer
    /// for a write by id, so it must never reach the warning log.
    void setExpectedError(const QString& message) const;

    /// Runs a statement without bindings (pragmas, migrations).
    bool execute(const QString& sql) const;

    /// RAII transaction: rolls back in the destructor unless commit() succeeded.
    /// A transaction created while another one is running joins the outer one, so
    /// services can wrap several repository writes into a single unit of work.
    class Transaction {
    public:
        /// Internal: instances are only produced by `Database::transaction()`, which
        /// is not a friend of this nested class.
        explicit Transaction(Database* database, bool owner);

        Transaction() = default;
        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;
        Transaction(Transaction&& other) noexcept;
        Transaction& operator=(Transaction&& other) noexcept;
        ~Transaction();

        bool commit();
        void rollback();
        bool isActive() const;
        bool isOwner() const;

    private:
        void finish();

        Database* m_database{nullptr};
        bool m_active{false};
        bool m_owner{false};
    };

    Transaction transaction();

    void deferChanged(Repository* repo);
    void flushPendingChanges();
    void clearPendingChanges();

private:
    bool begin();
    bool commitInternal();
    void rollbackInternal();
    bool readPragma(const QString& pragma, QVariant* value) const;
    void markOuterFailed();

    QString m_connectionName;
    QString m_path;
    mutable QString m_lastError;
    int m_transactionDepth{0};
    // QPointer, not a raw pointer: a repository may die inside the outer unit of work.
    // QObject rather than Repository so the header does not need Repository complete.
    QList<QPointer<QObject>> m_pendingChangeRepos;
    bool m_open{false};
    bool m_outerFailed{false};
};

}  // namespace JobPrep::Data
