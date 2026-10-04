#include <QSignalSpy>
#include <QTest>
#include "TestDatabase.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"

using namespace Qt::StringLiterals;
using JobPrep::Data::TopicRepository;
using JobPrep::Data::TrackRepository;
using JobPrep::Domain::Subtask;
using JobPrep::Domain::Topic;
using JobPrep::Domain::Track;
using JobPrep::Tests::TestDatabase;

class tst_TrackRepository : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void insertFillsIdAndRoundTrips();
    void findsByIdAndName();
    void updateStoresNewValues();
    void duplicateNameFailsSilently();
    void removeCascadesTopicsAndSubtasks();
    void changedEmittedOncePerWrite();
    void unknownIdReturnsNullopt();
    void countReflectsRows();

private:
    std::unique_ptr<TestDatabase> m_db;
    std::unique_ptr<TrackRepository> m_tracks;
    std::unique_ptr<TopicRepository> m_topics;
};

void tst_TrackRepository::init() {
    m_db = std::make_unique<TestDatabase>(u"tst_tracks"_s);
    QVERIFY(m_db->open(u":memory:"_s));
    m_tracks = std::make_unique<TrackRepository>(m_db->db());
    m_topics = std::make_unique<TopicRepository>(m_db->db());
}

void tst_TrackRepository::cleanup() {
    m_topics.reset();
    m_tracks.reset();
    m_db.reset();
}

void tst_TrackRepository::insertFillsIdAndRoundTrips() {
    Track track;
    track.name = u"C++"_s;
    track.color = u"#5B6CFF"_s;
    track.icon = u"study"_s;
    track.position = 3;

    QVERIFY(m_tracks->insert(track));
    QVERIFY(track.id > 0);

    const auto stored = m_tracks->byId(track.id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->name, u"C++"_s);
    QCOMPARE(stored->color, u"#5B6CFF"_s);
    QCOMPARE(stored->icon, u"study"_s);
    QCOMPARE(stored->position, 3);
}

void tst_TrackRepository::findsByIdAndName() {
    Track cpp;
    cpp.name = u"C++"_s;
    cpp.color = u"#5B6CFF"_s;
    Track english;
    english.name = u"English"_s;
    english.color = u"#F59E0B"_s;
    QVERIFY(m_tracks->insert(cpp));
    QVERIFY(m_tracks->insert(english));

    QCOMPARE(m_tracks->all().size(), 2);
    const auto id = m_tracks->idByName(u"English"_s);
    QVERIFY(id.has_value());
    QCOMPARE(*id, english.id);
    QVERIFY(!m_tracks->idByName(u"Missing"_s).has_value());
}

void tst_TrackRepository::updateStoresNewValues() {
    Track track;
    track.name = u"C++"_s;
    track.color = u"#5B6CFF"_s;
    QVERIFY(m_tracks->insert(track));

    track.name = u"C++ and Qt"_s;
    track.color = u"#14B8A6"_s;
    track.position = 1;
    QVERIFY(m_tracks->update(track));

    const auto stored = m_tracks->byId(track.id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->name, u"C++ and Qt"_s);
    QCOMPARE(stored->color, u"#14B8A6"_s);
    QCOMPARE(stored->position, 1);

    Track unknown;
    unknown.id = 4242;
    QVERIFY(!m_tracks->update(unknown));
    QVERIFY(!m_tracks->lastError().isEmpty());
}

void tst_TrackRepository::duplicateNameFailsSilently() {
    Track first;
    first.name = u"English"_s;
    first.color = u"#F59E0B"_s;
    QVERIFY(m_tracks->insert(first));

    QSignalSpy spy(m_tracks.get(), &TrackRepository::changed);
    Track duplicate;
    duplicate.name = u"English"_s;
    duplicate.color = u"#123456"_s;
    QVERIFY(!m_tracks->insert(duplicate));
    QVERIFY(!m_tracks->lastError().isEmpty());
    QCOMPARE(duplicate.id, 0);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(m_tracks->count(), 1);
}

void tst_TrackRepository::removeCascadesTopicsAndSubtasks() {
    Track track;
    track.name = u"C++"_s;
    track.color = u"#5B6CFF"_s;
    QVERIFY(m_tracks->insert(track));

    Topic topic;
    topic.trackId = track.id;
    topic.title = u"RAII"_s;
    QVERIFY(m_topics->insert(topic));
    Subtask subtask;
    subtask.topicId = topic.id;
    subtask.text = u"Read cppreference"_s;
    QVERIFY(m_topics->addSubtask(subtask));

    QVERIFY(m_tracks->remove(track.id));

    QVERIFY(m_topics->isEmpty());
    QCOMPARE(m_topics->subtaskCount(topic.id), 0);
    QVERIFY(m_db->countRows(u"subtasks"_s) == 0);
    QVERIFY(!m_tracks->remove(track.id));
}

void tst_TrackRepository::changedEmittedOncePerWrite() {
    QSignalSpy spy(m_tracks.get(), &TrackRepository::changed);

    Track track;
    track.name = u"C++"_s;
    track.color = u"#5B6CFF"_s;
    QVERIFY(m_tracks->insert(track));
    QCOMPARE(spy.count(), 1);

    QVERIFY(m_tracks->update(track));
    QCOMPARE(spy.count(), 2);

    QVERIFY(m_tracks->remove(track.id));
    QCOMPARE(spy.count(), 3);
}

void tst_TrackRepository::unknownIdReturnsNullopt() {
    QVERIFY(!m_tracks->byId(999).has_value());
    QCOMPARE(m_tracks->lastError(), QString());
    QVERIFY(m_tracks->isEmpty());
    QCOMPARE(m_tracks->count(), 0);
}

void tst_TrackRepository::countReflectsRows() {
    QVERIFY(m_tracks->isEmpty());
    Track track;
    track.name = u"C++"_s;
    track.color = u"#5B6CFF"_s;
    QVERIFY(m_tracks->insert(track));
    QCOMPARE(m_tracks->count(), 1);
    QVERIFY(!m_tracks->isEmpty());
}

QTEST_GUILESS_MAIN(tst_TrackRepository)
#include "tst_TrackRepository.moc"
