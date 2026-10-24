#include <QDate>
#include <QDateTime>
#include <QElapsedTimer>
#include <QTest>
#include <QTime>
#include "TestDatabase.h"
#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"
#include "data/SessionRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/Enums.h"
#include "domain/StageRank.h"
#include "domain/Structs.h"
#include "services/StatsService.h"

using namespace Qt::StringLiterals;
using JobPrep::Data::ApplicationRepository;
using JobPrep::Data::InterviewRepository;
using JobPrep::Data::SessionRepository;
using JobPrep::Data::TopicRepository;
using JobPrep::Data::TrackRepository;
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
using JobPrep::Services::StatsService;
using JobPrep::Tests::TestDatabase;

class tst_StatsService : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testStageRankEveryEnum();
    void testWeekBounds();
    void testHandCalculatedDataset();
    void testSundayFirst();
    void testFutureDatedSessionInWeek();
    void testZeroOrNegativeGoal();
    void testGoalCapping();
    void testTrackProgress();
    void testInterviewRateZeroDenominator();
    void testPerformanceSanity1000Apps5000Sessions();

private:
    std::unique_ptr<TestDatabase> m_db;
    std::unique_ptr<TrackRepository> m_tracks;
    std::unique_ptr<TopicRepository> m_topics;
    std::unique_ptr<ApplicationRepository> m_apps;
    std::unique_ptr<InterviewRepository> m_interviews;
    std::unique_ptr<SessionRepository> m_sessions;
    std::unique_ptr<StatsService> m_service;
};

void tst_StatsService::init() {
    m_db = std::make_unique<TestDatabase>(u"tst_stats"_s);
    QVERIFY(m_db->open(u":memory:"_s));
    m_tracks = std::make_unique<TrackRepository>(m_db->db());
    m_topics = std::make_unique<TopicRepository>(m_db->db());
    m_apps = std::make_unique<ApplicationRepository>(m_db->db());
    m_interviews = std::make_unique<InterviewRepository>(m_db->db());
    m_sessions = std::make_unique<SessionRepository>(m_db->db());
    m_service = std::make_unique<StatsService>(*m_apps, *m_interviews, *m_sessions, *m_topics, *m_tracks);
}

void tst_StatsService::cleanup() {
    m_service.reset();
    m_sessions.reset();
    m_interviews.reset();
    m_apps.reset();
    m_topics.reset();
    m_tracks.reset();
    m_db.reset();
}

void tst_StatsService::testStageRankEveryEnum() {
    // SPEC §6: wishlist 0, applied 1, hr_screen 2, technical 3, final 4, offer 5, accepted 5.
    // rejected, withdrawn, ghosted: std::nullopt.
    QCOMPARE(JobPrep::Domain::stageRank(ApplicationStatus::Wishlist), std::optional<int>(0));
    QCOMPARE(JobPrep::Domain::stageRank(ApplicationStatus::Applied), std::optional<int>(1));
    QCOMPARE(JobPrep::Domain::stageRank(ApplicationStatus::HrScreen), std::optional<int>(2));
    QCOMPARE(JobPrep::Domain::stageRank(ApplicationStatus::Technical), std::optional<int>(3));
    QCOMPARE(JobPrep::Domain::stageRank(ApplicationStatus::Final), std::optional<int>(4));
    QCOMPARE(JobPrep::Domain::stageRank(ApplicationStatus::Offer), std::optional<int>(5));
    QCOMPARE(JobPrep::Domain::stageRank(ApplicationStatus::Accepted), std::optional<int>(5));
    QCOMPARE(JobPrep::Domain::stageRank(ApplicationStatus::Rejected), std::nullopt);
    QCOMPARE(JobPrep::Domain::stageRank(ApplicationStatus::Withdrawn), std::nullopt);
    QCOMPARE(JobPrep::Domain::stageRank(ApplicationStatus::Ghosted), std::nullopt);

    // statusesWithRankAtLeast checks
    QCOMPARE(JobPrep::Domain::statusesWithRankAtLeast(1).size(), 6);
    QCOMPARE(JobPrep::Domain::statusesWithRankAtLeast(2).size(), 5);
    QCOMPARE(JobPrep::Domain::statusesWithRankAtLeast(3).size(), 4);
    QCOMPARE(JobPrep::Domain::statusesWithRankAtLeast(4).size(), 3);
    QCOMPARE(JobPrep::Domain::statusesWithRankAtLeast(5).size(), 2);
}

