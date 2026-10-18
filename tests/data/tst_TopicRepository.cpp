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
using JobPrep::Domain::TopicStatus;
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
    void byTargetDateRangeQueries();
    void moveTopicAcrossStatus();
    void moveTopicReorderInsideColumn();
    void moveTopicTrackFilteredPreservesHiddenOrder();
    void moveTopicAppendAtEnd();
    void moveTopicUnknownIdAndNoOp();
    void subtaskProgressBatchedQuery();

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

void tst_TopicRepository::byTargetDateRangeQueries() {
    const int trackId = addTrack(u"C++"_s);

    // 1. Open topic (backlog) with target date in range
    Topic t1;
    t1.trackId = trackId;
    t1.title = u"Move semantics"_s;
    t1.status = TopicStatus::Backlog;
    t1.targetDate = QDate(2026, 10, 5);
    QVERIFY(m_topics->insert(t1));

    // 2. Open topic (in_progress) with target date at start bound
    Topic t2;
    t2.trackId = trackId;
    t2.title = u"RAII"_s;
    t2.status = TopicStatus::InProgress;
    t2.targetDate = QDate(2026, 10, 1);
    QVERIFY(m_topics->insert(t2));

    // 3. Done topic with target date in range (should be excluded)
    Topic t3;
    t3.trackId = trackId;
    t3.title = u"Auto keyword"_s;
    t3.status = TopicStatus::Done;
    t3.targetDate = QDate(2026, 10, 5);
    QVERIFY(m_topics->insert(t3));

    // 4. Topic with target date before range
    Topic t4;
    t4.trackId = trackId;
    t4.title = u"Pointers"_s;
    t4.status = TopicStatus::Backlog;
    t4.targetDate = QDate(2026, 9, 30);
    QVERIFY(m_topics->insert(t4));

    // 5. Topic with target date after range
    Topic t5;
    t5.trackId = trackId;
    t5.title = u"Coroutines"_s;
    t5.status = TopicStatus::Backlog;
    t5.targetDate = QDate(2026, 10, 11);
    QVERIFY(m_topics->insert(t5));

    // 6. Topic with null target date
    Topic t6;
    t6.trackId = trackId;
    t6.title = u"Lambdas"_s;
    t6.status = TopicStatus::Backlog;
    QVERIFY(m_topics->insert(t6));

    const auto res = m_topics->byTargetDateRange(QDate(2026, 10, 1), QDate(2026, 10, 10));
    QCOMPARE(res.size(), 2);
    // Ordered by target_date, position, id
    QCOMPARE(res[0].title, u"RAII"_s);
    QCOMPARE(res[1].title, u"Move semantics"_s);
}

void tst_TopicRepository::moveTopicAcrossStatus() {
    const int trackId = addTrack(u"C++"_s);
    const int t1 = addTopic(u"Topic 1"_s, trackId);
    const int t2 = addTopic(u"Topic 2"_s, trackId);

    QVERIFY(m_topics->setStatus(t2, TopicStatus::InProgress));

    QSignalSpy spy(m_topics.get(), &TopicRepository::changed);
    QVERIFY(m_topics->moveTopic(t1, TopicStatus::InProgress, {t1, t2}));
    QCOMPARE(spy.count(), 1);

    const auto stored1 = m_topics->byId(t1);
    QVERIFY(stored1.has_value());
    QCOMPARE(stored1->status, TopicStatus::InProgress);

    const auto inProg = m_topics->all();
    int pos1 = -1, pos2 = -1;
    for (const auto& t : inProg) {
        if (t.id == t1) pos1 = t.position;
        if (t.id == t2) pos2 = t.position;
    }
    QVERIFY(pos1 < pos2);
}

void tst_TopicRepository::moveTopicReorderInsideColumn() {
    const int trackId = addTrack(u"C++"_s);
    const int t1 = addTopic(u"One"_s, trackId);
    const int t2 = addTopic(u"Two"_s, trackId);
    const int t3 = addTopic(u"Three"_s, trackId);

    QSignalSpy spy(m_topics.get(), &TopicRepository::changed);
    QVERIFY(m_topics->moveTopic(t3, TopicStatus::Backlog, {t3, t1, t2}));
    QCOMPARE(spy.count(), 1);

    const auto all = m_topics->all();
    QCOMPARE(all.size(), 3);
    QCOMPARE(all[0].id, t3);
    QCOMPARE(all[1].id, t1);
    QCOMPARE(all[2].id, t2);
}

