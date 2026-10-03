#include "domain/EnumStrings.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Domain {

QString EnumStrings::toString(TopicStatus status) {
    switch (status) {
        case TopicStatus::Backlog:
            return u"backlog"_s;
        case TopicStatus::InProgress:
            return u"in_progress"_s;
        case TopicStatus::Review:
            return u"review"_s;
        case TopicStatus::Done:
            return u"done"_s;
    }
    return u"backlog"_s;
}

std::optional<TopicStatus> EnumStrings::topicStatusFromString(QStringView str) {
    if (str == u"backlog") return TopicStatus::Backlog;
    if (str == u"in_progress") return TopicStatus::InProgress;
    if (str == u"review") return TopicStatus::Review;
    if (str == u"done") return TopicStatus::Done;
    return std::nullopt;
}

QString EnumStrings::toString(Priority priority) {
    switch (priority) {
        case Priority::Low:
            return u"low"_s;
        case Priority::Normal:
            return u"normal"_s;
        case Priority::High:
            return u"high"_s;
    }
    return u"normal"_s;
}

std::optional<Priority> EnumStrings::priorityFromString(QStringView str) {
    if (str == u"low") return Priority::Low;
    if (str == u"normal") return Priority::Normal;
    if (str == u"high") return Priority::High;
    return std::nullopt;
}

QString EnumStrings::toString(ApplicationStatus status) {
    switch (status) {
        case ApplicationStatus::Wishlist:
            return u"wishlist"_s;
        case ApplicationStatus::Applied:
            return u"applied"_s;
        case ApplicationStatus::HrScreen:
            return u"hr_screen"_s;
        case ApplicationStatus::Technical:
            return u"technical"_s;
        case ApplicationStatus::Final:
            return u"final"_s;
        case ApplicationStatus::Offer:
            return u"offer"_s;
        case ApplicationStatus::Accepted:
            return u"accepted"_s;
        case ApplicationStatus::Rejected:
            return u"rejected"_s;
        case ApplicationStatus::Withdrawn:
            return u"withdrawn"_s;
        case ApplicationStatus::Ghosted:
            return u"ghosted"_s;
    }
    return u"applied"_s;
}

std::optional<ApplicationStatus> EnumStrings::applicationStatusFromString(QStringView str) {
    if (str == u"wishlist") return ApplicationStatus::Wishlist;
    if (str == u"applied") return ApplicationStatus::Applied;
    if (str == u"hr_screen") return ApplicationStatus::HrScreen;
    if (str == u"technical") return ApplicationStatus::Technical;
    if (str == u"final") return ApplicationStatus::Final;
    if (str == u"offer") return ApplicationStatus::Offer;
    if (str == u"accepted") return ApplicationStatus::Accepted;
    if (str == u"rejected") return ApplicationStatus::Rejected;
    if (str == u"withdrawn") return ApplicationStatus::Withdrawn;
    if (str == u"ghosted") return ApplicationStatus::Ghosted;
    return std::nullopt;
}

QString EnumStrings::toString(WorkMode mode) {
    switch (mode) {
        case WorkMode::Remote:
            return u"remote"_s;
        case WorkMode::Hybrid:
            return u"hybrid"_s;
        case WorkMode::Onsite:
            return u"onsite"_s;
    }
    return u"remote"_s;
}

std::optional<WorkMode> EnumStrings::workModeFromString(QStringView str) {
    if (str == u"remote") return WorkMode::Remote;
    if (str == u"hybrid") return WorkMode::Hybrid;
    if (str == u"onsite") return WorkMode::Onsite;
    return std::nullopt;
}

QString EnumStrings::toString(InterviewType type) {
    switch (type) {
        case InterviewType::Hr:
            return u"hr"_s;
        case InterviewType::Technical:
            return u"technical"_s;
        case InterviewType::LiveCoding:
            return u"live_coding"_s;
        case InterviewType::SystemDesign:
            return u"system_design"_s;
        case InterviewType::Behavioral:
            return u"behavioral"_s;
        case InterviewType::Final:
            return u"final"_s;
        case InterviewType::Other:
            return u"other"_s;
    }
    return u"technical"_s;
}

std::optional<InterviewType> EnumStrings::interviewTypeFromString(QStringView str) {
    if (str == u"hr") return InterviewType::Hr;
    if (str == u"technical") return InterviewType::Technical;
    if (str == u"live_coding") return InterviewType::LiveCoding;
    if (str == u"system_design") return InterviewType::SystemDesign;
    if (str == u"behavioral") return InterviewType::Behavioral;
    if (str == u"final") return InterviewType::Final;
    if (str == u"other") return InterviewType::Other;
    return std::nullopt;
}

QString EnumStrings::toString(InterviewOutcome outcome) {
    switch (outcome) {
        case InterviewOutcome::Pending:
            return u"pending"_s;
        case InterviewOutcome::Passed:
            return u"passed"_s;
        case InterviewOutcome::Failed:
            return u"failed"_s;
        case InterviewOutcome::Cancelled:
            return u"cancelled"_s;
    }
    return u"pending"_s;
}

std::optional<InterviewOutcome> EnumStrings::interviewOutcomeFromString(QStringView str) {
    if (str == u"pending") return InterviewOutcome::Pending;
    if (str == u"passed") return InterviewOutcome::Passed;
    if (str == u"failed") return InterviewOutcome::Failed;
    if (str == u"cancelled") return InterviewOutcome::Cancelled;
    return std::nullopt;
}

QString EnumStrings::toString(AgendaKind kind) {
    switch (kind) {
        case AgendaKind::Interview:
            return u"interview"_s;
        case AgendaKind::FollowUp:
            return u"follow_up"_s;
        case AgendaKind::StudyTarget:
            return u"study_target"_s;
    }
    return u"interview"_s;
}

std::optional<AgendaKind> EnumStrings::agendaKindFromString(QStringView str) {
    if (str == u"interview") return AgendaKind::Interview;
    if (str == u"follow_up") return AgendaKind::FollowUp;
    if (str == u"study_target") return AgendaKind::StudyTarget;
    return std::nullopt;
}

}  // namespace JobPrep::Domain
