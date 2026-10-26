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

class tst_PageHome : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
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
    void testToastNotificationSmoke();
    void testToastScreenshots();

private:
    QTemporaryDir m_tempDir;
    std::unique_ptr<JobPrep::App::AppContext> m_ctx;
    std::unique_ptr<JobPrep::Ui::MainWindow> m_window;
};

void tst_PageHome::initTestCase() {
    JobPrep::UiTest::installWarningFilter();
    JobPrep::UiTest::isolateSettings(m_tempDir);

    m_ctx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    QVERIFY(m_ctx->database().isOpen());
    m_window = std::make_unique<JobPrep::Ui::MainWindow>(*m_ctx);
    m_window->show();
    QCoreApplication::processEvents();
}

void tst_PageHome::cleanupTestCase() {
    m_window.reset();
    m_ctx.reset();
    JOBPREP_VERIFY_ZERO_WARNINGS();
    JobPrep::UiTest::uninstallWarningFilter();
}

void tst_PageHome::testFormatHelper() {
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


void tst_PageHome::testDashboardVisitedBothThemes() {
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


void tst_PageHome::testDashboardNumbersMatchHandCalculatedDataset() {
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


void tst_PageHome::testDashboardEmptyStateActionButtons() {
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


void tst_PageHome::testDashboardTodayItemClicksOpenRespectiveDialogs() {
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


void tst_PageHome::testOpenInterviewAndApplicationFromAnotherPage() {
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


void tst_PageHome::testDashboardDayChangeRefreshesTodayAndStreak() {
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


void tst_PageHome::testBurstChangesCoalesceToOneRefresh() {
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


void tst_PageHome::testWeeklyGoalChangeUpdatesBarAndTextImmediately() {
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


void tst_PageHome::testFirstDayOfWeekChangeRefreshesWeekNumbers() {
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


void tst_PageHome::testStatusBarTextScenarios() {
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


void tst_PageHome::testWidgetMetricsAndRingWrapping() {
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


void tst_PageHome::testDashboardIgnoresThemeSidebarAndSplitterChanges() {
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


void tst_PageHome::testDashboardScreenshots() {
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


void tst_PageHome::testToastNotificationSmoke() {
    AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.resize(1100, 700);
    win.showNormal();
    win.show();
    QCoreApplication::processEvents();

    InAppNotifier notifier(win, win.themeManager());
    Notification n;
    n.kind = NotificationKind::Interview;
    n.company = u"Acme Smoke Corp"_s;
    n.position = u"Software Engineer"_s;
    n.interviewType = InterviewType::Technical;
    n.leadMinutes = 60;

    QVERIFY(notifier.notify(n));
    QCoreApplication::processEvents();
    QCOMPARE(notifier.visibleToastCount(), 1);

    for (auto mode : {JobPrep::Ui::Theme::ThemeMode::Light, JobPrep::Ui::Theme::ThemeMode::Dark}) {
        ctx.themeManager().setThemeMode(mode);
        QCoreApplication::processEvents();
    }

    notifier.dismissNewest();
    QCoreApplication::processEvents();
    QCOMPARE(notifier.totalToastCount(), 0);
}


void tst_PageHome::testToastScreenshots() {
    const QString dir = qEnvironmentVariable("JOBPREP_REPORT_IMG_DIR");
    if (dir.isEmpty()) QSKIP("JOBPREP_REPORT_IMG_DIR is not set");
    QVERIFY(QDir().mkpath(dir));

    // 1. Single toast on dashboard
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

        MainWindow win(ctx);
        win.resize(1100, 700);
        win.show();
        win.dashboardController()->refreshNow();
        QCoreApplication::processEvents();

        InAppNotifier notifier(win, win.themeManager());
        Notification n;
        n.kind = NotificationKind::Interview;
        n.company = u"Globex Systems"_s;
        n.position = u"Systems Developer"_s;
        n.interviewType = InterviewType::Technical;
        n.leadMinutes = 60;
        QVERIFY(notifier.notify(n));
        QCoreApplication::processEvents();

        for (auto mode : {JobPrep::Ui::Theme::ThemeMode::Light, JobPrep::Ui::Theme::ThemeMode::Dark}) {
            ctx.themeManager().setThemeMode(mode);
            QCoreApplication::processEvents();
            const QImage img = win.grab().toImage();
            QVERIFY(!img.isNull());
            const QString fileName = (mode == JobPrep::Ui::Theme::ThemeMode::Light)
                                         ? u"dashboard-toast-light.png"_s
                                         : u"dashboard-toast-dark.png"_s;
            QVERIFY(img.save(QDir(dir).filePath(fileName)));
        }
    }

    // 2. Stack of 4 toasts on dashboard
    {
        AppContext ctx(u":memory:"_s);
        const QDateTime now(QDate(2026, 10, 29), QTime(10, 0));
        ctx.statsService().setClock([now] { return now; });
        ctx.dayWatcher().setClock([now] { return now.date(); });

        Track t1; t1.name = u"C++"_s; t1.color = u"#3b82f6"_s; QVERIFY(ctx.tracks().insert(t1));
        Topic tp1; tp1.trackId = t1.id; tp1.title = u"Smart Pointers"_s; tp1.status = TopicStatus::Done; QVERIFY(ctx.topics().insert(tp1));

        JobApplication a1; a1.company = u"Acme Corp"_s; a1.position = u"C++ Engineer"_s; a1.status = ApplicationStatus::Technical; a1.appliedDate = QDate(2026, 10, 27); QVERIFY(ctx.applications().insert(a1));
        JobApplication a2; a2.company = u"Globex"_s; a2.position = u"Systems Dev"_s; a2.status = ApplicationStatus::Applied; a2.appliedDate = QDate(2026, 10, 29); QVERIFY(ctx.applications().insert(a2));

        MainWindow win(ctx);
        win.resize(1100, 700);
        win.show();
        win.dashboardController()->refreshNow();
        QCoreApplication::processEvents();

        InAppNotifier notifier(win, win.themeManager());

        Notification n1;
        n1.kind = NotificationKind::Interview;
        n1.company = u"Globex Systems"_s;
        n1.position = u"Systems Dev"_s;
        n1.interviewType = InterviewType::Technical;
        n1.leadMinutes = 60;
        notifier.notify(n1);

        Notification n2;
        n2.kind = NotificationKind::FollowUp;
        n2.company = u"Initech Inc"_s;
        n2.position = u"QA Automation"_s;
        notifier.notify(n2);

        Notification n3;
        n3.kind = NotificationKind::Interview;
        n3.company = u"Acme Corp"_s;
        n3.position = u"C++ Engineer"_s;
        n3.interviewType = InterviewType::Final;
        n3.leadMinutes = 1440;
        notifier.notify(n3);

        Notification n4;
        n4.kind = NotificationKind::OverdueFollowUps;
        n4.refId = 0;
        n4.count = 2;
        notifier.notify(n4);

        QCoreApplication::processEvents();
        QCOMPARE(notifier.totalToastCount(), 4);
        QCOMPARE(notifier.visibleToastCount(), 3);
        QVERIFY(notifier.isOverflowVisible());

        for (auto mode : {JobPrep::Ui::Theme::ThemeMode::Light, JobPrep::Ui::Theme::ThemeMode::Dark}) {
            ctx.themeManager().setThemeMode(mode);
            QCoreApplication::processEvents();
            const QImage img = win.grab().toImage();
            QVERIFY(!img.isNull());
            const QString fileName = (mode == JobPrep::Ui::Theme::ThemeMode::Light)
                                         ? u"dashboard-toasts-stack-light.png"_s
                                         : u"dashboard-toasts-stack-dark.png"_s;
            QVERIFY(img.save(QDir(dir).filePath(fileName)));
        }
    }
}


QTEST_MAIN(tst_PageHome)
#include "tst_PageHome.moc"
