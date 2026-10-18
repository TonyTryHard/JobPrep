#include <memory>
#include <QDate>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>
#include "TestDatabase.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/Enums.h"
#include "domain/Structs.h"
#include "domain/TopicProgress.h"
#include "models/TopicListModel.h"
#include "services/SettingsService.h"

using namespace Qt::StringLiterals;
using JobPrep::Data::TopicRepository;
using JobPrep::Data::TrackRepository;
using JobPrep::Domain::Priority;
using JobPrep::Domain::Subtask;
using JobPrep::Domain::Topic;
using JobPrep::Domain::TopicStatus;
using JobPrep::Domain::Track;
using JobPrep::Models::TopicListModel;
using JobPrep::Services::SettingsService;
using JobPrep::Tests::TestDatabase;

class tst_TopicListModel : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // A1: Progress rule
    void topicProgressRule();

    // Model structure and roles (A2)
    void modelStructureAndRoles();

    // A7: Overdue calculation with injectable clock
    void overdueRoleWithInjectableClock();

    // A5: Reload emits dataChanged for data-only changes and modelReset for additions/deletions
    void reloadEmitsDataChangedForDataOnlyChanges();
    void reloadEmitsModelResetForStructuralChanges();

    // A8: Track rename and recolour update the model
    void trackChangesUpdateModel();

    // A9: Settings accessors and fallback
    void settingsKeysAndFallbacks();

private:
    int addTrack(const QString& name, const QString& color = u"#5B6CFF"_s);
    int addTopic(const QString& title, int trackId, TopicStatus status = TopicStatus::Backlog);

    std::unique_ptr<TestDatabase> m_db;
    std::unique_ptr<TrackRepository> m_tracks;
    std::unique_ptr<TopicRepository> m_topics;
};

void tst_TopicListModel::init() {
    m_db = std::make_unique<TestDatabase>(u"tst_topic_list_model"_s);
    QVERIFY(m_db->open(u":memory:"_s));
    m_tracks = std::make_unique<TrackRepository>(m_db->db());
    m_topics = std::make_unique<TopicRepository>(m_db->db());
}

void tst_TopicListModel::cleanup() {
    m_topics.reset();
    m_tracks.reset();
    m_db.reset();
}

int tst_TopicListModel::addTrack(const QString& name, const QString& color) {
    Track track;
    track.name = name;
    track.color = color;
    const bool inserted = m_tracks->insert(track);
    return inserted ? track.id : 0;
}

int tst_TopicListModel::addTopic(const QString& title, int trackId, TopicStatus status) {
    Topic topic;
    topic.trackId = trackId;
    topic.title = title;
    topic.status = status;
    const bool inserted = m_topics->insert(topic);
    return inserted ? topic.id : 0;
}

void tst_TopicListModel::topicProgressRule() {
    // Has subtasks: done / total
    QCOMPARE(JobPrep::Domain::topicProgress(TopicStatus::Backlog, 1, 2), 50);
    QCOMPARE(JobPrep::Domain::topicProgress(TopicStatus::InProgress, 1, 3), 33);
    QCOMPARE(JobPrep::Domain::topicProgress(TopicStatus::Review, 3, 3), 100);
    QCOMPARE(JobPrep::Domain::topicProgress(TopicStatus::Done, 2, 4), 50);

    // No subtasks: status fallback (backlog 0, in_progress 50, review 80, done 100)
    QCOMPARE(JobPrep::Domain::topicProgress(TopicStatus::Backlog, 0, 0), 0);
    QCOMPARE(JobPrep::Domain::topicProgress(TopicStatus::InProgress, 0, 0), 50);
    QCOMPARE(JobPrep::Domain::topicProgress(TopicStatus::Review, 0, 0), 80);
    QCOMPARE(JobPrep::Domain::topicProgress(TopicStatus::Done, 0, 0), 100);

    // Total 0 and negative are safe
    QCOMPARE(JobPrep::Domain::topicProgress(TopicStatus::Backlog, 5, 0), 0);
    QCOMPARE(JobPrep::Domain::topicProgress(TopicStatus::InProgress, 0, -1), 50);

    // Done > total is safe (clamped to 100)
    QCOMPARE(JobPrep::Domain::topicProgress(TopicStatus::InProgress, 5, 3), 100);
    // Done < 0 is safe (clamped to 0)
    QCOMPARE(JobPrep::Domain::topicProgress(TopicStatus::InProgress, -2, 3), 0);
}

