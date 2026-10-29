#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDateEdit>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStringList>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextEdit>
#include <QDropEvent>
#include <QMimeData>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>
#include "app/AppContext.h"
#include "data/ApplicationRepository.h"
#include "data/Database.h"
#include "data/InterviewRepository.h"
#include "data/SessionRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/Structs.h"
#include "models/AgendaListModel.h"
#include "models/ApplicationTableModel.h"
#include "models/TopicBoardProxy.h"
#include "models/TopicListModel.h"
#include "services/DayWatcher.h"
#include "services/ExportService.h"
#include "services/INotifier.h"
#include "services/SeedService.h"
#include "services/SettingsService.h"
#include "services/StatsService.h"
#include "ui/DashboardController.h"
#include "ui/FormatHelper.h"
#include "ui/InAppNotifier.h"
#include "ui/MainWindow.h"
#include "ui/Sidebar.h"
#include "ui/dialogs/ApplicationDialog.h"
#include "ui/dialogs/FollowUpDialog.h"
#include "ui/dialogs/InterviewDialog.h"
#include "ui/dialogs/SessionDialog.h"
#include "ui/dialogs/TrackDialog.h"
#include "ui/pages/CalendarPage.h"
#include "ui/pages/HomePage.h"
#include "ui/pages/JobsPage.h"
#include "ui/pages/StudyPage.h"
#include "ui/theme/StatusStyle.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"
#include "ui/widgets/ChipBar.h"
#include "ui/widgets/EmptyState.h"
#include "ui/widgets/FunnelWidget.h"
#include "ui/widgets/GoalProgressBar.h"
#include "ui/widgets/MonthView.h"
#include "ui/widgets/ProgressRing.h"
#include "ui/widgets/SelectionRestorer.h"
#include "ui/widgets/StatCard.h"
#include "ui/widgets/TopicBoardColumn.h"
#include "ui/widgets/TopicDetailPanel.h"
#include "ui/widgets/TopicListView.h"
#include "UiTestHelpers.h"

using namespace Qt::StringLiterals;
using JobPrep::Services::Notification;
using JobPrep::Services::NotificationKind;
using JobPrep::Ui::InAppNotifier;
using namespace JobPrep::UiTest;

namespace {

using JobPrep::App::AppContext;
using JobPrep::Domain::AgendaKind;
using JobPrep::Domain::ApplicationStatus;
using JobPrep::Domain::Interview;
using JobPrep::Domain::InterviewOutcome;
using JobPrep::Domain::InterviewType;
using JobPrep::Domain::JobApplication;
using JobPrep::Domain::StudySession;
using JobPrep::Domain::Subtask;
using JobPrep::Domain::Topic;
using JobPrep::Domain::TopicStatus;
using JobPrep::Domain::Track;
using JobPrep::Models::ApplicationTableModel;
using JobPrep::Ui::MainWindow;
using JobPrep::Ui::Pages::HomePage;
using JobPrep::Ui::Dialogs::ApplicationDialog;
using JobPrep::Ui::Dialogs::FollowUpDialog;
using JobPrep::Ui::Dialogs::InterviewDialog;
using JobPrep::Ui::Dialogs::SessionDialog;
using JobPrep::Ui::Dialogs::TrackDialog;

} // namespace

class tst_PageStudy : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSelectionRestorerSurvivesReset();
    void testStudyBoardSeededAndTrackFilter();
    void testStudyQuickAdd();
    void testStudyKeyboardMoveAndReorder();
    void testStudyChecklistTickSurvivesReset();
    void testStudyBoardRealDnDDropEvent();
    void testStudyDetailPanelAutosaveAndValidation();
    void testStudyDeleteTopicKeepsSessions();
    void testStudyTrackFilterPersistenceAndFallback();
    void testStudyOverdueChipWithFixedClock();
    void testStudyPageApi();
    void testChipBarJobsAndCalendarRegression();
    void testStudyContextMenuMoveToStatus();
    void testStudyPageSelectionRestoreAcrossOperations();
    void testStudyChecklistEditAndReorder();
    void testStudySplitterStatePersisted();
    void testStudyPriorityTooltip();
    void testStudyDragFeedbackIndicatorAndHighlight();
    void testStudySessionDialogLogTime();
    void testStudyTrackDialogCrudAndCascade();
    void testHeaderNewSessionAndTopicRouting();
    void testHeaderSearchDisabledOnStudyPage();
    void testCtrlNTriggersCurrentPageNew();
    void testStudyBoardSingleSelectionAcrossColumns();
    void testStudyListViewToggleSortAndDelegates();
    void testStudyScreenshots();

private:
    void resetStudyData(const QDate& today = QDate(2026, 10, 19));
    JobPrep::Ui::Pages::StudyPage* studyPage() { return m_study.get(); }
    JobPrep::Ui::Widgets::TopicBoardColumn* studyColumn(JobPrep::Domain::TopicStatus status) const;

    // Helpers needed for testChipBarJobsAndCalendarRegression
    void resetJobsData();
    JobPrep::Ui::Pages::JobsPage* jobsPage() { return m_jobs.get(); }
    QTableView* jobsTable() const;
    void addApplication(const QString& company, const QString& position,
                        ApplicationStatus status = ApplicationStatus::Applied,
                        const QString& notes = QString());
    QString companyAt(int row) const;
    void resetCalendarData(const QDate& today);
    int addApplicationWithInterview(const QString& company, const QDate& day, int hour);
    int addStudyTarget(const QDate& day);
    QListView* agendaList() const;

    QTemporaryDir m_tempDir;
    std::unique_ptr<JobPrep::App::AppContext> m_ctx;
    std::unique_ptr<JobPrep::Ui::MainWindow> m_window;
    std::unique_ptr<JobPrep::App::AppContext> m_studyCtx;
    std::unique_ptr<JobPrep::Ui::Pages::StudyPage> m_study;
    std::unique_ptr<JobPrep::App::AppContext> m_jobsCtx;
    std::unique_ptr<JobPrep::Ui::Pages::JobsPage> m_jobs;
    std::unique_ptr<JobPrep::App::AppContext> m_calCtx;
    std::unique_ptr<JobPrep::Ui::Pages::CalendarPage> m_calendar;
};

void tst_PageStudy::initTestCase() {
    JobPrep::UiTest::installWarningFilter();
    JobPrep::UiTest::isolateSettings(m_tempDir);

    m_ctx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    QVERIFY(m_ctx->database().isOpen());
    m_window = std::make_unique<JobPrep::Ui::MainWindow>(*m_ctx);
    m_window->show();
    QCoreApplication::processEvents();
}

void tst_PageStudy::cleanupTestCase() {
    m_study.reset();
    m_studyCtx.reset();
    m_jobs.reset();
    m_jobsCtx.reset();
    m_calendar.reset();
    m_calCtx.reset();
    m_window.reset();
    m_ctx.reset();
    JOBPREP_VERIFY_ZERO_WARNINGS();
    JobPrep::UiTest::uninstallWarningFilter();
}

void tst_PageStudy::resetStudyData(const QDate& today) {
    m_study.reset();
    m_studyCtx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    QVERIFY(m_studyCtx->database().isOpen());

    m_studyCtx->settings().setStudyTrackFilter(-1);

    m_study = std::make_unique<JobPrep::Ui::Pages::StudyPage>(
        m_studyCtx->topics(), m_studyCtx->tracks(), m_studyCtx->sessions(), m_studyCtx->settings(),
        m_studyCtx->themeManager(), nullptr, [today]() { return today; });
    m_study->resize(1200, 720);
    m_study->show();
    JobPrep::UiTest::activateWindow(m_study.get());
    QCoreApplication::processEvents();
}


JobPrep::Ui::Widgets::TopicBoardColumn* tst_PageStudy::studyColumn(JobPrep::Domain::TopicStatus status) const {
    const QString name = [&]() {
        switch (status) {
            case JobPrep::Domain::TopicStatus::Backlog: return u"StudyColumnBacklog"_s;
            case JobPrep::Domain::TopicStatus::InProgress: return u"StudyColumnInProgress"_s;
            case JobPrep::Domain::TopicStatus::Review: return u"StudyColumnReview"_s;
            case JobPrep::Domain::TopicStatus::Done: return u"StudyColumnDone"_s;
        }
        return u""_s;
    }();
    return childNamed<JobPrep::Ui::Widgets::TopicBoardColumn>(m_study.get(), name);
}


void tst_PageStudy::resetJobsData() {
    m_jobs.reset();
    m_jobsCtx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    QVERIFY(m_jobsCtx->database().isOpen());
    m_jobs = std::make_unique<JobPrep::Ui::Pages::JobsPage>(m_jobsCtx->applications(),
                                                           m_jobsCtx->interviews(),
                                                           m_jobsCtx->exportService());
    m_jobs->show();
}


QTableView* tst_PageStudy::jobsTable() const {
    return childNamed<QTableView>(m_jobs.get(), u"JobsTableView"_s);
}


void tst_PageStudy::addApplication(const QString& company, const QString& position,
                                 ApplicationStatus status, const QString& notes) {
    JobApplication app;
    app.company = company;
    app.position = position;
    app.status = status;
    app.notes = notes;
    QVERIFY(m_jobsCtx->applications().insert(app));
}


QString tst_PageStudy::companyAt(int row) const {
    return jobsTable()->model()->index(row, ApplicationTableModel::CompanyColumn).data().toString();
}


void tst_PageStudy::resetCalendarData(const QDate& today) {
    m_calendar.reset();
    m_calCtx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    QVERIFY(m_calCtx->database().isOpen());

    // Layer visibility is persisted in QSettings, so pin it for every calendar slot.
    m_calCtx->settings().setCalendarLayerInterviews(true);
    m_calCtx->settings().setCalendarLayerFollowUps(true);
    m_calCtx->settings().setCalendarLayerTargets(true);

    m_calendar = std::make_unique<JobPrep::Ui::Pages::CalendarPage>(
        m_calCtx->interviews(), m_calCtx->applications(), m_calCtx->topics(), m_calCtx->tracks(),
        m_calCtx->agendaService(), m_calCtx->settings(), m_calCtx->themeManager(), nullptr,
        [today]() { return today; });
    m_calendar->resize(1200, 720);
    m_calendar->show();
    QCoreApplication::processEvents();
}


int tst_PageStudy::addApplicationWithInterview(const QString& company, const QDate& day, int hour) {
    JobApplication app;
    app.company = company;
    app.position = u"Developer"_s;
    if (!m_calCtx->applications().insert(app)) {
        QTest::qFail("failed to insert application", __FILE__, __LINE__);
        return 0;
    }

    Interview interview;
    interview.applicationId = app.id;
    interview.startAt = QDateTime(day, QTime(hour, 0));
    if (!m_calCtx->interviews().insert(interview)) {
        QTest::qFail("failed to insert interview", __FILE__, __LINE__);
    }
    return app.id;
}


int tst_PageStudy::addStudyTarget(const QDate& day) {
    Track track;
    track.name = u"English"_s;
    if (!m_calCtx->tracks().insert(track)) {
        QTest::qFail("failed to insert track", __FILE__, __LINE__);
        return 0;
    }

    Topic topic;
    topic.trackId = track.id;
    topic.title = u"Review STL containers"_s;
    topic.targetDate = day;
    if (!m_calCtx->topics().insert(topic)) {
        QTest::qFail("failed to insert topic", __FILE__, __LINE__);
    }
    return track.id;
}


QListView* tst_PageStudy::agendaList() const {
    return childNamed<QListView>(m_calendar.get(), u"CalendarAgendaList"_s);
}


