#include <memory>
#include <QMimeData>
#include <QSignalSpy>
#include <QTest>
#include "TestDatabase.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/Enums.h"
#include "domain/Structs.h"
#include "models/TopicBoardProxy.h"
#include "models/TopicListModel.h"

using namespace Qt::StringLiterals;
using JobPrep::Data::TopicRepository;
using JobPrep::Data::TrackRepository;
using JobPrep::Domain::Topic;
using JobPrep::Domain::TopicStatus;
using JobPrep::Domain::Track;
using JobPrep::Models::TopicBoardProxy;
using JobPrep::Models::TopicListModel;
using JobPrep::Tests::TestDatabase;

class tst_TopicBoardProxy : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void statusFilterAcceptsMatchingColumnOnly();
    void trackFilterFiltersByTrack();
    void mimeDataRoundTrip();
    void canDropMimeDataValidation();
    void dropOnCardInsertsBeforeIt();
    void dropOnEmptyAreaAppends();
    void dropOnSamePlaceIsNoOp();
    void dropAcrossStatusMovesTopic();
    void removeRowsIsUnimplemented();

private:
    int addTrack(const QString& name);
    int addTopic(const QString& title, int trackId, TopicStatus status = TopicStatus::Backlog);

    std::unique_ptr<TestDatabase> m_db;
    std::unique_ptr<TrackRepository> m_tracks;
    std::unique_ptr<TopicRepository> m_topics;
};

void tst_TopicBoardProxy::init() {
    m_db = std::make_unique<TestDatabase>(u"tst_topic_board_proxy"_s);
    QVERIFY(m_db->open(u":memory:"_s));
    m_tracks = std::make_unique<TrackRepository>(m_db->db());
    m_topics = std::make_unique<TopicRepository>(m_db->db());
}

void tst_TopicBoardProxy::cleanup() {
    m_topics.reset();
    m_tracks.reset();
    m_db.reset();
}

int tst_TopicBoardProxy::addTrack(const QString& name) {
    Track track;
    track.name = name;
    track.color = u"#5B6CFF"_s;
    const bool inserted = m_tracks->insert(track);
    return inserted ? track.id : 0;
}

int tst_TopicBoardProxy::addTopic(const QString& title, int trackId, TopicStatus status) {
    Topic topic;
    topic.trackId = trackId;
    topic.title = title;
    topic.status = status;
    const bool inserted = m_topics->insert(topic);
    return inserted ? topic.id : 0;
}

void tst_TopicBoardProxy::statusFilterAcceptsMatchingColumnOnly() {
    const int cpp = addTrack(u"C++"_s);
    addTopic(u"Backlog Topic"_s, cpp, TopicStatus::Backlog);
    addTopic(u"In Progress Topic"_s, cpp, TopicStatus::InProgress);
    addTopic(u"Review Topic"_s, cpp, TopicStatus::Review);
    addTopic(u"Done Topic"_s, cpp, TopicStatus::Done);

    TopicListModel sourceModel(*m_topics, *m_tracks);

    TopicBoardProxy backlogProxy(TopicStatus::Backlog, *m_topics);
    backlogProxy.setSourceModel(&sourceModel);
    QCOMPARE(backlogProxy.rowCount(), 1);
    QCOMPARE(backlogProxy.index(0, 0).data(Qt::DisplayRole).toString(), u"Backlog Topic"_s);

    TopicBoardProxy inProgProxy(TopicStatus::InProgress, *m_topics);
    inProgProxy.setSourceModel(&sourceModel);
    QCOMPARE(inProgProxy.rowCount(), 1);
    QCOMPARE(inProgProxy.index(0, 0).data(Qt::DisplayRole).toString(), u"In Progress Topic"_s);

    TopicBoardProxy reviewProxy(TopicStatus::Review, *m_topics);
    reviewProxy.setSourceModel(&sourceModel);
    QCOMPARE(reviewProxy.rowCount(), 1);
    QCOMPARE(reviewProxy.index(0, 0).data(Qt::DisplayRole).toString(), u"Review Topic"_s);

    TopicBoardProxy doneProxy(TopicStatus::Done, *m_topics);
    doneProxy.setSourceModel(&sourceModel);
    QCOMPARE(doneProxy.rowCount(), 1);
    QCOMPARE(doneProxy.index(0, 0).data(Qt::DisplayRole).toString(), u"Done Topic"_s);
}

