#include <QDateTime>
#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QSqlQuery>
#include <QTest>
#include "TestDatabase.h"
#include "data/ApplicationRepository.h"
#include "data/Database.h"
#include "data/DbFormat.h"
#include "data/InterviewRepository.h"
#include "models/ApplicationTableModel.h"

using namespace Qt::StringLiterals;

namespace {

const QDate kNowDate(2026, 3, 10);
const QTime kNowTime(12, 0);

JobPrep::Domain::JobApplication makeApplication(const QString& company) {
    JobPrep::Domain::JobApplication app;
    app.company = company;
    app.position = u"Dev"_s;
    return app;
}

JobPrep::Domain::Interview makeInterview(int applicationId, const QDateTime& startAt,
                                         JobPrep::Domain::InterviewOutcome outcome) {
    JobPrep::Domain::Interview interview;
    interview.applicationId = applicationId;
    interview.startAt = startAt;
    interview.outcome = outcome;
    return interview;
}

}  // namespace

class tst_ApplicationTableModel : public QObject {
    Q_OBJECT

private slots:
    void testColumnsAndHeaders();
    void testStatusColumnComesFromStatusRole();
    void testNextStepPrefersUpcomingPendingInterview();
    void testPastAndSettledInterviewsAreIgnored();
    void testFallsBackToNextActionTextAndDate();
    void testReloadOnRepositoryChanged();
    void testLegacyTimestampDisplayRoleShowsEmDash();
    void testModernTimestampDisplayRoleShowsFormattedDate();
    void testLegacyTimestampCutoffBoundary();
    void testSortingPreservesChronologicalOrderWithLegacyAndModernRows();
};

void tst_ApplicationTableModel::testColumnsAndHeaders() {
    JobPrep::Tests::TestDatabase db(u"atm1"_s);
    db.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(db.db());
    JobPrep::Data::InterviewRepository interviews(db.db());
    JobPrep::Models::ApplicationTableModel model(apps, interviews);

    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.columnCount(), JobPrep::Models::ApplicationTableModel::ColumnCount);
    QCOMPARE(model.headerData(JobPrep::Models::ApplicationTableModel::CompanyColumn,
                              Qt::Horizontal).toString(),
             u"Company"_s);
    QCOMPARE(model.headerData(JobPrep::Models::ApplicationTableModel::NextStepColumn,
                              Qt::Horizontal).toString(),
             u"Next step"_s);
    QVERIFY(model.headerData(0, Qt::Vertical).isValid());
    QVERIFY(!model.index(0, 0).isValid());
}

void tst_ApplicationTableModel::testStatusColumnComesFromStatusRole() {
    JobPrep::Tests::TestDatabase db(u"atm2"_s);
    db.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(db.db());
    JobPrep::Data::InterviewRepository interviews(db.db());
    JobPrep::Models::ApplicationTableModel model(apps, interviews);

    auto app = makeApplication(u"Acme"_s);
    app.status = JobPrep::Domain::ApplicationStatus::Offer;
    QVERIFY(apps.insert(app));

    const QModelIndex index = model.index(0, JobPrep::Models::ApplicationTableModel::StatusColumn);
    QVERIFY(index.isValid());
    QCOMPARE(index.data(JobPrep::Models::ApplicationTableModel::StatusRole).value<JobPrep::Domain::ApplicationStatus>(),
             JobPrep::Domain::ApplicationStatus::Offer);
    // The label lives in StatusStyle, which the badge delegate paints; the model must
    // not format it a second time.
    QVERIFY(index.data(Qt::DisplayRole).toString().isEmpty());
}

void tst_ApplicationTableModel::testNextStepPrefersUpcomingPendingInterview() {
    JobPrep::Tests::TestDatabase db(u"atm3"_s);
    db.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(db.db());
    JobPrep::Data::InterviewRepository interviews(db.db());
    JobPrep::Models::ApplicationTableModel model(apps, interviews);
    model.setClock([] { return QDateTime(kNowDate, kNowTime); });

    auto app = makeApplication(u"Acme"_s);
    app.nextAction = u"Follow up"_s;
    app.nextActionDate = QDate(2026, 4, 1);
    QVERIFY(apps.insert(app));

    auto later = makeInterview(app.id, QDateTime(QDate(2026, 3, 20), QTime(9, 0)),
                               JobPrep::Domain::InterviewOutcome::Pending);
    auto sooner = makeInterview(app.id, QDateTime(QDate(2026, 3, 12), QTime(15, 30)),
                                JobPrep::Domain::InterviewOutcome::Pending);
    QVERIFY(interviews.insert(later));
    QVERIFY(interviews.insert(sooner));

    const QModelIndex index = model.index(0, JobPrep::Models::ApplicationTableModel::NextStepColumn);
    QCOMPARE(index.data(Qt::DisplayRole).toString(), u"2026-03-12 15:30"_s);
}