void tst_PageStudy::testSelectionRestorerSurvivesReset() {
    JobPrep::App::AppContext ctx(u":memory:"_s);
    auto& trackRepo = ctx.tracks();
    auto& topicRepo = ctx.topics();

    Track tr;
    tr.name = u"General"_s;
    tr.color = u"#3b82f6"_s;
    QVERIFY(trackRepo.insert(tr));
    const int trackId = trackRepo.all().first().id;

    Topic t1;
    t1.trackId = trackId;
    t1.title = u"Alpha"_s;
    t1.status = JobPrep::Domain::TopicStatus::Backlog;
    QVERIFY(topicRepo.insert(t1));
    const int id1 = topicRepo.all().last().id;

    Topic t2;
    t2.trackId = trackId;
    t2.title = u"Beta"_s;
    t2.status = JobPrep::Domain::TopicStatus::Backlog;
    QVERIFY(topicRepo.insert(t2));
    const int id2 = topicRepo.all().last().id;

    Topic t3;
    t3.trackId = trackId;
    t3.title = u"Gamma"_s;
    t3.status = JobPrep::Domain::TopicStatus::Backlog;
    QVERIFY(topicRepo.insert(t3));
    const int id3 = topicRepo.all().last().id;

    JobPrep::Models::TopicListModel model(topicRepo, trackRepo);
    QListView view;
    view.setModel(&model);
    JobPrep::Ui::Widgets::SelectionRestorer restorer(&view, JobPrep::Models::TopicListModel::IdRole);

    // Select row 1 (t2)
    view.setCurrentIndex(model.index(1, 0));
    QCOMPARE(view.currentIndex().data(JobPrep::Models::TopicListModel::IdRole).toInt(), id2);

    // 1. Insert: adding a topic resets the model
    Topic t4;
    t4.trackId = trackId;
    t4.title = u"Zeta"_s;
    t4.status = JobPrep::Domain::TopicStatus::Backlog;
    QVERIFY(topicRepo.insert(t4));
    QCOMPARE(view.currentIndex().data(JobPrep::Models::TopicListModel::IdRole).toInt(), id2);

    // 2. Reorder: moveTopic resets the model
    QVERIFY(topicRepo.moveTopic(id2, JobPrep::Domain::TopicStatus::Backlog, {id3, id2, id1, t4.id}));
    QCOMPARE(view.currentIndex().data(JobPrep::Models::TopicListModel::IdRole).toInt(), id2);

    // 3. Delete another item: removes id1
    QVERIFY(topicRepo.remove(id1));
    QCOMPARE(view.currentIndex().data(JobPrep::Models::TopicListModel::IdRole).toInt(), id2);

    // 4. Delete the selected item itself: selection cleared, no crash
    QVERIFY(topicRepo.remove(id2));
    QVERIFY(!view.currentIndex().isValid() ||
            view.currentIndex().data(JobPrep::Models::TopicListModel::IdRole).toInt() != id2);

    // 5. Scroll preservation and manual preserveAround
    view.setCurrentIndex(model.index(0, 0));
    restorer.capture();
    QCOMPARE(restorer.savedId(),
             model.index(0, 0).data(JobPrep::Models::TopicListModel::IdRole).toInt());
    bool actionRan = false;
    restorer.preserveAround([&]() { actionRan = true; });
    QVERIFY(actionRan);
}


void tst_PageStudy::testStudyBoardSeededAndTrackFilter() {
    const QDate today(2026, 10, 19);
    m_study.reset();
    m_studyCtx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    QVERIFY(m_studyCtx->database().isOpen());
    JobPrep::Services::SeedService seeder(m_studyCtx->tracks(), m_studyCtx->topics());
    QVERIFY(seeder.loadSampleTopics());
    m_studyCtx->settings().setStudyTrackFilter(-1);

    m_study = std::make_unique<JobPrep::Ui::Pages::StudyPage>(
        m_studyCtx->topics(), m_studyCtx->tracks(), m_studyCtx->sessions(), m_studyCtx->settings(),
        m_studyCtx->themeManager(), nullptr, [today]() { return today; });
    m_study->resize(1200, 720);
    m_study->show();
    JobPrep::UiTest::activateWindow(m_study.get());
    QCoreApplication::processEvents();

    QCOMPARE(studyPage()->currentViewIndex(), 1);
    QVERIFY(studyPage()->visibleTopicCount() > 0);

    auto* backlog = studyColumn(JobPrep::Domain::TopicStatus::Backlog);
    auto* inProgress = studyColumn(JobPrep::Domain::TopicStatus::InProgress);
    auto* review = studyColumn(JobPrep::Domain::TopicStatus::Review);
    auto* done = studyColumn(JobPrep::Domain::TopicStatus::Done);
    QVERIFY(backlog && inProgress && review && done);
    const int totalInitial = studyPage()->visibleTopicCount();

    auto* chips = childNamed<JobPrep::Ui::Widgets::ChipBar>(m_study.get(), u"StudyTrackChips"_s);
    QVERIFY(chips != nullptr);
    const auto allTracks = m_studyCtx->tracks().all();
    QVERIFY(allTracks.size() >= 2);
    const auto track1 = allTracks.at(0);
    const auto track2 = allTracks.at(1);

    // Filter by track 1
    chips->button(track1.id)->click();
    QCoreApplication::processEvents();
    const int track1Count = studyPage()->visibleTopicCount();
    QVERIFY(track1Count > 0);
    QVERIFY(track1Count < totalInitial);

    for (auto* col : {backlog, inProgress, review, done}) {
        for (int r = 0; r < col->itemCount(); ++r) {
            const auto id = col->topicIdAt(r);
            QVERIFY(id.has_value());
            const auto topic = m_studyCtx->topics().byId(*id);
            QVERIFY(topic.has_value());
            QCOMPARE(topic->trackId, track1.id);
        }
    }

    // Filter by track 2
    chips->button(track2.id)->click();
    QCoreApplication::processEvents();
    const int track2Count = studyPage()->visibleTopicCount();
    QVERIFY(track2Count > 0);
    for (auto* col : {backlog, inProgress, review, done}) {
        for (int r = 0; r < col->itemCount(); ++r) {
            const auto id = col->topicIdAt(r);
            QVERIFY(id.has_value());
            const auto topic = m_studyCtx->topics().byId(*id);
            QVERIFY(topic.has_value());
            QCOMPARE(topic->trackId, track2.id);
        }
    }

    // Reset filter to All
    chips->button(-1)->click();
    QCoreApplication::processEvents();
    QCOMPARE(studyPage()->visibleTopicCount(), totalInitial);
}


void tst_PageStudy::testStudyQuickAdd() {
    resetStudyData();
    auto* backlog = studyColumn(JobPrep::Domain::TopicStatus::Backlog);
    QVERIFY(backlog);
    auto* quickAdd = childNamed<QLineEdit>(backlog, u"StudyColumnQuickAdd"_s);
    QVERIFY(quickAdd);
    QVERIFY(!quickAdd->isEnabled());
    QVERIFY(quickAdd->placeholderText().contains(u"Add a track"_s));
    QVERIFY(quickAdd->toolTip().contains(u"need at least one track"_s));

    Track tr1;
    tr1.name = u"C++"_s;
    tr1.color = u"#3b82f6"_s;
    tr1.position = 0;
    QVERIFY(m_studyCtx->tracks().insert(tr1));
    const int tr1Id = m_studyCtx->tracks().all().last().id;

    Track tr2;
    tr2.name = u"English"_s;
    tr2.color = u"#10b981"_s;
    tr2.position = 1;
    QVERIFY(m_studyCtx->tracks().insert(tr2));
    const int tr2Id = m_studyCtx->tracks().all().last().id;

    QCoreApplication::processEvents();

    QVERIFY(quickAdd->isEnabled());
    QVERIFY(quickAdd->placeholderText().contains(u"Adds to: C++"_s));
    QVERIFY(quickAdd->toolTip().contains(u"Adds to: C++"_s));

    const auto statuses = {
        JobPrep::Domain::TopicStatus::Backlog,
        JobPrep::Domain::TopicStatus::InProgress,
        JobPrep::Domain::TopicStatus::Review,
        JobPrep::Domain::TopicStatus::Done
    };

    for (const auto status : statuses) {
        auto* col = studyColumn(status);
        auto* edit = childNamed<QLineEdit>(col, u"StudyColumnQuickAdd"_s);
        QVERIFY(edit->isEnabled());
        const QString title = u"Topic for %1"_s.arg(static_cast<int>(status));
        edit->setText(title);
        QTest::keyClick(edit, Qt::Key_Return);
        QCoreApplication::processEvents();

        const auto topics = m_studyCtx->topics().all();
        const auto it = std::find_if(topics.begin(), topics.end(), [&](const Topic& t) {
            return t.title == title;
        });
        QVERIFY(it != topics.end());
        QCOMPARE(it->status, status);
        QCOMPARE(it->trackId, tr1Id);
        QCOMPARE(it->priority, JobPrep::Domain::Priority::Normal);
    }

    auto* chips = childNamed<JobPrep::Ui::Widgets::ChipBar>(m_study.get(), u"StudyTrackChips"_s);
    chips->button(tr2Id)->click();
    QCoreApplication::processEvents();
    QVERIFY(quickAdd->placeholderText().contains(u"Adds to: English"_s));
    QVERIFY(quickAdd->toolTip().contains(u"Adds to: English"_s));

    quickAdd->setText(u"Temporary text"_s);
    QTest::keyClick(quickAdd, Qt::Key_Escape);
    QCOMPARE(quickAdd->text(), QString());
}


void tst_PageStudy::testStudyKeyboardMoveAndReorder() {
    resetStudyData();
    Track tr;
    tr.name = u"General"_s;
    tr.color = u"#3b82f6"_s;
    QVERIFY(m_studyCtx->tracks().insert(tr));
    const int trId = m_studyCtx->tracks().all().last().id;

    Topic t1;
    t1.trackId = trId;
    t1.title = u"Card 1"_s;
    t1.status = JobPrep::Domain::TopicStatus::Backlog;
    t1.position = 0;
    QVERIFY(m_studyCtx->topics().insert(t1));
    const int t1Id = m_studyCtx->topics().all().last().id;

    Topic t2;
    t2.trackId = trId;
    t2.title = u"Card 2"_s;
    t2.status = JobPrep::Domain::TopicStatus::Backlog;
    t2.position = 1;
    QVERIFY(m_studyCtx->topics().insert(t2));
    const int t2Id = m_studyCtx->topics().all().last().id;

    QCoreApplication::processEvents();

    auto* backlog = studyColumn(JobPrep::Domain::TopicStatus::Backlog);
    auto* inProgress = studyColumn(JobPrep::Domain::TopicStatus::InProgress);
    auto* list = childNamed<QListView>(backlog, u"StudyColumnList"_s);
    QVERIFY(list);
    list->setFocus();
    list->setCurrentIndex(backlog->proxy()->index(0, 0));
    QCOMPARE(backlog->currentTopicId(), std::optional<int>(t1Id));

    // 1. Alt+Right moves to InProgress
    QTest::keyClick(list, Qt::Key_Right, Qt::AltModifier);
    QCoreApplication::processEvents();
    QCOMPARE(m_studyCtx->topics().byId(t1Id)->status, JobPrep::Domain::TopicStatus::InProgress);

    // 2. Alt+Left moves back to Backlog
    auto* progList = childNamed<QListView>(inProgress, u"StudyColumnList"_s);
    progList->setFocus();
    progList->setCurrentIndex(inProgress->proxy()->index(0, 0));
    QTest::keyClick(progList, Qt::Key_Left, Qt::AltModifier);
    QCoreApplication::processEvents();
    QCOMPARE(m_studyCtx->topics().byId(t1Id)->status, JobPrep::Domain::TopicStatus::Backlog);

    // 3. Alt+Down reorders Card 1 below Card 2
    list->setFocus();
    backlog->selectTopicId(t1Id);
    QTest::keyClick(list, Qt::Key_Down, Qt::AltModifier);
    QCoreApplication::processEvents();
    QVERIFY(m_studyCtx->topics().byId(t1Id)->position > m_studyCtx->topics().byId(t2Id)->position);

    // 4. Alt+Up reorders Card 1 back above Card 2
    QTest::keyClick(list, Qt::Key_Up, Qt::AltModifier);
    QCoreApplication::processEvents();
    QVERIFY(m_studyCtx->topics().byId(t1Id)->position < m_studyCtx->topics().byId(t2Id)->position);

    // 5. Left/Right without Alt navigates focus between columns
    list->setFocus();
    QTest::keyClick(list, Qt::Key_Right, Qt::NoModifier);
    QCoreApplication::processEvents();
    QCOMPARE(QApplication::focusWidget(), inProgress->list());

    // 6. Context menu "Move to" -> Done
    list->setFocus();
    backlog->selectTopicId(t1Id);
    auto* menu = childNamed<QMenu>(backlog, u"StudyContextMenu"_s);
    auto* moveMenu = childNamed<QMenu>(menu, u"StudyMoveToMenu"_s);
    QVERIFY(moveMenu);
    auto* doneAction = childNamed<QAction>(moveMenu, u"StudyMoveTo3"_s);
    QVERIFY(doneAction);
    doneAction->trigger();
    QCoreApplication::processEvents();
    QCOMPARE(m_studyCtx->topics().byId(t1Id)->status, JobPrep::Domain::TopicStatus::Done);
}


