#pragma once

namespace JobPrep::Domain {

/// Status of a study topic.
enum class TopicStatus {
    Backlog,
    InProgress,
    Review,
    Done,
};

/// Priority level for topics and tasks.
enum class Priority {
    Low,
    Normal,
    High,
};

/// Lifecycle status for job applications.
enum class ApplicationStatus {
    Wishlist,
    Applied,
    HrScreen,
    Technical,
    Final,
    Offer,
    Accepted,
    Rejected,
    Withdrawn,
    Ghosted,
};

/// Work arrangement mode.
enum class WorkMode {
    Remote,
    Hybrid,
    Onsite,
};

/// Type or stage of an interview round.
enum class InterviewType {
    Hr,
    Technical,
    LiveCoding,
    SystemDesign,
    Behavioral,
    Final,
    Other,
};

/// Outcome of an interview.
enum class InterviewOutcome {
    Pending,
    Passed,
    Failed,
    Cancelled,
};

/// Category of an agenda calendar item.
enum class AgendaKind {
    Interview,
    FollowUp,
    StudyTarget,
};

}  // namespace JobPrep::Domain
