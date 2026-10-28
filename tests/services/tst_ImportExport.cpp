#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>
#include <QTimeZone>
#include <QTimer>
#include "TestDatabase.h"
#include "data/ApplicationRepository.h"
#include "data/Database.h"
#include "data/InterviewRepository.h"
#include "data/Migrations.h"
#include "data/ReminderLogRepository.h"
#include "data/SessionRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/Structs.h"
#include "services/AgendaService.h"
#include "services/DayWatcher.h"
#include "services/ExportService.h"
#include "services/ReminderService.h"
#include "services/RestoreService.h"
#include "services/SeedService.h"
#include "services/SettingsService.h"
#include "services/StatsService.h"
#include "ui/DashboardController.h"
#include "ui/theme/ThemeManager.h"
#include "ui/widgets/TopicDetailPanel.h"

using namespace Qt::StringLiterals;
using JobPrep::Data::Database;
using JobPrep::Domain::Interview;
using JobPrep::Domain::InterviewOutcome;
using JobPrep::Domain::InterviewType;
using JobPrep::Domain::JobApplication;
using JobPrep::Services::ExportErrorKind;
using JobPrep::Services::ExportResult;
using JobPrep::Services::ExportService;
using JobPrep::Services::RestoreErrorKind;
using JobPrep::Services::RestoreResult;
using JobPrep::Services::RestoreService;
using JobPrep::Tests::TestDatabase;

class tst_ImportExport : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // ICS export tests (C5)
    void testIcsCrlfOnly();
    void testIcsFolding200CharsAndUtf8Boundaries();
    void testIcsEscapingSpecialCharacters();
    void testIcsValarmAndProperties();
    void testIcsCancelledInterviewsExcluded();
    void testIcsEmptyListProducesValidCalendar();

    // JSON backup tests (C4, SPEC §10)
    void testJsonBackupStructureAndHeader();
    void testJsonBackupEveryColumnPresent();
    void testJsonBackupAtomicWriteAndReadOnlyError();

    // Restore tests (C1-C3, C6, C7)
    void testRestoreEqualityHandBuiltDataset();
    void testRestoreEqualitySeedDataset();
    void testRestoreInvalidFormatLeavesDbUntouched();
    void testRestoreInvalidVersionLeavesDbUntouched();
    void testRestoreTruncatedJsonLeavesDbUntouched();
    void testRestoreEmptyFileLeavesDbUntouched();
    void testRestoreBadEnumLeavesDbUntouched();
    void testRestoreBrokenReferenceLeavesDbUntouched();
    void testRestoreDuplicateIdLeavesDbUntouched();
    void testRestoreOversizedFileLeavesDbUntouched();
    void testRestoreSafetyBackupCreatedPrunedAndAbortsWhenReadOnly();
    void testRestoreRemindersSentWiped();
    void testRestoreRemindersPausedDuringRestore();
    void testRestoreIdAllocationAfterRestore();
    void testRestoreAsyncEventLoopAlive();
    void testRestorePerformanceSanity();
    void testRestoreCoalescedRefresh();
    void testRestoreStudyPanelWithVanishedTopic();

private:
    /// Test helper to unfold RFC 5545 lines and unescape characters for round-trip verification.
    static QString unfoldAndUnescape(const QByteArray& icsBytes);

    /// Structured dump of a single SQLite database table for byte-for-byte equality comparisons.
    struct TableDump {
        QString tableName;
        QStringList columnNames;
        QList<QList<QVariant>> rows;
    };

    static QList<TableDump> dumpDatabase(Database& db);
    static bool compareDumps(const QList<TableDump>& d1, const QList<TableDump>& d2, QString* diff = nullptr);
    static void wipeDatabase(Database& db);
    static void populateHandBuiltDataset(Database& db);
    void verifyInvalidJsonLeavesDbUntouched(const QByteArray& invalidJson, RestoreErrorKind expectedError);
};

void tst_ImportExport::initTestCase() {
}

QString tst_ImportExport::unfoldAndUnescape(const QByteArray& icsBytes) {
    QByteArray unfolded = icsBytes;
    // RFC 5545 unfolding: replace CRLF followed by space or tab with nothing
    unfolded.replace(QByteArray("\r\n "), QByteArray());
    unfolded.replace(QByteArray("\r\n\t"), QByteArray());

    QString str = QString::fromUtf8(unfolded);
    str.replace(u"\\n"_s, u"\n"_s);
    str.replace(u"\\N"_s, u"\n"_s);
    str.replace(u"\\,"_s, u","_s);
    str.replace(u"\\;"_s, u";"_s);
    str.replace(u"\\\\"_s, u"\\"_s);
    return str;
}

void tst_ImportExport::testIcsCrlfOnly() {
    Interview iv;
    iv.id = 1;
    iv.applicationId = 10;
    iv.startAt = QDateTime(QDate(2026, 10, 28), QTime(14, 0, 0));
    iv.durationMin = 45;
    iv.type = InterviewType::Technical;

    JobApplication app;
    app.id = 10;
    app.company = u"Acme Corp"_s;

    QHash<int, JobApplication> apps;
    apps.insert(10, app);

    const QByteArray bytes = ExportService::generateIcs({iv}, apps);
    QVERIFY(!bytes.isEmpty());

    // Must have CRLF
    QVERIFY(bytes.contains("\r\n"));

    // Must NOT have bare LF without preceding CR
    for (int i = 0; i < bytes.size(); ++i) {
        if (bytes[i] == '\n') {
            QVERIFY2(i > 0 && bytes[i - 1] == '\r', "Found bare LF in ICS output!");
        }
    }
}

void tst_ImportExport::testIcsFolding200CharsAndUtf8Boundaries() {
    // Construct a long text with Cyrillic characters and 4-byte emoji exceeding 200 octets
    const QString longText =
        u"Очень длинное примечание для технического собеседования по C++20 и Qt6. "
        u"Включает многобайтовые символы UTF-8: Привет, Мир! "
        u"А также эмодзи: 🎯 🚀 💻 💡 🔥 🌟. Проверяем строгую границу 75 октетов!"_s;
    QVERIFY(longText.toUtf8().size() > 200);

    const QByteArray folded = ExportService::foldIcsLine(u"DESCRIPTION:"_s + ExportService::escapeIcsText(longText));
    QVERIFY(!folded.isEmpty());

    // Split by \r\n and check each physical line's length
    const auto lines = folded.split('\n');
    for (const QByteArray& rawLine : lines) {
        QByteArray l = rawLine;
        if (l.endsWith('\r')) l.chop(1);
        if (l.isEmpty()) continue;
        // Excluding line break, line must be <= 75 octets
        QVERIFY2(l.size() <= 75, qPrintable(u"Folded line exceeds 75 octets: %1"_s.arg(l.size())));
        // Must be valid UTF-8 (never split in the middle of a multi-byte sequence)
        QString decoded = QString::fromUtf8(l);
        QVERIFY(!decoded.contains(QChar::ReplacementCharacter));
    }

    // Unfold and verify round-trip
    const QString unfolded = unfoldAndUnescape(folded);
    QVERIFY(unfolded.contains(longText));
}

