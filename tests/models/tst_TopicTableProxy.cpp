#include <memory>
#include <QSignalSpy>
#include <QTest>
#include "TestDatabase.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/Enums.h"
#include "domain/Structs.h"
#include "models/TopicListModel.h"
#include "models/TopicTableProxy.h"

using namespace Qt::StringLiterals;
using JobPrep::Data::TopicRepository;
using JobPrep::Data::TrackRepository;
using JobPrep::Domain::Priority;
using JobPrep::Domain::Topic;
using JobPrep::Domain::TopicStatus;
using JobPrep::Domain::Track;
using JobPrep::Models::TopicListModel;
using JobPrep::Models::TopicTableProxy;
using JobPrep::Tests::TestDatabase;

class tst_TopicTableProxy : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void trackFilterFiltersByTrack();
    void searchFilterFiltersTitleAndTags();
    void sortByTrackName();
    void sortByStatusEnumOrder();
    void sortByPriorityRank();
    void sortByTargetDateEmptyAlwaysLast();

private:
    int addTrack(const QString& name, const QString& color = u"#5B6CFF"_s);
    int addTopic(const QString& title, int trackId, TopicStatus status = TopicStatus::Backlog,
                 Priority priority = Priority::Normal,
                 std::optional<QDate> targetDate = std::nullopt,
                 const QString& tags = QString());

    std::unique_ptr<TestDatabase> m_db;
    std::unique_ptr<TrackRepository> m_tracks;
    std::unique_ptr<TopicRepository> m_topics;
    std::unique_ptr<TopicListModel> m_model;
    std::unique_ptr<TopicTableProxy> m_proxy;
};

void tst_TopicTableProxy::init() {
    m_db = std::make_unique<TestDatabase>(u"tst_topic_table_proxy"_s);
    QVERIFY(m_db->open(u":memory:"_s));
    m_tracks = std::make_unique<TrackRepository>(m_db->db());
    m_topics = std::make_unique<TopicRepository>(m_db->db());
    m_model = std::make_unique<TopicListModel>(*m_topics, *m_tracks);
    m_proxy = std::make_unique<TopicTableProxy>();
    m_proxy->setSourceModel(m_model.get());
    m_proxy->setSortRole(TopicListModel::SortRole);
}

void tst_TopicTableProxy::cleanup() {
    m_proxy.reset();
    m_model.reset();
    m_topics.reset();
    m_tracks.reset();
    m_db.reset();
}

int tst_TopicTableProxy::addTrack(const QString& name, const QString& color) {
    Track t;
    t.name = name;
    t.color = color;
    t.position = m_tracks->count();
    m_tracks->insert(t);
    return t.id;
}

int tst_TopicTableProxy::addTopic(const QString& title, int trackId, TopicStatus status,
                                  Priority priority, std::optional<QDate> targetDate,
                                  const QString& tags) {
    Topic t;
    t.title = title;
    t.trackId = trackId;
    t.status = status;
    t.priority = priority;
    t.targetDate = targetDate;
    t.tags = tags;
    t.position = m_topics->count();
    m_topics->insert(t);
    return t.id;
}

void tst_TopicTableProxy::trackFilterFiltersByTrack() {
    const int tr1 = addTrack(u"C++"_s);
    const int tr2 = addTrack(u"English"_s);
    addTopic(u"Templates"_s, tr1);
    addTopic(u"Smart Pointers"_s, tr1);
    addTopic(u"Pronunciation"_s, tr2);

    QCOMPARE(m_proxy->rowCount(), 3);

    m_proxy->setTrackFilter(tr1);
    QCOMPARE(m_proxy->rowCount(), 2);
    for (int r = 0; r < m_proxy->rowCount(); ++r) {
        QCOMPARE(m_proxy->index(r, 0).data(TopicListModel::TrackIdRole).toInt(), tr1);
    }

    m_proxy->setTrackFilter(tr2);
    QCOMPARE(m_proxy->rowCount(), 1);
    QCOMPARE(m_proxy->index(0, 0).data(TopicListModel::TrackIdRole).toInt(), tr2);

    m_proxy->setTrackFilter(std::nullopt);
    QCOMPARE(m_proxy->rowCount(), 3);
}

void tst_TopicTableProxy::searchFilterFiltersTitleAndTags() {
    const int tr = addTrack(u"Algorithms"_s);
    addTopic(u"Binary Search"_s, tr, TopicStatus::Backlog, Priority::Normal, std::nullopt, u"search, divide"_s);
    addTopic(u"QuickSort"_s, tr, TopicStatus::Backlog, Priority::Normal, std::nullopt, u"sorting"_s);
    addTopic(u"MergeSort"_s, tr, TopicStatus::Backlog, Priority::Normal, std::nullopt, u"sorting, recursion"_s);

    m_proxy->setSearchFilter(u"sort"_s);
    QCOMPARE(m_proxy->rowCount(), 2);

    m_proxy->setSearchFilter(u"divide"_s);
    QCOMPARE(m_proxy->rowCount(), 1);
    QCOMPARE(m_proxy->index(0, TopicListModel::TitleColumn).data().toString(), u"Binary Search"_s);

    m_proxy->setSearchFilter(u"   "_s);
    QCOMPARE(m_proxy->rowCount(), 3);
}

