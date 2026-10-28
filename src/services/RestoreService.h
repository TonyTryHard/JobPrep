#pragma once

#include <functional>
#include <memory>
#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QThread>
#include "domain/Structs.h"

namespace JobPrep::Data {
class Database;
}

namespace JobPrep::Services {

class ReminderService;

/// Structured error kinds for database restore operations (SPEC §10, Amendment C3).
enum class RestoreErrorKind {
    None,
    CannotOpenFile,
    FileTooLarge,
    EmptyFile,
    InvalidJson,
    InvalidFormat,
    UnsupportedVersion,
    DuplicateId,
    InvalidId,
    InvalidEnumValue,
    InvalidDateFormat,
    BrokenReference,
    MissingRequiredField,
    SafetyBackupFailed,
    DatabaseError
};

/// Result of a restore operation with structured error kind and restored row counts.
struct RestoreResult {
    bool success{false};
    RestoreErrorKind errorKind{RestoreErrorKind::None};
    QString errorDetail;
    QString safetyBackupPath;
    int restoredTracks{0};
    int restoredTopics{0};
    int restoredSubtasks{0};
    int restoredSessions{0};
    int restoredApplications{0};
    int restoredStatusHistory{0};
    int restoredInterviews{0};

    RestoreResult() = default;
    RestoreResult(bool ok, RestoreErrorKind err, QString detail = {}, QString safetyPath = {})
        : success(ok),
          errorKind(err),
          errorDetail(std::move(detail)),
          safetyBackupPath(std::move(safetyPath)) {}
};

/// Coordinates transactional JSON restore, schema validation, safety backups,
/// and background worker execution (SPEC §10, Amendments C1–C3, C6, C7).
class RestoreService : public QObject {
    Q_OBJECT

public:
    using Clock = std::function<QDateTime()>;

    explicit RestoreService(QObject* parent = nullptr);
    ~RestoreService() override;

    /// Validates backup JSON content in-memory without accessing any database.
    static RestoreResult validateJson(const QByteArray& jsonBytes);

    /// Synchronously executes a restore on an open database instance.
    /// Creates a safety backup, wipes existing tables, and inserts all records in a single transaction.
    /// If validation or restore fails, the database is rolled back and left untouched.
    static RestoreResult restoreDatabase(Data::Database& db,
                                         const QByteArray& jsonBytes,
                                         const QString& safetyBackupDir,
                                         Clock clock = {});

    /// Convenience wrapper: reads file from disk (checking <= 64 MB size limit) and restores.
    static RestoreResult restoreFromFile(Data::Database& db,
                                         const QString& backupFilePath,
                                         const QString& safetyBackupDir,
                                         Clock clock = {});

    /// Prunes safety backups in `dir` matching `pre-restore-*.json` keeping only the newest `keepCount`.
    static void pruneSafetyBackups(const QString& dir, int keepCount = 5);

    /// Asynchronous restore on a worker thread:
    /// 1. Pauses reminderService (if provided).
    /// 2. Spawns worker thread which opens its own connection via Database::createWorkerConnection.
    /// 3. Validates and executes restore.
    /// 4. Closes worker connection.
    /// 5. Resumes reminderService.
    /// 6. Emits dataReplaced() and finished(result).
    void restoreAsync(const QString& databasePath,
                      const QString& backupFilePath,
                      const QString& safetyBackupDir,
                      ReminderService* reminderService = nullptr,
                      Clock clock = {});

    /// Returns true if an asynchronous restore is currently in progress.
    bool isRestoring() const;

    /// Blocks up to timeoutMs waiting for an active restore worker thread to finish.
    /// Resumes the reminder service if it was paused.
    /// Returns true if no restore is running or the restore finished within the timeout.
    bool waitForRestore(unsigned long timeoutMs = 5000);

    /// Test hook: injected delay/callback invoked inside the worker process method.
    void setWorkerDelayHook(std::function<void()> hook);

signals:
    void dataReplaced();
    void finished(const JobPrep::Services::RestoreResult& result);

private:
    QPointer<QThread> m_activeThread;
    ReminderService* m_activeReminderService{nullptr};
    std::function<void()> m_workerDelayHook;
};

}  // namespace JobPrep::Services

Q_DECLARE_METATYPE(JobPrep::Services::RestoreResult)
