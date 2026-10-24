#include <QDate>
#include <QDateTime>
#include <QTest>
#include <QTime>
#include "TestDatabase.h"
#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/Enums.h"
#include "domain/Structs.h"
#include "services/AgendaService.h"

using namespace Qt::StringLiterals;
using JobPrep::Data::ApplicationRepository;
using JobPrep::Data::InterviewRepository;
using JobPrep::Data::TopicRepository;
using JobPrep::Data::TrackRepository;
using JobPrep::Domain::AgendaItem;
using JobPrep::Domain::AgendaKind;
using JobPrep::Domain::ApplicationStatus;
using JobPrep::Domain::Interview;
using JobPrep::Domain::InterviewOutcome;
using JobPrep::Domain::InterviewType;
using JobPrep::Domain::JobApplication;
using JobPrep::Domain::Topic;
using JobPrep::Domain::TopicStatus;
using JobPrep::Domain::Track;
using JobPrep::Services::AgendaService;
using JobPrep::Tests::TestDatabase;

class tst_AgendaService : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void noItemsForEmptyDatabase();
    void interviewAppearsOnItsStartDay();
    void cancelledInterviewExcluded();
    void pastInterviewFlagged();
    void dayBoundaryInterviewCheck();
    void followUpForActiveAndWishlistApplications();
    void pastFollowUpFlagged();
    void studyTargetForOpenTopics();
    void pastStudyTargetFlagged();
    void rangeAcrossMonthBorder();
    void rangeAcrossYearEnd();
    void leapDayMonth();
    void lateNightInterview();
    void deterministicOrdering();
    void singleDayRange();

    // Today items & upcoming tests (amendment A4)
    void todayItemsIncludesOverdueFollowUp();
    void todayItemsIncludesOverdueTopic();
    void todayItemsExcludesDoneTopic();
    void todayItemsExcludesClosedApplication();
    void todayItemsExcludesCancelledInterview();
    void todayItemsEarlierTodayInterviewHasPast();
    void todayItemsOrderingOverdueFirstThenTodayTimed();
    void upcomingInterviewsExcludesCancelledOrdersAndLimits();
    void upcomingInterviewsAcrossMonthAndYearBorder();

private:
    int addApplication(const QString& company, ApplicationStatus status = ApplicationStatus::Applied);
    int addTrack(const QString& name);

    std::unique_ptr<TestDatabase> m_db;
    std::unique_ptr<TrackRepository> m_tracks;
    std::unique_ptr<TopicRepository> m_topics;
    std::unique_ptr<ApplicationRepository> m_apps;
    std::unique_ptr<InterviewRepository> m_interviews;
    std::unique_ptr<AgendaService> m_service;
};

void tst_AgendaService::init() {
    m_db = std::make_unique<TestDatabase>(u"tst_agenda"_s);
    QVERIFY(m_db->open(u":memory:"_s));
    m_tracks = std::make_unique<TrackRepository>(m_db->db());
    m_topics = std::make_unique<TopicRepository>(m_db->db());
    m_apps = std::make_unique<ApplicationRepository>(m_db->db());
    m_interviews = std::make_unique<InterviewRepository>(m_db->db());
    m_service = std::make_unique<AgendaService>(*m_interviews, *m_apps, *m_topics, *m_tracks);
}

void tst_AgendaService::cleanup() {
    m_service.reset();
    m_interviews.reset();
    m_apps.reset();
    m_topics.reset();
    m_tracks.reset();
    m_db.reset();
}

int tst_AgendaService::addApplication(const QString& company, ApplicationStatus status) {
    JobApplication app;
    app.company = company;
    app.position = u"Software Engineer"_s;
    app.status = status;
    return m_apps->insert(app) ? app.id : 0;
}

int tst_AgendaService::addTrack(const QString& name) {
    Track track;
    track.name = name;
    track.color = u"#5B6CFF"_s;
    return m_tracks->insert(track) ? track.id : 0;
}

void tst_AgendaService::noItemsForEmptyDatabase() {
    const auto items = m_service->items(QDate(2026, 10, 1), QDate(2026, 10, 31));
    QVERIFY(items.isEmpty());
}