void tst_TopicTableProxy::sortByTrackName() {
    const int trB = addTrack(u"Beta Track"_s);
    const int trA = addTrack(u"Alpha Track"_s);
    addTopic(u"Topic 1"_s, trB);
    addTopic(u"Topic 2"_s, trA);

    m_proxy->sort(TopicListModel::TrackColumn, Qt::AscendingOrder);
    QCOMPARE(m_proxy->index(0, TopicListModel::TrackColumn).data().toString(), u"Alpha Track"_s);
    QCOMPARE(m_proxy->index(1, TopicListModel::TrackColumn).data().toString(), u"Beta Track"_s);

    m_proxy->sort(TopicListModel::TrackColumn, Qt::DescendingOrder);
    QCOMPARE(m_proxy->index(0, TopicListModel::TrackColumn).data().toString(), u"Beta Track"_s);
    QCOMPARE(m_proxy->index(1, TopicListModel::TrackColumn).data().toString(), u"Alpha Track"_s);
}

void tst_TopicTableProxy::sortByStatusEnumOrder() {
    const int tr = addTrack(u"Track"_s);
    addTopic(u"T Done"_s, tr, TopicStatus::Done);
    addTopic(u"T Backlog"_s, tr, TopicStatus::Backlog);
    addTopic(u"T Review"_s, tr, TopicStatus::Review);
    addTopic(u"T InProgress"_s, tr, TopicStatus::InProgress);

    m_proxy->sort(TopicListModel::StatusColumn, Qt::AscendingOrder);
    QCOMPARE(m_proxy->index(0, TopicListModel::TitleColumn).data().toString(), u"T Backlog"_s);
    QCOMPARE(m_proxy->index(1, TopicListModel::TitleColumn).data().toString(), u"T InProgress"_s);
    QCOMPARE(m_proxy->index(2, TopicListModel::TitleColumn).data().toString(), u"T Review"_s);
    QCOMPARE(m_proxy->index(3, TopicListModel::TitleColumn).data().toString(), u"T Done"_s);
}

void tst_TopicTableProxy::sortByPriorityRank() {
    const int tr = addTrack(u"Track"_s);
    addTopic(u"Normal Task"_s, tr, TopicStatus::Backlog, Priority::Normal);
    addTopic(u"Low Task"_s, tr, TopicStatus::Backlog, Priority::Low);
    addTopic(u"High Task"_s, tr, TopicStatus::Backlog, Priority::High);

    m_proxy->sort(TopicListModel::PriorityColumn, Qt::AscendingOrder);
    QCOMPARE(m_proxy->index(0, TopicListModel::TitleColumn).data().toString(), u"Low Task"_s);
    QCOMPARE(m_proxy->index(1, TopicListModel::TitleColumn).data().toString(), u"Normal Task"_s);
    QCOMPARE(m_proxy->index(2, TopicListModel::TitleColumn).data().toString(), u"High Task"_s);

    m_proxy->sort(TopicListModel::PriorityColumn, Qt::DescendingOrder);
    QCOMPARE(m_proxy->index(0, TopicListModel::TitleColumn).data().toString(), u"High Task"_s);
    QCOMPARE(m_proxy->index(1, TopicListModel::TitleColumn).data().toString(), u"Normal Task"_s);
    QCOMPARE(m_proxy->index(2, TopicListModel::TitleColumn).data().toString(), u"Low Task"_s);
}

void tst_TopicTableProxy::sortByTargetDateEmptyAlwaysLast() {
    const int tr = addTrack(u"Track"_s);
    addTopic(u"No Date A"_s, tr, TopicStatus::Backlog, Priority::Normal, std::nullopt);
    addTopic(u"Early Date"_s, tr, TopicStatus::Backlog, Priority::Normal, QDate(2026, 10, 10));
    addTopic(u"Late Date"_s, tr, TopicStatus::Backlog, Priority::Normal, QDate(2026, 10, 25));
    addTopic(u"No Date B"_s, tr, TopicStatus::Backlog, Priority::Normal, std::nullopt);

    // 1. Ascending order: Early Date -> Late Date -> (No Date A/B last)
    m_proxy->sort(TopicListModel::TargetDateColumn, Qt::AscendingOrder);
    QCOMPARE(m_proxy->index(0, TopicListModel::TitleColumn).data().toString(), u"Early Date"_s);
    QCOMPARE(m_proxy->index(1, TopicListModel::TitleColumn).data().toString(), u"Late Date"_s);
    QVERIFY(!m_proxy->index(2, 0).data(TopicListModel::TargetDateRole).isValid());
    QVERIFY(!m_proxy->index(3, 0).data(TopicListModel::TargetDateRole).isValid());

    // 2. Descending order: Late Date -> Early Date -> (No Date A/B still last!)
    m_proxy->sort(TopicListModel::TargetDateColumn, Qt::DescendingOrder);
    QCOMPARE(m_proxy->index(0, TopicListModel::TitleColumn).data().toString(), u"Late Date"_s);
    QCOMPARE(m_proxy->index(1, TopicListModel::TitleColumn).data().toString(), u"Early Date"_s);
    QVERIFY(!m_proxy->index(2, 0).data(TopicListModel::TargetDateRole).isValid());
    QVERIFY(!m_proxy->index(3, 0).data(TopicListModel::TargetDateRole).isValid());
}

QTEST_MAIN(tst_TopicTableProxy)
#include "tst_TopicTableProxy.moc"
