#include "domain/StageRank.h"

namespace JobPrep::Domain {

std::optional<int> stageRank(ApplicationStatus status) {
    switch (status) {
        case ApplicationStatus::Wishlist:
            return 0;
        case ApplicationStatus::Applied:
            return 1;
        case ApplicationStatus::HrScreen:
            return 2;
        case ApplicationStatus::Technical:
            return 3;
        case ApplicationStatus::Final:
            return 4;
        case ApplicationStatus::Offer:
        case ApplicationStatus::Accepted:
            return 5;
        case ApplicationStatus::Rejected:
        case ApplicationStatus::Withdrawn:
        case ApplicationStatus::Ghosted:
            return std::nullopt;
    }
    return std::nullopt;
}

QList<ApplicationStatus> statusesWithRankAtLeast(int minRank) {
    static constexpr ApplicationStatus allStatuses[] = {
        ApplicationStatus::Wishlist,
        ApplicationStatus::Applied,
        ApplicationStatus::HrScreen,
        ApplicationStatus::Technical,
        ApplicationStatus::Final,
        ApplicationStatus::Offer,
        ApplicationStatus::Accepted,
        ApplicationStatus::Rejected,
        ApplicationStatus::Withdrawn,
        ApplicationStatus::Ghosted,
    };

    QList<ApplicationStatus> result;
    for (const auto status : allStatuses) {
        const auto rank = stageRank(status);
        if (rank.has_value() && *rank >= minRank) {
            result.append(status);
        }
    }
    return result;
}

}  // namespace JobPrep::Domain
