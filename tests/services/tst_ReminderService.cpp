#include <memory>
#include <QDateTime>
#include <QTest>
#include "TestDatabase.h"
#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"
#include "data/ReminderLogRepository.h"
#include "domain/Enums.h"
#include "domain/Structs.h"
#include "services/INotifier.h"
#include "services/ReminderService.h"

using namespace Qt::StringLiterals;
using JobPrep::Data::ApplicationRepository;
using JobPrep::Data::InterviewRepository;
using JobPrep::Data::ReminderLogRepository;
using JobPrep::Domain::ApplicationStatus;
using JobPrep::Domain::Interview;
using JobPrep::Domain::InterviewOutcome;
using JobPrep::Domain::InterviewType;
using JobPrep::Domain::JobApplication;
using JobPrep::Services::INotifier;
using JobPrep::Services::Notification;
using JobPrep::Services::NotificationKind;
using JobPrep::Services::ReminderService;
using JobPrep::Tests::TestDatabase;

namespace {

class FakeNotifier : public INotifier {
public:
    bool shouldSucceed{true};
    QList<Notification> received;

    bool notify(const Notification& notification) override {
        if (!shouldSucceed) return false;
        received.append(notification);
        return true;
    }
};

}  // namespace

class tst_ReminderService : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void interview59MinutesAheadFiresOnceAndRecordsBothKeysNoRepeatOnRestart();
    void leads24hAnd1hFireSeparatelyWhenRunning();
    void smallerLeadAlreadyDeliveredLargerAddedLaterMarkedNotNotified();
    void cancelledStartedPastInterviewsIgnoredRejectedApplicationReminded();
    void rescheduleMovedLaterFiresAgainMovedEarlierFiresDeletedNothingNotRescheduledNoRepeat();
    void notifierReturnsFalseNothingRecordedNextTickDeliversOnce();
    void followUpBefore0900NothingAt0900OnceClosedApplicationNothing();
    void overdueStartupPassOnlyOneAppSingleMultipleSummaryRestartSameDayNothingNextDayAgain();
    void leadMinutesChangeTakesEffectOnNextCheck();
    void dstDayBorderInterviewAt0030With1440LeadDuePreviousDay();

private:
    std::unique_ptr<TestDatabase> m_db;
    std::unique_ptr<ApplicationRepository> m_apps;
    std::unique_ptr<InterviewRepository> m_interviews;
    std::unique_ptr<ReminderLogRepository> m_reminderLog;
    std::unique_ptr<FakeNotifier> m_notifier;
};

void tst_ReminderService::init() {
    m_db = std::make_unique<TestDatabase>(u"tst_reminder_service"_s);
    QVERIFY(m_db->open(u":memory:"_s));
    m_apps = std::make_unique<ApplicationRepository>(m_db->db());
    m_interviews = std::make_unique<InterviewRepository>(m_db->db());
    m_reminderLog = std::make_unique<ReminderLogRepository>(m_db->db());
    m_notifier = std::make_unique<FakeNotifier>();
}

void tst_ReminderService::cleanup() {
    m_notifier.reset();
    m_reminderLog.reset();
    m_interviews.reset();
    m_apps.reset();
    m_db.reset();
}