void tst_TopicBoardProxy::trackFilterFiltersByTrack() {
    const int cpp = addTrack(u"C++"_s);
    const int eng = addTrack(u"English"_s);

    addTopic(u"C++ Topic 1"_s, cpp, TopicStatus::Backlog);
    addTopic(u"English Topic"_s, eng, TopicStatus::Backlog);
    addTopic(u"C++ Topic 2"_s, cpp, TopicStatus::Backlog);

    TopicListModel sourceModel(*m_topics, *m_tracks);
    TopicBoardProxy proxy(TopicStatus::Backlog, *m_topics);
    proxy.setSourceModel(&sourceModel);

    // All tracks
    QCOMPARE(proxy.rowCount(), 3);

    // Filter by C++
    proxy.setTrackFilter(cpp);
    QCOMPARE(proxy.rowCount(), 2);
    QCOMPARE(proxy.index(0, 0).data(Qt::DisplayRole).toString(), u"C++ Topic 1"_s);
    QCOMPARE(proxy.index(1, 0).data(Qt::DisplayRole).toString(), u"C++ Topic 2"_s);

    // Filter by English
    proxy.setTrackFilter(eng);
    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.index(0, 0).data(Qt::DisplayRole).toString(), u"English Topic"_s);

    // Clear filter
    proxy.setTrackFilter(std::nullopt);
    QCOMPARE(proxy.rowCount(), 3);
}

void tst_TopicBoardProxy::mimeDataRoundTrip() {
    const int cpp = addTrack(u"C++"_s);
    const int t1 = addTopic(u"Topic 1"_s, cpp, TopicStatus::Backlog);

    TopicListModel sourceModel(*m_topics, *m_tracks);
    TopicBoardProxy proxy(TopicStatus::Backlog, *m_topics);
    proxy.setSourceModel(&sourceModel);

    QCOMPARE(proxy.mimeTypes(), QStringList{QString::fromLatin1(TopicBoardProxy::kTopicMimeType)});

    std::unique_ptr<QMimeData> mime(proxy.mimeData({proxy.index(0, 0)}));
    QVERIFY(mime != nullptr);
    QVERIFY(mime->hasFormat(QString::fromLatin1(TopicBoardProxy::kTopicMimeType)));
    QCOMPARE(mime->data(QString::fromLatin1(TopicBoardProxy::kTopicMimeType)).toInt(), t1);
}

void tst_TopicBoardProxy::canDropMimeDataValidation() {
    const int cpp = addTrack(u"C++"_s);
    const int t1 = addTopic(u"Topic 1"_s, cpp, TopicStatus::Backlog);

    TopicListModel sourceModel(*m_topics, *m_tracks);
    TopicBoardProxy proxy(TopicStatus::Backlog, *m_topics);
    proxy.setSourceModel(&sourceModel);

    // Valid mime data with existing topic
    QMimeData validMime;
    validMime.setData(QString::fromLatin1(TopicBoardProxy::kTopicMimeType), QByteArray::number(t1));
    QVERIFY(proxy.canDropMimeData(&validMime, Qt::MoveAction, -1, -1, QModelIndex()));

    // Invalid format
    QMimeData badFormatMime;
    badFormatMime.setText(u"some text"_s);
    QVERIFY(!proxy.canDropMimeData(&badFormatMime, Qt::MoveAction, -1, -1, QModelIndex()));

    // Unknown topic ID
    QMimeData unknownIdMime;
    unknownIdMime.setData(QString::fromLatin1(TopicBoardProxy::kTopicMimeType), QByteArray::number(999));
    QVERIFY(!proxy.canDropMimeData(&unknownIdMime, Qt::MoveAction, -1, -1, QModelIndex()));
}

void tst_TopicBoardProxy::dropOnCardInsertsBeforeIt() {
    const int cpp = addTrack(u"C++"_s);
    const int t1 = addTopic(u"Topic 1"_s, cpp, TopicStatus::Backlog);
    const int t2 = addTopic(u"Topic 2"_s, cpp, TopicStatus::Backlog);
    const int t3 = addTopic(u"Topic 3"_s, cpp, TopicStatus::Backlog);

    TopicListModel sourceModel(*m_topics, *m_tracks);
    TopicBoardProxy proxy(TopicStatus::Backlog, *m_topics);
    proxy.setSourceModel(&sourceModel);

    QCOMPARE(proxy.rowCount(), 3);
    // Drop t3 on card t1 (parent = index 0): must insert before t1 -> new order: {t3, t1, t2}
    QMimeData mime;
    mime.setData(QString::fromLatin1(TopicBoardProxy::kTopicMimeType), QByteArray::number(t3));

    QSignalSpy spy(m_topics.get(), &TopicRepository::changed);
    QVERIFY(proxy.dropMimeData(&mime, Qt::MoveAction, -1, -1, proxy.index(0, 0)));
    QCOMPARE(spy.count(), 1);

    QCOMPARE(proxy.index(0, 0).data(TopicListModel::IdRole).toInt(), t3);
    QCOMPARE(proxy.index(1, 0).data(TopicListModel::IdRole).toInt(), t1);
    QCOMPARE(proxy.index(2, 0).data(TopicListModel::IdRole).toInt(), t2);
}

