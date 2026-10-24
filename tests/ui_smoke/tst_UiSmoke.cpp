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
#include "services/SeedService.h"
#include "services/SettingsService.h"
#include "services/StatsService.h"
#include "ui/DashboardController.h"
#include "ui/FormatHelper.h"
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
#include <QToolTip>
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

using namespace Qt::StringLiterals;

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

QStringList s_capturedWarnings;

/// The offscreen platform plugin cannot forward window size hints, so Qt logs this once
/// per shown dialog. It is a limitation of the test platform plugin, not of our code,
/// so it must not fail the zero-warning gate.
/// The offscreen platform plugin cannot forward window size hints or raise windows,
/// so Qt logs this once per shown dialog and on QToolTip::showText (which calls QTipLabel::raise()).
/// It is a limitation of the test platform plugin, not of our code,
/// so it must not fail the zero-warning gate.
bool isPlatformPluginNoise(const QString& msg) {
    return msg.contains(u"does not support propagateSizeHints"_s) ||
           msg.contains(u"does not support raise"_s);
}

void testMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    Q_UNUSED(context);
    if (type != QtWarningMsg && type != QtCriticalMsg && type != QtFatalMsg) return;
    if (isPlatformPluginNoise(msg)) return;
    s_capturedWarnings.append(msg);
}

/// Runs `action` against the next modal widget once `exec()`'s event loop spins, so
/// production code keeps its normal modal flow while the test still clicks something.
template <typename Action>
void withNextModal(Action action) {
    QTimer::singleShot(0, [action]() {
        if (auto* modal = qobject_cast<QWidget*>(QApplication::activeModalWidget())) action(modal);
    });
}

template <typename Action>
void answerNextMessageBox(Action action) {
    withNextModal([action](QWidget* modal) {
        if (auto* box = qobject_cast<QMessageBox*>(modal)) action(box);
    });
}

/// Answers the next modal widget, polling until one appears. Required when the trigger is
/// a synthesized mouse event (QTest spins the event loop before showing the modal, so a
/// one-shot timer would fire too early and miss it).
template <typename Action>
void answerModalWhenReady(Action action) {
    auto* timer = new QTimer;
    timer->setInterval(5);
    QObject::connect(timer, &QTimer::timeout, timer, [timer, action]() {
        if (auto* modal = qobject_cast<QWidget*>(QApplication::activeModalWidget())) {
            timer->stop();
            timer->deleteLater();
            action(modal);
        }
    });
    timer->start();
}

template <typename T>
T* childNamed(QWidget* root, const QString& objectName) {
    return root->findChild<T*>(objectName);
}

QPushButton* buttonNamed(QWidget* root, const QString& objectName) {
    return childNamed<QPushButton>(root, objectName);
}

/// Top-level ApplicationDialog widgets, used to prove an entry point opens exactly one.
int countApplicationDialogs() {
    int count = 0;
    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget* widget : widgets) {
        if (qobject_cast<ApplicationDialog*>(widget)) ++count;
    }
    return count;
}

/// Top-level InterviewDialog widgets, used to prove an entry point opens exactly one.
int countInterviewDialogs() {
    int count = 0;
    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget* widget : widgets) {
        if (qobject_cast<InterviewDialog*>(widget)) ++count;
    }
    return count;
}

/// Top-level SessionDialog widgets.
int countSessionDialogs() {
    int count = 0;
    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget* widget : widgets) {
        if (qobject_cast<SessionDialog*>(widget)) ++count;
    }
    return count;
}

/// Top-level TrackDialog widgets.
int countTrackDialogs() {
    int count = 0;
    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget* widget : widgets) {
        if (qobject_cast<TrackDialog*>(widget)) ++count;
    }
    return count;
}

}  // namespace

class tst_UiSmoke : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testWindowCreationAndNavigation();
    void testThemeAndAccentSwitching();
    void testSidebarCollapsing();
    void testJobsPageEmptyStates();
    void testJobsPageAddAndEdit();
    void testJobsPageDuplicateAndDelete();
    void testJobsPageStatusChangeAppearsInTimeline();
    void testJobsPageSortAndFilter();
    void testJobsPageCsvExport();
    void testJobsPageSelectionKeptAcrossReloads();
    void testApplicationDialogTabs();
    void testApplicationDialogValidation();
    void testApplicationDialogCancelKeepsEditing();
    void testInterviewDialogCreateAndEdit();
    void testInterviewDialogValidation();
    void testInterviewDialogDeleteWithConfirmation();
    void testInterviewDialogKeyboardShortcuts();
    void testInterviewTabDisabledUntilSaved();
    void testApplicationDialogInterviewsTabEditable();
    void testHeaderNewInterviewFromJobsRoutesToCalendar();
    void testJobsPageThemes();
    void testHeaderSearchFiltersJobsTable();
    void testNewEntryPointsOpenOneDialog();
    void testHeaderNewApplicationOnCalendarPage();
    void testJobsContextMenuAddInterviewEndToEnd();
    void testCalendarPageMarkersAndAgenda();
    void testCalendarLayerChipsFilterAgenda();
    void testCalendarNewInterviewFromDay();
    void testCalendarFollowUpEndToEnd();
    void testCalendarFollowUpOverwritePrompt();
    void testCalendarSplitterStatePersisted();
    void testCalendarThemes();
    void testCalendarScreenshots();
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
    void testCalendarJumpToStudyTopic();
    void testCalendarTopicDateAndTrackRename();
    void testCalendarTrackDeleteClearsMarkers();
    void testMonthViewDoubleClicksDayWithoutOpeningStudyTarget();
    void testCtrlNTriggersCurrentPageNew();
    void testStudyBoardSingleSelectionAcrossColumns();
    void testStudyListViewToggleSortAndDelegates();
    void testStudyScreenshots();
    void testFormatHelper();
    void testDashboardVisitedBothThemes();
    void testDashboardNumbersMatchHandCalculatedDataset();
    void testDashboardEmptyStateActionButtons();
    void testDashboardTodayItemClicksOpenRespectiveDialogs();
    void testOpenInterviewAndApplicationFromAnotherPage();
    void testDashboardDayChangeRefreshesTodayAndStreak();
    void testBurstChangesCoalesceToOneRefresh();
    void testWeeklyGoalChangeUpdatesBarAndTextImmediately();
    void testFirstDayOfWeekChangeRefreshesWeekNumbers();
    void testStatusBarTextScenarios();
    void testWidgetMetricsAndRingWrapping();
    void testDashboardIgnoresThemeSidebarAndSplitterChanges();
    void testDashboardScreenshots();
    void testZeroWarnings();
    void cleanupTestCase();

private:
    /// Rebuilds an isolated context plus a standalone JobsPage, so every acceptance slot
    /// starts from a known database.
    void resetJobsData();
    JobPrep::Ui::Pages::JobsPage* jobsPage() { return m_jobs.get(); }
    QTableView* jobsTable() const;
    void addApplication(const QString& company, const QString& position,
                        ApplicationStatus status = ApplicationStatus::Applied,
                        const QString& notes = QString());
    QString companyAt(int row) const;
    int idAt(int row) const;
    /// Fills and saves the next modal ApplicationDialog, exactly as the user would.
    void saveNextDialog(const QString& company, const QString& position);
    void clickChip(int index);

    /// Rebuilds an isolated context plus a standalone CalendarPage with a fixed "today",
    /// so marker/agenda assertions never depend on the real clock.
    void resetCalendarData(const QDate& today);
    JobPrep::Ui::Pages::CalendarPage* calendarPage() { return m_calendar.get(); }
    JobPrep::Ui::Widgets::MonthView* monthView() const;
    QListView* agendaList() const;
    /// Inserts an application plus one interview on `day`; returns the application id.
    int addApplicationWithInterview(const QString& company, const QDate& day, int hour);
    /// Inserts a track and one non-done topic targeting `day`; returns the track id.
    int addStudyTarget(const QDate& day);

    /// Rebuilds an isolated context plus a standalone StudyPage with a fixed "today".
    void resetStudyData(const QDate& today = QDate(2026, 10, 19));
    JobPrep::Ui::Pages::StudyPage* studyPage() { return m_study.get(); }
    JobPrep::Ui::Widgets::TopicBoardColumn* studyColumn(JobPrep::Domain::TopicStatus status) const;

    std::unique_ptr<JobPrep::App::AppContext> m_ctx;
    std::unique_ptr<JobPrep::Ui::MainWindow> m_window;
    std::unique_ptr<JobPrep::App::AppContext> m_jobsCtx;
    std::unique_ptr<JobPrep::Ui::Pages::JobsPage> m_jobs;
    std::unique_ptr<JobPrep::App::AppContext> m_calCtx;
    std::unique_ptr<JobPrep::Ui::Pages::CalendarPage> m_calendar;
    std::unique_ptr<JobPrep::App::AppContext> m_studyCtx;
    std::unique_ptr<JobPrep::Ui::Pages::StudyPage> m_study;
};

void tst_UiSmoke::initTestCase() {
    qInstallMessageHandler(testMessageHandler);

    QCoreApplication::setOrganizationName(u"JobPrepTest"_s);
    QCoreApplication::setApplicationName(u"JobPrepTest"_s);

    // An in-memory database keeps the smoke test away from the user's data folder.
    m_ctx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    QVERIFY(m_ctx->database().isOpen());
    m_window = std::make_unique<JobPrep::Ui::MainWindow>(*m_ctx);
    m_window->show();
}

void tst_UiSmoke::testWindowCreationAndNavigation() {
    QVERIFY(m_window != nullptr);
    QVERIFY(m_window->pageStack() != nullptr);
    QCOMPARE(m_window->pageStack()->count(), 5);

    // Initial page is Dashboard (0)
    QCOMPARE(m_window->pageStack()->currentIndex(), 0);

    // Walk through all pages
    for (int i = 0; i < 5; ++i) {
        m_window->sidebar()->setCurrentPage(i);
        emit m_window->sidebar()->pageSelected(i);
        QCOMPARE(m_window->pageStack()->currentIndex(), i);
        QVERIFY(m_window->pageStack()->currentWidget() != nullptr);
    }

    // Return to first page
    m_window->sidebar()->setCurrentPage(0);
    emit m_window->sidebar()->pageSelected(0);
    QCOMPARE(m_window->pageStack()->currentIndex(), 0);
}

void tst_UiSmoke::testThemeAndAccentSwitching() {
    auto& themeMgr = m_ctx->themeManager();

    // Toggle Light mode
    themeMgr.setThemeMode(JobPrep::Ui::Theme::ThemeMode::Light);
    QCOMPARE(themeMgr.currentMode(), JobPrep::Ui::Theme::ThemeMode::Light);
    QVERIFY(!themeMgr.isDark());

    // Toggle Dark mode
    themeMgr.setThemeMode(JobPrep::Ui::Theme::ThemeMode::Dark);
    QCOMPARE(themeMgr.currentMode(), JobPrep::Ui::Theme::ThemeMode::Dark);
    QVERIFY(themeMgr.isDark());

    // Cycle through all 6 accent presets
    const JobPrep::Ui::Theme::AccentPreset accents[] = {
        JobPrep::Ui::Theme::AccentPreset::Indigo,
        JobPrep::Ui::Theme::AccentPreset::Teal,
        JobPrep::Ui::Theme::AccentPreset::Rose,
        JobPrep::Ui::Theme::AccentPreset::Amber,
        JobPrep::Ui::Theme::AccentPreset::Emerald,
        JobPrep::Ui::Theme::AccentPreset::Sky,
    };

    for (auto accent : accents) {
        themeMgr.setAccentPreset(accent);
        QCOMPARE(themeMgr.currentAccent(), accent);
        const auto tokens = themeMgr.currentTokens();
        QVERIFY(tokens.accent.isValid());
        QVERIFY(tokens.bg.isValid());
        QVERIFY(tokens.surface.isValid());
    }
}

void tst_UiSmoke::testSidebarCollapsing() {
    auto* sidebar = m_window->sidebar();
    QVERIFY(sidebar != nullptr);

    // Test collapse
    sidebar->setCollapsed(true);
    QVERIFY(sidebar->isCollapsed());
    QCOMPARE(sidebar->width(), JobPrep::Ui::Sidebar::kWidthCollapsed);

    // Test expand
    sidebar->setCollapsed(false);
    QVERIFY(!sidebar->isCollapsed());
    QCOMPARE(sidebar->width(), JobPrep::Ui::Sidebar::kWidthExpanded);
}

void tst_UiSmoke::resetJobsData() {
    m_jobs.reset();
    m_jobsCtx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    QVERIFY(m_jobsCtx->database().isOpen());
    m_jobs = std::make_unique<JobPrep::Ui::Pages::JobsPage>(m_jobsCtx->applications(),
                                                           m_jobsCtx->interviews(),
                                                           m_jobsCtx->exportService());
    m_jobs->show();
}

QTableView* tst_UiSmoke::jobsTable() const {
    return childNamed<QTableView>(m_jobs.get(), u"JobsTableView"_s);
}

void tst_UiSmoke::addApplication(const QString& company, const QString& position,
                                 ApplicationStatus status, const QString& notes) {
    JobApplication app;
    app.company = company;
    app.position = position;
    app.status = status;
    app.notes = notes;
    QVERIFY(m_jobsCtx->applications().insert(app));
}

QString tst_UiSmoke::companyAt(int row) const {
    return jobsTable()->model()->index(row, ApplicationTableModel::CompanyColumn).data().toString();
}

