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
constexpr int kInterLineGap = 3;
constexpr int kVerticalPadding = 14;

}  // namespace

AgendaItemDelegate::AgendaItemDelegate(JobPrep::Ui::Theme::ThemeManager& theme, QObject* parent)
    : QStyledItemDelegate(parent), m_theme(theme) {}

AgendaItemLayout AgendaItemDelegate::computeLayout(const QStyleOptionViewItem& option,
                                                  const QModelIndex& index) {
    AgendaItemLayout layout;

    QFont titleFont = option.font;
    QFont subtitleFont = option.font;
    subtitleFont.setPointSizeF(qMax(7.5, option.font.pointSizeF() - 1.0));

    const QFontMetrics tm(titleFont);
    const QFontMetrics sm(subtitleFont);

    const QString title = index.data(Qt::DisplayRole).toString();
    const QString subtitle = index.data(JobPrep::Models::AgendaListModel::SubtitleRole).toString();
    layout.hasSubtitle = !subtitle.isEmpty();

    const bool isPast = index.data(JobPrep::Models::AgendaListModel::PastRole).toBool();
    const auto kind = static_cast<Domain::AgendaKind>(
        index.data(JobPrep::Models::AgendaListModel::KindRole).toInt());
    const bool isOverdue = (kind != Domain::AgendaKind::Interview && isPast);

    const QTime timeVal = index.data(JobPrep::Models::AgendaListModel::TimeRole).toTime();
    const QDate dateVal = index.data(JobPrep::Models::AgendaListModel::DateRole).toDate();
    QString time;
    if (timeVal.isValid()) {
        if (dateVal.isValid() && dateVal != QDate::currentDate()) {
            QString dayStr = QLocale().toString(dateVal, u"ddd"_s);
            if (!dayStr.isEmpty()) {
                dayStr[0] = dayStr[0].toUpper();
            }
            time = dayStr + u" "_s + timeVal.toString(u"HH:mm"_s);
        } else {
            time = timeVal.toString(u"HH:mm"_s);
        }
    }
    layout.rightText = isOverdue ? tr("Overdue") : time;

    const int titleH = tm.height();
    const int subtitleH = sm.height();
    const int textBlockHeight = layout.hasSubtitle ? (titleH + kInterLineGap + subtitleH) : titleH;
    layout.rowHeight = qMax(kIconSize + 16, textBlockHeight + kVerticalPadding);

    const int effectiveHeight = option.rect.height() > 0 ? option.rect.height() : layout.rowHeight;
    const int left = option.rect.left() + kHorizontalMargin;
    const int right = option.rect.right() - kHorizontalMargin;

    layout.iconRect = QRect(left, option.rect.top() + (effectiveHeight - kIconSize) / 2,
                            kIconSize, kIconSize);

    const int contentTop = option.rect.top() + (effectiveHeight - textBlockHeight) / 2;
    const int textLeft = layout.iconRect.right() + 8;

    int rightTextWidth = 0;
    if (!layout.rightText.isEmpty()) {
        rightTextWidth = tm.horizontalAdvance(layout.rightText);
        layout.rightTextRect = QRect(right - rightTextWidth + 1, contentTop, rightTextWidth, titleH);
    }

    const int titleAvailableWidth = qMax(0, (!layout.rightText.isEmpty()
                                                 ? (layout.rightTextRect.left() - 8)
                                                 : (right + 1)) - textLeft);
    layout.titleRect = QRect(textLeft, contentTop, titleAvailableWidth, titleH);
    layout.elidedTitle = tm.elidedText(title, Qt::ElideRight, titleAvailableWidth);

    if (layout.hasSubtitle) {
        const int subtitleTop = contentTop + titleH + kInterLineGap;
        const int subtitleAvailableWidth = qMax(0, right - textLeft + 1);
        layout.subtitleRect = QRect(textLeft, subtitleTop, subtitleAvailableWidth, subtitleH);
        layout.elidedSubtitle = sm.elidedText(subtitle, Qt::ElideRight, subtitleAvailableWidth);
    }

    return layout;
}

QSize AgendaItemDelegate::sizeHint(const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const {
    const auto layout = computeLayout(option, index);
    return {0, layout.rowHeight};
}

void AgendaItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                               const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const auto layout = computeLayout(option, index);

    const bool selected = option.state & QStyle::State_Selected;
    const bool isPast = index.data(JobPrep::Models::AgendaListModel::PastRole).toBool();
    const auto kind = static_cast<Domain::AgendaKind>(
        index.data(JobPrep::Models::AgendaListModel::KindRole).toInt());
    const bool dimmed = (kind == Domain::AgendaKind::Interview && isPast);
    const bool isOverdue = (kind != Domain::AgendaKind::Interview && isPast);
    const auto tokens = m_theme.currentTokens();

    if (selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    const QColor kindColor = Theme::AgendaKindStyle::color(kind);
    const QColor iconColor = dimmed ? tokens.textMuted : kindColor;
    const QIcon icon = Theme::AgendaKindStyle::icon(kind, iconColor);
    icon.paint(painter, layout.iconRect);

    QFont titleFont = option.font;
    QFont subtitleFont = option.font;
    subtitleFont.setPointSizeF(qMax(7.5, option.font.pointSizeF() - 1.0));

    QColor titleColor = selected ? option.palette.highlightedText().color() : tokens.text;
    QColor mutedColor = selected ? option.palette.highlightedText().color() : tokens.textMuted;
    if (dimmed) {
        titleColor = tokens.textMuted;
        mutedColor = tokens.textMuted;
    }

    painter->setFont(titleFont);
    painter->setPen(titleColor);
    painter->drawText(layout.titleRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::AlignAbsolute, layout.elidedTitle);

    if (!layout.rightText.isEmpty() && layout.rightTextRect.isValid()) {
        painter->setPen(isOverdue ? tokens.danger : mutedColor);
        painter->drawText(layout.rightTextRect, Qt::AlignRight | Qt::AlignVCenter | Qt::AlignAbsolute, layout.rightText);
    }

    if (layout.hasSubtitle && layout.subtitleRect.isValid()) {
        painter->setFont(subtitleFont);
        painter->setPen(mutedColor);
        painter->drawText(layout.subtitleRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::AlignAbsolute, layout.elidedSubtitle);
    }

    painter->restore();
}

}  // namespace JobPrep::Ui::Delegates