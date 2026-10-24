#include <QDate>
#include <QDateTime>
#include <QSignalSpy>
#include <QTest>
#include <QTime>
#include "TestDatabase.h"
#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"

using namespace Qt::StringLiterals;
using JobPrep::Data::ApplicationRepository;
using JobPrep::Data::InterviewRepository;
using JobPrep::Domain::ApplicationStatus;
using JobPrep::Domain::Interview;
using JobPrep::Domain::InterviewType;
using JobPrep::Domain::JobApplication;
using JobPrep::Domain::StatusChange;
using JobPrep::Domain::WorkMode;
using JobPrep::Tests::TestDatabase;

class tst_ApplicationRepository : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void insertFillsFieldsTimestampsAndHistory();
    void insertAppliesTodayForAppliedStatus();
    void updateStoresNewValuesAndBumpsTimestamp();
    void updateWithNewStatusAddsOneHistoryRow();
    void updateWithSameStatusAddsNoHistoryRow();
    void setStatusAddsExactlyOneHistoryRow();
    void setStatusAppliesTodayForAppliedStatus();
    void setNextActionUpdatesColumns();
    void duplicateCopiesFieldsWithOwnHistory();
    void removeCascadesHistoryAndInterviews();
    void queriesByStatuses();
    void countsByStatusAndTotals();
    void statusHistoryIsOrderedAndComplete();
    void unknownIdFailsWithoutChangedSignal();
    void byNextActionDateRangeQueries();
    void appliedCountInRangeInclusiveAndEmpty();
    void funnelStageCountsMatchesRanksAndHistory();
    void byNextActionDateUpToQueries();

private:
    int addApplication(const QString& company, ApplicationStatus status = ApplicationStatus::Applied);

    std::unique_ptr<TestDatabase> m_db;
    std::unique_ptr<ApplicationRepository> m_apps;
    std::unique_ptr<InterviewRepository> m_interviews;
};

int tst_ApplicationRepository::addApplication(const QString& company, ApplicationStatus status) {
    JobApplication application;
    application.company = company;
    application.position = u"C++ Developer"_s;
    application.status = status;
    application.appliedDate = QDate(2026, 9, 28);
    return m_apps->insert(application) ? application.id : 0;
}

void tst_ApplicationRepository::init() {
    m_db = std::make_unique<TestDatabase>(u"tst_applications"_s);
    QVERIFY(m_db->open(u":memory:"_s));
    m_apps = std::make_unique<ApplicationRepository>(m_db->db());
    m_interviews = std::make_unique<InterviewRepository>(m_db->db());
}

void tst_ApplicationRepository::cleanup() {
    m_interviews.reset();
    m_apps.reset();
    m_db.reset();
}