void tst_ReminderService::interview59MinutesAheadFiresOnceAndRecordsBothKeysNoRepeatOnRestart() {
    JobApplication app;
    app.company = u"Acme Corp"_s;
    app.position = u"C++ Engineer"_s;
    app.status = ApplicationStatus::Applied;
    QVERIFY(m_apps->insert(app));

    const QDateTime interviewTime(QDate(2026, 10, 29), QTime(14, 0));
    Interview iv;
    iv.applicationId = app.id;
    iv.startAt = interviewTime;
    iv.type = InterviewType::Technical;
    QVERIFY(m_interviews->insert(iv));

    QDateTime clockTime(QDate(2026, 10, 29), QTime(13, 1));  // 59 min ahead
    ReminderService service(*m_interviews, *m_apps, *m_reminderLog, *m_notifier);
    service.setClock([&clockTime] { return clockTime; });

    // Both 1440 and 60 leads are due; smallest lead (60) should notify
    service.checkNow(false);

    QCOMPARE(m_notifier->received.size(), 1);
    const auto notif = m_notifier->received.first();
    QCOMPARE(notif.kind, NotificationKind::Interview);
    QCOMPARE(notif.refId, iv.id);
    QCOMPARE(notif.company, u"Acme Corp"_s);
    QCOMPARE(notif.position, u"C++ Engineer"_s);
    QCOMPARE(notif.leadMinutes, 60);

    // Both keys recorded in reminders_sent
    QVERIFY(m_reminderLog->wasSent(u"interview:%1:60"_s.arg(iv.id)));
    QVERIFY(m_reminderLog->wasSent(u"interview:%1:1440"_s.arg(iv.id)));

    // Second checkNow() must not repeat
    m_notifier->received.clear();
    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 0);

    // Reconstruct service on same DB (simulating restart) -> must not repeat
    ReminderService restarted(*m_interviews, *m_apps, *m_reminderLog, *m_notifier);
    restarted.setClock([&clockTime] { return clockTime; });
    restarted.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 0);
}

void tst_ReminderService::leads24hAnd1hFireSeparatelyWhenRunning() {
    JobApplication app;
    app.company = u"Beta Ltd"_s;
    app.position = u"Lead Dev"_s;
    app.status = ApplicationStatus::Applied;
    QVERIFY(m_apps->insert(app));

    const QDateTime interviewTime(QDate(2026, 10, 30), QTime(14, 0));
    Interview iv;
    iv.applicationId = app.id;
    iv.startAt = interviewTime;
    QVERIFY(m_interviews->insert(iv));

    QDateTime clockTime(QDate(2026, 10, 29), QTime(13, 0));  // 25 hours ahead -> nothing
    ReminderService service(*m_interviews, *m_apps, *m_reminderLog, *m_notifier);
    service.setClock([&clockTime] { return clockTime; });

    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 0);

    // Advance to 24h ahead
    clockTime = QDateTime(QDate(2026, 10, 29), QTime(14, 0));
    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 1);
    QCOMPARE(m_notifier->received.first().leadMinutes, 1440);
    QVERIFY(m_reminderLog->wasSent(u"interview:%1:1440"_s.arg(iv.id)));
    QVERIFY(!m_reminderLog->wasSent(u"interview:%1:60"_s.arg(iv.id)));

    // Advance to 1h ahead
    m_notifier->received.clear();
    clockTime = QDateTime(QDate(2026, 10, 30), QTime(13, 0));
    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 1);
    QCOMPARE(m_notifier->received.first().leadMinutes, 60);
    QVERIFY(m_reminderLog->wasSent(u"interview:%1:60"_s.arg(iv.id)));
}

void tst_ReminderService::smallerLeadAlreadyDeliveredLargerAddedLaterMarkedNotNotified() {
    JobApplication app;
    app.company = u"Gamma Inc"_s;
    app.status = ApplicationStatus::Applied;
    QVERIFY(m_apps->insert(app));

    const QDateTime interviewTime(QDate(2026, 10, 29), QTime(14, 0));
    Interview iv;
    iv.applicationId = app.id;
    iv.startAt = interviewTime;
    QVERIFY(m_interviews->insert(iv));

    // Deliver 60m lead first
    QDateTime clockTime(QDate(2026, 10, 29), QTime(13, 10));
    ReminderService service(*m_interviews, *m_apps, *m_reminderLog, *m_notifier);
    service.setClock([&clockTime] { return clockTime; });
    service.setLeadMinutes({60});
    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 1);
    QCOMPARE(m_notifier->received.first().leadMinutes, 60);

    // Now user adds 120m lead in settings
    m_notifier->received.clear();
    service.setLeadMinutes({120, 60});
    service.checkNow(false);

    // Must NOT notify for 120 because smaller lead 60 was already delivered
    QCOMPARE(m_notifier->received.size(), 0);
    // But larger lead key must be recorded silently so it never fires
    QVERIFY(m_reminderLog->wasSent(u"interview:%1:120"_s.arg(iv.id)));
}

