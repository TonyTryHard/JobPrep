#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <algorithm>
#include "TestDatabase.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "services/SeedService.h"

using namespace Qt::StringLiterals;
using JobPrep::Data::TopicRepository;
using JobPrep::Data::TrackRepository;
using JobPrep::Domain::Topic;
using JobPrep::Domain::Track;
using JobPrep::Domain::TopicStatus;
using JobPrep::Services::SeedService;
using JobPrep::Tests::TestDatabase;

class tst_SeedService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void loadsTracksTopicsAndSubtasks();
    void countsMatchSeedFile();
    void secondRunIsIdempotent();
    void existingTopicIsNotDuplicated();
    void invalidDatabaseFailsWithoutPartialData();

private:
    int topicCountForTrack(const QString& trackName);

    std::unique_ptr<TestDatabase> m_db;
    std::unique_ptr<TrackRepository> m_tracks;
    std::unique_ptr<TopicRepository> m_topics;
    std::unique_ptr<SeedService> m_seed;
};

void tst_SeedService::initTestCase() {
    Q_INIT_RESOURCE(resources);
}

void tst_SeedService::init() {
    m_db = std::make_unique<TestDatabase>(u"tst_seed"_s);
    QVERIFY(m_db->open(u":memory:"_s));
    m_tracks = std::make_unique<TrackRepository>(m_db->db());
    m_topics = std::make_unique<TopicRepository>(m_db->db());
    m_seed = std::make_unique<SeedService>(*m_tracks, *m_topics);
}

void tst_SeedService::cleanup() {
    m_seed.reset();
    m_topics.reset();
    m_tracks.reset();
    m_db.reset();
}

int tst_SeedService::topicCountForTrack(const QString& trackName) {
    const auto id = m_tracks->idByName(trackName);
    return id.has_value() ? m_topics->byTrack(*id).size() : 0;
}

void tst_SeedService::loadsTracksTopicsAndSubtasks() {
    QSignalSpy trackSpy(m_tracks.get(), &TrackRepository::changed);
    QSignalSpy topicSpy(m_topics.get(), &TopicRepository::changed);

    QString error;
    QVERIFY2(m_seed->loadSampleTopics(&error), qPrintable(error));
    QVERIFY(error.isEmpty());

    QCOMPARE(trackSpy.count(), 1);
    QCOMPARE(topicSpy.count(), 1);

    const auto tracks = m_tracks->all();
    QCOMPARE(tracks.size(), 2);
    QCOMPARE(tracks.at(0).name, u"C++"_s);
    QCOMPARE(tracks.at(0).color, u"#5B6CFF"_s);
    QCOMPARE(tracks.at(1).name, u"English"_s);
    QCOMPARE(tracks.at(1).color, u"#F59E0B"_s);

    QCOMPARE(m_topics->count(), 22);
    QCOMPARE(topicCountForTrack(u"C++"_s), 12);
    QCOMPARE(topicCountForTrack(u"English"_s), 10);

    for (const Topic& topic : m_topics->all()) {
        QVERIFY(topic.status == TopicStatus::Backlog);
        const int total = m_topics->subtaskCount(topic.id);
        QVERIFY(total >= 3);
        QVERIFY(total <= 5);
        QCOMPARE(m_topics->doneSubtaskCount(topic.id), 0);
    }
}

void tst_SeedService::countsMatchSeedFile() {
    QFile file(u":/seed/topics.json"_s);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    file.close();

    QString error;
    QVERIFY2(m_seed->loadSampleTopics(&error), qPrintable(error));

    const QJsonArray trackArray = root.value(u"tracks"_s).toArray();
    QCOMPARE(trackArray.size(), m_tracks->count());

    int expectedTopics = 0;
    int expectedSubtasks = 0;
    for (const QJsonValue& trackValue : trackArray) {
        const QJsonObject trackObject = trackValue.toObject();
        const QJsonArray topicArray = trackObject.value(u"topics"_s).toArray();
        expectedTopics += topicArray.size();

        const auto trackId = m_tracks->idByName(trackObject.value(u"name"_s).toString());
        QVERIFY(trackId.has_value());
        const auto stored = m_topics->byTrack(*trackId);
        QCOMPARE(stored.size(), topicArray.size());

        for (const QJsonValue& topicValue : topicArray) {
            const QJsonObject topicObject = topicValue.toObject();
            const auto subtaskArray = topicObject.value(u"subtasks"_s).toArray();
            expectedSubtasks += subtaskArray.size();

            const QString title = topicObject.value(u"title"_s).toString();
            const auto found = std::find_if(stored.cbegin(), stored.cend(),
                                            [&title](const Topic& topic) {
                                                return topic.title == title;
                                            });
            QVERIFY(found != stored.cend());
            QCOMPARE(found->tags, topicObject.value(u"tags"_s).toString());
            QCOMPARE(m_topics->subtasks(found->id).size(), subtaskArray.size());
            QCOMPARE(m_topics->subtasks(found->id).first().text,
                     subtaskArray.first().toString());
        }
    }

    QCOMPARE(m_topics->count(), expectedTopics);
    QCOMPARE(m_db->countRows(u"subtasks"_s), expectedSubtasks);
}

void tst_SeedService::secondRunIsIdempotent() {
    QString error;
    QVERIFY(m_seed->loadSampleTopics(&error));
    const int topics = m_topics->count();
    const int subtasks = m_db->countRows(u"subtasks"_s);

    QSignalSpy trackSpy(m_tracks.get(), &TrackRepository::changed);
    QSignalSpy topicSpy(m_topics.get(), &TopicRepository::changed);

    QVERIFY2(m_seed->loadSampleTopics(&error), qPrintable(error));

    QCOMPARE(m_tracks->count(), 2);
    QCOMPARE(m_topics->count(), topics);
    QCOMPARE(m_db->countRows(u"subtasks"_s), subtasks);
    QCOMPARE(trackSpy.count(), 0);
    QCOMPARE(topicSpy.count(), 0);
}

void tst_SeedService::existingTopicIsNotDuplicated() {
    Track cpp;
    cpp.name = u"C++"_s;
    cpp.color = u"#000000"_s;
    QVERIFY(m_tracks->insert(cpp));

    Topic existing;
    existing.trackId = cpp.id;
    existing.title = u"RAII & smart pointers"_s;
    existing.notes = u"my own notes"_s;
    QVERIFY(m_topics->insert(existing));

    QString error;
    QVERIFY2(m_seed->loadSampleTopics(&error), qPrintable(error));

    QCOMPARE(m_tracks->count(), 2);
    QCOMPARE(m_topics->count(), 22);
    // The existing track keeps its color and the existing topic keeps its notes.
    QCOMPARE(m_tracks->byId(cpp.id)->color, u"#000000"_s);
    QCOMPARE(m_topics->byId(existing.id)->notes, u"my own notes"_s);
    QCOMPARE(topicCountForTrack(u"English"_s), 10);
    QCOMPARE(topicCountForTrack(u"C++"_s), 12);
}

void tst_SeedService::invalidDatabaseFailsWithoutPartialData() {
    m_db->db().close();

    QString error;
    QVERIFY(!m_seed->loadSampleTopics(&error));
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(tst_SeedService)
#include "tst_SeedService.moc"
