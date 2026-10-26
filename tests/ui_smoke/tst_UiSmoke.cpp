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

class tst_UiSmoke : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testWindowCreationAndNavigation();
    void testThemeAndAccentSwitching();
    void testSidebarCollapsing();
    void testNewEntryPointsOpenOneDialog();
    void testPageSwitchShortcutsRealKeyEvents();
    void testPreferencesShortcutRealKeyEvents();
    void testZeroWarnings();
    void cleanupTestCase();

private:
    QTemporaryDir m_tempDir;
    std::unique_ptr<JobPrep::App::AppContext> m_ctx;
    std::unique_ptr<JobPrep::Ui::MainWindow> m_window;
};

void tst_UiSmoke::initTestCase() {
    JobPrep::UiTest::installWarningFilter();
    JobPrep::UiTest::isolateSettings(m_tempDir);

    m_ctx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    QVERIFY(m_ctx->database().isOpen());
    m_window = std::make_unique<JobPrep::Ui::MainWindow>(*m_ctx);
    m_window->show();
    QCoreApplication::processEvents();
}

void tst_UiSmoke::cleanupTestCase() {
    m_window.reset();
    m_ctx.reset();
    JobPrep::UiTest::uninstallWarningFilter();
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

    const int initialCount = m_ctx->applications().count();

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
    QCOMPARE(m_ctx->applications().count(), initialCount);
}


void tst_UiSmoke::testPageSwitchShortcutsRealKeyEvents() {
    QVERIFY(m_window != nullptr);
    QVERIFY(JobPrep::UiTest::activateWindow(m_window.get()));

    // Start on home (page 0)
    m_window->sidebar()->setCurrentPage(0);
    emit m_window->sidebar()->pageSelected(0);
    QCOMPARE(m_window->pageStack()->currentIndex(), 0);

    // Ctrl+2 -> Jobs (page 1)
    QVERIFY(JobPrep::UiTest::sendKeySequence(m_window.get(), QKeySequence(Qt::CTRL | Qt::Key_2)));
    QCOMPARE(m_window->pageStack()->currentIndex(), 1);

    // Ctrl+3 -> Calendar (page 2)
    QVERIFY(JobPrep::UiTest::sendKeySequence(m_window.get(), QKeySequence(Qt::CTRL | Qt::Key_3)));
    QCOMPARE(m_window->pageStack()->currentIndex(), 2);

    // Ctrl+4 -> Study (page 3)
    QVERIFY(JobPrep::UiTest::sendKeySequence(m_window.get(), QKeySequence(Qt::CTRL | Qt::Key_4)));
    QCOMPARE(m_window->pageStack()->currentIndex(), 3);

    // Ctrl+1 -> Home (page 0)
    QVERIFY(JobPrep::UiTest::sendKeySequence(m_window.get(), QKeySequence(Qt::CTRL | Qt::Key_1)));
    QCOMPARE(m_window->pageStack()->currentIndex(), 0);
}

void tst_UiSmoke::testPreferencesShortcutRealKeyEvents() {
    QVERIFY(m_window != nullptr);
    QVERIFY(JobPrep::UiTest::activateWindow(m_window.get()));

    // Start on home (page 0)
    m_window->sidebar()->setCurrentPage(0);
    emit m_window->sidebar()->pageSelected(0);
    QCOMPARE(m_window->pageStack()->currentIndex(), 0);

    // Ctrl+, switches to Settings (page 4) per SPEC §3.7
    QVERIFY(JobPrep::UiTest::sendKeySequence(m_window.get(), QKeySequence(Qt::CTRL | Qt::Key_Comma)));
    QCOMPARE(m_window->pageStack()->currentIndex(), 4);
}

void tst_UiSmoke::testZeroWarnings() {
    const QStringList warnings = capturedWarnings();
    if (!warnings.isEmpty()) {
        const QString failures = warnings.join(u"\n"_s);
        QFAIL(qPrintable(u"Captured unexpected Qt warnings/criticals:\n"_s + failures));
    }
    QVERIFY(warnings.isEmpty());
}


QTEST_MAIN(tst_UiSmoke)
#include "tst_UiSmoke.moc"