void tst_ApplicationRepository::insertFillsFieldsTimestampsAndHistory() {
    JobApplication application;
    application.company = u"Globex"_s;
    application.position = u"Qt Engineer"_s;
    application.url = u"https://globex.example/jobs/1"_s;
    application.source = u"LinkedIn"_s;
    application.resumeVersion = u"cv-v3.pdf"_s;
    application.location = u"Berlin"_s;
    application.workMode = WorkMode::Hybrid;
    application.salaryMin = 4000;
    application.salaryMax = 5000;
    application.currency = u"EUR"_s;
    application.status = ApplicationStatus::Technical;
    application.appliedDate = QDate(2026, 9, 12);
    application.nextAction = u"Technical interview"_s;
    application.nextActionDate = QDate(2026, 10, 2);
    application.contactName = u"Anna K."_s;
    application.contactEmail = u"anna@globex.example"_s;
    application.notes = u"Referred by a friend"_s;

    QSignalSpy spy(m_apps.get(), &ApplicationRepository::changed);
    QVERIFY(m_apps->insert(application));
    QVERIFY(application.id > 0);
    QVERIFY(application.createdAt.isValid());
    QVERIFY(application.updatedAt.isValid());
    QCOMPARE(spy.count(), 1);

    const auto stored = m_apps->byId(application.id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->company, u"Globex"_s);
    QCOMPARE(stored->position, u"Qt Engineer"_s);
    QCOMPARE(stored->url, u"https://globex.example/jobs/1"_s);
    QCOMPARE(stored->source, u"LinkedIn"_s);
    QCOMPARE(stored->resumeVersion, u"cv-v3.pdf"_s);
    QCOMPARE(stored->location, u"Berlin"_s);
    QVERIFY(stored->workMode == WorkMode::Hybrid);
    QCOMPARE(stored->salaryMin, std::optional<int>(4000));
    QCOMPARE(stored->salaryMax, std::optional<int>(5000));
    QCOMPARE(stored->currency, u"EUR"_s);
    QVERIFY(stored->status == ApplicationStatus::Technical);
    QCOMPARE(stored->appliedDate, std::optional<QDate>(QDate(2026, 9, 12)));
    QCOMPARE(stored->nextAction, u"Technical interview"_s);
    QCOMPARE(stored->nextActionDate, std::optional<QDate>(QDate(2026, 10, 2)));
    QCOMPARE(stored->contactName, u"Anna K."_s);
    QCOMPARE(stored->contactEmail, u"anna@globex.example"_s);
    QCOMPARE(stored->notes, u"Referred by a friend"_s);
    QCOMPARE(stored->createdAt, application.createdAt);

    // The initial row records the starting point with an empty from_status (SPEC §5).
    const auto history = m_apps->statusHistory(application.id);
    QCOMPARE(history.size(), 1);
    QVERIFY(!history.first().fromStatus.has_value());
    QVERIFY(history.first().toStatus == ApplicationStatus::Technical);
    QVERIFY(history.first().changedAt.isValid());
}

void tst_ApplicationRepository::insertAppliesTodayForAppliedStatus() {
    JobApplication application;
    application.company = u"Acme"_s;
    application.position = u"C++ Developer"_s;
    application.status = ApplicationStatus::Applied;
    QVERIFY(m_apps->insert(application));

    QCOMPARE(application.appliedDate, std::optional<QDate>(QDate::currentDate()));
    QCOMPARE(m_apps->byId(application.id)->appliedDate,
             std::optional<QDate>(QDate::currentDate()));
}

void tst_ApplicationRepository::updateStoresNewValuesAndBumpsTimestamp() {
    const int id = addApplication(u"Acme"_s);
    JobApplication application = *m_apps->byId(id);
    const QDateTime createdAt = application.createdAt;

    application.position = u"Senior C++ Developer"_s;
    application.salaryMax = 6000;
    application.workMode = WorkMode::Remote;
    application.nextActionDate = QDate(2026, 10, 20);

    QSignalSpy spy(m_apps.get(), &ApplicationRepository::changed);
    QVERIFY(m_apps->update(application));
    QCOMPARE(spy.count(), 1);

    auto stored = m_apps->byId(id);
    QCOMPARE(stored->position, u"Senior C++ Developer"_s);
    QCOMPARE(stored->salaryMax, std::optional<int>(6000));
    QVERIFY(stored->workMode == WorkMode::Remote);
    QCOMPARE(stored->nextActionDate, std::optional<QDate>(QDate(2026, 10, 20)));
    QCOMPARE(stored->createdAt, createdAt);
    QVERIFY(stored->updatedAt >= createdAt);

    // Clearing an optional column stores NULL again.
    stored->salaryMax.reset();
    stored->appliedDate.reset();
    QVERIFY(m_apps->update(*stored));
    QVERIFY(!m_apps->byId(id)->salaryMax.has_value());
}