void tst_ReminderService::cancelledStartedPastInterviewsIgnoredRejectedApplicationReminded() {
    JobApplication rejectedApp;
    rejectedApp.company = u"Rejected Co"_s;
    rejectedApp.status = ApplicationStatus::Rejected;  // Rejected application
    QVERIFY(m_apps->insert(rejectedApp));

    const QDateTime interviewTime(QDate(2026, 10, 29), QTime(14, 0));

    // 1. Interview for rejected app (not cancelled) -> MUST BE REMINDED (rule A5)
    Interview ivRejected;
    ivRejected.applicationId = rejectedApp.id;
    ivRejected.startAt = interviewTime;
    ivRejected.outcome = InterviewOutcome::Pending;
    QVERIFY(m_interviews->insert(ivRejected));

    // 2. Cancelled interview -> IGNORED
    Interview ivCancelled;
    ivCancelled.applicationId = rejectedApp.id;
    ivCancelled.startAt = interviewTime;
    ivCancelled.outcome = InterviewOutcome::Cancelled;
    QVERIFY(m_interviews->insert(ivCancelled));

    // 3. Past interview -> IGNORED
    Interview ivPast;
    ivPast.applicationId = rejectedApp.id;
    ivPast.startAt = QDateTime(QDate(2026, 10, 29), QTime(10, 0));
    QVERIFY(m_interviews->insert(ivPast));

    QDateTime clockTime(QDate(2026, 10, 29), QTime(13, 30));
    ReminderService service(*m_interviews, *m_apps, *m_reminderLog, *m_notifier);
    service.setClock([&clockTime] { return clockTime; });

    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 1);
    QCOMPARE(m_notifier->received.first().refId, ivRejected.id);
}

void tst_ReminderService::rescheduleMovedLaterFiresAgainMovedEarlierFiresDeletedNothingNotRescheduledNoRepeat() {
    JobApplication app;
    app.company = u"Delta Systems"_s;
    app.status = ApplicationStatus::Applied;
    QVERIFY(m_apps->insert(app));

    QDateTime interviewTime(QDate(2026, 10, 29), QTime(14, 0));
    Interview iv;
    iv.applicationId = app.id;
    iv.startAt = interviewTime;
    QVERIFY(m_interviews->insert(iv));

    // Initial check at 13:00 -> fires 60m lead
    QDateTime clockTime(QDate(2026, 10, 29), QTime(13, 0));
    ReminderService service(*m_interviews, *m_apps, *m_reminderLog, *m_notifier);
    service.setClock([&clockTime] { return clockTime; });
    service.setLeadMinutes({60});
    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 1);

    // Not rescheduled: check at 13:05 -> no repeat
    m_notifier->received.clear();
    clockTime = QDateTime(QDate(2026, 10, 29), QTime(13, 5));
    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 0);

    // Move later to 16:00
    iv.startAt = QDateTime(QDate(2026, 10, 29), QTime(16, 0));
    QVERIFY(m_interviews->update(iv));

    // At 15:00: sent_at (13:00) is < (16:00 - 60m), so it is stale and FIRES AGAIN!
    clockTime = QDateTime(QDate(2026, 10, 29), QTime(15, 0));
    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 1);
    QCOMPARE(m_notifier->received.first().refId, iv.id);

    // Move earlier into window (new interview created at 18:00, moved earlier to 15:30)
    Interview iv2;
    iv2.applicationId = app.id;
    iv2.startAt = QDateTime(QDate(2026, 10, 29), QTime(18, 0));
    QVERIFY(m_interviews->insert(iv2));

    iv2.startAt = QDateTime(QDate(2026, 10, 29), QTime(15, 30));
    QVERIFY(m_interviews->update(iv2));

    m_notifier->received.clear();
    clockTime = QDateTime(QDate(2026, 10, 29), QTime(15, 0));
    service.checkNow(false);
    // iv2 fires because now (15:00) is >= (15:30 - 60m = 14:30)
    QCOMPARE(m_notifier->received.size(), 1);
    QCOMPARE(m_notifier->received.first().refId, iv2.id);

    // Delete interview -> nothing fires
    QVERIFY(m_interviews->remove(iv2.id));
    m_notifier->received.clear();
    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 0);
}

