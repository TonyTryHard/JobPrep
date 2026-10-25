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

QString NotificationText::formatRemainingTime(const Services::Notification& notification,
                                             const QDateTime& now) {
    if (notification.when.isValid()) {
        const QDateTime current = now.isValid() ? now : QDateTime::currentDateTime();
        const qint64 secs = current.secsTo(notification.when);
        if (secs > 0) {
            const qint64 mins = (secs + 30) / 60;
            if (mins < 60) {
                if (mins <= 1) {
                    return QCoreApplication::translate("NotificationText", "in 1 minute");
                }
                return QCoreApplication::translate("NotificationText", "in %1 minutes").arg(mins);
            }
            if (notification.when.date() == current.date()) {
                return QCoreApplication::translate("NotificationText", "today at %1")
                    .arg(notification.when.toString(u"HH:mm"_s));
            }
            if (notification.when.date() == current.date().addDays(1)) {
                return QCoreApplication::translate("NotificationText", "tomorrow at %1")
                    .arg(notification.when.toString(u"HH:mm"_s));
            }
            const qint64 hours = (secs + 1800) / 3600;
            if (hours < 24) {
                return QCoreApplication::translate("NotificationText", "in %1 hours").arg(hours);
            }
            return notification.when.toString(u"yyyy-MM-dd HH:mm"_s);
        }
    }
    return formatLeadTime(notification.leadMinutes);
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

QString NotificationText::formatMessage(const Services::Notification& notification,
                                        const QDateTime& now) {
    switch (notification.kind) {
        case Services::NotificationKind::Interview: {
            const QString typeLabel = notification.interviewType
                                          ? Theme::StatusStyle::label(*notification.interviewType)
                                          : QCoreApplication::translate("NotificationText", "Interview");
            const QString timeStr = formatRemainingTime(notification, now);
            if (!notification.position.isEmpty() && !timeStr.isEmpty()) {
                return QCoreApplication::translate("NotificationText", "%1 (%2) %3")
                    .arg(typeLabel, notification.position, timeStr);
            } else if (!timeStr.isEmpty()) {
                return QCoreApplication::translate("NotificationText", "%1 %2")
                    .arg(typeLabel, timeStr);
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