void tst_AgendaService::interviewAppearsOnItsStartDay() {
    const int appId = addApplication(u"Globex"_s);

    Interview iv;
    iv.applicationId = appId;
    iv.startAt = QDateTime(QDate(2026, 10, 15), QTime(14, 0));
    iv.type = InterviewType::Technical;
    iv.place = u"Zoom"_s;
    iv.interviewer = u"Anna K."_s;
    QVERIFY(m_interviews->insert(iv));

    const auto items = m_service->items(QDate(2026, 10, 1), QDate(2026, 10, 31));
    QCOMPARE(items.size(), 1);
    const auto& item = items.first();
    QCOMPARE(item.kind, AgendaKind::Interview);
    QCOMPARE(item.date, QDate(2026, 10, 15));
    QVERIFY(item.time.has_value());
    QCOMPARE(*item.time, QTime(14, 0));
    QCOMPARE(item.title, u"Technical · Globex"_s);
    QCOMPARE(item.subtitle, u"Zoom · Anna K."_s);
    QCOMPARE(item.refId, iv.id);
}

void tst_AgendaService::cancelledInterviewExcluded() {
    const int appId = addApplication(u"Globex"_s);

    Interview iv;
    iv.applicationId = appId;
    iv.startAt = QDateTime(QDate(2026, 10, 15), QTime(14, 0));
    iv.type = InterviewType::Technical;
    iv.outcome = InterviewOutcome::Cancelled;
    QVERIFY(m_interviews->insert(iv));

    const auto items = m_service->items(QDate(2026, 10, 1), QDate(2026, 10, 31));
    QVERIFY(items.isEmpty());
}

void tst_AgendaService::pastInterviewFlagged() {
    const int appId = addApplication(u"Globex"_s);

    // Fixed clock at 2026-10-15 12:00:00
    const QDateTime fixedClock(QDate(2026, 10, 15), QTime(12, 0, 0));
    m_service->setClock([fixedClock] { return fixedClock; });

    // Past interview: 10:00
    Interview ivPast;
    ivPast.applicationId = appId;
    ivPast.startAt = QDateTime(QDate(2026, 10, 15), QTime(10, 0, 0));
    QVERIFY(m_interviews->insert(ivPast));

    // Future interview: 14:00
    Interview ivFuture;
    ivFuture.applicationId = appId;
    ivFuture.startAt = QDateTime(QDate(2026, 10, 15), QTime(14, 0, 0));
    QVERIFY(m_interviews->insert(ivFuture));

    const auto items = m_service->items(QDate(2026, 10, 15), QDate(2026, 10, 15));
    QCOMPARE(items.size(), 2);
    QCOMPARE(items[0].refId, ivPast.id);
    QVERIFY(items[0].past);
    QCOMPARE(items[1].refId, ivFuture.id);
    QVERIFY(!items[1].past);
}

void tst_AgendaService::dayBoundaryInterviewCheck() {
    const int appId = addApplication(u"BoundaryCo"_s);

    const QDate from(2026, 10, 10);
    const QDate to(2026, 10, 12);

    // 1. Exactly on from at 00:00:00 -> INCLUDED
    Interview ivStart;
    ivStart.applicationId = appId;
    ivStart.startAt = QDateTime(from, QTime(0, 0, 0));
    QVERIFY(m_interviews->insert(ivStart));

    // 2. Exactly on to at 23:59:59 -> INCLUDED
    Interview ivEnd;
    ivEnd.applicationId = appId;
    ivEnd.startAt = QDateTime(to, QTime(23, 59, 59));
    QVERIFY(m_interviews->insert(ivEnd));

    // 3. Exactly on to+1 day at 00:00:00 -> EXCLUDED (half-open bound)
    Interview ivNextDay;
    ivNextDay.applicationId = appId;
    ivNextDay.startAt = QDateTime(to.addDays(1), QTime(0, 0, 0));
    QVERIFY(m_interviews->insert(ivNextDay));

    const auto items = m_service->items(from, to);
    QCOMPARE(items.size(), 2);
    QCOMPARE(items[0].refId, ivStart.id);
    QCOMPARE(items[1].refId, ivEnd.id);
}