void tst_ReminderService::notifierReturnsFalseNothingRecordedNextTickDeliversOnce() {
    JobApplication app;
    app.company = u"Epsilon Inc"_s;
    app.status = ApplicationStatus::Applied;
    QVERIFY(m_apps->insert(app));

    const QDateTime interviewTime(QDate(2026, 10, 29), QTime(14, 0));
    Interview iv;
    iv.applicationId = app.id;
    iv.startAt = interviewTime;
    QVERIFY(m_interviews->insert(iv));

    QDateTime clockTime(QDate(2026, 10, 29), QTime(13, 0));
    ReminderService service(*m_interviews, *m_apps, *m_reminderLog, *m_notifier);
    service.setClock([&clockTime] { return clockTime; });
    service.setLeadMinutes({60});

    // Notifier fails (window hidden)
    m_notifier->shouldSucceed = false;
    service.checkNow(false);

    // Nothing recorded in reminders_sent
    QVERIFY(!m_reminderLog->wasSent(u"interview:%1:60"_s.arg(iv.id)));

    // Next tick: notifier succeeds
    m_notifier->shouldSucceed = true;
    clockTime = QDateTime(QDate(2026, 10, 29), QTime(13, 1));
    service.checkNow(false);

    QCOMPARE(m_notifier->received.size(), 1);
    QVERIFY(m_reminderLog->wasSent(u"interview:%1:60"_s.arg(iv.id)));
}

void tst_ReminderService::followUpBefore0900NothingAt0900OnceClosedApplicationNothing() {
    JobApplication appDue;
    appDue.company = u"Zeta Tech"_s;
    appDue.status = ApplicationStatus::Applied;
    appDue.nextActionDate = QDate(2026, 10, 29);
    QVERIFY(m_apps->insert(appDue));

    JobApplication appClosed;
    appClosed.company = u"Closed Co"_s;
    appClosed.status = ApplicationStatus::Accepted;  // closed application
    appClosed.nextActionDate = QDate(2026, 10, 29);
    QVERIFY(m_apps->insert(appClosed));

    // 1. Before 09:00 -> nothing
    QDateTime clockTime(QDate(2026, 10, 29), QTime(8, 59));
    ReminderService service(*m_interviews, *m_apps, *m_reminderLog, *m_notifier);
    service.setClock([&clockTime] { return clockTime; });

    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 0);

    // 2. At 09:00 -> appDue notifies, appClosed is ignored
    clockTime = QDateTime(QDate(2026, 10, 29), QTime(9, 0));
    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 1);
    QCOMPARE(m_notifier->received.first().kind, NotificationKind::FollowUp);
    QCOMPARE(m_notifier->received.first().refId, appDue.id);
    QVERIFY(m_reminderLog->wasSent(u"followup:%1:2026-10-29"_s.arg(appDue.id)));

    // 3. Subsequent check -> does not repeat
    m_notifier->received.clear();
    clockTime = QDateTime(QDate(2026, 10, 29), QTime(9, 5));
    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 0);
}

