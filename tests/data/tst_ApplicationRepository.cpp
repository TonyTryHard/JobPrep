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

QTEST_GUILESS_MAIN(tst_ApplicationRepository)
#include "tst_ApplicationRepository.moc"
