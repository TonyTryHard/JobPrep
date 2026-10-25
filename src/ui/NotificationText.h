#pragma once

#include <QString>
#include "services/INotifier.h"

namespace JobPrep::Ui {

/// Formats data-carrying notifications into localized presentation text (SPEC §8, Amendment D).
class NotificationText {
public:
    static QString formatLeadTime(int leadMinutes);
    static QString formatRemainingTime(const Services::Notification& notification,
                                       const QDateTime& now = QDateTime());
    static QString formatTitle(const Services::Notification& notification,
                               const QDateTime& now = QDateTime());
    static QString formatMessage(const Services::Notification& notification,
                                 const QDateTime& now = QDateTime());
};

}  // namespace JobPrep::Ui
