#pragma once

#include <optional>
#include <QCoreApplication>
#include <QDateTime>
#include <QLocale>
#include <QString>
#include "domain/Enums.h"
#include "domain/Structs.h"

namespace JobPrep::Ui {

/// Centralized formatting helpers for dashboard metrics, goals, rates and status text.
class FormatHelper {
    Q_DECLARE_TR_FUNCTIONS(JobPrep::Ui::FormatHelper)

public:
    /// Formats total minutes as "Xh Ym", "Xh", or "Ym" (e.g. 260 -> "4h 20m", 360 -> "6h", 45 -> "45m", 0 -> "0m").
    static QString formatDuration(int totalMinutes);

    /// Formats weekly study progress vs goal as "Xh Ym / Zh" (e.g. "4h 20m / 6h", "7h 10m / 6h").
    static QString formatGoalText(int weekMinutes, int goalMinutes);

    /// Formats reminder lead time with singular/plural wording (e.g. 1440 -> "1 day", 120 -> "2 hours", 90 -> "90 minutes").
    static QString formatLeadTime(int minutes);

    /// Formats interview rate percentage with 1 decimal digit (e.g. "66.7%"), or "–" (en-dash) if nullopt.
    static QString formatInterviewRate(const std::optional<double>& rate);

    /// Formats streak days as a compound label (e.g. 0 -> "0-day streak", 1 -> "1-day streak", 4 -> "4-day streak").
    static QString formatStreak(int days);

    /// Formats active processes count with singular/plural wording (e.g. "0 active processes", "1 active process").
    static QString formatActiveProcesses(int count);

    /// Formats next interview for status bar using an AgendaItem:
    /// If no upcoming interview: "Next: No upcoming interviews"
    /// If interview present: "Next: <ddd> <hh:mm> <Company>" e.g. "Next: Fri 14:00 Globex".
    static QString formatNextInterview(const std::optional<Domain::AgendaItem>& item,
                                       const QLocale& locale = QLocale());

    /// Returns human-readable label for a hiring funnel stage.
    static QString funnelStageLabel(Domain::ApplicationStatus stage);
};

}  // namespace JobPrep::Ui
