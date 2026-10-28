#include "services/RestoreService.h"

#include <algorithm>
#include <chrono>
#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QUuid>
#include "data/Database.h"
#include "domain/EnumStrings.h"
#include "services/ExportService.h"
#include "services/ReminderService.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Services {

namespace {

constexpr qint64 kMaxBackupSize = 64 * 1024 * 1024;  // 64 MB

bool isValidIsoDate(const QString& str) {
    if (str.isEmpty()) return false;
    const QDate d = QDate::fromString(str, Qt::ISODate);
    return d.isValid();
}

bool isValidIsoDateTime(const QString& str) {
    if (str.isEmpty()) return false;
    const QDateTime dt = QDateTime::fromString(str, Qt::ISODate);
    return dt.isValid();
}

/// Worker running the restore logic on a dedicated thread with its own database connection.
class RestoreWorker : public QObject {
    Q_OBJECT

public:
    RestoreWorker(QString dbPath,
                  QString backupPath,
                  QString safetyDir,
                  RestoreService::Clock clock,
                  std::function<void()> delayHook = {})
        : m_dbPath(std::move(dbPath)),
          m_backupPath(std::move(backupPath)),
          m_safetyDir(std::move(safetyDir)),
          m_clock(std::move(clock)),
          m_delayHook(std::move(delayHook)) {}

public slots:
    void process() {
        if (m_delayHook) {
            m_delayHook();
        }
        RestoreResult result;
        auto workerDb = Data::Database::createWorkerConnection(m_dbPath);
        if (!workerDb) {
            result.success = false;
            result.errorKind = RestoreErrorKind::DatabaseError;
            result.errorDetail = u"Cannot open dedicated worker database connection"_s;
        } else {
            result = RestoreService::restoreFromFile(*workerDb, m_backupPath, m_safetyDir, m_clock);
            workerDb->close();
            workerDb.reset();
        }
        emit completed(result);
    }

signals:
    void completed(const JobPrep::Services::RestoreResult& result);

private:
    QString m_dbPath;
    QString m_backupPath;
    QString m_safetyDir;
    RestoreService::Clock m_clock;
    std::function<void()> m_delayHook;
};

}  // namespace

RestoreService::RestoreService(QObject* parent) : QObject(parent) {
    qRegisterMetaType<JobPrep::Services::RestoreResult>();
}

RestoreService::~RestoreService() {
    waitForRestore(5000);
}

bool RestoreService::isRestoring() const {
    return m_activeThread != nullptr && m_activeThread->isRunning();
}

bool RestoreService::waitForRestore(unsigned long timeoutMs) {
    if (!m_activeThread) {
        return true;
    }
    const auto start = std::chrono::steady_clock::now();
    while (m_activeThread && m_activeThread->isRunning()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (m_activeThread) {
            m_activeThread->wait(20);
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= static_cast<long long>(timeoutMs)) {
            break;
        }
    }
    const bool ok = !m_activeThread || !m_activeThread->isRunning();
    if (m_activeReminderService) {
        m_activeReminderService->resume();
        m_activeReminderService = nullptr;
    }
    return ok;
}

void RestoreService::setWorkerDelayHook(std::function<void()> hook) {
    m_workerDelayHook = std::move(hook);
}

