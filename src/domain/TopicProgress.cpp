#include "domain/TopicProgress.h"
#include <algorithm>

namespace JobPrep::Domain {

int topicProgress(TopicStatus status, int doneSubtasks, int totalSubtasks) {
    if (totalSubtasks > 0) {
        const int clampedDone = std::clamp(doneSubtasks, 0, totalSubtasks);
        return (clampedDone * 100) / totalSubtasks;
    }
    switch (status) {
        case TopicStatus::Backlog:
            return 0;
        case TopicStatus::InProgress:
            return 50;
        case TopicStatus::Review:
            return 80;
        case TopicStatus::Done:
            return 100;
    }
    return 0;
}

}  // namespace JobPrep::Domain
