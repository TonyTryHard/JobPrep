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
using JobPrep::Domain::Subtask;
using JobPrep::Domain::Topic;
using JobPrep::Domain::Track;
using JobPrep::Tests::MessageCapture;
using JobPrep::Tests::TestDatabase;

class tst_TopicRepository : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void insertFillsIdAndTimestamps();
    void resourcesRoundTripUnchanged();
    void updateStoresNewValuesAndBumpsTimestamp();
    void setStatusWritesStatusOnly();
    void updatePositionsRewritesOrder();
    void removeKeepsSessionsButClearsTopic();
    void subtaskLifecycle();
    void reorderSubtasksRewritesOrder();
    void unknownIdsFailWithoutChangedSignal();
    void queriesByTrackAndCount();

private:
    int addTrack(const QString& name);
    int addTopic(const QString& title, int trackId);

    std::unique_ptr<TestDatabase> m_db;
    std::unique_ptr<TrackRepository> m_tracks;
    std::unique_ptr<TopicRepository> m_topics;
    std::unique_ptr<SessionRepository> m_sessions;
};

int tst_TopicRepository::addTrack(const QString& name) {
    Track track;
    track.name = name;
    track.color = u"#5B6CFF"_s;
    const bool inserted = m_tracks->insert(track);
    return inserted ? track.id : 0;
}

int tst_TopicRepository::addTopic(const QString& title, int trackId) {
    Topic topic;
    topic.trackId = trackId;
    topic.title = title;
    const bool inserted = m_topics->insert(topic);
    return inserted ? topic.id : 0;
}

void tst_TopicRepository::init() {
    m_db = std::make_unique<TestDatabase>(u"tst_topics"_s);
    QVERIFY(m_db->open(u":memory:"_s));
    m_tracks = std::make_unique<TrackRepository>(m_db->db());
    m_topics = std::make_unique<TopicRepository>(m_db->db());
    m_sessions = std::make_unique<SessionRepository>(m_db->db());
}

void tst_TopicRepository::cleanup() {
    m_sessions.reset();
    m_topics.reset();
    m_tracks.reset();
    m_db.reset();
}

void tst_TopicRepository::insertFillsIdAndTimestamps() {
    const int trackId = addTrack(u"C++"_s);

    Topic topic;
    topic.trackId = trackId;
    topic.title = u"Move semantics"_s;
    topic.status = JobPrep::Domain::TopicStatus::InProgress;
    topic.priority = JobPrep::Domain::Priority::High;
    topic.targetDate = QDate(2026, 10, 20);
    topic.notes = u"value categories"_s;
    topic.tags = u"move, rvalue"_s;
    topic.position = 7;

    QVERIFY(m_topics->insert(topic));
    QVERIFY(topic.id > 0);
    QVERIFY(topic.createdAt.isValid());
    QVERIFY(topic.updatedAt.isValid());

    const auto stored = m_topics->byId(topic.id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->title, u"Move semantics"_s);
    QVERIFY(stored->status == JobPrep::Domain::TopicStatus::InProgress);
    QVERIFY(stored->priority == JobPrep::Domain::Priority::High);
    QCOMPARE(stored->targetDate, std::optional<QDate>(QDate(2026, 10, 20)));
    QCOMPARE(stored->notes, u"value categories"_s);
    QCOMPARE(stored->tags, u"move, rvalue"_s);
    QCOMPARE(stored->position, 7);
    QCOMPARE(stored->createdAt, topic.createdAt);
}

void tst_TopicRepository::resourcesRoundTripUnchanged() {
    const int trackId = addTrack(u"C++"_s);
    const QString resources = u"[{\"title\":\"cppreference\",\"url\":\"https://en.cppreference.com\"}]"_s;

    Topic topic;
    topic.trackId = trackId;
    topic.title = u"Templates"_s;
    topic.resources = resources;
    QVERIFY(m_topics->insert(topic));

    QCOMPARE(m_topics->byId(topic.id)->resources, resources);

    // A plain update must never wipe the stored JSON (AGENTS §8: no data loss).
    Topic update = *m_topics->byId(topic.id);
    update.notes = u"variadic"_s;
    QVERIFY(m_topics->update(update));

    const auto stored = m_topics->byId(topic.id);
    QCOMPARE(stored->resources, resources);
    QCOMPARE(stored->notes, u"variadic"_s);
}

