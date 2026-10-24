#include <QDate>
#include <QSignalSpy>
#include <QTest>
#include "MessageCapture.h"
#include "TestDatabase.h"
#include "data/SessionRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"

using namespace Qt::StringLiterals;
using JobPrep::Data::SessionRepository;
using JobPrep::Data::TopicRepository;
using JobPrep::Data::TrackRepository;
using JobPrep::Domain::StudySession;
using JobPrep::Domain::Topic;
using JobPrep::Domain::Track;
using JobPrep::Tests::MessageCapture;
using JobPrep::Tests::TestDatabase;

class tst_SessionRepository : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void insertFillsIdAndRoundTrips();
    void insertWithoutTopicIsAllowed();
    void updateStoresNewValues();
    void removeDeletesRow();
    void inRangeIsInclusiveAndOrdered();
    void byTopicFiltersSessions();
    void totalMinutesSumsRange();
    void nonPositiveMinutesRejected();
    void unknownIdReturnsNullopt();
    void distinctSessionDatesReturnsDescendingUpToOnly();

private:
    int addTopic();

    std::unique_ptr<TestDatabase> m_db;
    std::unique_ptr<TrackRepository> m_tracks;
    std::unique_ptr<TopicRepository> m_topics;
    std::unique_ptr<SessionRepository> m_sessions;
};

int tst_SessionRepository::addTopic() {
    Track track;
    track.name = u"C++"_s;
    track.color = u"#5B6CFF"_s;
    if (!m_tracks->insert(track)) return 0;

    Topic topic;
    topic.trackId = track.id;
    topic.title = u"Move semantics"_s;
    return m_topics->insert(topic) ? topic.id : 0;
}

void tst_SessionRepository::init() {
    m_db = std::make_unique<TestDatabase>(u"tst_sessions"_s);
    QVERIFY(m_db->open(u":memory:"_s));
    m_tracks = std::make_unique<TrackRepository>(m_db->db());
    m_topics = std::make_unique<TopicRepository>(m_db->db());
    m_sessions = std::make_unique<SessionRepository>(m_db->db());
}

void tst_SessionRepository::cleanup() {
    m_sessions.reset();
    m_topics.reset();
    m_tracks.reset();
    m_db.reset();
}

void tst_SessionRepository::insertFillsIdAndRoundTrips() {
    const int topicId = addTopic();

    StudySession session;
    session.topicId = topicId;
    session.sessionDate = QDate(2026, 10, 4);
    session.minutes = 45;
    session.note = u"Move semantics chapter"_s;

    QSignalSpy spy(m_sessions.get(), &SessionRepository::changed);
    QVERIFY(m_sessions->insert(session));
    QVERIFY(session.id > 0);
    QCOMPARE(spy.count(), 1);

    const auto stored = m_sessions->byId(session.id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->sessionDate, QDate(2026, 10, 4));
    QCOMPARE(stored->minutes, 45);
    QCOMPARE(stored->note, u"Move semantics chapter"_s);
    QVERIFY(stored->topicId.has_value());
    QCOMPARE(*stored->topicId, topicId);
}

void tst_SessionRepository::insertWithoutTopicIsAllowed() {
    StudySession session;
    session.sessionDate = QDate(2026, 10, 5);
    session.minutes = 30;
    QVERIFY(m_sessions->insert(session));

    const auto stored = m_sessions->byId(session.id);
    QVERIFY(stored.has_value());
    QVERIFY(!stored->topicId.has_value());
}

void tst_SessionRepository::updateStoresNewValues() {
    const int topicId = addTopic();
    StudySession session;
    session.topicId = topicId;
    session.sessionDate = QDate(2026, 10, 4);
    session.minutes = 45;
    QVERIFY(m_sessions->insert(session));

    session.minutes = 90;
    session.sessionDate = QDate(2026, 10, 6);
    session.note = u"long session"_s;
    QVERIFY(m_sessions->update(session));

    const auto stored = m_sessions->byId(session.id);
    QCOMPARE(stored->minutes, 90);
    QCOMPARE(stored->sessionDate, QDate(2026, 10, 6));
    QCOMPARE(stored->note, u"long session"_s);

    StudySession unknown;
    unknown.id = 999;
    unknown.sessionDate = QDate(2026, 10, 6);
    unknown.minutes = 15;
    QVERIFY(!m_sessions->update(unknown));
    QVERIFY(!m_sessions->lastError().isEmpty());
}

void tst_SessionRepository::removeDeletesRow() {
    StudySession session;
    session.sessionDate = QDate(2026, 10, 4);
    session.minutes = 20;
    QVERIFY(m_sessions->insert(session));

    QSignalSpy spy(m_sessions.get(), &SessionRepository::changed);
    QVERIFY(m_sessions->remove(session.id));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(m_sessions->count(), 0);
    QVERIFY(!m_sessions->remove(session.id));
    QCOMPARE(spy.count(), 1);
}

