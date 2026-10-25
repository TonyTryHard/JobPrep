#include <memory>
#include <QApplication>
#include <QDateTime>
#include <QKeyEvent>
#include <QLineEdit>
#include <QStackedWidget>
#include <QTest>
#include <QTimer>
#include "app/AppContext.h"
#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"
#include "data/ReminderLogRepository.h"
#include "domain/Enums.h"
#include "domain/Structs.h"
#include "services/INotifier.h"
#include "services/SettingsService.h"
#include "ui/InAppNotifier.h"
#include "ui/MainWindow.h"
#include "ui/NotificationText.h"
#include "ui/NotifierFactory.h"
#include "ui/ReminderCoordinator.h"
#include "ui/Toast.h"
#include "ui/TrayNotifier.h"
#include "ui/dialogs/InterviewDialog.h"

using namespace Qt::StringLiterals;
using namespace JobPrep;
using namespace JobPrep::Domain;
using namespace JobPrep::Services;
using namespace JobPrep::Ui;
using JobPrep::Ui::Dialogs::InterviewDialog;

class tst_Notifiers : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // 1. Notifier Selection
    void makeNotifierSelection();

    // 2. Notification Text formatting
    void notificationTextInterviewLeadTimes();
    void notificationTextFollowUp();
    void notificationTextOverdueSingleAndSummarySingularPlural();

    // 3. InAppNotifier & Toast
    void inAppNotifierHiddenOrMinimizedWindowReturnsFalse();
    void inAppNotifierVisibleWindowShowsToastAndNoAutoDismiss();
    void inAppNotifierFourNotificationsStackThreePlusOverflow();
    void inAppNotifierEscDismissesToast();
    void inAppNotifierClickOpensItem();
    void inAppNotifierFocusWidgetUnchangedOnToast();
    void inAppNotifierRepositionFollowsWindowResize();
    void toastAccessibleNameAndDescription();

    // 4. TrayNotifier offscreen behavior
    void trayNotifierOffscreenBehavior();

    // 5. Coordinator End-to-End
    void coordinatorEndToEndInterview59MinutesAheadOneToastAndOneDbPair();
    void coordinatorEndToEndClickOpensInterviewDialog();
    void coordinatorEndToEndOverdueSummaryClickSwitchesToJobs();
    void coordinatorEndToEndMinimizedWindowDelaysUntilShowNormal();
    void coordinatorEndToEndNoTrayFallbackToToast();
    void coordinatorEndToEndSettingsLeadMinutesChangeTakesEffect();
};

void tst_Notifiers::initTestCase() {
    App::AppContext ctx(u":memory:"_s);
    ctx.settings().setInterviewLeadMinutes({1440, 60});
}

void tst_Notifiers::makeNotifierSelection() {
    App::AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);

    auto inApp = makeNotifier(false, win, {});
    QVERIFY(dynamic_cast<InAppNotifier*>(inApp.get()) != nullptr);

    auto tray = makeNotifier(true, win, {});
    QVERIFY(dynamic_cast<TrayNotifier*>(tray.get()) != nullptr);
}

void tst_Notifiers::notificationTextInterviewLeadTimes() {
    // 60 minutes
    Notification n60;
    n60.kind = NotificationKind::Interview;
    n60.company = u"Globex"_s;
    n60.position = u"Dev"_s;
    n60.interviewType = InterviewType::Technical;
    n60.leadMinutes = 60;

    QCOMPARE(NotificationText::formatTitle(n60), u"Globex"_s);
    QCOMPARE(NotificationText::formatLeadTime(60), u"in 1 hour"_s);
    QVERIFY(NotificationText::formatMessage(n60).contains(u"in 1 hour"_s));
    QVERIFY(NotificationText::formatMessage(n60).contains(u"Technical"_s));

    // 1440 minutes
    Notification n1440;
    n1440.kind = NotificationKind::Interview;
    n1440.company = u"Initech"_s;
    n1440.leadMinutes = 1440;

    QCOMPARE(NotificationText::formatLeadTime(1440), u"in 24 hours"_s);
    QVERIFY(NotificationText::formatMessage(n1440).contains(u"in 24 hours"_s));

    // 90 minutes
    Notification n90;
    n90.kind = NotificationKind::Interview;
    n90.company = u"Acme"_s;
    n90.leadMinutes = 90;

    QCOMPARE(NotificationText::formatLeadTime(90), u"in 90 minutes"_s);
    QVERIFY(NotificationText::formatMessage(n90).contains(u"in 90 minutes"_s));

    // Singular / plural checks
    QCOMPARE(NotificationText::formatLeadTime(1), u"in 1 minute"_s);
    QCOMPARE(NotificationText::formatLeadTime(2), u"in 2 minutes"_s);
    QCOMPARE(NotificationText::formatLeadTime(120), u"in 2 hours"_s);
}

