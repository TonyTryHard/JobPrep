#include "ui/NotificationText.h"

#include <QCoreApplication>
#include "ui/theme/StatusStyle.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui {

QString NotificationText::formatLeadTime(int leadMinutes) {
    if (leadMinutes <= 0) return QString();
    if (leadMinutes % 60 == 0) {
        const int hours = leadMinutes / 60;
        if (hours == 1) {
            return QCoreApplication::translate("NotificationText", "in 1 hour");
        }
        return QCoreApplication::translate("NotificationText", "in %1 hours").arg(hours);
    }
    if (leadMinutes == 1) {
        return QCoreApplication::translate("NotificationText", "in 1 minute");
    }
    return QCoreApplication::translate("NotificationText", "in %1 minutes").arg(leadMinutes);
}

QString NotificationText::formatTitle(const Services::Notification& notification) {
    switch (notification.kind) {
        case Services::NotificationKind::Interview:
            if (!notification.company.isEmpty()) {
                return notification.company;
            }
            return QCoreApplication::translate("NotificationText", "Interview Reminder");
        case Services::NotificationKind::FollowUp:
            if (!notification.company.isEmpty()) {
                return notification.company;
            }
            return QCoreApplication::translate("NotificationText", "Follow-up Reminder");
        case Services::NotificationKind::OverdueFollowUps:
            if (notification.refId > 0 && notification.count <= 1) {
                return QCoreApplication::translate("NotificationText", "Overdue Follow-up");
            }
            return QCoreApplication::translate("NotificationText", "Overdue Follow-ups");
    }
    return QString();
}

QString NotificationText::formatMessage(const Services::Notification& notification) {
    switch (notification.kind) {
        case Services::NotificationKind::Interview: {
            const QString typeLabel = notification.interviewType
                                          ? Theme::StatusStyle::label(*notification.interviewType)
                                          : QCoreApplication::translate("NotificationText", "Interview");
            const QString leadStr = formatLeadTime(notification.leadMinutes);
            if (!notification.position.isEmpty() && !leadStr.isEmpty()) {
                return QCoreApplication::translate("NotificationText", "%1 (%2) %3")
                    .arg(typeLabel, notification.position, leadStr);
            } else if (!leadStr.isEmpty()) {
                return QCoreApplication::translate("NotificationText", "%1 %2")
                    .arg(typeLabel, leadStr);
            } else if (!notification.position.isEmpty()) {
                return QCoreApplication::translate("NotificationText", "%1 (%2)")
                    .arg(typeLabel, notification.position);
            }
            return typeLabel;
        }
        case Services::NotificationKind::FollowUp: {
            if (!notification.position.isEmpty()) {
                return QCoreApplication::translate("NotificationText", "%1 — Follow-up due today")
                    .arg(notification.position);
            }
            return QCoreApplication::translate("NotificationText", "Follow-up due today");
        }
        case Services::NotificationKind::OverdueFollowUps: {
            if (notification.refId > 0 && notification.count <= 1) {
                if (!notification.company.isEmpty() && !notification.position.isEmpty()) {
                    return u"%1 — %2"_s.arg(notification.company, notification.position);
                } else if (!notification.company.isEmpty()) {
                    return notification.company;
                } else if (!notification.position.isEmpty()) {
                    return notification.position;
                }
                return QCoreApplication::translate("NotificationText", "Follow-up is overdue");
            }
            if (notification.count == 1) {
                return QCoreApplication::translate("NotificationText", "1 follow-up is overdue");
            }
            return QCoreApplication::translate("NotificationText", "%1 follow-ups are overdue")
                .arg(notification.count);
        }
    }
    return QString();
}

}  // namespace JobPrep::Ui
