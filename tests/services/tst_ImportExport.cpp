#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>
#include <QTimeZone>
#include "TestDatabase.h"
#include "data/Database.h"
#include "data/Migrations.h"
#include "domain/Structs.h"
#include "services/ExportService.h"

using namespace Qt::StringLiterals;
using JobPrep::Data::Database;
using JobPrep::Domain::Interview;
using JobPrep::Domain::InterviewOutcome;
using JobPrep::Domain::InterviewType;
using JobPrep::Domain::JobApplication;
using JobPrep::Services::ExportErrorKind;
using JobPrep::Services::ExportResult;
using JobPrep::Services::ExportService;
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

private:
    /// Test helper to unfold RFC 5545 lines and unescape characters for round-trip verification.
    static QString unfoldAndUnescape(const QByteArray& icsBytes);
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

QTEST_MAIN(tst_ImportExport)
#include "tst_ImportExport.moc"