void tst_AgendaService::followUpForActiveAndWishlistApplications() {
    // Active app
    JobApplication appActive;
    appActive.company = u"Acme"_s;
    appActive.position = u"Dev"_s;
    appActive.status = ApplicationStatus::Applied;
    appActive.nextAction = u"Check status"_s;
    appActive.nextActionDate = QDate(2026, 10, 5);
    QVERIFY(m_apps->insert(appActive));

    // Wishlist app
    JobApplication appWishlist;
    appWishlist.company = u"DreamWorks"_s;
    appWishlist.position = u"Dev"_s;
    appWishlist.status = ApplicationStatus::Wishlist;
    appWishlist.nextAction = u"Reach out on LinkedIn"_s;
    appWishlist.nextActionDate = QDate(2026, 10, 7);
    QVERIFY(m_apps->insert(appWishlist));

    // Closed app (Rejected) - should NOT show up
    JobApplication appClosed;
    appClosed.company = u"RejectCo"_s;
    appClosed.position = u"Dev"_s;
    appClosed.status = ApplicationStatus::Rejected;
    appClosed.nextAction = u"Ask for feedback"_s;
    appClosed.nextActionDate = QDate(2026, 10, 6);
    QVERIFY(m_apps->insert(appClosed));

    const auto items = m_service->items(QDate(2026, 10, 1), QDate(2026, 10, 10));
    QCOMPARE(items.size(), 2);
    QCOMPARE(items[0].title, u"Follow-up · Acme"_s);
    QCOMPARE(items[0].subtitle, u"Check status"_s);
    QCOMPARE(items[0].refId, appActive.id);

    QCOMPARE(items[1].title, u"Follow-up · DreamWorks"_s);
    QCOMPARE(items[1].subtitle, u"Reach out on LinkedIn"_s);
    QCOMPARE(items[1].refId, appWishlist.id);
}

void tst_AgendaService::pastFollowUpFlagged() {
    // Clock at 2026-10-15
    const QDateTime fixedClock(QDate(2026, 10, 15), QTime(9, 0, 0));
    m_service->setClock([fixedClock] { return fixedClock; });

    // Past follow-up (2026-10-14)
    JobApplication appPast;
    appPast.company = u"PastCo"_s;
    appPast.position = u"Dev"_s;
    appPast.status = ApplicationStatus::Applied;
    appPast.nextAction = u"Call"_s;
    appPast.nextActionDate = QDate(2026, 10, 14);
    QVERIFY(m_apps->insert(appPast));

    // Today follow-up (2026-10-15) - not past
    JobApplication appToday;
    appToday.company = u"TodayCo"_s;
    appToday.position = u"Dev"_s;
    appToday.status = ApplicationStatus::Applied;
    appToday.nextAction = u"Call"_s;
    appToday.nextActionDate = QDate(2026, 10, 15);
    QVERIFY(m_apps->insert(appToday));

    // Future follow-up (2026-10-16) - not past
    JobApplication appFuture;
    appFuture.company = u"FutureCo"_s;
    appFuture.position = u"Dev"_s;
    appFuture.status = ApplicationStatus::Applied;
    appFuture.nextAction = u"Call"_s;
    appFuture.nextActionDate = QDate(2026, 10, 16);
    QVERIFY(m_apps->insert(appFuture));

    const auto items = m_service->items(QDate(2026, 10, 1), QDate(2026, 10, 20));
    QCOMPARE(items.size(), 3);
    QCOMPARE(items[0].refId, appPast.id);
    QVERIFY(items[0].past);
    QCOMPARE(items[1].refId, appToday.id);
    QVERIFY(!items[1].past);
    QCOMPARE(items[2].refId, appFuture.id);
    QVERIFY(!items[2].past);
}

