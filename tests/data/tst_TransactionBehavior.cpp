#include <QDateTime>
#include <QSignalSpy>
#include <QTest>
#include "data/ApplicationRepository.h"
#include "data/Database.h"
#include "data/InterviewRepository.h"
#include "data/SessionRepository.h"
#include "TestDatabase.h"

using namespace Qt::StringLiterals;

namespace {

JobPrep::Domain::JobApplication makeApplication(const QString& company) {
    JobPrep::Domain::JobApplication app;
    app.company = company;
    app.position = u"Dev"_s;
    return app;
}

/// `study_sessions.minutes` has `CHECK (minutes > 0)`, so this is a genuine SQL write
/// failure inside an open transaction, unlike an unknown id, which is "nothing to do".
JobPrep::Domain::StudySession makeSession(int minutes) {
    JobPrep::Domain::StudySession session;
    session.sessionDate = QDate(2026, 3, 1);
    session.minutes = minutes;
    session.note = u"study"_s;
    return session;
}

}  // namespace

class tst_TransactionBehavior : public QObject {
    Q_OBJECT

private slots:
    void testDeferredEmitsOnOuterCommit();
    void testNoEmitsOnOuterRollback();
    void testFailedWriteDoesNotQueue();
    void testJoinedFailurePoisonsOuterCommit();
    void testJoinedWritesEmitOncePerRepository();
    void testSuccessThenFailureLeavesNothing();
    void testMissingRowDoesNotPoisonOuterCommit();
    void testJoinedRollbackDoesNotNotify();
};

void tst_TransactionBehavior::testDeferredEmitsOnOuterCommit() {
    JobPrep::Tests::TestDatabase testDb(u"tx1"_s);
    testDb.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(testDb.db());
    QSignalSpy spy(&apps, &JobPrep::Data::ApplicationRepository::changed);

    auto outer = testDb.db().transaction();
    QVERIFY(outer.isOwner());
    QVERIFY(outer.isActive());

    auto app = makeApplication(u"Acme"_s);
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

    auto app = makeApplication(u"Acme"_s);
    QVERIFY(apps.insert(app));
    QCOMPARE(spy.count(), 0);
    outer.rollback();
    QCOMPARE(spy.count(), 0);
    QCOMPARE(apps.count(), 0);
}

void tst_TransactionBehavior::testFailedWriteDoesNotQueue() {
    JobPrep::Tests::TestDatabase testDb(u"tx3"_s);
    testDb.open(u":memory:"_s);
    JobPrep::Data::SessionRepository sessions(testDb.db());
    QSignalSpy spy(&sessions, &JobPrep::Data::SessionRepository::changed);

    auto outer = testDb.db().transaction();
    QVERIFY(outer.isOwner());

    auto invalid = makeSession(0);
    QVERIFY(!sessions.insert(invalid));
    QCOMPARE(spy.count(), 0);
    // The write failed, so the whole unit of work is unusable: the commit must say so
    // instead of committing whatever else happened to succeed.
    QVERIFY(!outer.commit());
    QCOMPARE(spy.count(), 0);
    QCOMPARE(sessions.all().size(), 0);
}

void tst_TransactionBehavior::testJoinedFailurePoisonsOuterCommit() {
    JobPrep::Tests::TestDatabase testDb(u"tx4"_s);
    testDb.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(testDb.db());
    JobPrep::Data::SessionRepository sessions(testDb.db());
    QSignalSpy appSpy(&apps, &JobPrep::Data::ApplicationRepository::changed);
    QSignalSpy sessionSpy(&sessions, &JobPrep::Data::SessionRepository::changed);

    auto outer = testDb.db().transaction();
    QVERIFY(outer.isOwner());

    auto acme = makeApplication(u"Acme"_s);
    QVERIFY(apps.insert(acme));
    QCOMPARE(appSpy.count(), 0);

    auto invalid = makeSession(0);
    QVERIFY(!sessions.insert(invalid));
    QCOMPARE(sessionSpy.count(), 0);

    QVERIFY(!outer.commit());
    QCOMPARE(appSpy.count(), 0);
    QCOMPARE(sessionSpy.count(), 0);
    // The successful joined write is gone with the poisoned unit of work.
    QCOMPARE(apps.count(), 0);
    QCOMPARE(sessions.all().size(), 0);
}