void tst_TopicListModel::modelStructureAndRoles() {
    const int cpp = addTrack(u"C++"_s, u"#5B6CFF"_s);
    const int topicId = addTopic(u"Move Semantics"_s, cpp, TopicStatus::InProgress);

    Subtask s1; s1.topicId = topicId; s1.text = u"Item 1"_s; s1.done = true;
    m_topics->addSubtask(s1);
    Subtask s2; s2.topicId = topicId; s2.text = u"Item 2"_s; s2.done = false;
    m_topics->addSubtask(s2);

    TopicListModel model(*m_topics, *m_tracks);
    QCOMPARE(model.columnCount(), TopicListModel::ColumnCount);
    QCOMPARE(model.rowCount(), 1);

    // Column headers
    QCOMPARE(model.headerData(TopicListModel::TitleColumn, Qt::Horizontal).toString(), u"Title"_s);
    QCOMPARE(model.headerData(TopicListModel::TrackColumn, Qt::Horizontal).toString(), u"Track"_s);
    QCOMPARE(model.headerData(TopicListModel::StatusColumn, Qt::Horizontal).toString(), u"Status"_s);
    QCOMPARE(model.headerData(TopicListModel::PriorityColumn, Qt::Horizontal).toString(), u"Priority"_s);
    QCOMPARE(model.headerData(TopicListModel::TargetDateColumn, Qt::Horizontal).toString(), u"Target Date"_s);
    QCOMPARE(model.headerData(TopicListModel::ProgressColumn, Qt::Horizontal).toString(), u"Progress"_s);
    QCOMPARE(model.headerData(TopicListModel::TagsColumn, Qt::Horizontal).toString(), u"Tags"_s);

    const QModelIndex idx = model.index(0, TopicListModel::TitleColumn);

    // DisplayRole
    QCOMPARE(model.data(model.index(0, TopicListModel::TitleColumn), Qt::DisplayRole).toString(), u"Move Semantics"_s);
    QCOMPARE(model.data(model.index(0, TopicListModel::TrackColumn), Qt::DisplayRole).toString(), u"C++"_s);
    QCOMPARE(model.data(model.index(0, TopicListModel::ProgressColumn), Qt::DisplayRole).toString(), u"50%"_s);

    // Roles: Neutral types only (A2)
    QCOMPARE(idx.data(TopicListModel::IdRole).toInt(), topicId);
    QCOMPARE(idx.data(TopicListModel::StatusRole).value<TopicStatus>(), TopicStatus::InProgress);
    QCOMPARE(idx.data(TopicListModel::PriorityRole).value<Priority>(), Priority::Normal);
    QCOMPARE(idx.data(TopicListModel::ProgressRole).toInt(), 50);
    QCOMPARE(idx.data(TopicListModel::TrackIdRole).toInt(), cpp);

    // Track color must be a hex QString, NEVER a QColor (A2)
    const QVariant colorVar = idx.data(TopicListModel::TrackColorRole);
    QCOMPARE(colorVar.userType(), QMetaType::QString);
    QCOMPARE(colorVar.toString(), u"#5B6CFF"_s);

    QCOMPARE(idx.data(TopicListModel::TrackNameRole).toString(), u"C++"_s);
    QCOMPARE(idx.data(TopicListModel::DoneSubtasksRole).toInt(), 1);
    QCOMPARE(idx.data(TopicListModel::TotalSubtasksRole).toInt(), 2);

    // Helpers
    QVERIFY(model.topicAt(0).has_value());
    QCOMPARE(model.topicAt(0)->id, topicId);
    QVERIFY(model.topicById(topicId).has_value());
    QCOMPARE(model.rowForTopicId(topicId), 0);
    QCOMPARE(model.rowForTopicId(999), -1);
}

