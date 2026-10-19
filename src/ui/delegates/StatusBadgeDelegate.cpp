#include "ui/delegates/StatusBadgeDelegate.h"
#include <QPainter>
#include <QPainterPath>
#include "domain/Enums.h"
#include "models/ApplicationTableModel.h"
#include "models/TopicListModel.h"
#include "ui/theme/StatusStyle.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Delegates {

StatusBadgeDelegate::StatusBadgeDelegate(Mode mode, QObject* parent)
    : QStyledItemDelegate(parent), m_mode(mode) {}

void StatusBadgeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QString label;
    QColor color;

    if (m_mode == Mode::Topic) {
        const auto status = index.data(JobPrep::Models::TopicListModel::StatusRole).value<JobPrep::Domain::TopicStatus>();
        label = JobPrep::Ui::Theme::StatusStyle::label(status);
        color = JobPrep::Ui::Theme::StatusStyle::color(status);
    } else {
        const auto status = index.data(JobPrep::Models::ApplicationTableModel::StatusRole).value<JobPrep::Domain::ApplicationStatus>();
        label = JobPrep::Ui::Theme::StatusStyle::label(status);
        color = JobPrep::Ui::Theme::StatusStyle::color(status);
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QRect rect = option.rect;
    rect.adjust(4, 4, -4, -4);

    QPainterPath path;
    int radius = 8;
    path.addRoundedRect(rect, radius, radius);

    QColor bg = color;
    bg.setAlphaF(0.15);
    painter->fillPath(path, bg);

    painter->setPen(color);
    QFontMetrics fm(option.font);
    QRect textRect = rect;
    painter->drawText(textRect, Qt::AlignCenter | Qt::AlignVCenter, fm.elidedText(label, Qt::ElideRight, textRect.width()));

    painter->restore();
}

}  // namespace JobPrep::Ui::Delegates
