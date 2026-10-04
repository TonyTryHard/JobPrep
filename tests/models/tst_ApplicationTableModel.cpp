#include <QSignalSpy>
#include <QTest>
#include <QDateTime>
#include "data/ApplicationRepository.h"
#include "data/Database.h"
#include "data/InterviewRepository.h"
#include "data/DbFormat.h"
#include "models/ApplicationTableModel.h"
#include "TestDatabase.h"

using namespace Qt::StringLiterals;

class tst_ApplicationTableModel : public QObject {
    Q_OBJECT

private slots:
    void testNextStepAndReload();
};

void tst_ApplicationTableModel::testNextStepAndReload() {
    JobPrep::Tests::TestDatabase db(u"atm1"_s);
    db.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(db.db());
    JobPrep::Data::InterviewRepository interviews(db.db());
    JobPrep::Models::ApplicationTableModel model(apps, interviews);

    JobPrep::Domain::JobApplication app;
    app.company = u"A"_s;
    app.position = u"B"_s;
    app.nextAction = u"Follow"_s;
    app.nextActionDate = QDate::currentDate().addDays(1);
    QVERIFY(apps.insert(app));

    model.reload();
    QCOMPARE(model.rowCount(), 1);
}

QTEST_GUILESS_MAIN(tst_ApplicationTableModel)
#include "tst_ApplicationTableModel.moc"