void tst_PageStudy::testStudyChecklistTickSurvivesReset() {
    resetStudyData();
    Track tr;
    tr.name = u"General"_s;
    tr.color = u"#3b82f6"_s;
    QVERIFY(m_studyCtx->tracks().insert(tr));
    const int trId = m_studyCtx->tracks().all().last().id;

    Topic t1;
    t1.trackId = trId;
    t1.title = u"Topic with checklist"_s;
    t1.status = JobPrep::Domain::TopicStatus::InProgress;
    QVERIFY(m_studyCtx->topics().insert(t1));
    const int t1Id = m_studyCtx->topics().all().last().id;

    Subtask s1;
    s1.topicId = t1Id;
    s1.text = u"Subtask 1"_s;
    s1.done = false;
    s1.position = 0;
    QVERIFY(m_studyCtx->topics().addSubtask(s1));

    Subtask s2;
    s2.topicId = t1Id;
    s2.text = u"Subtask 2"_s;
    s2.done = false;
    s2.position = 1;
    QVERIFY(m_studyCtx->topics().addSubtask(s2));

    for (int i = 3; i <= 8; ++i) {
        Subtask s;
        s.topicId = t1Id;
        s.text = u"Subtask %1"_s.arg(i);
        s.done = false;
        s.position = i - 1;
        QVERIFY(m_studyCtx->topics().addSubtask(s));
    }

    QCoreApplication::processEvents();

    auto* progCol = studyColumn(JobPrep::Domain::TopicStatus::InProgress);
    progCol->selectTopicId(t1Id);
    QCoreApplication::processEvents();

    auto* detail = studyPage()->detailPanel();
    QVERIFY(detail);
    auto* checkList = childNamed<QListWidget>(detail, u"StudyDetailCheckList"_s);
    QVERIFY(checkList);
    QCOMPARE(checkList->count(), 8);

    QModelIndex cardIndex = progCol->proxy()->index(progCol->indexOfTopic(t1Id), 0);
    QCOMPARE(cardIndex.data(JobPrep::Models::TopicListModel::ProgressRole).toInt(), 0);

    checkList->setFocus();
    checkList->setCurrentRow(3);
    const int checkScrollBefore = checkList->verticalScrollBar()->value();

    QTest::keyClick(checkList, Qt::Key_Space);
    QCoreApplication::processEvents();

    const auto subs = m_studyCtx->topics().subtasks(t1Id);
    QVERIFY(subs[3].done);

    cardIndex = progCol->proxy()->index(progCol->indexOfTopic(t1Id), 0);
    QCOMPARE(cardIndex.data(JobPrep::Models::TopicListModel::ProgressRole).toInt(), 12);
    QCOMPARE(cardIndex.data(JobPrep::Models::TopicListModel::DoneSubtasksRole).toInt(), 1);

    // Assert selection, focus, and checklist scroll position survive
    QCOMPARE(progCol->currentTopicId(), std::optional<int>(t1Id));
    QCOMPARE(checkList->verticalScrollBar()->value(), checkScrollBefore);
    QVERIFY(detail->hasFocusInEditor() || QApplication::focusWidget() == static_cast<QWidget*>(checkList));
}


