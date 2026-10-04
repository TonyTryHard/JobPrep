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
using JobPrep::Domain::Interview;
using JobPrep::Domain::InterviewOutcome;
using JobPrep::Domain::InterviewType;
using JobPrep::Domain::JobApplication;
using JobPrep::Tests::TestDatabase;

class tst_InterviewRepository : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void insertFillsFieldsAndRoundTrips();
    void updateStoresNewValues();
    void removeDeletesRow();
    void byApplicationFiltersRows();
    void inRangeIsInclusiveAndOrdered();
    void nextUpcomingSkipsPast();

private:
    int addApplication();
    int addInterview(const QDateTime& startAt);

    std::unique_ptr<TestDatabase> m_db;
    std::unique_ptr<ApplicationRepository> m_apps;
    std::unique_ptr<InterviewRepository> m_interviews;
};

int tst_InterviewRepository::addApplication() {
    JobApplication application;
    application.company = u"Globex"_s;
    application.position = u"Qt Engineer"_s;
    return m_apps->insert(application) ? application.id : 0;
}

int tst_InterviewRepository::addInterview(const QDateTime& startAt) {
    Interview interview;
    interview.applicationId = addApplication();
    interview.startAt = startAt;
    interview.type = InterviewType::Technical;
    interview.durationMin = 90;
    return m_interviews->insert(interview) ? interview.id : 0;
}

void tst_InterviewRepository::init() {
    m_db = std::make_unique<TestDatabase>(u"tst_interviews"_s);
    QVERIFY(m_db->open(u":memory:"_s));
    m_apps = std::make_unique<ApplicationRepository>(m_db->db());
    m_interviews = std::make_unique<InterviewRepository>(m_db->db());
}

void tst_InterviewRepository::cleanup() {
    m_interviews.reset();
    m_apps.reset();
    m_db.reset();
}

void tst_InterviewRepository::insertFillsFieldsAndRoundTrips() {
    const int applicationId = addApplication();

    Interview interview;
    interview.applicationId = applicationId;
    interview.startAt = QDateTime(QDate(2026, 10, 2), QTime(14, 0));
    interview.durationMin = 90;
    interview.type = InterviewType::SystemDesign;
    interview.place = u"Zoom"_s;
    interview.interviewer = u"Anna K."_s;
    interview.notes = u"Prepare the k8s story"_s;
    interview.outcome = InterviewOutcome::Pending;

    QSignalSpy spy(m_interviews.get(), &InterviewRepository::changed);
    QVERIFY(m_interviews->insert(interview));
    QVERIFY(interview.id > 0);
    QCOMPARE(spy.count(), 1);

    const auto stored = m_interviews->byId(interview.id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->applicationId, applicationId);
    QCOMPARE(stored->startAt, QDateTime(QDate(2026, 10, 2), QTime(14, 0)));
    QCOMPARE(stored->durationMin, 90);
    QVERIFY(stored->type == InterviewType::SystemDesign);
    QCOMPARE(stored->place, u"Zoom"_s);
    QCOMPARE(stored->interviewer, u"Anna K."_s);
    QCOMPARE(stored->notes, u"Prepare the k8s story"_s);
    QVERIFY(stored->outcome == InterviewOutcome::Pending);

    Interview orphan;
    orphan.applicationId = 999;
    orphan.startAt = QDateTime(QDate(2026, 10, 3), QTime(9, 0));
    orphan.durationMin = 60;
    QVERIFY(!m_interviews->insert(orphan));
    QCOMPARE(spy.count(), 1);
}

void tst_InterviewRepository::updateStoresNewValues() {
    const int id = addInterview(QDateTime(QDate(2026, 10, 2), QTime(14, 0)));

    Interview interview = *m_interviews->byId(id);
    interview.startAt = QDateTime(QDate(2026, 10, 3), QTime(10, 30));
    interview.outcome = InterviewOutcome::Passed;
    interview.durationMin = 45;

    QVERIFY(m_interviews->update(interview));
    const auto stored = m_interviews->byId(id);
    QCOMPARE(stored->startAt, QDateTime(QDate(2026, 10, 3), QTime(10, 30)));
    QVERIFY(stored->outcome == InterviewOutcome::Passed);
    QCOMPARE(stored->durationMin, 45);

    Interview unknown;
    unknown.id = 999;
    unknown.applicationId = 1;
    unknown.startAt = QDateTime(QDate(2026, 10, 3), QTime(10, 30));
    unknown.durationMin = 60;
    QVERIFY(!m_interviews->update(unknown));
    QVERIFY(!m_interviews->lastError().isEmpty());
}

