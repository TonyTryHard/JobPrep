#include "ui/widgets/FunnelWidget.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include "ui/FormatHelper.h"
#include "ui/theme/StatusStyle.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Widgets {

FunnelWidget::FunnelWidget(Theme::ThemeManager& theme, QWidget* parent)
    : QWidget(parent), m_theme(theme) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(&m_theme, &Theme::ThemeManager::themeChanged, this, [this] { update(); });
    updateAccessibility();
}

void FunnelWidget::setFunnel(const QList<Services::FunnelStage>& stages,
                             const std::optional<double>& interviewRate) {
    m_stages = stages;
    m_interviewRate = interviewRate;
    updateAccessibility();
    updateGeometry();
    update();
}

void FunnelWidget::updateAccessibility() {
    setAccessibleName(tr("Hiring funnel"));
    QStringList parts;
    for (const auto& stage : m_stages) {
        parts.append(u"%1: %2"_s.arg(Theme::StatusStyle::label(stage.status))
                         .arg(stage.reachedCount));
    }
    parts.append(u"Interview rate: %1"_s.arg(FormatHelper::formatInterviewRate(m_interviewRate)));
    setAccessibleDescription(parts.join(u", "_s));
}

QSize FunnelWidget::sizeHint() const {
    const QFontMetrics labelFm(font());
    QFont countFont = font();
    countFont.setPointSizeF(font().pointSizeF() * 1.5);
    countFont.setBold(true);
    const QFontMetrics countFm(countFont);

    const int stageCount = qMax(1, static_cast<int>(m_stages.size()));
    const int stageWidth = qMax(72, static_cast<int>(labelFm.height() * 5.5));
    const int gap = qMax(8, labelFm.height() / 2);

    const int totalWidth = stageCount * stageWidth + (stageCount - 1) * gap;

    const int barHeight = qMax(4, labelFm.height() / 3);
    const int stageHeight = barHeight + gap / 2 + countFm.height() + gap / 2 + labelFm.height();
    const int rateHeight = labelFm.height() + gap;
    const int totalHeight = stageHeight + gap + rateHeight;

    return {totalWidth, totalHeight};
}

QSize FunnelWidget::minimumSizeHint() const {
    const QFontMetrics labelFm(font());
    QFont countFont = font();
    countFont.setPointSizeF(font().pointSizeF() * 1.5);
    countFont.setBold(true);
    const QFontMetrics countFm(countFont);

    const int stageCount = qMax(1, static_cast<int>(m_stages.size()));
    const int stageWidth = qMax(48, static_cast<int>(labelFm.height() * 3.5));
    const int gap = qMax(4, labelFm.height() / 4);

    const int totalWidth = stageCount * stageWidth + (stageCount - 1) * gap;
    const int barHeight = qMax(4, labelFm.height() / 3);
    const int stageHeight = barHeight + gap / 2 + countFm.height() + gap / 2 + labelFm.height();
    const int rateHeight = labelFm.height() + gap;

    return {totalWidth, stageHeight + gap + rateHeight};
}

void FunnelWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    if (m_stages.isEmpty()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const auto tokens = m_theme.currentTokens();
    const QFontMetrics labelFm(font());
    QFont countFont = font();
    countFont.setPointSizeF(font().pointSizeF() * 1.5);
    countFont.setBold(true);
    const QFontMetrics countFm(countFont);

    const int stageCount = static_cast<int>(m_stages.size());
    const int barHeight = qMax(4, labelFm.height() / 3);
    const int gap = qMax(8, labelFm.height() / 2);
    const int arrowWidth = qMax(10, labelFm.height());

    const int availableW = width();
    const int totalGaps = (stageCount - 1) * (gap + arrowWidth);
    const int stageColWidth = qMax(32, (availableW - totalGaps) / stageCount);

    // 1. Draw stages
    for (int i = 0; i < stageCount; ++i) {
        const auto& stage = m_stages.at(i);
        const int colX = i * (stageColWidth + gap + arrowWidth);

        // Stage indicator bar
        const QRectF barRect(colX, 0, stageColWidth, barHeight);
        const QColor stageColor = Theme::StatusStyle::color(stage.status);
        painter.setPen(Qt::NoPen);
        painter.setBrush(stageColor);
        painter.drawRoundedRect(barRect, 2.0, 2.0);

        // Count
        painter.setFont(countFont);
        painter.setPen(stage.reachedCount == 0 ? tokens.textMuted : tokens.text);
        const QRect countRect(colX, barHeight + gap / 2, stageColWidth, countFm.height());
        painter.drawText(countRect, Qt::AlignCenter, QString::number(stage.reachedCount));

        // Label
        painter.setFont(font());
        painter.setPen(tokens.textMuted);
        const int labelTop = barHeight + gap / 2 + countFm.height() + gap / 2;
        const QRect textRect(colX, labelTop, stageColWidth, labelFm.height());
        const QString label = Theme::StatusStyle::label(stage.status);
        painter.drawText(textRect, Qt::AlignCenter,
                         labelFm.elidedText(label, Qt::ElideRight, stageColWidth));

        // Separator arrow between stages
        if (i < stageCount - 1) {
            const int arrowX = colX + stageColWidth + gap / 2;
            const QRect arrowRect(arrowX, barHeight + gap / 2, arrowWidth, countFm.height());
            painter.setFont(font());
            painter.setPen(tokens.textMuted);
            painter.drawText(arrowRect, Qt::AlignCenter, u"→"_s);
        }
    }

    // 2. Interview rate at bottom
    const int stageHeight = barHeight + gap / 2 + countFm.height() + gap / 2 + labelFm.height();
    const int rateY = stageHeight + gap;

    painter.setFont(font());
    painter.setPen(tokens.textMuted);
    const QString ratePrefix = tr("Interview rate: ");
    const int prefixWidth = labelFm.horizontalAdvance(ratePrefix);

    QFont rateFont = font();
    rateFont.setBold(true);
    const QFontMetrics rateFm(rateFont);
    const QString rateStr = FormatHelper::formatInterviewRate(m_interviewRate);

    const QRect rateRect(0, rateY, availableW, labelFm.height());
    painter.drawText(rateRect, Qt::AlignLeft | Qt::AlignVCenter, ratePrefix);

    painter.setFont(rateFont);
    painter.setPen(m_interviewRate.has_value() ? tokens.accent : tokens.textMuted);
    const QRect valRect(prefixWidth, rateY, availableW - prefixWidth, rateFm.height());
    painter.drawText(valRect, Qt::AlignLeft | Qt::AlignVCenter, rateStr);
}

}  // namespace JobPrep::Ui::Widgets