void tst_ReminderService::overdueStartupPassOnlyOneAppSingleMultipleSummaryRestartSameDayNothingNextDayAgain() {
    JobApplication app1;
    app1.company = u"App One"_s;
    app1.status = ApplicationStatus::Applied;
    app1.nextActionDate = QDate(2026, 10, 20);  // overdue
    QVERIFY(m_apps->insert(app1));

    QDateTime clockTime(QDate(2026, 10, 29), QTime(10, 0));
    ReminderService service(*m_interviews, *m_apps, *m_reminderLog, *m_notifier);
    service.setClock([&clockTime] { return clockTime; });

    // Normal check (startupPass = false) -> does NOT evaluate overdue
    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 0);

    // Startup pass with 1 overdue application -> single notification with app1.id
    service.checkNow(true);
    QCOMPARE(m_notifier->received.size(), 1);
    QCOMPARE(m_notifier->received.first().kind, NotificationKind::OverdueFollowUps);
    QCOMPARE(m_notifier->received.first().refId, app1.id);
    QCOMPARE(m_notifier->received.first().count, 1);
    QVERIFY(m_reminderLog->wasSent(u"overdue:%1:2026-10-29"_s.arg(app1.id)));

    // Add 2 more overdue applications
    JobApplication app2;
    app2.company = u"App Two"_s;
    app2.status = ApplicationStatus::Applied;
    app2.nextActionDate = QDate(2026, 10, 21);
    QVERIFY(m_apps->insert(app2));

    JobApplication app3;
    app3.company = u"App Three"_s;
    app3.status = ApplicationStatus::Applied;
    app3.nextActionDate = QDate(2026, 10, 22);
    QVERIFY(m_apps->insert(app3));

    // Restart on same day: app1 was sent today; remaining unsent overdue are app2 and app3 (count 2)
    m_notifier->received.clear();
    service.checkNow(true);
    QCOMPARE(m_notifier->received.size(), 1);
    QCOMPARE(m_notifier->received.first().kind, NotificationKind::OverdueFollowUps);
    QCOMPARE(m_notifier->received.first().refId, 0);  // summary
    QCOMPARE(m_notifier->received.first().count, 2);

    // Another check on same day -> all 3 are recorded for today, so nothing
    m_notifier->received.clear();
    service.checkNow(true);
    QCOMPARE(m_notifier->received.size(), 0);

    // Next day -> date is 2026-10-30; all 3 overdue applications are reminded again as summary of 3
    clockTime = QDateTime(QDate(2026, 10, 30), QTime(9, 0));
    service.checkNow(true);
    QCOMPARE(m_notifier->received.size(), 1);
    QCOMPARE(m_notifier->received.first().refId, 0);
    QCOMPARE(m_notifier->received.first().count, 3);
}

void tst_ReminderService::leadMinutesChangeTakesEffectOnNextCheck() {
    JobApplication app;
    app.company = u"Theta Software"_s;
    app.status = ApplicationStatus::Applied;
    QVERIFY(m_apps->insert(app));

    const QDateTime interviewTime(QDate(2026, 10, 29), QTime(14, 0));
    Interview iv;
    iv.applicationId = app.id;
    iv.startAt = interviewTime;
    QVERIFY(m_interviews->insert(iv));

    // At 12:10 (110 minutes ahead): with lead {60}, 60m is not due until 13:00
    QDateTime clockTime(QDate(2026, 10, 29), QTime(12, 10));
    ReminderService service(*m_interviews, *m_apps, *m_reminderLog, *m_notifier);
    service.setClock([&clockTime] { return clockTime; });
    service.setLeadMinutes({60});

    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 0);

    // Change lead minutes to include 120m
    service.setLeadMinutes({120, 60});
    service.checkNow(false);

    // 120m lead is now due and fires
    QCOMPARE(m_notifier->received.size(), 1);
    QCOMPARE(m_notifier->received.first().leadMinutes, 120);
}

void tst_ReminderService::dstDayBorderInterviewAt0030With1440LeadDuePreviousDay() {
    JobApplication app;
    app.company = u"Midnight Corp"_s;
    app.status = ApplicationStatus::Applied;
    QVERIFY(m_apps->insert(app));

    // Interview at 00:30 on Oct 30
    const QDateTime interviewTime(QDate(2026, 10, 30), QTime(0, 30));
    Interview iv;
    iv.applicationId = app.id;
    iv.startAt = interviewTime;
    QVERIFY(m_interviews->insert(iv));

    // 1440 lead is 24 hours -> due at 00:30 on Oct 29 (previous calendar day!)
    QDateTime clockTime(QDate(2026, 10, 29), QTime(0, 35));
    ReminderService service(*m_interviews, *m_apps, *m_reminderLog, *m_notifier);
    service.setClock([&clockTime] { return clockTime; });

    service.checkNow(false);
    QCOMPARE(m_notifier->received.size(), 1);
    QCOMPARE(m_notifier->received.first().refId, iv.id);
    QCOMPARE(m_notifier->received.first().leadMinutes, 1440);
}

QTEST_GUILESS_MAIN(tst_ReminderService)
#include "tst_ReminderService.moc"
