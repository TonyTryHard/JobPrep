#include "services/StatsService.h"

#include <algorithm>
#include <QHash>
#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"
#include "data/SessionRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/TopicProgress.h"

namespace JobPrep::Services {

StatsService::StatsService(Data::ApplicationRepository& apps,
                           Data::InterviewRepository& interviews,
                           Data::SessionRepository& sessions,
                           Data::TopicRepository& topics,
                           Data::TrackRepository& tracks,
                           Clock clock)
    : m_apps(apps),
      m_interviews(interviews),
      m_sessions(sessions),
      m_topics(topics),
      m_tracks(tracks),
      m_clock(clock ? std::move(clock) : Clock([] { return QDateTime::currentDateTime(); })) {}

void StatsService::setClock(Clock clock) {
    m_clock = clock ? std::move(clock) : Clock([] { return QDateTime::currentDateTime(); });
}

void StatsService::setDateClock(std::function<QDate()> dateClock) {
    if (dateClock) {
        m_clock = [dateClock] { return QDateTime(dateClock(), QTime(0, 0, 0)); };
    } else {
        m_clock = [] { return QDateTime::currentDateTime(); };
    }
}

std::pair<QDate, QDate> StatsService::weekBounds(QDate date, Qt::DayOfWeek firstDayOfWeek) {
    if (!date.isValid()) return {QDate(), QDate()};
    const int day = date.dayOfWeek();
    const int startDay = static_cast<int>(firstDayOfWeek);
    const int diff = (day - startDay + 7) % 7;
    const QDate weekStart = date.addDays(-diff);
    const QDate weekEnd = weekStart.addDays(6);
    return {weekStart, weekEnd};
}

DashboardStats StatsService::computeStats(int weeklyGoalMinutes, Qt::DayOfWeek firstDayOfWeek) const {
    DashboardStats stats;

    const QDateTime now = m_clock();
    const QDate today = now.date();

    // 1. Week date range
    const auto [weekStart, weekEnd] = weekBounds(today, firstDayOfWeek);

    // 2. Applied this week (inclusive)
    stats.appliedThisWeek = m_apps.appliedCountInRange(weekStart, weekEnd);

    // 3. Active processes: applied, hr_screen, technical, final, offer (SPEC §4)
    const auto statusCounts = m_apps.countsByStatus();
    stats.activeProcesses = statusCounts.value(Domain::ApplicationStatus::Applied, 0) +
                            statusCounts.value(Domain::ApplicationStatus::HrScreen, 0) +
                            statusCounts.value(Domain::ApplicationStatus::Technical, 0) +
                            statusCounts.value(Domain::ApplicationStatus::Final, 0) +
                            statusCounts.value(Domain::ApplicationStatus::Offer, 0);

    // 4. Interviews in next 7 days [now, today + 6 days 23:59:59] (amendment A6)
    const QDateTime windowEnd(today.addDays(6), QTime(23, 59, 59));
    stats.interviewsNext7Days = m_interviews.countUpcoming(now, windowEnd);

    // 5. Study streak (days)
    const auto dates = m_sessions.distinctSessionDates(today);
    if (!dates.isEmpty()) {
        QDate targetDay = (dates.first() == today) ? today : today.addDays(-1);
        for (const auto& d : dates) {
            if (d > targetDay) continue;
            if (d == targetDay) {
                ++stats.studyStreakDays;
                targetDay = targetDay.addDays(-1);
            } else {
                break;
            }
        }
    }

    // 6. Weekly minutes and goal progress
    stats.weekMinutes = m_sessions.totalMinutesInRange(weekStart, weekEnd);
    stats.goalMinutes = weeklyGoalMinutes;
    if (weeklyGoalMinutes > 0) {
        stats.goalPercentReal = (stats.weekMinutes * 100) / weeklyGoalMinutes;
        stats.goalPercentCapped = std::clamp(stats.goalPercentReal, 0, 100);
    } else {
        stats.goalPercentReal = 0;
        stats.goalPercentCapped = 0;
    }

    // 7. Hiring funnel & interview rate
    const auto stageCounts = m_apps.funnelStageCounts();
    const int countRank1 = stageCounts.value(1, 0);
    const int countRank2 = stageCounts.value(2, 0);
    const int countRank3 = stageCounts.value(3, 0);
    const int countRank4 = stageCounts.value(4, 0);
    const int countRank5 = stageCounts.value(5, 0);

    stats.funnel.append({Domain::ApplicationStatus::Applied, countRank1});
    stats.funnel.append({Domain::ApplicationStatus::HrScreen, countRank2});
    stats.funnel.append({Domain::ApplicationStatus::Technical, countRank3});
    stats.funnel.append({Domain::ApplicationStatus::Final, countRank4});
    stats.funnel.append({Domain::ApplicationStatus::Offer, countRank5});

    if (countRank1 > 0) {
        stats.interviewRate = static_cast<double>(countRank2) / static_cast<double>(countRank1);
    } else {
        stats.interviewRate = std::nullopt;
    }

    // 8. Track progress (fixed 3 queries: tracks, topics, subtaskProgress; SPEC §4, amendment A7)
    const auto tracks = m_tracks.all();
    const auto topics = m_topics.all();
    const auto subtaskProgressMap = m_topics.subtaskProgress();

    QHash<int, QList<Domain::Topic>> topicsByTrack;
    for (const auto& topic : topics) {
        topicsByTrack[topic.trackId].append(topic);
    }

    for (const auto& track : tracks) {
        TrackProgressStat stat;
        stat.trackId = track.id;
        stat.trackName = track.name;
        stat.colorHex = track.color;

        const auto& trackTopics = topicsByTrack.value(track.id);
        if (trackTopics.isEmpty()) {
            stat.percent = 0;
        } else {
            int sum = 0;
            for (const auto& topic : trackTopics) {
                const auto sub = subtaskProgressMap.value(topic.id);
                sum += Domain::topicProgress(topic.status, sub.done, sub.total);
            }
            stat.percent = sum / static_cast<int>(trackTopics.size());
        }
        stats.trackProgress.append(stat);
    }

    return stats;
}

}  // namespace JobPrep::Services
