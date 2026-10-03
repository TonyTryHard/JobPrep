#pragma once

#include <optional>
#include <QString>
#include <QStringView>
#include "domain/Enums.h"

namespace JobPrep::Domain {

/// Centralized conversions between domain enums and snake_case strings.
class EnumStrings {
public:
    static QString toString(TopicStatus status);
    static std::optional<TopicStatus> topicStatusFromString(QStringView str);

    static QString toString(Priority priority);
    static std::optional<Priority> priorityFromString(QStringView str);

    static QString toString(ApplicationStatus status);
    static std::optional<ApplicationStatus> applicationStatusFromString(QStringView str);

    static QString toString(WorkMode mode);
    static std::optional<WorkMode> workModeFromString(QStringView str);

    static QString toString(InterviewType type);
    static std::optional<InterviewType> interviewTypeFromString(QStringView str);

    static QString toString(InterviewOutcome outcome);
    static std::optional<InterviewOutcome> interviewOutcomeFromString(QStringView str);

    static QString toString(AgendaKind kind);
    static std::optional<AgendaKind> agendaKindFromString(QStringView str);
};

}  // namespace JobPrep::Domain
