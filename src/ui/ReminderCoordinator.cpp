#include "ui/ReminderCoordinator.h"

#include <QTimer>
#include "services/SettingsService.h"
#include "ui/MainWindow.h"
#include "ui/NotifierFactory.h"

namespace JobPrep::Ui {

ReminderCoordinator::ReminderCoordinator(
    Data::InterviewRepository& interviews,
    Data::ApplicationRepository& apps,
    Data::ReminderLogRepository& reminderLog,
    Services::SettingsService& settings,
    MainWindow& window,
    bool trayAvailable,
    std::function<QDateTime()> clock,
    QObject* parent)
    : QObject(parent), m_settings(settings) {
    m_notifier = makeNotifier(trayAvailable, window, [&window](const Services::Notification& n) {
        window.openItemFromNotification(n);
    });

    m_service = std::make_unique<Services::ReminderService>(
        interviews, apps, reminderLog, *m_notifier
    );

    if (clock) {
        m_service->setClock(clock);
    }

    m_service->setLeadMinutes(m_settings.interviewLeadMinutes());

    connect(&m_settings, &Services::SettingsService::interviewLeadMinutesChanged,
            this, [this](const QList<int>& leads) {
        m_service->setLeadMinutes(leads);
    });
}

ReminderCoordinator::~ReminderCoordinator() {
    stop();
}

void ReminderCoordinator::start(int intervalMs) {
    checkNow(true);

    if (!m_timer) {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, [this]() {
            checkNow(false);
        });
    }
    m_timer->start(intervalMs);
}

void ReminderCoordinator::stop() {
    if (m_timer) {
        m_timer->stop();
    }
}

void ReminderCoordinator::checkNow(bool startupPass) {
    if (m_service) {
        m_service->checkNow(startupPass);
    }
}

}  // namespace JobPrep::Ui