void tst_ApplicationTableModel::testPastAndSettledInterviewsAreIgnored() {
    JobPrep::Tests::TestDatabase db(u"atm4"_s);
    db.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(db.db());
    JobPrep::Data::InterviewRepository interviews(db.db());
    JobPrep::Models::ApplicationTableModel model(apps, interviews);
    model.setClock([] { return QDateTime(kNowDate, kNowTime); });

    auto app = makeApplication(u"Acme"_s);
    app.nextAction = u"Sent offer"_s;
    QVERIFY(apps.insert(app));

    auto past = makeInterview(app.id, QDateTime(QDate(2026, 3, 1), QTime(10, 0)),
                              JobPrep::Domain::InterviewOutcome::Pending);
    auto passed = makeInterview(app.id, QDateTime(QDate(2026, 3, 25), QTime(10, 0)),
                                JobPrep::Domain::InterviewOutcome::Passed);
    auto failed = makeInterview(app.id, QDateTime(QDate(2026, 3, 26), QTime(10, 0)),
                                JobPrep::Domain::InterviewOutcome::Failed);
    auto cancelled = makeInterview(app.id, QDateTime(QDate(2026, 3, 27), QTime(10, 0)),
                                   JobPrep::Domain::InterviewOutcome::Cancelled);
    QVERIFY(interviews.insert(past));
    QVERIFY(interviews.insert(passed));
    QVERIFY(interviews.insert(failed));
    QVERIFY(interviews.insert(cancelled));

    const QModelIndex index = model.index(0, JobPrep::Models::ApplicationTableModel::NextStepColumn);
    QCOMPARE(index.data(Qt::DisplayRole).toString(), u"Sent offer"_s);
}

void tst_ApplicationTableModel::testFallsBackToNextActionTextAndDate() {
    JobPrep::Tests::TestDatabase db(u"atm5"_s);
    db.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(db.db());
    JobPrep::Data::InterviewRepository interviews(db.db());
    JobPrep::Models::ApplicationTableModel model(apps, interviews);
    model.setClock([] { return QDateTime(kNowDate, kNowTime); });

    struct Case {
        QString text;
        std::optional<QDate> date;
        QString expected;
    };
    const QList<Case> cases = {
        {u"Follow up"_s, QDate(2026, 4, 2), u"Follow up (2026-04-02)"_s},
        {QString(), QDate(2026, 4, 2), u"2026-04-02"_s},
        {u"Follow up"_s, std::nullopt, u"Follow up"_s},
        {QString(), std::nullopt, QString()},
    };

    int row = 0;
    for (const Case& testCase : cases) {
        auto app = makeApplication(u"Acme %1"_s.arg(QString::number(row)));
        app.nextAction = testCase.text;
        app.nextActionDate = testCase.date;
        QVERIFY(apps.insert(app));
        ++row;
    }

    QCOMPARE(model.rowCount(), cases.size());
    // `all()` is ordered by updated_at desc, so rows are found by company name.
    const auto nextStepOf = [&model](const QString& company) {
        for (int r = 0; r < model.rowCount(); ++r) {
            const QModelIndex name =
                model.index(r, JobPrep::Models::ApplicationTableModel::CompanyColumn);
            if (name.data(Qt::DisplayRole).toString() != company) continue;
            return model.index(r, JobPrep::Models::ApplicationTableModel::NextStepColumn)
                .data(Qt::DisplayRole)
                .toString();
        }
        return QStringLiteral("<row not found>");
    };
    for (int i = 0; i < cases.size(); ++i) {
        QCOMPARE(nextStepOf(u"Acme %1"_s.arg(QString::number(i))), cases.at(i).expected);
    }
}

void tst_ApplicationTableModel::testReloadOnRepositoryChanged() {
    JobPrep::Tests::TestDatabase db(u"atm6"_s);
    db.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(db.db());
    JobPrep::Data::InterviewRepository interviews(db.db());
    JobPrep::Models::ApplicationTableModel model(apps, interviews);
    model.setClock([] { return QDateTime(kNowDate, kNowTime); });

    auto app = makeApplication(u"Acme"_s);
    app.nextAction = u"Send resume"_s;
    QVERIFY(apps.insert(app));
    QCOMPARE(model.rowCount(), 1);

    // No explicit reload: the model listens to the repositories.
    auto copy = app;
    copy.id = 0;
    copy.company = u"Globex"_s;
    QVERIFY(apps.insert(copy));
    QCOMPARE(model.rowCount(), 2);

    auto interview = makeInterview(app.id, QDateTime(QDate(2026, 3, 15), QTime(11, 0)),
                                   JobPrep::Domain::InterviewOutcome::Pending);
    QVERIFY(interviews.insert(interview));
    QCOMPARE(model.rowCount(), 2);
}