void tst_StatsService::testWeekBounds() {
    // Monday-first checks
    // 2026-10-29 is a Thursday -> week is 2026-10-26 (Mon) to 2026-11-01 (Sun)
    auto [mStart, mEnd] = StatsService::weekBounds(QDate(2026, 10, 29), Qt::Monday);
    QCOMPARE(mStart, QDate(2026, 10, 26));
    QCOMPARE(mEnd, QDate(2026, 11, 1));

    // Monday itself
    auto [monStart, monEnd] = StatsService::weekBounds(QDate(2026, 10, 26), Qt::Monday);
    QCOMPARE(monStart, QDate(2026, 10, 26));
    QCOMPARE(monEnd, QDate(2026, 11, 1));

    // Sunday itself
    auto [sunStart, sunEnd] = StatsService::weekBounds(QDate(2026, 11, 1), Qt::Monday);
    QCOMPARE(sunStart, QDate(2026, 10, 26));
    QCOMPARE(sunEnd, QDate(2026, 11, 1));

    // Sunday-first checks
    // 2026-10-29 is a Thursday -> week is 2026-10-25 (Sun) to 2026-10-31 (Sat)
    auto [sStart, sEnd] = StatsService::weekBounds(QDate(2026, 10, 29), Qt::Sunday);
    QCOMPARE(sStart, QDate(2026, 10, 25));
    QCOMPARE(sEnd, QDate(2026, 10, 31));

    // Sunday itself
    auto [sunFirstStart, sunFirstEnd] = StatsService::weekBounds(QDate(2026, 10, 25), Qt::Sunday);
    QCOMPARE(sunFirstStart, QDate(2026, 10, 25));
    QCOMPARE(sunFirstEnd, QDate(2026, 10, 31));

    // Saturday itself
    auto [satStart, satEnd] = StatsService::weekBounds(QDate(2026, 10, 31), Qt::Sunday);
    QCOMPARE(satStart, QDate(2026, 10, 25));
    QCOMPARE(satEnd, QDate(2026, 10, 31));

    // Year-end transition: 2026-12-31 is Thursday -> Mon-first week 2026-12-28 to 2027-01-03
    auto [yearStart, yearEnd] = StatsService::weekBounds(QDate(2026, 12, 31), Qt::Monday);
    QCOMPARE(yearStart, QDate(2026, 12, 28));
    QCOMPARE(yearEnd, QDate(2027, 1, 3));
}

