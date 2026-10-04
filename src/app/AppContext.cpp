#include "app/AppContext.h"

#include <QDir>
#include <QStandardPaths>
#include "data/ApplicationRepository.h"
#include "data/Database.h"
#include "data/InterviewRepository.h"
#include "data/ReminderLogRepository.h"
#include "data/SessionRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "services/ExportService.h"
#include "services/SeedService.h"
#include "services/SettingsService.h"
#include "ui/theme/ThemeManager.h"

using namespace Qt::StringLiterals;

// Q_INIT_RESOURCE mangles the generated symbol with the enclosing namespace and
// declares it with the enclosing linkage, while AUTORCC defines a global symbol,
// so this helper must sit at global scope with external linkage.
void initializeResources() {
    Q_INIT_RESOURCE(resources);
}

namespace JobPrep::App {

namespace {

/// `QStandardPaths::AppDataLocation` + `/jobprep.sqlite` (SPEC §8, AGENTS §12).
QString defaultDatabasePath() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath(u"jobprep.sqlite"_s);
}

}  // namespace

AppContext::AppContext(const QString& databasePath) {
    initializeResources();
    m_database = std::make_unique<Data::Database>();
    m_database->open(databasePath.isEmpty() ? defaultDatabasePath() : databasePath);

    m_tracks = std::make_unique<Data::TrackRepository>(*m_database);
    m_topics = std::make_unique<Data::TopicRepository>(*m_database);
    m_sessions = std::make_unique<Data::SessionRepository>(*m_database);
    m_applications = std::make_unique<Data::ApplicationRepository>(*m_database);
    m_interviews = std::make_unique<Data::InterviewRepository>(*m_database);
    m_reminderLog = std::make_unique<Data::ReminderLogRepository>(*m_database);

    m_seedService = std::make_unique<Services::SeedService>(*m_tracks, *m_topics);
    m_exportService = std::make_unique<Services::ExportService>();
    m_settings = std::make_unique<Services::SettingsService>();
    m_themeManager = std::make_unique<Ui::Theme::ThemeManager>(*m_settings);
}

AppContext::~AppContext() = default;

Data::Database& AppContext::database() const {
    return *m_database;
}

Data::TrackRepository& AppContext::tracks() const {
    return *m_tracks;
}

Data::TopicRepository& AppContext::topics() const {
    return *m_topics;
}

Data::SessionRepository& AppContext::sessions() const {
    return *m_sessions;
}

Data::ApplicationRepository& AppContext::applications() const {
    return *m_applications;
}

Data::InterviewRepository& AppContext::interviews() const {
    return *m_interviews;
}

Data::ReminderLogRepository& AppContext::reminderLog() const {
    return *m_reminderLog;
}

Services::ExportService& AppContext::exportService() const {
    return *m_exportService;
}

Services::SeedService& AppContext::seedService() const {
    return *m_seedService;
}

Services::SettingsService& AppContext::settings() const {
    return *m_settings;
}

Ui::Theme::ThemeManager& AppContext::themeManager() const {
    return *m_themeManager;
}

bool AppContext::isDataEmpty() const {
    return m_tracks->isEmpty() && m_topics->isEmpty() && m_applications->isEmpty();
}

}  // namespace JobPrep::App
