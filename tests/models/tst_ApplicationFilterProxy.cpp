#include <QTest>
#include "data/ApplicationRepository.h"
#include "data/Database.h"
#include "data/InterviewRepository.h"
#include "models/ApplicationTableModel.h"
#include "models/ApplicationFilterProxy.h"
#include "TestDatabase.h"

using namespace Qt::StringLiterals;

class tst_ApplicationFilterProxy : public QObject {
    Q_OBJECT

private slots:
    void testBasic();
    void testCyrillicSearch();
};

void tst_ApplicationFilterProxy::testBasic() {
    JobPrep::Tests::TestDatabase db(u"afp1"_s);
    db.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(db.db());
    JobPrep::Data::InterviewRepository interviews(db.db());
    JobPrep::Models::ApplicationTableModel model(apps, interviews);
    JobPrep::Models::ApplicationFilterProxy proxy;
    proxy.setSourceModel(&model);

    JobPrep::Domain::JobApplication app;
    app.company = u"Acme"_s;
    app.position = u"Dev"_s;
    app.notes = u"C++"_s;
    QVERIFY(apps.insert(app));

    proxy.setFilterChip(JobPrep::Models::ApplicationFilterProxy::All);
    proxy.setSearchText(u"acme"_s);
    QCOMPARE(proxy.rowCount(), 1);

    proxy.setSearchText(u"xyz"_s);
    QCOMPARE(proxy.rowCount(), 0);
}

void tst_ApplicationFilterProxy::testCyrillicSearch() {
    JobPrep::Tests::TestDatabase db(u"afp2"_s);
    db.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(db.db());
    JobPrep::Data::InterviewRepository interviews(db.db());
    JobPrep::Models::ApplicationTableModel model(apps, interviews);
    JobPrep::Models::ApplicationFilterProxy proxy;
    proxy.setSourceModel(&model);

    JobPrep::Domain::JobApplication app;
    app.company = u"Компания"_s;
    app.position = u"Разработчик"_s;
    QVERIFY(apps.insert(app));

    proxy.setFilterChip(JobPrep::Models::ApplicationFilterProxy::All);
    proxy.setSearchText(u"комп"_s);
    QCOMPARE(proxy.rowCount(), 1);
}


QTEST_GUILESS_MAIN(tst_ApplicationFilterProxy)
#include "tst_ApplicationFilterProxy.moc"