void tst_Notifiers::notificationTextFollowUp() {
    Notification nWithPos;
    nWithPos.kind = NotificationKind::FollowUp;
    nWithPos.company = u"Acme Corp"_s;
    nWithPos.position = u"Systems Dev"_s;

    QCOMPARE(NotificationText::formatTitle(nWithPos), u"Acme Corp"_s);
    QVERIFY(NotificationText::formatMessage(nWithPos).contains(u"Follow-up due today"_s));
    QVERIFY(NotificationText::formatMessage(nWithPos).contains(u"Systems Dev"_s));

    Notification nNoPos;
    nNoPos.kind = NotificationKind::FollowUp;
    nNoPos.company = u"Acme Corp"_s;

    QCOMPARE(NotificationText::formatMessage(nNoPos), u"Follow-up due today"_s);
}

void tst_Notifiers::notificationTextOverdueSingleAndSummarySingularPlural() {
    // Single overdue
    Notification single;
    single.kind = NotificationKind::OverdueFollowUps;
    single.refId = 42;
    single.count = 1;
    single.company = u"Wayne Enterprises"_s;
    single.position = u"Platform Engineer"_s;

    QCOMPARE(NotificationText::formatTitle(single), u"Overdue Follow-up"_s);
    QCOMPARE(NotificationText::formatMessage(single), u"Wayne Enterprises — Platform Engineer"_s);

    // Summary overdue (refId = 0)
    Notification sum1;
    sum1.kind = NotificationKind::OverdueFollowUps;
    sum1.refId = 0;
    sum1.count = 1;

    QCOMPARE(NotificationText::formatTitle(sum1), u"Overdue Follow-ups"_s);
    QCOMPARE(NotificationText::formatMessage(sum1), u"1 follow-up is overdue"_s);

    Notification sum4;
    sum4.kind = NotificationKind::OverdueFollowUps;
    sum4.refId = 0;
    sum4.count = 4;

    QCOMPARE(NotificationText::formatTitle(sum4), u"Overdue Follow-ups"_s);
    QCOMPARE(NotificationText::formatMessage(sum4), u"4 follow-ups are overdue"_s);
}

void tst_Notifiers::inAppNotifierHiddenOrMinimizedWindowReturnsFalse() {
    App::AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.hide();

    InAppNotifier notifier(win, win.themeManager());
    Notification n;
    n.company = u"Acme"_s;

    QVERIFY(!notifier.notify(n));
    QCOMPARE(notifier.totalToastCount(), 0);

    win.show();
    win.showMinimized();
    QVERIFY(!notifier.notify(n));
    QCOMPARE(notifier.totalToastCount(), 0);
}

void tst_Notifiers::inAppNotifierVisibleWindowShowsToastAndNoAutoDismiss() {
    App::AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.showNormal();
    win.show();
    QCoreApplication::processEvents();

    InAppNotifier notifier(win, win.themeManager());
    Notification n;
    n.kind = NotificationKind::Interview;
    n.company = u"Acme"_s;

    QVERIFY(notifier.notify(n));
    QCOMPARE(notifier.visibleToastCount(), 1);
    QCOMPARE(notifier.totalToastCount(), 1);

    // Sticky toast: simulate a wait, verify it is still visible (no auto-dismiss timer)
    QTest::qWait(200);
    QCOMPARE(notifier.visibleToastCount(), 1);
    QCOMPARE(notifier.totalToastCount(), 1);
}