void tst_ImportExport::testIcsEscapingSpecialCharacters() {
    const QString textWithSpecials = u"One\\Two;Three,Four\nLineTwo\r\nLineThree"_s;
    const QString escaped = ExportService::escapeIcsText(textWithSpecials);

    QCOMPARE(escaped, u"One\\\\Two\\;Three\\,Four\\nLineTwo\\nLineThree"_s);
}

void tst_ImportExport::testIcsValarmAndProperties() {
    const QDateTime fixedClock(QDate(2026, 10, 28), QTime(9, 30, 0), QTimeZone::UTC);

    Interview iv;
    iv.id = 42;
    iv.applicationId = 101;
    iv.startAt = QDateTime(QDate(2026, 10, 29), QTime(15, 0, 0));
    iv.durationMin = 60;
    iv.type = InterviewType::Final;
    iv.place = u"Room 404, Building B"_s;
    iv.interviewer = u"Alice & Bob"_s;
    iv.notes = u"Bring laptop"_s;

    JobApplication app;
    app.id = 101;
    app.company = u"TechNova Labs"_s;
    app.position = u"Senior Architect"_s;

    QHash<int, JobApplication> apps;
    apps.insert(101, app);

    const QByteArray ics = ExportService::generateIcs({iv}, apps, [fixedClock] { return fixedClock; });
    const QString content = QString::fromUtf8(ics);

    QVERIFY(content.contains(u"UID:jobprep-interview-42@jobprep.local\r\n"_s));
    QVERIFY(content.contains(u"DTSTAMP:20261028T093000Z\r\n"_s));
    QVERIFY(content.contains(u"DTSTART:20261029T150000\r\n"_s));
    QVERIFY(content.contains(u"DTEND:20261029T160000\r\n"_s));
    QVERIFY(content.contains(u"SUMMARY:Interview: TechNova Labs — Final\r\n"_s));
    QVERIFY(content.contains(u"LOCATION:Room 404\\, Building B\r\n"_s));
    QVERIFY(content.contains(u"BEGIN:VALARM\r\n"_s));
    QVERIFY(content.contains(u"ACTION:DISPLAY\r\n"_s));
    QVERIFY(content.contains(u"TRIGGER:-PT60M\r\n"_s));
    QVERIFY(content.contains(u"END:VALARM\r\n"_s));
}

void tst_ImportExport::testIcsCancelledInterviewsExcluded() {
    Interview ivActive;
    ivActive.id = 1;
    ivActive.applicationId = 10;
    ivActive.startAt = QDateTime(QDate(2026, 10, 30), QTime(10, 0, 0));
    ivActive.durationMin = 30;
    ivActive.outcome = InterviewOutcome::Pending;

    Interview ivCancelled;
    ivCancelled.id = 2;
    ivCancelled.applicationId = 10;
    ivCancelled.startAt = QDateTime(QDate(2026, 10, 30), QTime(11, 0, 0));
    ivCancelled.durationMin = 30;
    ivCancelled.outcome = InterviewOutcome::Cancelled;

    JobApplication app;
    app.id = 10;
    app.company = u"Globex"_s;

    QHash<int, JobApplication> apps;
    apps.insert(10, app);

    const QByteArray ics = ExportService::generateIcs({ivActive, ivCancelled}, apps);
    const QString content = QString::fromUtf8(ics);

    QVERIFY(content.contains(u"UID:jobprep-interview-1@jobprep.local"_s));
    QVERIFY(!content.contains(u"UID:jobprep-interview-2@jobprep.local"_s));
}

void tst_ImportExport::testIcsEmptyListProducesValidCalendar() {
    const QByteArray ics = ExportService::generateIcs({}, {});
    const QString content = QString::fromUtf8(ics);

    QVERIFY(content.startsWith(u"BEGIN:VCALENDAR\r\n"_s));
    QVERIFY(content.contains(u"VERSION:2.0\r\n"_s));
    QVERIFY(content.contains(u"PRODID:-//JobPrep//JobPrep Desktop//EN\r\n"_s));
    QVERIFY(content.contains(u"CALSCALE:GREGORIAN\r\n"_s));
    QVERIFY(content.endsWith(u"END:VCALENDAR\r\n"_s));
    QVERIFY(!content.contains(u"BEGIN:VEVENT"_s));
}

void tst_ImportExport::testJsonBackupStructureAndHeader() {
    const QDateTime fixedClock(QDate(2026, 10, 28), QTime(14, 15, 0), QTimeZone::UTC);

    TestDatabase testDb(u"tst_json_header"_s);
    QVERIFY(testDb.open(u":memory:"_s));

    const QByteArray jsonBytes = ExportService::generateBackupJson(testDb.db(), [fixedClock] { return fixedClock; });
    QVERIFY(!jsonBytes.isEmpty());

    const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes);
    QVERIFY(!doc.isNull());
    QVERIFY(doc.isObject());

    const QJsonObject root = doc.object();
    QCOMPARE(root[u"format"_s].toString(), u"jobprep-backup"_s);
    QCOMPARE(root[u"version"_s].toInt(), 1);
    QCOMPARE(root[u"exportedAt"_s].toString(), fixedClock.toString(Qt::ISODateWithMs));

    // Must contain all 7 table keys
    QVERIFY(root.contains(u"tracks"_s) && root[u"tracks"_s].isArray());
    QVERIFY(root.contains(u"topics"_s) && root[u"topics"_s].isArray());
    QVERIFY(root.contains(u"subtasks"_s) && root[u"subtasks"_s].isArray());
    QVERIFY(root.contains(u"sessions"_s) && root[u"sessions"_s].isArray());
    QVERIFY(root.contains(u"applications"_s) && root[u"applications"_s].isArray());
    QVERIFY(root.contains(u"statusHistory"_s) && root[u"statusHistory"_s].isArray());
    QVERIFY(root.contains(u"interviews"_s) && root[u"interviews"_s].isArray());
}