void tst_TopicRepository::moveTopicTrackFilteredPreservesHiddenOrder() {
    const int cpp = addTrack(u"C++"_s);
    const int eng = addTrack(u"English"_s);

    const int c1 = addTopic(u"C++ 1"_s, cpp);
    const int e1 = addTopic(u"English 1"_s, eng);
    const int c2 = addTopic(u"C++ 2"_s, cpp);
    const int e2 = addTopic(u"English 2"_s, eng);

    QSignalSpy spy(m_topics.get(), &TopicRepository::changed);
    QVERIFY(m_topics->moveTopic(c2, TopicStatus::Backlog, {c2, c1}));
    QCOMPARE(spy.count(), 1);

    const auto cppTopics = m_topics->byTrack(cpp);
    QCOMPARE(cppTopics.size(), 2);
    QCOMPARE(cppTopics[0].id, c2);
    QCOMPARE(cppTopics[1].id, c1);

    const auto engTopics = m_topics->byTrack(eng);
    QCOMPARE(engTopics.size(), 2);
    QCOMPARE(engTopics[0].id, e1);
    QCOMPARE(engTopics[1].id, e2);
}

void tst_TopicRepository::moveTopicAppendAtEnd() {
    const int trackId = addTrack(u"C++"_s);
    const int t1 = addTopic(u"One"_s, trackId);
    const int t2 = addTopic(u"Two"_s, trackId);
    QVERIFY(m_topics->setStatus(t1, TopicStatus::Review));

    QSignalSpy spy(m_topics.get(), &TopicRepository::changed);
    QVERIFY(m_topics->moveTopic(t2, TopicStatus::Review, {t1, t2}));
    QCOMPARE(spy.count(), 1);

    QCOMPARE(m_topics->byId(t2)->status, TopicStatus::Review);
    QVERIFY(m_topics->byId(t1)->position < m_topics->byId(t2)->position);
}

void tst_TopicRepository::moveTopicUnknownIdAndNoOp() {
    const int trackId = addTrack(u"C++"_s);
    const int t1 = addTopic(u"One"_s, trackId);
    const int t2 = addTopic(u"Two"_s, trackId);

    QSignalSpy spy(m_topics.get(), &TopicRepository::changed);

    // No-op move
    QVERIFY(m_topics->moveTopic(t1, TopicStatus::Backlog, {t1, t2}));
    QCOMPARE(spy.count(), 0);

    // Unknown id
    QVERIFY(!m_topics->moveTopic(999, TopicStatus::Backlog, {999}));
    QVERIFY(!m_topics->lastError().isEmpty());
    QCOMPARE(spy.count(), 0);

    // Unknown id in orderedIds
    QVERIFY(!m_topics->moveTopic(t1, TopicStatus::Backlog, {t1, 999}));
    QVERIFY(!m_topics->lastError().isEmpty());
    QCOMPARE(spy.count(), 0);

    // Missing id from orderedIds
    QVERIFY(!m_topics->moveTopic(t1, TopicStatus::Backlog, {t2}));
    QVERIFY(!m_topics->lastError().isEmpty());
    QCOMPARE(spy.count(), 0);
}

void tst_TopicRepository::subtaskProgressBatchedQuery() {
    const int trackId = addTrack(u"C++"_s);
    const int t1 = addTopic(u"Topic 1"_s, trackId);
    const int t2 = addTopic(u"Topic 2"_s, trackId);
    const int t3 = addTopic(u"Topic 3"_s, trackId);

    Subtask s1; s1.topicId = t1; s1.text = u"Sub 1"_s; s1.done = true;
    QVERIFY(m_topics->addSubtask(s1));
    Subtask s2; s2.topicId = t1; s2.text = u"Sub 2"_s; s2.done = false;
    QVERIFY(m_topics->addSubtask(s2));

    Subtask s3; s3.topicId = t2; s3.text = u"Sub 3"_s; s3.done = true;
    QVERIFY(m_topics->addSubtask(s3));
    Subtask s4; s4.topicId = t2; s4.text = u"Sub 4"_s; s4.done = true;
    QVERIFY(m_topics->addSubtask(s4));
    Subtask s5; s5.topicId = t2; s5.text = u"Sub 5"_s; s5.done = true;
    QVERIFY(m_topics->addSubtask(s5));

    const auto progress = m_topics->subtaskProgress();
    QCOMPARE(progress.size(), 2);

    QVERIFY(progress.contains(t1));
    QCOMPARE(progress.value(t1).done, 1);
    QCOMPARE(progress.value(t1).total, 2);

    QVERIFY(progress.contains(t2));
    QCOMPARE(progress.value(t2).done, 3);
    QCOMPARE(progress.value(t2).total, 3);

    QVERIFY(!progress.contains(t3));
    QCOMPARE(progress.value(t3).done, 0);
    QCOMPARE(progress.value(t3).total, 0);
}

QTEST_GUILESS_MAIN(tst_TopicRepository)
#include "tst_TopicRepository.moc"