void tst_AgendaService::studyTargetForOpenTopics() {
    const int trackId = addTrack(u"C++"_s);

    // Open topic (Backlog)
    Topic openTopic;
    openTopic.trackId = trackId;
    openTopic.title = u"Smart Pointers"_s;
    openTopic.status = TopicStatus::Backlog;
    openTopic.targetDate = QDate(2026, 10, 8);
    QVERIFY(m_topics->insert(openTopic));

    // In progress topic
    Topic inProgTopic;
    inProgTopic.trackId = trackId;
    inProgTopic.title = u"Move Semantics"_s;
    inProgTopic.status = TopicStatus::InProgress;
    inProgTopic.targetDate = QDate(2026, 10, 9);
    QVERIFY(m_topics->insert(inProgTopic));

    // Done topic (should be excluded)
    Topic doneTopic;
    doneTopic.trackId = trackId;
    doneTopic.title = u"Basic Types"_s;
    doneTopic.status = TopicStatus::Done;
    doneTopic.targetDate = QDate(2026, 10, 8);
    QVERIFY(m_topics->insert(doneTopic));

    const auto items = m_service->items(QDate(2026, 10, 1), QDate(2026, 10, 10));
    QCOMPARE(items.size(), 2);
    QCOMPARE(items[0].kind, AgendaKind::StudyTarget);
    QCOMPARE(items[0].title, u"Target · Smart Pointers"_s);
    QCOMPARE(items[0].subtitle, u"C++"_s);
    QCOMPARE(items[0].refId, openTopic.id);

    QCOMPARE(items[1].kind, AgendaKind::StudyTarget);
    QCOMPARE(items[1].title, u"Target · Move Semantics"_s);
    QCOMPARE(items[1].subtitle, u"C++"_s);
    QCOMPARE(items[1].refId, inProgTopic.id);
}

void tst_AgendaService::pastStudyTargetFlagged() {
    // Clock at 2026-10-15; targets compare their date to today.
    const QDateTime fixedClock(QDate(2026, 10, 15), QTime(9, 0, 0));
    m_service->setClock([fixedClock] { return fixedClock; });

    const int trackId = addTrack(u"C++"_s);

    Topic pastTopic;
    pastTopic.trackId = trackId;
    pastTopic.title = u"Past target"_s;
    pastTopic.targetDate = QDate(2026, 10, 14);
    QVERIFY(m_topics->insert(pastTopic));

    Topic todayTopic;
    todayTopic.trackId = trackId;
    todayTopic.title = u"Today target"_s;
    todayTopic.targetDate = QDate(2026, 10, 15);
    QVERIFY(m_topics->insert(todayTopic));

    Topic futureTopic;
    futureTopic.trackId = trackId;
    futureTopic.title = u"Future target"_s;
    futureTopic.targetDate = QDate(2026, 10, 16);
    QVERIFY(m_topics->insert(futureTopic));

    const auto items = m_service->items(QDate(2026, 10, 1), QDate(2026, 10, 20));
    QCOMPARE(items.size(), 3);
    QCOMPARE(items[0].refId, pastTopic.id);
    QVERIFY(items[0].past);
    QCOMPARE(items[1].refId, todayTopic.id);
    QVERIFY(!items[1].past);
    QCOMPARE(items[2].refId, futureTopic.id);
    QVERIFY(!items[2].past);
}

void tst_AgendaService::rangeAcrossMonthBorder() {
    const int appId = addApplication(u"BorderCo"_s);

    // 31 Oct
    Interview ivOct;
    ivOct.applicationId = appId;
    ivOct.startAt = QDateTime(QDate(2026, 10, 31), QTime(10, 0));
    QVERIFY(m_interviews->insert(ivOct));

    // 1 Nov
    Interview ivNov;
    ivNov.applicationId = appId;
    ivNov.startAt = QDateTime(QDate(2026, 11, 1), QTime(14, 0));
    QVERIFY(m_interviews->insert(ivNov));

    const auto items = m_service->items(QDate(2026, 10, 31), QDate(2026, 11, 1));
    QCOMPARE(items.size(), 2);
    QCOMPARE(items[0].date, QDate(2026, 10, 31));
    QCOMPARE(items[1].date, QDate(2026, 11, 1));
}

void tst_AgendaService::rangeAcrossYearEnd() {
    const int appId = addApplication(u"YearCo"_s);

    Interview ivDec;
    ivDec.applicationId = appId;
    ivDec.startAt = QDateTime(QDate(2026, 12, 31), QTime(15, 0));
    QVERIFY(m_interviews->insert(ivDec));

    Interview ivJan;
    ivJan.applicationId = appId;
    ivJan.startAt = QDateTime(QDate(2027, 1, 1), QTime(10, 0));
    QVERIFY(m_interviews->insert(ivJan));

    const auto items = m_service->items(QDate(2026, 12, 31), QDate(2027, 1, 1));
    QCOMPARE(items.size(), 2);
    QCOMPARE(items[0].date, QDate(2026, 12, 31));
    QCOMPARE(items[1].date, QDate(2027, 1, 1));
}