void tst_TopicRepository::updateStoresNewValuesAndBumpsTimestamp() {
    const int trackId = addTrack(u"C++"_s);
    const int topicId = addTopic(u"RAII"_s, trackId);

    Topic topic = *m_topics->byId(topicId);
    const QDateTime createdAt = topic.createdAt;
    topic.title = u"RAII and smart pointers"_s;
    topic.status = JobPrep::Domain::TopicStatus::Review;
    topic.targetDate = QDate(2026, 11, 1);
    QVERIFY(m_topics->update(topic));

    const auto stored = m_topics->byId(topicId);
    QCOMPARE(stored->title, u"RAII and smart pointers"_s);
    QVERIFY(stored->status == JobPrep::Domain::TopicStatus::Review);
    QCOMPARE(stored->targetDate, std::optional<QDate>(QDate(2026, 11, 1)));
    QCOMPARE(stored->createdAt, createdAt);
    QVERIFY(stored->updatedAt >= createdAt);

    Topic unknown;
    unknown.id = 999;
    unknown.trackId = trackId;
    unknown.title = u"Ghost"_s;
    QVERIFY(!m_topics->update(unknown));
}

void tst_TopicRepository::setStatusWritesStatusOnly() {
    const int trackId = addTrack(u"C++"_s);
    const int topicId = addTopic(u"STL"_s, trackId);

    QSignalSpy spy(m_topics.get(), &TopicRepository::changed);
    QVERIFY(m_topics->setStatus(topicId, JobPrep::Domain::TopicStatus::Done));
    QCOMPARE(spy.count(), 1);

    const auto stored = m_topics->byId(topicId);
    QVERIFY(stored->status == JobPrep::Domain::TopicStatus::Done);
    QCOMPARE(stored->title, u"STL"_s);
    QVERIFY(!m_topics->setStatus(999, JobPrep::Domain::TopicStatus::Done));
    QCOMPARE(spy.count(), 1);
}

void tst_TopicRepository::updatePositionsRewritesOrder() {
    const int trackId = addTrack(u"C++"_s);
    const int first = addTopic(u"First"_s, trackId);
    const int second = addTopic(u"Second"_s, trackId);
    const int third = addTopic(u"Third"_s, trackId);

    QSignalSpy spy(m_topics.get(), &TopicRepository::changed);
    QVERIFY(m_topics->updatePositions({third, first, second}));
    QCOMPARE(spy.count(), 1);

    QCOMPARE(m_topics->byId(first)->position, 1);
    QCOMPARE(m_topics->byId(second)->position, 2);
    QCOMPARE(m_topics->byId(third)->position, 0);

    // An unknown id fails the whole reorder and emits nothing.
    QVERIFY(!m_topics->updatePositions({first, 999}));
    QVERIFY(!m_topics->lastError().isEmpty());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(m_topics->byId(third)->position, 0);
}

void tst_TopicRepository::removeKeepsSessionsButClearsTopic() {
    const int trackId = addTrack(u"C++"_s);
    const int topicId = addTopic(u"Concurrency"_s, trackId);

    StudySession session;
    session.topicId = topicId;
    session.sessionDate = QDate(2026, 10, 3);
    session.minutes = 45;
    QVERIFY(m_sessions->insert(session));

    QVERIFY(m_topics->remove(topicId));

    QVERIFY(m_topics->isEmpty());
    // study_sessions.topic_id is ON DELETE SET NULL, so the session row survives.
    QCOMPARE(m_sessions->count(), 1);
    QCOMPARE(m_sessions->all().size(), 1);
    QVERIFY(!m_sessions->all().first().topicId.has_value());
    QVERIFY(!m_topics->remove(999));
}