void tst_TopicListModel::overdueRoleWithInjectableClock() {
    const int cpp = addTrack(u"C++"_s);

    const QDate today(2026, 10, 15);

    // 1. Target date before today, not done -> Overdue = true
    Topic t1;
    t1.trackId = cpp;
    t1.title = u"Overdue topic"_s;
    t1.status = TopicStatus::InProgress;
    t1.targetDate = QDate(2026, 10, 14);
    QVERIFY(m_topics->insert(t1));

    // 2. Target date on today -> Overdue = false
    Topic t2;
    t2.trackId = cpp;
    t2.title = u"Today topic"_s;
    t2.status = TopicStatus::InProgress;
    t2.targetDate = QDate(2026, 10, 15);
    QVERIFY(m_topics->insert(t2));

    // 3. Target date after today -> Overdue = false
    Topic t3;
    t3.trackId = cpp;
    t3.title = u"Future topic"_s;
    t3.status = TopicStatus::InProgress;
    t3.targetDate = QDate(2026, 10, 16);
    QVERIFY(m_topics->insert(t3));

    // 4. Target date before today, but status is DONE -> NEVER overdue! (A7, SPEC §14)
    Topic t4;
    t4.trackId = cpp;
    t4.title = u"Done topic"_s;
    t4.status = TopicStatus::Done;
    t4.targetDate = QDate(2026, 10, 10);
    QVERIFY(m_topics->insert(t4));

    // 5. No target date -> Overdue = false
    Topic t5;
    t5.trackId = cpp;
    t5.title = u"Undated topic"_s;
    t5.status = TopicStatus::Backlog;
    QVERIFY(m_topics->insert(t5));

    TopicListModel model(*m_topics, *m_tracks, nullptr, [today] { return today; });

    const int r1 = model.rowForTopicId(t1.id);
    const int r2 = model.rowForTopicId(t2.id);
    const int r3 = model.rowForTopicId(t3.id);
    const int r4 = model.rowForTopicId(t4.id);
    const int r5 = model.rowForTopicId(t5.id);

    QVERIFY(model.index(r1, 0).data(TopicListModel::OverdueRole).toBool());
    QVERIFY(!model.index(r2, 0).data(TopicListModel::OverdueRole).toBool());
    QVERIFY(!model.index(r3, 0).data(TopicListModel::OverdueRole).toBool());
    QVERIFY(!model.index(r4, 0).data(TopicListModel::OverdueRole).toBool());
    QVERIFY(!model.index(r5, 0).data(TopicListModel::OverdueRole).toBool());
}

void tst_TopicListModel::reloadEmitsDataChangedForDataOnlyChanges() {
    const int cpp = addTrack(u"C++"_s);
    const int topicId = addTopic(u"Move Semantics"_s, cpp);

    Subtask s1; s1.topicId = topicId; s1.text = u"Item 1"_s; s1.done = false;
    m_topics->addSubtask(s1);

    TopicListModel model(*m_topics, *m_tracks);

    QSignalSpy dataChangedSpy(&model, &TopicListModel::dataChanged);
    QSignalSpy modelResetSpy(&model, &TopicListModel::modelReset);

    // Ticking a subtask changes progress data, but NOT the row set (A5)
    QVERIFY(m_topics->setSubtaskDone(s1.id, true));

    // Repository changed signal triggered reload()
    QVERIFY(dataChangedSpy.count() >= 1);
    QCOMPARE(modelResetSpy.count(), 0);  // Must NOT reset the model!

    const QModelIndex idx = model.index(0, TopicListModel::ProgressColumn);
    QCOMPARE(idx.data(TopicListModel::ProgressRole).toInt(), 100);

    // Updating topic title/notes also keeps the row set
    auto topic = *m_topics->byId(topicId);
    topic.title = u"Move Semantics & Rvalues"_s;
    QVERIFY(m_topics->update(topic));

    QVERIFY(dataChangedSpy.count() >= 2);
    QCOMPARE(modelResetSpy.count(), 0);
    QCOMPARE(model.index(0, TopicListModel::TitleColumn).data(Qt::DisplayRole).toString(),
             u"Move Semantics & Rvalues"_s);
}

