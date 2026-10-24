#include "ui/DashboardController.h"

#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"
#include "data/SessionRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "services/AgendaService.h"
#include "services/DayWatcher.h"
#include "services/SettingsService.h"
#include "services/StatsService.h"

namespace JobPrep::Ui {

DashboardController::DashboardController(JobPrep::Services::StatsService& statsService,
                                         JobPrep::Services::AgendaService& agendaService,
                                         JobPrep::Services::SettingsService& settingsService,
                                         JobPrep::Services::DayWatcher& dayWatcher,
                                         JobPrep::Data::ApplicationRepository& apps,
                                         JobPrep::Data::InterviewRepository& interviews,
                                         JobPrep::Data::SessionRepository& sessions,
                                         JobPrep::Data::TopicRepository& topics,
                                         JobPrep::Data::TrackRepository& tracks,
                                         QObject* parent)
    : QObject(parent),
      m_statsService(statsService),
      m_agendaService(agendaService),
      m_settings(settingsService),
      m_dayWatcher(dayWatcher),
      m_apps(apps),
      m_interviews(interviews),
      m_sessions(sessions),
      m_topics(topics),
      m_tracks(tracks) {
    m_coalesceTimer = new QTimer(this);
    m_coalesceTimer->setSingleShot(true);
    m_coalesceTimer->setInterval(0);
    connect(m_coalesceTimer, &QTimer::timeout, this, &DashboardController::refreshNow);

    wireSignals();
    refreshNow();
}

void DashboardController::wireSignals() {
    connect(&m_apps, &JobPrep::Data::ApplicationRepository::changed, this,
            &DashboardController::requestRefresh);
    connect(&m_interviews, &JobPrep::Data::InterviewRepository::changed, this,
            &DashboardController::requestRefresh);
    connect(&m_sessions, &JobPrep::Data::SessionRepository::changed, this,
            &DashboardController::requestRefresh);
    connect(&m_topics, &JobPrep::Data::TopicRepository::changed, this,
            &DashboardController::requestRefresh);
    connect(&m_tracks, &JobPrep::Data::TrackRepository::changed, this,
            &DashboardController::requestRefresh);

    connect(&m_settings, &JobPrep::Services::SettingsService::weeklyGoalMinutesChanged, this,
            &DashboardController::requestRefresh);
    connect(&m_settings, &JobPrep::Services::SettingsService::firstDayOfWeekChanged, this,
            &DashboardController::requestRefresh);

    connect(&m_dayWatcher, &JobPrep::Services::DayWatcher::dayChanged, this,
            &DashboardController::requestRefresh);
}

void DashboardController::requestRefresh() {
    if (!m_coalesceTimer->isActive()) {
        m_coalesceTimer->start();
    }
}

void DashboardController::refreshNow() {
    m_coalesceTimer->stop();
    m_refreshCount++;

    const int goalMinutes = m_settings.weeklyGoalMinutes();
    const Qt::DayOfWeek firstDay =
        (m_settings.firstDayOfWeek() == 7) ? Qt::Sunday : Qt::Monday;

    m_snapshot.weeklyGoalMinutes = goalMinutes;
    m_snapshot.isDataEmpty = m_apps.isEmpty() && m_topics.isEmpty();
    m_snapshot.stats = m_statsService.computeStats(goalMinutes, firstDay);
    m_snapshot.today = m_dayWatcher.currentDate();
    m_snapshot.todayItems = m_agendaService.todayItems(m_snapshot.today);

    const QDateTime now = QDateTime(m_snapshot.today, QTime::currentTime());
    m_snapshot.upcomingInterviews = m_agendaService.upcomingInterviews(now, 5);

    if (!m_snapshot.upcomingInterviews.isEmpty()) {
        m_snapshot.nextInterview = m_snapshot.upcomingInterviews.first();
    } else {
        m_snapshot.nextInterview = std::nullopt;
    }

    emit snapshotChanged(m_snapshot);
}

}  // namespace JobPrep::Ui