void tst_ApplicationTableModel::testLegacyTimestampDisplayRoleShowsEmDash() {
    JobPrep::Tests::TestDatabase db(u"atm_legacy"_s);
    db.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(db.db());
    JobPrep::Data::InterviewRepository interviews(db.db());
    JobPrep::Models::ApplicationTableModel model(apps, interviews);

    auto app = makeApplication(u"Legacy Corp"_s);
    QVERIFY(apps.insert(app));

    // Simulate legacy 1970 timestamp stored prior to M6 fix
    QSqlQuery q(db.db().connection());
    QVERIFY(q.exec(u"UPDATE applications SET updated_at = '1970-01-01T00:00:00' WHERE id = %1"_s.arg(app.id)));
    model.reload();

    const QModelIndex idx = model.index(0, JobPrep::Models::ApplicationTableModel::UpdatedColumn);
    QVERIFY(idx.isValid());
    // DisplayRole must return "—" (em dash)
    QCOMPARE(idx.data(Qt::DisplayRole).toString(), u"—"_s);
    // SortRole must retain the real underlying QDateTime
    const auto sortVal = idx.data(JobPrep::Models::ApplicationTableModel::SortRole).toDateTime();
    QCOMPARE(sortVal.date(), QDate(1970, 1, 1));
}

void tst_ApplicationTableModel::testModernTimestampDisplayRoleShowsFormattedDate() {
    JobPrep::Tests::TestDatabase db(u"atm_modern"_s);
    db.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(db.db());
    JobPrep::Data::InterviewRepository interviews(db.db());
    JobPrep::Models::ApplicationTableModel model(apps, interviews);

    auto app = makeApplication(u"Modern Corp"_s);
    QVERIFY(apps.insert(app));

    QSqlQuery q(db.db().connection());
    QVERIFY(q.exec(u"UPDATE applications SET updated_at = '2026-03-10T14:30:00' WHERE id = %1"_s.arg(app.id)));
    model.reload();

    const QModelIndex idx = model.index(0, JobPrep::Models::ApplicationTableModel::UpdatedColumn);
    QVERIFY(idx.isValid());
    QCOMPARE(idx.data(Qt::DisplayRole).toString(), u"2026-03-10"_s);
}

void tst_ApplicationTableModel::testLegacyTimestampCutoffBoundary() {
    // 1. Direct DbFormat functions with UTC timestamps
    const QDateTime dtBefore = QDateTime::fromString(u"2012-12-31T23:59:59Z"_s, Qt::ISODate);
    QVERIFY(dtBefore.isValid());
    QVERIFY(!JobPrep::Data::DbFormat::isPlausibleTimestamp(dtBefore));
    QCOMPARE(JobPrep::Data::DbFormat::formatStoredTimestamp(dtBefore), u"—"_s);

    const QDateTime dtCutoff = QDateTime::fromString(u"2013-01-01T00:00:00Z"_s, Qt::ISODate);
    QVERIFY(dtCutoff.isValid());
    QVERIFY(JobPrep::Data::DbFormat::isPlausibleTimestamp(dtCutoff));
    QCOMPARE(JobPrep::Data::DbFormat::formatStoredTimestamp(dtCutoff), u"2013-01-01"_s);

    // 2. In model via SQLite storage
    JobPrep::Tests::TestDatabase db(u"atm_boundary"_s);
    db.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(db.db());
    JobPrep::Data::InterviewRepository interviews(db.db());
    JobPrep::Models::ApplicationTableModel model(apps, interviews);

    auto appBefore = makeApplication(u"BeforeCutoff"_s);
    QVERIFY(apps.insert(appBefore));
    auto appAtCutoff = makeApplication(u"AtCutoff"_s);
    QVERIFY(apps.insert(appAtCutoff));

    QSqlQuery q(db.db().connection());
    QVERIFY(q.exec(u"UPDATE applications SET updated_at = '2012-12-31T23:59:59' WHERE id = %1"_s.arg(appBefore.id)));
    QVERIFY(q.exec(u"UPDATE applications SET updated_at = '2013-01-01T00:00:00' WHERE id = %1"_s.arg(appAtCutoff.id)));
    model.reload();

    for (int r = 0; r < model.rowCount(); ++r) {
        const QString company = model.index(r, JobPrep::Models::ApplicationTableModel::CompanyColumn).data().toString();
        const QString updatedText = model.index(r, JobPrep::Models::ApplicationTableModel::UpdatedColumn).data(Qt::DisplayRole).toString();
        if (company == u"BeforeCutoff"_s) {
            QCOMPARE(updatedText, u"—"_s);
        } else if (company == u"AtCutoff"_s) {
            QCOMPARE(updatedText, u"2013-01-01"_s);
        }
    }
}