RestoreResult RestoreService::validateJson(const QByteArray& jsonBytes) {
    if (jsonBytes.isEmpty()) {
        return {false, RestoreErrorKind::EmptyFile, u"Payload is empty"_s};
    }
    if (jsonBytes.size() > kMaxBackupSize) {
        return {false, RestoreErrorKind::FileTooLarge, u"Payload exceeds 64 MB"_s};
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || doc.isNull()) {
        return {false, RestoreErrorKind::InvalidJson, parseError.errorString()};
    }
    if (!doc.isObject()) {
        return {false, RestoreErrorKind::InvalidFormat, u"Root must be a JSON object"_s};
    }

    const QJsonObject root = doc.object();
    if (root[u"format"_s].toString() != u"jobprep-backup"_s) {
        return {false, RestoreErrorKind::InvalidFormat,
                u"Invalid format: %1"_s.arg(root[u"format"_s].toString())};
    }
    if (root[u"version"_s].toInt() != 1) {
        return {false, RestoreErrorKind::UnsupportedVersion,
                u"Unsupported version: %1"_s.arg(root[u"version"_s].toInt())};
    }

    const QStringList requiredArrays = {u"tracks"_s,       u"topics"_s,       u"subtasks"_s,
                                        u"sessions"_s,     u"applications"_s, u"statusHistory"_s,
                                        u"interviews"_s};
    for (const auto& key : requiredArrays) {
        if (!root.contains(key) || !root[key].isArray()) {
            return {false, RestoreErrorKind::MissingRequiredField,
                    u"Missing required table array: %1"_s.arg(key)};
        }
    }

    RestoreResult res;
    QSet<int> trackIds;
    QSet<int> topicIds;
    QSet<int> appIds;
    QSet<int> subtaskIds;
    QSet<int> sessionIds;
    QSet<int> historyIds;
    QSet<int> interviewIds;

    // 1. Tracks
    const QJsonArray tracks = root[u"tracks"_s].toArray();
    for (const auto& val : tracks) {
        if (!val.isObject()) return {false, RestoreErrorKind::InvalidFormat, u"Track is not an object"_s};
        const QJsonObject obj = val.toObject();
        const int id = obj[u"id"_s].toInt();
        if (id <= 0) return {false, RestoreErrorKind::InvalidId, u"Track id must be positive"_s};
        if (trackIds.contains(id)) return {false, RestoreErrorKind::DuplicateId, u"Duplicate track id: %1"_s.arg(id)};
        trackIds.insert(id);

        if (obj[u"name"_s].toString().trimmed().isEmpty()) {
            return {false, RestoreErrorKind::MissingRequiredField, u"Track %1 missing name"_s.arg(id)};
        }
    }
    res.restoredTracks = static_cast<int>(tracks.size());

    // 2. Topics
    const QJsonArray topics = root[u"topics"_s].toArray();
    for (const auto& val : topics) {
        if (!val.isObject()) return {false, RestoreErrorKind::InvalidFormat, u"Topic is not an object"_s};
        const QJsonObject obj = val.toObject();
        const int id = obj[u"id"_s].toInt();
        if (id <= 0) return {false, RestoreErrorKind::InvalidId, u"Topic id must be positive"_s};
        if (topicIds.contains(id)) return {false, RestoreErrorKind::DuplicateId, u"Duplicate topic id: %1"_s.arg(id)};
        topicIds.insert(id);

        const int trackId = obj[u"track_id"_s].toInt();
        if (!trackIds.contains(trackId)) {
            return {false, RestoreErrorKind::BrokenReference,
                    u"Topic %1 references non-existent track %2"_s.arg(id).arg(trackId)};
        }

        if (obj[u"title"_s].toString().trimmed().isEmpty()) {
            return {false, RestoreErrorKind::MissingRequiredField, u"Topic %1 missing title"_s.arg(id)};
        }

        const QString status = obj[u"status"_s].toString();
        if (!Domain::EnumStrings::topicStatusFromString(status).has_value()) {
            return {false, RestoreErrorKind::InvalidEnumValue, u"Topic %1 has invalid status: %2"_s.arg(id).arg(status)};
        }

        const QString priority = obj[u"priority"_s].toString();
        if (!Domain::EnumStrings::priorityFromString(priority).has_value()) {
            return {false, RestoreErrorKind::InvalidEnumValue, u"Topic %1 has invalid priority: %2"_s.arg(id).arg(priority)};
        }

        if (obj.contains(u"target_date"_s) && !obj[u"target_date"_s].isNull()) {
            const QString td = obj[u"target_date"_s].toString();
            if (!td.isEmpty() && !isValidIsoDate(td)) {
                return {false, RestoreErrorKind::InvalidDateFormat, u"Topic %1 has invalid target_date"_s.arg(id)};
            }
        }

        const QString createdAt = obj[u"created_at"_s].toString();
        const QString updatedAt = obj[u"updated_at"_s].toString();
        if (!isValidIsoDateTime(createdAt) || !isValidIsoDateTime(updatedAt)) {
            return {false, RestoreErrorKind::InvalidDateFormat, u"Topic %1 has invalid timestamps"_s.arg(id)};
        }
    }
    res.restoredTopics = static_cast<int>(topics.size());

    // 3. Subtasks
    const QJsonArray subtasks = root[u"subtasks"_s].toArray();
    for (const auto& val : subtasks) {
        if (!val.isObject()) return {false, RestoreErrorKind::InvalidFormat, u"Subtask is not an object"_s};
        const QJsonObject obj = val.toObject();
        const int id = obj[u"id"_s].toInt();
        if (id <= 0) return {false, RestoreErrorKind::InvalidId, u"Subtask id must be positive"_s};
        if (subtaskIds.contains(id)) return {false, RestoreErrorKind::DuplicateId, u"Duplicate subtask id: %1"_s.arg(id)};
        subtaskIds.insert(id);

        const int topicId = obj[u"topic_id"_s].toInt();
        if (!topicIds.contains(topicId)) {
            return {false, RestoreErrorKind::BrokenReference,
                    u"Subtask %1 references non-existent topic %2"_s.arg(id).arg(topicId)};
        }
    }
    res.restoredSubtasks = static_cast<int>(subtasks.size());

    // 4. Study Sessions
    const QJsonArray sessions = root[u"sessions"_s].toArray();
    for (const auto& val : sessions) {
        if (!val.isObject()) return {false, RestoreErrorKind::InvalidFormat, u"Session is not an object"_s};
        const QJsonObject obj = val.toObject();
        const int id = obj[u"id"_s].toInt();
        if (id <= 0) return {false, RestoreErrorKind::InvalidId, u"Session id must be positive"_s};
        if (sessionIds.contains(id)) return {false, RestoreErrorKind::DuplicateId, u"Duplicate session id: %1"_s.arg(id)};
        sessionIds.insert(id);

        if (obj.contains(u"topic_id"_s) && !obj[u"topic_id"_s].isNull()) {
            const int topicId = obj[u"topic_id"_s].toInt();
            if (topicId > 0 && !topicIds.contains(topicId)) {
                return {false, RestoreErrorKind::BrokenReference,
                        u"Session %1 references non-existent topic %2"_s.arg(id).arg(topicId)};
            }
        }

        const QString sessionDate = obj[u"session_date"_s].toString();
        if (!isValidIsoDate(sessionDate)) {
            return {false, RestoreErrorKind::InvalidDateFormat, u"Session %1 has invalid session_date"_s.arg(id)};
        }

        if (obj[u"minutes"_s].toInt() <= 0) {
            return {false, RestoreErrorKind::MissingRequiredField, u"Session %1 minutes must be positive"_s.arg(id)};
        }
    }
    res.restoredSessions = static_cast<int>(sessions.size());

    // 5. Applications
    const QJsonArray apps = root[u"applications"_s].toArray();
    for (const auto& val : apps) {
        if (!val.isObject()) return {false, RestoreErrorKind::InvalidFormat, u"Application is not an object"_s};
        const QJsonObject obj = val.toObject();
        const int id = obj[u"id"_s].toInt();
        if (id <= 0) return {false, RestoreErrorKind::InvalidId, u"Application id must be positive"_s};
        if (appIds.contains(id)) return {false, RestoreErrorKind::DuplicateId, u"Duplicate application id: %1"_s.arg(id)};
        appIds.insert(id);

        if (obj[u"company"_s].toString().trimmed().isEmpty() || obj[u"position"_s].toString().trimmed().isEmpty()) {
            return {false, RestoreErrorKind::MissingRequiredField, u"Application %1 missing company or position"_s.arg(id)};
        }

        const QString workMode = obj[u"work_mode"_s].toString();
        if (!Domain::EnumStrings::workModeFromString(workMode).has_value()) {
            return {false, RestoreErrorKind::InvalidEnumValue, u"Application %1 invalid work_mode: %2"_s.arg(id).arg(workMode)};
        }

        const QString status = obj[u"status"_s].toString();
        if (!Domain::EnumStrings::applicationStatusFromString(status).has_value()) {
            return {false, RestoreErrorKind::InvalidEnumValue, u"Application %1 invalid status: %2"_s.arg(id).arg(status)};
        }

        if (obj.contains(u"applied_date"_s) && !obj[u"applied_date"_s].isNull()) {
            const QString d = obj[u"applied_date"_s].toString();
            if (!d.isEmpty() && !isValidIsoDate(d)) {
                return {false, RestoreErrorKind::InvalidDateFormat, u"Application %1 invalid applied_date"_s.arg(id)};
            }
        }

        if (obj.contains(u"next_action_date"_s) && !obj[u"next_action_date"_s].isNull()) {
            const QString d = obj[u"next_action_date"_s].toString();
            if (!d.isEmpty() && !isValidIsoDate(d)) {
                return {false, RestoreErrorKind::InvalidDateFormat, u"Application %1 invalid next_action_date"_s.arg(id)};
            }
        }

        const QString createdAt = obj[u"created_at"_s].toString();
        const QString updatedAt = obj[u"updated_at"_s].toString();
        if (!isValidIsoDateTime(createdAt) || !isValidIsoDateTime(updatedAt)) {
            return {false, RestoreErrorKind::InvalidDateFormat, u"Application %1 invalid timestamps"_s.arg(id)};
        }
    }
    res.restoredApplications = static_cast<int>(apps.size());

    // 6. Status History
    const QJsonArray history = root[u"statusHistory"_s].toArray();
    for (const auto& val : history) {
        if (!val.isObject()) return {false, RestoreErrorKind::InvalidFormat, u"StatusHistory is not an object"_s};
        const QJsonObject obj = val.toObject();
        const int id = obj[u"id"_s].toInt();
        if (id <= 0) return {false, RestoreErrorKind::InvalidId, u"StatusHistory id must be positive"_s};
        if (historyIds.contains(id)) return {false, RestoreErrorKind::DuplicateId, u"Duplicate status history id: %1"_s.arg(id)};
        historyIds.insert(id);

        const int appId = obj[u"application_id"_s].toInt();
        if (!appIds.contains(appId)) {
            return {false, RestoreErrorKind::BrokenReference,
                    u"StatusHistory %1 references non-existent application %2"_s.arg(id).arg(appId)};
        }

        if (obj.contains(u"from_status"_s) && !obj[u"from_status"_s].isNull()) {
            const QString fs = obj[u"from_status"_s].toString();
            if (!fs.isEmpty() && !Domain::EnumStrings::applicationStatusFromString(fs).has_value()) {
                return {false, RestoreErrorKind::InvalidEnumValue, u"StatusHistory %1 invalid from_status: %2"_s.arg(id).arg(fs)};
            }
        }

        const QString toStatus = obj[u"to_status"_s].toString();
        if (!Domain::EnumStrings::applicationStatusFromString(toStatus).has_value()) {
            return {false, RestoreErrorKind::InvalidEnumValue, u"StatusHistory %1 invalid to_status: %2"_s.arg(id).arg(toStatus)};
        }

        const QString changedAt = obj[u"changed_at"_s].toString();
        if (!isValidIsoDateTime(changedAt)) {
            return {false, RestoreErrorKind::InvalidDateFormat, u"StatusHistory %1 invalid changed_at"_s.arg(id)};
        }
    }
    res.restoredStatusHistory = static_cast<int>(history.size());

    // 7. Interviews
    const QJsonArray interviews = root[u"interviews"_s].toArray();
    for (const auto& val : interviews) {
        if (!val.isObject()) return {false, RestoreErrorKind::InvalidFormat, u"Interview is not an object"_s};
        const QJsonObject obj = val.toObject();
        const int id = obj[u"id"_s].toInt();
        if (id <= 0) return {false, RestoreErrorKind::InvalidId, u"Interview id must be positive"_s};
        if (interviewIds.contains(id)) return {false, RestoreErrorKind::DuplicateId, u"Duplicate interview id: %1"_s.arg(id)};
        interviewIds.insert(id);

        const int appId = obj[u"application_id"_s].toInt();
        if (!appIds.contains(appId)) {
            return {false, RestoreErrorKind::BrokenReference,
                    u"Interview %1 references non-existent application %2"_s.arg(id).arg(appId)};
        }

        const QString startAt = obj[u"start_at"_s].toString();
        if (!isValidIsoDateTime(startAt)) {
            return {false, RestoreErrorKind::InvalidDateFormat, u"Interview %1 invalid start_at"_s.arg(id)};
        }

        if (obj[u"duration_min"_s].toInt() <= 0) {
            return {false, RestoreErrorKind::MissingRequiredField, u"Interview %1 duration must be positive"_s.arg(id)};
        }

        const QString typeStr = obj[u"type"_s].toString();
        if (!Domain::EnumStrings::interviewTypeFromString(typeStr).has_value()) {
            return {false, RestoreErrorKind::InvalidEnumValue, u"Interview %1 invalid type: %2"_s.arg(id).arg(typeStr)};
        }

        const QString outcomeStr = obj[u"outcome"_s].toString();
        if (!Domain::EnumStrings::interviewOutcomeFromString(outcomeStr).has_value()) {
            return {false, RestoreErrorKind::InvalidEnumValue, u"Interview %1 invalid outcome: %2"_s.arg(id).arg(outcomeStr)};
        }
    }
    res.restoredInterviews = static_cast<int>(interviews.size());

    res.success = true;
    res.errorKind = RestoreErrorKind::None;
    return res;
}

