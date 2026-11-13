#include "ui/NotificationText.h"

#include <QCoreApplication>
#include "ui/theme/StatusStyle.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui {

namespace {
inline QString englishPlural(int n, const QString& one, const QString& otherPattern) {
    if (n == 1) {
        return one;
    }
    return otherPattern.arg(n);
}
}  // namespace

QString NotificationText::formatLeadTime(int leadMinutes) {
    if (leadMinutes <= 0) return QString();
    if (leadMinutes % 60 == 0) {
        const int hours = leadMinutes / 60;
        const QString translated =
            QCoreApplication::translate("NotificationText", "in %n hour(s)", "", hours);
        if (translated != QString(u"in %1 hour(s)"_s).arg(hours)) {
            return translated;
        }
        return englishPlural(hours, u"in 1 hour"_s, u"in %1 hours"_s);
    }
    const QString translated =
        QCoreApplication::translate("NotificationText", "in %n minute(s)", "", leadMinutes);
    if (translated != QString(u"in %1 minute(s)"_s).arg(leadMinutes)) {
        return translated;
    }
    return englishPlural(leadMinutes, u"in 1 minute"_s, u"in %1 minutes"_s);
}

QString NotificationText::formatRemainingTime(const Services::Notification& notification,
                                             const QDateTime& now) {
    if (notification.when.isValid()) {
        const QDateTime current = now.isValid() ? now : QDateTime::currentDateTime();
        const qint64 secs = current.secsTo(notification.when);
        if (secs > 0) {
            const qint64 mins = (secs + 30) / 60;
            if (mins < 60) {
                const QString translated =
                    QCoreApplication::translate("NotificationText", "in %n minute(s)", "", mins);
                if (translated != QString(u"in %1 minute(s)"_s).arg(mins)) {
                    return translated;
                }
                return englishPlural(mins, u"in 1 minute"_s, u"in %1 minutes"_s);
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
                const QString translated =
                    QCoreApplication::translate("NotificationText", "in %n hour(s)", "", hours);
                if (translated != QString(u"in %1 hour(s)"_s).arg(hours)) {
                    return translated;
                }
                return englishPlural(hours, u"in 1 hour"_s, u"in %1 hours"_s);
            }
            return notification.when.toString(u"yyyy-MM-dd HH:mm"_s);
        }
    }
    return formatLeadTime(notification.leadMinutes);
}

QString NotificationText::formatTitle(const Services::Notification& notification,
                                      const QDateTime& now) {
    switch (notification.kind) {
        case Services::NotificationKind::Interview: {
            const QString timeStr = formatRemainingTime(notification, now);
            if (!timeStr.isEmpty()) {
                return QCoreApplication::translate("NotificationText", "Interview %1").arg(timeStr);
            }
            return QCoreApplication::translate("NotificationText", "Interview Reminder");
        }
        case Services::NotificationKind::FollowUp:
            return QCoreApplication::translate("NotificationText", "Follow-up due today");
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
    Q_UNUSED(now);
    switch (notification.kind) {
        case Services::NotificationKind::Interview: {
            const QString typeLabel = notification.interviewType
                                          ? Theme::StatusStyle::label(*notification.interviewType)
                                          : QString();
            // Format: Company · Type (position)
            if (!notification.company.isEmpty()) {
                if (!typeLabel.isEmpty() && !notification.position.isEmpty()) {
                    return u"%1 · %2 (%3)"_s.arg(notification.company, typeLabel, notification.position);
                } else if (!typeLabel.isEmpty()) {
                    return u"%1 · %2"_s.arg(notification.company, typeLabel);
                } else if (!notification.position.isEmpty()) {
                    return u"%1 (%2)"_s.arg(notification.company, notification.position);
                }
                return notification.company;
            }
            if (!typeLabel.isEmpty() && !notification.position.isEmpty()) {
                return u"%1 (%2)"_s.arg(typeLabel, notification.position);
            } else if (!typeLabel.isEmpty()) {
                return typeLabel;
            } else if (!notification.position.isEmpty()) {
                return notification.position;
            }
            return QCoreApplication::translate("NotificationText", "Interview");
        }
        case Services::NotificationKind::FollowUp: {
            if (!notification.company.isEmpty() && !notification.position.isEmpty()) {
                return u"%1 · %2"_s.arg(notification.company, notification.position);
            } else if (!notification.company.isEmpty()) {
                return notification.company;
            } else if (!notification.position.isEmpty()) {
                return notification.position;
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
            const QString translated = QCoreApplication::translate(
                "NotificationText", "%n follow-up(s) overdue", "", notification.count);
            if (translated != QString(u"%1 follow-up(s) overdue"_s).arg(notification.count)) {
                return translated;
            }
            return englishPlural(notification.count, u"1 follow-up is overdue"_s, u"%1 follow-ups are overdue"_s);
        }
    }
    return QString();
}

}  // namespace JobPrep::Ui