void tst_ApplicationRepository::updateWithNewStatusAddsOneHistoryRow() {
    const int id = addApplication(u"Acme"_s);
    QVERIFY(m_apps->statusHistory(id).size() == 1);

    JobApplication application = *m_apps->byId(id);
    application.status = ApplicationStatus::HrScreen;

    QSignalSpy spy(m_apps.get(), &ApplicationRepository::changed);
    QVERIFY(m_apps->update(application));
    QCOMPARE(spy.count(), 1);

    const auto history = m_apps->statusHistory(id);
    QCOMPARE(history.size(), 2);
    const StatusChange& last = history.last();
    QVERIFY(last.fromStatus.has_value());
    QVERIFY(*last.fromStatus == ApplicationStatus::Applied);
    QVERIFY(last.toStatus == ApplicationStatus::HrScreen);
    QCOMPARE(last.applicationId, id);
    QVERIFY(m_apps->byId(id)->status == ApplicationStatus::HrScreen);
}

void tst_ApplicationRepository::updateWithSameStatusAddsNoHistoryRow() {
    const int id = addApplication(u"Acme"_s);
    JobApplication application = *m_apps->byId(id);
    application.notes = u"Recruiter called on Monday"_s;

    QVERIFY(m_apps->update(application));
    QCOMPARE(m_apps->statusHistory(id).size(), 1);
    QCOMPARE(m_apps->byId(id)->notes, u"Recruiter called on Monday"_s);
}

void tst_ApplicationRepository::setStatusAddsExactlyOneHistoryRow() {
    const int id = addApplication(u"Acme"_s);

    QSignalSpy spy(m_apps.get(), &ApplicationRepository::changed);
    QVERIFY(m_apps->setStatus(id, ApplicationStatus::Technical, u"invited to a live session"_s));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(m_apps->statusHistory(id).size(), 2);

    QVERIFY(m_apps->setStatus(id, ApplicationStatus::Final));
    QCOMPARE(spy.count(), 2);

    const auto history = m_apps->statusHistory(id);
    QCOMPARE(history.size(), 3);
    QVERIFY(*history.at(1).fromStatus == ApplicationStatus::Applied);
    QVERIFY(history.at(1).toStatus == ApplicationStatus::Technical);
    QCOMPARE(history.at(1).note, u"invited to a live session"_s);
    QVERIFY(*history.at(2).fromStatus == ApplicationStatus::Technical);
    QVERIFY(history.at(2).toStatus == ApplicationStatus::Final);
    QVERIFY(history.at(2).note.isEmpty());

    QVERIFY(!m_apps->setStatus(999, ApplicationStatus::Offer));
    QCOMPARE(spy.count(), 2);
}

void tst_ApplicationRepository::setStatusAppliesTodayForAppliedStatus() {
    JobApplication application;
    application.company = u"Initech"_s;
    application.position = u"C++ Developer"_s;
    application.status = ApplicationStatus::Wishlist;
    QVERIFY(m_apps->insert(application));
    QVERIFY(!application.appliedDate.has_value());

    QVERIFY(m_apps->setStatus(application.id, ApplicationStatus::Applied));
    QCOMPARE(m_apps->byId(application.id)->appliedDate,
             std::optional<QDate>(QDate::currentDate()));

    // An existing applied date is never overwritten.
    JobApplication stored = *m_apps->byId(application.id);
    stored.appliedDate = QDate(2026, 9, 1);
    QVERIFY(m_apps->update(stored));
    QVERIFY(m_apps->setStatus(application.id, ApplicationStatus::Rejected));
    QVERIFY(m_apps->setStatus(application.id, ApplicationStatus::Applied));
    QCOMPARE(m_apps->byId(application.id)->appliedDate, std::optional<QDate>(QDate(2026, 9, 1)));
}

