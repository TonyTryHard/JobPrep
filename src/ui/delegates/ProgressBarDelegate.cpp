#include "ui/delegates/ProgressBarDelegate.h"

#include <QFontMetrics>
#include <QPainter>
#include "models/TopicListModel.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Delegates {

ProgressBarDelegate::ProgressBarDelegate(Theme::ThemeManager& theme, QObject* parent)
    : QStyledItemDelegate(parent), m_theme(theme) {}

void ProgressBarDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                const QModelIndex& index) const {
    const int progress = std::clamp(index.data(Models::TopicListModel::ProgressRole).toInt(), 0, 100);
    const auto tokens = m_theme.currentTokens();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRect rect = option.rect.adjusted(6, 4, -6, -4);
    const QFontMetrics fm(option.font);
    const QString text = QString::number(progress) + u"%"_s;
    const int textWidth = fm.horizontalAdvance(u"100%"_s) + 6;

    // Text on the right
    const QRect textRect(rect.right() - textWidth, rect.top(), textWidth, rect.height());
    painter->setFont(option.font);
    painter->setPen(tokens.textMuted);
    painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, text);

    // Progress track on the left
    constexpr int kBarHeight = 6;
    const int barY = rect.top() + (rect.height() - kBarHeight) / 2;
    const int barWidth = qMax(0, rect.width() - textWidth - 8);
    const QRectF barRect(rect.left(), barY, barWidth, kBarHeight);

    painter->setPen(Qt::NoPen);
    painter->setBrush(tokens.surfaceAlt);
    painter->drawRoundedRect(barRect, 3.0, 3.0);

    if (progress > 0 && barWidth > 0) {
        const qreal fillWidth = barWidth * (progress / 100.0);
        const QRectF fillRect(barRect.left(), barRect.top(), fillWidth, barRect.height());
        painter->setBrush(tokens.accent);
        painter->drawRoundedRect(fillRect, 3.0, 3.0);
    }

    painter->restore();
}

}  // namespace JobPrep::Ui::Delegates