void tst_TopicBoardProxy::dropOnEmptyAreaAppends() {
    const int cpp = addTrack(u"C++"_s);
    const int t1 = addTopic(u"Topic 1"_s, cpp, TopicStatus::Backlog);
    const int t2 = addTopic(u"Topic 2"_s, cpp, TopicStatus::Backlog);

    TopicListModel sourceModel(*m_topics, *m_tracks);
    TopicBoardProxy proxy(TopicStatus::Backlog, *m_topics);
    proxy.setSourceModel(&sourceModel);

    // Drop t1 with row == -1 and invalid parent: append to the end -> {t2, t1}
    QMimeData mime;
    mime.setData(QString::fromLatin1(TopicBoardProxy::kTopicMimeType), QByteArray::number(t1));

    QSignalSpy spy(m_topics.get(), &TopicRepository::changed);
    QVERIFY(proxy.dropMimeData(&mime, Qt::MoveAction, -1, -1, QModelIndex()));
    QCOMPARE(spy.count(), 1);

    QCOMPARE(proxy.index(0, 0).data(TopicListModel::IdRole).toInt(), t2);
    QCOMPARE(proxy.index(1, 0).data(TopicListModel::IdRole).toInt(), t1);
}

void tst_TopicBoardProxy::dropOnSamePlaceIsNoOp() {
    const int cpp = addTrack(u"C++"_s);
    const int t1 = addTopic(u"Topic 1"_s, cpp, TopicStatus::Backlog);
    addTopic(u"Topic 2"_s, cpp, TopicStatus::Backlog);

    TopicListModel sourceModel(*m_topics, *m_tracks);
    TopicBoardProxy proxy(TopicStatus::Backlog, *m_topics);
    proxy.setSourceModel(&sourceModel);

    // Drop t1 on t1 (parent = index 0): dropping on current position is a no-op
    QMimeData mime;
    mime.setData(QString::fromLatin1(TopicBoardProxy::kTopicMimeType), QByteArray::number(t1));

    QSignalSpy spy(m_topics.get(), &TopicRepository::changed);
    QVERIFY(proxy.dropMimeData(&mime, Qt::MoveAction, -1, -1, proxy.index(0, 0)));
    QCOMPARE(spy.count(), 0);  // No changed() emitted!
}

void tst_TopicBoardProxy::dropAcrossStatusMovesTopic() {
    const int cpp = addTrack(u"C++"_s);
    const int t1 = addTopic(u"Topic 1"_s, cpp, TopicStatus::Backlog);
    const int t2 = addTopic(u"Topic 2"_s, cpp, TopicStatus::InProgress);

    TopicListModel sourceModel(*m_topics, *m_tracks);
    TopicBoardProxy inProgProxy(TopicStatus::InProgress, *m_topics);
    inProgProxy.setSourceModel(&sourceModel);

    QCOMPARE(inProgProxy.rowCount(), 1);

    // Drop t1 (from Backlog) onto InProgress before t2
    QMimeData mime;
    mime.setData(QString::fromLatin1(TopicBoardProxy::kTopicMimeType), QByteArray::number(t1));

    QSignalSpy spy(m_topics.get(), &TopicRepository::changed);
    QVERIFY(inProgProxy.dropMimeData(&mime, Qt::MoveAction, -1, -1, inProgProxy.index(0, 0)));
    QCOMPARE(spy.count(), 1);

    QCOMPARE(inProgProxy.rowCount(), 2);
    QCOMPARE(inProgProxy.index(0, 0).data(TopicListModel::IdRole).toInt(), t1);
    QCOMPARE(inProgProxy.index(1, 0).data(TopicListModel::IdRole).toInt(), t2);

    QCOMPARE(m_topics->byId(t1)->status, TopicStatus::InProgress);
}

void tst_TopicBoardProxy::removeRowsIsUnimplemented() {
    TopicBoardProxy proxy(TopicStatus::Backlog, *m_topics);
    // removeRows must return false (finding 10)
    QVERIFY(!proxy.removeRows(0, 1));
}

QTEST_GUILESS_MAIN(tst_TopicBoardProxy)
#include "tst_TopicBoardProxy.moc"