void tst_ImportExport::testJsonBackupEveryColumnPresent() {
    TestDatabase testDb(u"tst_json_cols"_s);
    QVERIFY(testDb.open(u":memory:"_s));

    // Insert sample row into each table
    QSqlQuery q(testDb.db().connection());
    QVERIFY(q.exec(u"INSERT INTO tracks (id, name, color, icon, position) VALUES (1, 'Track 1', '#123456', 'icon', 0)"_s));
    QVERIFY(q.exec(u"INSERT INTO topics (id, track_id, title, status, priority, target_date, notes, resources, tags, position, created_at, updated_at) "
                   u"VALUES (1, 1, 'Topic 1', 'backlog', 'high', '2026-11-01', 'Notes', '[]', 'tag1', 0, '2026-10-28T10:00:00', '2026-10-28T10:00:00')"_s));
    QVERIFY(q.exec(u"INSERT INTO subtasks (id, topic_id, text, done, position) VALUES (1, 1, 'Sub 1', 1, 0)"_s));
    QVERIFY(q.exec(u"INSERT INTO study_sessions (id, topic_id, session_date, minutes, note) VALUES (1, 1, '2026-10-28', 45, 'Session note')"_s));
    QVERIFY(q.exec(u"INSERT INTO applications (id, company, position, url, source, resume_version, location, work_mode, salary_min, salary_max, currency, status, applied_date, next_action, next_action_date, contact_name, contact_email, notes, created_at, updated_at) "
                   u"VALUES (1, 'Acme', 'Dev', 'url', 'src', 'v1', 'Remote', 'remote', 5000, 7000, 'USD', 'applied', '2026-10-28', 'Action', '2026-11-05', 'Bob', 'bob@test.com', 'App note', '2026-10-28T10:00:00', '2026-10-28T10:00:00')"_s));
    QVERIFY(q.exec(u"INSERT INTO status_history (id, application_id, from_status, to_status, changed_at, note) VALUES (1, 1, NULL, 'applied', '2026-10-28T10:00:00', 'Initial')"_s));
    QVERIFY(q.exec(u"INSERT INTO interviews (id, application_id, start_at, duration_min, type, place, interviewer, notes, outcome) "
                   u"VALUES (1, 1, '2026-10-30T14:00:00', 60, 'technical', 'Zoom', 'Alice', 'Prep notes', 'pending')"_s));

    const QByteArray jsonBytes = ExportService::generateBackupJson(testDb.db());
    const QJsonObject root = QJsonDocument::fromJson(jsonBytes).object();

    // Verify all columns of tracks
    const QJsonObject trackObj = root[u"tracks"_s].toArray().first().toObject();
    QVERIFY(trackObj.contains(u"id"_s));
    QVERIFY(trackObj.contains(u"name"_s));
    QVERIFY(trackObj.contains(u"color"_s));
    QVERIFY(trackObj.contains(u"icon"_s));
    QVERIFY(trackObj.contains(u"position"_s));

    // Verify all columns of topics
    const QJsonObject topicObj = root[u"topics"_s].toArray().first().toObject();
    QVERIFY(topicObj.contains(u"id"_s));
    QVERIFY(topicObj.contains(u"track_id"_s));
    QVERIFY(topicObj.contains(u"title"_s));
    QVERIFY(topicObj.contains(u"status"_s));
    QVERIFY(topicObj.contains(u"priority"_s));
    QVERIFY(topicObj.contains(u"target_date"_s));
    QVERIFY(topicObj.contains(u"notes"_s));
    QVERIFY(topicObj.contains(u"resources"_s));
    QVERIFY(topicObj.contains(u"tags"_s));
    QVERIFY(topicObj.contains(u"position"_s));
    QVERIFY(topicObj.contains(u"created_at"_s));
    QVERIFY(topicObj.contains(u"updated_at"_s));

    // Verify all columns of subtasks
    const QJsonObject subtaskObj = root[u"subtasks"_s].toArray().first().toObject();
    QVERIFY(subtaskObj.contains(u"id"_s));
    QVERIFY(subtaskObj.contains(u"topic_id"_s));
    QVERIFY(subtaskObj.contains(u"text"_s));
    QVERIFY(subtaskObj.contains(u"done"_s));
    QVERIFY(subtaskObj.contains(u"position"_s));

    // Verify all columns of study_sessions (sessions)
    const QJsonObject sessionObj = root[u"sessions"_s].toArray().first().toObject();
    QVERIFY(sessionObj.contains(u"id"_s));
    QVERIFY(sessionObj.contains(u"topic_id"_s));
    QVERIFY(sessionObj.contains(u"session_date"_s));
    QVERIFY(sessionObj.contains(u"minutes"_s));
    QVERIFY(sessionObj.contains(u"note"_s));

    // Verify all columns of applications
    const QJsonObject appObj = root[u"applications"_s].toArray().first().toObject();
    QVERIFY(appObj.contains(u"id"_s));
    QVERIFY(appObj.contains(u"company"_s));
    QVERIFY(appObj.contains(u"position"_s));
    QVERIFY(appObj.contains(u"url"_s));
    QVERIFY(appObj.contains(u"source"_s));
    QVERIFY(appObj.contains(u"resume_version"_s));
    QVERIFY(appObj.contains(u"location"_s));
    QVERIFY(appObj.contains(u"work_mode"_s));
    QVERIFY(appObj.contains(u"salary_min"_s));
    QVERIFY(appObj.contains(u"salary_max"_s));
    QVERIFY(appObj.contains(u"currency"_s));
    QVERIFY(appObj.contains(u"status"_s));
    QVERIFY(appObj.contains(u"applied_date"_s));
    QVERIFY(appObj.contains(u"next_action"_s));
    QVERIFY(appObj.contains(u"next_action_date"_s));
    QVERIFY(appObj.contains(u"contact_name"_s));
    QVERIFY(appObj.contains(u"contact_email"_s));
    QVERIFY(appObj.contains(u"notes"_s));
    QVERIFY(appObj.contains(u"created_at"_s));
    QVERIFY(appObj.contains(u"updated_at"_s));

    // Verify all columns of status_history (statusHistory)
    const QJsonObject historyObj = root[u"statusHistory"_s].toArray().first().toObject();
    QVERIFY(historyObj.contains(u"id"_s));
    QVERIFY(historyObj.contains(u"application_id"_s));
    QVERIFY(historyObj.contains(u"from_status"_s));
    QVERIFY(historyObj.contains(u"to_status"_s));
    QVERIFY(historyObj.contains(u"changed_at"_s));
    QVERIFY(historyObj.contains(u"note"_s));

    // Verify all columns of interviews
    const QJsonObject interviewObj = root[u"interviews"_s].toArray().first().toObject();
    QVERIFY(interviewObj.contains(u"id"_s));
    QVERIFY(interviewObj.contains(u"application_id"_s));
    QVERIFY(interviewObj.contains(u"start_at"_s));
    QVERIFY(interviewObj.contains(u"duration_min"_s));
    QVERIFY(interviewObj.contains(u"type"_s));
    QVERIFY(interviewObj.contains(u"place"_s));
    QVERIFY(interviewObj.contains(u"interviewer"_s));
    QVERIFY(interviewObj.contains(u"notes"_s));
    QVERIFY(interviewObj.contains(u"outcome"_s));
}

