#include "ui/FormatHelper.h"

#include "ui/theme/StatusStyle.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui {

QString FormatHelper::formatDuration(int totalMinutes) {
    if (totalMinutes <= 0) {
        return tr("0m");
    }
    const int hours = totalMinutes / 60;
    const int mins = totalMinutes % 60;
    if (hours > 0 && mins > 0) {
        return tr("%1h %2m").arg(hours).arg(mins);
    }
    if (hours > 0) {
        return tr("%1h").arg(hours);
    }
    return tr("%1m").arg(mins);
}

QString FormatHelper::formatGoalText(int weekMinutes, int goalMinutes) {
    return tr("%1 / %2").arg(formatDuration(weekMinutes), formatDuration(goalMinutes));
}

QString FormatHelper::formatInterviewRate(const std::optional<double>& rate) {
    if (!rate.has_value()) {
        return QString::fromUtf8("\u2013");  // en-dash
    }
    return tr("%1%").arg(QString::number(*rate, 'f', 1));
}

QString FormatHelper::formatStreak(int days) {
    return tr("%1-day streak").arg(days);
}

QString FormatHelper::formatStreakDays(int days) {
    if (days == 1) {
        return tr("1 day");
    }
    return tr("%1 days").arg(days);
}

QString FormatHelper::formatActiveProcesses(int count) {
    if (count == 1) {
        return tr("1 active process");
    }
    return tr("%1 active processes").arg(count);
}

QString FormatHelper::formatNextInterview(const std::optional<Domain::Interview>& interview,
                                         const QString& companyName,
                                         const QLocale& locale) {
    if (!interview.has_value()) {
        return tr("Next: No upcoming interviews");
    }
    const QString day = locale.toString(interview->startAt.date(), u"ddd"_s);
    const QString time = interview->startAt.time().toString(u"hh:mm"_s);
    return tr("Next: %1 %2 %3").arg(day, time, companyName);
}

QString FormatHelper::formatNextInterview(const std::optional<Domain::AgendaItem>& item,
                                         const QLocale& locale) {
    if (!item.has_value()) {
        return tr("Next: No upcoming interviews");
    }
    const QString day = locale.toString(item->date, u"ddd"_s);
    const QString time = item->time.has_value() ? item->time->toString(u"hh:mm"_s) : QString();
    return tr("Next: %1 %2 %3").arg(day, time, item->title);
}

QString FormatHelper::funnelStageLabel(Domain::ApplicationStatus stage) {
    return Theme::StatusStyle::label(stage);
}

}  // namespace JobPrep::Ui
