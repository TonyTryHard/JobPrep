#pragma once

#include <functional>
#include <optional>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringView>
#include "domain/Structs.h"
#include "services/INotifier.h"

class QTimer;

namespace JobPrep::Data {
class ApplicationRepository;
class InterviewRepository;
class ReminderLogRepository;
}  // namespace JobPrep::Data

namespace JobPrep::Services {

/// Interview candidate bundled with parent application metadata for notifications.
struct CandidateInterview {
    Domain::Interview interview;
    QString company;
    QString position;
};

/// A single due reminder notification and the keys to record upon successful delivery.
struct DueReminder {
    Notification notification;
    QList<QString> keysToRecord;
};

/// Plan of notifications to deliver and keys to record.
struct ReminderPlan {
    QList<DueReminder> notifications;
    QList<QString> silentKeysToRecord;
};

/// Service coordinating interview lead, today's follow-up, and overdue reminders (SPEC §3.5, §8, Amendments C1-C7).
class ReminderService : public QObject {
    Q_OBJECT

public:
    using Clock = std::function<QDateTime()>;
    using SentAtLookup = std::function<std::optional<QDateTime>(QStringView key)>;

    ReminderService(Data::InterviewRepository& interviews,
                    Data::ApplicationRepository& apps,
                    Data::ReminderLogRepository& reminderLog,
                    INotifier& notifier,
                    QObject* parent = nullptr);
    ~ReminderService() override;

    /// Sets the interview lead minutes list (positive, unique, sorted descending).
    void setLeadMinutes(const QList<int>& leadMinutes);
    QList<int> leadMinutes() const { return m_leadMinutes; }

    /// Test seam: injects a custom clock.
    void setClock(Clock clock);

    /// Starts the periodic 60-second timer and executes the startup pass.
    void start();

    /// Pauses reminders evaluation (e.g. during a database restore).
    void pause();
    /// Resumes reminders evaluation.
    void resume();
    bool isPaused() const { return m_paused; }

    /// Executes an immediate evaluation pass.
    /// When `startupPass` is true, overdue follow-up checks are included.
    void checkNow(bool startupPass = false);

    /// Pure evaluation function separating business rules from database I/O.
    static ReminderPlan computePlan(const QDateTime& now,
                                    bool startupPass,
                                    const QList<int>& leadMinutes,
                                    const QList<CandidateInterview>& interviews,
                                    const QList<Domain::JobApplication>& followUpApps,
                                    const QList<Domain::JobApplication>& overdueApps,
                                    const SentAtLookup& sentAtLookup);

private:
    Data::InterviewRepository& m_interviews;
    Data::ApplicationRepository& m_apps;
    Data::ReminderLogRepository& m_reminderLog;
    INotifier& m_notifier;

    Clock m_clock;
    QList<int> m_leadMinutes{1440, 60};
    QTimer* m_timer{nullptr};
    bool m_paused{false};
};

}  // namespace JobPrep::Services