void tst_Notifiers::inAppNotifierFourNotificationsStackThreePlusOverflow() {
    App::AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.showNormal();
    win.show();
    QCoreApplication::processEvents();

    InAppNotifier notifier(win, win.themeManager());
    for (int i = 0; i < 4; ++i) {
        Notification n;
        n.kind = NotificationKind::Interview;
        n.company = u"Company %1"_s.arg(i);
        QVERIFY(notifier.notify(n));
    }

    QCOMPARE(notifier.totalToastCount(), 4);
    QCOMPARE(notifier.visibleToastCount(), 3);
    QVERIFY(notifier.isOverflowVisible());
    QCOMPARE(notifier.overflowText(), u"+1 more"_s);

    // Dismiss one toast -> 3 remain, overflow label hides
    notifier.dismissNewest();
    QCOMPARE(notifier.totalToastCount(), 3);
    QCOMPARE(notifier.visibleToastCount(), 3);
    QVERIFY(!notifier.isOverflowVisible());
}

void tst_Notifiers::inAppNotifierEscDismissesToast() {
    App::AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.showNormal();
    win.show();
    QCoreApplication::processEvents();

    InAppNotifier notifier(win, win.themeManager());
    Notification n;
    n.company = u"Esc Co"_s;
    notifier.notify(n);
    QCOMPARE(notifier.totalToastCount(), 1);

    // Press Escape
    QKeyEvent escPress(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QCoreApplication::sendEvent(&win, &escPress);

    QCOMPARE(notifier.totalToastCount(), 0);
}

void tst_Notifiers::inAppNotifierClickOpensItem() {
    App::AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.showNormal();
    win.show();
    QCoreApplication::processEvents();

    Notification received;
    InAppNotifier notifier(win, win.themeManager(), [&](const Notification& n) {
        received = n;
    });

    Notification n;
    n.kind = NotificationKind::Interview;
    n.refId = 777;
    n.company = u"Clickable Corp"_s;
    notifier.notify(n);

    QCOMPARE(notifier.activeToasts().size(), 1);
    Toast* toast = notifier.activeToasts().first();

    QTest::mouseClick(toast, Qt::LeftButton);

    QCOMPARE(received.refId, 777);
    QCOMPARE(notifier.totalToastCount(), 0);
}

void tst_Notifiers::inAppNotifierFocusWidgetUnchangedOnToast() {
    App::AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.showNormal();
    win.show();
    win.activateWindow();
    QCoreApplication::processEvents();

    auto* searchEdit = win.findChild<QLineEdit*>(u"HeaderSearchEdit"_s);
    if (searchEdit) {
        searchEdit->setFocus();
        QCoreApplication::processEvents();
        QCOMPARE(QApplication::focusWidget(), searchEdit);
    }

    InAppNotifier notifier(win, win.themeManager());
    Notification n;
    n.company = u"Focus Co"_s;
    notifier.notify(n);

    if (searchEdit) {
        QCOMPARE(QApplication::focusWidget(), searchEdit);
    }
}

void tst_Notifiers::inAppNotifierRepositionFollowsWindowResize() {
    App::AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.resize(1100, 700);
    win.showNormal();
    win.show();
    QCoreApplication::processEvents();

    InAppNotifier notifier(win, win.themeManager());
    Notification n;
    n.company = u"Resize Co"_s;
    notifier.notify(n);

    auto* toast = notifier.activeToasts().first();
    const int initialX = toast->geometry().x();

    win.resize(1300, 800);
    QCoreApplication::processEvents();

    const int newX = toast->geometry().x();
    QVERIFY(newX > initialX);
}

void tst_Notifiers::toastAccessibleNameAndDescription() {
    App::AppContext ctx(u":memory:"_s);
    Notification n;
    n.kind = NotificationKind::FollowUp;
    n.company = u"Access Corp"_s;
    n.position = u"Engineer"_s;

    Toast toast(n, ctx.themeManager());
    QCOMPARE(toast.accessibleName(), u"Access Corp: Engineer — Follow-up due today"_s);
    QCOMPARE(toast.accessibleDescription(), u"Engineer — Follow-up due today"_s);
}

void tst_Notifiers::trayNotifierOffscreenBehavior() {
    App::AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);

    Notification received;
    TrayNotifier tray(win, [&](const Notification& n) {
        received = n;
    });

    Notification n;
    n.kind = NotificationKind::Interview;
    n.refId = 888;
    n.company = u"Tray Tech"_s;

    QVERIFY(tray.notify(n));
    QVERIFY(tray.lastNotification().has_value());
    QCOMPARE(tray.lastNotification()->refId, 888);

    tray.triggerMessageClick();
    QCOMPARE(received.refId, 888);
}