void RestoreService::pruneSafetyBackups(const QString& dir, int keepCount) {
    QDir d(dir);
    if (!d.exists()) return;

    // Matches only pre-restore-*.json files
    const QStringList nameFilters = {u"pre-restore-*.json"_s};
    QFileInfoList list = d.entryInfoList(nameFilters, QDir::Files, QDir::Name);

    if (list.size() > keepCount) {
        const int toRemove = static_cast<int>(list.size()) - keepCount;
        for (int i = 0; i < toRemove; ++i) {
            QFile::remove(list.at(i).absoluteFilePath());
        }
    }
}

RestoreResult RestoreService::restoreDatabase(Data::Database& db,
                                             const QByteArray& jsonBytes,
                                             const QString& safetyBackupDir,
                                             Clock clock) {
    // 1. Validation before touching the database
    RestoreResult res = validateJson(jsonBytes);
    if (!res.success) {
        return res;
    }

    // 2. Create safety backup if safetyBackupDir is provided
    if (!safetyBackupDir.isEmpty()) {
        QDir d(safetyBackupDir);
        if (!d.exists() && !d.mkpath(safetyBackupDir)) {
            return {false, RestoreErrorKind::SafetyBackupFailed,
                    u"Cannot create safety backup directory %1"_s.arg(safetyBackupDir)};
        }

        const QDateTime now = clock ? clock() : QDateTime::currentDateTime();
        const QString ts = now.toString(u"yyyyMMdd-HHmmss"_s);
        const QString backupPath = d.filePath(u"pre-restore-%1.json"_s.arg(ts));

        const QByteArray currentDbJson = ExportService::generateBackupJson(db, clock);
        QSaveFile saveFile(backupPath);
        if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return {false, RestoreErrorKind::SafetyBackupFailed, saveFile.errorString()};
        }
        if (saveFile.write(currentDbJson) != currentDbJson.size()) {
            return {false, RestoreErrorKind::SafetyBackupFailed, saveFile.errorString()};
        }
        if (!saveFile.commit()) {
            return {false, RestoreErrorKind::SafetyBackupFailed, saveFile.errorString()};
        }

        pruneSafetyBackups(safetyBackupDir, 5);
        res.safetyBackupPath = backupPath;
    }

    // 3. Transactional wipe and restore
    const QJsonObject root = QJsonDocument::fromJson(jsonBytes).object();
    auto tx = db.transaction();

    QSqlQuery q(db.connection());
    if (!q.exec(u"PRAGMA defer_foreign_keys=ON;"_s)) {
        tx.rollback();
        return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
    }

    // Wipe tables (children before parents, and wipe reminders_sent)
    const QStringList wipeSqls = {
        u"DELETE FROM interviews;"_s,
        u"DELETE FROM status_history;"_s,
        u"DELETE FROM applications;"_s,
        u"DELETE FROM study_sessions;"_s,
        u"DELETE FROM subtasks;"_s,
        u"DELETE FROM topics;"_s,
        u"DELETE FROM tracks;"_s,
        u"DELETE FROM reminders_sent;"_s
    };
    for (const auto& sql : wipeSqls) {
        if (!q.exec(sql)) {
            tx.rollback();
            return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
        }
    }

    // Insert tracks
    if (!q.prepare(u"INSERT INTO tracks (id, name, color, icon, position) VALUES (?, ?, ?, ?, ?);"_s)) {
        tx.rollback();
        return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
    }
    for (const auto& val : root[u"tracks"_s].toArray()) {
        const QJsonObject o = val.toObject();
        q.bindValue(0, o[u"id"_s].toInt());
        q.bindValue(1, o[u"name"_s].toString());
        q.bindValue(2, o[u"color"_s].toString());
        q.bindValue(3, o[u"icon"_s].toString());
        q.bindValue(4, o[u"position"_s].toInt());
        if (!q.exec()) {
            tx.rollback();
            return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
        }
    }

    // Insert topics
    if (!q.prepare(u"INSERT INTO topics (id, track_id, title, status, priority, target_date, notes, resources, tags, position, created_at, updated_at) "
                   u"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"_s)) {
        tx.rollback();
        return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
    }
    for (const auto& val : root[u"topics"_s].toArray()) {
        const QJsonObject o = val.toObject();
        q.bindValue(0, o[u"id"_s].toInt());
        q.bindValue(1, o[u"track_id"_s].toInt());
        q.bindValue(2, o[u"title"_s].toString());
        q.bindValue(3, o[u"status"_s].toString());
        q.bindValue(4, o[u"priority"_s].toString());
        q.bindValue(5, o[u"target_date"_s].isNull() ? QVariant() : o[u"target_date"_s].toString());
        q.bindValue(6, o[u"notes"_s].toString());
        q.bindValue(7, o[u"resources"_s].toString());
        q.bindValue(8, o[u"tags"_s].toString());
        q.bindValue(9, o[u"position"_s].toInt());
        q.bindValue(10, o[u"created_at"_s].toString());
        q.bindValue(11, o[u"updated_at"_s].toString());
        if (!q.exec()) {
            tx.rollback();
            return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
        }
    }

    // Insert subtasks
    if (!q.prepare(u"INSERT INTO subtasks (id, topic_id, text, done, position) VALUES (?, ?, ?, ?, ?);"_s)) {
        tx.rollback();
        return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
    }
    for (const auto& val : root[u"subtasks"_s].toArray()) {
        const QJsonObject o = val.toObject();
        q.bindValue(0, o[u"id"_s].toInt());
        q.bindValue(1, o[u"topic_id"_s].toInt());
        q.bindValue(2, o[u"text"_s].toString());
        q.bindValue(3, o[u"done"_s].toInt());
        q.bindValue(4, o[u"position"_s].toInt());
        if (!q.exec()) {
            tx.rollback();
            return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
        }
    }

    // Insert study sessions
    if (!q.prepare(u"INSERT INTO study_sessions (id, topic_id, session_date, minutes, note) VALUES (?, ?, ?, ?, ?);"_s)) {
        tx.rollback();
        return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
    }
    for (const auto& val : root[u"sessions"_s].toArray()) {
        const QJsonObject o = val.toObject();
        q.bindValue(0, o[u"id"_s].toInt());
        q.bindValue(1, o[u"topic_id"_s].isNull() ? QVariant() : o[u"topic_id"_s].toInt());
        q.bindValue(2, o[u"session_date"_s].toString());
        q.bindValue(3, o[u"minutes"_s].toInt());
        q.bindValue(4, o[u"note"_s].toString());
        if (!q.exec()) {
            tx.rollback();
            return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
        }
    }

    // Insert applications
    if (!q.prepare(u"INSERT INTO applications (id, company, position, url, source, resume_version, location, work_mode, salary_min, salary_max, currency, status, applied_date, next_action, next_action_date, contact_name, contact_email, notes, created_at, updated_at) "
                   u"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"_s)) {
        tx.rollback();
        return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
    }
    for (const auto& val : root[u"applications"_s].toArray()) {
        const QJsonObject o = val.toObject();
        q.bindValue(0, o[u"id"_s].toInt());
        q.bindValue(1, o[u"company"_s].toString());
        q.bindValue(2, o[u"position"_s].toString());
        q.bindValue(3, o[u"url"_s].toString());
        q.bindValue(4, o[u"source"_s].toString());
        q.bindValue(5, o[u"resume_version"_s].toString());
        q.bindValue(6, o[u"location"_s].toString());
        q.bindValue(7, o[u"work_mode"_s].toString());
        q.bindValue(8, o[u"salary_min"_s].isNull() ? QVariant() : o[u"salary_min"_s].toInt());
        q.bindValue(9, o[u"salary_max"_s].isNull() ? QVariant() : o[u"salary_max"_s].toInt());
        q.bindValue(10, o[u"currency"_s].toString());
        q.bindValue(11, o[u"status"_s].toString());
        q.bindValue(12, o[u"applied_date"_s].isNull() ? QVariant() : o[u"applied_date"_s].toString());
        q.bindValue(13, o[u"next_action"_s].toString());
        q.bindValue(14, o[u"next_action_date"_s].isNull() ? QVariant() : o[u"next_action_date"_s].toString());
        q.bindValue(15, o[u"contact_name"_s].toString());
        q.bindValue(16, o[u"contact_email"_s].toString());
        q.bindValue(17, o[u"notes"_s].toString());
        q.bindValue(18, o[u"created_at"_s].toString());
        q.bindValue(19, o[u"updated_at"_s].toString());
        if (!q.exec()) {
            tx.rollback();
            return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
        }
    }

    // Insert status history
    if (!q.prepare(u"INSERT INTO status_history (id, application_id, from_status, to_status, changed_at, note) "
                   u"VALUES (?, ?, ?, ?, ?, ?);"_s)) {
        tx.rollback();
        return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
    }
    for (const auto& val : root[u"statusHistory"_s].toArray()) {
        const QJsonObject o = val.toObject();
        q.bindValue(0, o[u"id"_s].toInt());
        q.bindValue(1, o[u"application_id"_s].toInt());
        q.bindValue(2, o[u"from_status"_s].isNull() ? QVariant() : o[u"from_status"_s].toString());
        q.bindValue(3, o[u"to_status"_s].toString());
        q.bindValue(4, o[u"changed_at"_s].toString());
        q.bindValue(5, o[u"note"_s].toString());
        if (!q.exec()) {
            tx.rollback();
            return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
        }
    }

    // Insert interviews
    if (!q.prepare(u"INSERT INTO interviews (id, application_id, start_at, duration_min, type, place, interviewer, notes, outcome) "
                   u"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);"_s)) {
        tx.rollback();
        return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
    }
    for (const auto& val : root[u"interviews"_s].toArray()) {
        const QJsonObject o = val.toObject();
        q.bindValue(0, o[u"id"_s].toInt());
        q.bindValue(1, o[u"application_id"_s].toInt());
        q.bindValue(2, o[u"start_at"_s].toString());
        q.bindValue(3, o[u"duration_min"_s].toInt());
        q.bindValue(4, o[u"type"_s].toString());
        q.bindValue(5, o[u"place"_s].toString());
        q.bindValue(6, o[u"interviewer"_s].toString());
        q.bindValue(7, o[u"notes"_s].toString());
        q.bindValue(8, o[u"outcome"_s].toString());
        if (!q.exec()) {
            tx.rollback();
            return {false, RestoreErrorKind::DatabaseError, q.lastError().text()};
        }
    }

    if (!tx.commit()) {
        return {false, RestoreErrorKind::DatabaseError, db.lastError()};
    }

    res.success = true;
    return res;
}