void tst_TopicListModel::reloadEmitsModelResetForStructuralChanges() {
    const int cpp = addTrack(u"C++"_s);
    const int t1 = addTopic(u"Topic 1"_s, cpp);

    TopicListModel model(*m_topics, *m_tracks);

    QSignalSpy modelResetSpy(&model, &TopicListModel::modelReset);

    // Adding a topic changes the row set -> modelReset
    addTopic(u"Topic 2"_s, cpp);
    QCOMPARE(modelResetSpy.count(), 1);

    // Removing a topic changes the row set -> modelReset
    m_topics->remove(t1);
    QCOMPARE(modelResetSpy.count(), 2);
}

void tst_TopicListModel::trackChangesUpdateModel() {
    int cpp = addTrack(u"C++"_s, u"#5B6CFF"_s);
    addTopic(u"Move Semantics"_s, cpp);

    TopicListModel model(*m_topics, *m_tracks);
    const QModelIndex idx = model.index(0, TopicListModel::TrackColumn);
    QCOMPARE(idx.data(TopicListModel::TrackNameRole).toString(), u"C++"_s);
    QCOMPARE(idx.data(TopicListModel::TrackColorRole).toString(), u"#5B6CFF"_s);

    QSignalSpy dataChangedSpy(&model, &TopicListModel::dataChanged);
    QSignalSpy modelResetSpy(&model, &TopicListModel::modelReset);

    // Rename and recolor track (A8)
    auto track = *m_tracks->byId(cpp);
    track.name = u"Modern C++"_s;
    track.color = u"#0EA5E9"_s;
    QVERIFY(m_tracks->update(track));

    // Model reloaded on TrackRepository::changed
    QVERIFY(dataChangedSpy.count() >= 1);
    QCOMPARE(modelResetSpy.count(), 0);

    QCOMPARE(idx.data(TopicListModel::TrackNameRole).toString(), u"Modern C++"_s);
    QCOMPARE(idx.data(TopicListModel::TrackColorRole).toString(), u"#0EA5E9"_s);
}

void tst_TopicListModel::settingsKeysAndFallbacks() {
    auto clearStudySettings = []() {
        QSettings s;
        s.remove(u"study/viewMode"_s);
        s.remove(u"study/trackFilter"_s);
        s.remove(u"study/splitterState"_s);
    };

    clearStudySettings();

    SettingsService settings;

    // View mode: default board
    QCOMPARE(settings.studyViewMode(), u"board"_s);

    settings.setStudyViewMode(u"list"_s);
    QCOMPARE(settings.studyViewMode(), u"list"_s);

    // Unknown view mode returns "board" (A9)
    settings.setStudyViewMode(u"invalid_mode"_s);
    QCOMPARE(settings.studyViewMode(), u"board"_s);

    // Track filter: default -1 (All)
    QCOMPARE(settings.studyTrackFilter(), -1);

    settings.setStudyTrackFilter(42);
    QCOMPARE(settings.studyTrackFilter(), 42);  // Stored id returned as is (A9)

    // Splitter state
    const QByteArray dummyState("test_splitter_state");
    settings.setStudySplitterState(dummyState);
    QCOMPARE(settings.studySplitterState(), dummyState);

    clearStudySettings();
}

QTEST_GUILESS_MAIN(tst_TopicListModel)
#include "tst_TopicListModel.moc"
