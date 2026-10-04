#pragma once

#include <QStyledItemDelegate>

namespace JobPrep::Ui::Delegates {

class StatusBadgeDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit StatusBadgeDelegate(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

}  // namespace JobPrep::Ui::Delegates