void tst_StatsService::testHandCalculatedDataset() {
    /*
     Hand-calculated dataset per Amendment A8:
     Clock: Thursday 2026-10-29 10:00:00.
     Week (Monday-first): 2026-10-26 (Mon) - 2026-11-01 (Sun).

     Applications (11):
     - 2 wishlist (no applied date)
     - 3 applied: applied dates Oct 27, Oct 29, Oct 20
     - 2 hr_screen: applied dates before Oct 26 (e.g. Oct 15)
     - 1 technical: applied date before Oct 26
     - 1 final: applied date before Oct 26
     - 1 offer: applied date before Oct 26
     - 1 rejected with history: applied -> hr_screen -> technical -> rejected (applied date Oct 10)

     Expected application metrics:
     - appliedThisWeek: 2 (Oct 27, Oct 29; Oct 20 is outside the week)
     - activeProcesses: 3 + 2 + 1 + 1 + 1 = 8 (wishlist and rejected are excluded)
     - Funnel reached counts:
       - Rank 1 (Applied): 3 applied + 2 hr_screen + 1 technical + 1 final + 1 offer + 1 rejected = 9
       - Rank 2 (HrScreen): 2 hr_screen + 1 technical + 1 final + 1 offer + 1 rejected = 6
       - Rank 3 (Technical): 1 technical + 1 final + 1 offer + 1 rejected = 4
       - Rank 4 (Final): 1 final + 1 offer = 2
       - Rank 5 (Offer): 1 offer = 1
     - Interview rate: reached(rank >= 2) / reached(rank >= 1) = 6 / 9 = 66.7% (2/3)

     Sessions:
     - Sun Oct 25 = 30
     - Mon Oct 26 = 60
     - Tue Oct 27 = 120
     - Wed Oct 28 = 80
     - Thu Oct 29 = nothing (today)
     Expected Monday-first:
     - weekMinutes: 60 + 120 + 80 = 260
     - streak: 4 (today is empty, so count Wed 28, Tue 27, Mon 26, Sun 25 = 4 consecutive days)
     - goalMinutes: 360 -> goalPercentReal = (260 * 100) / 360 = 72, goalPercentCapped = 72

     Interviews:
     Window [2026-10-29 10:00:00, 2026-11-04 23:59:59] (today + 6 calendar days)
     - Oct 27 past (no)
     - Oct 29 08:00 earlier today (no, before 10:00)
     - Oct 30 14:00 (counts)
     - Oct 31 cancelled (no)
     - Nov 4 11:00 (counts)
     - Nov 4 23:00 (counts)
     - Nov 5 09:00 (no, after window end)
     Count = 3.
    */

    const QDateTime now(QDate(2026, 10, 29), QTime(10, 0));
    m_service->setClock([now] { return now; });

    // 1. Applications
    // 2 wishlist
    for (int i = 0; i < 2; ++i) {
        JobApplication w;
        w.company = u"Wishlist_%1"_s.arg(i);
        w.position = u"Dev"_s;
        w.status = ApplicationStatus::Wishlist;
        QVERIFY(m_apps->insert(w));
    }

    // 3 applied: Oct 27, Oct 29, Oct 20
    JobApplication a1; a1.company = u"App_27"_s; a1.position = u"Dev"_s; a1.status = ApplicationStatus::Applied; a1.appliedDate = QDate(2026, 10, 27); QVERIFY(m_apps->insert(a1));
    JobApplication a2; a2.company = u"App_29"_s; a2.position = u"Dev"_s; a2.status = ApplicationStatus::Applied; a2.appliedDate = QDate(2026, 10, 29); QVERIFY(m_apps->insert(a2));
    JobApplication a3; a3.company = u"App_20"_s; a3.position = u"Dev"_s; a3.status = ApplicationStatus::Applied; a3.appliedDate = QDate(2026, 10, 20); QVERIFY(m_apps->insert(a3));

    // 2 hr_screen
    for (int i = 0; i < 2; ++i) {
        JobApplication hr;
        hr.company = u"HrScreen_%1"_s.arg(i);
        hr.position = u"Dev"_s;
        hr.status = ApplicationStatus::HrScreen;
        hr.appliedDate = QDate(2026, 10, 15);
        QVERIFY(m_apps->insert(hr));
    }

    // 1 technical, 1 final, 1 offer
    JobApplication tech; tech.company = u"TechCo"_s; tech.position = u"Dev"_s; tech.status = ApplicationStatus::Technical; tech.appliedDate = QDate(2026, 10, 15); QVERIFY(m_apps->insert(tech));
    JobApplication finalApp; finalApp.company = u"FinalCo"_s; finalApp.position = u"Dev"_s; finalApp.status = ApplicationStatus::Final; finalApp.appliedDate = QDate(2026, 10, 15); QVERIFY(m_apps->insert(finalApp));
    JobApplication offer; offer.company = u"OfferCo"_s; offer.position = u"Dev"_s; offer.status = ApplicationStatus::Offer; offer.appliedDate = QDate(2026, 10, 15); QVERIFY(m_apps->insert(offer));

    // 1 rejected with history applied -> hr_screen -> technical -> rejected
    JobApplication rej;
    rej.company = u"RejCo"_s;
    rej.position = u"Dev"_s;
    rej.status = ApplicationStatus::Applied;
    rej.appliedDate = QDate(2026, 10, 10);
    QVERIFY(m_apps->insert(rej));
    QVERIFY(m_apps->setStatus(rej.id, ApplicationStatus::HrScreen));
    QVERIFY(m_apps->setStatus(rej.id, ApplicationStatus::Technical));
    QVERIFY(m_apps->setStatus(rej.id, ApplicationStatus::Rejected));

    // 2. Sessions
    StudySession sSun; sSun.sessionDate = QDate(2026, 10, 25); sSun.minutes = 30; QVERIFY(m_sessions->insert(sSun));
    StudySession sMon; sMon.sessionDate = QDate(2026, 10, 26); sMon.minutes = 60; QVERIFY(m_sessions->insert(sMon));
    StudySession sTue; sTue.sessionDate = QDate(2026, 10, 27); sTue.minutes = 120; QVERIFY(m_sessions->insert(sTue));
    StudySession sWed; sWed.sessionDate = QDate(2026, 10, 28); sWed.minutes = 80; QVERIFY(m_sessions->insert(sWed));

    // 3. Interviews
    Interview ivPast; ivPast.applicationId = a1.id; ivPast.startAt = QDateTime(QDate(2026, 10, 27), QTime(14, 0)); QVERIFY(m_interviews->insert(ivPast));
    Interview ivEarlierToday; ivEarlierToday.applicationId = a1.id; ivEarlierToday.startAt = QDateTime(QDate(2026, 10, 29), QTime(8, 0)); QVERIFY(m_interviews->insert(ivEarlierToday));
    Interview iv1; iv1.applicationId = a1.id; iv1.startAt = QDateTime(QDate(2026, 10, 30), QTime(14, 0)); QVERIFY(m_interviews->insert(iv1));
    Interview ivCanc; ivCanc.applicationId = a1.id; ivCanc.startAt = QDateTime(QDate(2026, 10, 31), QTime(15, 0)); ivCanc.outcome = InterviewOutcome::Cancelled; QVERIFY(m_interviews->insert(ivCanc));
    Interview iv2; iv2.applicationId = a1.id; iv2.startAt = QDateTime(QDate(2026, 11, 4), QTime(11, 0)); QVERIFY(m_interviews->insert(iv2));
    Interview iv3; iv3.applicationId = a1.id; iv3.startAt = QDateTime(QDate(2026, 11, 4), QTime(23, 0)); QVERIFY(m_interviews->insert(iv3));
    Interview ivFuture; ivFuture.applicationId = a1.id; ivFuture.startAt = QDateTime(QDate(2026, 11, 5), QTime(9, 0)); QVERIFY(m_interviews->insert(ivFuture));

    // Compute stats
    const auto stats = m_service->computeStats(360, Qt::Monday);

    // Verify all metrics
    QCOMPARE(stats.appliedThisWeek, 2);
    QCOMPARE(stats.activeProcesses, 8);
    QCOMPARE(stats.interviewsNext7Days, 3);
    QCOMPARE(stats.studyStreakDays, 4);
    QCOMPARE(stats.weekMinutes, 260);
    QCOMPARE(stats.goalMinutes, 360);
    QCOMPARE(stats.goalPercentReal, 72);
    QCOMPARE(stats.goalPercentCapped, 72);

    // Verify Funnel
    QCOMPARE(stats.funnel.size(), 5);
    QCOMPARE(stats.funnel[0].status, ApplicationStatus::Applied);
    QCOMPARE(stats.funnel[0].reachedCount, 9);
    QCOMPARE(stats.funnel[1].status, ApplicationStatus::HrScreen);
    QCOMPARE(stats.funnel[1].reachedCount, 6);
    QCOMPARE(stats.funnel[2].status, ApplicationStatus::Technical);
    QCOMPARE(stats.funnel[2].reachedCount, 4);
    QCOMPARE(stats.funnel[3].status, ApplicationStatus::Final);
    QCOMPARE(stats.funnel[3].reachedCount, 2);
    QCOMPARE(stats.funnel[4].status, ApplicationStatus::Offer);
    QCOMPARE(stats.funnel[4].reachedCount, 1);

    QVERIFY(stats.interviewRate.has_value());
    QVERIFY(qAbs(*stats.interviewRate - (6.0 / 9.0)) < 0.0001);

    // Test goal 480
    const auto stats480 = m_service->computeStats(480, Qt::Monday);
    QCOMPARE(stats480.goalMinutes, 480);
    QCOMPARE(stats480.goalPercentReal, 54);
    QCOMPARE(stats480.goalPercentCapped, 54);
}