void tst_Notifiers::coordinatorEndToEndInterview59MinutesAheadOneToastAndOneDbPair() {
    App::AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.resize(1100, 700);
    win.showNormal();
    win.show();
    QCoreApplication::processEvents();

    JobApplication app;
    app.company = u"Coordinator Co"_s;
    app.position = u"Lead Architect"_s;
    app.status = ApplicationStatus::Applied;
    QVERIFY(ctx.applications().insert(app));

    ctx.settings().setInterviewLeadMinutes({1440, 60});

    const QDateTime interviewTime(QDate(2026, 10, 29), QTime(10, 59));
    Interview iv;
    iv.applicationId = app.id;
    iv.startAt = interviewTime;
    iv.type = InterviewType::Technical;
    QVERIFY(ctx.interviews().insert(iv));

    // Clock set to 10:00 (exactly 59 minutes before interview)
    QDateTime clockTime(QDate(2026, 10, 29), QTime(10, 0));
    ReminderCoordinator coord(
        ctx.interviews(),
        ctx.applications(),
        ctx.reminderLog(),
        ctx.settings(),
        win,
        false,  // trayAvailable = false (uses in-app toast)
        [&clockTime] { return clockTime; }
    );

    coord.checkNow(false);

    // Exactly one toast presented
    auto* inApp = dynamic_cast<InAppNotifier*>(&coord.notifier());
    QVERIFY(inApp != nullptr);
    QCOMPARE(inApp->totalToastCount(), 1);

    // Both keys marked sent in DB (smallest lead delivered, larger recorded silently)
    QVERIFY(ctx.reminderLog().wasSent(u"interview:%1:60"_s.arg(iv.id)));
    QVERIFY(ctx.reminderLog().wasSent(u"interview:%1:1440"_s.arg(iv.id)));
}

void tst_Notifiers::coordinatorEndToEndClickOpensInterviewDialog() {
    App::AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.resize(1100, 700);
    win.showNormal();
    win.show();
    QCoreApplication::processEvents();

    JobApplication app;
    app.company = u"Dialog Click Corp"_s;
    app.position = u"Backend Dev"_s;
    app.status = ApplicationStatus::Applied;
    QVERIFY(ctx.applications().insert(app));

    const QDateTime interviewTime(QDate(2026, 10, 29), QTime(10, 59));
    Interview iv;
    iv.applicationId = app.id;
    iv.startAt = interviewTime;
    iv.type = InterviewType::Technical;
    QVERIFY(ctx.interviews().insert(iv));

    QDateTime clockTime(QDate(2026, 10, 29), QTime(10, 0));
    ReminderCoordinator coord(
        ctx.interviews(),
        ctx.applications(),
        ctx.reminderLog(),
        ctx.settings(),
        win,
        false,
        [&clockTime] { return clockTime; }
    );

    coord.checkNow(false);

    auto* inApp = dynamic_cast<InAppNotifier*>(&coord.notifier());
    QVERIFY(inApp != nullptr);
    QCOMPARE(inApp->activeToasts().size(), 1);
    Toast* toast = inApp->activeToasts().first();

    bool dialogHandled = false;
    QTimer::singleShot(0, [&dialogHandled]() {
        if (auto* modal = QApplication::activeModalWidget()) {
            if (auto* dlg = qobject_cast<InterviewDialog*>(modal)) {
                dialogHandled = true;
                dlg->reject();
            }
        }
    });

    QTest::mouseClick(toast, Qt::LeftButton);
    QCoreApplication::processEvents();

    QVERIFY(dialogHandled);
}

void tst_Notifiers::coordinatorEndToEndOverdueSummaryClickSwitchesToJobs() {
    App::AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.resize(1100, 700);
    win.showNormal();
    win.show();
    QCoreApplication::processEvents();

    // Start on home page (index 0)
    QCOMPARE(win.pageStack()->currentIndex(), 0);

    Notification overdueSummary;
    overdueSummary.kind = NotificationKind::OverdueFollowUps;
    overdueSummary.refId = 0;
    overdueSummary.count = 3;

    // Routing summary with refId = 0 switches directly to Jobs page
    win.routeNotification(overdueSummary);
    QCoreApplication::processEvents();

    // Jobs page is index 2
    QCOMPARE(win.pageStack()->currentIndex(), 2);
}

