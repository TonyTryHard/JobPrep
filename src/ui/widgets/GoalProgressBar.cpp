#include "ui/widgets/GoalProgressBar.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include "ui/FormatHelper.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Widgets {

GoalProgressBar::GoalProgressBar(Theme::ThemeManager& theme, QWidget* parent)
    : QWidget(parent), m_theme(theme) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(&m_theme, &Theme::ThemeManager::themeChanged, this, [this] { update(); });
    updateAccessibility();
}

void GoalProgressBar::setGoal(int weekMinutes, int goalMinutes) {
    m_weekMinutes = qMax(0, weekMinutes);
    m_goalMinutes = qMax(1, goalMinutes);
    updateAccessibility();
    updateGeometry();
    update();
}

void GoalProgressBar::updateAccessibility() {
    const int realPercent =
        m_goalMinutes > 0 ? qRound(m_weekMinutes * 100.0 / m_goalMinutes) : 0;
    setAccessibleName(tr("Weekly study goal"));
    setAccessibleDescription(
        tr("%1 of %2 (%3%)")
            .arg(FormatHelper::formatDuration(m_weekMinutes),
                 FormatHelper::formatDuration(m_goalMinutes))
            .arg(realPercent));
}

QSize GoalProgressBar::sizeHint() const {
    const QFontMetrics fm(font());
    const int barHeight = qMax(6, fm.height() / 3);
    const int gap = qMax(4, fm.height() / 4);
    const int totalHeight = fm.height() + gap + barHeight;
    return {200, totalHeight};
}

QSize GoalProgressBar::minimumSizeHint() const {
    return sizeHint();
}

void GoalProgressBar::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const auto tokens = m_theme.currentTokens();
    const QFontMetrics fm(font());
    const int barHeight = qMax(6, fm.height() / 3);
    const int gap = qMax(4, fm.height() / 4);

    const int realPercent =
        m_goalMinutes > 0 ? qRound(m_weekMinutes * 100.0 / m_goalMinutes) : 0;
    const QString goalText =
        u"%1 (%2%)"_s.arg(FormatHelper::formatGoalText(m_weekMinutes, m_goalMinutes)).arg(realPercent);

    // Left label: "Weekly goal"
    painter.setFont(font());
    painter.setPen(tokens.textMuted);
    const QRect leftRect(0, 0, width() / 2, fm.height());
    painter.drawText(leftRect, Qt::AlignLeft | Qt::AlignVCenter, tr("Weekly goal"));

    // Right label: "4h 20m / 6h (72%)"
    painter.setPen(tokens.text);
    const QRect rightRect(width() / 2, 0, width() - width() / 2, fm.height());
    painter.drawText(rightRect, Qt::AlignRight | Qt::AlignVCenter, goalText);

    // Bar background track
    const int barY = fm.height() + gap;
    const qreal radius = barHeight / 2.0;
    const QRectF trackRect(0, barY, width(), barHeight);

    painter.setPen(Qt::NoPen);
    painter.setBrush(tokens.border);
    painter.drawRoundedRect(trackRect, radius, radius);

    // Bar fill (capped visually at 100%)
    if (m_weekMinutes > 0 && width() > 0) {
        const double cappedFraction = qMin(1.0, m_weekMinutes / static_cast<double>(m_goalMinutes));
        const qreal fillWidth = qMax(radius * 2.0, width() * cappedFraction);
        const QRectF fillRect(0, barY, qMin(static_cast<qreal>(width()), fillWidth), barHeight);

        painter.setBrush(tokens.accent);
        painter.drawRoundedRect(fillRect, radius, radius);
    }
}

}  // namespace JobPrep::Ui::Widgets