void tst_ApplicationTableModel::testSortingPreservesChronologicalOrderWithLegacyAndModernRows() {
    JobPrep::Tests::TestDatabase db(u"atm_sorting"_s);
    db.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(db.db());
    JobPrep::Data::InterviewRepository interviews(db.db());
    JobPrep::Models::ApplicationTableModel model(apps, interviews);

    auto app1970 = makeApplication(u"App1970"_s);
    QVERIFY(apps.insert(app1970));
    auto app2015 = makeApplication(u"App2015"_s);
    QVERIFY(apps.insert(app2015));
    auto app2026 = makeApplication(u"App2026"_s);
    QVERIFY(apps.insert(app2026));

    QSqlQuery q(db.db().connection());
    QVERIFY(q.exec(u"UPDATE applications SET updated_at = '1970-01-01T00:00:00' WHERE id = %1"_s.arg(app1970.id)));
    QVERIFY(q.exec(u"UPDATE applications SET updated_at = '2015-06-15T10:00:00' WHERE id = %1"_s.arg(app2015.id)));
    QVERIFY(q.exec(u"UPDATE applications SET updated_at = '2026-11-04T12:00:00' WHERE id = %1"_s.arg(app2026.id)));
    model.reload();

    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setSortRole(JobPrep::Models::ApplicationTableModel::SortRole);

    // Ascending sort: 1970 -> 2015 -> 2026
    proxy.sort(JobPrep::Models::ApplicationTableModel::UpdatedColumn, Qt::AscendingOrder);
    QCOMPARE(proxy.index(0, JobPrep::Models::ApplicationTableModel::CompanyColumn).data().toString(), u"App1970"_s);
    QCOMPARE(proxy.index(0, JobPrep::Models::ApplicationTableModel::UpdatedColumn).data(Qt::DisplayRole).toString(), u"—"_s);
    QCOMPARE(proxy.index(1, JobPrep::Models::ApplicationTableModel::CompanyColumn).data().toString(), u"App2015"_s);
    QCOMPARE(proxy.index(1, JobPrep::Models::ApplicationTableModel::UpdatedColumn).data(Qt::DisplayRole).toString(), u"2015-06-15"_s);
    QCOMPARE(proxy.index(2, JobPrep::Models::ApplicationTableModel::CompanyColumn).data().toString(), u"App2026"_s);
    QCOMPARE(proxy.index(2, JobPrep::Models::ApplicationTableModel::UpdatedColumn).data(Qt::DisplayRole).toString(), u"2026-11-04"_s);

    // Descending sort: 2026 -> 2015 -> 1970
    proxy.sort(JobPrep::Models::ApplicationTableModel::UpdatedColumn, Qt::DescendingOrder);
    QCOMPARE(proxy.index(0, JobPrep::Models::ApplicationTableModel::CompanyColumn).data().toString(), u"App2026"_s);
    QCOMPARE(proxy.index(0, JobPrep::Models::ApplicationTableModel::UpdatedColumn).data(Qt::DisplayRole).toString(), u"2026-11-04"_s);
    QCOMPARE(proxy.index(1, JobPrep::Models::ApplicationTableModel::CompanyColumn).data().toString(), u"App2015"_s);
    QCOMPARE(proxy.index(1, JobPrep::Models::ApplicationTableModel::UpdatedColumn).data(Qt::DisplayRole).toString(), u"2015-06-15"_s);
    QCOMPARE(proxy.index(2, JobPrep::Models::ApplicationTableModel::CompanyColumn).data().toString(), u"App1970"_s);
    QCOMPARE(proxy.index(2, JobPrep::Models::ApplicationTableModel::UpdatedColumn).data(Qt::DisplayRole).toString(), u"—"_s);
}

QTEST_GUILESS_MAIN(tst_ApplicationTableModel)
#include "tst_ApplicationTableModel.moc"