void tst_AgendaService::leapDayMonth() {
    const int appId = addApplication(u"LeapCo"_s);

    // 2028 is a leap year; 29 Feb 2028 is valid
    Interview iv;
    iv.applicationId = appId;
    iv.startAt = QDateTime(QDate(2028, 2, 29), QTime(11, 0));
    QVERIFY(m_interviews->insert(iv));

    const auto items = m_service->items(QDate(2028, 2, 1), QDate(2028, 2, 29));
    QCOMPARE(items.size(), 1);
    QCOMPARE(items.first().date, QDate(2028, 2, 29));
}

void tst_AgendaService::lateNightInterview() {
    const int appId = addApplication(u"NightCo"_s);

    // 23:30 on 2026-10-15 must belong to 2026-10-15, not 16
    Interview iv;
    iv.applicationId = appId;
    iv.startAt = QDateTime(QDate(2026, 10, 15), QTime(23, 30));
    QVERIFY(m_interviews->insert(iv));

    const auto items = m_service->items(QDate(2026, 10, 15), QDate(2026, 10, 15));
    QCOMPARE(items.size(), 1);
    QCOMPARE(items.first().date, QDate(2026, 10, 15));
    QCOMPARE(items.first().time, std::optional<QTime>(QTime(23, 30)));
}

void tst_AgendaService::deterministicOrdering() {
    const int trackId = addTrack(u"English"_s);

    // All on same date: 2026-10-15
    const QDate targetDate(2026, 10, 15);

    // 1. Follow-up B
    JobApplication appB;
    appB.company = u"Beta"_s;
    appB.position = u"Dev"_s;
    appB.status = ApplicationStatus::Applied;
    appB.nextAction = u"Action B"_s;
    appB.nextActionDate = targetDate;
    QVERIFY(m_apps->insert(appB));

    // 2. Follow-up A
    JobApplication appA;
    appA.company = u"Alpha"_s;
    appA.position = u"Dev"_s;
    appA.status = ApplicationStatus::Applied;
    appA.nextAction = u"Action A"_s;
    appA.nextActionDate = targetDate;
    QVERIFY(m_apps->insert(appA));

    // 3. Interview late (15:00)
    Interview ivLate;
    ivLate.applicationId = appA.id;
    ivLate.startAt = QDateTime(targetDate, QTime(15, 0));
    ivLate.type = InterviewType::Hr;
    QVERIFY(m_interviews->insert(ivLate));

    // 4. Interview early (10:00)
    Interview ivEarly;
    ivEarly.applicationId = appB.id;
    ivEarly.startAt = QDateTime(targetDate, QTime(10, 0));
    ivEarly.type = InterviewType::Technical;
    QVERIFY(m_interviews->insert(ivEarly));

    // 5. Study target
    Topic topic;
    topic.trackId = trackId;
    topic.title = u"STAR Stories"_s;
    topic.status = TopicStatus::Backlog;
    topic.targetDate = targetDate;
    QVERIFY(m_topics->insert(topic));

    const auto items = m_service->items(targetDate, targetDate);
    QCOMPARE(items.size(), 5);

    // Expected order on same date:
    // 1. Interviews (by time): 10:00 Technical · Beta, then 15:00 Hr · Alpha
    QCOMPARE(items[0].kind, AgendaKind::Interview);
    QCOMPARE(*items[0].time, QTime(10, 0));

    QCOMPARE(items[1].kind, AgendaKind::Interview);
    QCOMPARE(*items[1].time, QTime(15, 0));

    // 2. Follow-ups (by title): Follow-up · Alpha, then Follow-up · Beta
    QCOMPARE(items[2].kind, AgendaKind::FollowUp);
    QCOMPARE(items[2].title, u"Follow-up · Alpha"_s);

    QCOMPARE(items[3].kind, AgendaKind::FollowUp);
    QCOMPARE(items[3].title, u"Follow-up · Beta"_s);

    // 3. Study target
    QCOMPARE(items[4].kind, AgendaKind::StudyTarget);
    QCOMPARE(items[4].title, u"Target · STAR Stories"_s);
}

