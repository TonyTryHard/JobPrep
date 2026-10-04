#include <QSignalSpy>
#include <QTest>
#include <QDate>
#include "data/ApplicationRepository.h"
#include "data/Database.h"
#include "TestDatabase.h"

using namespace Qt::StringLiterals;

class tst_TransactionBehavior : public QObject {
    Q_OBJECT

private slots:
    void testDeferredEmitsOnOuterCommit();
    void testNoEmitsOnOuterRollback();
    void testFailedWriteDoesNotQueue();
};

void tst_TransactionBehavior::testDeferredEmitsOnOuterCommit() {
    JobPrep::Tests::TestDatabase testDb(u"tx1"_s);
    testDb.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(testDb.db());
    QSignalSpy spy(&apps, &JobPrep::Data::ApplicationRepository::changed);

    auto outer = testDb.db().transaction();
    QVERIFY(outer.isOwner());
    QVERIFY(outer.isActive());

    JobPrep::Domain::JobApplication app;
    app.company = u"Acme"_s;
    app.position = u"Dev"_s;
    app.appliedDate = QDate::currentDate();
    QVERIFY(apps.insert(app));
    QCOMPARE(spy.count(), 0);
    QVERIFY(outer.commit());
    QCOMPARE(spy.count(), 1);
}

void tst_TransactionBehavior::testNoEmitsOnOuterRollback() {
    JobPrep::Tests::TestDatabase testDb(u"tx2"_s);
    testDb.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(testDb.db());
    QSignalSpy spy(&apps, &JobPrep::Data::ApplicationRepository::changed);

    auto outer = testDb.db().transaction();
    QVERIFY(outer.isOwner());

    JobPrep::Domain::JobApplication app;
    app.company = u"Acme"_s;
    app.position = u"Dev"_s;
    QVERIFY(apps.insert(app));
    QCOMPARE(spy.count(), 0);
    outer.rollback();
    QCOMPARE(spy.count(), 0);
}

void tst_TransactionBehavior::testFailedWriteDoesNotQueue() {
    JobPrep::Tests::TestDatabase testDb(u"tx3"_s);
    testDb.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(testDb.db());
    QSignalSpy spy(&apps, &JobPrep::Data::ApplicationRepository::changed);

    auto outer = testDb.db().transaction();
    QVERIFY(outer.isOwner());

    JobPrep::Domain::JobApplication app;
    app.id = 123;
    app.company = u"X"_s;
    app.position = u"Y"_s;
    QVERIFY(!apps.update(app));
    QCOMPARE(spy.count(), 0);
    QVERIFY(outer.commit());
    QCOMPARE(spy.count(), 0);
}

QTEST_GUILESS_MAIN(tst_TransactionBehavior)
#include "tst_TransactionBehavior.moc"
