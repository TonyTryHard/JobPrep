#include <QDateTime>
#include <QSignalSpy>
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
        for (int row = 0; row < model.rowCount(); ++row) {
            const QModelIndex name =
                model.index(row, JobPrep::Models::ApplicationTableModel::CompanyColumn);
            if (name.data(Qt::DisplayRole).toString() != company) continue;
            return model.index(row, JobPrep::Models::ApplicationTableModel::NextStepColumn)
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

QTEST_GUILESS_MAIN(tst_ApplicationTableModel)
#include "tst_ApplicationTableModel.moc"