void tst_AgendaService::singleDayRange() {
    const int appId = addApplication(u"SingleCo"_s);

    Interview ivPrev;
    ivPrev.applicationId = appId;
    ivPrev.startAt = QDateTime(QDate(2026, 10, 14), QTime(10, 0));
    QVERIFY(m_interviews->insert(ivPrev));

    Interview ivTarget;
    ivTarget.applicationId = appId;
    ivTarget.startAt = QDateTime(QDate(2026, 10, 15), QTime(10, 0));
    QVERIFY(m_interviews->insert(ivTarget));

    Interview ivNext;
    ivNext.applicationId = appId;
    ivNext.startAt = QDateTime(QDate(2026, 10, 16), QTime(10, 0));
    QVERIFY(m_interviews->insert(ivNext));

    const auto items = m_service->items(QDate(2026, 10, 15), QDate(2026, 10, 15));
    QCOMPARE(items.size(), 1);
    QCOMPARE(items.first().refId, ivTarget.id);
}

void tst_AgendaService::todayItemsIncludesOverdueFollowUp() {
    const QDate today(2026, 10, 20);
    m_service->setClock([today] { return QDateTime(today, QTime(10, 0)); });

    JobApplication appOverdue;
    appOverdue.company = u"OverdueCo"_s;
    appOverdue.position = u"Dev"_s;
    appOverdue.status = ApplicationStatus::Applied;
    appOverdue.nextAction = u"Send reminder"_s;
    appOverdue.nextActionDate = QDate(2026, 10, 15); // 5 days overdue
    QVERIFY(m_apps->insert(appOverdue));

    JobApplication appToday;
    appToday.company = u"TodayCo"_s;
    appToday.position = u"Dev"_s;
    appToday.status = ApplicationStatus::Applied;
    appToday.nextAction = u"Call recruiter"_s;
    appToday.nextActionDate = today;
    QVERIFY(m_apps->insert(appToday));

    const auto items = m_service->todayItems(today);
    QCOMPARE(items.size(), 2);
    // Overdue item comes first, has past = true
    QCOMPARE(items[0].refId, appOverdue.id);
    QCOMPARE(items[0].kind, AgendaKind::FollowUp);
    QCOMPARE(items[0].date, QDate(2026, 10, 15));
    QVERIFY(items[0].past);

    // Today's item has past = false
    QCOMPARE(items[1].refId, appToday.id);
    QCOMPARE(items[1].kind, AgendaKind::FollowUp);
    QCOMPARE(items[1].date, today);
    QVERIFY(!items[1].past);
}

void tst_AgendaService::todayItemsIncludesOverdueTopic() {
    const QDate today(2026, 10, 20);
    m_service->setClock([today] { return QDateTime(today, QTime(10, 0)); });

    const int trackId = addTrack(u"C++"_s);

    Topic topicOverdue;
    topicOverdue.trackId = trackId;
    topicOverdue.title = u"Pointers"_s;
    topicOverdue.status = TopicStatus::InProgress;
    topicOverdue.targetDate = QDate(2026, 10, 12);
    QVERIFY(m_topics->insert(topicOverdue));

    Topic topicToday;
    topicToday.trackId = trackId;
    topicToday.title = u"Templates"_s;
    topicToday.status = TopicStatus::Review;
    topicToday.targetDate = today;
    QVERIFY(m_topics->insert(topicToday));

    const auto items = m_service->todayItems(today);
    QCOMPARE(items.size(), 2);
    QCOMPARE(items[0].refId, topicOverdue.id);
    QCOMPARE(items[0].kind, AgendaKind::StudyTarget);
    QVERIFY(items[0].past);

    QCOMPARE(items[1].refId, topicToday.id);
    QCOMPARE(items[1].kind, AgendaKind::StudyTarget);
    QVERIFY(!items[1].past);
}

void tst_AgendaService::todayItemsExcludesDoneTopic() {
    const QDate today(2026, 10, 20);
    const int trackId = addTrack(u"C++"_s);

    Topic topicDone;
    topicDone.trackId = trackId;
    topicDone.title = u"Done Topic"_s;
    topicDone.status = TopicStatus::Done;
    topicDone.targetDate = QDate(2026, 10, 15); // overdue, but Done!
    QVERIFY(m_topics->insert(topicDone));

    const auto items = m_service->todayItems(today);
    QCOMPARE(items.size(), 0);
}

