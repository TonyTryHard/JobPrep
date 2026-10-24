#include "services/ReminderService.h"

#include <algorithm>
#include <QTime>
#include <QTimer>
#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"
#include "data/ReminderLogRepository.h"
#include "domain/Enums.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Services {

namespace {

bool isClosed(Domain::ApplicationStatus status) {
    return status == Domain::ApplicationStatus::Accepted ||
           status == Domain::ApplicationStatus::Rejected ||
           status == Domain::ApplicationStatus::Withdrawn;
}

QList<int> sanitizeLeads(const QList<int>& raw) {
    QList<int> valid;
    for (int v : raw) {
        if (v > 0 && !valid.contains(v)) {
            valid.append(v);
        }
    }
    std::sort(valid.begin(), valid.end(), std::greater<int>());
    if (valid.isEmpty()) {
        return {1440, 60};
    }
    return valid;
}

}  // namespace

ReminderService::ReminderService(Data::InterviewRepository& interviews,
                                 Data::ApplicationRepository& apps,
                                 Data::ReminderLogRepository& reminderLog,
                                 INotifier& notifier,
                                 QObject* parent)
    : QObject(parent),
      m_interviews(interviews),
      m_apps(apps),
      m_reminderLog(reminderLog),
      m_notifier(notifier) {
    m_timer = new QTimer(this);
    m_timer->setInterval(60000);
    connect(m_timer, &QTimer::timeout, this, [this] { checkNow(false); });
}

ReminderService::~ReminderService() = default;

void ReminderService::setLeadMinutes(const QList<int>& leadMinutes) {
    m_leadMinutes = sanitizeLeads(leadMinutes);
}

void ReminderService::setClock(Clock clock) {
    m_clock = std::move(clock);
}

void ReminderService::start() {
    checkNow(true);
    if (!m_timer->isActive()) {
        m_timer->start();
    }
}

void ReminderService::checkNow(bool startupPass) {
    const QDateTime now = m_clock ? m_clock() : QDateTime::currentDateTime();

    // 1. Collect upcoming interviews up to the maximum lead window
    const int maxLead = m_leadMinutes.isEmpty() ? 1440 : m_leadMinutes.first();
    const auto upcomingList = m_interviews.upcoming(now, 1000);
    QList<CandidateInterview> candidates;
    for (const auto& iv : upcomingList) {
        if (iv.startAt <= now.addSecs(maxLead * 60)) {
            const auto app = m_apps.byId(iv.applicationId);
            candidates.append({iv, app ? app->company : QString(), app ? app->position : QString()});
        }
    }

    // 2. Collect follow-ups and overdue applications
    const auto dueApps = m_apps.byNextActionDateUpTo(now.date());
    QList<Domain::JobApplication> todayFollowUps;
    QList<Domain::JobApplication> overdueFollowUps;
    for (const auto& app : dueApps) {
        if (!app.nextActionDate.has_value()) continue;
        if (*app.nextActionDate == now.date()) {
            todayFollowUps.append(app);
        } else if (*app.nextActionDate < now.date()) {
            overdueFollowUps.append(app);
        }
    }

    // 3. Sent at lookup predicate
    auto lookup = [this](QStringView key) {
        return m_reminderLog.sentAt(key);
    };

    // 4. Compute plan via pure function
    const auto plan = computePlan(now, startupPass, m_leadMinutes, candidates,
                                  todayFollowUps, overdueFollowUps, lookup);

    // 5. Record silent keys (e.g. larger due lead added after smaller was delivered)
    for (const auto& key : plan.silentKeysToRecord) {
        if (m_reminderLog.wasSent(key)) {
            m_reminderLog.remove(key);
        }
        m_reminderLog.markSent(key, now);
    }

    // 6. Deliver notifications
    for (const auto& due : plan.notifications) {
        if (m_notifier.notify(due.notification)) {
            for (const auto& key : due.keysToRecord) {
                if (m_reminderLog.wasSent(key)) {
                    m_reminderLog.remove(key);
                }
                m_reminderLog.markSent(key, now);
            }
        }
    }
}