void tst_ImportExport::testJsonBackupAtomicWriteAndReadOnlyError() {
    ExportService svc;
    TestDatabase testDb(u"tst_json_atomic"_s);
    QVERIFY(testDb.open(u":memory:"_s));

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString targetPath = tempDir.filePath(u"backup.json"_s);

    // Initial write
    const auto res1 = svc.exportBackupJson(testDb.db(), targetPath);
    QVERIFY(res1.success);
    QCOMPARE(res1.errorKind, ExportErrorKind::None);
    QVERIFY(QFile::exists(targetPath));

    const qint64 origSize = QFileInfo(targetPath).size();
    QVERIFY(origSize > 0);

    // Attempting to write into a nonexistent directory
    const QString invalidPath = tempDir.filePath(u"nonexistent_subfolder/backup.json"_s);
    const auto res2 = svc.exportBackupJson(testDb.db(), invalidPath);
    QVERIFY(!res2.success);
    QCOMPARE(res2.errorKind, ExportErrorKind::CannotOpenFile);

    // Original file intact
    QCOMPARE(QFileInfo(targetPath).size(), origSize);
}

QList<tst_ImportExport::TableDump> tst_ImportExport::dumpDatabase(Database& db) {
    // Application schema tables to dump (excludes internal sqlite_sequence table)
    const QStringList tables = {
        u"tracks"_s,
        u"topics"_s,
        u"subtasks"_s,
        u"study_sessions"_s,
        u"applications"_s,
        u"status_history"_s,
        u"interviews"_s
    };
    QList<TableDump> dumps;
    for (const auto& table : tables) {
        TableDump td;
        td.tableName = table;
        QSqlQuery q(db.connection());
        q.exec(u"SELECT * FROM %1 ORDER BY id ASC;"_s.arg(table));
        const auto record = q.record();
        for (int i = 0; i < record.count(); ++i) {
            td.columnNames.append(record.fieldName(i));
        }
        while (q.next()) {
            QList<QVariant> row;
            for (int i = 0; i < record.count(); ++i) {
                row.append(q.value(i));
            }
            td.rows.append(row);
        }
        dumps.append(td);
    }
    return dumps;
}

bool tst_ImportExport::compareDumps(const QList<TableDump>& d1, const QList<TableDump>& d2, QString* diff) {
    if (d1.size() != d2.size()) {
        if (diff) *diff = u"Table count mismatch: %1 vs %2"_s.arg(d1.size()).arg(d2.size());
        return false;
    }
    for (int i = 0; i < d1.size(); ++i) {
        const auto& t1 = d1[i];
        const auto& t2 = d2[i];
        if (t1.tableName != t2.tableName) {
            if (diff) *diff = u"Table name mismatch at %1: %2 vs %3"_s.arg(i).arg(t1.tableName, t2.tableName);
            return false;
        }
        if (t1.columnNames != t2.columnNames) {
            if (diff) *diff = u"Columns mismatch in table %1"_s.arg(t1.tableName);
            return false;
        }
        if (t1.rows.size() != t2.rows.size()) {
            if (diff) *diff = u"Row count mismatch in table %1: %2 vs %3"_s.arg(t1.tableName).arg(t1.rows.size()).arg(t2.rows.size());
            return false;
        }
        for (int r = 0; r < t1.rows.size(); ++r) {
            const auto& r1 = t1.rows[r];
            const auto& r2 = t2.rows[r];
            for (int c = 0; c < r1.size(); ++c) {
                const auto& v1 = r1[c];
                const auto& v2 = r2[c];
                if (v1.isNull() != v2.isNull()) {
                    if (diff) *diff = u"Nullness mismatch in %1 row %2 col %3"_s.arg(t1.tableName).arg(r).arg(t1.columnNames[c]);
                    return false;
                }
                if (!v1.isNull() && v1 != v2) {
                    if (diff) *diff = u"Value mismatch in %1 row %2 col %3: '%4' vs '%5'"_s
                        .arg(t1.tableName).arg(r).arg(t1.columnNames[c], v1.toString(), v2.toString());
                    return false;
                }
            }
        }
    }
    return true;
}

void tst_ImportExport::wipeDatabase(Database& db) {
    QSqlQuery q(db.connection());
    q.exec(u"PRAGMA defer_foreign_keys=ON;"_s);
    q.exec(u"DELETE FROM interviews;"_s);
    q.exec(u"DELETE FROM status_history;"_s);
    q.exec(u"DELETE FROM applications;"_s);
    q.exec(u"DELETE FROM study_sessions;"_s);
    q.exec(u"DELETE FROM subtasks;"_s);
    q.exec(u"DELETE FROM topics;"_s);
    q.exec(u"DELETE FROM tracks;"_s);
    q.exec(u"DELETE FROM reminders_sent;"_s);
}

