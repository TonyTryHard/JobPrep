#pragma once

#include <optional>
#include <QList>
#include "domain/Enums.h"

namespace JobPrep::Domain {

/// Computes application stage rank per SPEC §6.
/// Wishlist: 0, Applied: 1, HrScreen: 2, Technical: 3, Final: 4, Offer: 5, Accepted: 5.
/// Rejected, Withdrawn, Ghosted have no rank (std::nullopt).
std::optional<int> stageRank(ApplicationStatus status);

/// Returns all application statuses that have stageRank >= minRank.
/// The list is derived exclusively from stageRank() without hardcoding status names.
QList<ApplicationStatus> statusesWithRankAtLeast(int minRank);

}  // namespace JobPrep::Domain
