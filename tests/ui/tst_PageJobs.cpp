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

class tst_PageJobs : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
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
    void testJobsPageThemes();
    void testHeaderSearchFiltersJobsTable();
    void testHeaderNewInterviewFromJobsRoutesToCalendar();
    void testHeaderNewApplicationOnCalendarPage();
    void testJobsContextMenuAddInterviewEndToEnd();

private:
    void resetJobsData();
    JobPrep::Ui::Pages::JobsPage* jobsPage() { return m_jobs.get(); }
    QTableView* jobsTable() const;
    void addApplication(const QString& company, const QString& position,
                        ApplicationStatus status = ApplicationStatus::Applied,
                        const QString& notes = QString());
    QString companyAt(int row) const;
    int idAt(int row) const;
    void saveNextDialog(const QString& company, const QString& position);
    void clickChip(int index);

    QTemporaryDir m_tempDir;
    std::unique_ptr<JobPrep::App::AppContext> m_ctx;
    std::unique_ptr<JobPrep::Ui::MainWindow> m_window;
    std::unique_ptr<JobPrep::App::AppContext> m_jobsCtx;
    std::unique_ptr<JobPrep::Ui::Pages::JobsPage> m_jobs;
};

void tst_PageJobs::initTestCase() {
    JobPrep::UiTest::installWarningFilter();
    JobPrep::UiTest::isolateSettings(m_tempDir);

    m_ctx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    QVERIFY(m_ctx->database().isOpen());
    m_window = std::make_unique<JobPrep::Ui::MainWindow>(*m_ctx);
    m_window->show();
    QCoreApplication::processEvents();
}

void tst_PageJobs::cleanupTestCase() {
    m_jobs.reset();
    m_jobsCtx.reset();
    m_window.reset();
    m_ctx.reset();
    JOBPREP_VERIFY_ZERO_WARNINGS();
    JobPrep::UiTest::uninstallWarningFilter();
}

void tst_PageJobs::resetJobsData() {
    m_jobs.reset();
    m_jobsCtx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    QVERIFY(m_jobsCtx->database().isOpen());
    m_jobs = std::make_unique<JobPrep::Ui::Pages::JobsPage>(m_jobsCtx->applications(),
                                                           m_jobsCtx->interviews(),
                                                           m_jobsCtx->exportService());
    m_jobs->show();
}


QTableView* tst_PageJobs::jobsTable() const {
    return childNamed<QTableView>(m_jobs.get(), u"JobsTableView"_s);
}


void tst_PageJobs::addApplication(const QString& company, const QString& position,
                                 ApplicationStatus status, const QString& notes) {
    JobApplication app;
    app.company = company;
    app.position = position;
    app.status = status;
    app.notes = notes;
    QVERIFY(m_jobsCtx->applications().insert(app));
}


QString tst_PageJobs::companyAt(int row) const {
    return jobsTable()->model()->index(row, ApplicationTableModel::CompanyColumn).data().toString();
}


int tst_PageJobs::idAt(int row) const {
    return jobsTable()->model()
        ->index(row, ApplicationTableModel::CompanyColumn)
        .data(ApplicationTableModel::IdRole)
        .toInt();
}


void tst_PageJobs::saveNextDialog(const QString& company, const QString& position) {
    withNextModal([company, position](QWidget* modal) {
        auto* dialog = qobject_cast<ApplicationDialog*>(modal);
        if (!dialog) return;
        childNamed<QLineEdit>(dialog, u"FieldCompany"_s)->setText(company);
        childNamed<QLineEdit>(dialog, u"FieldPosition"_s)->setText(position);
        buttonNamed(dialog, u"DialogSaveBtn"_s)->click();
    });
}


void tst_PageJobs::clickChip(int index) {
    auto* chips = childNamed<JobPrep::Ui::Widgets::ChipBar>(m_jobs.get(), u"JobsStatusChips"_s);
    QVERIFY(chips != nullptr);
    const auto buttons = chips->findChildren<QPushButton*>();
    QCOMPARE(buttons.size(), 5);
    buttons.at(index)->click();
}


void tst_PageJobs::testJobsPageEmptyStates() {
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


void tst_PageJobs::testJobsPageAddAndEdit() {
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


void tst_PageJobs::testJobsPageDuplicateAndDelete() {
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


void tst_PageJobs::testJobsPageStatusChangeAppearsInTimeline() {
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


void tst_PageJobs::testJobsPageSortAndFilter() {
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


void tst_PageJobs::testJobsPageCsvExport() {
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


void tst_PageJobs::testJobsPageSelectionKeptAcrossReloads() {
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


void tst_PageJobs::testApplicationDialogTabs() {
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


void tst_PageJobs::testApplicationDialogValidation() {
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


void tst_PageJobs::testApplicationDialogCancelKeepsEditing() {
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


void tst_PageJobs::testInterviewDialogCreateAndEdit() {
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


void tst_PageJobs::testInterviewDialogValidation() {
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


void tst_PageJobs::testInterviewDialogDeleteWithConfirmation() {
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


void tst_PageJobs::testInterviewDialogKeyboardShortcuts() {
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


void tst_PageJobs::testInterviewTabDisabledUntilSaved() {
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


void tst_PageJobs::testApplicationDialogInterviewsTabEditable() {
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


void tst_PageJobs::testJobsPageThemes() {
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


void tst_PageJobs::testHeaderSearchFiltersJobsTable() {
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


void tst_PageJobs::testHeaderNewInterviewFromJobsRoutesToCalendar() {
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


void tst_PageJobs::testHeaderNewApplicationOnCalendarPage() {
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


void tst_PageJobs::testJobsContextMenuAddInterviewEndToEnd() {
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


QTEST_MAIN(tst_PageJobs)
#include "tst_PageJobs.moc"
