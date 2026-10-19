#include "ui/delegates/TopicCardDelegate.h"

#include <QCoreApplication>
#include <QFontMetrics>
#include <QLocale>
#include <QModelIndex>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionViewItem>
#include "models/TopicListModel.h"
#include "ui/theme/StatusStyle.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Delegates {

namespace {

constexpr qreal kCardRadius = 12.0;
constexpr qreal kStripeWidth = 6.0;
constexpr qreal kChipRadius = 8.0;
constexpr int kGap = 4;

/// Marker text for a priority, readable without colour (B3).
QString priorityMark(Domain::Priority priority) {
    switch (priority) {
        case Domain::Priority::High:
            return QCoreApplication::translate("TopicCardDelegate", "↑ High");
        case Domain::Priority::Normal:
            return QCoreApplication::translate("TopicCardDelegate", "· Normal");
        case Domain::Priority::Low:
            return QCoreApplication::translate("TopicCardDelegate", "↓ Low");
    }
    return {};
}

}  // namespace

TopicCardDelegate::TopicCardDelegate(Theme::ThemeManager& theme, QObject* parent)
    : QStyledItemDelegate(parent), m_theme(theme) {}

QSize TopicCardDelegate::sizeHint(const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const {
    QFont titleFont = option.font;
    titleFont.setWeight(QFont::DemiBold);

    const QFontMetrics metrics(option.font);
    const QFontMetrics titleMetrics(titleFont);
    const int padding = metrics.height() / 2;
    const int chipHeight = metrics.height() + 6;

    int height = padding + titleMetrics.height();
    const int total = index.data(Models::TopicListModel::TotalSubtasksRole).toInt();
    if (total > 0) {
        height += kGap + 6 + metrics.height() + kGap;
    }
    height += kGap + chipHeight + padding;

    return {option.rect.isValid() ? option.rect.width() : 160, height};
}

void TopicCardDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const auto tokens = m_theme.currentTokens();
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;

    const QRectF card = option.rect.adjusted(2, 1, -2, -1);
    if (card.width() <= 4 || card.height() <= 4) {
        painter->restore();
        return;
    }

    QColor trackColor(index.data(Models::TopicListModel::TrackColorRole).toString());
    if (!trackColor.isValid()) trackColor = tokens.accent;

    painter->setPen(Qt::NoPen);
    painter->setBrush(selected ? tokens.surfaceAlt : tokens.surface);
    painter->drawRoundedRect(card, kCardRadius, kCardRadius);

    // Track stripe clipped to the rounded card body.
    {
        QPainterPath clipPath;
        clipPath.addRoundedRect(card, kCardRadius, kCardRadius);
        painter->save();
        painter->setClipPath(clipPath);
        painter->setBrush(trackColor);
        painter->drawRect(QRectF(card.left(), card.top(), kStripeWidth, card.height()));
        painter->restore();
    }

    QPen borderPen(selected || hovered ? tokens.accent : tokens.border);
    borderPen.setWidthF(selected ? 1.5 : 1.0);
    painter->setPen(borderPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(card, kCardRadius, kCardRadius);

    const int padding = option.fontMetrics.height() / 2;
    const QRectF content = card.adjusted(kStripeWidth + padding, padding, -padding, -padding);

    QFont titleFont = option.font;
    titleFont.setWeight(QFont::DemiBold);
    painter->setFont(titleFont);
    painter->setPen(tokens.text);

    const bool overdue = index.data(Models::TopicListModel::OverdueRole).toBool();
    const int total = index.data(Models::TopicListModel::TotalSubtasksRole).toInt();
    const int done = index.data(Models::TopicListModel::DoneSubtasksRole).toInt();

    // Bottom row: due chip on the left, priority marker on the right.
    const QFontMetrics metrics(option.font);
    const int chipHeight = metrics.height() + 6;
    QRectF chipRect(content.left(), content.bottom() - chipHeight, 0, chipHeight);
    QRectF priorityRect = content;
    priorityRect.setTop(chipRect.top());

    const auto targetDate = index.data(Models::TopicListModel::TargetDateRole).toDate();
    if (targetDate.isValid()) {
        const QString chipText = overdue
                                     ? u"! %1"_s.arg(QLocale().toString(targetDate, QLocale::ShortFormat))
                                     : QLocale().toString(targetDate, QLocale::ShortFormat);
        const int chipWidth = metrics.horizontalAdvance(chipText) + 14;
        chipRect.setWidth(chipWidth);
        painter->setPen(Qt::NoPen);
        painter->setBrush(overdue ? tokens.danger : tokens.surfaceAlt);
        painter->drawRoundedRect(chipRect, kChipRadius, kChipRadius);
        painter->setPen(overdue ? tokens.accentText : tokens.textMuted);
        painter->drawText(chipRect, Qt::AlignCenter, chipText);
    }

    const auto priority = static_cast<Domain::Priority>(
        index.data(Models::TopicListModel::PriorityRole).toInt());
    const QString priorityText = priorityMark(priority);
    const int priorityWidth = metrics.horizontalAdvance(priorityText);
    priorityRect.setLeft(priorityRect.right() - priorityWidth);
    painter->setPen(priority == Domain::Priority::High ? tokens.warning : tokens.textMuted);
    painter->drawText(priorityRect, Qt::AlignRight | Qt::AlignVCenter, priorityText);

    // Progress bar (only with a checklist).
    qreal titleBottom = content.bottom() - chipHeight - kGap;
    if (total > 0) {
        const QRectF bar(content.left(), titleBottom - 6, content.width(), 6);
        painter->setPen(Qt::NoPen);
        painter->setBrush(tokens.surfaceAlt);
        painter->drawRoundedRect(bar, 3, 3);
        const int progress = index.data(Models::TopicListModel::ProgressRole).toInt();
        if (progress > 0) {
            const QRectF fill(bar.left(), bar.top(), bar.width() * progress / 100.0, bar.height());
            painter->setBrush(tokens.accent);
            painter->drawRoundedRect(fill, 3, 3);
        }
        const QString progressText = u"%1/%2"_s.arg(done).arg(total);
        const QRectF progressTextRect(content.left(), bar.top() - metrics.height(),
                                      content.width(), metrics.height());
        painter->setPen(tokens.textMuted);
        painter->drawText(progressTextRect, Qt::AlignRight | Qt::AlignVCenter, progressText);
        titleBottom = progressTextRect.top() - kGap;
    }

    // Title on top, elided to one line.
    const QRectF titleRect(content.left(), content.top(), content.width(),
                           titleBottom - content.top());
    const QFontMetrics titleMetrics(titleFont);
    painter->setPen(tokens.text);
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignTop | Qt::AlignAbsolute | Qt::TextSingleLine,
                      titleMetrics.elidedText(index.data(Qt::DisplayRole).toString(),
                                              Qt::ElideRight, static_cast<int>(titleRect.width())));

    painter->restore();
}

}  // namespace JobPrep::Ui::Delegates
