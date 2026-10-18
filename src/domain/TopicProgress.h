#pragma once

#include "domain/Enums.h"

namespace JobPrep::Domain {

/// Computes topic progress percentage [0, 100] per SPEC §4.
/// If totalSubtasks > 0, returns (doneSubtasks * 100) / totalSubtasks.
/// Otherwise falls back to status: Backlog 0, InProgress 50, Review 80, Done 100.
int topicProgress(TopicStatus status, int doneSubtasks, int totalSubtasks);

}  // namespace JobPrep::Domain