void tst_ImportExport::populateHandBuiltDataset(Database& db) {
    // Hand-built dataset arithmetic (16 rows total across 7 domain tables):
    // 2 tracks:
    //   id=1: 'Core C++', color '#1f77b4', icon 'code', position 0
    //   id=2: 'System Design', color '#ff7f0e', icon 'layers', position 1
    // 2 topics:
    //   id=10: track 1, 'Templates & Concepts (Вопросы & 🚀)', status 'in_progress', priority 'high', target_date '2026-11-01', resources '["https://cppreference.com"]', tags 'cpp,metaprogramming', position 0
    //   id=20: track 2, 'Distributed Caching', status 'backlog', priority 'medium', target_date NULL, empty notes, resources '[]', tags 'distributed', position 1
    // 3 subtasks:
    //   id=101: topic 10, 'Study std::span', done 1, position 0
    //   id=102: topic 10, 'Constexpr containers', done 0, position 1
    //   id=103: topic 20, 'Consistent hashing review', done 0, position 0
    // 2 study sessions:
    //   id=50: topic 10, '2026-10-27', 45 min, 'Deep dive into templates'
    //   id=51: topic_id NULL, '2026-10-28', 30 min, 'General reading'
    // 2 applications:
    //   id=201: 'Acme Corp', 'C++ Backend', empty url/notes, salary_min NULL, salary_max NULL, currency '', status 'applied', applied_date '2026-10-25'
    //   id=202: 'Globex Software', 'Platform Engineer', salary 8000..12000, notes 'Интервью и тестовое задание 💼', status 'technical', applied_date '2026-10-20'
    // 3 status history:
    //   id=301: app 201, from NULL -> 'applied', '2026-10-25T10:00:00'
    //   id=302: app 202, from NULL -> 'applied', '2026-10-20T09:00:00'
    //   id=303: app 202, from 'applied' -> 'technical', '2026-10-26T14:30:00'
    // 2 interviews:
    //   id=401: app 202, '2026-10-29T14:00:00', 60 min, 'technical', outcome 'pending'
    //   id=402: app 202, '2026-10-31T16:00:00', 45 min, 'hr', outcome 'cancelled'
    // Sum = 2 + 2 + 3 + 2 + 2 + 3 + 2 = 16 rows.
    QSqlQuery q(db.connection());
    q.exec(u"PRAGMA defer_foreign_keys=ON;"_s);
    q.exec(u"INSERT INTO tracks (id, name, color, icon, position) VALUES (1, 'Core C++', '#1f77b4', 'code', 0);"_s);
    q.exec(u"INSERT INTO tracks (id, name, color, icon, position) VALUES (2, 'System Design', '#ff7f0e', 'layers', 1);"_s);

    q.exec(u"INSERT INTO topics (id, track_id, title, status, priority, target_date, notes, resources, tags, position, created_at, updated_at) "
           u"VALUES (10, 1, 'Templates & Concepts (Вопросы & 🚀)', 'in_progress', 'high', '2026-11-01', 'Deep notes', '[\"https://cppreference.com\"]', 'cpp,metaprogramming', 0, '2026-10-25T10:00:00', '2026-10-26T11:00:00');"_s);
    q.exec(u"INSERT INTO topics (id, track_id, title, status, priority, target_date, notes, resources, tags, position, created_at, updated_at) "
           u"VALUES (20, 2, 'Distributed Caching', 'backlog', 'normal', NULL, '', '[]', 'distributed', 1, '2026-10-25T12:00:00', '2026-10-25T12:00:00');"_s);

    q.exec(u"INSERT INTO subtasks (id, topic_id, text, done, position) VALUES (101, 10, 'Study std::span', 1, 0);"_s);
    q.exec(u"INSERT INTO subtasks (id, topic_id, text, done, position) VALUES (102, 10, 'Constexpr containers', 0, 1);"_s);
    q.exec(u"INSERT INTO subtasks (id, topic_id, text, done, position) VALUES (103, 20, 'Consistent hashing review', 0, 0);"_s);

    q.exec(u"INSERT INTO study_sessions (id, topic_id, session_date, minutes, note) VALUES (50, 10, '2026-10-27', 45, 'Deep dive into templates');"_s);
    q.exec(u"INSERT INTO study_sessions (id, topic_id, session_date, minutes, note) VALUES (51, NULL, '2026-10-28', 30, 'General reading');"_s);

    q.exec(u"INSERT INTO applications (id, company, position, url, source, resume_version, location, work_mode, salary_min, salary_max, currency, status, applied_date, next_action, next_action_date, contact_name, contact_email, notes, created_at, updated_at) "
           u"VALUES (201, 'Acme Corp', 'C++ Backend', '', 'Referral', 'v1.0', 'Berlin', 'hybrid', NULL, NULL, '', 'applied', '2026-10-25', '', NULL, '', '', '', '2026-10-25T10:00:00', '2026-10-25T10:00:00');"_s);
    q.exec(u"INSERT INTO applications (id, company, position, url, source, resume_version, location, work_mode, salary_min, salary_max, currency, status, applied_date, next_action, next_action_date, contact_name, contact_email, notes, created_at, updated_at) "
           u"VALUES (202, 'Globex Software', 'Platform Engineer', 'https://globex.example', 'LinkedIn', 'v2.1', 'Remote', 'remote', 8000, 12000, 'EUR', 'technical', '2026-10-20', 'Final round', '2026-10-31', 'Alice Recruiter', 'alice@globex.example', 'Интервью и тестовое задание 💼', '2026-10-20T09:00:00', '2026-10-26T14:30:00');"_s);

    q.exec(u"INSERT INTO status_history (id, application_id, from_status, to_status, changed_at, note) VALUES (301, 201, NULL, 'applied', '2026-10-25T10:00:00', 'Application sent');"_s);
    q.exec(u"INSERT INTO status_history (id, application_id, from_status, to_status, changed_at, note) VALUES (302, 202, NULL, 'applied', '2026-10-20T09:00:00', 'Applied online');"_s);
    q.exec(u"INSERT INTO status_history (id, application_id, from_status, to_status, changed_at, note) VALUES (303, 202, 'applied', 'technical', '2026-10-26T14:30:00', 'Invited to interview');"_s);

    q.exec(u"INSERT INTO interviews (id, application_id, start_at, duration_min, type, place, interviewer, notes, outcome) "
           u"VALUES (401, 202, '2026-10-29T14:00:00', 60, 'technical', 'Google Meet', 'Lead Architect', 'Live coding', 'pending');"_s);
    q.exec(u"INSERT INTO interviews (id, application_id, start_at, duration_min, type, place, interviewer, notes, outcome) "
           u"VALUES (402, 202, '2026-10-31T16:00:00', 45, 'hr', 'Phone', 'HR Manager', 'Compensation discussion', 'cancelled');"_s);
}

void tst_ImportExport::verifyInvalidJsonLeavesDbUntouched(const QByteArray& invalidJson, RestoreErrorKind expectedError) {
    TestDatabase testDb(u"tst_inv"_s);
    QVERIFY(testDb.open(u":memory:"_s));
    populateHandBuiltDataset(testDb.db());
    const auto dumpBefore = dumpDatabase(testDb.db());

    const auto res = RestoreService::restoreDatabase(testDb.db(), invalidJson, "");
    QVERIFY(!res.success);
    QCOMPARE(res.errorKind, expectedError);

    const auto dumpAfter = dumpDatabase(testDb.db());
    QString diff;
    QVERIFY2(compareDumps(dumpBefore, dumpAfter, &diff), qPrintable(diff));
}