void tst_TopicRepository::subtaskLifecycle() {
    const int trackId = addTrack(u"C++"_s);
    const int topicId = addTopic(u"Qt essentials"_s, trackId);

    Subtask first;
    first.topicId = topicId;
    first.text = u"Read cppreference page"_s;
    QVERIFY(m_topics->addSubtask(first));
    QVERIFY(first.id > 0);
    QCOMPARE(m_topics->subtaskCount(topicId), 1);
    QCOMPARE(m_topics->doneSubtaskCount(topicId), 0);

    Subtask second;
    second.topicId = topicId;
    second.text = u"Write a 30-line example"_s;
    second.position = 1;
    QVERIFY(m_topics->addSubtask(second));

    QCOMPARE(m_topics->subtasks(topicId).size(), 2);
    QCOMPARE(m_topics->subtasks(topicId).first().text, u"Read cppreference page"_s);

    QVERIFY(m_topics->setSubtaskDone(first.id, true));
    QCOMPARE(m_topics->doneSubtaskCount(topicId), 1);
    QVERIFY(m_topics->subtasks(topicId).first().done);

    second.text = u"Write a 60-line example"_s;
    second.done = true;
    QVERIFY(m_topics->updateSubtask(second));
    QCOMPARE(m_topics->doneSubtaskCount(topicId), 2);

    QVERIFY(m_topics->removeSubtask(first.id));
    QCOMPARE(m_topics->subtaskCount(topicId), 1);
    QCOMPARE(m_topics->doneSubtaskCount(topicId), 1);

    // Subtasks of a deleted topic disappear with it.
    QVERIFY(m_topics->remove(topicId));
    QCOMPARE(m_db->countRows(u"subtasks"_s), 0);

    QVERIFY(!m_topics->setSubtaskDone(999, true));
    QVERIFY(!m_topics->removeSubtask(999));
}

void tst_TopicRepository::reorderSubtasksRewritesOrder() {
    const int trackId = addTrack(u"C++"_s);
    const int topicId = addTopic(u"Build and tooling"_s, trackId);

    Subtask first;
    first.topicId = topicId;
    first.text = u"one"_s;
    QVERIFY(m_topics->addSubtask(first));
    Subtask second;
    second.topicId = topicId;
    second.text = u"two"_s;
    QVERIFY(m_topics->addSubtask(second));
    Subtask third;
    third.topicId = topicId;
    third.text = u"three"_s;
    QVERIFY(m_topics->addSubtask(third));

    QSignalSpy spy(m_topics.get(), &TopicRepository::changed);
    QVERIFY(m_topics->reorderSubtasks(topicId, {third.id, first.id, second.id}));
    QCOMPARE(spy.count(), 1);

    const auto ordered = m_topics->subtasks(topicId);
    QCOMPARE(ordered.size(), 3);
    QCOMPARE(ordered[0].text, u"three"_s);
    QCOMPARE(ordered[1].text, u"one"_s);
    QCOMPARE(ordered[2].text, u"two"_s);

    // A subtask that belongs to another topic must not be reordered here.
    const int otherTopic = addTopic(u"Other"_s, trackId);
    Subtask foreign;
    foreign.topicId = otherTopic;
    foreign.text = u"foreign"_s;
    QVERIFY(m_topics->addSubtask(foreign));

    QSignalSpy failedSpy(m_topics.get(), &TopicRepository::changed);
    QVERIFY(!m_topics->reorderSubtasks(topicId, {foreign.id}));
    QCOMPARE(failedSpy.count(), 0);
}

void tst_TopicRepository::unknownIdsFailWithoutChangedSignal() {
    QSignalSpy spy(m_topics.get(), &TopicRepository::changed);
    QVERIFY(!m_topics->byId(999).has_value());
    QCOMPARE(m_topics->subtasks(999).size(), 0);
    QCOMPARE(m_topics->subtaskCount(999), 0);
    QCOMPARE(m_topics->doneSubtaskCount(999), 0);

    MessageCapture capture;
    QVERIFY(!m_topics->remove(999));
    QVERIFY(!m_topics->setSubtaskDone(999, true));
    QVERIFY(!m_topics->removeSubtask(999));
    QCOMPARE(spy.count(), 0);
    QVERIFY2(capture.messages().isEmpty(), "an unknown id must not log a warning");
}

void tst_TopicRepository::queriesByTrackAndCount() {
    const int cpp = addTrack(u"C++"_s);
    const int english = addTrack(u"English"_s);
    addTopic(u"RAII"_s, cpp);
    addTopic(u"Templates"_s, cpp);
    addTopic(u"Grammar refresh"_s, english);

    QCOMPARE(m_topics->count(), 3);
    QCOMPARE(m_topics->byTrack(cpp).size(), 2);
    QCOMPARE(m_topics->byTrack(english).size(), 1);
    QCOMPARE(m_topics->byTrack(999).size(), 0);
    QCOMPARE(m_topics->all().size(), 3);
    QVERIFY(!m_topics->isEmpty());

    Topic orphan;
    orphan.trackId = 999;
    orphan.title = u"Orphan"_s;
    QVERIFY(!m_topics->insert(orphan));
    QCOMPARE(m_topics->count(), 3);
}

QTEST_GUILESS_MAIN(tst_TopicRepository)
#include "tst_TopicRepository.moc"