void tst_AgendaService::todayItemsExcludesClosedApplication() {
    const QDate today(2026, 10, 20);

    // Rejected application
    JobApplication appRej;
    appRej.company = u"RejCo"_s;
    appRej.position = u"Dev"_s;
    appRej.status = ApplicationStatus::Rejected;
    appRej.nextAction = u"Follow up"_s;
    appRej.nextActionDate = QDate(2026, 10, 15);
    QVERIFY(m_apps->insert(appRej));

    // Accepted application
    JobApplication appAcc;
    appAcc.company = u"AccCo"_s;
    appAcc.position = u"Dev"_s;
    appAcc.status = ApplicationStatus::Accepted;
    appAcc.nextAction = u"Paperwork"_s;
    appAcc.nextActionDate = today;
    QVERIFY(m_apps->insert(appAcc));

    const auto items = m_service->todayItems(today);
    QCOMPARE(items.size(), 0);
}

void tst_AgendaService::todayItemsExcludesCancelledInterview() {
    const QDate today(2026, 10, 20);
    const int appId = addApplication(u"CancCo"_s);

    Interview iv;
    iv.applicationId = appId;
    iv.startAt = QDateTime(today, QTime(14, 0));
    iv.outcome = InterviewOutcome::Cancelled;
    QVERIFY(m_interviews->insert(iv));

    const auto items = m_service->todayItems(today);
    QCOMPARE(items.size(), 0);
}

void tst_AgendaService::todayItemsEarlierTodayInterviewHasPast() {
    const QDate today(2026, 10, 20);
    // Clock at 14:00 today
    m_service->setClock([today] { return QDateTime(today, QTime(14, 0)); });

    const int appId = addApplication(u"TimeCo"_s);

    // Interview at 10:00 earlier today -> past = true
    Interview ivMorning;
    ivMorning.applicationId = appId;
    ivMorning.startAt = QDateTime(today, QTime(10, 0));
    QVERIFY(m_interviews->insert(ivMorning));

    // Interview at 16:00 later today -> past = false
    Interview ivAfternoon;
    ivAfternoon.applicationId = appId;
    ivAfternoon.startAt = QDateTime(today, QTime(16, 0));
    QVERIFY(m_interviews->insert(ivAfternoon));

    const auto items = m_service->todayItems(today);
    QCOMPARE(items.size(), 2);
    QCOMPARE(items[0].refId, ivMorning.id);
    QVERIFY(items[0].past);

    QCOMPARE(items[1].refId, ivAfternoon.id);
    QVERIFY(!items[1].past);
}

void tst_AgendaService::todayItemsOrderingOverdueFirstThenTodayTimed() {
    const QDate today(2026, 10, 20);
    m_service->setClock([today] { return QDateTime(today, QTime(12, 0)); });

    const int appId = addApplication(u"OrderCo"_s);
    const int trackId = addTrack(u"English"_s);

    // 1. Overdue follow-up (older: Oct 10)
    JobApplication appOld;
    appOld.company = u"OldFollowUp"_s;
    appOld.position = u"Dev"_s;
    appOld.nextAction = u"Act"_s;
    appOld.nextActionDate = QDate(2026, 10, 10);
    QVERIFY(m_apps->insert(appOld));

    // 2. Overdue topic (newer overdue: Oct 15)
    Topic topicOverdue;
    topicOverdue.trackId = trackId;
    topicOverdue.title = u"OverdueTopic"_s;
    topicOverdue.targetDate = QDate(2026, 10, 15);
    QVERIFY(m_topics->insert(topicOverdue));

    // 3. Today interview late (15:00)
    Interview ivLate;
    ivLate.applicationId = appId;
    ivLate.startAt = QDateTime(today, QTime(15, 0));
    QVERIFY(m_interviews->insert(ivLate));

    // 4. Today interview early (09:00)
    Interview ivEarly;
    ivEarly.applicationId = appId;
    ivEarly.startAt = QDateTime(today, QTime(9, 0));
    QVERIFY(m_interviews->insert(ivEarly));

    // 5. Today follow-up (no time)
    JobApplication appToday;
    appToday.company = u"TodayFollowUp"_s;
    appToday.position = u"Dev"_s;
    appToday.nextAction = u"Act Today"_s;
    appToday.nextActionDate = today;
    QVERIFY(m_apps->insert(appToday));

    const auto items = m_service->todayItems(today);
    QCOMPARE(items.size(), 5);

    // Overdue first (by date):
    // 0: OldFollowUp (2026-10-10)
    QCOMPARE(items[0].refId, appOld.id);
    QCOMPARE(items[0].date, QDate(2026, 10, 10));

    // 1: OverdueTopic (2026-10-15)
    QCOMPARE(items[1].refId, topicOverdue.id);
    QCOMPARE(items[1].date, QDate(2026, 10, 15));

    // Today's timed items next (by time):
    // 2: 09:00 interview
    QCOMPARE(items[2].refId, ivEarly.id);
    QCOMPARE(*items[2].time, QTime(9, 0));

    // 3: 15:00 interview
    QCOMPARE(items[3].refId, ivLate.id);
    QCOMPARE(*items[3].time, QTime(15, 0));

    // Today's untimed items last:
    // 4: TodayFollowUp
    QCOMPARE(items[4].refId, appToday.id);
    QVERIFY(!items[4].time.has_value());
}