RestoreResult RestoreService::restoreFromFile(Data::Database& db,
                                             const QString& backupFilePath,
                                             const QString& safetyBackupDir,
                                             Clock clock) {
    QFile file(backupFilePath);
    if (!file.exists()) {
        return {false, RestoreErrorKind::CannotOpenFile, u"Backup file does not exist"_s};
    }
    const qint64 size = file.size();
    if (size == 0) {
        return {false, RestoreErrorKind::EmptyFile, u"Backup file is empty"_s};
    }
    if (size > kMaxBackupSize) {
        return {false, RestoreErrorKind::FileTooLarge, u"Backup file exceeds 64 MB"_s};
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return {false, RestoreErrorKind::CannotOpenFile, file.errorString()};
    }

    const QByteArray data = file.readAll();
    return restoreDatabase(db, data, safetyBackupDir, clock);
}

void RestoreService::restoreAsync(const QString& databasePath,
                                  const QString& backupFilePath,
                                  const QString& safetyBackupDir,
                                  ReminderService* reminderService,
                                  Clock clock) {
    if (reminderService) {
        reminderService->pause();
    }
    m_activeReminderService = reminderService;

    auto* thread = new QThread(this);
    m_activeThread = thread;
    auto* worker = new RestoreWorker(databasePath, backupFilePath, safetyBackupDir, clock, m_workerDelayHook);
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &RestoreWorker::process);
    connect(worker, &RestoreWorker::completed, this, [this, thread, worker](const RestoreResult& res) {
        if (m_activeReminderService) {
            m_activeReminderService->resume();
            m_activeReminderService = nullptr;
        }
        m_activeThread = nullptr;
        if (res.success) {
            emit dataReplaced();
        }
        emit finished(res);

        thread->quit();
        thread->wait();
        worker->deleteLater();
        thread->deleteLater();
    });

    thread->start();
}

}  // namespace JobPrep::Services

#include "RestoreService.moc"