void tst_ImportExport::testRestoreEqualityHandBuiltDataset() {
    TestDatabase testDb(u"tst_restore_handbuilt"_s);
    QVERIFY(testDb.open(u":memory:"_s));
    populateHandBuiltDataset(testDb.db());

    const auto dumpBefore = dumpDatabase(testDb.db());
    const QByteArray jsonBytes = ExportService::generateBackupJson(testDb.db());
    QVERIFY(!jsonBytes.isEmpty());

    wipeDatabase(testDb.db());
    const auto dumpEmpty = dumpDatabase(testDb.db());
    for (const auto& td : dumpEmpty) {
        QCOMPARE(td.rows.size(), 0);
    }

    const auto res = RestoreService::restoreDatabase(testDb.db(), jsonBytes, "");
    QVERIFY(res.success);
    QCOMPARE(res.errorKind, RestoreErrorKind::None);

    const auto dumpAfter = dumpDatabase(testDb.db());
    QString diff;
    QVERIFY2(compareDumps(dumpBefore, dumpAfter, &diff), qPrintable(diff));
}

void tst_ImportExport::testRestoreEqualitySeedDataset() {
    TestDatabase testDb(u"tst_restore_seed"_s);
    QVERIFY(testDb.open(u":memory:"_s));

    JobPrep::Data::TrackRepository tracks(testDb.db());
    JobPrep::Data::TopicRepository topics(testDb.db());
    JobPrep::Services::SeedService seed(tracks, topics);

    QString seedError;
    QVERIFY(seed.loadSampleTopics(&seedError));

    const auto dumpBefore = dumpDatabase(testDb.db());
    const QByteArray jsonBytes = ExportService::generateBackupJson(testDb.db());
    QVERIFY(!jsonBytes.isEmpty());

    wipeDatabase(testDb.db());
    const auto res = RestoreService::restoreDatabase(testDb.db(), jsonBytes, "");
    QVERIFY(res.success);

    const auto dumpAfter = dumpDatabase(testDb.db());
    QString diff;
    QVERIFY2(compareDumps(dumpBefore, dumpAfter, &diff), qPrintable(diff));
}

void tst_ImportExport::testRestoreInvalidFormatLeavesDbUntouched() {
    verifyInvalidJsonLeavesDbUntouched(
        QByteArray("{\"format\": \"not-jobprep\", \"version\": 1, \"tracks\": []}"),
        RestoreErrorKind::InvalidFormat);
}

void tst_ImportExport::testRestoreInvalidVersionLeavesDbUntouched() {
    verifyInvalidJsonLeavesDbUntouched(
        QByteArray("{\"format\": \"jobprep-backup\", \"version\": 99, \"tracks\": []}"),
        RestoreErrorKind::UnsupportedVersion);
}

void tst_ImportExport::testRestoreTruncatedJsonLeavesDbUntouched() {
    verifyInvalidJsonLeavesDbUntouched(
        QByteArray("{\"format\": \"jobprep-backup\", \"version\": 1, \"tracks\": ["),
        RestoreErrorKind::InvalidJson);
}

void tst_ImportExport::testRestoreEmptyFileLeavesDbUntouched() {
    verifyInvalidJsonLeavesDbUntouched(
        QByteArray(""),
        RestoreErrorKind::EmptyFile);
}

void tst_ImportExport::testRestoreBadEnumLeavesDbUntouched() {
    TestDatabase testDb(u"tst_bad_enum"_s);
    QVERIFY(testDb.open(u":memory:"_s));
    populateHandBuiltDataset(testDb.db());
    const QByteArray valid = ExportService::generateBackupJson(testDb.db());
    QJsonDocument doc = QJsonDocument::fromJson(valid);
    QJsonObject root = doc.object();
    QJsonArray topics = root[u"topics"_s].toArray();
    QJsonObject top = topics.first().toObject();
    top[u"status"_s] = u"super_active"_s;
    topics[0] = top;
    root[u"topics"_s] = topics;
    verifyInvalidJsonLeavesDbUntouched(QJsonDocument(root).toJson(QJsonDocument::Compact),
                                       RestoreErrorKind::InvalidEnumValue);
}

void tst_ImportExport::testRestoreBrokenReferenceLeavesDbUntouched() {
    TestDatabase testDb(u"tst_broken_ref"_s);
    QVERIFY(testDb.open(u":memory:"_s));
    populateHandBuiltDataset(testDb.db());
    const QByteArray valid = ExportService::generateBackupJson(testDb.db());
    QJsonDocument doc = QJsonDocument::fromJson(valid);
    QJsonObject root = doc.object();
    QJsonArray topics = root[u"topics"_s].toArray();
    QJsonObject top = topics.first().toObject();
    top[u"track_id"_s] = 9999;
    topics[0] = top;
    root[u"topics"_s] = topics;
    verifyInvalidJsonLeavesDbUntouched(QJsonDocument(root).toJson(QJsonDocument::Compact),
                                       RestoreErrorKind::BrokenReference);
}

void tst_ImportExport::testRestoreDuplicateIdLeavesDbUntouched() {
    TestDatabase testDb(u"tst_dup_id"_s);
    QVERIFY(testDb.open(u":memory:"_s));
    populateHandBuiltDataset(testDb.db());
    const QByteArray valid = ExportService::generateBackupJson(testDb.db());
    QJsonDocument doc = QJsonDocument::fromJson(valid);
    QJsonObject root = doc.object();
    QJsonArray tracks = root[u"tracks"_s].toArray();
    QJsonObject dupTrack = tracks.first().toObject();
    tracks.append(dupTrack);
    root[u"tracks"_s] = tracks;
    verifyInvalidJsonLeavesDbUntouched(QJsonDocument(root).toJson(QJsonDocument::Compact),
                                       RestoreErrorKind::DuplicateId);
}

void tst_ImportExport::testRestoreOversizedFileLeavesDbUntouched() {
    QByteArray oversized(65 * 1024 * 1024, ' ');
    verifyInvalidJsonLeavesDbUntouched(oversized, RestoreErrorKind::FileTooLarge);
}