void tst_AgendaService::upcomingInterviewsExcludesCancelledOrdersAndLimits() {
    const int appId = addApplication(u"UpcomingCo"_s);
    const QDateTime now(QDate(2026, 10, 20), QTime(10, 0));
    m_service->setClock([now] { return now; });

    // Past interview (should not be returned if query starts from `now`)
    Interview ivPast;
    ivPast.applicationId = appId;
    ivPast.startAt = QDateTime(QDate(2026, 10, 20), QTime(9, 0));
    QVERIFY(m_interviews->insert(ivPast));

    // Upcoming 1
    Interview iv1;
    iv1.applicationId = appId;
    iv1.startAt = QDateTime(QDate(2026, 10, 21), QTime(14, 0));
    QVERIFY(m_interviews->insert(iv1));

    // Cancelled interview
    Interview ivCanc;
    ivCanc.applicationId = appId;
    ivCanc.startAt = QDateTime(QDate(2026, 10, 22), QTime(11, 0));
    ivCanc.outcome = InterviewOutcome::Cancelled;
    QVERIFY(m_interviews->insert(ivCanc));

    // Upcoming 2
    Interview iv2;
    iv2.applicationId = appId;
    iv2.startAt = QDateTime(QDate(2026, 10, 23), QTime(15, 0));
    QVERIFY(m_interviews->insert(iv2));

    // Upcoming 3
    Interview iv3;
    iv3.applicationId = appId;
    iv3.startAt = QDateTime(QDate(2026, 10, 24), QTime(16, 0));
    QVERIFY(m_interviews->insert(iv3));

    // Query from `now`, limit = 2
    const auto res = m_service->upcomingInterviews(now, 2);
    QCOMPARE(res.size(), 2);
    QCOMPARE(res[0].refId, iv1.id);
    QCOMPARE(res[1].refId, iv2.id);

    // Query from `now`, limit = 5
    const auto resAll = m_service->upcomingInterviews(now, 5);
    QCOMPARE(resAll.size(), 3);
    QCOMPARE(resAll[0].refId, iv1.id);
    QCOMPARE(resAll[1].refId, iv2.id);
    QCOMPARE(resAll[2].refId, iv3.id);
}

void tst_AgendaService::upcomingInterviewsAcrossMonthAndYearBorder() {
    const int appId = addApplication(u"BorderCo"_s);

    Interview ivDec;
    ivDec.applicationId = appId;
    ivDec.startAt = QDateTime(QDate(2026, 12, 31), QTime(20, 0));
    QVERIFY(m_interviews->insert(ivDec));

    Interview ivJan;
    ivJan.applicationId = appId;
    ivJan.startAt = QDateTime(QDate(2027, 1, 1), QTime(9, 0));
    QVERIFY(m_interviews->insert(ivJan));

    const auto res = m_service->upcomingInterviews(QDateTime(QDate(2026, 12, 31), QTime(0, 0)), 5);
    QCOMPARE(res.size(), 2);
    QCOMPARE(res[0].date, QDate(2026, 12, 31));
    QCOMPARE(res[1].date, QDate(2027, 1, 1));
}

QTEST_GUILESS_MAIN(tst_AgendaService)
#include "tst_AgendaService.moc"