void tst_ApplicationRepository::setNextActionUpdatesColumns() {
    const int id = addApplication(u"Acme"_s);

    QSignalSpy spy(m_apps.get(), &ApplicationRepository::changed);
    QVERIFY(m_apps->setNextAction(id, u"Follow up with the recruiter"_s, QDate(2026, 10, 5)));
    QCOMPARE(spy.count(), 1);

    auto stored = m_apps->byId(id);
    QCOMPARE(stored->nextAction, u"Follow up with the recruiter"_s);
    QCOMPARE(stored->nextActionDate, std::optional<QDate>(QDate(2026, 10, 5)));

    QVERIFY(m_apps->setNextAction(id, QString(), std::nullopt));
    stored = m_apps->byId(id);
    QVERIFY(stored->nextAction.isEmpty());
    QVERIFY(!stored->nextActionDate.has_value());

    QVERIFY(!m_apps->setNextAction(999, u"ghost"_s, std::nullopt));
    QCOMPARE(spy.count(), 2);
}

void tst_ApplicationRepository::duplicateCopiesFieldsWithOwnHistory() {
    const int id = addApplication(u"Globex"_s, ApplicationStatus::Technical);
    JobApplication original = *m_apps->byId(id);
    original.notes = u"Strong C++ and Qt background"_s;
    // duplicate() copies the stored row, so the notes have to be persisted first.
    QVERIFY(m_apps->update(original));
    original = *m_apps->byId(id);

    JobApplication copy;
    QSignalSpy spy(m_apps.get(), &ApplicationRepository::changed);
    QVERIFY2(m_apps->duplicate(id, copy), qPrintable(m_apps->lastError()));
    QCOMPARE(spy.count(), 1);

    QVERIFY(copy.id > 0);
    QVERIFY(copy.id != id);
    QCOMPARE(copy.company, original.company);
    QCOMPARE(copy.position, original.position);
    QCOMPARE(copy.notes, original.notes);
    QVERIFY(copy.status == ApplicationStatus::Technical);
    QCOMPARE(copy.appliedDate, original.appliedDate);
    QCOMPARE(copy.createdAt, copy.updatedAt);

    const auto history = m_apps->statusHistory(copy.id);
    QCOMPARE(history.size(), 1);
    QVERIFY(!history.first().fromStatus.has_value());
    QVERIFY(history.first().toStatus == ApplicationStatus::Technical);
    QCOMPARE(m_apps->statusHistory(id).size(), 1);
    QCOMPARE(m_apps->count(), 2);

    QVERIFY(!m_apps->duplicate(999, copy));
}

void tst_ApplicationRepository::removeCascadesHistoryAndInterviews() {
    const int id = addApplication(u"Globex"_s);

    Interview interview;
    interview.applicationId = id;
    interview.startAt = QDateTime(QDate(2026, 10, 2), QTime(14, 0));
    interview.type = InterviewType::Technical;
    QVERIFY(m_interviews->insert(interview));
    QCOMPARE(m_db->countRows(u"status_history"_s), 1);
    QCOMPARE(m_db->countRows(u"interviews"_s), 1);

    QSignalSpy spy(m_apps.get(), &ApplicationRepository::changed);
    QVERIFY(m_apps->remove(id));
    QCOMPARE(spy.count(), 1);

    QVERIFY(m_apps->isEmpty());
    QCOMPARE(m_db->countRows(u"status_history"_s), 0);
    QCOMPARE(m_db->countRows(u"interviews"_s), 0);
    QVERIFY(!m_apps->remove(id));
}

void tst_ApplicationRepository::queriesByStatuses() {
    addApplication(u"Acme"_s, ApplicationStatus::Applied);
    addApplication(u"Globex"_s, ApplicationStatus::Technical);
    addApplication(u"Initech"_s, ApplicationStatus::Rejected);
    addApplication(u"Wile"_s, ApplicationStatus::Wishlist);

    QCOMPARE(m_apps->all().size(), 4);
    QCOMPARE(m_apps->byStatuses({}).size(), 0);
    QCOMPARE(m_apps->byStatuses({ApplicationStatus::Applied}).size(), 1);
    QCOMPARE(
        m_apps->byStatuses({ApplicationStatus::Technical, ApplicationStatus::Rejected}).size(), 2);
    QCOMPARE(m_apps->byStatuses({ApplicationStatus::Offer}).size(), 0);
    QCOMPARE(m_apps->byStatuses({ApplicationStatus::Wishlist}).first().company, u"Wile"_s);
}