void tst_StatsService::testSundayFirst() {
    // Populate dataset same as testHandCalculatedDataset
    testHandCalculatedDataset();

    // Compute stats with Sunday-first
    // Week is Oct 25 - Oct 31 -> includes Sun Oct 25 (30m) + Mon (60m) + Tue (120m) + Wed (80m) = 290m
    const auto stats = m_service->computeStats(360, Qt::Sunday);
    QCOMPARE(stats.weekMinutes, 290);
    QCOMPARE(stats.studyStreakDays, 4);
    QCOMPARE(stats.appliedThisWeek, 2); // Oct 27, Oct 29
}

void tst_StatsService::testFutureDatedSessionInWeek() {
    testHandCalculatedDataset();

    // Add session Fri Oct 30 = 45m (inside current Monday-first week, but in the future relative to clock Oct 29)
    StudySession sFri;
    sFri.sessionDate = QDate(2026, 10, 30);
    sFri.minutes = 45;
    QVERIFY(m_sessions->insert(sFri));

    const auto stats = m_service->computeStats(360, Qt::Monday);
    // Weekly minutes includes the Friday session: 260 + 45 = 305
    QCOMPARE(stats.weekMinutes, 305);
    // Streak ignores the future session: still 4 days
    QCOMPARE(stats.studyStreakDays, 4);
}

