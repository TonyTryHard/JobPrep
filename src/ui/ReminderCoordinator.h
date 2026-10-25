#pragma once

#include <functional>
#include <memory>
#include <QDateTime>
#include <QObject>
#include "services/INotifier.h"
#include "services/ReminderService.h"

class QTimer;

namespace JobPrep::Data {
class ApplicationRepository;
class InterviewRepository;
class ReminderLogRepository;
}  // namespace JobPrep::Data

namespace JobPrep::Services {
class SettingsService;
}

namespace JobPrep::Ui {
class MainWindow;

/// Coordinates ReminderService and INotifier lifecycle with MainWindow and SettingsService (SPEC §8, Amendment D).
class ReminderCoordinator : public QObject {
    Q_OBJECT

public:
    ReminderCoordinator(Data::InterviewRepository& interviews,
                        Data::ApplicationRepository& apps,
                        Data::ReminderLogRepository& reminderLog,
                        Services::SettingsService& settings,
                        MainWindow& window,
                        bool trayAvailable,
                        std::function<QDateTime()> clock = {},
                        QObject* parent = nullptr);
    ~ReminderCoordinator() override;

    void start(int intervalMs = 60000);
    void stop();
    void checkNow(bool startupPass = false);

    Services::ReminderService& service() { return *m_service; }
    Services::INotifier& notifier() { return *m_notifier; }

private:
    // IMPORTANT: Members are destroyed in reverse order of declaration.
    // m_service must be destroyed before m_notifier because it holds a reference to m_notifier.
    std::unique_ptr<Services::INotifier> m_notifier;
    std::unique_ptr<Services::ReminderService> m_service;

    Services::SettingsService& m_settings;
    QTimer* m_timer{nullptr};
};

}  // namespace JobPrep::Ui
