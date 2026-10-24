#pragma once

#include <optional>
#include <QDateTime>
#include <QString>
#include "domain/Enums.h"

namespace JobPrep::Services {

/// Notification kind category for reminder notifications.
enum class NotificationKind {
    Interview,
    FollowUp,
    OverdueFollowUps
};

/// Data-carrying notification payload (SPEC §14 v1.7).
/// Presentation text is built exclusively by notifiers in ui/.
struct Notification {
    NotificationKind kind{NotificationKind::Interview};
    int refId{0};  // interview id or application id; 0 for a multi-application summary
    QString company;
    QString position;
    std::optional<Domain::InterviewType> interviewType{std::nullopt};
    QDateTime when;
    int leadMinutes{0};
    int count{1};
};

/// Abstract interface for notification delivery (SPEC §8, Amendment C1).
class INotifier {
public:
    virtual ~INotifier() = default;

    /// Delivers the notification. Returns true only if presented to the user.
    /// If false is returned (e.g. window hidden or minimized), the reminder is retried.
    virtual bool notify(const Notification& notification) = 0;
};

}  // namespace JobPrep::Services
