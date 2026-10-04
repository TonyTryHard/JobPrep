#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>
#include "services/ExportService.h"

using namespace Qt::StringLiterals;

class tst_ExportService : public QObject {
    Q_OBJECT

private slots:
    void testExportBasic();
};

void tst_ExportService::testExportBasic() {
    JobPrep::Services::ExportService svc;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString path = dir.filePath(u"test.csv"_s);

    QList<JobPrep::Services::ExportRow> rows;
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
    r.contactName = u"Иван"_s; // Cyrillic
    r.contactEmail = u"ivan@example.com"_s;
    r.notes = u"Line1\nLine2"_s; // newline
    r.createdAt = u"2026-10-01"_s;
    r.updatedAt = u"2026-10-02"_s;
    rows.append(r);

    QStringList headers;
    headers << u"Company"_s << u"Position"_s << u"Status"_s << u"Applied"_s << u"Next step"_s << u"Salary"_s
            << u"Source"_s << u"Updated"_s << u"URL"_s << u"Resume version"_s << u"Location"_s << u"Work mode"_s
            << u"Salary min"_s << u"Salary max"_s << u"Currency"_s << u"Next action"_s << u"Next action date"_s
            << u"Contact name"_s << u"Contact email"_s << u"Notes"_s << u"Created at"_s << u"Updated at"_s;

    QVERIFY(svc.exportToFile(rows, headers, path, nullptr));

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QByteArray data = f.readAll();
    // BOM
    QVERIFY(data.startsWith(QByteArray("\xEF\xBB\xBF")));
    // Contains Cyrillic (UTF-8)
    QVERIFY(data.contains("Иван"));
}

QTEST_GUILESS_MAIN(tst_ExportService)
#include "tst_ExportService.moc"