void tst_TransactionBehavior::testJoinedWritesEmitOncePerRepository() {
    JobPrep::Tests::TestDatabase testDb(u"tx5"_s);
    testDb.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(testDb.db());
    JobPrep::Data::InterviewRepository interviews(testDb.db());
    QSignalSpy appSpy(&apps, &JobPrep::Data::ApplicationRepository::changed);
    QSignalSpy interviewSpy(&interviews, &JobPrep::Data::InterviewRepository::changed);

    auto outer = testDb.db().transaction();
    QVERIFY(outer.isOwner());

    for (int i = 0; i < 3; ++i) {
        auto app = makeApplication(u"Acme %1"_s.arg(QString::number(i)));
        QVERIFY(apps.insert(app));
        QCOMPARE(appSpy.count(), 0);
    }
    JobPrep::Domain::Interview interview;
    interview.applicationId = apps.all().constFirst().id;
    interview.type = JobPrep::Domain::InterviewType::Hr;
    interview.startAt = QDateTime(QDate(2026, 3, 1), QTime(10, 0));
    interview.durationMin = 45;
    QVERIFY(interviews.insert(interview));
    QCOMPARE(interviewSpy.count(), 0);

    QVERIFY(outer.commit());
    QCOMPARE(appSpy.count(), 1);
    QCOMPARE(interviewSpy.count(), 1);
    QCOMPARE(apps.count(), 3);
    QCOMPARE(interviews.all().size(), 1);

    // The next unit of work is a fresh outer one, not a join.
    auto second = testDb.db().transaction();
    QVERIFY(second.isOwner());
    QVERIFY(second.commit());
}

void tst_TransactionBehavior::testSuccessThenFailureLeavesNothing() {
    JobPrep::Tests::TestDatabase testDb(u"tx6"_s);
    testDb.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(testDb.db());
    JobPrep::Data::SessionRepository sessions(testDb.db());
    QSignalSpy spy(&apps, &JobPrep::Data::ApplicationRepository::changed);

    auto outer = testDb.db().transaction();
    auto kept = makeApplication(u"Kept"_s);
    QVERIFY(apps.insert(kept));
    auto invalid = makeSession(0);
    QVERIFY(!sessions.insert(invalid));
    QVERIFY(!outer.commit());

    QCOMPARE(spy.count(), 0);
    QCOMPARE(apps.count(), 0);
    QVERIFY(apps.all().isEmpty());
}

void tst_TransactionBehavior::testMissingRowDoesNotPoisonOuterCommit() {
    JobPrep::Tests::TestDatabase testDb(u"tx7"_s);
    testDb.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(testDb.db());
    JobPrep::Data::SessionRepository sessions(testDb.db());
    QSignalSpy spy(&apps, &JobPrep::Data::ApplicationRepository::changed);

    auto outer = testDb.db().transaction();
    QVERIFY(outer.isOwner());

    auto acme = makeApplication(u"Acme"_s);
    QVERIFY(apps.insert(acme));

    // An unknown id is "nothing to do", not a database failure: the repository must not
    // open a transaction for it, so the surrounding unit of work still commits.
    auto missing = makeApplication(u"Ghost"_s);
    missing.id = 4242;
    QVERIFY(!apps.update(missing));
    QVERIFY(!apps.setStatus(4242, JobPrep::Domain::ApplicationStatus::Offer));
    QVERIFY(!apps.remove(4242));
    QVERIFY(!apps.setNextAction(4242, u"nope"_s, std::nullopt));
    auto copy = missing;
    QVERIFY(!apps.duplicate(4242, copy));
    QCOMPARE(spy.count(), 0);

    QVERIFY(outer.commit());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(apps.count(), 1);
    QCOMPARE(apps.lastError(), u"No application with id 4242."_s);
}

void tst_TransactionBehavior::testJoinedRollbackDoesNotNotify() {
    JobPrep::Tests::TestDatabase testDb(u"tx8"_s);
    testDb.open(u":memory:"_s);
    JobPrep::Data::ApplicationRepository apps(testDb.db());
    JobPrep::Data::SessionRepository sessions(testDb.db());
    QSignalSpy spy(&apps, &JobPrep::Data::ApplicationRepository::changed);

    auto outer = testDb.db().transaction();
    QVERIFY(outer.isOwner());

    auto acme = makeApplication(u"Acme"_s);
    QVERIFY(apps.insert(acme));
    {
        // Abandoning a joined unit of work must not notify: its writes may still be
        // rolled back, and it must not throw away anyone else's queued notification.
        auto joined = testDb.db().transaction();
        QVERIFY(!joined.isOwner());
        joined.rollback();
    }
    QCOMPARE(spy.count(), 0);
    QVERIFY(!outer.commit());
    QCOMPARE(spy.count(), 0);
    QCOMPARE(apps.count(), 0);

    // After the rollback the queue is empty again, so the next write notifies once.
    auto second = makeApplication(u"Acme 2"_s);
    QVERIFY(apps.insert(second));
    QCOMPARE(spy.count(), 1);
QCOMPARE(apps.count(), 1);

    // A later, unrelated write notifies only its own repository.
    auto session = makeSession(30);
    QVERIFY(sessions.insert(session));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(sessions.all().size(), 1);
}

QTEST_GUILESS_MAIN(tst_TransactionBehavior)
#include "tst_TransactionBehavior.moc"