int tst_UiSmoke::idAt(int row) const {
    return jobsTable()->model()
        ->index(row, ApplicationTableModel::CompanyColumn)
        .data(ApplicationTableModel::IdRole)
        .toInt();
}

void tst_UiSmoke::saveNextDialog(const QString& company, const QString& position) {
    withNextModal([company, position](QWidget* modal) {
        auto* dialog = qobject_cast<ApplicationDialog*>(modal);
        if (!dialog) return;
        childNamed<QLineEdit>(dialog, u"FieldCompany"_s)->setText(company);
        childNamed<QLineEdit>(dialog, u"FieldPosition"_s)->setText(position);
        buttonNamed(dialog, u"DialogSaveBtn"_s)->click();
    });
}

void tst_UiSmoke::clickChip(int index) {
    auto* chips = childNamed<JobPrep::Ui::Widgets::ChipBar>(m_jobs.get(), u"JobsStatusChips"_s);
    QVERIFY(chips != nullptr);
    const auto buttons = chips->findChildren<QPushButton*>();
    QCOMPARE(buttons.size(), 5);
    buttons.at(index)->click();
}

void tst_UiSmoke::resetCalendarData(const QDate& today) {
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

JobPrep::Ui::Widgets::MonthView* tst_UiSmoke::monthView() const {
    return childNamed<JobPrep::Ui::Widgets::MonthView>(m_calendar.get(), u"CalendarMonthView"_s);
}

QListView* tst_UiSmoke::agendaList() const {
    return childNamed<QListView>(m_calendar.get(), u"CalendarAgendaList"_s);
}

int tst_UiSmoke::addApplicationWithInterview(const QString& company, const QDate& day, int hour) {
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

int tst_UiSmoke::addStudyTarget(const QDate& day) {
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

void tst_UiSmoke::resetStudyData(const QDate& today) {
    m_study.reset();
    m_studyCtx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    QVERIFY(m_studyCtx->database().isOpen());

    m_studyCtx->settings().setStudyTrackFilter(-1);

    m_study = std::make_unique<JobPrep::Ui::Pages::StudyPage>(
        m_studyCtx->topics(), m_studyCtx->tracks(), m_studyCtx->sessions(), m_studyCtx->settings(),
        m_studyCtx->themeManager(), nullptr, [today]() { return today; });
    m_study->resize(1200, 720);
    m_study->show();
    QCoreApplication::processEvents();
}

JobPrep::Ui::Widgets::TopicBoardColumn* tst_UiSmoke::studyColumn(JobPrep::Domain::TopicStatus status) const {
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

void tst_UiSmoke::testJobsPageEmptyStates() {
    resetJobsData();
    // 0 = no data, 1 = table, 2 = no matches.
    QCOMPARE(jobsPage()->currentViewIndex(), 0);
    QCOMPARE(jobsPage()->visibleRowCount(), 0);

    addApplication(u"Acme"_s, u"C++ Developer"_s);
    QCOMPARE(jobsPage()->currentViewIndex(), 1);
    QCOMPARE(jobsPage()->visibleRowCount(), 1);

    // A search nothing can match shows "no matches", not the empty database state.
    jobsPage()->setSearchText(u"zzz-no-such-company"_s);
    QCOMPARE(jobsPage()->currentViewIndex(), 2);
    QCOMPARE(jobsPage()->visibleRowCount(), 0);

    jobsPage()->setSearchText(QString());
    QCOMPARE(jobsPage()->currentViewIndex(), 1);
    QCOMPARE(jobsPage()->visibleRowCount(), 1);
}

void tst_UiSmoke::testJobsPageAddAndEdit() {
    resetJobsData();
    saveNextDialog(u"Northwind"_s, u"Senior Qt Engineer"_s);
    jobsPage()->newApplication();
    QCOMPARE(m_jobsCtx->applications().count(), 1);
    QCOMPARE(jobsPage()->visibleRowCount(), 1);
    QCOMPARE(companyAt(0), u"Northwind"_s);
    const int id = idAt(0);

    // Enter on the selected row opens the editor.
    auto* view = jobsTable();
    view->setCurrentIndex(view->model()->index(0, ApplicationTableModel::CompanyColumn));
    saveNextDialog(u"Northwind GmbH"_s, u"Senior Qt Engineer"_s);
    QTest::keyClick(view, Qt::Key_Return);

    QCOMPARE(companyAt(0), u"Northwind GmbH"_s);
    QCOMPARE(m_jobsCtx->applications().byId(id)->company, u"Northwind GmbH"_s);
    QCOMPARE(jobsPage()->visibleRowCount(), 1);
}

void tst_UiSmoke::testJobsPageDuplicateAndDelete() {
    resetJobsData();
    addApplication(u"Globex"_s, u"Backend Engineer"_s);
    const int id = m_jobsCtx->applications().all().first().id;

    JobApplication copy;
    QVERIFY(m_jobsCtx->applications().duplicate(id, copy));
    QCOMPARE(m_jobsCtx->applications().count(), 2);
    QCOMPARE(jobsPage()->visibleRowCount(), 2);

    // Declining the confirmation keeps the row.
    answerNextMessageBox([](QMessageBox* box) { box->button(QMessageBox::No)->click(); });
    jobsPage()->deleteApplication(id);
    QCOMPARE(m_jobsCtx->applications().count(), 2);
    QCOMPARE(jobsPage()->visibleRowCount(), 2);

    // Accepting removes it from the database and the table.
    answerNextMessageBox([](QMessageBox* box) { box->button(QMessageBox::Yes)->click(); });
    jobsPage()->deleteApplication(id);
    QCOMPARE(m_jobsCtx->applications().count(), 1);
    QCOMPARE(jobsPage()->visibleRowCount(), 1);
    QCOMPARE(companyAt(0), u"Globex"_s);
}

void tst_UiSmoke::testJobsPageStatusChangeAppearsInTimeline() {
    resetJobsData();
    addApplication(u"Initech"_s, u"Systems Programmer"_s);
    const int id = m_jobsCtx->applications().all().first().id;
    QCOMPARE(m_jobsCtx->applications().statusHistory(id).size(), 1);

    jobsPage()->changeStatus(id, ApplicationStatus::Offer);
    QCOMPARE(m_jobsCtx->applications().byId(id)->status, ApplicationStatus::Offer);
    QCOMPARE(m_jobsCtx->applications().statusHistory(id).size(), 2);

    ApplicationDialog dialog(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                             ApplicationDialog::Edit);
    dialog.setApplication(*m_jobsCtx->applications().byId(id));
    auto* tabs = childNamed<QTabWidget>(&dialog, u"ApplicationDialogTabs"_s);
    QVERIFY(tabs != nullptr);
    QCOMPARE(tabs->count(), 4);
    tabs->setCurrentIndex(3);
    auto* timeline = childNamed<QTableWidget>(&dialog, u"TimelineTable"_s);
    QVERIFY(timeline != nullptr);
    QCOMPARE(timeline->rowCount(), 2);
    QCOMPARE(timeline->item(0, 1)->text(), u"—"_s);
    QCOMPARE(timeline->item(1, 1)->text(), u"Applied"_s);
    QCOMPARE(timeline->item(1, 2)->text(), u"Offer"_s);
}

void tst_UiSmoke::testJobsPageSortAndFilter() {
    resetJobsData();
    addApplication(u"Bravo"_s, u"Analyst"_s);
    addApplication(u"Alpha"_s, u"Designer"_s, ApplicationStatus::Rejected);
    addApplication(u"Cesar"_s, u"Tester"_s, ApplicationStatus::Technical,
                   u"приглашение на интервью"_s);

    auto* view = jobsTable();
    QCOMPARE(view->model()->rowCount(), 3);

    // A header click flips the company order.
    view->sortByColumn(ApplicationTableModel::CompanyColumn, Qt::AscendingOrder);
    QCOMPARE(companyAt(0), u"Alpha"_s);
    view->sortByColumn(ApplicationTableModel::CompanyColumn, Qt::DescendingOrder);
    QCOMPARE(companyAt(0), u"Cesar"_s);

    // The Closed chip keeps the rejected row and hides the rest.
    clickChip(4);
    QCOMPARE(jobsPage()->visibleRowCount(), 1);
    QCOMPARE(companyAt(0), u"Alpha"_s);

    // The search box reaches the notes column too.
    clickChip(0);
    QCOMPARE(jobsPage()->visibleRowCount(), 3);
    jobsPage()->setSearchText(u"ИНТЕРВЬЮ"_s);
    QCOMPARE(jobsPage()->visibleRowCount(), 1);
    QCOMPARE(companyAt(0), u"Cesar"_s);
}

void tst_UiSmoke::testJobsPageCsvExport() {
    resetJobsData();
    addApplication(u"Umbrella"_s, u"Lead"_s);
    addApplication(u"Wyco"_s, u"Intern"_s, ApplicationStatus::Wishlist);
    addApplication(u"Alpha"_s, u"Designer"_s);

    // Sorting by company makes the expected export order deterministic.
    auto* view = jobsTable();
    view->sortByColumn(ApplicationTableModel::CompanyColumn, Qt::AscendingOrder);
    const QStringList visibleOrder = {companyAt(0), companyAt(1), companyAt(2)};
    QCOMPARE(visibleOrder, QStringList({u"Alpha"_s, u"Umbrella"_s, u"Wyco"_s}));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(u"jobs.csv"_s);
    int exportedRows = -1;
    QString error;
    QVERIFY2(jobsPage()->exportTo(path, &exportedRows, &error), qPrintable(error));
    QCOMPARE(exportedRows, 3);

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray raw = file.readAll();
    // The UTF-8 BOM is required for spreadsheets, so check the bytes themselves:
    // QString::fromUtf8 would swallow it.
    QVERIFY(raw.startsWith(QByteArray("\xEF\xBB\xBF", 3)));
    const QStringList lines = QString::fromUtf8(raw).split(u"\r\n"_s, Qt::SkipEmptyParts);
    QCOMPARE(lines.size(), 4);
    QCOMPARE(lines.first(), JobPrep::Services::ExportService::headers().join(u","_s));

    // Data rows follow the visible order.
    for (int row = 0; row < visibleOrder.size(); ++row) {
        QVERIFY(lines.at(row + 1).startsWith(visibleOrder.at(row) + u","_s));
    }

    // With everything filtered out the file still carries the header row.
    jobsPage()->setSearchText(u"zzz-no-such-company"_s);
    const QString emptyPath = dir.filePath(u"empty.csv"_s);
    exportedRows = -1;
    QVERIFY2(jobsPage()->exportTo(emptyPath, &exportedRows, &error), qPrintable(error));
    QCOMPARE(exportedRows, 0);
    QVERIFY(QFile::exists(emptyPath));
    QFile emptyFile(emptyPath);
    QVERIFY(emptyFile.open(QIODevice::ReadOnly));
    const QStringList emptyLines =
        QString::fromUtf8(emptyFile.readAll()).split(u"\r\n"_s, Qt::SkipEmptyParts);
    QCOMPARE(emptyLines.size(), 1);
    QCOMPARE(emptyLines.first(), JobPrep::Services::ExportService::headers().join(u","_s));
}

void tst_UiSmoke::testJobsPageSelectionKeptAcrossReloads() {
    resetJobsData();
    addApplication(u"One"_s, u"First"_s);
    addApplication(u"Two"_s, u"Second"_s);
    addApplication(u"Three"_s, u"Third"_s);

    auto* view = jobsTable();
    view->sortByColumn(ApplicationTableModel::CompanyColumn, Qt::AscendingOrder);
    // Ascending by company gives One, Three, Two.
    QCOMPARE(companyAt(2), u"Two"_s);
    view->setCurrentIndex(view->model()->index(2, ApplicationTableModel::CompanyColumn));
    const int selectedId = idAt(2);

    // A repository write resets the model; the same row must come back selected.
    addApplication(u"Zero"_s, u"Inserted"_s);
    QCOMPARE(view->currentIndex().data(ApplicationTableModel::IdRole).toInt(), selectedId);
    QCOMPARE(view->currentIndex().data().toString(), u"Two"_s);
    QCOMPARE(view->model()->rowCount(), 4);
}

void tst_UiSmoke::testApplicationDialogTabs() {
    resetJobsData();
    addApplication(u"Umbrella"_s, u"Lead"_s);
    const int id = m_jobsCtx->applications().all().first().id;

    ApplicationDialog edit(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                           ApplicationDialog::Edit);
    edit.setApplication(*m_jobsCtx->applications().byId(id));
    auto* tabs = childNamed<QTabWidget>(&edit, u"ApplicationDialogTabs"_s);
    QVERIFY(tabs != nullptr);
    QCOMPARE(tabs->count(), 4);
    QCOMPARE(tabs->tabText(0), u"Overview"_s);
    QCOMPARE(tabs->tabText(1), u"Notes"_s);
    QCOMPARE(tabs->tabText(2), u"Interviews"_s);
    QCOMPARE(tabs->tabText(3), u"Timeline"_s);
    QVERIFY(tabs->isTabEnabled(2));
    QCOMPARE(childNamed<QLineEdit>(&edit, u"FieldCompany"_s)->text(), u"Umbrella"_s);
    QCOMPARE(childNamed<QTextEdit>(&edit, u"FieldNotes"_s)->toPlainText(), QString());
    QVERIFY(!edit.isDirty());

    // A brand new application has no row to list, so those tabs are disabled and empty.
    ApplicationDialog create(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                             ApplicationDialog::Create);
    auto* createTabs = childNamed<QTabWidget>(&create, u"ApplicationDialogTabs"_s);
    QVERIFY(!createTabs->isTabEnabled(2));
    QVERIFY(!createTabs->isTabEnabled(3));
    QCOMPARE(childNamed<QTableWidget>(&create, u"TimelineTable"_s)->rowCount(), 0);
}

void tst_UiSmoke::testApplicationDialogValidation() {
    resetJobsData();
    ApplicationDialog dialog(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                             ApplicationDialog::Create);

    // Company and position are required.
    buttonNamed(&dialog, u"DialogSaveBtn"_s)->click();
    QVERIFY(!dialog.validationMessage().isEmpty());
    QCOMPARE(m_jobsCtx->applications().count(), 0);

    // Salary min above max blocks the save.
    childNamed<QLineEdit>(&dialog, u"FieldCompany"_s)->setText(u"Acme"_s);
    childNamed<QLineEdit>(&dialog, u"FieldPosition"_s)->setText(u"Developer"_s);
    childNamed<QSpinBox>(&dialog, u"FieldSalaryMin"_s)->setValue(9000);
    childNamed<QSpinBox>(&dialog, u"FieldSalaryMax"_s)->setValue(1000);
    buttonNamed(&dialog, u"DialogSaveBtn"_s)->click();
    QCOMPARE(dialog.validationMessage(), u"Salary min cannot be greater than salary max."_s);
    QCOMPARE(m_jobsCtx->applications().count(), 0);

    // A malformed e-mail only warns, so the save goes through.
    childNamed<QSpinBox>(&dialog, u"FieldSalaryMin"_s)->setValue(1000);
    childNamed<QSpinBox>(&dialog, u"FieldSalaryMax"_s)->setValue(9000);
    childNamed<QLineEdit>(&dialog, u"FieldContactEmail"_s)->setText(u"not-an-address"_s);
    buttonNamed(&dialog, u"DialogSaveBtn"_s)->click();
    QCOMPARE(dialog.validationMessage(),
             u"Warning: the contact e-mail does not look like an address."_s);
    QCOMPARE(m_jobsCtx->applications().count(), 1);
    QCOMPARE(m_jobsCtx->applications().all().first().status, ApplicationStatus::Applied);
}

void tst_UiSmoke::testApplicationDialogCancelKeepsEditing() {
    resetJobsData();
    addApplication(u"Umbrella"_s, u"Lead"_s);
    const int id = m_jobsCtx->applications().all().first().id;

    ApplicationDialog dialog(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                             ApplicationDialog::Edit);
    dialog.setApplication(*m_jobsCtx->applications().byId(id));
    dialog.show();
    childNamed<QLineEdit>(&dialog, u"FieldCompany"_s)->setText(u"Edited"_s);
    QVERIFY(dialog.isDirty());

    // Cancel means keep editing, so the dialog stays open with the edits intact.
    answerNextMessageBox([](QMessageBox* box) { box->button(QMessageBox::Cancel)->click(); });
    dialog.reject();
    QVERIFY(dialog.isVisible());
    QVERIFY(dialog.isDirty());
    QCOMPARE(childNamed<QLineEdit>(&dialog, u"FieldCompany"_s)->text(), u"Edited"_s);
    QCOMPARE(m_jobsCtx->applications().byId(id)->company, u"Umbrella"_s);

    // Discard really closes it, still without saving.
    answerNextMessageBox([](QMessageBox* box) { box->button(QMessageBox::Discard)->click(); });
    dialog.reject();
    QVERIFY(!dialog.isVisible());
    QCOMPARE(m_jobsCtx->applications().byId(id)->company, u"Umbrella"_s);
}

void tst_UiSmoke::testInterviewDialogCreateAndEdit() {
    resetJobsData();
    addApplication(u"Acme"_s, u"Developer"_s);
    addApplication(u"Globex"_s, u"Engineer"_s);
    const int appId = m_jobsCtx->applications().all().at(1).id;

    InterviewDialog create(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                           InterviewDialog::Create);
    create.show();
    QCOMPARE(countInterviewDialogs(), 1);

    auto* appCombo = childNamed<QComboBox>(&create, u"InterviewApplicationCombo"_s);
    QVERIFY(appCombo != nullptr);
    create.setApplicationId(appId);
    QCOMPARE(appCombo->currentData().toInt(), appId);

    auto* typeCombo = childNamed<QComboBox>(&create, u"InterviewTypeCombo"_s);
    typeCombo->setCurrentIndex(typeCombo->findData(static_cast<int>(InterviewType::SystemDesign)));
    childNamed<QDateEdit>(&create, u"InterviewDateEdit"_s)->setDate(QDate(2026, 5, 4));
    childNamed<QLineEdit>(&create, u"InterviewPlaceEdit"_s)->setText(u"Zoom"_s);
    QVERIFY(create.isDirty());

    buttonNamed(&create, u"DialogSaveBtn"_s)->click();
    QCOMPARE(m_jobsCtx->interviews().byApplication(appId).size(), 1);
    const auto saved = m_jobsCtx->interviews().byApplication(appId).first();
    QCOMPARE(saved.startAt.date(), QDate(2026, 5, 4));
    QCOMPARE(saved.type, InterviewType::SystemDesign);
    QCOMPARE(saved.place, u"Zoom"_s);
    create.close();

    // Editing loads the row, tracks dirty state and can be discarded unchanged.
    InterviewDialog edit(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                         InterviewDialog::Edit);
    edit.setInterview(*m_jobsCtx->interviews().byId(saved.id));
    edit.show();
    QVERIFY(!edit.isDirty());
    auto* placeEdit = childNamed<QLineEdit>(&edit, u"InterviewPlaceEdit"_s);
    QCOMPARE(placeEdit->text(), u"Zoom"_s);
    placeEdit->setText(u"Google Meet"_s);
    QVERIFY(edit.isDirty());

    answerNextMessageBox([](QMessageBox* box) { box->button(QMessageBox::Cancel)->click(); });
    edit.reject();
    QVERIFY(edit.isVisible());
    QVERIFY(edit.isDirty());
    QCOMPARE(m_jobsCtx->interviews().byId(saved.id)->place, u"Zoom"_s);

    answerNextMessageBox([](QMessageBox* box) { box->button(QMessageBox::Discard)->click(); });
    edit.reject();
    QVERIFY(!edit.isVisible());
}

void tst_UiSmoke::testInterviewDialogValidation() {
    resetJobsData();

    // With no applications the combo is empty, so an application is required.
    InterviewDialog noApps(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                           InterviewDialog::Create);
    buttonNamed(&noApps, u"DialogSaveBtn"_s)->click();
    QVERIFY(!noApps.validationMessage().isEmpty());
    QCOMPARE(m_jobsCtx->interviews().all().size(), 0);

    addApplication(u"Acme"_s, u"Developer"_s);
    const int appId = m_jobsCtx->applications().all().first().id;

    InterviewDialog dialog(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                           InterviewDialog::Create);
    dialog.setApplicationId(appId);
    childNamed<QSpinBox>(&dialog, u"InterviewDurationSpin"_s)->setValue(0);
    buttonNamed(&dialog, u"DialogSaveBtn"_s)->click();
    QCOMPARE(dialog.validationMessage(), u"Duration must be at least 1 minute."_s);
    QCOMPARE(m_jobsCtx->interviews().all().size(), 0);

    // A valid duration saves.
    childNamed<QSpinBox>(&dialog, u"InterviewDurationSpin"_s)->setValue(30);
    buttonNamed(&dialog, u"DialogSaveBtn"_s)->click();
    QCOMPARE(m_jobsCtx->interviews().all().size(), 1);
}

void tst_UiSmoke::testInterviewDialogDeleteWithConfirmation() {
    resetJobsData();
    addApplication(u"Acme"_s, u"Developer"_s);
    const int appId = m_jobsCtx->applications().all().first().id;

    Interview interview;
    interview.applicationId = appId;
    interview.startAt = QDateTime(QDate(2026, 5, 4), QTime(10, 0));
    QVERIFY(m_jobsCtx->interviews().insert(interview));

    InterviewDialog edit(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                         InterviewDialog::Edit);
    edit.setInterview(*m_jobsCtx->interviews().byId(interview.id));
    auto* deleteBtn = buttonNamed(&edit, u"DialogDeleteBtn"_s);
    QVERIFY(deleteBtn != nullptr);

    // Declining keeps the row.
    answerNextMessageBox([](QMessageBox* box) { box->button(QMessageBox::No)->click(); });
    deleteBtn->click();
    QCOMPARE(m_jobsCtx->interviews().all().size(), 1);

    // Accepting deletes it.
    answerNextMessageBox([](QMessageBox* box) { box->button(QMessageBox::Yes)->click(); });
    deleteBtn->click();
    QCOMPARE(m_jobsCtx->interviews().all().size(), 0);
}

void tst_UiSmoke::testInterviewDialogKeyboardShortcuts() {
    resetJobsData();
    addApplication(u"Acme"_s, u"Developer"_s);
    const int appId = m_jobsCtx->applications().all().first().id;

    // Esc cancels a clean form straight away (no discard prompt needed).
    InterviewDialog cancelDialog(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                                 InterviewDialog::Create);
    cancelDialog.show();
    cancelDialog.setFocus();
    QTest::keyClick(&cancelDialog, Qt::Key_Escape);
    QVERIFY(!cancelDialog.isVisible());

    // Save is the default button, so Enter accepts a valid form.
    InterviewDialog saveDialog(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                               InterviewDialog::Create);
    saveDialog.setApplicationId(appId);
    saveDialog.show();
    auto* saveBtn = buttonNamed(&saveDialog, u"DialogSaveBtn"_s);
    QVERIFY(saveBtn != nullptr);
    QVERIFY(saveBtn->isDefault());
    saveDialog.setFocus();
    QTest::keyClick(&saveDialog, Qt::Key_Return);
    QCOMPARE(m_jobsCtx->interviews().byApplication(appId).size(), 1);
    QVERIFY(!saveDialog.isVisible());
}

void tst_UiSmoke::testInterviewTabDisabledUntilSaved() {
    resetJobsData();

    // Create mode has no application id yet, so "+ Interview" is disabled and explains
    // why ("Save the application first..."); clicking it must not open a dialog or write
    // anything. This is the disabled case (C0 item 1), not a save-first-on-click case.
    ApplicationDialog create(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                             ApplicationDialog::Create);
    auto* addBtn = buttonNamed(&create, u"InterviewsAddBtn"_s);
    QVERIFY(addBtn != nullptr);
    QVERIFY(!addBtn->isEnabled());
    QVERIFY(addBtn->toolTip().contains(u"Save"_s));

    const int before = m_jobsCtx->interviews().all().size();
    addBtn->click();
    QCOMPARE(m_jobsCtx->interviews().all().size(), before);
    QCOMPARE(m_jobsCtx->applications().count(), 0);

    // Once saved, the same button is enabled.
    addApplication(u"Umbrella"_s, u"Lead"_s);
    const int appId = m_jobsCtx->applications().all().first().id;
    ApplicationDialog edit(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                           ApplicationDialog::Edit);
    edit.setApplication(*m_jobsCtx->applications().byId(appId));
    QVERIFY(buttonNamed(&edit, u"InterviewsAddBtn"_s)->isEnabled());
}

void tst_UiSmoke::testApplicationDialogInterviewsTabEditable() {
    resetJobsData();
    addApplication(u"Umbrella"_s, u"Lead"_s);
    const int appId = m_jobsCtx->applications().all().first().id;

    // A new application has no interviews tab and says to save first.
    ApplicationDialog create(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                             ApplicationDialog::Create);
    auto* createTabs = childNamed<QTabWidget>(&create, u"ApplicationDialogTabs"_s);
    QVERIFY(!createTabs->isTabEnabled(2));
    QVERIFY(childNamed<QLabel>(&create, u"InterviewsHintLabel"_s)->text().contains(u"Save"_s));

    // A saved application has an editable tab that saves interviews immediately.
    ApplicationDialog edit(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                           ApplicationDialog::Edit);
    edit.setApplication(*m_jobsCtx->applications().byId(appId));
    auto* tabs = childNamed<QTabWidget>(&edit, u"ApplicationDialogTabs"_s);
    QVERIFY(tabs->isTabEnabled(2));
    tabs->setCurrentIndex(2);
    auto* table = childNamed<QTableWidget>(&edit, u"InterviewsTable"_s);
    QCOMPARE(table->rowCount(), 0);
    auto* addBtn = buttonNamed(&edit, u"InterviewsAddBtn"_s);
    QVERIFY(addBtn->isEnabled());
    QVERIFY(buttonNamed(&edit, u"InterviewsEditBtn"_s)->isEnabled() == false);

    withNextModal([](QWidget* modal) {
        auto* dialog = qobject_cast<InterviewDialog*>(modal);
        QVERIFY(dialog != nullptr);
        childNamed<QDateEdit>(dialog, u"InterviewDateEdit"_s)->setDate(QDate(2026, 5, 4));
        buttonNamed(dialog, u"DialogSaveBtn"_s)->click();
    });
    addBtn->click();
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(m_jobsCtx->interviews().byApplication(appId).size(), 1);

    // Selecting a row enables Edit and Delete; deleting with confirmation clears it.
    table->selectRow(0);
    QVERIFY(buttonNamed(&edit, u"InterviewsEditBtn"_s)->isEnabled());
    auto* deleteBtn = buttonNamed(&edit, u"InterviewsDeleteBtn"_s);
    QVERIFY(deleteBtn->isEnabled());
    answerNextMessageBox([](QMessageBox* box) { box->button(QMessageBox::Yes)->click(); });
    deleteBtn->click();
    QCOMPARE(table->rowCount(), 0);
    QCOMPARE(m_jobsCtx->interviews().byApplication(appId).size(), 0);
}

void tst_UiSmoke::testJobsPageThemes() {
    resetJobsData();
    addApplication(u"Acme"_s, u"Developer"_s, ApplicationStatus::Technical);
    auto& themeMgr = m_jobsCtx->themeManager();

    for (const auto mode :
         {JobPrep::Ui::Theme::ThemeMode::Light, JobPrep::Ui::Theme::ThemeMode::Dark}) {
        themeMgr.setThemeMode(mode);
        QCOMPARE(jobsPage()->currentViewIndex(), 1);
        QCOMPARE(jobsPage()->visibleRowCount(), 1);
        QCoreApplication::processEvents();
    }

    ApplicationDialog dialog(m_jobsCtx->applications(), m_jobsCtx->interviews(),
                             ApplicationDialog::Create);
    dialog.show();
    QCoreApplication::processEvents();
    QCOMPARE(dialog.validationMessage(), QString());
    dialog.close();
    QCoreApplication::processEvents();
}

void tst_UiSmoke::testHeaderSearchFiltersJobsTable() {
    // The window's own context is empty until now, so the table starts out blank.
    QCOMPARE(m_window->pageStack()->currentIndex(), 0);
    QCOMPARE(m_window->findChild<QTableView*>(u"JobsTableView"_s)->model()->rowCount(), 0);

    for (const auto& company : {u"Southwind"_s, u"Eastwind"_s, u"Westwind"_s}) {
        JobApplication app;
        app.company = company;
        app.position = u"Engineer"_s;
        QVERIFY(m_ctx->applications().insert(app));
    }

    auto* headerSearch = m_window->findChild<QLineEdit*>(u"HeaderSearchEdit"_s);
    auto* pageSearch = m_window->findChild<QLineEdit*>(u"JobsSearchEdit"_s);
    auto* view = m_window->findChild<QTableView*>(u"JobsTableView"_s);
    QVERIFY(headerSearch != nullptr);
    QVERIFY(pageSearch != nullptr);
    QVERIFY(view != nullptr);
    QCOMPARE(view->model()->rowCount(), 3);

    // Typing in the header box switches to Jobs and filters there.
    headerSearch->setText(u"Southwind"_s);
    QCOMPARE(m_window->pageStack()->currentIndex(), 2);
    QCOMPARE(m_window->pageStack()->currentWidget(), m_window->findChild<JobPrep::Ui::Pages::JobsPage*>());
    QCOMPARE(pageSearch->text(), u"Southwind"_s);
    QCOMPARE(view->model()->rowCount(), 1);

    headerSearch->setText(QString());
    QCOMPARE(pageSearch->text(), QString());
    QCOMPARE(view->model()->rowCount(), 3);
}

void tst_UiSmoke::testNewEntryPointsOpenOneDialog() {
    auto* newBtn = m_window->findChild<QToolButton*>(u"HeaderNewBtn"_s);
    auto* applicationAction = m_window->findChild<QAction*>(u"HeaderNewApplication"_s);
    auto* topicAction = m_window->findChild<QAction*>(u"HeaderNewTopic"_s);
    auto* interviewAction = m_window->findChild<QAction*>(u"HeaderNewInterview"_s);
    auto* sessionAction = m_window->findChild<QAction*>(u"HeaderNewSession"_s);
    auto* pageAddBtn = m_window->findChild<QPushButton*>(u"JobsAddBtn"_s);
    QVERIFY(newBtn != nullptr);
    QVERIFY(newBtn->menu() != nullptr);
    QVERIFY(applicationAction != nullptr);
    QVERIFY(pageAddBtn != nullptr);

    // Application, Interview, Topic, and Study session all work now.
    QVERIFY(applicationAction->isEnabled());
    QVERIFY(interviewAction != nullptr && interviewAction->isEnabled());
    QVERIFY(topicAction != nullptr && topicAction->isEnabled());
    QVERIFY(sessionAction != nullptr && sessionAction->isEnabled());

    m_window->sidebar()->setCurrentPage(2);
    emit m_window->sidebar()->pageSelected(2);
    QCOMPARE(m_window->pageStack()->currentIndex(), 2);

    int dialogs = 0;
    withNextModal([&dialogs](QWidget* modal) {
        dialogs = countApplicationDialogs();
        QCOMPARE(dialogs, 1);
        auto* dialog = qobject_cast<ApplicationDialog*>(modal);
        QVERIFY(dialog != nullptr);
        dialog->reject();
    });
    applicationAction->trigger();
    QCOMPARE(dialogs, 1);

    withNextModal([&dialogs](QWidget* modal) {
        dialogs = countApplicationDialogs();
        auto* dialog = qobject_cast<ApplicationDialog*>(modal);
        QVERIFY(dialog != nullptr);
        dialog->reject();
    });
    pageAddBtn->click();
    QCOMPARE(dialogs, 1);
    QCOMPARE(m_ctx->applications().count(), 3);
}

void tst_UiSmoke::testHeaderNewApplicationOnCalendarPage() {
    // On the Calendar page the header "Application" entry must still open one
    // ApplicationDialog, and must switch to the Jobs page (Amendment B1).
    m_window->sidebar()->setCurrentPage(3);
    emit m_window->sidebar()->pageSelected(3);
    QCOMPARE(m_window->pageStack()->currentIndex(), 3);

    auto* applicationAction = m_window->findChild<QAction*>(u"HeaderNewApplication"_s);
    QVERIFY(applicationAction != nullptr);

    int dialogs = 0;
    withNextModal([&dialogs](QWidget* modal) {
        dialogs = countApplicationDialogs();
        auto* dialog = qobject_cast<ApplicationDialog*>(modal);
        QVERIFY(dialog != nullptr);
        dialog->reject();
    });
    applicationAction->trigger();
    QCOMPARE(dialogs, 1);
    QCOMPARE(m_window->pageStack()->currentIndex(), 2);
}

void tst_UiSmoke::testHeaderNewInterviewFromJobsRoutesToCalendar() {
    // From the Jobs page the header "Interview" entry opens exactly one InterviewDialog
    // on the Calendar page, preselected to the day the calendar is showing (Amendment B1).
    m_window->sidebar()->setCurrentPage(2);
    emit m_window->sidebar()->pageSelected(2);
    QCOMPARE(m_window->pageStack()->currentIndex(), 2);

    auto* calendar = m_window->findChild<JobPrep::Ui::Pages::CalendarPage*>();
    QVERIFY(calendar != nullptr);
    const QDate shownDay = calendar->selectedDay();

    auto* interviewAction = m_window->findChild<QAction*>(u"HeaderNewInterview"_s);
    QVERIFY(interviewAction != nullptr && interviewAction->isEnabled());

    int dialogs = 0;
    withNextModal([&dialogs, shownDay](QWidget* modal) {
        dialogs = countInterviewDialogs();
        QCOMPARE(dialogs, 1);
        auto* dialog = qobject_cast<InterviewDialog*>(modal);
        QVERIFY(dialog != nullptr);
        QCOMPARE(childNamed<QDateEdit>(dialog, u"InterviewDateEdit"_s)->date(), shownDay);
        dialog->reject();
    });
    interviewAction->trigger();
    QCOMPARE(dialogs, 1);
    QCOMPARE(m_window->pageStack()->currentIndex(), 3);

    // Leave the shared window where later slots expect it.
    m_window->sidebar()->setCurrentPage(0);
    emit m_window->sidebar()->pageSelected(0);
}

void tst_UiSmoke::testJobsContextMenuAddInterviewEndToEnd() {
    JobApplication app;
    app.company = u"EndToEnd Co"_s;
    app.position = u"Test Engineer"_s;
    QVERIFY(m_ctx->applications().insert(app));

    m_window->sidebar()->setCurrentPage(2);
    emit m_window->sidebar()->pageSelected(2);
    auto* jobs = m_window->findChild<JobPrep::Ui::Pages::JobsPage*>();
    QVERIFY(jobs != nullptr);
    auto* view = jobs->findChild<QTableView*>(u"JobsTableView"_s);
    QVERIFY(view != nullptr);

    int row = -1;
    for (int r = 0; r < view->model()->rowCount(); ++r) {
        const int id = view->model()
                           ->index(r, ApplicationTableModel::CompanyColumn)
                           .data(ApplicationTableModel::IdRole)
                           .toInt();
        if (id == app.id) {
            row = r;
            break;
        }
    }
    QVERIFY(row >= 0);
    view->setCurrentIndex(view->model()->index(row, ApplicationTableModel::CompanyColumn));

    // The context menu cannot be popped up offscreen, so trigger its action directly.
    auto* menu = jobs->findChild<QMenu*>(u"JobsContextMenu"_s);
    QVERIFY(menu != nullptr);
    QAction* addInterview = menu->findChild<QAction*>(u"JobsAddInterviewAction"_s);
    QVERIFY(addInterview != nullptr);

    const QDate day(2026, 5, 4);
    withNextModal([day](QWidget* modal) {
        auto* dialog = qobject_cast<InterviewDialog*>(modal);
        QVERIFY(dialog != nullptr);
        childNamed<QDateEdit>(dialog, u"InterviewDateEdit"_s)->setDate(day);
        buttonNamed(dialog, u"DialogSaveBtn"_s)->click();
    });
    addInterview->trigger();
    QCOMPARE(m_ctx->interviews().byApplication(app.id).size(), 1);

    // The Calendar page reflects the marker on that day.
    m_window->sidebar()->setCurrentPage(3);
    emit m_window->sidebar()->pageSelected(3);
    auto* calendar = m_window->findChild<JobPrep::Ui::Pages::CalendarPage*>();
    QVERIFY(calendar != nullptr);
    QVERIFY(calendar->hasItemsOn(day));
}

void tst_UiSmoke::testCalendarPageMarkersAndAgenda() {
    const QDate today(2026, 10, 11);
    resetCalendarData(today);

    auto* view = monthView();
    auto* list = agendaList();
    QVERIFY(view != nullptr);
    QVERIFY(list != nullptr);
    QCOMPARE(view->visibleMonth(), QDate(2026, 10, 1));
    QCOMPARE(list->model()->rowCount(), 0);
    QVERIFY(!calendarPage()->hasItemsOn(today));

    // An interview lands on the selected day: one marker, one agenda row.
    const int appId = addApplicationWithInterview(u"Acme"_s, today, 10);
    QVERIFY(calendarPage()->hasItemsOn(today));
    QVERIFY(!calendarPage()->hasItemsOn(today.addDays(1)));
    QCOMPARE(view->markersFor(today).total, 1);
    QCOMPARE(view->markersFor(today).kinds.first(), AgendaKind::Interview);
    QCOMPARE(list->model()->rowCount(), 1);
    QCOMPARE(list->model()->index(0, 0).data(JobPrep::Models::AgendaListModel::KindRole).toInt(),
             static_cast<int>(AgendaKind::Interview));

    // A follow-up on the same day adds a second marker and agenda row.
    QVERIFY(m_calCtx->applications().setNextAction(appId, u"Email the recruiter"_s, today));
    QCOMPARE(view->markersFor(today).total, 2);
    QCOMPARE(view->markersFor(today).kinds.size(), 2);
    QCOMPARE(list->model()->rowCount(), 2);
}

void tst_UiSmoke::testCalendarLayerChipsFilterAgenda() {
    const QDate today(2026, 10, 11);
    resetCalendarData(today);
    const int appId = addApplicationWithInterview(u"Acme"_s, today, 10);
    QVERIFY(m_calCtx->applications().setNextAction(appId, u"Email the recruiter"_s, today));
    addStudyTarget(today);

    auto* view = monthView();
    auto* list = agendaList();
    QCOMPARE(list->model()->rowCount(), 3);
    QCOMPARE(view->markersFor(today).total, 3);

    // Hiding a layer drops its marker and its agenda row but keeps the others.
    auto* followUpChip = buttonNamed(m_calendar.get(), u"CalendarLayerFollowUpBtn"_s);
    QVERIFY(followUpChip != nullptr);
    followUpChip->click();
    QCOMPARE(m_calCtx->settings().calendarLayerFollowUps(), false);
    QCOMPARE(list->model()->rowCount(), 2);
    QCOMPARE(view->markersFor(today).total, 2);
    QVERIFY(!view->markersFor(today).kinds.contains(AgendaKind::FollowUp));
    QVERIFY(calendarPage()->hasItemsOn(today));

    auto* targetChip = buttonNamed(m_calendar.get(), u"CalendarLayerTargetBtn"_s);
    targetChip->click();
    QCOMPARE(m_calCtx->settings().calendarLayerTargets(), false);
    QCOMPARE(list->model()->rowCount(), 1);

    // Re-enabling restores everything.
    followUpChip->click();
    targetChip->click();
    QCOMPARE(list->model()->rowCount(), 3);
    QCOMPARE(view->markersFor(today).total, 3);
}

void tst_UiSmoke::testCalendarNewInterviewFromDay() {
    resetCalendarData(QDate(2026, 10, 11));
    JobApplication app;
    app.company = u"Acme"_s;
    app.position = u"Developer"_s;
    QVERIFY(m_calCtx->applications().insert(app));

    // Double-clicking a day opens one InterviewDialog preselected to that day.
    const QDate day(2026, 10, 20);
    auto* view = monthView();
    view->setVisibleMonth(day);
    QCoreApplication::processEvents();
    int dialogs = 0;
    answerModalWhenReady([&dialogs, day](QWidget* modal) {
        auto* dialog = qobject_cast<InterviewDialog*>(modal);
        QVERIFY(dialog != nullptr);
        ++dialogs;
        QCOMPARE(childNamed<QDateEdit>(dialog, u"InterviewDateEdit"_s)->date(), day);
        dialog->done(QDialog::Rejected);  // skip the "discard changes?" prompt
    });
    QTest::mouseDClick(view, Qt::LeftButton, Qt::KeyboardModifiers(), view->dayRect(day).center());
    QCOMPARE(dialogs, 1);

    // triggerNew uses the selected day.
    view->setSelectedDay(QDate(2026, 10, 25));
    withNextModal([](QWidget* modal) {
        auto* dialog = qobject_cast<InterviewDialog*>(modal);
        QVERIFY(dialog != nullptr);
        QCOMPARE(childNamed<QDateEdit>(dialog, u"InterviewDateEdit"_s)->date(), QDate(2026, 10, 25));
        dialog->done(QDialog::Rejected);
    });
    calendarPage()->triggerNew();
}

void tst_UiSmoke::testCalendarFollowUpEndToEnd() {
    const QDate today(2026, 10, 11);
    resetCalendarData(today);
    JobApplication app;
    app.company = u"Acme"_s;
    app.position = u"Developer"_s;
    QVERIFY(m_calCtx->applications().insert(app));

    auto* view = monthView();
    const QDate day(2026, 10, 15);
    view->setVisibleMonth(day);
    view->setSelectedDay(day);
    QCoreApplication::processEvents();

    withNextModal([appId = app.id](QWidget* modal) {
        auto* dialog = qobject_cast<FollowUpDialog*>(modal);
        QVERIFY(dialog != nullptr);
        auto* combo = childNamed<QComboBox>(dialog, u"FollowUpApplicationCombo"_s);
        combo->setCurrentIndex(combo->findData(appId));
        childNamed<QLineEdit>(dialog, u"FollowUpActionEdit"_s)->setText(u"Email recruiter"_s);
        buttonNamed(dialog, u"DialogSaveBtn"_s)->click();
    });
    buttonNamed(m_calendar.get(), u"CalendarAddFollowUpBtn"_s)->click();

    QVERIFY(m_calCtx->applications().byId(app.id)->nextActionDate.has_value());
    QCOMPARE(*m_calCtx->applications().byId(app.id)->nextActionDate, day);
    QCOMPARE(agendaList()->model()->rowCount(), 1);
    QCOMPARE(agendaList()->model()->index(0, 0).data(JobPrep::Models::AgendaListModel::KindRole).toInt(),
             static_cast<int>(AgendaKind::FollowUp));
    QCOMPARE(view->markersFor(day).total, 1);
}

void tst_UiSmoke::testCalendarFollowUpOverwritePrompt() {
    resetCalendarData(QDate(2026, 10, 11));
    JobApplication app;
    app.company = u"Acme"_s;
    app.position = u"Developer"_s;
    QVERIFY(m_calCtx->applications().insert(app));
    const QDate first(2026, 10, 15);
    const QDate second(2026, 10, 20);
    QVERIFY(m_calCtx->applications().setNextAction(app.id, u"First action"_s, first));

    FollowUpDialog dialog(m_calCtx->applications(), second);
    dialog.show();
    auto* combo = childNamed<QComboBox>(&dialog, u"FollowUpApplicationCombo"_s);
    combo->setCurrentIndex(combo->findData(app.id));
    childNamed<QLineEdit>(&dialog, u"FollowUpActionEdit"_s)->setText(u"Second action"_s);
    auto* saveBtn = buttonNamed(&dialog, u"DialogSaveBtn"_s);
    QVERIFY(saveBtn != nullptr);

    // Declining the overwrite prompt keeps the original date and the dialog open.
    answerNextMessageBox([](QMessageBox* box) { box->button(QMessageBox::No)->click(); });
    saveBtn->click();
    QCOMPARE(*m_calCtx->applications().byId(app.id)->nextActionDate, first);
    QVERIFY(dialog.isVisible());

    // Accepting replaces it.
    answerNextMessageBox([](QMessageBox* box) { box->button(QMessageBox::Yes)->click(); });
    saveBtn->click();
    QCOMPARE(*m_calCtx->applications().byId(app.id)->nextActionDate, second);
    QVERIFY(!dialog.isVisible());
}

void tst_UiSmoke::testCalendarSplitterStatePersisted() {
    resetCalendarData(QDate(2026, 10, 11));
    auto* splitter = childNamed<QSplitter>(m_calendar.get(), u"CalendarSplitter"_s);
    QVERIFY(splitter != nullptr);

    if (splitter->sizes().value(1) < splitter->width() / 4) {
        splitter->setSizes({splitter->width() / 2, splitter->width() / 2});
        QCoreApplication::processEvents();
    }

    // Drag handle #1 like a user would; only a real move emits splitterMoved, which is
    // what saves the state (QSplitter::moveSplitter is protected).
    auto* handle = splitter->handle(1);
    QVERIFY(handle != nullptr);
    const QPoint start(handle->width() / 2, handle->height() / 2);
    QTest::mousePress(handle, Qt::LeftButton, Qt::KeyboardModifiers(), start);
    QTest::mouseMove(handle, start + QPoint(80, 0));
    QTest::mouseRelease(handle, Qt::LeftButton, Qt::KeyboardModifiers(), start + QPoint(80, 0));
    QCoreApplication::processEvents();
    const QList<int> movedSizes = splitter->sizes();
    const QByteArray saved = m_calCtx->settings().calendarSplitterState();
    QVERIFY(!saved.isEmpty());

    // A fresh page in the same context restores exactly those sizes.
    JobPrep::Ui::Pages::CalendarPage second(
        m_calCtx->interviews(), m_calCtx->applications(), m_calCtx->topics(), m_calCtx->tracks(),
        m_calCtx->agendaService(), m_calCtx->settings(), m_calCtx->themeManager(), nullptr,
        [] { return QDate(2026, 10, 11); });
    second.resize(1200, 720);
    second.show();
    QCoreApplication::processEvents();
    auto* restored = childNamed<QSplitter>(&second, u"CalendarSplitter"_s);
    QVERIFY(restored != nullptr);
    QCOMPARE(restored->sizes(), movedSizes);
}

void tst_UiSmoke::testCalendarThemes() {
    const QDate today(2026, 10, 11);
    resetCalendarData(today);
    const int appId = addApplicationWithInterview(u"Acme"_s, today, 10);
    QVERIFY(m_calCtx->applications().setNextAction(appId, u"Email the recruiter"_s, today));
    addStudyTarget(today);

    for (const auto mode :
         {JobPrep::Ui::Theme::ThemeMode::Light, JobPrep::Ui::Theme::ThemeMode::Dark}) {
        m_calCtx->themeManager().setThemeMode(mode);
        QCoreApplication::processEvents();
        QCOMPARE(monthView()->markersFor(today).total, 3);
        QCOMPARE(agendaList()->model()->rowCount(), 3);
    }
}

void tst_UiSmoke::testCalendarScreenshots() {
    const QString dir = qEnvironmentVariable("JOBPREP_REPORT_IMG_DIR");
    if (dir.isEmpty()) QSKIP("JOBPREP_REPORT_IMG_DIR is not set");
    QVERIFY(QDir().mkpath(dir));

    const QDate today(2026, 10, 11);
    resetCalendarData(today);

    const auto save = [this, dir](JobPrep::Ui::Theme::ThemeMode mode, const QString& name) {
        m_calCtx->themeManager().setThemeMode(mode);
        QCoreApplication::processEvents();
        const QImage image = m_calendar->grab().toImage();
        QVERIFY(!image.isNull());
        QVERIFY(image.save(QDir(dir).filePath(name + u".png"_s)));
    };
    const auto forBothThemes = [&save](const QString& name) {
        save(JobPrep::Ui::Theme::ThemeMode::Light, name + u"-light"_s);
        save(JobPrep::Ui::Theme::ThemeMode::Dark, name + u"-dark"_s);
    };

    // Empty: no agenda data, the "Nothing Planned" state is shown.
    forBothThemes(u"calendar-empty"_s);

    // With markers: interviews, a follow-up and a study target across the month.
    addApplicationWithInterview(u"Google"_s, today, 10);
    const int second = addApplicationWithInterview(u"DeepMind"_s, today.addDays(3), 14);
    QVERIFY(m_calCtx->applications().setNextAction(second, u"Send thank-you email"_s,
                                                   today.addDays(1)));
    addStudyTarget(today.addDays(2));
    forBothThemes(u"calendar-markers"_s);

    // With a selected day: the agenda panel is filled for a specific interview day.
    monthView()->setSelectedDay(today.addDays(3));
    QCoreApplication::processEvents();
    forBothThemes(u"calendar-selected"_s);
}

void tst_UiSmoke::testSelectionRestorerSurvivesReset() {
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

void tst_UiSmoke::testStudyBoardSeededAndTrackFilter() {
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

void tst_UiSmoke::testStudyQuickAdd() {
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

void tst_UiSmoke::testStudyKeyboardMoveAndReorder() {
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

void tst_UiSmoke::testStudyChecklistTickSurvivesReset() {
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

void tst_UiSmoke::testStudyBoardRealDnDDropEvent() {
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

void tst_UiSmoke::testStudyDetailPanelAutosaveAndValidation() {
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

    // Target date set and clear
    auto* dateBtn = childNamed<QPushButton>(detail, u"StudyDetailDateClearBtn"_s);
    QCOMPARE(dateBtn->text(), u"Set date"_s);
    dateBtn->click();
    QCoreApplication::processEvents();
    QCOMPARE(dateBtn->text(), u"Clear"_s);
    QVERIFY(m_studyCtx->topics().byId(t2Id)->targetDate.has_value());

    dateBtn->click();
    QCoreApplication::processEvents();
    QCOMPARE(dateBtn->text(), u"Set date"_s);
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

void tst_UiSmoke::testStudyDeleteTopicKeepsSessions() {
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

void tst_UiSmoke::testStudyTrackFilterPersistenceAndFallback() {
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

void tst_UiSmoke::testStudyOverdueChipWithFixedClock() {
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

void tst_UiSmoke::testStudyPageApi() {
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

void tst_UiSmoke::testChipBarJobsAndCalendarRegression() {
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

void tst_UiSmoke::testStudyContextMenuMoveToStatus() {
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

void tst_UiSmoke::testStudyPageSelectionRestoreAcrossOperations() {
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

void tst_UiSmoke::testStudyChecklistEditAndReorder() {
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

void tst_UiSmoke::testStudySplitterStatePersisted() {
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

void tst_UiSmoke::testStudyPriorityTooltip() {
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

void tst_UiSmoke::testStudyDragFeedbackIndicatorAndHighlight() {
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

void tst_UiSmoke::testStudySessionDialogLogTime() {
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

void tst_UiSmoke::testStudyTrackDialogCrudAndCascade() {
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

void tst_UiSmoke::testHeaderNewSessionAndTopicRouting() {
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

void tst_UiSmoke::testHeaderSearchDisabledOnStudyPage() {
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

void tst_UiSmoke::testCalendarJumpToStudyTopic() {
    const QDate day(2026, 10, 20);
    Track track;
    track.name = u"System Design"_s;
    track.color = u"#0d9488"_s;
    QVERIFY(m_ctx->tracks().insert(track));
    const int trackId = m_ctx->tracks().all().last().id;

    Topic topic;
    topic.trackId = trackId;
    topic.title = u"Rate Limiter"_s;
    topic.targetDate = day;
    QVERIFY(m_ctx->topics().insert(topic));
    const int topicId = m_ctx->topics().all().last().id;

    // Switch to Calendar
    m_window->sidebar()->setCurrentPage(3);
    emit m_window->sidebar()->pageSelected(3);
    QCOMPARE(m_window->pageStack()->currentIndex(), 3);

    // Verify SPEC C5: Double-clicking a day with study target still opens InterviewDialog
    int interviewDialogs = 0;
    answerModalWhenReady([&interviewDialogs](QWidget* modal) {
        if (auto* dlg = qobject_cast<JobPrep::Ui::Dialogs::InterviewDialog*>(modal)) {
            interviewDialogs++;
            dlg->done(QDialog::Rejected);
        }
    });
    auto* calPage = qobject_cast<JobPrep::Ui::Pages::CalendarPage*>(m_window->pageStack()->currentWidget());
    QVERIFY(calPage != nullptr);
    auto* mv = calPage->findChild<JobPrep::Ui::Widgets::MonthView*>();
    QVERIFY(mv != nullptr);
    mv->setVisibleMonth(day);
    QTest::mouseDClick(mv, Qt::LeftButton, Qt::KeyboardModifiers(), mv->dayRect(day).center());
    QCOMPARE(interviewDialogs, 1);

    // Call openTopic(topicId) in Board view
    m_window->openTopic(topicId);
    QCoreApplication::processEvents();

    // Verify switched to Study page (index 1) and topic is selected
    QCOMPARE(m_window->pageStack()->currentIndex(), 1);
    auto* study = qobject_cast<JobPrep::Ui::Pages::StudyPage*>(m_window->pageStack()->currentWidget());
    QVERIFY(study != nullptr);
    QCOMPARE(study->selectedTopicId(), std::optional<int>(topicId));
    QVERIFY(study->isDetailVisible());

    // Test jumping while Study is in List view (works in both views)
    study->setViewMode(u"list"_s);
    m_window->sidebar()->setCurrentPage(3);
    emit m_window->sidebar()->pageSelected(3);
    m_window->openTopic(topicId);
    QCoreApplication::processEvents();
    QCOMPARE(m_window->pageStack()->currentIndex(), 1);
    QCOMPARE(study->viewMode(), u"list"_s);
    QCOMPARE(study->selectedTopicId(), std::optional<int>(topicId));
    QVERIFY(study->isDetailVisible());
    study->setViewMode(u"board"_s);

    // Now test jump when topic is hidden by track filter and search text
    Track track2;
    track2.name = u"Databases"_s;
    track2.color = u"#e11d48"_s;
    QVERIFY(m_ctx->tracks().insert(track2));
    const int track2Id = m_ctx->tracks().all().last().id;

    auto* studyChips = childNamed<JobPrep::Ui::Widgets::ChipBar>(study, u"StudyTrackChips"_s);
    if (studyChips && studyChips->button(track2Id)) studyChips->button(track2Id)->click();
    study->setSearchText(u"nonexistent_xyz"_s);
    QCoreApplication::processEvents();

    m_window->openTopic(topicId);
    QCoreApplication::processEvents();

    QCOMPARE(study->searchText(), QString());
    auto* chips = childNamed<JobPrep::Ui::Widgets::ChipBar>(study, u"StudyTrackChips"_s);
    QCOMPARE(chips->currentId(), -1);
    QCOMPARE(study->selectedTopicId(), std::optional<int>(topicId));

    // Test non-fatal message when topic was deleted
    QVERIFY(m_ctx->topics().remove(topicId));
    m_window->openTopic(topicId);
    QCoreApplication::processEvents();
    // Does not crash, stays responsive
    QCOMPARE(m_window->pageStack()->currentIndex(), 1);
}

void tst_UiSmoke::testCalendarTopicDateAndTrackRename() {
    const QDate today(2026, 10, 11);
    resetCalendarData(today);

    Track track;
    track.name = u"Algorithms Track"_s;
    track.color = u"#3b82f6"_s;
    QVERIFY(m_calCtx->tracks().insert(track));
    const int trackId = m_calCtx->tracks().all().last().id;

    Topic topic;
    topic.trackId = trackId;
    topic.title = u"Dynamic Programming"_s;
    topic.targetDate = today;
    QVERIFY(m_calCtx->topics().insert(topic));

    m_calCtx->topics().changed();
    QCoreApplication::processEvents();

    // Verify calendar month view has target marker
    QVERIFY(monthView()->markersFor(today).kinds.contains(JobPrep::Domain::AgendaKind::StudyTarget));

    // Rename track -> agenda subtitle updates
    track.name = u"Advanced DP"_s;
    QVERIFY(m_calCtx->tracks().update(track));
    m_calCtx->tracks().changed();
    QCoreApplication::processEvents();

    auto* agenda = agendaList();
    QVERIFY(agenda != nullptr);
    auto* agendaModel = agenda->model();
    QVERIFY(agendaModel != nullptr);
    QVERIFY(agendaModel->rowCount() > 0);
    bool foundUpdatedSubtitle = false;
    for (int r = 0; r < agendaModel->rowCount(); ++r) {
        const QString sub = agendaModel->index(r, 0).data(JobPrep::Models::AgendaListModel::SubtitleRole).toString();
        if (sub.contains(u"Advanced DP"_s)) {
            foundUpdatedSubtitle = true;
            break;
        }
    }
    QVERIFY(foundUpdatedSubtitle);

    // Delete track -> cascade deletes topic, markers clear
    QVERIFY(m_calCtx->tracks().remove(trackId));
    m_calCtx->tracks().changed();
    m_calCtx->topics().changed();
    QCoreApplication::processEvents();

    QVERIFY(!monthView()->markersFor(today).kinds.contains(JobPrep::Domain::AgendaKind::StudyTarget));
}

void tst_UiSmoke::testCalendarTrackDeleteClearsMarkers() {
    const QDate today(2026, 10, 11);
    resetCalendarData(today);

    Track track;
    track.name = u"Track Cascade Markers"_s;
    track.color = u"#3b82f6"_s;
    QVERIFY(m_calCtx->tracks().insert(track));
    const int trackId = m_calCtx->tracks().all().last().id;

    Topic topic;
    topic.trackId = trackId;
    topic.title = u"Target Marker Topic"_s;
    topic.targetDate = today;
    QVERIFY(m_calCtx->topics().insert(topic));

    m_calCtx->topics().changed();
    QCoreApplication::processEvents();

    QVERIFY(monthView()->markersFor(today).kinds.contains(JobPrep::Domain::AgendaKind::StudyTarget));

    // Deleting track ONLY (triggers tracks().changed() which cascades in DB and signals CalendarPage)
    QVERIFY(m_calCtx->tracks().remove(trackId));
    m_calCtx->tracks().changed();
    QCoreApplication::processEvents();

    QVERIFY(!monthView()->markersFor(today).kinds.contains(JobPrep::Domain::AgendaKind::StudyTarget));
}

void tst_UiSmoke::testMonthViewDoubleClicksDayWithoutOpeningStudyTarget() {
    const QDate day(2026, 10, 22);
    Track track;
    track.name = u"Interview Target Track"_s;
    track.color = u"#3b82f6"_s;
    QVERIFY(m_ctx->tracks().insert(track));
    const int trackId = m_ctx->tracks().all().last().id;

    Topic topic;
    topic.trackId = trackId;
    topic.title = u"System Architecture"_s;
    topic.targetDate = day;
    QVERIFY(m_ctx->topics().insert(topic));

    // Switch to Calendar
    m_window->sidebar()->setCurrentPage(3);
    emit m_window->sidebar()->pageSelected(3);
    QCOMPARE(m_window->pageStack()->currentIndex(), 3);

    auto* calPage = qobject_cast<JobPrep::Ui::Pages::CalendarPage*>(m_window->pageStack()->currentWidget());
    QVERIFY(calPage != nullptr);
    auto* mv = calPage->findChild<JobPrep::Ui::Widgets::MonthView*>();
    QVERIFY(mv != nullptr);
    mv->setVisibleMonth(day);

    // Double-click on day having a study target opens InterviewDialog per SPEC C5, never jumps to Study
    int interviewDialogs = 0;
    answerModalWhenReady([&interviewDialogs](QWidget* modal) {
        if (auto* dlg = qobject_cast<JobPrep::Ui::Dialogs::InterviewDialog*>(modal)) {
            interviewDialogs++;
            dlg->done(QDialog::Rejected);
        }
    });

    QTest::mouseDClick(mv, Qt::LeftButton, Qt::KeyboardModifiers(), mv->dayRect(day).center());
    QCOMPARE(interviewDialogs, 1);
    // MainWindow remains on Calendar page (did not jump to Study)
    QCOMPARE(m_window->pageStack()->currentIndex(), 3);
}

void tst_UiSmoke::testCtrlNTriggersCurrentPageNew() {
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
    QCOMPARE(intDialogs, 1);

    // 3. Study page: triggerNew() focuses Backlog quick-add
    m_window->sidebar()->setCurrentPage(1);
    emit m_window->sidebar()->pageSelected(1);
    m_window->triggerCurrentPageNew();
    auto* study = qobject_cast<JobPrep::Ui::Pages::StudyPage*>(m_window->pageStack()->currentWidget());
    QVERIFY(study != nullptr);
    auto* backlog = childNamed<JobPrep::Ui::Widgets::TopicBoardColumn>(study, u"StudyColumn_Backlog"_s);
    if (backlog) {
        auto* quickAdd = childNamed<QLineEdit>(backlog, u"StudyColumnQuickAdd"_s);
        QCOMPARE(QApplication::focusWidget(), quickAdd);
    }

    // 4. Home page: triggerNew() is a safe no-op (no modal opened)
    m_window->sidebar()->setCurrentPage(0);
    emit m_window->sidebar()->pageSelected(0);
    m_window->triggerCurrentPageNew();
    QVERIFY(QApplication::activeModalWidget() == nullptr);

    // 5. Settings page: triggerNew() is a safe no-op (no modal opened)
    m_window->sidebar()->setCurrentPage(4);
    emit m_window->sidebar()->pageSelected(4);
    m_window->triggerCurrentPageNew();
    QVERIFY(QApplication::activeModalWidget() == nullptr);
}

void tst_UiSmoke::testStudyBoardSingleSelectionAcrossColumns() {
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

void tst_UiSmoke::testStudyListViewToggleSortAndDelegates() {
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

void tst_UiSmoke::testStudyScreenshots() {
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

void tst_UiSmoke::testFormatHelper() {
    using JobPrep::Ui::FormatHelper;

    // Goal text
    QCOMPARE(FormatHelper::formatDuration(0), u"0m"_s);
    QCOMPARE(FormatHelper::formatDuration(45), u"45m"_s);
    QCOMPARE(FormatHelper::formatDuration(60), u"1h"_s);
    QCOMPARE(FormatHelper::formatDuration(260), u"4h 20m"_s);
    QCOMPARE(FormatHelper::formatDuration(430), u"7h 10m"_s);
    QCOMPARE(FormatHelper::formatGoalText(260, 360), u"4h 20m / 6h"_s);
    QCOMPARE(FormatHelper::formatGoalText(430, 360), u"7h 10m / 6h"_s);

    // Interview rate
    QCOMPARE(FormatHelper::formatInterviewRate(std::nullopt), QString::fromUtf8("\u2013"));
    QCOMPARE(FormatHelper::formatInterviewRate(6.0 / 9.0), u"66.7%"_s);

    // Goal progress bar real and capped percentages
    AppContext ctx(u":memory:"_s);
    JobPrep::Ui::Theme::ThemeManager theme(ctx.settings());
    JobPrep::Ui::Widgets::GoalProgressBar bar(theme);
    bar.setGoal(260, 360);
    QCOMPARE(bar.realPercent(), 72);
    QCOMPARE(bar.cappedPercent(), 72);
    bar.setGoal(430, 360);
    QCOMPARE(bar.realPercent(), 119);
    QCOMPARE(bar.cappedPercent(), 100);

    // Streak wording
    QCOMPARE(FormatHelper::formatStreak(0), u"0-day streak"_s);
    QCOMPARE(FormatHelper::formatStreak(1), u"1-day streak"_s);
    QCOMPARE(FormatHelper::formatStreak(4), u"4-day streak"_s);

    // Active processes
    QCOMPARE(FormatHelper::formatActiveProcesses(0), u"0 active processes"_s);
    QCOMPARE(FormatHelper::formatActiveProcesses(1), u"1 active process"_s);
    QCOMPARE(FormatHelper::formatActiveProcesses(8), u"8 active processes"_s);

    // Next interview status bar text
    QCOMPARE(FormatHelper::formatNextInterview(std::nullopt),
             u"Next: No upcoming interviews"_s);
    JobPrep::Domain::AgendaItem agendaItem;
    agendaItem.title = u"Globex"_s;
    agendaItem.date = QDate(2026, 10, 30);
    agendaItem.time = QTime(14, 0);
    QCOMPARE(FormatHelper::formatNextInterview(agendaItem, QLocale(QLocale::English)),
             u"Next: Fri 14:00 Globex"_s);

    // Funnel stage labels
    QCOMPARE(FormatHelper::funnelStageLabel(ApplicationStatus::Applied), u"Applied"_s);
}

void tst_UiSmoke::testDashboardVisitedBothThemes() {
    AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.resize(1100, 700);
    win.show();
    QCoreApplication::processEvents();

    QCOMPARE(win.pageStack()->currentIndex(), 0);
    QVERIFY(win.homePage() != nullptr);

    ctx.themeManager().setThemeMode(JobPrep::Ui::Theme::ThemeMode::Dark);
    QCoreApplication::processEvents();
    ctx.themeManager().setThemeMode(JobPrep::Ui::Theme::ThemeMode::Light);
    QCoreApplication::processEvents();
}

void tst_UiSmoke::testDashboardNumbersMatchHandCalculatedDataset() {
    AppContext ctx(u":memory:"_s);
    const QDateTime now(QDate(2026, 10, 29), QTime(10, 0));
    ctx.statsService().setClock([now] { return now; });
    ctx.dayWatcher().setClock([now] { return now.date(); });
    ctx.settings().setFirstDayOfWeek(1);

    // 2 wishlist
    for (int i = 0; i < 2; ++i) {
        JobApplication w;
        w.company = u"Wishlist_%1"_s.arg(i);
        w.position = u"Dev"_s;
        w.status = ApplicationStatus::Wishlist;
        QVERIFY(ctx.applications().insert(w));
    }
    // 3 applied: Oct 27, Oct 29, Oct 20
    JobApplication a1; a1.company = u"App_27"_s; a1.position = u"Dev"_s; a1.status = ApplicationStatus::Applied; a1.appliedDate = QDate(2026, 10, 27); QVERIFY(ctx.applications().insert(a1));
    JobApplication a2; a2.company = u"App_29"_s; a2.position = u"Dev"_s; a2.status = ApplicationStatus::Applied; a2.appliedDate = QDate(2026, 10, 29); QVERIFY(ctx.applications().insert(a2));
    JobApplication a3; a3.company = u"App_20"_s; a3.position = u"Dev"_s; a3.status = ApplicationStatus::Applied; a3.appliedDate = QDate(2026, 10, 20); QVERIFY(ctx.applications().insert(a3));

    // 2 hr_screen
    for (int i = 0; i < 2; ++i) {
        JobApplication hr;
        hr.company = u"HrScreen_%1"_s.arg(i);
        hr.position = u"Dev"_s;
        hr.status = ApplicationStatus::HrScreen;
        hr.appliedDate = QDate(2026, 10, 15);
        QVERIFY(ctx.applications().insert(hr));
    }

    // 1 technical, 1 final, 1 offer
    JobApplication tech; tech.company = u"TechCo"_s; tech.position = u"Dev"_s; tech.status = ApplicationStatus::Technical; tech.appliedDate = QDate(2026, 10, 15); QVERIFY(ctx.applications().insert(tech));
    JobApplication finalApp; finalApp.company = u"FinalCo"_s; finalApp.position = u"Dev"_s; finalApp.status = ApplicationStatus::Final; finalApp.appliedDate = QDate(2026, 10, 15); QVERIFY(ctx.applications().insert(finalApp));
    JobApplication offer; offer.company = u"OfferCo"_s; offer.position = u"Dev"_s; offer.status = ApplicationStatus::Offer; offer.appliedDate = QDate(2026, 10, 15); QVERIFY(ctx.applications().insert(offer));

    // 1 rejected with history
    JobApplication rej; rej.company = u"RejCo"_s; rej.position = u"Dev"_s; rej.status = ApplicationStatus::Applied; rej.appliedDate = QDate(2026, 10, 10); QVERIFY(ctx.applications().insert(rej));
    QVERIFY(ctx.applications().setStatus(rej.id, ApplicationStatus::HrScreen));
    QVERIFY(ctx.applications().setStatus(rej.id, ApplicationStatus::Technical));
    QVERIFY(ctx.applications().setStatus(rej.id, ApplicationStatus::Rejected));

    // Sessions
    StudySession sSun; sSun.sessionDate = QDate(2026, 10, 25); sSun.minutes = 30; QVERIFY(ctx.sessions().insert(sSun));
    StudySession sMon; sMon.sessionDate = QDate(2026, 10, 26); sMon.minutes = 60; QVERIFY(ctx.sessions().insert(sMon));
    StudySession sTue; sTue.sessionDate = QDate(2026, 10, 27); sTue.minutes = 120; QVERIFY(ctx.sessions().insert(sTue));
    StudySession sWed; sWed.sessionDate = QDate(2026, 10, 28); sWed.minutes = 80; QVERIFY(ctx.sessions().insert(sWed));

    // Interviews
    Interview ivPast; ivPast.applicationId = a1.id; ivPast.startAt = QDateTime(QDate(2026, 10, 27), QTime(14, 0)); QVERIFY(ctx.interviews().insert(ivPast));
    Interview ivEarlierToday; ivEarlierToday.applicationId = a1.id; ivEarlierToday.startAt = QDateTime(QDate(2026, 10, 29), QTime(8, 0)); QVERIFY(ctx.interviews().insert(ivEarlierToday));
    Interview iv1; iv1.applicationId = a1.id; iv1.startAt = QDateTime(QDate(2026, 10, 30), QTime(14, 0)); QVERIFY(ctx.interviews().insert(iv1));
    Interview ivCanc; ivCanc.applicationId = a1.id; ivCanc.startAt = QDateTime(QDate(2026, 10, 31), QTime(15, 0)); ivCanc.outcome = InterviewOutcome::Cancelled; QVERIFY(ctx.interviews().insert(ivCanc));
    Interview iv2; iv2.applicationId = a1.id; iv2.startAt = QDateTime(QDate(2026, 11, 4), QTime(11, 0)); QVERIFY(ctx.interviews().insert(iv2));
    Interview iv3; iv3.applicationId = a1.id; iv3.startAt = QDateTime(QDate(2026, 11, 4), QTime(23, 0)); QVERIFY(ctx.interviews().insert(iv3));
    Interview ivFuture; ivFuture.applicationId = a1.id; ivFuture.startAt = QDateTime(QDate(2026, 11, 5), QTime(9, 0)); QVERIFY(ctx.interviews().insert(ivFuture));

    MainWindow win(ctx);
    win.resize(1100, 700);
    win.show();
    win.dashboardController()->refreshNow();
    QCoreApplication::processEvents();

    const auto snap = win.dashboardController()->snapshot();
    QCOMPARE(snap.stats.appliedThisWeek, 2);
    QCOMPARE(snap.stats.activeProcesses, 8);
    QCOMPARE(snap.stats.interviewsNext7Days, 3);
    QCOMPARE(snap.stats.studyStreakDays, 4);
    QCOMPARE(snap.stats.weekMinutes, 260);
    QCOMPARE(snap.stats.funnel.size(), 5);
    QCOMPARE(snap.stats.funnel[0].reachedCount, 9);
    QCOMPARE(snap.stats.funnel[1].reachedCount, 6);
    QCOMPARE(snap.stats.funnel[2].reachedCount, 4);
    QCOMPARE(snap.stats.funnel[3].reachedCount, 2);
    QCOMPARE(snap.stats.funnel[4].reachedCount, 1);
    QVERIFY(snap.stats.interviewRate.has_value());
    QVERIFY(qAbs(*snap.stats.interviewRate - (6.0 / 9.0)) < 0.0001);

    // Verify FunnelWidget displays all 5 stages in order and interview rate
    using JobPrep::Ui::FormatHelper;
    auto* funnelWidget = win.homePage()->findChild<JobPrep::Ui::Widgets::FunnelWidget*>();
    QVERIFY(funnelWidget != nullptr);
    QCOMPARE(funnelWidget->stages().size(), 5);
    QCOMPARE(FormatHelper::funnelStageLabel(funnelWidget->stages()[0].status), u"Applied"_s);
    QCOMPARE(funnelWidget->stages()[0].reachedCount, 9);
    QCOMPARE(FormatHelper::funnelStageLabel(funnelWidget->stages()[1].status), u"HR Screen"_s);
    QCOMPARE(funnelWidget->stages()[1].reachedCount, 6);
    QCOMPARE(FormatHelper::funnelStageLabel(funnelWidget->stages()[2].status), u"Technical"_s);
    QCOMPARE(funnelWidget->stages()[2].reachedCount, 4);
    QCOMPARE(FormatHelper::funnelStageLabel(funnelWidget->stages()[3].status), u"Final"_s);
    QCOMPARE(funnelWidget->stages()[3].reachedCount, 2);
    QCOMPARE(FormatHelper::funnelStageLabel(funnelWidget->stages()[4].status), u"Offer"_s);
    QCOMPARE(funnelWidget->stages()[4].reachedCount, 1);
    QCOMPARE(FormatHelper::formatInterviewRate(funnelWidget->interviewRate()), u"66.7%"_s);
    QVERIFY(funnelWidget->accessibleDescription().contains(u"Interview rate: 66.7%"_s));
}

void tst_UiSmoke::testDashboardEmptyStateActionButtons() {
    AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.resize(1100, 700);
    win.show();
    QCoreApplication::processEvents();

    auto* emptyState = win.homePage()->findChild<JobPrep::Ui::Widgets::EmptyState*>();
    QVERIFY(emptyState != nullptr);
    QVERIFY(emptyState->isVisible());

    // Button 1: "+ Add application" -> exactly one ApplicationDialog
    withNextModal([](QWidget* modal) {
        auto* dlg = qobject_cast<ApplicationDialog*>(modal);
        if (dlg) dlg->reject();
    });
    auto* appBtn = emptyState->findChild<QPushButton*>();
    QVERIFY(appBtn != nullptr);
    appBtn->click();
    QCoreApplication::processEvents();
    QCOMPARE(countApplicationDialogs(), 0);

    // Button 2: "+ Add topic" with no tracks -> emits addTopicRequested -> TrackDialog
    withNextModal([](QWidget* modal) {
        if (auto* dlg = qobject_cast<TrackDialog*>(modal)) dlg->reject();
    });
    emit emptyState->secondaryActionClicked();
    QCoreApplication::processEvents();

    // Button 2: "+ Add topic" with tracks -> switches to Study page, Backlog quick-add focused, NO dialog
    Track track;
    track.name = u"C++"_s;
    track.color = u"#3b82f6"_s;
    QVERIFY(ctx.tracks().insert(track));

    // Switch back to HomePage
    win.sidebar()->setCurrentPage(0);
    emit win.sidebar()->pageSelected(0);
    QCoreApplication::processEvents();
    QCOMPARE(win.pageStack()->currentIndex(), 0);

    bool unexpectedDialogShown = false;
    withNextModal([&unexpectedDialogShown](QWidget*) {
        unexpectedDialogShown = true;
    });
    emit emptyState->secondaryActionClicked();
    QCoreApplication::processEvents();

    QCOMPARE(win.pageStack()->currentIndex(), 1); // StudyPage
    QVERIFY(!unexpectedDialogShown);
}

void tst_UiSmoke::testDashboardTodayItemClicksOpenRespectiveDialogs() {
    AppContext ctx(u":memory:"_s);
    const QDate today(2026, 10, 29);
    const QDateTime now(today, QTime(10, 0));
    ctx.statsService().setClock([now] { return now; });
    ctx.dayWatcher().setClock([today] { return today; });

    // 1 track & 1 topic
    Track track; track.name = u"C++"_s; track.color = u"#3b82f6"_s;
    QVERIFY(ctx.tracks().insert(track));
    Topic topic; topic.trackId = track.id; topic.title = u"Lambdas"_s;
    topic.targetDate = today; topic.status = TopicStatus::InProgress;
    QVERIFY(ctx.topics().insert(topic));

    // 1 application with follow-up today
    JobApplication app; app.company = u"Initech"_s; app.position = u"Dev"_s;
    app.status = ApplicationStatus::Applied; app.nextActionDate = today;
    app.nextAction = u"Call HR"_s;
    QVERIFY(ctx.applications().insert(app));

    // 1 interview today
    Interview iv; iv.applicationId = app.id;
    iv.startAt = QDateTime(today, QTime(14, 0));
    QVERIFY(ctx.interviews().insert(iv));

    MainWindow win(ctx);
    win.resize(1100, 700);
    win.show();
    win.dashboardController()->refreshNow();
    QCoreApplication::processEvents();

    auto* todayView = win.homePage()->findChild<QListView*>();
    QVERIFY(todayView != nullptr);
    auto* model = qobject_cast<JobPrep::Models::AgendaListModel*>(todayView->model());
    QVERIFY(model != nullptr);
    QCOMPARE(model->rowCount(), 3);

    // 1. Activate Interview
    withNextModal([](QWidget* modal) {
        auto* dlg = qobject_cast<InterviewDialog*>(modal);
        if (dlg) dlg->reject();
    });
    win.openInterview(iv.id);
    QCoreApplication::processEvents();

    // 2. Activate Follow-up (Application)
    withNextModal([](QWidget* modal) {
        auto* dlg = qobject_cast<ApplicationDialog*>(modal);
        if (dlg) dlg->reject();
    });
    win.openApplication(app.id);
    QCoreApplication::processEvents();

    // 3. Activate Topic
    win.openTopic(topic.id);
    QCoreApplication::processEvents();
    QCOMPARE(win.pageStack()->currentIndex(), 1);

    // 4. Test deleted items: delete from DB and try opening
    QVERIFY(ctx.interviews().remove(iv.id));
    QVERIFY(ctx.applications().remove(app.id));
    QVERIFY(ctx.topics().remove(topic.id));

    win.openInterview(iv.id);
    QCoreApplication::processEvents();
    win.openApplication(app.id);
    QCoreApplication::processEvents();
    win.openTopic(topic.id);
    QCoreApplication::processEvents();
}

void tst_UiSmoke::testOpenInterviewAndApplicationFromAnotherPage() {
    AppContext ctx(u":memory:"_s);
    JobApplication app; app.company = u"Acme"_s; app.position = u"Dev"_s;
    QVERIFY(ctx.applications().insert(app));
    Interview iv; iv.applicationId = app.id; iv.startAt = QDateTime::currentDateTime().addDays(1);
    QVERIFY(ctx.interviews().insert(iv));

    MainWindow win(ctx);
    win.resize(1100, 700);
    win.show();
    QCoreApplication::processEvents();

    // From StudyPage (index 1), call openInterview
    win.sidebar()->setCurrentPage(1);
    emit win.sidebar()->pageSelected(1);
    QCoreApplication::processEvents();
    QCOMPARE(win.pageStack()->currentIndex(), 1);

    withNextModal([](QWidget* modal) {
        if (auto* dlg = qobject_cast<InterviewDialog*>(modal)) dlg->reject();
    });
    win.openInterview(iv.id);
    QCoreApplication::processEvents();
    QCOMPARE(win.pageStack()->currentIndex(), 3); // CalendarPage

    // From CalendarPage (index 3), call openApplication
    win.sidebar()->setCurrentPage(3);
    emit win.sidebar()->pageSelected(3);
    QCoreApplication::processEvents();
    QCOMPARE(win.pageStack()->currentIndex(), 3);

    withNextModal([](QWidget* modal) {
        if (auto* dlg = qobject_cast<ApplicationDialog*>(modal)) dlg->reject();
    });
    win.openApplication(app.id);
    QCoreApplication::processEvents();
    QCOMPARE(win.pageStack()->currentIndex(), 2); // JobsPage

    // Call with invalid ID
    win.openInterview(-999);
    QCoreApplication::processEvents();
    win.openApplication(-999);
    QCoreApplication::processEvents();
}

void tst_UiSmoke::testDashboardDayChangeRefreshesTodayAndStreak() {
    AppContext ctx(u":memory:"_s);
    QDate currentDay(2026, 10, 20);
    ctx.statsService().setDateClock([&currentDay] { return currentDay; });
    ctx.dayWatcher().setClock([&currentDay] { return currentDay; });

    MainWindow win(ctx);
    win.resize(1100, 700);
    win.show();
    win.dashboardController()->refreshNow();
    QCoreApplication::processEvents();

    QCOMPARE(win.dashboardController()->snapshot().today, currentDay);

    // Advance day
    currentDay = QDate(2026, 10, 21);
    ctx.dayWatcher().checkNow();
    QCoreApplication::processEvents();

    QCOMPARE(win.dashboardController()->snapshot().today, currentDay);
}

void tst_UiSmoke::testBurstChangesCoalesceToOneRefresh() {
    AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.resize(1100, 700);
    win.show();
    QCoreApplication::processEvents();

    auto* ctrl = win.dashboardController();
    QVERIFY(ctrl != nullptr);
    const int countBefore = ctrl->refreshCount();

    // Fire 10 repository changed signals during the same event loop turn
    emit ctx.applications().changed();
    emit ctx.interviews().changed();
    emit ctx.sessions().changed();
    emit ctx.topics().changed();
    emit ctx.tracks().changed();
    emit ctx.applications().changed();
    emit ctx.interviews().changed();
    emit ctx.sessions().changed();
    emit ctx.topics().changed();
    emit ctx.tracks().changed();

    // Before processing events, no refresh has run yet
    QCOMPARE(ctrl->refreshCount(), countBefore);

    // Process events
    QCoreApplication::processEvents();

    // Exactly one refresh ran
    QCOMPARE(ctrl->refreshCount(), countBefore + 1);
}

void tst_UiSmoke::testWeeklyGoalChangeUpdatesBarAndTextImmediately() {
    AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.resize(1100, 700);
    win.show();
    QCoreApplication::processEvents();

    ctx.settings().setWeeklyGoalMinutes(360);
    QCoreApplication::processEvents();
    QCOMPARE(win.dashboardController()->snapshot().weeklyGoalMinutes, 360);

    ctx.settings().setWeeklyGoalMinutes(480);
    QCoreApplication::processEvents();
    QCOMPARE(win.dashboardController()->snapshot().weeklyGoalMinutes, 480);
}

void tst_UiSmoke::testFirstDayOfWeekChangeRefreshesWeekNumbers() {
    AppContext ctx(u":memory:"_s);
    const QDateTime now(QDate(2026, 10, 25), QTime(10, 0)); // Sunday Oct 25
    ctx.statsService().setClock([now] { return now; });
    ctx.dayWatcher().setClock([now] { return now.date(); });
    ctx.settings().setFirstDayOfWeek(1);

    // Session on Sunday Oct 25 = 60m
    StudySession sess; sess.sessionDate = QDate(2026, 10, 25); sess.minutes = 60;
    QVERIFY(ctx.sessions().insert(sess));

    MainWindow win(ctx);
    win.resize(1100, 700);
    win.show();

    // Monday first (1): Sunday Oct 25 is end of previous week
    ctx.settings().setFirstDayOfWeek(1);
    win.dashboardController()->refreshNow();
    QCoreApplication::processEvents();
    const auto statsMon = win.dashboardController()->snapshot().stats;
    QCOMPARE(statsMon.weekMinutes, 60);

    // Sunday first (7): Sunday Oct 25 is start of current week
    ctx.settings().setFirstDayOfWeek(7);
    QCoreApplication::processEvents();
    const auto statsSun = win.dashboardController()->snapshot().stats;
    QCOMPARE(statsSun.weekMinutes, 60);
}

void tst_UiSmoke::testStatusBarTextScenarios() {
    AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.resize(1100, 700);
    win.show();
    QCoreApplication::processEvents();

    auto* status = win.statusBar();
    QVERIFY(status != nullptr);

    // Add 1 interview
    JobApplication app; app.company = u"WayneTech"_s; app.position = u"Engineer"_s;
    QVERIFY(ctx.applications().insert(app));
    Interview iv; iv.applicationId = app.id;
    iv.startAt = QDateTime(QDate(2026, 10, 30), QTime(14, 0));
    QVERIFY(ctx.interviews().insert(iv));

    win.dashboardController()->refreshNow();
    QCoreApplication::processEvents();
    const auto snap = win.dashboardController()->snapshot();
    QVERIFY(snap.nextInterview.has_value());
    const QString nextText = JobPrep::Ui::FormatHelper::formatNextInterview(snap.nextInterview, QLocale(QLocale::English));
    QVERIFY(nextText.contains(u"WayneTech"_s));
}

void tst_UiSmoke::testWidgetMetricsAndRingWrapping() {
    AppContext ctx(u":memory:"_s);
    auto& theme = ctx.themeManager();

    // 1. StatCard sizing
    JobPrep::Ui::Widgets::StatCard card(theme);
    card.setTitle(u"Study streak"_s);
    card.setValue(u"4-day streak"_s);
    QVERIFY(card.sizeHint().width() > 0);
    QVERIFY(card.sizeHint().height() > 0);
    QCOMPARE(card.minimumSizeHint(), card.sizeHint());

    // 2. GoalProgressBar
    JobPrep::Ui::Widgets::GoalProgressBar goalBar(theme);
    goalBar.setGoal(430, 360);
    QCOMPARE(goalBar.weekMinutes(), 430);
    QCOMPARE(goalBar.goalMinutes(), 360);
    QVERIFY(goalBar.sizeHint().height() > 0);

    // 3. FunnelWidget
    JobPrep::Ui::Widgets::FunnelWidget funnel(theme);
    funnel.setFunnel({{ApplicationStatus::Applied, 9}, {ApplicationStatus::HrScreen, 6}}, 66.7);
    QVERIFY(funnel.sizeHint().width() > 0);
    QVERIFY(funnel.minimumSizeHint().width() > 0);

    // 4. ProgressRing: 1, 2, and 8 tracks
    JobPrep::Ui::Widgets::ProgressRing ring(theme);
    ring.resize(600, 400);

    // 1 track
    ring.setTracks({{1, u"C++"_s, u"#3b82f6"_s, 50}});
    QRect r0 = ring.ringRect(0, 600);
    QVERIFY(r0.width() > 0 && r0.height() > 0);

    // 2 tracks: no overlap
    ring.setTracks({{1, u"C++"_s, u"#3b82f6"_s, 50}, {2, u"English"_s, u"#10b981"_s, 75}});
    QRect r1_0 = ring.ringRect(0, 600);
    QRect r1_1 = ring.ringRect(1, 600);
    QVERIFY(!r1_0.intersects(r1_1));

    // 8 tracks: wrapping and no overlap
    QList<JobPrep::Services::TrackProgressStat> eightTracks;
    for (int i = 0; i < 8; ++i) {
        eightTracks.append({i, u"Track %1"_s.arg(i), u"#3b82f6"_s, i * 10});
    }
    ring.setTracks(eightTracks);
    for (int i = 0; i < 8; ++i) {
        const QRect ri = ring.ringRect(i, 600);
        for (int j = i + 1; j < 8; ++j) {
            const QRect rj = ring.ringRect(j, 600);
            QVERIFY2(!ri.intersects(rj), qPrintable(u"Ring %1 overlaps with ring %2"_s.arg(i).arg(j)));
        }
    }
}

void tst_UiSmoke::testDashboardIgnoresThemeSidebarAndSplitterChanges() {
    AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.resize(1100, 700);
    win.show();
    QCoreApplication::processEvents();

    const int initialCount = win.dashboardController()->refreshCount();
    ctx.settings().setThemeMode(u"dark"_s);
    ctx.settings().setSidebarCollapsed(true);
    ctx.settings().setStudySplitterState(QByteArray("fake_splitter_state"));
    QCoreApplication::processEvents();

    // Must NOT have triggered a dashboard refresh
    QCOMPARE(win.dashboardController()->refreshCount(), initialCount);

    // Activating HomePage MUST trigger requestRefresh()
    win.homePage()->onActivated();
    QCoreApplication::processEvents();
    QCOMPARE(win.dashboardController()->refreshCount(), initialCount + 1);
}

void tst_UiSmoke::testDashboardScreenshots() {
    const QString dir = qEnvironmentVariable("JOBPREP_REPORT_IMG_DIR");
    if (dir.isEmpty()) QSKIP("JOBPREP_REPORT_IMG_DIR is not set");
    QVERIFY(QDir().mkpath(dir));

    // 1. Dashboard with data
    {
        AppContext ctx(u":memory:"_s);
        const QDateTime now(QDate(2026, 10, 29), QTime(10, 0));
        ctx.statsService().setClock([now] { return now; });
        ctx.dayWatcher().setClock([now] { return now.date(); });

        Track t1; t1.name = u"C++"_s; t1.color = u"#3b82f6"_s; QVERIFY(ctx.tracks().insert(t1));
        Track t2; t2.name = u"English"_s; t2.color = u"#10b981"_s; QVERIFY(ctx.tracks().insert(t2));
        Topic tp1; tp1.trackId = t1.id; tp1.title = u"Smart Pointers"_s; tp1.status = TopicStatus::Done; QVERIFY(ctx.topics().insert(tp1));
        Topic tp2; tp2.trackId = t2.id; tp2.title = u"Behavioral Questions"_s; tp2.status = TopicStatus::InProgress; tp2.targetDate = now.date(); QVERIFY(ctx.topics().insert(tp2));

        JobApplication a1; a1.company = u"Acme Corp"_s; a1.position = u"C++ Engineer"_s; a1.status = ApplicationStatus::Technical; a1.appliedDate = QDate(2026, 10, 27); QVERIFY(ctx.applications().insert(a1));
        JobApplication a2; a2.company = u"Globex"_s; a2.position = u"Systems Dev"_s; a2.status = ApplicationStatus::Applied; a2.appliedDate = QDate(2026, 10, 29); a2.nextActionDate = now.date(); a2.nextAction = u"Send code samples"_s; QVERIFY(ctx.applications().insert(a2));
        JobApplication a3; a3.company = u"Initech"_s; a3.position = u"Senior Architect"_s; a3.status = ApplicationStatus::Final; a3.appliedDate = QDate(2026, 10, 15); QVERIFY(ctx.applications().insert(a3));

        StudySession s1; s1.sessionDate = QDate(2026, 10, 26); s1.minutes = 60; QVERIFY(ctx.sessions().insert(s1));
        StudySession s2; s2.sessionDate = QDate(2026, 10, 27); s2.minutes = 120; QVERIFY(ctx.sessions().insert(s2));
        StudySession s3; s3.sessionDate = QDate(2026, 10, 28); s3.minutes = 80; QVERIFY(ctx.sessions().insert(s3));

        Interview iv1; iv1.applicationId = a1.id; iv1.startAt = QDateTime(QDate(2026, 10, 29), QTime(8, 0)); iv1.type = InterviewType::Hr; QVERIFY(ctx.interviews().insert(iv1));
        Interview iv2; iv2.applicationId = a1.id; iv2.startAt = QDateTime(QDate(2026, 10, 30), QTime(14, 0)); iv2.type = InterviewType::Technical; QVERIFY(ctx.interviews().insert(iv2));
        Interview iv3; iv3.applicationId = a3.id; iv3.startAt = QDateTime(QDate(2026, 11, 4), QTime(11, 0)); iv3.type = InterviewType::Final; QVERIFY(ctx.interviews().insert(iv3));

        MainWindow win(ctx);
        win.resize(1100, 700);
        win.show();
        win.dashboardController()->refreshNow();
        QCoreApplication::processEvents();

        for (auto mode : {JobPrep::Ui::Theme::ThemeMode::Light, JobPrep::Ui::Theme::ThemeMode::Dark}) {
            ctx.themeManager().setThemeMode(mode);
            QCoreApplication::processEvents();
            const QImage img = win.homePage()->grab().toImage();
            QVERIFY(!img.isNull());
            const QString fileName = (mode == JobPrep::Ui::Theme::ThemeMode::Light)
                                         ? u"dashboard-with-data-light.png"_s
                                         : u"dashboard-with-data-dark.png"_s;
            QVERIFY(img.save(QDir(dir).filePath(fileName)));
        }
    }

    // 2. Empty dashboard
    {
        AppContext ctx(u":memory:"_s);
        MainWindow win(ctx);
        win.resize(1100, 700);
        win.show();
        win.dashboardController()->refreshNow();
        QCoreApplication::processEvents();

        for (auto mode : {JobPrep::Ui::Theme::ThemeMode::Light, JobPrep::Ui::Theme::ThemeMode::Dark}) {
            ctx.themeManager().setThemeMode(mode);
            QCoreApplication::processEvents();
            const QImage img = win.homePage()->grab().toImage();
            QVERIFY(!img.isNull());
            const QString fileName = (mode == JobPrep::Ui::Theme::ThemeMode::Light)
                                         ? u"dashboard-empty-light.png"_s
                                         : u"dashboard-empty-dark.png"_s;
            QVERIFY(img.save(QDir(dir).filePath(fileName)));
        }
    }
}

void tst_UiSmoke::testZeroWarnings() {
    if (!s_capturedWarnings.isEmpty()) {
        const QString failures = s_capturedWarnings.join(u"\n"_s);
        QFAIL(qPrintable(u"Captured unexpected Qt warnings/criticals:\n"_s + failures));
    }
    QVERIFY(s_capturedWarnings.isEmpty());
}

void tst_UiSmoke::cleanupTestCase() {
    m_study.reset();
    m_studyCtx.reset();
    m_jobs.reset();
    m_jobsCtx.reset();
    m_calendar.reset();
    m_calCtx.reset();
    m_window.reset();
    m_ctx.reset();
    qInstallMessageHandler(nullptr);
}

QTEST_MAIN(tst_UiSmoke)
#include "tst_UiSmoke.moc"