void tst_StatsService::testZeroOrNegativeGoal() {
    const auto statsZero = m_service->computeStats(0, Qt::Monday);
    QCOMPARE(statsZero.goalMinutes, 0);
    QCOMPARE(statsZero.goalPercentReal, 0);
    QCOMPARE(statsZero.goalPercentCapped, 0);

    const auto statsNeg = m_service->computeStats(-100, Qt::Monday);
    QCOMPARE(statsNeg.goalMinutes, -100);
    QCOMPARE(statsNeg.goalPercentReal, 0);
    QCOMPARE(statsNeg.goalPercentCapped, 0);
}

void tst_StatsService::testGoalCapping() {
    const QDateTime now(QDate(2026, 10, 29), QTime(10, 0));
    m_service->setClock([now] { return now; });

    // Session of 430 minutes in current week
    StudySession s;
    s.sessionDate = QDate(2026, 10, 27);
    s.minutes = 430;
    QVERIFY(m_sessions->insert(s));

    // Goal 360: real is (430 * 100) / 360 = 119%, capped is 100%
    const auto stats = m_service->computeStats(360, Qt::Monday);
    QCOMPARE(stats.weekMinutes, 430);
    QCOMPARE(stats.goalPercentReal, 119);
    QCOMPARE(stats.goalPercentCapped, 100);
}

