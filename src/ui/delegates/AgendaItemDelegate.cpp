#include "ui/delegates/AgendaItemDelegate.h"

#include <QFontMetrics>
#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionViewItem>
#include "models/AgendaListModel.h"
#include "ui/theme/AgendaKindStyle.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Delegates {

namespace {

constexpr int kIconSize = 16;
constexpr int kHorizontalMargin = 10;
constexpr int kInterItemGap = 2;
constexpr int kRowHeight = 44;

}  // namespace

AgendaItemDelegate::AgendaItemDelegate(JobPrep::Ui::Theme::ThemeManager& theme, QObject* parent)
    : QStyledItemDelegate(parent), m_theme(theme) {}

QSize AgendaItemDelegate::sizeHint(const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const {
    Q_UNUSED(option);
    Q_UNUSED(index);
    return {0, kRowHeight};
}

void AgendaItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                               const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const bool selected = option.state & QStyle::State_Selected;
    const bool dimmed = index.data(JobPrep::Models::AgendaListModel::PastRole).toBool();
    const auto tokens = m_theme.currentTokens();

    if (selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    const auto kind = static_cast<Domain::AgendaKind>(
        index.data(JobPrep::Models::AgendaListModel::KindRole).toInt());
    const QColor kindColor = Theme::AgendaKindStyle::color(kind);
    const QColor iconColor = dimmed ? tokens.textMuted : kindColor;
    const QIcon icon = Theme::AgendaKindStyle::icon(kind, iconColor);

    const QRect content = option.rect.adjusted(kHorizontalMargin, (option.rect.height() - kRowHeight) / 2,
                                               -kHorizontalMargin, -((option.rect.height() - kRowHeight) / 2));
    QRect iconRect(content.topLeft(), QSize(kIconSize, kIconSize));
    iconRect.translate(0, (content.height() - kIconSize) / 2);
    icon.paint(painter, iconRect);

    QFont titleFont = option.font;
    QFont subtitleFont = option.font;
    subtitleFont.setPointSizeF(qMax(7.5, option.font.pointSizeF() - 1.0));

    QColor titleColor = selected ? option.palette.highlightedText().color() : tokens.text;
    QColor mutedColor =
        selected ? option.palette.highlightedText().color() : tokens.textMuted;
    if (dimmed) {
        titleColor = tokens.textMuted;
        mutedColor = tokens.textMuted;
    }

    const QRect textRect = content.adjusted(kIconSize + 8, 3, 0, -3);
    const QString title = index.data(Qt::DisplayRole).toString();
    const QString subtitle = index.data(JobPrep::Models::AgendaListModel::SubtitleRole).toString();
    const QString time = index.data(JobPrep::Models::AgendaListModel::TimeRole).toString();

    painter->setFont(titleFont);
    painter->setPen(titleColor);
    const QFontMetrics titleMetrics(titleFont);
    const int titleWidth = time.isEmpty() ? textRect.width() : textRect.width() - titleMetrics.horizontalAdvance(time) - 12;
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::AlignAbsolute,
                      titleMetrics.elidedText(title, Qt::ElideRight, titleWidth));

    if (!time.isEmpty()) {
        painter->setPen(mutedColor);
        painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter | Qt::AlignAbsolute, time);
    }

    if (!subtitle.isEmpty()) {
        painter->setFont(subtitleFont);
        painter->setPen(mutedColor);
        const QFontMetrics subtitleMetrics(subtitleFont);
        painter->drawText(textRect.adjusted(0, titleMetrics.height() / 2 + 2, 0, 0),
                          Qt::AlignLeft | Qt::AlignVCenter | Qt::AlignAbsolute,
                          subtitleMetrics.elidedText(subtitle, Qt::ElideRight, textRect.width()));
    }

    painter->restore();
}

}  // namespace JobPrep::Ui::Delegates