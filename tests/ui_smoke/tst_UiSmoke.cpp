#include <QApplication>
#include <QFile>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStringList>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextEdit>
#include <QTimer>
#include "app/AppContext.h"
#include "data/ApplicationRepository.h"
#include "data/Database.h"
#include "models/ApplicationTableModel.h"
#include "services/ExportService.h"
#include "ui/MainWindow.h"
#include "ui/Sidebar.h"
#include "ui/dialogs/ApplicationDialog.h"
#include "ui/pages/JobsPage.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"
#include "ui/widgets/ChipBar.h"

using namespace Qt::StringLiterals;

namespace {

using JobPrep::Domain::ApplicationStatus;
using JobPrep::Domain::JobApplication;
using JobPrep::Models::ApplicationTableModel;
using JobPrep::Ui::Dialogs::ApplicationDialog;

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

template <typename T>
T* childNamed(QWidget* root, const QString& objectName) {
    return root->findChild<T*>(objectName);
}

QPushButton* buttonNamed(QWidget* root, const QString& objectName) {
    return childNamed<QPushButton>(root, objectName);
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
    void testJobsPageThemes();
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

    std::unique_ptr<JobPrep::App::AppContext> m_ctx;
    std::unique_ptr<JobPrep::Ui::MainWindow> m_window;
    std::unique_ptr<JobPrep::App::AppContext> m_jobsCtx;
    std::unique_ptr<JobPrep::Ui::Pages::JobsPage> m_jobs;
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
    m_window.reset();
    m_ctx.reset();
    qInstallMessageHandler(nullptr);
}

QTEST_MAIN(tst_UiSmoke)
#include "tst_UiSmoke.moc"