void tst_Notifiers::coordinatorEndToEndMinimizedWindowDelaysUntilShowNormal() {
    App::AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.resize(1100, 700);
    win.showNormal();
    win.show();
    QCoreApplication::processEvents();

    JobApplication app;
    app.company = u"Delayed Co"_s;
    app.position = u"Dev"_s;
    app.status = ApplicationStatus::Applied;
    QVERIFY(ctx.applications().insert(app));

    const QDateTime interviewTime(QDate(2026, 10, 29), QTime(10, 50));
    Interview iv;
    iv.applicationId = app.id;
    iv.startAt = interviewTime;
    QVERIFY(ctx.interviews().insert(iv));

    QDateTime clockTime(QDate(2026, 10, 29), QTime(10, 0));
    ReminderCoordinator coord(
        ctx.interviews(),
        ctx.applications(),
        ctx.reminderLog(),
        ctx.settings(),
        win,
        false,
        [&clockTime] { return clockTime; }
    );

    // Minimize window -> notification cannot be presented
    win.showMinimized();

    coord.checkNow(false);

    // Nothing recorded in reminders_sent
    QVERIFY(!ctx.reminderLog().wasSent(u"interview:%1:60"_s.arg(iv.id)));

    // Restore window
    win.showNormal();
    win.show();
    QCoreApplication::processEvents();

    // Next checkNow delivers
    coord.checkNow(false);

    QVERIFY(ctx.reminderLog().wasSent(u"interview:%1:60"_s.arg(iv.id)));
    auto* inApp = dynamic_cast<InAppNotifier*>(&coord.notifier());
    QVERIFY(inApp != nullptr);
    QCOMPARE(inApp->totalToastCount(), 1);
}

void tst_Notifiers::coordinatorEndToEndNoTrayFallbackToToast() {
    App::AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);

    ReminderCoordinator coord(
        ctx.interviews(),
        ctx.applications(),
        ctx.reminderLog(),
        ctx.settings(),
        win,
        false  // no tray
    );

    QVERIFY(dynamic_cast<InAppNotifier*>(&coord.notifier()) != nullptr);
}

void tst_Notifiers::coordinatorEndToEndSettingsLeadMinutesChangeTakesEffect() {
    App::AppContext ctx(u":memory:"_s);
    MainWindow win(ctx);
    win.resize(1100, 700);
    win.showNormal();
    win.show();
    QCoreApplication::processEvents();

    JobApplication app;
    app.company = u"Lead Change Co"_s;
    app.position = u"Dev"_s;
    app.status = ApplicationStatus::Applied;
    QVERIFY(ctx.applications().insert(app));

    // Interview at 12:00
    const QDateTime interviewTime(QDate(2026, 10, 29), QTime(12, 0));
    Interview iv;
    iv.applicationId = app.id;
    iv.startAt = interviewTime;
    QVERIFY(ctx.interviews().insert(iv));

    // Set initial lead minutes to {60} only
    ctx.settings().setInterviewLeadMinutes({60});

    // Clock at 10:10 (110 minutes before interview)
    QDateTime clockTime(QDate(2026, 10, 29), QTime(10, 10));
    ReminderCoordinator coord(
        ctx.interviews(),
        ctx.applications(),
        ctx.reminderLog(),
        ctx.settings(),
        win,
        false,
        [&clockTime] { return clockTime; }
    );

    // With lead {60}, 60m is due at 11:00, so at 10:10 it is not due.
    coord.checkNow(false);
    auto* inApp = dynamic_cast<InAppNotifier*>(&coord.notifier());
    QVERIFY(inApp != nullptr);
    QCOMPARE(inApp->totalToastCount(), 0);

    // Change setting to include 120 minutes lead
    ctx.settings().setInterviewLeadMinutes({120, 60});

    // Next check fires 120m lead!
    coord.checkNow(false);
    QCOMPARE(inApp->totalToastCount(), 1);
    QVERIFY(ctx.reminderLog().wasSent(u"interview:%1:120"_s.arg(iv.id)));

    // Reset back to default
    ctx.settings().setInterviewLeadMinutes({1440, 60});
}

QTEST_MAIN(tst_Notifiers)
#include "tst_Notifiers.moc"
