#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include "services/ExportService.h"

using namespace Qt::StringLiterals;

class tst_ExportService : public QObject {
    Q_OBJECT

private slots:
    void testHeadersAreFixedAndOrdered();
    void testExportBasic();
    void testExportEmptyRowsWritesHeaderOnly();
    void testUnwritablePathReportsError();

private:
    static QList<QByteArray> readLines(const QString& path, bool* ok);
};

QList<QByteArray> tst_ExportService::readLines(const QString& path, bool* ok) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *ok = false;
        return {};
    }
    *ok = true;
    QByteArray data = file.readAll();
    // Records are separated by CRLF; a quoted field may still contain a bare LF.
    if (data.startsWith(QByteArray("\xEF\xBB\xBF", 3))) data.remove(0, 3);
    QList<QByteArray> records;
    for (const QByteArray& record : data.split('\n')) {
        records.append(record.endsWith('\r') ? record.left(record.size() - 1) : record);
    }
    while (!records.isEmpty() && records.last().isEmpty()) records.removeLast();
    return records;
}

void tst_ExportService::testHeadersAreFixedAndOrdered() {
    const QStringList expected = {
        u"Company"_s,         u"Position"_s,         u"Status"_s,       u"Applied"_s,
        u"Next step"_s,       u"Salary"_s,           u"Source"_s,       u"Updated"_s,
        u"URL"_s,             u"Resume version"_s,   u"Location"_s,     u"Work mode"_s,
        u"Salary min"_s,      u"Salary max"_s,       u"Currency"_s,     u"Next action"_s,
        u"Next action date"_s, u"Contact name"_s,    u"Contact email"_s, u"Notes"_s,
        u"Created at"_s,      u"Updated at"_s};

    // Pinned so a column cannot be renamed, reordered, added or dropped unnoticed.
    QCOMPARE(JobPrep::Services::ExportService::headers(), expected);
    // SPEC §3.3 A1: no id column and Updated appears exactly once.
    QVERIFY(!expected.contains(u"Id"_s, Qt::CaseInsensitive));
    QCOMPARE(expected.count(u"Updated"_s), 1);
    QCOMPARE(expected.count(u"Updated at"_s), 1);

    // The row writer and the header must stay in step.
    JobPrep::Services::ExportRow row;
    QCOMPARE(row.toFields().size(), expected.size());
    for (qsizetype i = 0; i < row.toFields().size(); ++i) {
        QVERIFY(row.toFields().at(i).isEmpty());
        QCOMPARE(row.toFields().at(i), row.toFields().at(i));
    }
}

void tst_ExportService::testExportBasic() {
    JobPrep::Services::ExportService svc;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(u"test.csv"_s);

    JobPrep::Services::ExportRow r;
    r.company = u"Acme, Inc"_s;
    r.position = u"C++ \"Senior\""_s;
    r.statusLabel = u"Technical"_s;
    r.applied = u"2026-10-01"_s;
    r.nextStepText = u"2026-10-05 14:00"_s;
    r.salaryText = u"4000–5000 USD"_s;
    r.source = u"LinkedIn"_s;
    r.updated = u"2026-10-02"_s;
    r.url = u"https://example.com"_s;
    r.resumeVersion = u"v1"_s;
    r.location = u"Remote"_s;
    r.workModeLabel = u"Remote"_s;
    r.salaryMin = u"4000"_s;
    r.salaryMax = u"5000"_s;
    r.currency = u"USD"_s;
    r.nextAction = u"Follow up"_s;
    r.nextActionDate = u"2026-10-05"_s;
    r.contactName = u"Иван"_s;  // Cyrillic
    r.contactEmail = u"ivan@example.com"_s;
    r.notes = u"Line1\nLine2"_s;  // newline
    r.createdAt = u"2026-10-01"_s;
    r.updatedAt = u"2026-10-02"_s;

    QString error;
    QVERIFY2(svc.exportToFile({r}, JobPrep::Services::ExportService::headers(), path, &error), qPrintable(error));
    QVERIFY(error.isEmpty());

    bool ok = false;
    const QList<QByteArray> lines = readLines(path, &ok);
    QVERIFY(ok);
    QCOMPARE(lines.size(), 3);
    QCOMPARE(lines.at(0), JobPrep::Services::ExportService::headers().join(u","_s).toUtf8());

    // The quoted Notes field holds an LF, so the one data record spans two lines.
    const QByteArray expectedRow =
        "\"Acme, Inc\",\"C++ \"\"Senior\"\"\",Technical,2026-10-01,2026-10-05 14:00,"
        // En dash and Cyrillic in UTF-8. The literals are split after every escape,
        // because \x keeps eating the hex digits that follow it.
        "4000\xe2\x80\x93" "5000 USD,LinkedIn,2026-10-02,https://example.com,v1,Remote,Remote,"
        "4000,5000,USD,Follow up,2026-10-05,"
        "\xd0\x98" "\xd0\xb2" "\xd0\xb0" "\xd0\xbd,"
        "ivan@example.com,\"Line1\nLine2\",2026-10-01,2026-10-02";
    QCOMPARE(lines.at(1) + QByteArray("\n") + lines.at(2), expectedRow);

    // UTF-8 BOM for Excel.
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.readAll().startsWith(QByteArray("\xEF\xBB\xBF", 3)));
}

void tst_ExportService::testExportEmptyRowsWritesHeaderOnly() {
    JobPrep::Services::ExportService svc;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(u"empty.csv"_s);

    // An empty header list falls back to the canonical one.
    QVERIFY(svc.exportToFile({}, {}, path, nullptr));

    bool ok = false;
    const QList<QByteArray> lines = readLines(path, &ok);
    QVERIFY(ok);
    QCOMPARE(lines.size(), 1);
    QCOMPARE(lines.at(0), JobPrep::Services::ExportService::headers().join(u","_s).toUtf8());
}

void tst_ExportService::testUnwritablePathReportsError() {
    JobPrep::Services::ExportService svc;
    QString error;
    QVERIFY(!svc.exportToFile({}, JobPrep::Services::ExportService::headers(), u"/nonexistent-dir/out.csv"_s, &error));
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(tst_ExportService)
#include "tst_ExportService.moc"