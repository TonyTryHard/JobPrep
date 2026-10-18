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
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
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
#include <QTimer>
#include <QToolButton>
#include "app/AppContext.h"
#include "data/ApplicationRepository.h"
#include "data/Database.h"
#include "data/InterviewRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/Structs.h"
#include "models/AgendaListModel.h"
#include "models/ApplicationTableModel.h"
#include "models/TopicListModel.h"
#include "services/ExportService.h"
#include "services/SettingsService.h"
#include "ui/MainWindow.h"
#include "ui/Sidebar.h"
#include "ui/dialogs/ApplicationDialog.h"
#include "ui/dialogs/FollowUpDialog.h"
#include "ui/dialogs/InterviewDialog.h"
#include "ui/pages/CalendarPage.h"
#include "ui/pages/JobsPage.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"
#include "ui/widgets/ChipBar.h"
#include "ui/widgets/MonthView.h"
#include "ui/widgets/SelectionRestorer.h"

using namespace Qt::StringLiterals;

namespace {

using JobPrep::Domain::AgendaKind;
using JobPrep::Domain::ApplicationStatus;
using JobPrep::Domain::Interview;
using JobPrep::Domain::InterviewType;
using JobPrep::Domain::JobApplication;
using JobPrep::Domain::Topic;
using JobPrep::Domain::Track;
using JobPrep::Models::ApplicationTableModel;
using JobPrep::Ui::Dialogs::ApplicationDialog;
using JobPrep::Ui::Dialogs::FollowUpDialog;
using JobPrep::Ui::Dialogs::InterviewDialog;

QStringList s_capturedWarnings;

/// The offscreen platform plugin cannot forward window size hints, so Qt logs this once
/// per shown dialog. It is a limitation of the test platform plugin, not of our code,
/// so it must not fail the zero-warning gate.
bool isPlatformPluginNoise(const QString& msg) {
    return msg.contains(u"does not support propagateSizeHints"_s);
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

    std::unique_ptr<JobPrep::App::AppContext> m_ctx;
    std::unique_ptr<JobPrep::Ui::MainWindow> m_window;
    std::unique_ptr<JobPrep::App::AppContext> m_jobsCtx;
    std::unique_ptr<JobPrep::Ui::Pages::JobsPage> m_jobs;
    std::unique_ptr<JobPrep::App::AppContext> m_calCtx;
    std::unique_ptr<JobPrep::Ui::Pages::CalendarPage> m_calendar;
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
        m_calCtx->interviews(), m_calCtx->applications(), m_calCtx->topics(),
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
    auto* interviewAction = m_window->findChild<QAction*>(u"HeaderNewInterview"_s);
    auto* sessionAction = m_window->findChild<QAction*>(u"HeaderNewSession"_s);
    auto* pageAddBtn = m_window->findChild<QPushButton*>(u"JobsAddBtn"_s);
    QVERIFY(newBtn != nullptr);
    QVERIFY(newBtn->menu() != nullptr);
    QVERIFY(applicationAction != nullptr);
    QVERIFY(pageAddBtn != nullptr);

    // Application and Interview both work now; Study session says which milestone brings it.
    QVERIFY(applicationAction->isEnabled());
    QVERIFY(interviewAction != nullptr && interviewAction->isEnabled());
    QVERIFY(sessionAction != nullptr && !sessionAction->isEnabled());
    QCOMPARE(sessionAction->toolTip(), u"Arrives in M4");

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
        m_calCtx->interviews(), m_calCtx->applications(), m_calCtx->topics(),
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

void tst_UiSmoke::testZeroWarnings() {
    if (!s_capturedWarnings.isEmpty()) {
        const QString failures = s_capturedWarnings.join(u"\n"_s);
        QFAIL(qPrintable(u"Captured unexpected Qt warnings/criticals:\n"_s + failures));
    }
    QVERIFY(s_capturedWarnings.isEmpty());
}

void tst_UiSmoke::cleanupTestCase() {
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