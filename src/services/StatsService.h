#pragma once

#include <functional>
#include <optional>
#include <utility>
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QString>
#include "domain/Enums.h"

namespace JobPrep::Data {
class ApplicationRepository;
class InterviewRepository;
class SessionRepository;
class TopicRepository;
class TrackRepository;
}  // namespace JobPrep::Data

namespace JobPrep::Services {

/// Reached count for a single hiring funnel stage rank (1..5).
struct FunnelStage {
    Domain::ApplicationStatus status{Domain::ApplicationStatus::Applied};
    int reachedCount{0};
};

/// Average study completion percentage for a track.
struct TrackProgressStat {
    int trackId{0};
    QString trackName;
    QString colorHex;
    int percent{0};
};

/// Numeric stats computed for the Dashboard (SPEC §6, amendment A1).
/// Holds numeric and domain types only — no presentation strings or UI formatting.
struct DashboardStats {
    int appliedThisWeek{0};
    int activeProcesses{0};
    int interviewsNext7Days{0};
    int studyStreakDays{0};
    int weekMinutes{0};
    int goalMinutes{0};
    int goalPercentReal{0};
    int goalPercentCapped{0};
    QList<FunnelStage> funnel;
    std::optional<double> interviewRate;
    QList<TrackProgressStat> trackProgress;
};

/// Computes aggregated metrics for the dashboard without reading settings.
class StatsService {
public:
    using Clock = std::function<QDateTime()>;

    explicit StatsService(Data::ApplicationRepository& apps,
                          Data::InterviewRepository& interviews,
                          Data::SessionRepository& sessions,
                          Data::TopicRepository& topics,
                          Data::TrackRepository& tracks,
                          Clock clock = [] { return QDateTime::currentDateTime(); });

    void setClock(Clock clock);
    void setDateClock(std::function<QDate()> dateClock);

    /// Pure week-bounds helper returning [weekStart, weekEnd] inclusive (amendment A7).
    static std::pair<QDate, QDate> weekBounds(QDate date, Qt::DayOfWeek firstDayOfWeek);

    /// Computes full dashboard statistics for the specified weekly goal and week start (SPEC §6).
    DashboardStats computeStats(int weeklyGoalMinutes, Qt::DayOfWeek firstDayOfWeek) const;

private:
    Data::ApplicationRepository& m_apps;
    Data::InterviewRepository& m_interviews;
    Data::SessionRepository& m_sessions;
    Data::TopicRepository& m_topics;
    Data::TrackRepository& m_tracks;
    Clock m_clock;
};

}  // namespace JobPrep::Services
