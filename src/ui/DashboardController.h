#pragma once

#include <optional>
#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>
#include "domain/Structs.h"
#include "services/StatsService.h"

namespace JobPrep::Data {
class ApplicationRepository;
class InterviewRepository;
class SessionRepository;
class TopicRepository;
class TrackRepository;
}  // namespace JobPrep::Data

namespace JobPrep::Services {
class AgendaService;
class DayWatcher;
class SettingsService;
}  // namespace JobPrep::Services

namespace JobPrep::Ui {

/// Snapshot of aggregated dashboard data produced by a single refresh cycle.
struct DashboardSnapshot {
    Services::DashboardStats stats;
    QList<Domain::AgendaItem> todayItems;
    QList<Domain::AgendaItem> upcomingInterviews;
    QDate today;
    int weeklyGoalMinutes{360};
    std::optional<Domain::AgendaItem> nextInterview;
    bool isDataEmpty{false};
};

/// Coalescing controller for dashboard data (SPEC §6, §7.2, Amendment B3).
/// Aggregates repository and settings changes into a single snapshot computation
/// per event loop turn, feeding both HomePage and MainWindow's status bar.
class DashboardController : public QObject {
    Q_OBJECT

public:
    DashboardController(JobPrep::Services::StatsService& statsService,
                        JobPrep::Services::AgendaService& agendaService,
                        JobPrep::Services::SettingsService& settingsService,
                        JobPrep::Services::DayWatcher& dayWatcher,
                        JobPrep::Data::ApplicationRepository& apps,
                        JobPrep::Data::InterviewRepository& interviews,
                        JobPrep::Data::SessionRepository& sessions,
                        JobPrep::Data::TopicRepository& topics,
                        JobPrep::Data::TrackRepository& tracks,
                        QObject* parent = nullptr);
    ~DashboardController() override = default;

    /// The latest computed snapshot.
    const DashboardSnapshot& snapshot() const { return m_snapshot; }

    /// Test seam: number of full snapshot refreshes executed.
    int refreshCount() const { return m_refreshCount; }

    /// Stops any pending coalesce refresh timer.
    void stop();

public slots:
    /// Schedules a coalesced refresh on the next event loop turn (0 ms timer).
    void requestRefresh();

    /// Forces immediate synchronous snapshot computation.
    void refreshNow();

signals:
    void snapshotChanged(const JobPrep::Ui::DashboardSnapshot& snapshot);

private:
    void wireSignals();

    JobPrep::Services::StatsService& m_statsService;
    JobPrep::Services::AgendaService& m_agendaService;
    JobPrep::Services::SettingsService& m_settings;
    JobPrep::Services::DayWatcher& m_dayWatcher;
    JobPrep::Data::ApplicationRepository& m_apps;
    JobPrep::Data::InterviewRepository& m_interviews;
    JobPrep::Data::SessionRepository& m_sessions;
    JobPrep::Data::TopicRepository& m_topics;
    JobPrep::Data::TrackRepository& m_tracks;

    QTimer* m_coalesceTimer{nullptr};
    DashboardSnapshot m_snapshot;
    int m_refreshCount{0};
};

}  // namespace JobPrep::Ui