void tst_InterviewRepository::removeDeletesRow() {
    const int id = addInterview(QDateTime(QDate(2026, 10, 2), QTime(14, 0)));

    QSignalSpy spy(m_interviews.get(), &InterviewRepository::changed);
    QVERIFY(m_interviews->remove(id));
    QCOMPARE(spy.count(), 1);
    QVERIFY(!m_interviews->byId(id).has_value());
    QVERIFY(!m_interviews->remove(id));
    QCOMPARE(spy.count(), 1);
}

void tst_InterviewRepository::byApplicationFiltersRows() {
    const int first = addApplication();
    JobApplication second;
    second.company = u"Acme"_s;
    second.position = u"C++ Developer"_s;
    QVERIFY(m_apps->insert(second));

    Interview firstInterview;
    firstInterview.applicationId = first;
    firstInterview.startAt = QDateTime(QDate(2026, 10, 2), QTime(9, 0));
    firstInterview.durationMin = 60;
    QVERIFY(m_interviews->insert(firstInterview));

    Interview hrInterview;
    hrInterview.applicationId = first;
    hrInterview.startAt = QDateTime(QDate(2026, 10, 1), QTime(11, 0));
    hrInterview.type = InterviewType::Hr;
    hrInterview.durationMin = 45;
    QVERIFY(m_interviews->insert(hrInterview));

    Interview otherInterview;
    otherInterview.applicationId = second.id;
    otherInterview.startAt = QDateTime(QDate(2026, 10, 5), QTime(15, 0));
    otherInterview.durationMin = 60;
    QVERIFY(m_interviews->insert(otherInterview));

    const auto rows = m_interviews->byApplication(first);
    QCOMPARE(rows.size(), 2);
    QCOMPARE(rows[0].startAt, QDateTime(QDate(2026, 10, 1), QTime(11, 0)));
    QCOMPARE(m_interviews->byApplication(second.id).size(), 1);
    QCOMPARE(m_interviews->byApplication(999).size(), 0);
    QCOMPARE(m_interviews->all().size(), 3);
}

void tst_InterviewRepository::inRangeIsInclusiveAndOrdered() {
    const QList<QDateTime> starts{QDateTime(QDate(2026, 9, 30), QTime(9, 0)),
                                  QDateTime(QDate(2026, 10, 1), QTime(9, 0)),
                                  QDateTime(QDate(2026, 10, 1), QTime(17, 0)),
                                  QDateTime(QDate(2026, 10, 7), QTime(9, 0))};
    for (const QDateTime& start : starts) QVERIFY(addInterview(start) > 0);

    const auto range = m_interviews->inRange(QDateTime(QDate(2026, 10, 1), QTime(9, 0)),
                                             QDateTime(QDate(2026, 10, 1), QTime(17, 0)));
    QCOMPARE(range.size(), 2);
    QCOMPARE(range[0].startAt, starts[1]);
    QCOMPARE(range[1].startAt, starts[2]);

    QCOMPARE(m_interviews->inRange(QDateTime(QDate(2026, 11, 1), QTime(0, 0)),
                                   QDateTime(QDate(2026, 11, 30), QTime(23, 59)))
                 .size(),
             0);
}

void tst_InterviewRepository::nextUpcomingSkipsPast() {
    const int past = addInterview(QDateTime(QDate(2026, 9, 30), QTime(9, 0)));
    const int soon = addInterview(QDateTime(QDate(2026, 10, 2), QTime(9, 0)));
    const int later = addInterview(QDateTime(QDate(2026, 10, 5), QTime(9, 0)));

    const QDateTime now(QDate(2026, 10, 1), QTime(12, 0));
    const auto upcoming = m_interviews->nextUpcoming(now);
    QVERIFY(upcoming.has_value());
    QCOMPARE(upcoming->id, soon);
    QCOMPARE(upcoming->id, m_interviews->byId(soon)->id);

    QVERIFY(!m_interviews->nextUpcoming(QDateTime(QDate(2027, 1, 1), QTime(0, 0))).has_value());
    QVERIFY(m_interviews->byId(past).has_value());
    QVERIFY(m_interviews->byId(later).has_value());
}

QTEST_GUILESS_MAIN(tst_InterviewRepository)
#include "tst_InterviewRepository.moc"