void tst_ImportExport::testRestoreSafetyBackupCreatedPrunedAndAbortsWhenReadOnly() {
    TestDatabase testDb(u"tst_safety"_s);
    QVERIFY(testDb.open(u":memory:"_s));
    populateHandBuiltDataset(testDb.db());
    const QByteArray validJson = ExportService::generateBackupJson(testDb.db());

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // 1. Restore with safety backup dir
    const auto res = RestoreService::restoreDatabase(testDb.db(), validJson, tempDir.path());
    QVERIFY(res.success);
    QVERIFY(!res.safetyBackupPath.isEmpty());
    QVERIFY(QFile::exists(res.safetyBackupPath));

    // 2. Prune test: create dummy pre-restore files and an unrelated file
    for (int i = 1; i <= 6; ++i) {
        QFile f(tempDir.filePath(u"pre-restore-2026010%1-000000.json"_s.arg(i)));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("{}");
        f.close();
    }
    QFile unrelated(tempDir.filePath(u"unrelated_document.txt"_s));
    QVERIFY(unrelated.open(QIODevice::WriteOnly));
    unrelated.write("Do not touch");
    unrelated.close();

    RestoreService::pruneSafetyBackups(tempDir.path(), 5);

    QDir d(tempDir.path());
    const QStringList preRestores = d.entryList({u"pre-restore-*.json"_s}, QDir::Files);
    QCOMPARE(preRestores.size(), 5);
    QVERIFY(unrelated.exists());

    // 3. Read-only safety backup dir aborts restore and leaves database untouched
    const auto dumpBefore = dumpDatabase(testDb.db());
    const QString roDir = tempDir.filePath(u"ro_folder"_s);
    QVERIFY(d.mkdir(u"ro_folder"_s));
    QFile::setPermissions(roDir, QFileDevice::ReadOwner | QFileDevice::ReadUser | QFileDevice::ExeOwner | QFileDevice::ExeUser);

    const auto roRes = RestoreService::restoreDatabase(testDb.db(), validJson, roDir);
    QVERIFY(!roRes.success);
    QCOMPARE(roRes.errorKind, RestoreErrorKind::SafetyBackupFailed);

    const auto dumpAfter = dumpDatabase(testDb.db());
    QString diff;
    QVERIFY2(compareDumps(dumpBefore, dumpAfter, &diff), qPrintable(diff));

    QFile::setPermissions(roDir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
}

void tst_ImportExport::testRestoreRemindersSentWiped() {
    TestDatabase testDb(u"tst_rem_wiped"_s);
    QVERIFY(testDb.open(u":memory:"_s));
    populateHandBuiltDataset(testDb.db());

    QSqlQuery q(testDb.db().connection());
    QVERIFY(q.exec(u"INSERT INTO reminders_sent (key, sent_at) VALUES ('interview-401-60', '2026-10-28T10:00:00');"_s));

    QVERIFY(q.exec(u"SELECT COUNT(*) FROM reminders_sent;"_s) && q.next());
    QCOMPARE(q.value(0).toInt(), 1);

    const QByteArray validJson = ExportService::generateBackupJson(testDb.db());
    const auto res = RestoreService::restoreDatabase(testDb.db(), validJson, "");
    QVERIFY(res.success);

    QVERIFY(q.exec(u"SELECT COUNT(*) FROM reminders_sent;"_s) && q.next());
    QCOMPARE(q.value(0).toInt(), 0);
}

void tst_ImportExport::testRestoreRemindersPausedDuringRestore() {
    TestDatabase testDb(u"tst_rem_pause"_s);
    QVERIFY(testDb.open(u":memory:"_s));
    populateHandBuiltDataset(testDb.db());

    JobPrep::Data::InterviewRepository interviews(testDb.db());
    JobPrep::Data::ApplicationRepository apps(testDb.db());
    JobPrep::Data::ReminderLogRepository reminderLog(testDb.db());
    class DummyNotifier : public JobPrep::Services::INotifier {
    public:
        bool notify(const JobPrep::Services::Notification&) override {
            ++count;
            return true;
        }
        int count{0};
    } notifier;

    const QDateTime fixedClock(QDate(2026, 10, 29), QTime(13, 0, 0));  // 60 min before interview 401
    JobPrep::Services::ReminderService reminderService(interviews, apps, reminderLog, notifier);
    reminderService.setClock([fixedClock] { return fixedClock; });

    QVERIFY(!reminderService.isPaused());

    reminderService.pause();
    QVERIFY(reminderService.isPaused());

    reminderService.checkNow();
    QCOMPARE(notifier.count, 0);

    reminderService.resume();
    QVERIFY(!reminderService.isPaused());

    reminderService.checkNow();
    QCOMPARE(notifier.count, 1);
}

void tst_ImportExport::testRestoreIdAllocationAfterRestore() {
    TestDatabase testDb(u"tst_id_alloc"_s);
    QVERIFY(testDb.open(u":memory:"_s));
    populateHandBuiltDataset(testDb.db());

    // Restore dataset with highest topic id = 20
    const QByteArray validJson = ExportService::generateBackupJson(testDb.db());
    const auto res = RestoreService::restoreDatabase(testDb.db(), validJson, "");
    QVERIFY(res.success);

    JobPrep::Data::TopicRepository topics(testDb.db());
    JobPrep::Domain::Topic newTopic;
    newTopic.trackId = 1;
    newTopic.title = u"Post Restore Topic"_s;
    newTopic.status = JobPrep::Domain::TopicStatus::Backlog;
    newTopic.priority = JobPrep::Domain::Priority::Normal;
    QVERIFY(topics.insert(newTopic));

    QVERIFY2(newTopic.id > 20, qPrintable(u"Allocated ID %1 must be > 20"_s.arg(newTopic.id)));
}

void tst_ImportExport::testRestoreAsyncEventLoopAlive() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dbPath = tempDir.filePath(u"async_test.db"_s);
    const QString backupPath = tempDir.filePath(u"async_backup.json"_s);
    const QString safetyDir = tempDir.filePath(u"safety_backups"_s);

    {
        Database db(u"tst_async_setup"_s);
        QVERIFY(db.open(dbPath));
        QString migrationErr;
        QVERIFY(JobPrep::Data::runMigrations(db, JobPrep::Data::migrations(), &migrationErr));
        populateHandBuiltDataset(db);
        const QByteArray json = ExportService::generateBackupJson(db);
        QFile f(backupPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(json);
        f.close();
        db.close();
    }

    RestoreService service;
    QTimer timer;
    int tickCount = 0;
    connect(&timer, &QTimer::timeout, [&] { ++tickCount; });

    QEventLoop loop;
    RestoreResult asyncResult;
    connect(&service, &RestoreService::finished, [&](const RestoreResult& r) {
        asyncResult = r;
        loop.quit();
    });

    timer.start(0);
    service.restoreAsync(dbPath, backupPath, safetyDir);
    loop.exec();
    timer.stop();

    QVERIFY(asyncResult.success);
    QVERIFY2(tickCount > 0, "Event loop did not tick during async restore!");
}

void tst_ImportExport::testRestorePerformanceSanity() {
    TestDatabase testDb(u"tst_perf"_s);
    QVERIFY(testDb.open(u":memory:"_s));

    // Generate large JSON payload: 1 track, 1 topic, 1000 applications with history, 5000 sessions
    QJsonObject root;
    root[u"format"_s] = u"jobprep-backup"_s;
    root[u"version"_s] = 1;
    root[u"exportedAt"_s] = u"2026-10-28T00:00:00Z"_s;

    QJsonArray tracks;
    QJsonObject tr;
    tr[u"id"_s] = 1;
    tr[u"name"_s] = u"General"_s;
    tr[u"color"_s] = u"#333"_s;
    tr[u"icon"_s] = u"folder"_s;
    tr[u"position"_s] = 0;
    tracks.append(tr);
    root[u"tracks"_s] = tracks;

    QJsonArray topics;
    QJsonObject top;
    top[u"id"_s] = 1;
    top[u"track_id"_s] = 1;
    top[u"title"_s] = u"Algorithms"_s;
    top[u"status"_s] = u"in_progress"_s;
    top[u"priority"_s] = u"high"_s;
    top[u"notes"_s] = u""_s;
    top[u"resources"_s] = u"[]"_s;
    top[u"tags"_s] = u""_s;
    top[u"position"_s] = 0;
    top[u"created_at"_s] = u"2026-10-01T00:00:00"_s;
    top[u"updated_at"_s] = u"2026-10-01T00:00:00"_s;
    topics.append(top);
    root[u"topics"_s] = topics;

    root[u"subtasks"_s] = QJsonArray();
    root[u"interviews"_s] = QJsonArray();

    QJsonArray sessions;
    for (int i = 1; i <= 5000; ++i) {
        QJsonObject s;
        s[u"id"_s] = i;
        s[u"topic_id"_s] = 1;
        s[u"session_date"_s] = u"2026-10-28"_s;
        s[u"minutes"_s] = 30;
        s[u"note"_s] = u"Practice"_s;
        sessions.append(s);
    }
    root[u"sessions"_s] = sessions;

    QJsonArray apps;
    QJsonArray history;
    for (int i = 1; i <= 1000; ++i) {
        QJsonObject a;
        a[u"id"_s] = i;
        a[u"company"_s] = u"Company %1"_s.arg(i);
        a[u"position"_s] = u"Software Engineer"_s;
        a[u"url"_s] = u""_s;
        a[u"source"_s] = u"Direct"_s;
        a[u"resume_version"_s] = u"v1"_s;
        a[u"location"_s] = u"Remote"_s;
        a[u"work_mode"_s] = u"remote"_s;
        a[u"currency"_s] = u"USD"_s;
        a[u"status"_s] = u"applied"_s;
        a[u"applied_date"_s] = u"2026-10-28"_s;
        a[u"next_action"_s] = u""_s;
        a[u"contact_name"_s] = u""_s;
        a[u"contact_email"_s] = u""_s;
        a[u"notes"_s] = u""_s;
        a[u"created_at"_s] = u"2026-10-28T10:00:00"_s;
        a[u"updated_at"_s] = u"2026-10-28T10:00:00"_s;
        apps.append(a);

        QJsonObject h;
        h[u"id"_s] = i;
        h[u"application_id"_s] = i;
        h[u"to_status"_s] = u"applied"_s;
        h[u"changed_at"_s] = u"2026-10-28T10:00:00"_s;
        h[u"note"_s] = u"Applied"_s;
        history.append(h);
    }
    root[u"applications"_s] = apps;
    root[u"statusHistory"_s] = history;

    const QByteArray largeJson = QJsonDocument(root).toJson(QJsonDocument::Compact);

    QElapsedTimer timer;
    timer.start();
    const auto res = RestoreService::restoreDatabase(testDb.db(), largeJson, "");
    const qint64 elapsedMs = timer.elapsed();

    qInfo() << "Restore performance sanity:" << elapsedMs << "ms for 1,000 applications and 5,000 sessions";
    QVERIFY(res.success);
    QCOMPARE(res.restoredApplications, 1000);
    QCOMPARE(res.restoredSessions, 5000);
    QVERIFY2(elapsedMs < 10000, "Performance sanity bound exceeded (took > 10s)");
}

void tst_ImportExport::testRestoreCoalescedRefresh() {
    TestDatabase testDb(u"tst_coalesce"_s);
    QVERIFY(testDb.open(u":memory:"_s));
    populateHandBuiltDataset(testDb.db());

    JobPrep::Data::TrackRepository tracks(testDb.db());
    JobPrep::Data::TopicRepository topics(testDb.db());
    JobPrep::Data::SessionRepository sessions(testDb.db());
    JobPrep::Data::ApplicationRepository apps(testDb.db());
    JobPrep::Data::InterviewRepository interviews(testDb.db());
    JobPrep::Services::SettingsService settings;
    JobPrep::Services::DayWatcher dayWatcher;
    JobPrep::Services::StatsService stats(apps, interviews, sessions, topics, tracks);
    JobPrep::Services::AgendaService agenda(interviews, apps, topics, tracks);
    JobPrep::Ui::DashboardController controller(stats, agenda, settings, dayWatcher, apps, interviews, sessions, topics, tracks);

    const int initialCount = controller.refreshCount();

    // Emitting changed() across all 5 repositories coalesces into a single refresh
    tracks.notifyChanged();
    topics.notifyChanged();
    sessions.notifyChanged();
    apps.notifyChanged();
    interviews.notifyChanged();

    QCoreApplication::processEvents();

    QCOMPARE(controller.refreshCount(), initialCount + 1);
}

void tst_ImportExport::testRestoreStudyPanelWithVanishedTopic() {
    TestDatabase testDb(u"tst_study_vanished"_s);
    QVERIFY(testDb.open(u":memory:"_s));
    populateHandBuiltDataset(testDb.db());

    JobPrep::Data::TopicRepository topics(testDb.db());
    JobPrep::Services::SettingsService settings;
    JobPrep::Ui::Theme::ThemeManager theme(settings);

    JobPrep::Ui::Widgets::TopicDetailPanel panel(topics, theme);
    QVERIFY(panel.loadFrom(10));
    QCOMPARE(panel.currentTopicId(), std::optional<int>(10));

    // Remove topic 10 from the repository
    QVERIFY(topics.remove(10));

    // Loading vanished topic resets detail panel safely
    QVERIFY(!panel.loadFrom(10));
    QCOMPARE(panel.currentTopicId(), std::nullopt);
}

QTEST_MAIN(tst_ImportExport)
#include "tst_ImportExport.moc"