void tst_ApplicationRepository::countsByStatusAndTotals() {
    addApplication(u"Acme"_s, ApplicationStatus::Applied);
    addApplication(u"Globex"_s, ApplicationStatus::Applied);
    addApplication(u"Initech"_s, ApplicationStatus::Offer);

    QVERIFY(!m_apps->isEmpty());
    QCOMPARE(m_apps->count(), 3);

    const auto counts = m_apps->countsByStatus();
    QCOMPARE(counts.value(ApplicationStatus::Applied), 2);
    QCOMPARE(counts.value(ApplicationStatus::Offer), 1);
    QVERIFY(!counts.contains(ApplicationStatus::Rejected));
}

void tst_ApplicationRepository::statusHistoryIsOrderedAndComplete() {
    const int first = addApplication(u"Acme"_s);
    const int second = addApplication(u"Globex"_s);
    QVERIFY(m_apps->setStatus(first, ApplicationStatus::HrScreen));
    QVERIFY(m_apps->setStatus(second, ApplicationStatus::Technical));

    const auto firstHistory = m_apps->statusHistory(first);
    QCOMPARE(firstHistory.size(), 2);
    for (int i = 1; i < firstHistory.size(); ++i) {
        QVERIFY(firstHistory[i - 1].changedAt <= firstHistory[i].changedAt);
    }
    QCOMPARE(firstHistory.last().applicationId, first);

    const auto all = m_apps->allStatusHistory();
    QCOMPARE(all.size(), 4);
    QVERIFY(!all.isEmpty());
}

void tst_ApplicationRepository::unknownIdFailsWithoutChangedSignal() {
    QSignalSpy spy(m_apps.get(), &ApplicationRepository::changed);
    QVERIFY(!m_apps->byId(999).has_value());
    QCOMPARE(m_apps->lastError(), QString());
    QVERIFY(!m_apps->remove(999));
    QCOMPARE(spy.count(), 0);
}

void tst_ApplicationRepository::byNextActionDateRangeQueries() {
    // 1. Applied app within range
    JobApplication app1;
    app1.company = u"App1"_s;
    app1.position = u"Dev"_s;
    app1.status = ApplicationStatus::Applied;
    app1.nextAction = u"Follow up 1"_s;
    app1.nextActionDate = QDate(2026, 10, 5);
    QVERIFY(m_apps->insert(app1));

    // 2. Wishlist app within range
    JobApplication app2;
    app2.company = u"App2"_s;
    app2.position = u"Dev"_s;
    app2.status = ApplicationStatus::Wishlist;
    app2.nextAction = u"Follow up 2"_s;
    app2.nextActionDate = QDate(2026, 10, 1);
    QVERIFY(m_apps->insert(app2));

    // 3. Rejected app within range (should be excluded as closed)
    JobApplication app3;
    app3.company = u"App3"_s;
    app3.position = u"Dev"_s;
    app3.status = ApplicationStatus::Rejected;
    app3.nextAction = u"Follow up 3"_s;
    app3.nextActionDate = QDate(2026, 10, 5);
    QVERIFY(m_apps->insert(app3));

    // 4. Accepted app within range (closed)
    JobApplication app4;
    app4.company = u"App4"_s;
    app4.position = u"Dev"_s;
    app4.status = ApplicationStatus::Accepted;
    app4.nextAction = u"Follow up 4"_s;
    app4.nextActionDate = QDate(2026, 10, 5);
    QVERIFY(m_apps->insert(app4));

    // 5. App with date before range
    JobApplication app5;
    app5.company = u"App5"_s;
    app5.position = u"Dev"_s;
    app5.status = ApplicationStatus::Applied;
    app5.nextAction = u"Follow up 5"_s;
    app5.nextActionDate = QDate(2026, 9, 30);
    QVERIFY(m_apps->insert(app5));

    // 6. App with date after range
    JobApplication app6;
    app6.company = u"App6"_s;
    app6.position = u"Dev"_s;
    app6.status = ApplicationStatus::Applied;
    app6.nextAction = u"Follow up 6"_s;
    app6.nextActionDate = QDate(2026, 10, 11);
    QVERIFY(m_apps->insert(app6));

    // 7. App with null nextActionDate
    JobApplication app7;
    app7.company = u"App7"_s;
    app7.position = u"Dev"_s;
    app7.status = ApplicationStatus::Applied;
    app7.nextAction = u"Follow up 7"_s;
    QVERIFY(m_apps->insert(app7));

    // Query inclusive bounds [2026-10-01, 2026-10-10]
    const auto res = m_apps->byNextActionDateRange(QDate(2026, 10, 1), QDate(2026, 10, 10));
    QCOMPARE(res.size(), 2);
    // Ordered by next_action_date, id
    QCOMPARE(res[0].company, u"App2"_s);
    QCOMPARE(res[1].company, u"App1"_s);
}