void tst_StatsService::testTrackProgress() {
    Track trCpp; trCpp.name = u"C++"_s; trCpp.color = u"#5B6CFF"_s; QVERIFY(m_tracks->insert(trCpp));
    Track trEng; trEng.name = u"English"_s; trEng.color = u"#F59E0B"_s; QVERIFY(m_tracks->insert(trEng));

    // C++ has 2 topics:
    // Topic 1: InProgress (subtasks: 2/4 done -> 50%)
    Topic top1; top1.trackId = trCpp.id; top1.title = u"Pointers"_s; top1.status = TopicStatus::InProgress; QVERIFY(m_topics->insert(top1));
    Subtask sub1; sub1.topicId = top1.id; sub1.text = u"s1"_s; sub1.done = true; QVERIFY(m_topics->addSubtask(sub1));
    Subtask sub2; sub2.topicId = top1.id; sub2.text = u"s2"_s; sub2.done = true; QVERIFY(m_topics->addSubtask(sub2));
    Subtask sub3; sub3.topicId = top1.id; sub3.text = u"s3"_s; sub3.done = false; QVERIFY(m_topics->addSubtask(sub3));
    Subtask sub4; sub4.topicId = top1.id; sub4.text = u"s4"_s; sub4.done = false; QVERIFY(m_topics->addSubtask(sub4));

    // Topic 2: Done (no subtasks -> 100%)
    Topic top2; top2.trackId = trCpp.id; top2.title = u"Move"_s; top2.status = TopicStatus::Done; QVERIFY(m_topics->insert(top2));

    // English has 0 topics -> 0%

    const auto stats = m_service->computeStats(360, Qt::Monday);
    QCOMPARE(stats.trackProgress.size(), 2);

    // C++ average: (50 + 100) / 2 = 75%
    QCOMPARE(stats.trackProgress[0].trackName, u"C++"_s);
    QCOMPARE(stats.trackProgress[0].percent, 75);

    // English: 0 topics -> 0%
    QCOMPARE(stats.trackProgress[1].trackName, u"English"_s);
    QCOMPARE(stats.trackProgress[1].percent, 0);
}

void tst_StatsService::testInterviewRateZeroDenominator() {
    // 0 applications
    const auto stats = m_service->computeStats(360, Qt::Monday);
    QVERIFY(!stats.interviewRate.has_value());
}

void tst_StatsService::testPerformanceSanity1000Apps5000Sessions() {
    // Amendment A9: 1,000 applications with history and 5,000 sessions in in-memory DB.
    // Assert generous bound < 1000 ms (SPEC §11 cold-start budget) and print measured time with qInfo.

    auto transaction = m_db->db().transaction();
    QVERIFY(transaction.isActive());

    // Insert 1,000 applications, each with 2 history rows
    const QDate baseDate(2026, 1, 1);
    for (int i = 1; i <= 1000; ++i) {
        JobApplication app;
        app.company = u"Company_%1"_s.arg(i);
        app.position = u"Position_%1"_s.arg(i);
        app.status = (i % 2 == 0) ? ApplicationStatus::Technical : ApplicationStatus::Applied;
        app.appliedDate = baseDate.addDays(i % 300);
        QVERIFY(m_apps->insert(app));
        QVERIFY(m_apps->setStatus(app.id, ApplicationStatus::HrScreen));
    }

    // Insert 5,000 sessions
    for (int i = 1; i <= 5000; ++i) {
        StudySession s;
        s.sessionDate = baseDate.addDays(i % 300);
        s.minutes = 15 + (i % 60);
        QVERIFY(m_sessions->insert(s));
    }

    QVERIFY(transaction.commit());

    // Measure computeStats
    QElapsedTimer timer;
    timer.start();
    const auto stats = m_service->computeStats(360, Qt::Monday);
    const qint64 elapsedMs = timer.elapsed();

    qInfo() << "computeStats with 1,000 apps and 5,000 sessions completed in:" << elapsedMs << "ms";

    QVERIFY(stats.activeProcesses > 0);
    QVERIFY(elapsedMs < 1000);
}

QTEST_GUILESS_MAIN(tst_StatsService)
#include "tst_StatsService.moc"
