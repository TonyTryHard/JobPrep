#pragma once

#include <memory>
#include <QString>

namespace JobPrep::Data {
class ApplicationRepository;
class Database;
class InterviewRepository;
class ReminderLogRepository;
class SessionRepository;
class TopicRepository;
class TrackRepository;
}  // namespace JobPrep::Data

namespace JobPrep::Services {
class SeedService;
class SettingsService;
}  // namespace JobPrep::Services

namespace JobPrep::Ui::Theme {
class ThemeManager;
}  // namespace JobPrep::Ui::Theme

namespace JobPrep::App {

/// Composition root container holding the database, repositories, services and the
/// theme manager. `databasePath` defaults to the standard app-data folder; tests
/// pass `:memory:`.
class AppContext {
public:
    explicit AppContext(const QString& databasePath = QString());
    ~AppContext();

    AppContext(const AppContext&) = delete;
    AppContext& operator=(const AppContext&) = delete;
    AppContext(AppContext&&) = delete;
    AppContext& operator=(AppContext&&) = delete;

    Data::Database& database() const;
    Data::TrackRepository& tracks() const;
    Data::TopicRepository& topics() const;
    Data::SessionRepository& sessions() const;
    Data::ApplicationRepository& applications() const;
    Data::InterviewRepository& interviews() const;
    Data::ReminderLogRepository& reminderLog() const;

    Services::SeedService& seedService() const;
    Services::SettingsService& settings() const;
    Ui::Theme::ThemeManager& themeManager() const;

    /// True while the user has no tracks, topics and no applications.
    bool isDataEmpty() const;

private:
    // Declared first so the repositories and services die before the connection.
    std::unique_ptr<Data::Database> m_database;
    std::unique_ptr<Data::TrackRepository> m_tracks;
    std::unique_ptr<Data::TopicRepository> m_topics;
    std::unique_ptr<Data::SessionRepository> m_sessions;
    std::unique_ptr<Data::ApplicationRepository> m_applications;
    std::unique_ptr<Data::InterviewRepository> m_interviews;
    std::unique_ptr<Data::ReminderLogRepository> m_reminderLog;
    std::unique_ptr<Services::SeedService> m_seedService;
    std::unique_ptr<Services::SettingsService> m_settings;
    std::unique_ptr<Ui::Theme::ThemeManager> m_themeManager;
};

}  // namespace JobPrep::App