void tst_ApplicationRepository::appliedCountInRangeInclusiveAndEmpty() {
    // Empty database case
    QCOMPARE(m_apps->appliedCountInRange(QDate(2026, 10, 1), QDate(2026, 10, 10)), 0);

    JobApplication app1;
    app1.company = u"A1"_s;
    app1.position = u"Dev"_s;
    app1.appliedDate = QDate(2026, 10, 1); // exactly at start
    QVERIFY(m_apps->insert(app1));

    JobApplication app2;
    app2.company = u"A2"_s;
    app2.position = u"Dev"_s;
    app2.appliedDate = QDate(2026, 10, 5); // inside range
    QVERIFY(m_apps->insert(app2));

    JobApplication app3;
    app3.company = u"A3"_s;
    app3.position = u"Dev"_s;
    app3.appliedDate = QDate(2026, 10, 10); // exactly at end
    QVERIFY(m_apps->insert(app3));

    JobApplication appBefore;
    appBefore.company = u"Before"_s;
    appBefore.position = u"Dev"_s;
    appBefore.appliedDate = QDate(2026, 9, 30);
    QVERIFY(m_apps->insert(appBefore));

    JobApplication appAfter;
    appAfter.company = u"After"_s;
    appAfter.position = u"Dev"_s;
    appAfter.appliedDate = QDate(2026, 10, 11);
    QVERIFY(m_apps->insert(appAfter));

    JobApplication appNullDate;
    appNullDate.company = u"NullDate"_s;
    appNullDate.position = u"Dev"_s;
    appNullDate.appliedDate = std::nullopt;
    QVERIFY(m_apps->insert(appNullDate));

    QCOMPARE(m_apps->appliedCountInRange(QDate(2026, 10, 1), QDate(2026, 10, 10)), 3);
}