void tst_SessionRepository::inRangeIsInclusiveAndOrdered() {
    const int topicId = addTopic();
    const QList<QDate> dates{QDate(2026, 9, 30), QDate(2026, 10, 1), QDate(2026, 10, 3),
                            QDate(2026, 10, 7)};
    for (const QDate& date : dates) {
        StudySession session;
        session.topicId = topicId;
        session.sessionDate = date;
        session.minutes = 60;
        QVERIFY(m_sessions->insert(session));
    }

    const auto range = m_sessions->inRange(QDate(2026, 10, 1), QDate(2026, 10, 7));
    QCOMPARE(range.size(), 3);
    QCOMPARE(range[0].sessionDate, QDate(2026, 10, 1));
    QCOMPARE(range[1].sessionDate, QDate(2026, 10, 3));
    QCOMPARE(range[2].sessionDate, QDate(2026, 10, 7));

    QCOMPARE(m_sessions->inRange(QDate(2026, 10, 2), QDate(2026, 10, 6)).size(), 1);
    QCOMPARE(m_sessions->inRange(QDate(2027, 1, 1), QDate(2027, 12, 31)).size(), 0);
}

void tst_SessionRepository::byTopicFiltersSessions() {
    const int first = addTopic();
    Track track;
    track.name = u"English"_s;
    track.color = u"#F59E0B"_s;
    QVERIFY(m_tracks->insert(track));
    Topic second;
    second.trackId = track.id;
    second.title = u"Listening practice"_s;
    QVERIFY(m_topics->insert(second));

    StudySession firstSession;
    firstSession.topicId = first;
    firstSession.sessionDate = QDate(2026, 10, 2);
    firstSession.minutes = 30;
    QVERIFY(m_sessions->insert(firstSession));

    StudySession secondSession;
    secondSession.topicId = second.id;
    secondSession.sessionDate = QDate(2026, 10, 2);
    secondSession.minutes = 25;
    QVERIFY(m_sessions->insert(secondSession));

    QCOMPARE(m_sessions->byTopic(first).size(), 1);
    QCOMPARE(m_sessions->byTopic(second.id).first().id, secondSession.id);
    QCOMPARE(m_sessions->byTopic(999).size(), 0);
    QCOMPARE(m_sessions->all().size(), 2);
}

void tst_SessionRepository::totalMinutesSumsRange() {
    const int topicId = addTopic();
    const QList<QPair<QDate, int>> entries{
        {QDate(2026, 10, 5), 60}, {QDate(2026, 10, 6), 45}, {QDate(2026, 10, 12), 30}};
    for (const auto& [date, minutes] : entries) {
        StudySession session;
        session.topicId = topicId;
        session.sessionDate = date;
        session.minutes = minutes;
        QVERIFY(m_sessions->insert(session));
    }

    QCOMPARE(m_sessions->totalMinutesInRange(QDate(2026, 10, 5), QDate(2026, 10, 12)), 135);
    QCOMPARE(m_sessions->totalMinutesInRange(QDate(2026, 10, 5), QDate(2026, 10, 6)), 105);
    QCOMPARE(m_sessions->totalMinutesInRange(QDate(2027, 1, 1), QDate(2027, 1, 31)), 0);
}

void tst_SessionRepository::nonPositiveMinutesRejected() {
    StudySession session;
    session.sessionDate = QDate(2026, 10, 4);
    session.minutes = 0;

    QSignalSpy spy(m_sessions.get(), &SessionRepository::changed);
    MessageCapture capture;
    QVERIFY(!m_sessions->insert(session));
    QVERIFY(!m_sessions->lastError().isEmpty());
    QCOMPARE(session.id, 0);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(m_sessions->count(), 0);
}

void tst_SessionRepository::unknownIdReturnsNullopt() {
    QVERIFY(!m_sessions->byId(999).has_value());
    QCOMPARE(m_sessions->lastError(), QString());
}

void tst_SessionRepository::distinctSessionDatesReturnsDescendingUpToOnly() {
    // 1. Empty DB case
    QCOMPARE(m_sessions->distinctSessionDates(QDate(2026, 10, 10)), QList<QDate>());

    // 2. Populate sessions
    const int topicId = addTopic();

    // 2026-10-01
    StudySession s1;
    s1.topicId = topicId;
    s1.sessionDate = QDate(2026, 10, 1);
    s1.minutes = 30;
    QVERIFY(m_sessions->insert(s1));

    // 2026-10-03 (session A)
    StudySession s2;
    s2.topicId = topicId;
    s2.sessionDate = QDate(2026, 10, 3);
    s2.minutes = 45;
    QVERIFY(m_sessions->insert(s2));

    // 2026-10-03 (session B, same date)
    StudySession s3;
    s3.sessionDate = QDate(2026, 10, 3);
    s3.minutes = 15;
    QVERIFY(m_sessions->insert(s3));

    // 2026-10-05 (on upTo)
    StudySession s4;
    s4.sessionDate = QDate(2026, 10, 5);
    s4.minutes = 60;
    QVERIFY(m_sessions->insert(s4));

    // 2026-10-06 (after upTo)
    StudySession s5;
    s5.sessionDate = QDate(2026, 10, 6);
    s5.minutes = 20;
    QVERIFY(m_sessions->insert(s5));

    const auto dates = m_sessions->distinctSessionDates(QDate(2026, 10, 5));
    QCOMPARE(dates.size(), 3);
    // Descending order: 10-05, 10-03, 10-01
    QCOMPARE(dates[0], QDate(2026, 10, 5));
    QCOMPARE(dates[1], QDate(2026, 10, 3));
    QCOMPARE(dates[2], QDate(2026, 10, 1));
}

QTEST_GUILESS_MAIN(tst_SessionRepository)
#include "tst_SessionRepository.moc"
