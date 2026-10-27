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

QString FormatHelper::formatLeadTime(int minutes) {
    if (minutes <= 0) {
        return tr("0 minutes");
    }
    if (minutes % 1440 == 0) {
        const int days = minutes / 1440;
        return days == 1 ? tr("1 day") : tr("%1 days").arg(days);
    }
    if (minutes % 60 == 0) {
        const int hours = minutes / 60;
        return hours == 1 ? tr("1 hour") : tr("%1 hours").arg(hours);
    }
    return minutes == 1 ? tr("1 minute") : tr("%1 minutes").arg(minutes);
}

QString FormatHelper::formatInterviewRate(const std::optional<double>& rate) {
    if (!rate.has_value()) {
        return QString::fromUtf8("\u2013");  // en-dash
    }
    return tr("%1%").arg(QString::number(*rate * 100.0, 'f', 1));
}

QString FormatHelper::formatStreak(int days) {
    return tr("%1-day streak").arg(days);
}

QString FormatHelper::formatActiveProcesses(int count) {
    if (count == 1) {
        return tr("1 active process");
    }
    return tr("%1 active processes").arg(count);
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