void tst_ApplicationRepository::funnelStageCountsMatchesRanksAndHistory() {
    // Empty DB: all ranks 1..5 return 0
    auto countsEmpty = m_apps->funnelStageCounts();
    for (int r = 1; r <= 5; ++r) {
        QCOMPARE(countsEmpty.value(r, -1), 0);
    }

    // Insert applications:
    // 1. Wishlist (rank 0) - should not reach any rank 1..5
    JobApplication appWishlist;
    appWishlist.company = u"WishlistCorp"_s;
    appWishlist.position = u"Dev"_s;
    appWishlist.status = ApplicationStatus::Wishlist;
    QVERIFY(m_apps->insert(appWishlist));

    // 2. Applied (rank 1)
    JobApplication appApplied;
    appApplied.company = u"AppliedCorp"_s;
    appApplied.position = u"Dev"_s;
    appApplied.status = ApplicationStatus::Applied;
    QVERIFY(m_apps->insert(appApplied));

    // 3. Technical (rank 3) - reached ranks 1, 2, 3
    JobApplication appTech;
    appTech.company = u"TechCorp"_s;
    appTech.position = u"Dev"_s;
    appTech.status = ApplicationStatus::Technical;
    QVERIFY(m_apps->insert(appTech));

    // 4. Offer (rank 5) - reached ranks 1, 2, 3, 4, 5
    JobApplication appOffer;
    appOffer.company = u"OfferCorp"_s;
    appOffer.position = u"Dev"_s;
    appOffer.status = ApplicationStatus::Offer;
    QVERIFY(m_apps->insert(appOffer));

    // 5. Rejected application that reached Technical via status_history
    JobApplication appRejected;
    appRejected.company = u"RejectedCorp"_s;
    appRejected.position = u"Dev"_s;
    appRejected.status = ApplicationStatus::Applied;
    QVERIFY(m_apps->insert(appRejected));
    // Move to hr_screen, then technical, then rejected
    QVERIFY(m_apps->setStatus(appRejected.id, ApplicationStatus::HrScreen));
    QVERIFY(m_apps->setStatus(appRejected.id, ApplicationStatus::Technical));
    QVERIFY(m_apps->setStatus(appRejected.id, ApplicationStatus::Rejected));

    auto counts = m_apps->funnelStageCounts();
    // Rank 1 (Applied): AppliedCorp, TechCorp, OfferCorp, RejectedCorp = 4
    QCOMPARE(counts.value(1), 4);
    // Rank 2 (HrScreen): TechCorp, OfferCorp, RejectedCorp = 3
    QCOMPARE(counts.value(2), 3);
    // Rank 3 (Technical): TechCorp, OfferCorp, RejectedCorp = 3
    QCOMPARE(counts.value(3), 3);
    // Rank 4 (Final): OfferCorp = 1
    QCOMPARE(counts.value(4), 1);
    // Rank 5 (Offer): OfferCorp = 1
    QCOMPARE(counts.value(5), 1);
}

void tst_ApplicationRepository::byNextActionDateUpToQueries() {
    JobApplication app1;
    app1.company = u"Past1"_s;
    app1.position = u"Dev"_s;
    app1.status = ApplicationStatus::Applied;
    app1.nextAction = u"Action 1"_s;
    app1.nextActionDate = QDate(2026, 9, 20);
    QVERIFY(m_apps->insert(app1));

    JobApplication app2;
    app2.company = u"OnUpTo"_s;
    app2.position = u"Dev"_s;
    app2.status = ApplicationStatus::Applied;
    app2.nextAction = u"Action 2"_s;
    app2.nextActionDate = QDate(2026, 10, 10);
    QVERIFY(m_apps->insert(app2));

    JobApplication appFuture;
    appFuture.company = u"Future"_s;
    appFuture.position = u"Dev"_s;
    appFuture.status = ApplicationStatus::Applied;
    appFuture.nextAction = u"Action Future"_s;
    appFuture.nextActionDate = QDate(2026, 10, 11);
    QVERIFY(m_apps->insert(appFuture));

    // Closed applications must be excluded even if next_action_date is <= upTo
    JobApplication appRejected;
    appRejected.company = u"Closed"_s;
    appRejected.position = u"Dev"_s;
    appRejected.status = ApplicationStatus::Rejected;
    appRejected.nextAction = u"Action Closed"_s;
    appRejected.nextActionDate = QDate(2026, 10, 5);
    QVERIFY(m_apps->insert(appRejected));

    const auto res = m_apps->byNextActionDateUpTo(QDate(2026, 10, 10));
    QCOMPARE(res.size(), 2);
    QCOMPARE(res[0].company, u"Past1"_s);
    QCOMPARE(res[1].company, u"OnUpTo"_s);
}

QTEST_GUILESS_MAIN(tst_ApplicationRepository)
#include "tst_ApplicationRepository.moc"