ReminderPlan ReminderService::computePlan(const QDateTime& now,
                                          bool startupPass,
                                          const QList<int>& leadMinutes,
                                          const QList<CandidateInterview>& interviews,
                                          const QList<Domain::JobApplication>& followUpApps,
                                          const QList<Domain::JobApplication>& overdueApps,
                                          const SentAtLookup& sentAtLookup) {
    ReminderPlan plan;
    const auto sanitizedLeads = sanitizeLeads(leadMinutes);

    // 1. Interviews
    for (const auto& ci : interviews) {
        if (ci.interview.outcome == Domain::InterviewOutcome::Cancelled) continue;
        if (now >= ci.interview.startAt) continue;

        QList<int> deliveredLeads;
        QList<int> dueUnsentLeads;

        for (int lead : sanitizedLeads) {
            const QDateTime dueWindowStart = ci.interview.startAt.addSecs(-lead * 60);
            if (now >= dueWindowStart && now < ci.interview.startAt) {
                const QString key = u"interview:%1:%2"_s.arg(ci.interview.id).arg(lead);
                const auto sentAt = sentAtLookup(key);
                const bool validSent = sentAt.has_value() && (*sentAt >= dueWindowStart);
                if (validSent) {
                    deliveredLeads.append(lead);
                } else {
                    dueUnsentLeads.append(lead);
                }
            }
        }

        if (dueUnsentLeads.isEmpty()) continue;

        const int minDelivered = deliveredLeads.isEmpty()
                                     ? -1
                                     : *std::min_element(deliveredLeads.begin(), deliveredLeads.end());

        QList<int> toNotify;
        for (int lead : dueUnsentLeads) {
            if (minDelivered != -1 && lead > minDelivered) {
                plan.silentKeysToRecord.append(u"interview:%1:%2"_s.arg(ci.interview.id).arg(lead));
            } else {
                toNotify.append(lead);
            }
        }

        if (!toNotify.isEmpty()) {
            const int smallestLead = *std::min_element(toNotify.begin(), toNotify.end());
            Notification n;
            n.kind = NotificationKind::Interview;
            n.refId = ci.interview.id;
            n.company = ci.company;
            n.position = ci.position;
            n.interviewType = ci.interview.type;
            n.when = ci.interview.startAt;
            n.leadMinutes = smallestLead;
            n.count = 1;

            DueReminder due;
            due.notification = n;
            for (int lead : toNotify) {
                due.keysToRecord.append(u"interview:%1:%2"_s.arg(ci.interview.id).arg(lead));
            }
            plan.notifications.append(due);
        }
    }

    // 2. Today follow-ups (due at or after 09:00 for non-closed applications)
    if (now.time() >= QTime(9, 0)) {
        for (const auto& app : followUpApps) {
            if (isClosed(app.status)) continue;
            if (!app.nextActionDate.has_value() || *app.nextActionDate != now.date()) continue;

            const QString key = u"followup:%1:%2"_s.arg(app.id).arg(now.date().toString(Qt::ISODate));
            if (sentAtLookup(key).has_value()) continue;

            Notification n;
            n.kind = NotificationKind::FollowUp;
            n.refId = app.id;
            n.company = app.company;
            n.position = app.position;
            n.interviewType = std::nullopt;
            n.when = QDateTime(now.date(), QTime(9, 0));
            n.leadMinutes = 0;
            n.count = 1;

            DueReminder due;
            due.notification = n;
            due.keysToRecord.append(key);
            plan.notifications.append(due);
        }
    }

    // 3. Overdue follow-ups (startup pass only for non-closed applications)
    if (startupPass) {
        QList<Domain::JobApplication> unsentOverdue;
        for (const auto& app : overdueApps) {
            if (isClosed(app.status)) continue;
            if (!app.nextActionDate.has_value() || *app.nextActionDate >= now.date()) continue;

            const QString key = u"overdue:%1:%2"_s.arg(app.id).arg(now.date().toString(Qt::ISODate));
            if (sentAtLookup(key).has_value()) continue;

            unsentOverdue.append(app);
        }

        if (unsentOverdue.size() == 1) {
            const auto& app = unsentOverdue.first();
            Notification n;
            n.kind = NotificationKind::OverdueFollowUps;
            n.refId = app.id;
            n.company = app.company;
            n.position = app.position;
            n.interviewType = std::nullopt;
            n.when = now;
            n.leadMinutes = 0;
            n.count = 1;

            DueReminder due;
            due.notification = n;
            due.keysToRecord.append(u"overdue:%1:%2"_s.arg(app.id).arg(now.date().toString(Qt::ISODate)));
            plan.notifications.append(due);
        } else if (unsentOverdue.size() > 1) {
            Notification n;
            n.kind = NotificationKind::OverdueFollowUps;
            n.refId = 0;
            n.company = QString();
            n.position = QString();
            n.interviewType = std::nullopt;
            n.when = now;
            n.leadMinutes = 0;
            n.count = static_cast<int>(unsentOverdue.size());

            DueReminder due;
            due.notification = n;
            for (const auto& app : unsentOverdue) {
                due.keysToRecord.append(
                    u"overdue:%1:%2"_s.arg(app.id).arg(now.date().toString(Qt::ISODate)));
            }
            plan.notifications.append(due);
        }
    }

    return plan;
}

}  // namespace JobPrep::Services