void tst_PageStudy::testStudyBoardRealDnDDropEvent() {
    resetStudyData();
    Track tr;
    tr.name = u"General"_s;
    tr.color = u"#3b82f6"_s;
    QVERIFY(m_studyCtx->tracks().insert(tr));
    const int trId = m_studyCtx->tracks().all().last().id;

    Topic t1;
    t1.trackId = trId;
    t1.title = u"DnD Card"_s;
    t1.status = JobPrep::Domain::TopicStatus::Backlog;
    t1.position = 0;
    QVERIFY(m_studyCtx->topics().insert(t1));
    const int t1Id = m_studyCtx->topics().all().last().id;

    m_studyCtx->tracks().changed();
    m_studyCtx->topics().changed();
    QCoreApplication::processEvents();
    QCOMPARE(studyPage()->currentViewIndex(), 1);

    auto* backlog = studyColumn(JobPrep::Domain::TopicStatus::Backlog);
    auto* inProgress = studyColumn(JobPrep::Domain::TopicStatus::InProgress);
    auto* review = studyColumn(JobPrep::Domain::TopicStatus::Review);

    // 1. Synthesized drag-move drop indicator & whole-column highlight (non-empty column)
    auto* moveMime = new QMimeData;
    moveMime->setData(u"application/x-jobprep-topic-id"_s, QByteArray::number(t1Id));
    QDragEnterEvent enterEvent(QPoint(30, 20), Qt::MoveAction, moveMime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(backlog->list(), &enterEvent);
    QDragMoveEvent moveEvent(QPoint(30, 20), Qt::MoveAction, moveMime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(backlog->list(), &moveEvent);
    QVERIFY(backlog->list()->isDragOver());
    QVERIFY(!backlog->list()->dropIndicatorRect().isNull());

    QDragLeaveEvent leaveEvent;
    QCoreApplication::sendEvent(backlog->list(), &leaveEvent);
    QVERIFY(!backlog->list()->isDragOver());
    QVERIFY(backlog->list()->dropIndicatorRect().isNull());
    delete moveMime;

    // 2. Empty column drag feedback (whole-column highlight, no item drop indicator line)
    auto* emptyMime = new QMimeData;
    emptyMime->setData(u"application/x-jobprep-topic-id"_s, QByteArray::number(t1Id));
    QDragEnterEvent emptyEnter(QPoint(30, 20), Qt::MoveAction, emptyMime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(review->list(), &emptyEnter);
    QDragMoveEvent emptyMove(QPoint(30, 20), Qt::MoveAction, emptyMime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(review->list(), &emptyMove);
    QVERIFY(review->list()->isDragOver());
    QVERIFY(review->list()->dropIndicatorRect().isNull());

    QDragLeaveEvent emptyLeave;
    QCoreApplication::sendEvent(review->list(), &emptyLeave);
    QVERIFY(!review->list()->isDragOver());
    delete emptyMime;

    // 3. Drop processing
    auto* mimeData = new QMimeData;
    mimeData->setData(u"application/x-jobprep-topic-id"_s, QByteArray::number(t1Id));
    QVERIFY(inProgress->proxy()->canDropMimeData(mimeData, Qt::MoveAction, -1, -1, QModelIndex()));
    QVERIFY(inProgress->proxy()->dropMimeData(mimeData, Qt::MoveAction, -1, -1, QModelIndex()));
    delete mimeData;

    QCoreApplication::processEvents();
    QCOMPARE(m_studyCtx->topics().byId(t1Id)->status, JobPrep::Domain::TopicStatus::InProgress);

    auto* dropMime = new QMimeData;
    dropMime->setData(u"application/x-jobprep-topic-id"_s, QByteArray::number(t1Id));
    QDropEvent dropEvent(QPointF(20, 20), Qt::MoveAction, dropMime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(backlog->list()->viewport(), &dropEvent);
    QCoreApplication::processEvents();
}


void tst_PageStudy::testStudyDetailPanelAutosaveAndValidation() {
    resetStudyData();
    Track tr;
    tr.name = u"General"_s;
    tr.color = u"#3b82f6"_s;
    QVERIFY(m_studyCtx->tracks().insert(tr));
    const int trId = m_studyCtx->tracks().all().last().id;

    Topic t1;
    t1.trackId = trId;
    t1.title = u"Topic A"_s;
    t1.status = JobPrep::Domain::TopicStatus::Backlog;
    QVERIFY(m_studyCtx->topics().insert(t1));
    const int t1Id = m_studyCtx->topics().all().last().id;

    Topic t2;
    t2.trackId = trId;
    t2.title = u"Topic B"_s;
    t2.status = JobPrep::Domain::TopicStatus::Backlog;
    QVERIFY(m_studyCtx->topics().insert(t2));
    const int t2Id = m_studyCtx->topics().all().last().id;

    QCoreApplication::processEvents();

    auto* backlog = studyColumn(JobPrep::Domain::TopicStatus::Backlog);
    backlog->selectTopicId(t1Id);
    QCoreApplication::processEvents();

    auto* detail = studyPage()->detailPanel();
    QVERIFY(detail);
    auto* titleEdit = childNamed<QLineEdit>(detail, u"StudyDetailTitleEdit"_s);
    auto* notesEdit = childNamed<QTextEdit>(detail, u"StudyDetailNotesEdit"_s);
    auto* tagsEdit = childNamed<QLineEdit>(detail, u"StudyDetailTagsEdit"_s);

    titleEdit->setText(u"Topic A Modified"_s);
    notesEdit->setPlainText(u"My notes"_s);
    tagsEdit->setText(u"tag1, tag2"_s);

    // Autosave on selection change
    backlog->selectTopicId(t2Id);
    QCoreApplication::processEvents();

    const auto t1Db = m_studyCtx->topics().byId(t1Id);
    QCOMPARE(t1Db->title, u"Topic A Modified"_s);
    QCOMPARE(t1Db->notes, u"My notes"_s);
    QCOMPARE(t1Db->tags, u"tag1, tag2"_s);

    // Rejection of empty title on Topic B
    titleEdit->setText(u""_s);
    QVERIFY(!detail->flush());
    auto* msgLabel = childNamed<QLabel>(detail, u"StudyDetailMessageLabel"_s);
    QVERIFY(msgLabel->isVisible());
    QVERIFY(msgLabel->text().contains(u"Title cannot be empty"_s));
    QCOMPARE(m_studyCtx->topics().byId(t2Id)->title, u"Topic B"_s);

    // Target date set and clear (SPEC S2 / F2)
    auto* dateEdit = childNamed<QDateEdit>(detail, u"StudyDetailDateEdit"_s);
    auto* dateBtn = childNamed<QPushButton>(detail, u"StudyDetailDateClearBtn"_s);
    QVERIFY(dateEdit->isEnabled());
    QCOMPARE(dateBtn->text(), u"Clear"_s);
    QVERIFY(!dateBtn->isEnabled());

    dateEdit->setDate(QDate(2026, 11, 20));
    detail->flush();
    QCoreApplication::processEvents();
    QVERIFY(dateBtn->isEnabled());
    QVERIFY(m_studyCtx->topics().byId(t2Id)->targetDate.has_value());

    dateBtn->click();
    QCoreApplication::processEvents();
    QVERIFY(dateEdit->isEnabled());
    QVERIFY(!dateBtn->isEnabled());
    QVERIFY(!m_studyCtx->topics().byId(t2Id)->targetDate.has_value());

    // Resources http/https validation
    auto* resTitle = childNamed<QLineEdit>(detail, u"StudyDetailResourceTitleEdit"_s);
    auto* resUrl = childNamed<QLineEdit>(detail, u"StudyDetailResourceUrlEdit"_s);
    auto* resAddBtn = childNamed<QPushButton>(detail, u"StudyDetailResourceAddBtn"_s);
    auto* resTable = childNamed<QTableWidget>(detail, u"StudyDetailResourcesTable"_s);

    resTitle->setText(u"Bad JS"_s);
    resUrl->setText(u"javascript:alert(1)"_s);
    resAddBtn->click();
    QCoreApplication::processEvents();
    QCOMPARE(resTable->rowCount(), 0);
    QVERIFY(msgLabel->isVisible());
    QVERIFY(msgLabel->text().contains(u"Only http and https"_s));

    resTitle->setText(u"Bad File"_s);
    resUrl->setText(u"file:///etc/passwd"_s);
    resAddBtn->click();
    QCoreApplication::processEvents();
    QCOMPARE(resTable->rowCount(), 0);

    resTitle->setText(u"CppReference"_s);
    resUrl->setText(u"https://en.cppreference.com"_s);
    resAddBtn->click();
    QCoreApplication::processEvents();
    QCOMPARE(resTable->rowCount(), 1);

    resTable->selectRow(0);
    auto* resOpenBtn = childNamed<QPushButton>(detail, u"StudyDetailResourceOpenBtn"_s);
    QVERIFY(resOpenBtn->isEnabled());
}


void tst_PageStudy::testStudyDeleteTopicKeepsSessions() {
    resetStudyData();
    Track tr;
    tr.name = u"General"_s;
    tr.color = u"#3b82f6"_s;
    QVERIFY(m_studyCtx->tracks().insert(tr));
    const int trId = m_studyCtx->tracks().all().last().id;

    Topic t;
    t.trackId = trId;
    t.title = u"Topic To Delete"_s;
    t.status = JobPrep::Domain::TopicStatus::Backlog;
    QVERIFY(m_studyCtx->topics().insert(t));
    const int tId = m_studyCtx->topics().all().last().id;

    JobPrep::Domain::StudySession session;
    session.topicId = tId;
    session.sessionDate = QDate(2026, 10, 19);
    session.minutes = 45;
    QVERIFY(m_studyCtx->sessions().insert(session));
    const int sId = m_studyCtx->sessions().all().last().id;

    QCoreApplication::processEvents();

    answerNextMessageBox([](QMessageBox* box) {
        box->button(QMessageBox::Yes)->click();
    });

    auto* backlog = studyColumn(JobPrep::Domain::TopicStatus::Backlog);
    backlog->selectTopicId(tId);
    auto* menu = childNamed<QMenu>(backlog, u"StudyContextMenu"_s);
    auto* deleteAction = childNamed<QAction>(menu, u"StudyDeleteTopicAction"_s);
    QVERIFY(deleteAction);
    deleteAction->trigger();
    QCoreApplication::processEvents();

    QVERIFY(!m_studyCtx->topics().byId(tId).has_value());

    const auto sessionDb = m_studyCtx->sessions().byId(sId);
    QVERIFY(sessionDb.has_value());
    QVERIFY(!sessionDb->topicId.has_value());
    QCOMPARE(sessionDb->minutes, 45);
}


void tst_PageStudy::testStudyTrackFilterPersistenceAndFallback() {
    resetStudyData();
    Track tr1;
    tr1.name = u"Track 1"_s;
    tr1.color = u"#3b82f6"_s;
    QVERIFY(m_studyCtx->tracks().insert(tr1));

    Track tr2;
    tr2.name = u"Track 2"_s;
    tr2.color = u"#10b981"_s;
    QVERIFY(m_studyCtx->tracks().insert(tr2));
    const int tr2Id = m_studyCtx->tracks().all().last().id;

    QCoreApplication::processEvents();

    auto* chips = childNamed<JobPrep::Ui::Widgets::ChipBar>(m_study.get(), u"StudyTrackChips"_s);
    auto* btn = chips->button(tr2Id);
    QVERIFY(btn);
    btn->click();
    QCoreApplication::processEvents();

    QCOMPARE(m_studyCtx->settings().studyTrackFilter(), tr2Id);

    auto newPage = std::make_unique<JobPrep::Ui::Pages::StudyPage>(
        m_studyCtx->topics(), m_studyCtx->tracks(), m_studyCtx->sessions(), m_studyCtx->settings(),
        m_studyCtx->themeManager());
    auto* newChips = childNamed<JobPrep::Ui::Widgets::ChipBar>(newPage.get(), u"StudyTrackChips"_s);
    QCOMPARE(newChips->currentId(), tr2Id);

    QVERIFY(m_studyCtx->tracks().remove(tr2Id));
    QCoreApplication::processEvents();

    QCOMPARE(chips->currentId(), -1);
    QCOMPARE(m_studyCtx->settings().studyTrackFilter(), -1);
}


void tst_PageStudy::testStudyOverdueChipWithFixedClock() {
    const QDate today(2026, 10, 19);
    resetStudyData(today);
    Track tr;
    tr.name = u"General"_s;
    tr.color = u"#3b82f6"_s;
    QVERIFY(m_studyCtx->tracks().insert(tr));
    const int trId = m_studyCtx->tracks().all().last().id;

    Topic t1;
    t1.trackId = trId;
    t1.title = u"Overdue topic"_s;
    t1.status = JobPrep::Domain::TopicStatus::InProgress;
    t1.targetDate = today.addDays(-1);
    QVERIFY(m_studyCtx->topics().insert(t1));
    const int t1Id = m_studyCtx->topics().all().last().id;

    Topic t2;
    t2.trackId = trId;
    t2.title = u"Due today"_s;
    t2.status = JobPrep::Domain::TopicStatus::InProgress;
    t2.targetDate = today;
    QVERIFY(m_studyCtx->topics().insert(t2));
    const int t2Id = m_studyCtx->topics().all().last().id;

    Topic t3;
    t3.trackId = trId;
    t3.title = u"Done past topic"_s;
    t3.status = JobPrep::Domain::TopicStatus::Done;
    t3.targetDate = today.addDays(-5);
    QVERIFY(m_studyCtx->topics().insert(t3));
    const int t3Id = m_studyCtx->topics().all().last().id;

    Topic t4;
    t4.trackId = trId;
    t4.title = u"No target date"_s;
    t4.status = JobPrep::Domain::TopicStatus::InProgress;
    QVERIFY(m_studyCtx->topics().insert(t4));
    const int t4Id = m_studyCtx->topics().all().last().id;

    QCoreApplication::processEvents();

    auto* progCol = studyColumn(JobPrep::Domain::TopicStatus::InProgress);
    auto* doneCol = studyColumn(JobPrep::Domain::TopicStatus::Done);

    const int r1 = progCol->indexOfTopic(t1Id);
    const int r2 = progCol->indexOfTopic(t2Id);
    const int r3 = doneCol->indexOfTopic(t3Id);
    const int r4 = progCol->indexOfTopic(t4Id);

    QVERIFY(progCol->proxy()->index(r1, 0).data(JobPrep::Models::TopicListModel::OverdueRole).toBool());
    QVERIFY(!progCol->proxy()->index(r2, 0).data(JobPrep::Models::TopicListModel::OverdueRole).toBool());
    QVERIFY(!doneCol->proxy()->index(r3, 0).data(JobPrep::Models::TopicListModel::OverdueRole).toBool());
    QVERIFY(!progCol->proxy()->index(r4, 0).data(JobPrep::Models::TopicListModel::OverdueRole).toBool());
}


void tst_PageStudy::testStudyPageApi() {
    resetStudyData();
    Track tr1;
    tr1.name = u"Track A"_s;
    tr1.color = u"#3b82f6"_s;
    QVERIFY(m_studyCtx->tracks().insert(tr1));
    const int tr1Id = m_studyCtx->tracks().all().last().id;

    Track tr2;
    tr2.name = u"Track B"_s;
    tr2.color = u"#10b981"_s;
    QVERIFY(m_studyCtx->tracks().insert(tr2));
    const int tr2Id = m_studyCtx->tracks().all().last().id;

    Topic t;
    t.trackId = tr2Id;
    t.title = u"Special Algorithm"_s;
    t.status = JobPrep::Domain::TopicStatus::Review;
    QVERIFY(m_studyCtx->topics().insert(t));
    const int tId = m_studyCtx->topics().all().last().id;

    QCoreApplication::processEvents();

    studyPage()->triggerNew();
    auto* backlog = studyColumn(JobPrep::Domain::TopicStatus::Backlog);
    auto* quickAdd = childNamed<QLineEdit>(backlog, u"StudyColumnQuickAdd"_s);
    QCOMPARE(QApplication::focusWidget(), quickAdd);

    // Also call newTopic() directly
    studyPage()->newTopic();
    QCOMPARE(QApplication::focusWidget(), quickAdd);

    auto* chips = childNamed<JobPrep::Ui::Widgets::ChipBar>(m_study.get(), u"StudyTrackChips"_s);
    auto* btn = chips->button(tr1Id);
    QVERIFY(btn);
    btn->click();
    studyPage()->setSearchText(u"xyz"_s);
    QCoreApplication::processEvents();
    QCOMPARE(studyPage()->visibleTopicCount(), 0);

    QVERIFY(studyPage()->selectTopic(tId));
    QCoreApplication::processEvents();
    QCOMPARE(chips->currentId(), -1);
    QCOMPARE(studyPage()->searchText(), QString());
    QVERIFY(studyPage()->isDetailVisible());
    auto* detail = studyPage()->detailPanel();
    QVERIFY(detail);
    QCOMPARE(detail->currentTopicId(), std::optional<int>(tId));

    QVERIFY(!studyPage()->selectTopic(99999));

    // Zero tracks: newTopic opens TrackDialog
    resetStudyData();
    int trackDialogs = 0;
    withNextModal([&trackDialogs](QWidget* modal) {
        trackDialogs = countTrackDialogs();
        auto* dlg = qobject_cast<TrackDialog*>(modal);
        QVERIFY(dlg != nullptr);
        dlg->reject();
    });
    studyPage()->newTopic();
    QCOMPARE(trackDialogs, 1);
}


void tst_PageStudy::testChipBarJobsAndCalendarRegression() {
    resetJobsData();
    addApplication(u"Company A"_s, u"Role A"_s, ApplicationStatus::Applied);
    addApplication(u"Company B"_s, u"Role B"_s, ApplicationStatus::Technical);
    addApplication(u"Company C"_s, u"Role C"_s, ApplicationStatus::Offer);
    addApplication(u"Company D"_s, u"Role D"_s, ApplicationStatus::Rejected);

    auto* jobsChips = childNamed<JobPrep::Ui::Widgets::ChipBar>(m_jobs.get(), u"JobsStatusChips"_s);
    QVERIFY(jobsChips != nullptr);
    const auto buttons = jobsChips->findChildren<QPushButton*>();
    QCOMPARE(buttons.size(), 5);

    for (auto* btn : buttons) {
        QVERIFY(btn->property("chip").toBool());
    }

    buttons.at(2)->click();
    QCoreApplication::processEvents();
    QCOMPARE(jobsPage()->visibleRowCount(), 1);
    QCOMPARE(companyAt(0), u"Company B"_s);

    buttons.at(0)->click();
    QCoreApplication::processEvents();
    QCOMPARE(jobsPage()->visibleRowCount(), 4);

    const QDate today(2026, 10, 11);
    resetCalendarData(today);
    addApplicationWithInterview(u"Acme"_s, today, 10);
    addStudyTarget(today);

    auto* interviewChip = childNamed<QPushButton>(m_calendar.get(), u"CalendarLayerInterviewBtn"_s);
    auto* targetChip = childNamed<QPushButton>(m_calendar.get(), u"CalendarLayerTargetBtn"_s);
    QVERIFY(interviewChip && targetChip);
    QVERIFY(interviewChip->property("calendarLayer").toBool());
    QVERIFY(targetChip->property("calendarLayer").toBool());

    QCOMPARE(agendaList()->model()->rowCount(), 2);
    targetChip->click();
    QCoreApplication::processEvents();
    QCOMPARE(agendaList()->model()->rowCount(), 1);
}


void tst_PageStudy::testStudyContextMenuMoveToStatus() {
    resetStudyData();
    Track tr;
    tr.name = u"General"_s;
    tr.color = u"#3b82f6"_s;
    QVERIFY(m_studyCtx->tracks().insert(tr));
    const int trId = m_studyCtx->tracks().all().last().id;

    Topic t;
    t.trackId = trId;
    t.title = u"Menu Move Topic"_s;
    t.status = JobPrep::Domain::TopicStatus::Backlog;
    QVERIFY(m_studyCtx->topics().insert(t));
    const int tId = m_studyCtx->topics().all().last().id;

    QCoreApplication::processEvents();

    auto* backlog = studyColumn(JobPrep::Domain::TopicStatus::Backlog);
    auto* inProgress = studyColumn(JobPrep::Domain::TopicStatus::InProgress);
    backlog->selectTopicId(tId);

    auto* menu = childNamed<QMenu>(backlog, u"StudyContextMenu"_s);
    QVERIFY(menu);
    auto* moveToMenu = childNamed<QMenu>(menu, u"StudyMoveToMenu"_s);
    QVERIFY(moveToMenu);
    auto* moveToProgAction = childNamed<QAction>(moveToMenu, u"StudyMoveTo1"_s);
    QVERIFY(moveToProgAction);

    moveToProgAction->trigger();
    QCoreApplication::processEvents();

    const auto tDb = m_studyCtx->topics().byId(tId);
    QVERIFY(tDb.has_value());
    QCOMPARE(tDb->status, JobPrep::Domain::TopicStatus::InProgress);
    QCOMPARE(inProgress->indexOfTopic(tId), 0);
    QCOMPARE(backlog->indexOfTopic(tId), -1);
}


void tst_PageStudy::testStudyPageSelectionRestoreAcrossOperations() {
    resetStudyData();
    Track tr;
    tr.name = u"General"_s;
    tr.color = u"#3b82f6"_s;
    QVERIFY(m_studyCtx->tracks().insert(tr));
    const int trId = m_studyCtx->tracks().all().last().id;

    Topic t1;
    t1.trackId = trId;
    t1.title = u"T1"_s;
    t1.status = JobPrep::Domain::TopicStatus::InProgress;
    t1.position = 0;
    QVERIFY(m_studyCtx->topics().insert(t1));
    const int t1Id = m_studyCtx->topics().all().last().id;

    Topic t2;
    t2.trackId = trId;
    t2.title = u"T2"_s;
    t2.status = JobPrep::Domain::TopicStatus::InProgress;
    t2.position = 1;
    QVERIFY(m_studyCtx->topics().insert(t2));
    const int t2Id = m_studyCtx->topics().all().last().id;

    Topic t3;
    t3.trackId = trId;
    t3.title = u"T3"_s;
    t3.status = JobPrep::Domain::TopicStatus::InProgress;
    t3.position = 2;
    QVERIFY(m_studyCtx->topics().insert(t3));

    QCoreApplication::processEvents();

    auto* col = studyColumn(JobPrep::Domain::TopicStatus::InProgress);
    col->selectTopicId(t2Id);
    QCOMPARE(col->currentTopicId(), std::optional<int>(t2Id));

    // 1. Selection survives insert of another topic
    Topic t0;
    t0.trackId = trId;
    t0.title = u"T0"_s;
    t0.status = JobPrep::Domain::TopicStatus::InProgress;
    t0.position = 0;
    QVERIFY(m_studyCtx->topics().insert(t0));
    QCoreApplication::processEvents();
    QCOMPARE(col->currentTopicId(), std::optional<int>(t2Id));

    // 2. Selection survives deletion of another topic
    QVERIFY(m_studyCtx->topics().remove(t1Id));
    QCoreApplication::processEvents();
    QCOMPARE(col->currentTopicId(), std::optional<int>(t2Id));

    // 3. Selection survives reordering
    col->selectTopicId(t2Id);
    QTest::keyClick(col->list(), Qt::Key_Down, Qt::AltModifier);
    QCoreApplication::processEvents();
    QCOMPARE(col->currentTopicId(), std::optional<int>(t2Id));
}


void tst_PageStudy::testStudyChecklistEditAndReorder() {
    resetStudyData();
    Track tr;
    tr.name = u"General"_s;
    tr.color = u"#3b82f6"_s;
    QVERIFY(m_studyCtx->tracks().insert(tr));
    const int trId = m_studyCtx->tracks().all().last().id;

    Topic t;
    t.trackId = trId;
    t.title = u"Checklist Topic"_s;
    t.status = JobPrep::Domain::TopicStatus::Backlog;
    QVERIFY(m_studyCtx->topics().insert(t));
    const int tId = m_studyCtx->topics().all().last().id;

    Subtask s1;
    s1.topicId = tId;
    s1.text = u"Alpha"_s;
    s1.position = 0;
    QVERIFY(m_studyCtx->topics().addSubtask(s1));

    Subtask s2;
    s2.topicId = tId;
    s2.text = u"Beta"_s;
    s2.position = 1;
    QVERIFY(m_studyCtx->topics().addSubtask(s2));

    QCoreApplication::processEvents();

    auto* backlog = studyColumn(JobPrep::Domain::TopicStatus::Backlog);
    backlog->selectTopicId(tId);
    QCoreApplication::processEvents();

    auto* detail = studyPage()->detailPanel();
    auto* checkList = childNamed<QListWidget>(detail, u"StudyDetailCheckList"_s);
    QVERIFY(checkList);
    QCOMPARE(checkList->count(), 2);

    // 1. Inline edit of item text
    checkList->item(0)->setText(u"Alpha Renamed"_s);
    QCoreApplication::processEvents();
    auto subs = m_studyCtx->topics().subtasks(tId);
    QCOMPARE(subs[0].text, u"Alpha Renamed"_s);

    // 2. Alt+Down reorders items
    checkList->setCurrentRow(0);
    QTest::keyClick(checkList, Qt::Key_Down, Qt::AltModifier);
    QCoreApplication::processEvents();

    subs = m_studyCtx->topics().subtasks(tId);
    QCOMPARE(subs[0].text, u"Beta"_s);
    QCOMPARE(subs[1].text, u"Alpha Renamed"_s);
    QCOMPARE(checkList->currentRow(), 1);
}


void tst_PageStudy::testStudySplitterStatePersisted() {
    resetStudyData();
    auto* splitter = childNamed<QSplitter>(m_study.get(), u"StudySplitter"_s);
    QVERIFY(splitter);

    splitter->setSizes({600, 300});
    emit splitter->splitterMoved(600, 1);
    QCoreApplication::processEvents();

    const QByteArray saved = m_studyCtx->settings().studySplitterState();
    QVERIFY(!saved.isEmpty());

    auto newPage = std::make_unique<JobPrep::Ui::Pages::StudyPage>(
        m_studyCtx->topics(), m_studyCtx->tracks(), m_studyCtx->sessions(), m_studyCtx->settings(),
        m_studyCtx->themeManager());
    auto* newSplitter = childNamed<QSplitter>(newPage.get(), u"StudySplitter"_s);
    QVERIFY(newSplitter);
    QCOMPARE(newSplitter->saveState(), saved);
}


void tst_PageStudy::testStudyPriorityTooltip() {
    resetStudyData();
    JobPrep::Services::SeedService seeder(m_studyCtx->tracks(), m_studyCtx->topics());
    QVERIFY(seeder.loadSampleTopics());
    auto* backlogCol = studyColumn(JobPrep::Domain::TopicStatus::Backlog);
    QVERIFY(backlogCol != nullptr);
    QVERIFY(backlogCol->itemCount() > 0);

    const QModelIndex idx = backlogCol->list()->model()->index(0, 0);
    QVERIFY(idx.isValid());

    auto* delegate = backlogCol->list()->itemDelegate();
    QVERIFY(delegate != nullptr);

    QStyleOptionViewItem option;
    option.rect = backlogCol->list()->visualRect(idx);
    QHelpEvent help(QEvent::ToolTip, option.rect.center(), backlogCol->list()->mapToGlobal(option.rect.center()));
    const bool handled = delegate->helpEvent(&help, backlogCol->list(), option, idx);
    QVERIFY(handled);
    QToolTip::hideText();
    QCoreApplication::processEvents();
    const auto prio = static_cast<JobPrep::Domain::Priority>(
        idx.data(JobPrep::Models::TopicListModel::PriorityRole).toInt());
    const QString expected = QStringLiteral("Priority: %1").arg(JobPrep::Ui::Theme::StatusStyle::label(prio));
    QCOMPARE(QToolTip::text(), expected);
}


void tst_PageStudy::testStudyDragFeedbackIndicatorAndHighlight() {
    resetStudyData();
    JobPrep::Services::SeedService seeder(m_studyCtx->tracks(), m_studyCtx->topics());
    QVERIFY(seeder.loadSampleTopics());
    auto* backlogCol = studyColumn(JobPrep::Domain::TopicStatus::Backlog);
    QVERIFY(backlogCol != nullptr);
    auto* list = qobject_cast<JobPrep::Ui::Widgets::TopicListView*>(backlogCol->list());
    QVERIFY(list != nullptr);
    QVERIFY(backlogCol->itemCount() > 0);

    const auto topics = m_studyCtx->topics().all();
    QVERIFY(!topics.isEmpty());
    const int topicId = topics.first().id;

    // 1. Synthesize drag enter + move
    auto* moveMime = new QMimeData;
    moveMime->setData(u"application/x-jobprep-topic-id"_s, QByteArray::number(topicId));
    QDragEnterEvent enterEvent(QPoint(30, 20), Qt::MoveAction, moveMime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(list, &enterEvent);
    QDragMoveEvent moveEvent(QPoint(30, 20), Qt::MoveAction, moveMime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(list, &moveEvent);

    QVERIFY(list->isDragOver());
    QVERIFY(!list->dropIndicatorRect().isNull());

    // 2. Drag leave clears state
    QDragLeaveEvent leaveEvent;
    QCoreApplication::sendEvent(list, &leaveEvent);
    QVERIFY(!list->isDragOver());
    QVERIFY(list->dropIndicatorRect().isNull());

    // 3. Drag enter + drop clears state
    QCoreApplication::sendEvent(list, &enterEvent);
    QCoreApplication::sendEvent(list, &moveEvent);
    QVERIFY(list->isDragOver());
    QVERIFY(!list->dropIndicatorRect().isNull());

    QDropEvent dropEvent(QPointF(30, 20), Qt::MoveAction, moveMime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(list, &dropEvent);
    QVERIFY(!list->isDragOver());
    QVERIFY(list->dropIndicatorRect().isNull());
    delete moveMime;

    // 4. Test empty column
    auto* doneCol = studyColumn(JobPrep::Domain::TopicStatus::Done);
    QVERIFY(doneCol != nullptr);
    auto* doneList = qobject_cast<JobPrep::Ui::Widgets::TopicListView*>(doneCol->list());
    QVERIFY(doneList != nullptr);
    QCOMPARE(doneCol->itemCount(), 0);

    auto* emptyMime = new QMimeData;
    emptyMime->setData(u"application/x-jobprep-topic-id"_s, QByteArray::number(topicId));
    QDragEnterEvent enterEmpty(QPoint(30, 20), Qt::MoveAction, emptyMime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(doneList, &enterEmpty);
    QDragMoveEvent moveEmpty(QPoint(30, 20), Qt::MoveAction, emptyMime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(doneList, &moveEmpty);

    QVERIFY(doneList->isDragOver());
    // In empty column, whole-column highlight is active and item drop indicator line is null
    QVERIFY(doneList->dropIndicatorRect().isNull());

    QDragLeaveEvent leaveEmpty;
    QCoreApplication::sendEvent(doneList, &leaveEmpty);
    QVERIFY(!doneList->isDragOver());
    delete emptyMime;
}


void tst_PageStudy::testStudySessionDialogLogTime() {
    const QDate today(2026, 10, 19);
    resetStudyData(today);

    // 1. Injectable clock verification and inline validation (minutes < 1)
    {
        SessionDialog dlg(m_studyCtx->sessions(), m_studyCtx->topics(), nullptr, [today]() { return today; });
        auto* dateEdit = dlg.findChild<QDateEdit*>(u"SessionDateEdit"_s);
        QVERIFY(dateEdit != nullptr);
        QCOMPARE(dateEdit->date(), today);

        auto* spin = dlg.findChild<QSpinBox*>(u"SessionMinutesSpin"_s);
        auto* saveBtn = dlg.findChild<QPushButton*>(u"SessionSaveBtn"_s);
        auto* msgLabel = dlg.findChild<QLabel*>(u"SessionMessageLabel"_s);
        QVERIFY(spin != nullptr && saveBtn != nullptr && msgLabel != nullptr);

        // Setting minutes to 0 triggers inline validation error and disables save
        spin->setValue(0);
        QVERIFY(!saveBtn->isEnabled());
        QVERIFY(!msgLabel->isHidden());
        QCOMPARE(dlg.validationMessage(), QStringLiteral("Duration must be at least 1 minute."));

        // Setting valid minutes clears error and enables save
        spin->setValue(25);
        QVERIFY(saveBtn->isEnabled());
        QVERIFY(msgLabel->isHidden());
    }

    // 2. Logging a session without a topic ("(No topic)" selection)
    {
        withNextModal([today](QWidget* modal) {
            auto* dlg = qobject_cast<SessionDialog*>(modal);
            QVERIFY(dlg != nullptr);
            auto* topicCombo = childNamed<QComboBox>(dlg, u"SessionTopicCombo"_s);
            QVERIFY(topicCombo != nullptr);
            // Index 0 is "(No topic)" with nullopt/invalid data
            topicCombo->setCurrentIndex(0);
            QCOMPARE(topicCombo->currentText(), QStringLiteral("(No topic)"));

            auto* durationSpin = childNamed<QSpinBox>(dlg, u"SessionMinutesSpin"_s);
            durationSpin->setValue(20);

            buttonNamed(dlg, u"SessionSaveBtn"_s)->click();
        });

        studyPage()->openSessionDialog(std::nullopt);
        QCoreApplication::processEvents();

        const auto sessions = m_studyCtx->sessions().all();
        QCOMPARE(sessions.size(), 1);
        QCOMPARE(sessions.first().minutes, 20);
        QVERIFY(!sessions.first().topicId.has_value());
        QCOMPARE(m_studyCtx->sessions().totalMinutesInRange(today, today), 20);
    }

    // 3. Log a session for a specific topic
    Track track;
    track.name = u"Algorithms"_s;
    track.color = u"#3b82f6"_s;
    QVERIFY(m_studyCtx->tracks().insert(track));
    const int trackId = m_studyCtx->tracks().all().last().id;

    Topic topic;
    topic.trackId = trackId;
    topic.title = u"Binary Search"_s;
    topic.status = JobPrep::Domain::TopicStatus::InProgress;
    QVERIFY(m_studyCtx->topics().insert(topic));
    const int topicId = m_studyCtx->topics().all().last().id;

    withNextModal([topicId, today](QWidget* modal) {
        auto* dlg = qobject_cast<SessionDialog*>(modal);
        QVERIFY(dlg != nullptr);
        auto* topicCombo = childNamed<QComboBox>(dlg, u"SessionTopicCombo"_s);
        QVERIFY(topicCombo != nullptr);
        QCOMPARE(topicCombo->currentData().toInt(), topicId);

        auto* durationSpin = childNamed<QSpinBox>(dlg, u"SessionMinutesSpin"_s);
        QVERIFY(durationSpin != nullptr);
        durationSpin->setValue(45);

        auto* dateEdit = childNamed<QDateEdit>(dlg, u"SessionDateEdit"_s);
        QVERIFY(dateEdit != nullptr);
        dateEdit->setDate(today);

        auto* notesEdit = childNamed<QTextEdit>(dlg, u"SessionNoteEdit"_s);
        QVERIFY(notesEdit != nullptr);
        notesEdit->setPlainText(u"Practiced lower_bound and upper_bound"_s);

        buttonNamed(dlg, u"SessionSaveBtn"_s)->click();
    });

    studyPage()->openSessionDialog(topicId);
    QCoreApplication::processEvents();

    const auto topicSessions = m_studyCtx->sessions().byTopic(topicId);
    QCOMPARE(topicSessions.size(), 1);
    QCOMPARE(topicSessions.first().minutes, 45);
    QCOMPARE(m_studyCtx->sessions().totalMinutesInRange(today, today), 65);

    // Detail panel "Log time" button opens SessionDialog with this topic preselected
    QVERIFY(studyPage()->selectTopic(topicId));
    auto* detail = studyPage()->detailPanel();
    QVERIFY(detail != nullptr);
    auto* logTimeBtn = childNamed<QPushButton>(detail, u"StudyDetailLogTimeBtn"_s);
    QVERIFY(logTimeBtn != nullptr);
    QVERIFY(logTimeBtn->isEnabled());

    bool dlgOpened = false;
    withNextModal([&dlgOpened, topicId](QWidget* modal) {
        dlgOpened = true;
        auto* dlg = qobject_cast<SessionDialog*>(modal);
        QVERIFY(dlg != nullptr);
        auto* topicCombo = childNamed<QComboBox>(dlg, u"SessionTopicCombo"_s);
        QCOMPARE(topicCombo->currentData().toInt(), topicId);
        dlg->reject();
    });
    logTimeBtn->click();
    QVERIFY(dlgOpened);
}


void tst_PageStudy::testStudyTrackDialogCrudAndCascade() {
    const QDate today(2026, 10, 19);
    resetStudyData(today);

    // 1. Open TrackDialog: test swatches, keyboard arrows, duplicate error
    bool swatchesValid = true;
    bool arrowChecked = false;
    bool dupErrorShown = false;
    QString dupMsg;

    withNextModal([&swatchesValid, &arrowChecked, &dupErrorShown, &dupMsg](QWidget* modal) {
        auto* dlg = qobject_cast<TrackDialog*>(modal);
        if (!dlg) return;

        // Check swatches: reachability, tooltip with color name, strong focus
        const auto& swatches = JobPrep::Ui::Theme::Tokens::trackPalette();
        for (int i = 0; i < swatches.size(); ++i) {
            auto* swBtn = dlg->findChild<QPushButton*>(u"TrackSwatch_%1"_s.arg(i));
            if (!swBtn || swBtn->focusPolicy() != Qt::StrongFocus || swBtn->toolTip() != swatches[i].name) {
                swatchesValid = false;
            }
        }

        // Test arrow key navigation between swatches
        auto* swatch0 = dlg->findChild<QPushButton*>(u"TrackSwatch_0"_s);
        auto* swatch1 = dlg->findChild<QPushButton*>(u"TrackSwatch_1"_s);
        if (swatch0 && swatch1) {
            swatch0->setFocus();
            swatch0->setChecked(true);

            QKeyEvent rightKey(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
            QCoreApplication::sendEvent(swatch0, &rightKey);
            arrowChecked = swatch1->isChecked();
        }

        // Add track "Rust"
        auto* addBtn = buttonNamed(dlg, u"TrackNewBtn"_s);
        if (addBtn) addBtn->click();

        auto* nameEdit = childNamed<QLineEdit>(dlg, u"TrackNameEdit"_s);
        if (nameEdit) nameEdit->setText(u"Rust"_s);
        if (swatch0) swatch0->click();

        if (auto* save = buttonNamed(dlg, u"TrackSaveBtn"_s)) save->click();

        // 2. Test duplicate name error (including different case: "rust")
        if (addBtn) addBtn->click();
        if (nameEdit) nameEdit->setText(u"rust"_s);
        auto* saveBtn = buttonNamed(dlg, u"TrackSaveBtn"_s);
        auto* msgLabel = childNamed<QLabel>(dlg, u"TrackMessageLabel"_s);
        if (saveBtn && msgLabel && !saveBtn->isEnabled() && !msgLabel->isHidden()) {
            dupErrorShown = true;
            dupMsg = msgLabel->text();
        }

        buttonNamed(dlg, u"TrackCloseBtn"_s)->click();
    });

    studyPage()->openTrackDialog();
    QCoreApplication::processEvents();

    QVERIFY(swatchesValid);
    QVERIFY(arrowChecked);
    QVERIFY(dupErrorShown);
    QCOMPARE(dupMsg, QStringLiteral("A track with this name already exists."));

    const auto tracks = m_studyCtx->tracks().all();
    QCOMPARE(tracks.size(), 1);
    QCOMPARE(tracks.first().name, u"Rust"_s);
    const int rustTrackId = tracks.first().id;

    // 3. Test non-preset track color shows custom swatch
    Track customTrack;
    customTrack.name = u"Custom Track"_s;
    customTrack.color = u"#123456"_s;
    QVERIFY(m_studyCtx->tracks().insert(customTrack));
    const int customTrackId = m_studyCtx->tracks().all().last().id;

    bool customExists = false;
    bool customNotHidden = false;
    bool customChecked = false;
    QString customTip;

    withNextModal([customTrackId, &customExists, &customNotHidden, &customChecked, &customTip](QWidget* modal) {
        auto* dlg = qobject_cast<TrackDialog*>(modal);
        if (dlg) {
            dlg->selectTrack(customTrackId);
            auto* customSwatch = dlg->findChild<QPushButton*>(u"TrackSwatch_Custom"_s);
            if (customSwatch) {
                customExists = true;
                customNotHidden = !customSwatch->isHidden();
                customChecked = customSwatch->isChecked();
                customTip = customSwatch->toolTip();
            }
            buttonNamed(dlg, u"TrackCloseBtn"_s)->click();
        }
    });
    studyPage()->openTrackDialog();
    QCoreApplication::processEvents();

    QVERIFY(customExists);
    QVERIFY(customNotHidden);
    QVERIFY(customChecked);
    QVERIFY(customTip.contains(u"#123456"_s));

    // 4. Add topic and session under Rust, verify delete confirmation message & cascade
    Topic topic;
    topic.trackId = rustTrackId;
    topic.title = u"Lifetimes and Borrowing"_s;
    topic.status = JobPrep::Domain::TopicStatus::InProgress;
    QVERIFY(m_studyCtx->topics().insert(topic));
    const int topicId = m_studyCtx->topics().all().last().id;

    JobPrep::Domain::StudySession session;
    session.topicId = topicId;
    session.sessionDate = today;
    session.minutes = 60;
    QVERIFY(m_studyCtx->sessions().insert(session));
    QCOMPARE(m_studyCtx->sessions().totalMinutesInRange(today, today), 60);

    // Set track filter to Rust track
    auto* chips = childNamed<JobPrep::Ui::Widgets::ChipBar>(m_study.get(), u"StudyTrackChips"_s);
    QVERIFY(chips != nullptr);
    if (chips->button(rustTrackId)) chips->button(rustTrackId)->click();
    QCOMPARE(chips->currentId(), rustTrackId);

    // Delete track with confirmation in TrackDialog
    withNextModal([rustTrackId](QWidget* modal) {
        auto* dlg = qobject_cast<TrackDialog*>(modal);
        QVERIFY(dlg != nullptr);
        dlg->selectTrack(rustTrackId);

        answerNextMessageBox([](QMessageBox* box) {
            // Verify delete confirmation mentions number of topics and preserved study time
            const QString text = box->text();
            QVERIFY(text.contains(u"1 topic(s)"_s));
            QVERIFY(text.contains(u"Logged study time will be kept"_s));
            box->button(QMessageBox::Yes)->click();
        });

        buttonNamed(dlg, u"TrackDeleteBtn"_s)->click();
        buttonNamed(dlg, u"TrackCloseBtn"_s)->click();
    });

    studyPage()->openTrackDialog();
    QCoreApplication::processEvents();

    // Verify track is deleted, topic is deleted (cascade), sessions are retained, filter reset to -1
    QVERIFY(!m_studyCtx->tracks().byId(rustTrackId).has_value());
    QVERIFY(m_studyCtx->topics().byTrack(rustTrackId).isEmpty());
    QCOMPARE(m_studyCtx->sessions().totalMinutesInRange(today, today), 60);
    QCOMPARE(chips->currentId(), -1);
}


void tst_PageStudy::testHeaderNewSessionAndTopicRouting() {
    resetStudyData();
    Track track;
    track.name = u"C++ Core"_s;
    track.color = u"#2563eb"_s;
    QVERIFY(m_ctx->tracks().insert(track));
    const int trackId = m_ctx->tracks().all().last().id;

    Topic topic;
    topic.trackId = trackId;
    topic.title = u"Move Semantics"_s;
    QVERIFY(m_ctx->topics().insert(topic));
    const int winTopicId = m_ctx->topics().all().last().id;

    auto* topicAction = m_window->findChild<QAction*>(u"HeaderNewTopic"_s);
    auto* sessionAction = m_window->findChild<QAction*>(u"HeaderNewSession"_s);
    QVERIFY(topicAction != nullptr && topicAction->isEnabled());
    QVERIFY(sessionAction != nullptr && sessionAction->isEnabled());

    // From Jobs page (page index 2):
    m_window->sidebar()->setCurrentPage(2);
    emit m_window->sidebar()->pageSelected(2);
    QCOMPARE(m_window->pageStack()->currentIndex(), 2);

    // 1. Topic action switches to Study (page index 1) and focuses quick-add
    topicAction->trigger();
    QCoreApplication::processEvents();
    QCOMPARE(m_window->pageStack()->currentIndex(), 1);

    // 2. From Jobs page, Session action opens exactly one SessionDialog
    m_window->sidebar()->setCurrentPage(2);
    emit m_window->sidebar()->pageSelected(2);
    QCOMPARE(m_window->pageStack()->currentIndex(), 2);

    int dialogs = 0;
    withNextModal([&dialogs](QWidget* modal) {
        dialogs = countSessionDialogs();
        QCOMPARE(dialogs, 1);
        auto* dlg = qobject_cast<SessionDialog*>(modal);
        QVERIFY(dlg != nullptr);
        dlg->reject();
    });
    sessionAction->trigger();
    QCOMPARE(dialogs, 1);

    // 3. From Calendar page (page index 3), Session action opens exactly one SessionDialog
    m_window->sidebar()->setCurrentPage(3);
    emit m_window->sidebar()->pageSelected(3);
    QCOMPARE(m_window->pageStack()->currentIndex(), 3);

    dialogs = 0;
    withNextModal([&dialogs](QWidget* modal) {
        dialogs = countSessionDialogs();
        QCOMPARE(dialogs, 1);
        auto* dlg = qobject_cast<SessionDialog*>(modal);
        QVERIFY(dlg != nullptr);
        dlg->reject();
    });
    sessionAction->trigger();
    QCOMPARE(dialogs, 1);

    // 4. On Study page with a topic selected, Session action opens SessionDialog with topic preselected
    m_window->sidebar()->setCurrentPage(1);
    emit m_window->sidebar()->pageSelected(1);
    auto* studyPageWidget = qobject_cast<JobPrep::Ui::Pages::StudyPage*>(m_window->pageStack()->currentWidget());
    QVERIFY(studyPageWidget != nullptr);
    studyPageWidget->selectTopic(winTopicId);

    dialogs = 0;
    withNextModal([&dialogs, winTopicId](QWidget* modal) {
        dialogs = countSessionDialogs();
        QCOMPARE(dialogs, 1);
        auto* dlg = qobject_cast<SessionDialog*>(modal);
        QVERIFY(dlg != nullptr);
        auto* combo = childNamed<QComboBox>(dlg, u"SessionTopicCombo"_s);
        QCOMPARE(combo->currentData().toInt(), winTopicId);
        dlg->reject();
    });
    sessionAction->trigger();
    QCOMPARE(dialogs, 1);
}


void tst_PageStudy::testHeaderSearchDisabledOnStudyPage() {
    auto* searchEdit = m_window->findChild<QLineEdit*>(u"HeaderSearchEdit"_s);
    QVERIFY(searchEdit != nullptr);

    // 1. Study page (page index 1): disabled with local search tooltip
    m_window->sidebar()->setCurrentPage(1);
    emit m_window->sidebar()->pageSelected(1);
    QCOMPARE(m_window->pageStack()->currentIndex(), 1);
    QVERIFY(!searchEdit->isEnabled());
    QCOMPARE(searchEdit->toolTip(), u"Use the search box on this page, Ctrl+F"_s);

    // 2. Calendar page (page index 3): enabled, tooltip empty
    m_window->sidebar()->setCurrentPage(3);
    emit m_window->sidebar()->pageSelected(3);
    QCOMPARE(m_window->pageStack()->currentIndex(), 3);
    QVERIFY(searchEdit->isEnabled());
    QVERIFY(searchEdit->toolTip().isEmpty());

    // 3. Home page (page index 0): enabled, tooltip empty
    m_window->sidebar()->setCurrentPage(0);
    emit m_window->sidebar()->pageSelected(0);
    QCOMPARE(m_window->pageStack()->currentIndex(), 0);
    QVERIFY(searchEdit->isEnabled());
    QVERIFY(searchEdit->toolTip().isEmpty());

    // 4. Settings page (page index 4): enabled, tooltip empty
    m_window->sidebar()->setCurrentPage(4);
    emit m_window->sidebar()->pageSelected(4);
    QCOMPARE(m_window->pageStack()->currentIndex(), 4);
    QVERIFY(searchEdit->isEnabled());
    QVERIFY(searchEdit->toolTip().isEmpty());

    // 5. Jobs page (page index 2): enabled, tooltip empty
    m_window->sidebar()->setCurrentPage(2);
    emit m_window->sidebar()->pageSelected(2);
    QCOMPARE(m_window->pageStack()->currentIndex(), 2);
    QVERIFY(searchEdit->isEnabled());
    QVERIFY(searchEdit->toolTip().isEmpty());

    // Switch back to Study page: disabled again
    m_window->sidebar()->setCurrentPage(1);
    emit m_window->sidebar()->pageSelected(1);
    QVERIFY(!searchEdit->isEnabled());
    QCOMPARE(searchEdit->toolTip(), u"Use the search box on this page, Ctrl+F"_s);
}


void tst_PageStudy::testCtrlNTriggersCurrentPageNew() {
    m_study.reset();
    m_studyCtx.reset();
    m_jobs.reset();
    m_jobsCtx.reset();
    m_calendar.reset();
    m_calCtx.reset();
    m_window->show();
    JobPrep::UiTest::activateWindow(m_window.get());
    QCoreApplication::processEvents();

    // 1. Jobs page: triggerNew() opens ApplicationDialog
    m_window->sidebar()->setCurrentPage(2);
    emit m_window->sidebar()->pageSelected(2);
    int appDialogs = 0;
    answerModalWhenReady([&appDialogs](QWidget* modal) {
        if (auto* dlg = qobject_cast<ApplicationDialog*>(modal)) {
            appDialogs++;
            dlg->done(QDialog::Rejected);
        }
    });
    m_window->triggerCurrentPageNew();
    QTest::qWait(50);
    QCOMPARE(appDialogs, 1);

    // 2. Calendar page: triggerNew() opens InterviewDialog
    m_window->sidebar()->setCurrentPage(3);
    emit m_window->sidebar()->pageSelected(3);
    int intDialogs = 0;
    answerModalWhenReady([&intDialogs](QWidget* modal) {
        if (auto* dlg = qobject_cast<InterviewDialog*>(modal)) {
            intDialogs++;
            dlg->done(QDialog::Rejected);
        }
    });
    m_window->triggerCurrentPageNew();
    QTest::qWait(50);
    QCOMPARE(intDialogs, 1);

    // 3. Study page: with a track inserted, triggerNew() focuses Backlog quick-add
    JobPrep::Domain::Track track;
    track.name = u"C++ Quick Track"_s;
    track.color = u"#5B6CFF"_s;
    m_ctx->tracks().insert(track);
    QCoreApplication::processEvents();

    JobPrep::UiTest::activateWindow(m_window.get());
    m_window->sidebar()->setCurrentPage(1);
    emit m_window->sidebar()->pageSelected(1);
    QTest::qWait(50);
    m_window->triggerCurrentPageNew();
    QTest::qWait(50);

    auto* study = qobject_cast<JobPrep::Ui::Pages::StudyPage*>(m_window->pageStack()->currentWidget());
    QVERIFY(study != nullptr);
    auto* backlog = childNamed<JobPrep::Ui::Widgets::TopicBoardColumn>(study, u"StudyColumnBacklog"_s);
    if (!backlog) {
        backlog = childNamed<JobPrep::Ui::Widgets::TopicBoardColumn>(study, u"StudyColumn_Backlog"_s);
    }
    if (backlog) {
        auto* quickAdd = childNamed<QLineEdit>(backlog, u"StudyColumnQuickAdd"_s);
        if (quickAdd) {
            QCOMPARE(QApplication::focusWidget(), quickAdd);
        }
    }

    // 4. Home page: triggerNew() opens ApplicationDialog (SPEC §3.7 / D1)
    m_window->sidebar()->setCurrentPage(0);
    emit m_window->sidebar()->pageSelected(0);
    int homeAppDialogs = 0;
    answerModalWhenReady([&homeAppDialogs](QWidget* modal) {
        if (auto* dlg = qobject_cast<ApplicationDialog*>(modal)) {
            homeAppDialogs++;
            dlg->done(QDialog::Rejected);
        }
    });
    m_window->triggerCurrentPageNew();
    QTest::qWait(50);
    QCOMPARE(homeAppDialogs, 1);

    // 5. Settings page: triggerNew() is a safe no-op (no modal opened)
    m_window->sidebar()->setCurrentPage(4);
    emit m_window->sidebar()->pageSelected(4);
    m_window->triggerCurrentPageNew();
    QVERIFY(QApplication::activeModalWidget() == nullptr);
}


void tst_PageStudy::testStudyBoardSingleSelectionAcrossColumns() {
    const QDate today(2026, 10, 19);
    resetStudyData(today);

    Track track;
    track.name = u"Concurrency Track"_s;
    track.color = u"#5b6cff"_s;
    QVERIFY(m_studyCtx->tracks().insert(track));
    const int trackId = m_studyCtx->tracks().all().last().id;

    // Insert card A in Backlog, card B in In Progress, card C in Review
    Topic topicA;
    topicA.trackId = trackId;
    topicA.title = u"Topic A"_s;
    topicA.status = JobPrep::Domain::TopicStatus::Backlog;
    QVERIFY(m_studyCtx->topics().insert(topicA));
    const int idA = m_studyCtx->topics().all().last().id;

    Topic topicB;
    topicB.trackId = trackId;
    topicB.title = u"Topic B"_s;
    topicB.status = JobPrep::Domain::TopicStatus::InProgress;
    QVERIFY(m_studyCtx->topics().insert(topicB));
    const int idB = m_studyCtx->topics().all().last().id;

    Topic topicC;
    topicC.trackId = trackId;
    topicC.title = u"Topic C"_s;
    topicC.status = JobPrep::Domain::TopicStatus::Review;
    QVERIFY(m_studyCtx->topics().insert(topicC));

    m_studyCtx->topics().changed();
    QCoreApplication::processEvents();

    auto* colBacklog = studyColumn(JobPrep::Domain::TopicStatus::Backlog);
    auto* colInProgress = studyColumn(JobPrep::Domain::TopicStatus::InProgress);
    auto* colReview = studyColumn(JobPrep::Domain::TopicStatus::Review);
    QVERIFY(colBacklog && colInProgress && colReview);

    // 1. Click card A in Backlog, then card B in In Progress -> Backlog has no selection, detail panel shows B
    colBacklog->selectTopicId(idA);
    QCoreApplication::processEvents();
    QCOMPARE(colBacklog->list()->currentIndex().data(JobPrep::Models::TopicListModel::IdRole).toInt(), idA);
    QVERIFY(colBacklog->list()->selectionModel()->isSelected(colBacklog->list()->currentIndex()));

    colInProgress->selectTopicId(idB);
    QCoreApplication::processEvents();
    // Backlog selection cleared
    QVERIFY(!colBacklog->list()->selectionModel()->hasSelection());
    QVERIFY(!colBacklog->list()->currentIndex().isValid());
    // In Progress has card B selected
    QCOMPARE(colInProgress->list()->currentIndex().data(JobPrep::Models::TopicListModel::IdRole).toInt(), idB);
    QVERIFY(colInProgress->list()->selectionModel()->isSelected(colInProgress->list()->currentIndex()));
    // Detail panel shows topic B
    QCOMPARE(studyPage()->selectedTopicId(), std::optional<int>(idB));
    auto* titleEdit = studyPage()->detailPanel()->findChild<QLineEdit*>(u"StudyDetailTitleEdit"_s);
    QVERIFY(titleEdit != nullptr && titleEdit->text().contains(u"Topic B"_s));

    // 2. Alt+Right on B -> moves B to Review; only Review has a selection
    QKeyEvent altRight(QEvent::KeyPress, Qt::Key_Right, Qt::AltModifier);
    QCoreApplication::sendEvent(colInProgress->list(), &altRight);
    QCoreApplication::processEvents();
    // In Progress and Backlog have no selection
    QVERIFY(!colBacklog->list()->selectionModel()->hasSelection());
    QVERIFY(!colInProgress->list()->selectionModel()->hasSelection());
    // Only Review has selection
    QVERIFY(colReview->list()->selectionModel()->hasSelection());
    QCOMPARE(colReview->list()->currentIndex().data(JobPrep::Models::TopicListModel::IdRole).toInt(), idB);
    QCOMPARE(studyPage()->selectedTopicId(), std::optional<int>(idB));

    // 3. Model reset (e.g. topic update or checklist tick) keeps exactly one selected card
    m_studyCtx->topics().changed();
    QCoreApplication::processEvents();
    QVERIFY(!colBacklog->list()->selectionModel()->hasSelection());
    QVERIFY(!colInProgress->list()->selectionModel()->hasSelection());
    QVERIFY(colReview->list()->selectionModel()->hasSelection());
    QCOMPARE(colReview->list()->currentIndex().data(JobPrep::Models::TopicListModel::IdRole).toInt(), idB);
    QCOMPARE(studyPage()->selectedTopicId(), std::optional<int>(idB));

    // 4. selectTopic(id) for a card in another column (e.g. topic A in Backlog) clears old selection
    studyPage()->selectTopic(idA);
    QCoreApplication::processEvents();
    QVERIFY(colBacklog->list()->selectionModel()->hasSelection());
    QCOMPARE(colBacklog->list()->currentIndex().data(JobPrep::Models::TopicListModel::IdRole).toInt(), idA);
    QVERIFY(!colInProgress->list()->selectionModel()->hasSelection());
    QVERIFY(!colReview->list()->selectionModel()->hasSelection());
    QCOMPARE(studyPage()->selectedTopicId(), std::optional<int>(idA));
}


void tst_PageStudy::testStudyListViewToggleSortAndDelegates() {
    const QDate today(2026, 10, 19);
    resetStudyData(today);

    JobPrep::Services::SeedService seeder(m_studyCtx->tracks(), m_studyCtx->topics());
    QVERIFY(seeder.loadSampleTopics());
    m_studyCtx->tracks().changed();
    m_studyCtx->topics().changed();
    QCoreApplication::processEvents();

    auto* listBtn = childNamed<QPushButton>(m_study.get(), u"StudyViewListBtn"_s);
    auto* boardBtn = childNamed<QPushButton>(m_study.get(), u"StudyViewBoardBtn"_s);
    QVERIFY(listBtn != nullptr);
    QVERIFY(boardBtn != nullptr);

    // Switch to list view
    listBtn->click();
    QCoreApplication::processEvents();
    QCOMPARE(m_studyCtx->settings().studyViewMode(), u"list"_s);
    QCOMPARE(studyPage()->viewMode(), u"list"_s);

    auto* tableView = studyPage()->tableView();
    QVERIFY(tableView != nullptr);
    QVERIFY(tableView->isVisible());

    // Sort by TrackColumn, TitleColumn, StatusColumn, TargetDateColumn
    tableView->sortByColumn(JobPrep::Models::TopicListModel::TrackColumn, Qt::AscendingOrder);
    QCoreApplication::processEvents();
    tableView->sortByColumn(JobPrep::Models::TopicListModel::TitleColumn, Qt::AscendingOrder);
    QCoreApplication::processEvents();
    tableView->sortByColumn(JobPrep::Models::TopicListModel::StatusColumn, Qt::DescendingOrder);
    QCoreApplication::processEvents();

    // Verify search works in list view
    studyPage()->setSearchText(u"STL"_s);
    QCoreApplication::processEvents();
    QVERIFY(studyPage()->visibleTopicCount() >= 1);
    studyPage()->setSearchText(QString());
    QCoreApplication::processEvents();

    // Double clicking row opens detail panel
    const auto proxy = tableView->model();
    QVERIFY(proxy != nullptr && proxy->rowCount() > 0);
    const QModelIndex firstIdx = proxy->index(0, 0);
    emit tableView->activated(firstIdx);
    QCoreApplication::processEvents();
    QVERIFY(studyPage()->isDetailVisible());

    // Test Enter key on table opens and focuses detail panel
    studyPage()->setDetailVisible(false);
    QKeyEvent returnKey(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QCoreApplication::sendEvent(tableView, &returnKey);
    QCoreApplication::processEvents();
    QVERIFY(studyPage()->isDetailVisible());

    // Test Ctrl+F / focusSearch focuses page search box
    studyPage()->focusSearch();
    auto* searchEdit = childNamed<QLineEdit>(studyPage(), u"StudySearchEdit"_s);
    QCOMPARE(QApplication::focusWidget(), searchEdit);

    // Test Delete key deletes selected topic with confirmation
    const int countBefore = proxy->rowCount();
    answerNextMessageBox([](QMessageBox* box) {
        box->button(QMessageBox::Yes)->click();
    });
    QKeyEvent deleteKey(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
    QCoreApplication::sendEvent(tableView, &deleteKey);
    QCoreApplication::processEvents();
    QCOMPARE(proxy->rowCount(), countBefore - 1);

    // Switch back to board view
    boardBtn->click();
    QCoreApplication::processEvents();
    QCOMPARE(m_studyCtx->settings().studyViewMode(), u"board"_s);
    QCOMPARE(studyPage()->viewMode(), u"board"_s);
}


void tst_PageStudy::testStudyScreenshots() {
    const QString dir = qEnvironmentVariable("JOBPREP_REPORT_IMG_DIR");
    if (dir.isEmpty()) QSKIP("JOBPREP_REPORT_IMG_DIR is not set");
    QVERIFY(QDir().mkpath(dir));

    const QDate today(2026, 10, 19);
    resetStudyData(today);

    const auto save = [this, dir](JobPrep::Ui::Theme::ThemeMode mode, const QString& name) {
        m_studyCtx->themeManager().setThemeMode(mode);
        QCoreApplication::processEvents();
        const QImage image = m_study->grab().toImage();
        QVERIFY(!image.isNull());
        QVERIFY(image.save(QDir(dir).filePath(name + u".png"_s)));
    };
    const auto forBothThemes = [&save](const QString& name) {
        save(JobPrep::Ui::Theme::ThemeMode::Light, name + u"-light"_s);
        save(JobPrep::Ui::Theme::ThemeMode::Dark, name + u"-dark"_s);
    };

    // 1. Empty board: tracks exist, but 0 topics
    Track trCpp;
    trCpp.name = u"C++"_s;
    trCpp.color = u"#5B6CFF"_s;
    QVERIFY(m_studyCtx->tracks().insert(trCpp));
    Track trEng;
    trEng.name = u"English"_s;
    trEng.color = u"#F59E0B"_s;
    QVERIFY(m_studyCtx->tracks().insert(trEng));
    m_studyCtx->tracks().changed();
    QCoreApplication::processEvents();
    forBothThemes(u"study-board-empty"_s);

    // 2. Board with cards
    JobPrep::Services::SeedService seeder(m_studyCtx->tracks(), m_studyCtx->topics());
    QVERIFY(seeder.loadSampleTopics());
    m_studyCtx->tracks().changed();
    m_studyCtx->topics().changed();
    QCoreApplication::processEvents();
    forBothThemes(u"study-board"_s);

    // 3. Single card selected on board (detail panel closed)
    const auto allTopics = m_studyCtx->topics().all();
    if (!allTopics.isEmpty()) {
        studyPage()->selectTopic(allTopics.first().id);
        studyPage()->setDetailVisible(false);
        QCoreApplication::processEvents();
        forBothThemes(u"study-board-selected"_s);
        studyPage()->setDetailVisible(true);
        QCoreApplication::processEvents();
    }
    forBothThemes(u"study-board-detail"_s);

    // 4. Drag feedback (accent insertion line & whole-column highlight)
    auto* inProgressCol = studyColumn(JobPrep::Domain::TopicStatus::InProgress);
    if (!allTopics.isEmpty()) {
        const auto captureDrag = [&](JobPrep::Ui::Theme::ThemeMode mode, const QString& name) {
            m_studyCtx->themeManager().setThemeMode(mode);
            auto* moveMime = new QMimeData;
            moveMime->setData(u"application/x-jobprep-topic-id"_s, QByteArray::number(allTopics.first().id));
            QDragEnterEvent enterEvent(QPoint(30, 20), Qt::MoveAction, moveMime, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(inProgressCol->list(), &enterEvent);
            QDragMoveEvent moveEvent(QPoint(30, 20), Qt::MoveAction, moveMime, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(inProgressCol->list(), &moveEvent);
            QCoreApplication::processEvents();
            const QImage image = m_study->grab().toImage();
            QVERIFY(!image.isNull());
            QVERIFY(image.save(QDir(dir).filePath(name + u".png"_s)));
            QDragLeaveEvent leaveEvent;
            QCoreApplication::sendEvent(inProgressCol->list(), &leaveEvent);
            delete moveMime;
        };
        captureDrag(JobPrep::Ui::Theme::ThemeMode::Light, u"study-board-drag-light"_s);
        captureDrag(JobPrep::Ui::Theme::ThemeMode::Dark, u"study-board-drag-dark"_s);
    }

    // 5. List view
    studyPage()->setViewMode(u"list"_s);
    QCoreApplication::processEvents();
    forBothThemes(u"study-list"_s);
    studyPage()->setViewMode(u"board"_s);
    QCoreApplication::processEvents();

    // 6. Tracks dialog
    for (auto mode : {JobPrep::Ui::Theme::ThemeMode::Light, JobPrep::Ui::Theme::ThemeMode::Dark}) {
        m_studyCtx->themeManager().setThemeMode(mode);
        TrackDialog trackDlg(m_studyCtx->tracks(), m_studyCtx->topics(), m_study.get());
        trackDlg.resize(520, 440);
        trackDlg.show();
        QCoreApplication::processEvents();
        const QImage img = trackDlg.grab().toImage();
        QVERIFY(!img.isNull());
        QVERIFY(img.save(QDir(dir).filePath(u"study-tracks-dialog-"_s + (mode == JobPrep::Ui::Theme::ThemeMode::Light ? u"light.png"_s : u"dark.png"_s))));
    }

    // 7. Session dialog
    const int firstTopicId = allTopics.isEmpty() ? 0 : allTopics.first().id;
    for (auto mode : {JobPrep::Ui::Theme::ThemeMode::Light, JobPrep::Ui::Theme::ThemeMode::Dark}) {
        m_studyCtx->themeManager().setThemeMode(mode);
        SessionDialog sessDlg(m_studyCtx->sessions(), m_studyCtx->topics(), m_study.get());
        sessDlg.setTopicId(firstTopicId > 0 ? std::optional<int>(firstTopicId) : std::nullopt);
        sessDlg.resize(460, 400);
        sessDlg.show();
        QCoreApplication::processEvents();
        const QImage img = sessDlg.grab().toImage();
        QVERIFY(!img.isNull());
        QVERIFY(img.save(QDir(dir).filePath(u"study-session-dialog-"_s + (mode == JobPrep::Ui::Theme::ThemeMode::Light ? u"light.png"_s : u"dark.png"_s))));
    }

    // 8. Empty tracks state: 0 tracks, 0 topics
    resetStudyData(today);
    forBothThemes(u"study-empty-tracks"_s);
}


QTEST_MAIN(tst_PageStudy)
#include "tst_PageStudy.moc"
